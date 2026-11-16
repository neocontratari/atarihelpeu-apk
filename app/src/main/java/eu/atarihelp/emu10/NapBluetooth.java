package eu.atarihelp.emu10;

import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothClass;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothManager;
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

    NapBluetooth(Activity a, NapOvladac ovladac, NapAktualizace.Hlaska log) {
        this.a = a;
        this.ovladac = ovladac;
        this.log = log;
    }

    private void log(String s) { try { if (log != null) log.rekni("B331 BT: " + s); } catch (Throwable ignored) {} }

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
        if (vse) hledejUi();
        else nastav(NapBluetoothTexty.F_ODMITNUTO, "");
    }

    /** Vysledek okna "zapnout Bluetooth" (MainActivity.onActivityResult). */
    void zapnutiVysledek(int vysledek) {
        final BluetoothAdapter ad = adapter();
        final boolean zapnuto = ad != null && ad.isEnabled();
        log("Bluetooth " + (zapnuto ? "ZAPNUTO" : "zustalo vypnute") + " (vysledek okna " + vysledek + ")");
        if (zapnuto) hledejUi();
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
                    final boolean pripojen = mezi(pripojene, jm);
                    if (!NapBluetoothTexty.jeOvladac(trida, jm) && !pripojen) continue;
                    final NapBluetoothTexty.Nalez n = new NapBluetoothTexty.Nalez(d.getAddress(), jm, trida, Integer.MIN_VALUE, true);
                    n.pripojeny = pripojen;
                    zapamatovane.add(n);
                }
            }
        } catch (Throwable ignored) {}
        String f, jm, ch;
        int vse, ovl, zbyva;
        final ArrayList<NapBluetoothTexty.Nalez> seznam;
        boolean hlasPripojeni = false;
        synchronized (this) {
            // sparovany ovladac se pripojil (Android ho uz posila jako vstupni zarizeni)
            if ((NapBluetoothTexty.F_SPAROVANO.equals(faze) || NapBluetoothTexty.F_PARUJI.equals(faze)) && mezi(pripojene, parujiJmeno)) {
                faze = NapBluetoothTexty.F_PRIPOJENO;
                if (!pripojenoHlaseno) { pripojenoHlaseno = true; hlasPripojeni = true; }
            }
            for (NapBluetoothTexty.Nalez n : nalezene.values()) n.pripojeny = mezi(pripojene, n.jmeno);
            seznam = new ArrayList<>(nalezene.values());
            f = faze; jm = parujiJmeno; ch = chyba;
            vse = nalezene.size();
            ovl = 0;
            for (NapBluetoothTexty.Nalez n : seznam) if (n.ovladac()) ovl++;
            zbyva = (int) Math.max(0L, (konecHledaniMs - System.currentTimeMillis() + 999L) / 1000L);
        }
        if (hlasPripojeni) log("\"" + jm + "\" PRIPOJEN jako herni ovladac - hotovo");
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
