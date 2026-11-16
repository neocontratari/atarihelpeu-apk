package eu.atarihelp.emu10;

import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothClass;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothManager;
import android.bluetooth.BluetoothProfile;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.provider.Settings;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedHashMap;
import java.util.List;

/**
 * B331: BLUETOOTH OVLADAC PRIMO V APPCE (OPTIONS -> HERNI OVLADAC -> PRIPOJIT OVLADAC (BLUETOOTH)).
 *
 * Postup: povoleni (Android 12+ "Zarizeni v okoli", 6-11 poloha - Android ji chce pro hledani Bluetooth),
 * zapnuti Bluetooth (systemove okno), hledani (~12 s, Android), seznam pro stranku (ovladace nahore),
 * klepnuti = createBond (parovani), pak se ovladac pripoji sam (NapOvladac ho uvidi jako vstupni zarizeni).
 * Parovani si pamatuje Android: priste staci ovladac zapnout. Do logu jen ovladace a konec adresy
 * (jmena cizich zarizeni v okoli se nezapisuji). Texty a rozhodovani: NapBluetoothTexty (testy v JVM).
 */
final class NapBluetooth {

    static final int REQ_POVOLENI = 31, REQ_ZAPNUTI = 32;
    private static final int MAX_NALEZENYCH = 60;

    private final Activity a;
    private final NapOvladac ovladac;
    private final NapAktualizace.Hlaska log;
    private final Handler ui = new Handler(Looper.getMainLooper());

    // stav - pristup jen pod zamkem (stranka se pta z vlakna JS rozhrani)
    private String faze = NapBluetoothTexty.F_NIC, chyba = "";
    private final LinkedHashMap<String, NapBluetoothTexty.Nalez> nalezene = new LinkedHashMap<>();
    private String parujiAdresa = null, parujiJmeno = "";
    private long konecHledaniMs = 0;
    private boolean prijimacZapnut = false, pripojenoHlaseno = false;
    private int ovladacuHlaseno = 0;

    // ===== B332: zapamatovany ovladac (NapBluetoothTexty: proc appka od Androidu 9 sama nepripoji) =====
    /** HID profil Androidu: verejne getConnectionState / getConnectedDevices (kdo je opravdu pripojeny). */
    static final int PROFIL_HID = 4;   // BluetoothProfile.HID_HOST (konstanta neni v SDK, profil ano)
    static final String AKCE_HID = "android.bluetooth.input.profile.action.CONNECTION_STATE_CHANGED";
    private volatile BluetoothProfile hid;
    private final HashMap<String, Long> aclOd = new HashMap<>();     // adresa -> kdy se ozval (pod zamkem)
    private final HashSet<String> ozvalHlaseno = new HashSet<>();    // "ozval se, ale nepripojil" uz hlaseno
    private String cekaPripojit = null;                               // PRIPOJIT ceka na povoleni / zapnuti
    private String pripojujiAdresa = null;
    private long pripojujiOd = 0;
    private boolean vysledekHlasen = false, hlidacZapnut = false;

    NapBluetooth(Activity a, NapOvladac ovladac, NapAktualizace.Hlaska log) {
        this.a = a;
        this.ovladac = ovladac;
        this.log = log;
        try { spustHlidac(); } catch (Throwable t) { log("hlidac nejde spustit: " + t); }
    }

    private void log(String s) { try { if (log != null) log.rekni("B331 BT: " + s); } catch (Throwable ignored) {} }
    private void log332(String s) { try { if (log != null) log.rekni("B332 BT: " + s); } catch (Throwable ignored) {} }

    // ================= B332: hlidac zapamatovanych ovladacu (po celou dobu behu appky) =================

    private void spustHlidac() {
        final BluetoothAdapter ad = adapter();
        if (ad == null) return;
        final IntentFilter f = new IntentFilter();
        f.addAction(BluetoothDevice.ACTION_ACL_CONNECTED);
        f.addAction(BluetoothDevice.ACTION_ACL_DISCONNECTED);
        f.addAction(AKCE_HID);
        f.addAction(BluetoothAdapter.ACTION_STATE_CHANGED);
        // zpravy posila jen system (chranene) - Android 13+ chce priznak, odkud smi prijit
        if (Build.VERSION.SDK_INT >= 33) a.registerReceiver(hlidac, f, 2 /* Context.RECEIVER_EXPORTED */);
        else a.registerReceiver(hlidac, f);
        synchronized (this) { hlidacZapnut = true; }
        final boolean proxy = ad.getProfileProxy(a, new BluetoothProfile.ServiceListener() {
            @Override public void onServiceConnected(int profil, BluetoothProfile p) {
                hid = p;
                log332("HID profil Androidu pripraven - zapamatovane ovladace: " + zapamatovaneLog());
            }
            @Override public void onServiceDisconnected(int profil) { hid = null; }
        }, PROFIL_HID);
        if (!proxy) log332("HID profil Androidu neni k dispozici - stav ovladacu jen podle vstupnich zarizeni");
    }

    /** Konec appky (onDestroy, po zastav()): hlidac a HID profil. */
    void konec() {
        boolean byl;
        synchronized (this) { byl = hlidacZapnut; hlidacZapnut = false; }
        if (byl) try { a.unregisterReceiver(hlidac); } catch (Throwable ignored) {}
        final BluetoothProfile p = hid;
        hid = null;
        if (p != null) try { final BluetoothAdapter ad = adapter(); if (ad != null) ad.closeProfileProxy(PROFIL_HID, p); } catch (Throwable ignored) {}
    }

    private int hidStav(BluetoothDevice d) {
        final BluetoothProfile h = hid;
        if (h == null || d == null) return NapBluetoothTexty.HID_NEVI;
        try { return h.getConnectionState(d); } catch (Throwable t) { return NapBluetoothTexty.HID_NEVI; }
    }

    private static String jmeno(BluetoothDevice d) {
        try { final String j = d.getName(); return j == null ? "" : j; } catch (Throwable t) { return ""; }
    }

    private static int trida(BluetoothDevice d) {
        try { return d.getBluetoothClass() != null ? d.getBluetoothClass().getDeviceClass() : -1; } catch (Throwable t) { return -1; }
    }

    /** Sparovany herni ovladac (jen o nich hlidac pise do logu). */
    private static boolean zapamatovanyOvladac(BluetoothDevice d) {
        try { return d.getBondState() == BluetoothDevice.BOND_BONDED && NapBluetoothTexty.jeOvladac(trida(d), jmeno(d)); }
        catch (Throwable t) { return false; }
    }

    private String zapamatovaneLog() {
        final StringBuilder b = new StringBuilder();
        try {
            final BluetoothAdapter ad = adapter();
            if (ad == null) return "?";
            if (Build.VERSION.SDK_INT >= 31 && a.checkSelfPermission("android.permission.BLUETOOTH_CONNECT") != PackageManager.PERMISSION_GRANTED)
                return "? (bez povoleni Zarizeni v okoli)";
            for (BluetoothDevice d : ad.getBondedDevices()) {
                if (!zapamatovanyOvladac(d)) continue;
                b.append(b.length() > 0 ? ", " : "").append('"').append(jmeno(d)).append("\" ").append(NapBluetoothTexty.hidLog(hidStav(d)));
            }
        } catch (Throwable t) { return "? (" + t.getClass().getSimpleName() + ")"; }
        return b.length() == 0 ? "zadny" : b.toString();
    }

    private final BroadcastReceiver hlidac = new BroadcastReceiver() {
        @Override
        public void onReceive(Context c, Intent i) {
            try {
                final String akce = i.getAction();
                if (BluetoothAdapter.ACTION_STATE_CHANGED.equals(akce)) {
                    final int st = i.getIntExtra(BluetoothAdapter.EXTRA_STATE, -1);
                    if (st == BluetoothAdapter.STATE_ON) log332("Bluetooth ZAPNUTO");
                    else if (st == BluetoothAdapter.STATE_OFF) {
                        log332("Bluetooth VYPNUTO");
                        synchronized (NapBluetooth.this) { aclOd.clear(); }
                    }
                    return;
                }
                final BluetoothDevice d = i.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);
                if (d == null || !zapamatovanyOvladac(d)) return;
                final String adr = d.getAddress(), jm = jmeno(d), kde = "\"" + jm + "\" (" + NapBluetoothTexty.kratkaAdresa(adr) + ")";
                if (BluetoothDevice.ACTION_ACL_CONNECTED.equals(akce)) {
                    synchronized (NapBluetooth.this) { aclOd.put(adr, System.currentTimeMillis()); ozvalHlaseno.remove(adr); }
                    log332(kde + " se ozval (spojeni Bluetooth ANO), jako ovladac: " + NapBluetoothTexty.hidLog(hidStav(d)));
                    ui.postDelayed(() -> kontrolaOzvani(d), NapBluetoothTexty.LIMIT_OZVAL_MS + 500L);
                } else if (BluetoothDevice.ACTION_ACL_DISCONNECTED.equals(akce)) {
                    synchronized (NapBluetooth.this) { aclOd.remove(adr); }
                    log332(kde + " spojeni Bluetooth NE (vypnuty nebo mimo dosah)");
                } else if (AKCE_HID.equals(akce)) {
                    final int st = i.getIntExtra(BluetoothProfile.EXTRA_STATE, -1), pred = i.getIntExtra(BluetoothProfile.EXTRA_PREVIOUS_STATE, -1);
                    log332(kde + " jako ovladac: " + NapBluetoothTexty.hidLog(pred) + " -> " + NapBluetoothTexty.hidLog(st));
                }
            } catch (Throwable t) {
                log332("hlidac chyba: " + t);
            }
        }
    };

    /** 10 s po ozvani: porad spojeny, ale ne jako ovladac -> zprava (jednou za ozvani) a log. */
    private void kontrolaOzvani(BluetoothDevice d) {
        try {
            final String adr = d.getAddress(), jm = jmeno(d);
            synchronized (this) {
                if (!aclOd.containsKey(adr) || ozvalHlaseno.contains(adr)) return;
            }
            final int h = hidStav(d);
            final boolean vstup = ovladac != null && mezi(ovladac.jmenaPripojenych(), jm);
            if (vstup || h == NapBluetoothTexty.HID_PRIPOJEN || h == NapBluetoothTexty.HID_PRIPOJUJE) return;
            synchronized (this) { ozvalHlaseno.add(adr); }
            log332("\"" + jm + "\" (" + NapBluetoothTexty.kratkaAdresa(adr) + ") se ozval, ale za 10 s se jako ovladac nepripojil (stav "
                    + NapBluetoothTexty.hidLog(h) + ") - radim vypnout a zapnout");
            android.widget.Toast.makeText(a, NapBluetoothTexty.zpravaOzvalSe(en(), jm), android.widget.Toast.LENGTH_LONG).show();
        } catch (Throwable t) {
            log332("kontrola ozvani chyba: " + t);
        }
    }

    // ================= B332: PRIPOJIT u zapamatovaneho ovladace (OPTIONS) =================

    /** Klepnuti na PRIPOJIT u zapamatovaneho ovladace. */
    void pripoj(final String adresa) { ui.post(() -> pripojUi(adresa)); }

    private void pripojUi(String adresa) {
        try {
            final BluetoothAdapter ad = adapter();
            if (ad == null) { nastav(NapBluetoothTexty.F_NENI_BT, ""); return; }
            if (adresa == null || !BluetoothAdapter.checkBluetoothAddress(adresa)) return;
            // Android 12+: pripojeni potrebuje jen "Zarizeni v okoli" (BLUETOOTH_CONNECT), hledat netreba
            if (Build.VERSION.SDK_INT >= 31 && a.checkSelfPermission("android.permission.BLUETOOTH_CONNECT") != PackageManager.PERMISSION_GRANTED) {
                synchronized (this) { cekaPripojit = adresa; }
                nastav(NapBluetoothTexty.F_POVOLENI, "");
                log332("PRIPOJIT: zadam o povoleni BLUETOOTH_CONNECT");
                a.requestPermissions(new String[]{"android.permission.BLUETOOTH_CONNECT"}, REQ_POVOLENI);
                return;
            }
            if (!ad.isEnabled()) {
                synchronized (this) { cekaPripojit = adresa; }
                nastav(NapBluetoothTexty.F_ZAPINANI, "");
                log332("PRIPOJIT: Bluetooth vypnute - Android se zepta na zapnuti");
                a.startActivityForResult(new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE), REQ_ZAPNUTI);
                return;
            }
            if (ad.isDiscovering()) ad.cancelDiscovery();
            final BluetoothDevice d = ad.getRemoteDevice(adresa);
            final String jm = jmeno(d);
            synchronized (this) {
                cekaPripojit = null;
                parujiAdresa = adresa; parujiJmeno = jm; pripojenoHlaseno = false;
                pripojujiAdresa = adresa; pripojujiOd = System.currentTimeMillis(); vysledekHlasen = false;
            }
            nastav(NapBluetoothTexty.F_PRIPOJUJI, "");
            final String jak = NapBluetoothTexty.appkaSmiPripojit(Build.VERSION.SDK_INT)
                    ? "appka pripojuje (Android " + Build.VERSION.RELEASE + "): " + pripojSkryte(d)
                    : "Android " + Build.VERSION.RELEASE + " aplikacim pripojit nedovoli (umi to jen Nastaveni) - cekam 30 s, az se ovladac ozve sam (HOME)";
            log332("PRIPOJIT \"" + jm + "\" (" + NapBluetoothTexty.kratkaAdresa(adresa) + ", jako ovladac " + NapBluetoothTexty.hidLog(hidStav(d)) + "): " + jak);
        } catch (SecurityException se) {
            nastav(NapBluetoothTexty.F_ODMITNUTO, "");
            log332("PRIPOJIT: chybi povoleni: " + se.getMessage());
        } catch (Throwable t) {
            nastav(NapBluetoothTexty.F_NEPOVEDLO, "");
            log332("PRIPOJIT chyba: " + t);
        }
    }

    /** Jen Android 7 a 8: BluetoothHidHost.connect (skryte, opravneni BLUETOOTH_ADMIN). Od Androidu 9 ho Android aplikacim nepusti. */
    private String pripojSkryte(BluetoothDevice d) {
        final BluetoothProfile h = hid;
        if (h == null) return "HID profil jeste neni pripraveny";
        try {
            return "connect = " + h.getClass().getMethod("connect", BluetoothDevice.class).invoke(h, d);
        } catch (Throwable t) {
            return "nejde (" + t.getClass().getSimpleName() + ")";
        }
    }

    private boolean en() { try { return NapJazyk.en(a); } catch (Throwable t) { return false; } }

    private BluetoothAdapter adapter() {
        try {
            final BluetoothManager m = (BluetoothManager) a.getSystemService(Context.BLUETOOTH_SERVICE);
            return m != null ? m.getAdapter() : null;
        } catch (Throwable t) { return null; }
    }

    private synchronized void nastav(String f, String ch) { faze = f; chyba = ch == null ? "" : ch; }

    private String[] chybejiciPovoleni() {
        final ArrayList<String> r = new ArrayList<>();
        if (Build.VERSION.SDK_INT >= 23)
            for (String p : NapBluetoothTexty.povoleni(Build.VERSION.SDK_INT))
                if (a.checkSelfPermission(p) != PackageManager.PERMISSION_GRANTED) r.add(p);
        return r.toArray(new String[0]);
    }

    private boolean polohaZapnuta() {
        try {
            final android.location.LocationManager lm = (android.location.LocationManager) a.getSystemService(Context.LOCATION_SERVICE);
            return lm == null || Build.VERSION.SDK_INT < 28 || lm.isLocationEnabled();
        } catch (Throwable t) { return true; }
    }

    // ================= ovladani ze stranky OPTIONS (JS rozhrani AHBT) =================

    /** PRIPOJIT OVLADAC: povoleni -> zapnuti Bluetooth -> hledani. */
    void hledej() { ui.post(this::hledejUi); }

    private void hledejUi() {
        synchronized (this) { cekaPripojit = null; pripojujiAdresa = null; }   // B332: hledani ukonci cekani PRIPOJIT
        try {
            final BluetoothAdapter ad = adapter();
            if (ad == null) { nastav(NapBluetoothTexty.F_NENI_BT, ""); log("telefon nema Bluetooth"); return; }
            final String[] chybi = chybejiciPovoleni();
            if (chybi.length > 0) {
                nastav(NapBluetoothTexty.F_POVOLENI, "");
                final StringBuilder co = new StringBuilder();
                for (String p : chybi) co.append(co.length() > 0 ? ", " : "").append(p.replace("android.permission.", ""));
                log("zadam o povoleni (Android " + Build.VERSION.RELEASE + "): " + co);
                a.requestPermissions(chybi, REQ_POVOLENI);
                return;
            }
            if (!ad.isEnabled()) {
                nastav(NapBluetoothTexty.F_ZAPINANI, "");
                log("Bluetooth vypnute - Android se zepta na zapnuti");
                a.startActivityForResult(new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE), REQ_ZAPNUTI);
                return;
            }
            if (NapBluetoothTexty.potrebujePolohuZapnutou(Build.VERSION.SDK_INT) && !polohaZapnuta()) {
                nastav(NapBluetoothTexty.F_POLOHA_VYPNUTA, "");
                log("Poloha vypnuta (Android " + Build.VERSION.RELEASE + " bez ni nehleda)");
                return;
            }
            zapniPrijimac();
            if (ad.isDiscovering()) ad.cancelDiscovery();
            synchronized (this) { nalezene.clear(); ovladacuHlaseno = 0; }
            if (!ad.startDiscovery()) {
                nastav(NapBluetoothTexty.F_CHYBA, "startDiscovery");
                log("hledani nejde spustit (startDiscovery = false)");
                return;
            }
            synchronized (this) { konecHledaniMs = System.currentTimeMillis() + 13000L; }
            nastav(NapBluetoothTexty.F_HLEDAM, "");
            log("hledam ovladace (Android " + Build.VERSION.RELEASE + ", API " + Build.VERSION.SDK_INT + ")");
        } catch (SecurityException se) {
            nastav(NapBluetoothTexty.F_ODMITNUTO, "");
            log("chybi povoleni: " + se.getMessage());
        } catch (Throwable t) {
            nastav(NapBluetoothTexty.F_CHYBA, t.getClass().getSimpleName());
            log("chyba hledani: " + t);
        }
    }

    /** Vysledek okna povoleni (MainActivity.onRequestPermissionsResult). */
    void povoleniVysledek(int[] vysledky) {
        boolean vse = vysledky != null && vysledky.length > 0;
        if (vysledky != null) for (int v : vysledky) vse &= v == PackageManager.PERMISSION_GRANTED;
        // poloha na Androidu 6-11: staci FINE (COARSE muze clovek odmitnout jen spolu s ni)
        if (!vse && chybejiciPovoleni().length == 0) vse = true;
        log("povoleni " + (vse ? "UDELENO" : "ODMITNUTO"));
        String p;
        synchronized (this) { p = cekaPripojit; if (!vse) cekaPripojit = null; }
        if (vse && p != null) pripojUi(p);      // B332: povoleni chtel PRIPOJIT (zapamatovany ovladac)
        else if (vse) hledejUi();
        else nastav(NapBluetoothTexty.F_ODMITNUTO, "");
    }

    /** Vysledek okna "zapnout Bluetooth" (MainActivity.onActivityResult). */
    void zapnutiVysledek(int vysledek) {
        final BluetoothAdapter ad = adapter();
        final boolean zapnuto = ad != null && ad.isEnabled();
        log("Bluetooth " + (zapnuto ? "ZAPNUTO" : "zustalo vypnute") + " (vysledek okna " + vysledek + ")");
        String p;
        synchronized (this) { p = cekaPripojit; if (!zapnuto) cekaPripojit = null; }
        if (zapnuto && p != null) pripojUi(p);  // B332: zapnuti chtel PRIPOJIT (zapamatovany ovladac)
        else if (zapnuto) hledejUi();
        else nastav(NapBluetoothTexty.F_BT_VYPNUTE, "");
    }

    /** Klepnuti na zarizeni v seznamu: sparovat (createBond). */
    void sparuj(final String adresa) {
        ui.post(() -> {
            try {
                final BluetoothAdapter ad = adapter();
                if (ad == null || adresa == null || !BluetoothAdapter.checkBluetoothAddress(adresa)) return;
                if (ad.isDiscovering()) ad.cancelDiscovery();
                final BluetoothDevice d = ad.getRemoteDevice(adresa);
                String jm;
                synchronized (this) {
                    final NapBluetoothTexty.Nalez n = nalezene.get(adresa);
                    jm = n != null && n.jmeno != null ? n.jmeno : "";
                }
                if (jm.isEmpty()) { try { jm = String.valueOf(d.getName()); } catch (Throwable ignored) { jm = ""; } }
                synchronized (this) { parujiAdresa = adresa; parujiJmeno = jm; pripojenoHlaseno = false; }
                if (d.getBondState() == BluetoothDevice.BOND_BONDED) {
                    nastav(NapBluetoothTexty.F_SPAROVANO, "");
                    log("\"" + jm + "\" (" + NapBluetoothTexty.kratkaAdresa(adresa) + ") uz je sparovany - staci ho zapnout");
                    return;
                }
                nastav(NapBluetoothTexty.F_PARUJI, "");
                final boolean ok = d.createBond();
                log("paruji \"" + jm + "\" (" + NapBluetoothTexty.kratkaAdresa(adresa) + "): createBond = " + ok);
                if (!ok) nastav(NapBluetoothTexty.F_NEPOVEDLO, "");
            } catch (Throwable t) {
                nastav(NapBluetoothTexty.F_NEPOVEDLO, "");
                log("parovani chyba: " + t);
            }
        });
    }

    /** Zavreni OPTIONS / konec appky: prestat hledat a poslouchat (parovani, ktere bezi, dobehne v Androidu). */
    void zastav() {
        if (Looper.myLooper() == Looper.getMainLooper()) zastavUi();   // onDestroy: hned (jinak by prijimac "unikl")
        else ui.post(this::zastavUi);
    }

    private void zastavUi() {
        try {
            final BluetoothAdapter ad = adapter();
            if (ad != null && ad.isDiscovering()) ad.cancelDiscovery();
        } catch (Throwable ignored) {}
        vypniPrijimac();
        synchronized (this) {
            if (NapBluetoothTexty.F_HLEDAM.equals(faze) || NapBluetoothTexty.F_HOTOVO.equals(faze)) faze = NapBluetoothTexty.F_NIC;
        }
    }

    void otevriNastaveni() {
        ui.post(() -> { try { a.startActivity(new Intent(Settings.ACTION_BLUETOOTH_SETTINGS)); log("otevreno nastaveni Bluetooth Androidu"); } catch (Throwable t) { log("nastaveni nejde otevrit: " + t); } });
    }

    void otevriPolohu() {
        ui.post(() -> { try { a.startActivity(new Intent(Settings.ACTION_LOCATION_SOURCE_SETTINGS)); } catch (Throwable t) { log("nastaveni polohy nejde otevrit: " + t); } });
    }

    // ================= prijem zprav Androidu =================

    private final BroadcastReceiver prijimac = new BroadcastReceiver() {
        @Override
        public void onReceive(Context c, Intent i) {
            try {
                final String akce = i.getAction();
                if (BluetoothDevice.ACTION_FOUND.equals(akce) || BluetoothDevice.ACTION_NAME_CHANGED.equals(akce)
                        || BluetoothDevice.ACTION_CLASS_CHANGED.equals(akce)) {
                    final BluetoothDevice d = i.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);
                    final BluetoothClass k = i.getParcelableExtra(BluetoothDevice.EXTRA_CLASS);
                    pridej(d, i.getStringExtra(BluetoothDevice.EXTRA_NAME), k,
                            i.getShortExtra(BluetoothDevice.EXTRA_RSSI, Short.MIN_VALUE));
                } else if (BluetoothAdapter.ACTION_DISCOVERY_FINISHED.equals(akce)) {
                    hledaniSkoncilo();
                } else if (BluetoothDevice.ACTION_BOND_STATE_CHANGED.equals(akce)) {
                    final BluetoothDevice d = i.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);
                    zmenaParovani(d, i.getIntExtra(BluetoothDevice.EXTRA_BOND_STATE, BluetoothDevice.ERROR),
                            i.getIntExtra(BluetoothDevice.EXTRA_PREVIOUS_BOND_STATE, BluetoothDevice.ERROR));
                }
            } catch (Throwable t) {
                log("zprava Androidu chyba: " + t);
            }
        }
    };

    private void zapniPrijimac() {
        synchronized (this) { if (prijimacZapnut) return; prijimacZapnut = true; }
        final IntentFilter f = new IntentFilter();
        f.addAction(BluetoothDevice.ACTION_FOUND);
        f.addAction(BluetoothDevice.ACTION_NAME_CHANGED);
        f.addAction(BluetoothDevice.ACTION_CLASS_CHANGED);
        f.addAction(BluetoothAdapter.ACTION_DISCOVERY_FINISHED);
        f.addAction(BluetoothDevice.ACTION_BOND_STATE_CHANGED);
        a.registerReceiver(prijimac, f);
    }

    private void vypniPrijimac() {
        synchronized (this) { if (!prijimacZapnut) return; prijimacZapnut = false; }
        try { a.unregisterReceiver(prijimac); } catch (Throwable ignored) {}
    }

    private void pridej(BluetoothDevice d, String jmenoZpravy, BluetoothClass k, short rssi) {
        if (d == null) return;
        final String adr = d.getAddress();
        String jm = jmenoZpravy;
        if (jm == null || jm.isEmpty()) { try { jm = d.getName(); } catch (Throwable ignored) {} }
        int trida = -1;
        try { trida = k != null ? k.getDeviceClass() : (d.getBluetoothClass() != null ? d.getBluetoothClass().getDeviceClass() : -1); } catch (Throwable ignored) {}
        boolean sparovany = false;
        try { sparovany = d.getBondState() == BluetoothDevice.BOND_BONDED; } catch (Throwable ignored) {}
        boolean novyOvladac = false;
        synchronized (this) {
            NapBluetoothTexty.Nalez n = nalezene.get(adr);
            if (n == null) {
                if (nalezene.size() >= MAX_NALEZENYCH) return;
                n = new NapBluetoothTexty.Nalez(adr, jm, trida, rssi == Short.MIN_VALUE ? Integer.MIN_VALUE : rssi, sparovany);
                nalezene.put(adr, n);
                novyOvladac = n.ovladac();
            } else {
                final boolean byl = n.ovladac();
                if (jm != null && !jm.isEmpty()) n.jmeno = jm;
                if (trida >= 0) n.trida = trida;
                if (rssi != Short.MIN_VALUE) n.sila = rssi;
                n.sparovany = sparovany;
                novyOvladac = !byl && n.ovladac();
            }
            if (novyOvladac) ovladacuHlaseno++;
        }
        if (novyOvladac)   // do logu jen ovladace (jmena cizich zarizeni v okoli ne)
            log("nalezen ovladac \"" + jm + "\" (trida 0x" + Integer.toHexString(Math.max(trida, 0)) + ", signal "
                    + (rssi == Short.MIN_VALUE ? "?" : rssi + " dBm") + ", " + NapBluetoothTexty.kratkaAdresa(adr)
                    + (sparovany ? ", uz sparovany" : "") + ")");
    }

    private void hledaniSkoncilo() {
        int vse, ovl;
        synchronized (this) {
            vse = nalezene.size();
            ovl = 0;
            for (NapBluetoothTexty.Nalez n : nalezene.values()) if (n.ovladac()) ovl++;
            if (NapBluetoothTexty.F_HLEDAM.equals(faze)) faze = NapBluetoothTexty.F_HOTOVO;
        }
        log("hledani skoncilo: zarizeni " + vse + ", z toho ovladacu " + ovl);
    }

    private void zmenaParovani(BluetoothDevice d, int stav, int predtim) {
        if (d == null) return;
        final String adr = d.getAddress();
        String jm;
        synchronized (this) {
            final NapBluetoothTexty.Nalez n = nalezene.get(adr);
            if (n != null) n.sparovany = stav == BluetoothDevice.BOND_BONDED;
            if (parujiAdresa == null || !parujiAdresa.equals(adr)) return;
            jm = parujiJmeno;
        }
        if (stav == BluetoothDevice.BOND_BONDING) {
            nastav(NapBluetoothTexty.F_PARUJI, "");
        } else if (stav == BluetoothDevice.BOND_BONDED) {
            nastav(NapBluetoothTexty.F_SPAROVANO, "");
            log("\"" + jm + "\" SPAROVANO (" + NapBluetoothTexty.kratkaAdresa(adr) + ") - ceka se, az se pripoji jako ovladac");
        } else if (stav == BluetoothDevice.BOND_NONE && predtim == BluetoothDevice.BOND_BONDING) {
            nastav(NapBluetoothTexty.F_NEPOVEDLO, "");
            log("parovani \"" + jm + "\" se NEPOVEDLO (" + NapBluetoothTexty.kratkaAdresa(adr) + ")");
        }
    }

    // ================= stav pro stranku =================

    private static boolean mezi(List<String> jmena, String jm) {
        if (jm == null || jm.isEmpty()) return false;
        for (String s : jmena) if (s != null && s.trim().equalsIgnoreCase(jm.trim())) return true;
        return false;
    }

    /** JSON pro stranku OPTIONS (vola se z vlakna JS rozhrani). */
    String stav() {
        final boolean en = en();
        final List<String> pripojene = ovladac != null ? ovladac.jmenaPripojenych() : new ArrayList<>();
        // zapamatovane ovladace (sparovane v Androidu) - Android 12+ jen s povolenim "Zarizeni v okoli"
        final ArrayList<NapBluetoothTexty.Nalez> zapamatovane = new ArrayList<>();
        try {
            final BluetoothAdapter ad = adapter();
            if (ad != null && (Build.VERSION.SDK_INT < 31
                    || a.checkSelfPermission("android.permission.BLUETOOTH_CONNECT") == PackageManager.PERMISSION_GRANTED)) {
                for (BluetoothDevice d : ad.getBondedDevices()) {
                    String jm = null;
                    int trida = -1;
                    try { jm = d.getName(); } catch (Throwable ignored) {}
                    try { if (d.getBluetoothClass() != null) trida = d.getBluetoothClass().getDeviceClass(); } catch (Throwable ignored) {}
                    final int h = hidStav(d);   // B332: stav v HID profilu Androidu
                    final boolean pripojen = mezi(pripojene, jm) || h == NapBluetoothTexty.HID_PRIPOJEN;
                    if (!NapBluetoothTexty.jeOvladac(trida, jm) && !pripojen) continue;
                    final NapBluetoothTexty.Nalez n = new NapBluetoothTexty.Nalez(d.getAddress(), jm, trida, Integer.MIN_VALUE, true);
                    n.pripojeny = pripojen;
                    n.hid = h;
                    zapamatovane.add(n);
                }
            }
        } catch (Throwable ignored) {}
        // B332: cekani po PRIPOJIT - stav ovladace v Androidu a jestli se ozval (hlidac)
        int hidPripojuji = NapBluetoothTexty.HID_NEVI;
        String adrPripojuji;
        synchronized (this) { adrPripojuji = pripojujiAdresa; }
        if (adrPripojuji != null) {
            try { final BluetoothAdapter ad = adapter(); if (ad != null) hidPripojuji = hidStav(ad.getRemoteDevice(adrPripojuji)); } catch (Throwable ignored) {}
        }
        String f, jm, ch;
        int vse, ovl, zbyva;
        final ArrayList<NapBluetoothTexty.Nalez> seznam;
        boolean hlasPripojeni = false;
        String vysledek332 = null;
        synchronized (this) {
            // sparovany ovladac se pripojil (Android ho uz posila jako vstupni zarizeni)
            if ((NapBluetoothTexty.F_SPAROVANO.equals(faze) || NapBluetoothTexty.F_PARUJI.equals(faze)) && mezi(pripojene, parujiJmeno)) {
                faze = NapBluetoothTexty.F_PRIPOJENO;
                if (!pripojenoHlaseno) { pripojenoHlaseno = true; hlasPripojeni = true; }
            }
            final long ted = System.currentTimeMillis();
            final boolean cekam = NapBluetoothTexty.F_PRIPOJUJI.equals(faze) || NapBluetoothTexty.F_OZVAL_SE.equals(faze)
                    || NapBluetoothTexty.F_NEOZVAL.equals(faze);
            if (pripojujiAdresa != null && cekam) {
                final Long acl = aclOd.get(pripojujiAdresa);
                final String nova = NapBluetoothTexty.hlidani(ted, pripojujiOd, acl == null ? 0L : acl, hidPripojuji, mezi(pripojene, parujiJmeno));
                if (!nova.equals(faze)) {
                    faze = nova;
                    if (NapBluetoothTexty.F_PRIPOJENO.equals(nova)) vysledek332 = nova;   // uspech vzdy (pak se uz neceka)
                    else if (!NapBluetoothTexty.F_PRIPOJUJI.equals(nova) && !vysledekHlasen) { vysledekHlasen = true; vysledek332 = nova; }
                }
                if (NapBluetoothTexty.F_PRIPOJENO.equals(nova)) { pripojujiAdresa = null; pripojenoHlaseno = true; }
            }
            for (NapBluetoothTexty.Nalez n : nalezene.values()) n.pripojeny = mezi(pripojene, n.jmeno);
            seznam = new ArrayList<>(nalezene.values());
            f = faze; jm = parujiJmeno; ch = chyba;
            vse = nalezene.size();
            ovl = 0;
            for (NapBluetoothTexty.Nalez n : seznam) if (n.ovladac()) ovl++;
            zbyva = NapBluetoothTexty.F_PRIPOJUJI.equals(faze) ? NapBluetoothTexty.zbyva(ted, pripojujiOd)
                    : (int) Math.max(0L, (konecHledaniMs - ted + 999L) / 1000L);
        }
        if (hlasPripojeni) log("\"" + jm + "\" PRIPOJEN jako herni ovladac - hotovo");
        if (vysledek332 != null)
            log332("PRIPOJIT \"" + jm + "\": " + (NapBluetoothTexty.F_PRIPOJENO.equals(vysledek332) ? "PRIPOJEN jako herni ovladac - hotovo"
                    : NapBluetoothTexty.F_OZVAL_SE.equals(vysledek332) ? "ozval se, ale jako ovladac se nepripojil (stav " + NapBluetoothTexty.hidLog(hidPripojuji) + ")"
                    : "za 30 s se neozval (vypnuty, vybity, jiny rezim ovladace?)"));
        final String text = NapBluetoothTexty.text(en, f, Build.VERSION.SDK_INT, jm, vse - ovl, ovl, zbyva, ch);
        return NapBluetoothTexty.json(en, f, text, seznam, zapamatovane);
    }

    /** Pro log pri startu: kolik je v Androidu zapamatovanych ovladacu (bez povoleni na Androidu 12+ nevime). */
    String souhrn() {
        try {
            final BluetoothAdapter ad = adapter();
            if (ad == null) return "telefon nema Bluetooth";
            if (Build.VERSION.SDK_INT >= 31 && a.checkSelfPermission("android.permission.BLUETOOTH_CONNECT") != PackageManager.PERMISSION_GRANTED)
                return "Bluetooth " + (ad.isEnabled() ? "zapnute" : "vypnute") + ", zapamatovane ovladace: ? (bez povoleni Zarizeni v okoli)";
            int n = 0;
            final StringBuilder b = new StringBuilder();
            for (BluetoothDevice d : ad.getBondedDevices()) {
                String jm = null;
                int trida = -1;
                try { jm = d.getName(); } catch (Throwable ignored) {}
                try { if (d.getBluetoothClass() != null) trida = d.getBluetoothClass().getDeviceClass(); } catch (Throwable ignored) {}
                if (!NapBluetoothTexty.jeOvladac(trida, jm)) continue;
                if (n++ > 0) b.append(", ");
                b.append(jm);
            }
            return "Bluetooth " + (ad.isEnabled() ? "zapnute" : "vypnute") + ", zapamatovane ovladace: " + (n == 0 ? "zadny" : b.toString());
        } catch (Throwable t) {
            return "Bluetooth stav: " + t.getClass().getSimpleName();
        }
    }
}
