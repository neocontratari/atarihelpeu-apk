package eu.atarihelp.emu10;

import android.Manifest;
import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.ContentValues;
import android.content.ContentUris;
import android.content.pm.PackageManager;
import android.database.Cursor;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.BatteryManager;
import android.content.ClipData;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.os.Build;
import android.os.Debug; // BUILD2RW: passive heap/GC audit only
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.provider.OpenableColumns;
import android.provider.MediaStore;
import android.util.Base64;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.PixelFormat;
import android.graphics.Rect;
import android.hardware.display.VirtualDisplay;
import android.media.AudioAttributes;
import android.media.AudioFormat;
import android.media.Image;
import android.media.ImageReader;
import android.media.AudioTrack;
import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaFormat;
import android.media.projection.MediaProjection;
import android.media.projection.MediaProjectionManager;
import android.view.View;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.InputDevice;
import android.view.TextureView;
import android.graphics.SurfaceTexture;
import android.view.ViewGroup;
import android.util.DisplayMetrics;
import android.widget.FrameLayout;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;
import android.webkit.JavascriptInterface;
import android.webkit.ValueCallback;
import android.webkit.WebChromeClient;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.webkit.WebResourceRequest;
import android.webkit.WebResourceError;
import android.webkit.WebResourceResponse;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.Inet4Address;
import java.net.InetAddress;
import java.net.NetworkInterface;
import java.net.ServerSocket;
import java.net.Socket;
import java.net.URL;
import java.net.URLDecoder;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.Enumeration;
import java.util.HashSet;

/**
 * AtariHelp.eu EMU-10 BUILD2BM
 * - file chooser (NAHRAJ XEX/ATR/ZIP)
 * - AHSAVE (ulozeni logu)
 * - DownloadListener: ZIP/XEX/ATR z webu se stahne a rovnou spusti v emulatoru
 * - BUILD2AG UI: NET HRY + XC12 WAV/MP3 real seek pres REW/F.FWD
 * - BUILD2AQ INTRO MP3: MP3 PRIDAT nacita/pripojuje skladby do playlistu + EJECT reset
 * - BUILD2BM XC12: cycle-accurate tape tone for CLOAD + quieter WAV + touch zone alignment
 */
public class MainActivity extends Activity {
    private static final int PICK_FILE = 1;
    private static final int PICK_BRIDGE = 2;
    private static final int PICK_PS1_GAME = 7; // BUILD2SA2
    private static final int PICK_AUDIO_PERMISSION = 14; // BUILD2SA13C14: local MP3/WAV library permissions
    private static final int PICK_TV_WEB_SCREEN = 13; // BUILD2SA13C9: whole-phone MediaProjection mirror
    private static final String ATARIHELP_BROWSER_UA = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/126.0.0.0 Safari/537.36"; // BUILD2SA5K
    private static final long ATARIHELP_MIN_REQUEST_GAP_MS = 30000L; // BUILD2SA5M: no accidental hammering.
    private static final long ATARIHELP_FAIL_COOLDOWN_MS = 15L * 60L * 1000L;
    private static final long ATARI_NET_INJECT_RETRY_MS = 300L; // BUILD2SA5AJ: only polls; does not keep reloading 130XE.
    private static final int ATARI_NET_INJECT_MAX_ATTEMPTS = 42;
    private static final long ATARI_NET_OPEN_SETTLE_MS = 2600L; // BUILD2SA5AJ: like Sega delayed inject after opening emulator page.
    private static final long ATARI_NET_INJECT_SETTLE_MS = 900L;
    private static final long ATARI_NET_INJECT_FALLBACK_MS = 6200L;
    private static final long PS1_REMOTE_MAX_BYTES = 1800L * 1024L * 1024L; // BUILD2SA5AK: PS1 image from Reneho PC, streamovane na disk.

    // ===================================================================
    //  SEGA: PLOCHA PRO OBRAZ  (stejna kostra jako PS1)
    //
    //  Doted se obraz Segy snimal z okna aplikace - proto to zpozdeni
    //  proti mobilu. Ted kresli nativni cast primo na plochu pres
    //  OpenGL ES, uplne stejne jako PS1.
    //  Plocha lezi POD strankou, aby ovladac zustal nad obrazem.
    // ===================================================================
    private android.view.SurfaceView segaPlocha = null;

    private void segaPlochaZapni() {
        try {
            if (segaPlocha != null || rootFrame == null) return;
            android.view.SurfaceView sv = new android.view.SurfaceView(MainActivity.this);
            sv.setClickable(false);
            sv.setEnabled(false);
            sv.setFocusable(false);
            final android.view.SurfaceView tato = sv;
            sv.getHolder().addCallback(new android.view.SurfaceHolder.Callback() {
                @Override public void surfaceCreated(android.view.SurfaceHolder h) {
                    if (segaPlocha != tato) return;
                    appendNativeLog("SEGA_PLOCHA_VYTVORENA - obraz jde z jadra pres OpenGL ES");
                    NativeSegaCoreBridge.setDisplaySurfaceSafe(h.getSurface());
                }
                @Override public void surfaceChanged(android.view.SurfaceHolder h, int f, int w, int hh) {
                    // znovu NEPRIPOJOVAT - u PS1 to delalo melu vlaken
                    appendNativeLog("SEGA_PLOCHA_ZMENENA " + w + "x" + hh);
                }
                @Override public void surfaceDestroyed(android.view.SurfaceHolder h) {
                    if (segaPlocha != null && segaPlocha != tato) return;
                    NativeSegaCoreBridge.setDisplaySurfaceSafe(null);
                }
            });
            rootFrame.addView(sv, 0, new FrameLayout.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
            segaPlocha = sv;
            appendNativeLog("SEGA_OBRAZ_PRIMO_ZAPNUT");
        } catch (Throwable t) {
            appendNativeLog("SEGA_OBRAZ_PRIMO_CHYBA " + safeMsg(t));
        }
    }

    private void segaPlochaVypni() {
        final android.view.SurfaceView old = segaPlocha;
        segaPlocha = null;
        Runnable r = () -> {
            try {
                NativeSegaCoreBridge.setDisplaySurfaceSafe(null);
                if (old != null && old.getParent() instanceof ViewGroup)
                    ((ViewGroup) old.getParent()).removeView(old);
                appendNativeLog("SEGA_OBRAZ_PRIMO_VYPNUT");
            } catch (Throwable ignored) {}
        };
        if (isUiThread()) r.run(); else ui.post(r);
    }

    private static final String SEGA_URL = "file:///android_asset/emu_sega/index.html"; // BUILD2SA2
    private static final String PS1_URL = "file:///android_asset/emu_ps1/index.html"; // BUILD2SA5AM
    private static final String PS1_GOOGLE_GAMES_URL = "https://atarihelp.eu/?page_id=1048"; // BUILD2SA5AM
    private static byte[] pendingSegaGame = null;   // BUILD2SA2: hra ze SBIRKY cekajici na nacteni Sega stranky
    private static String pendingSegaName = null;
    private static android.os.ParcelFileDescriptor ps1GamePfd = null; // BUILD2SA2: drzi fd otevrene hry
    private static volatile String ps1LastBootResult = "not_booted";
    private static volatile int ps1AudioGen = 0; // BUILD2SA3
    private volatile int ps1LifecycleGen = 0; // BUILD2SA5I: cancels stale PS1 boots/audio after leaving PS1.
    private volatile boolean ps1BootActive = false;
    private volatile boolean ps1SessionActive = false;
    // Kdyz se spusti okno hry (NativeActivity + eglrender), prebira si jadro
    // ONO. Appka do nej od te chvile nesmi sahat - drive ho pri svem onPause
    // tvrde zastavovala prave ve chvili, kdy se okno hry rozjizdelo:
    //   17:30:45.754  KROKC EGL_PS1_LAUNCH
    //   17:30:45.914  PS1_SESSION_STOP reason=activityPause
    // To jsou ty "dve verze v jednom emulatoru".
    private volatile boolean ps1GameWindowOwnsCore = false;
    private volatile boolean ps1BiosStarting = false;
    private volatile boolean ps1BiosRunning = false;

    /** Spusti PS1 bez disku (menu BIOSu), kdyz nic jineho nebezi. */
    private void ps1MaybeStartBios() {
        if (ps1BiosRunning || ps1BiosStarting) return;
        if (ps1GameWindowOwnsCore || ps1SessionActive || ps1BootActive) return;
        ps1BiosStarting = true;
        new Thread(() -> {
            java.io.File pokus = new java.io.File(getFilesDir(), "ps1_bios_pokus");
            try {
                // ===== POJISTKA PROTI OPAKOVANEMU PADU =====
                // Kdyz minuly pokus o start BIOSu skoncil padem, zustane tu
                // znacka. Priste uz start nezkousime, aby se appka nezacyklila
                // v padani a dala se normalne pouzivat.
                if (pokus.exists()) {
                    appendNativeLog("PS1_BIOS_PRESKOCENO minuly pokus skoncil padem - mazu znacku, priste to zkusim znovu");
                    pokus.delete();
                    ps1BiosStarting = false;
                    return;
                }
                try { pokus.createNewFile(); } catch (Throwable ignored) {}
                java.io.File sysDir  = new java.io.File(getFilesDir(), "ps1_system");
                java.io.File saveDir = new java.io.File(getFilesDir(), "ps1_saves");
                if (!sysDir.exists())  sysDir.mkdirs();
                if (!saveDir.exists()) saveDir.mkdirs();
                ps1EnsureBios(sysDir);          // BIOS musi byt na miste
                appendNativeLog("PS1_BIOS_START bez disku (jako zapnuti konzole)");
                // Plocha, na kterou jadro kresli PRIMO. Musi byt i pro BIOS -
                // drive se zapinala jen pro hru a BIOS kreslil pres JPEG.
                ui.post(() -> ps1GlEnable());
                String r = NativePs1CoreBridge.bootBiosSafe(
                        sysDir.getAbsolutePath(), saveDir.getAbsolutePath());
                appendNativeLog("PS1_BIOS_START vysledek=" + r);
                ps1BiosRunning = r != null && r.startsWith("PS1_BIOS_OK");
                ps1LastBootResult = r;
                pokus.delete();   // dobehlo bez padu - znacku uklidime
            } catch (Throwable t) {
                appendNativeLog("PS1_BIOS_START_ERR " + safeMsg(t));
            } finally {
                ps1BiosStarting = false;
            }
        }, "ps1-bios-start").start();
    }
    private volatile boolean ps1RemoteDownloadActive = false;
    private volatile String ps1RemoteDownloadStatus = "idle";
    private volatile long atariHelpLastRequestAtMs = 0L; // BUILD2SA5M
    private volatile long atariHelpBlockedUntilMs = 0L;
    private volatile AudioTrack ps1CurrentAudioTrack = null; // BUILD2SA3B: hard-stop pri prepnuti PS1 hry
    private Thread ps1AudioThread = null;
    private volatile String ps1CurrentGameLabel = "ps1_game";
    private static final String EMU_URL = "file:///android_asset/emu/index.html";
    private WebView web;
    private FrameLayout rootFrame;
    private static class NapPlayerAudioItem {
        String name;
        String uri;
        long size;
        long modified;
        NapPlayerAudioItem(String name, String uri, long size, long modified) {
            this.name = name;
            this.uri = uri;
            this.size = size;
            this.modified = modified;
        }
    }
    // ===== BUILD2SA13: VLASTNI TV VYSTUP PRO CELOU APKU =====
    // Zadna externi appka: kdyz je pripojena TV/monitor (HDMI/USB-C adapter,
    // nebo bezdratove pres systemove pripojeni displeje), appka na ni SAMA
    // kresli svuj obraz (vsechny emulatory, prehravac, stranky). Kresli se
    // rootFrame (WebView + TextureView = jde to, TextureView se do canvasu
    // vykresli). ~25 fps, dvojity buffer, meritko podle TV.
    private android.hardware.display.DisplayManager napDisplayManager;
    private NapTvPresentation napTvPresentation;
    private android.graphics.Bitmap napTvBmpA, napTvBmpB;
    private boolean napTvUseA = true;
    private final Runnable napTvFrameTick = new Runnable() {
        @Override public void run() {
            try {
                if (napTvPresentation != null && napTvPresentation.isShowing() && rootFrame != null && rootFrame.getWidth() > 0) {
                    int sw = rootFrame.getWidth(), sh = rootFrame.getHeight();
                    float scale = 0.75f;
                    int bw = Math.max(2, (int)(sw * scale)), bh = Math.max(2, (int)(sh * scale));
                    android.graphics.Bitmap target = napTvUseA ? napTvBmpA : napTvBmpB;
                    if (target == null || target.getWidth() != bw || target.getHeight() != bh) {
                        target = android.graphics.Bitmap.createBitmap(bw, bh, android.graphics.Bitmap.Config.RGB_565);
                        if (napTvUseA) napTvBmpA = target; else napTvBmpB = target;
                    }
                    android.graphics.Canvas cv = new android.graphics.Canvas(target);
                    cv.drawColor(0xFF000000);
                    cv.save(); cv.scale(scale, scale);
                    rootFrame.draw(cv);
                    cv.restore();
                    napTvPresentation.showFrame(target);
                    napTvUseA = !napTvUseA;
                }
            } catch (Throwable ignored) {}
            if (napTvPresentation != null) ui.postDelayed(this, 40);
        }
    };
    private final class NapTvPresentation extends android.app.Presentation {
        private android.widget.ImageView iv;
        NapTvPresentation(android.content.Context ctx, android.view.Display d) { super(ctx, d); }
        @Override protected void onCreate(Bundle b) {
            super.onCreate(b);
            iv = new android.widget.ImageView(getContext());
            iv.setScaleType(android.widget.ImageView.ScaleType.FIT_CENTER);
            iv.setBackgroundColor(0xFF000000);
            setContentView(iv);
        }
        void showFrame(android.graphics.Bitmap bm) { if (iv != null) iv.setImageBitmap(bm); }
    }
    private void napTvUpdatePresentation() {
        try {
            if (napDisplayManager == null) return;
            android.view.Display[] ds = napDisplayManager.getDisplays(android.hardware.display.DisplayManager.DISPLAY_CATEGORY_PRESENTATION);
            android.view.Display ext = (ds != null && ds.length > 0) ? ds[0] : null;
            if (ext != null && (napTvPresentation == null || napTvPresentation.getDisplay().getDisplayId() != ext.getDisplayId())) {
                if (napTvPresentation != null) { try { napTvPresentation.dismiss(); } catch (Throwable ignored) {} }
                napTvPresentation = new NapTvPresentation(this, ext);
                try {
                    napTvPresentation.show();
                    appendNativeLog("BUILD2SA13 TV_MIRROR_ON display=" + ext.getName() + " id=" + ext.getDisplayId());
                    ui.removeCallbacks(napTvFrameTick);
                    ui.post(napTvFrameTick);
                } catch (Throwable t) { appendNativeLog("BUILD2SA13 TV_MIRROR_SHOW_FAIL " + safeMsg(t)); napTvPresentation = null; }
            } else if (ext == null && napTvPresentation != null) {
                try { napTvPresentation.dismiss(); } catch (Throwable ignored) {}
                napTvPresentation = null;
                ui.removeCallbacks(napTvFrameTick);
                appendNativeLog("BUILD2SA13 TV_MIRROR_OFF display odpojen");
            }
        } catch (Throwable t) { appendNativeLog("BUILD2SA13 TV_MIRROR_ERR " + safeMsg(t)); }
    }
    private final android.hardware.display.DisplayManager.DisplayListener napTvListener =
            new android.hardware.display.DisplayManager.DisplayListener() {
        @Override public void onDisplayAdded(int id) { ui.post(() -> napTvUpdatePresentation()); }
        @Override public void onDisplayRemoved(int id) { ui.post(() -> napTvUpdatePresentation()); }
        @Override public void onDisplayChanged(int id) { }
    };
    // ===== BUILD2SA13C: TV WEB CAST FALLBACK =====
    // Chromecast/Google Home screen cast is system owned and laggy for games.
    // This fallback keeps everything inside the app: phone serves a low-latency
    // JPEG stream over local Wi-Fi and the Android TV opens the shown URL.
    private ServerSocket napTvWebServer;
    private Thread napTvWebServerThread;
    private volatile boolean napTvWebRunning = false;
    private volatile int napTvWebPort = 0;
    private volatile byte[] napTvWebJpeg = null;
    private volatile long napTvWebSeq = 0;
    private Bitmap napTvWebBitmap;
    private volatile boolean napTvWebPixelCopyPending = false;
    private volatile long napTvWebPixelCopyRequestGen = 0;
    private volatile long napTvWebPixelCopyPendingAtMs = 0;
    private volatile long napTvWebPixelCopyFallbackLogMs = 0;
    private volatile long napTvWebPixelCopyDisabledUntilMs = 0;
    private volatile long napTvWebLastFrameMs = 0;
    private HandlerThread napTvWebCopyThread;
    private Handler napTvWebCopyHandler;
    private final Object napTvWebAudioLock = new Object();
    private final byte[] napTvWebAudioRing = new byte[1024 * 1024];
    private volatile long napTvWebAudioSeq = 0;
    private volatile int napTvWebAudioRate = 44100;
    private volatile long napTvWebAudioLastPushMs = 0;
    private volatile String napTvWebAudioSource = "NONE";
    private final float[] napTvWebEqGains = new float[]{0f, 0f, 0f, 0f, 0f};
    private volatile float napTvWebBassGain = 0f;
    private volatile float napTvWebTrebleGain = 0f;
    private volatile float napTvWebBalance = 0f;
    private volatile float napTvWebVolume = 1f;
    private volatile int napTvWebJpegQuality = 62;
    private volatile int napTvWebFrameDelayMs = 55;
    // BUILD2SK81: pojmenovana konstanta pro H264 rychly tick strop (drive bylo
    // "5" napsane natvrdo jen v gate, zatimco TICK_AVG log porad vypisoval
    // stary tier-based napTvWebFrameDelayMs - log tak lhal, i kdyz appka
    // fungovala spravne). Ted jedno misto pravdy pro obe strany.
    // ===== TADY SE KOUSAL ZVUK PO ZAPNUTI TV =====
    // Kdyz se pripoji TV, smycka snimani se prepne z 10 ms na tuhle hodnotu.
    // Bylo tu 5 ms = DVE STE pruchodu za vterinu, prestoze jadro vyrobi
    // nejvys 60 snimku. Kazdy pruchod snima, orezava cerne okraje a krmi
    // enkoder - devet z deseti uplne zbytecne. Sebralo to procesor emulaci
    // a zvuk zacal podtekat. V logu: TV_WEB_TICK_AVG avgTickGapMs=35
    // pri targetDelayMs=5.
    // 16 ms = cca 60 za vterinu, coz je presne tolik, kolik ma smysl.
    // POZOR: nesnizovat. Vic snimku na TV se stejne nevejde a zvuk to zabije.
    // 12 ms misto 16: enkoder ma podle logu rezervu (avgDrawMs=3 pri
    // stropu 16), a Sega i PS1 jedou 60 snimku za vterinu. Kratsi krok
    // znamena, ze se snimek dostane na TV drive - mensi zpozdeni.
    // NESNIZOVAT POD 12. Pri 5 ms se v B82 kousal zvuk, protoze smycka
    // brala procesor emulaci.
    private static final int napTvWebH264FastTickMs = 12;
    // === BUILD2SK57: H.264 STREAM PRO PS1 (misto MJPEG) ===
    // Pouzivame MediaCodec v ByteBuffer rezimu (ne Surface) - vstupem je
    // rucne prevedeny YUV420 obraz z JIZ existujici PixelCopy Bitmapy
    // (stejny zdroj, ktery uz spolehlive funguje pro PS1 od SK46), takze
    // se PS1 zachytavani vubec nemeni - meni se jen co se s tim snimkem
    // dal deje. Vystup enkoderu je surovy H.264 elementarni stream
    // (Annex-B, start-kody 00 00 00 01) - presne format, ktery ocekava
    // JMuxer.js na klientovi (zadne MP4 balenu na Android strane, zadny
    // rizikovy externi muxer s neoverenym API).
    private MediaCodec napTvWebH264Encoder;
    // Kdyz je zapnuta prima cesta, snimky pro TV kresli nativni cast a
    // javova cesta (snimani, orez, Bitmap, Canvas) se VUBEC nespousti.
    // Jinak by do enkoderu kreslily dva zdroje naraz.
    private volatile boolean tvPrimoBezi = false;
    // Vyladeni obrazu pro TV pred kompresi (viz napTvWebCaptureFromCore).
    // Da se prepnout z prohlizece, at jde porovnat s puvodnim stavem.
    private volatile boolean napTvVyhlazeni = true;
    private long napTvKresDiagSum = 0;
    private int napTvKresDiagPocet = 0;
    private android.view.Surface napTvWebH264InputSurface;
    private final Object napTvWebH264Lock = new Object();
    private volatile int napTvWebH264W = 0;
    private volatile int napTvWebH264H = 0;
    private volatile long napTvWebH264Seq = 0;
    private volatile long napTvWebH264LastFrameMs = 0;
    private volatile long napTvWebH264FrameIndex = 0;
    private volatile long napTvWebH264StartNanos = 0;
    // ===== ZAHAZOVANI SNIMKU, KDYZ ENKODER NESTIHA =====
    // lockHardwareCanvas() na vstupu enkoderu CEKA, dokud enkoder neuvolni
    // buffer. Pri 1280x720 to telefon nestiha - v logu bylo 1149 snimku
    // s "H264_FRAME_SLOW", median 90 ms a maximum 586 ms. Cele vlakno tim
    // stalo a kousal se obraz i zvuk, na mobilu i na TV.
    // Reseni: kdyz predchozi snimek trval dlouho, TENHLE VYNECHAME. Radeji
    // mene snimku na TV nez zadrhavajici emulace.
    private volatile long napTvWebH264PosledniMs = 0;
    private volatile int  napTvWebH264Vynechano = 0;
    private static final long NAP_TV_H264_STROP_MS = 40;
    // BUILD2SK61: KRITICKA OPRAVA - /status endpoint bezi na SAMOSTATNEM
    // vlakne per-klient (napTvWebHandleClient), NE na UI vlakne. Volani
    // web.getUrl() PRIMO z tohoto vlakna je nebezpecne (WebView metody
    // ocekavaji UI vlakno) a tise SELHAVALO (catch(Throwable) to
    // polykalo) - proto klient v /status VZDY videl prazdne "url=",
    // takze detekce PS1 na strane prohlizece NIKDY neuspela a H264 se
    // nikdy nespustilo, i kdyz vsechno ostatni (SK57-60) bylo spravne.
    // Reseni: URL se ted bezpecne cachuje ZDE, v tick smycce (UI vlakno,
    // presne tam, kde uz web.getUrl() beztak bezpecne volame pro
    // periodicky log), a /status jen cte tuhle cache - zadne dalsi
    // volani web.getUrl() mimo UI vlakno.
    private volatile String napTvWebCurrentUrl = "";
    // BUILD2SK67: detekce duplicitnich snimku - overuje jestli PS1
    // emulator sam produkuje novy obsah tak rychle, jak ho zachytavame
    private long napTvWebLastSampleHash = Long.MIN_VALUE;
    private int napTvWebDupCheckCount = 0, napTvWebDupCheckSame = 0;
    // BUILD2SK68: prumerovani PixelCopy latence a JPEG komprese pro
    // VSECHNY snimky (ne jen "pomale" vyjimky) - viz vysvetleni u
    // mista pouziti
    private long napTvWebPcDiagSumMs = 0;
    private int napTvWebPcDiagCount = 0;
    private long napTvWebCompressDiagSumMs = 0;
    private int napTvWebCompressDiagCount = 0;
    // BUILD2SK63: kdyz se enkoder restartuje kvuli zmene rozliseni (napr.
    // portret<->landscape, zmena kvalitni urovne), NOVY enkoder produkuje
    // NOVE SPS/PPS (parametry kodeku). Pokud tohle dorazi klientovi
    // UPROSTRED existujiciho spojeni/JMuxer instance, prohlizec to tise
    // odmitne (zadna chyba, jen se prestane neco dit) - presne to bylo
    // videt v logu: opakovane restarty enkoderu, ale klient se po prvnim
    // uspesnem spojeni uz nikdy znovu nepripojil. Reseni: generacni
    // pocitadlo - kazde pripojeni klienta si zapamatuje generaci PRI
    // pripojeni, a pokud se PRUBEZNE zmeni (enkoder restartoval), server
    // spojeni AKTIVNE UKONCI - klientuv fetch() dostane "stream done"
    // prirozene, a jeho JS (SK63 oprava) se kvuli tomu spravne resetuje
    // a pri dalsim pollFps cyklu se pripoji ZNOVU s cerstvym JMuxerem.
    private volatile long napTvWebH264Generation = 0;
    // BUILD2SK64: predavaci mechanismus pro pracovni vlakno (viz
    // napTvWebH264FeedFrame vs napTvWebH264FeedFrameInternal)
    private final Object napTvWebH264PendingLock = new Object();
    private Bitmap napTvWebH264PendingBitmap = null;
    private volatile Thread napTvWebH264WorkerThread = null;
    private volatile boolean napTvWebH264WorkerRunning = false;
    // kazdy pripojeny /stream.h264 klient ma vlastni frontu - na rozdil od
    // MJPEG (kde stačí poslat jen NEJNOVEJSI snimek) tady KAZDA jednotka
    // (NAL) musi dorazit VSEM klientum V PORADI, jinak dekoder dostane
    // poskozeny bitstream (P-snimky zavisi na predchozich).
    private final java.util.List<java.util.concurrent.LinkedBlockingQueue<byte[]>> napTvWebH264ClientQueues =
            new java.util.concurrent.CopyOnWriteArrayList<>();

    // BUILD2SK15: potvrzeno na S8 ze soucasny HIGH byl v pohode (plynulost i zvuk) -
    // takze byl zbytecne konzervativni. MEDIUM posunut na uroven puvodniho HIGH,
    // HIGH posunut vyrazne dal. LOW zustava PRESNE stejny jako v SK11-14 (nikomu
    // se nic nemeni, dokud sam neprepne).
    private volatile int napTvWebQualityTier = 0; // 0=LOW 1=MEDIUM 2=HIGH
    private int[] napTvWebQualityFor(boolean djScreen, boolean hqLiteScreen, boolean landscape) {
        int t = napTvWebQualityTier; if (t < 0) t = 0; if (t > 2) t = 2;
        int[][] table;
        // BUILD2SK17: HIGH tlacen jeste dal - S8 potvrzeno v pohode na predchozim
        // HIGH (SK15), takze je prostor. LOW a MEDIUM beze zmeny.
        // BUILD2SK39: na vyslovny pozadavek - HIGH delay ztrojnasoben (hodnoty
        // snizeny na tretinu), rozliseni ANI kvalita NEDOTCENY - jen rychlost.
        // BUILD2SK55: na vyslovny pozadavek - FPS ted PEVNE fixovane misto
        // promenlivych hodnot podle vetve: LOW=20fps(50ms), MEDIUM=25fps(40ms),
        // HIGH=30fps(33ms). Rozliseni a kvalita% NEDOTCENY, meni se jen treti
        // sloupec (frame delay) ve vsech ctyrech vetvich.
        // BUILD2SK78: landscape (hqLiteScreen/ostatni radky) mela driv nizsi
        // cilove rozliseni nez portret VE VSECH urovnich - Rene to potvrdil
        // vizualne ("landscape nesaha portretu"), a cisla to primo dokazuji
        // (HIGH portret 1920 vs landscape 1680, stejny vzorec u LOW/MEDIUM).
        // Tohle nejspis vzniklo driv kvuli vykonu - po Surface prepracovani
        // (SK75, zadna Java YUV smycka) uz je vykonova rezerva vetsi, takze
        // landscape rozliseni ted odpovida portretu ve vsech trech urovnich.
        // BUILD2SK92: CELA STUPNICE POSUNUTA NAHORU na Reneho vyslovny
        // pozadavek - "to, co ted bylo HIGH (overene, funguje), ma byt nova
        // LOW". POKUS SELHAL - viz SK94 nize.
        // BUILD2SK94: SK92 CISLA BYLA CHYBA, POTVRZENO PRIMO Z LOGU - vsechny
        // tri nova cisla (1920/2300/2700) PRESAHOVALA skutecnou zmerenou
        // velikost zachytavane plochy na tomhle zarizeni (~1384x672 landscape -
        // opakovane potvrzeno napric desitkami logu v teto relaci). Vzorec
        // "scale = min(1.0, cil/skutecna_velikost)" se tak VZDY sepnul na
        // strop 1.0 pro VSECHNY TRI urovne - LOW i HIGH tak vyprodukovaly
        // BYTOVE STEJNE rozliseni i bitrate (overeno v ENCODER_START logu:
        // w=1384 h=672 bitrate=5580288 pro OBA tier=0 I tier=2). Presne to,
        // co Rene nahlasil - "nechapu princip", protoze mezi urovnemi
        // OPRAVDU nebyl zadny rozdil.
        // OPRAVA: LOW/MEDIUM jsou ted cisla, ktera na BEZNYCH telefonech
        // (vcetne tohohle S8) skutecne zpusobi rozliseni POD nativnim
        // stropem - realny, viditelny rozdil. HIGH = zamerne velmi vysoke
        // cislo (9999) - na KAZDEM telefonu se strop stejne uplatni pres
        // min(1.0,...), takze HIGH VZDY znamena "cele nativni rozliseni
        // teto konkretni obrazovky", at uz je to S8 nebo vykonnejsi telefon
        // za rok - presne Reneho puvodni myslenka (HIGH = strop pro DANY
        // telefon), jen bez nutnosti hadat konkretni cislo predem.
        if (!landscape)         table = new int[][]{{900,94,33},{1150,94,28},{9999,94,25}};
        else if (djScreen)      table = new int[][]{{900,94,33},{1150,94,28},{9999,94,25}};
        else if (hqLiteScreen)  table = new int[][]{{900,90,33},{1150,93,28},{9999,94,25}};
        else                    table = new int[][]{{900,86,33},{1150,90,28},{9999,94,25}};
        return table[t];
    }
    private volatile String napTvWebVideoProfile = "AUTO";
    private MediaProjectionManager napTvWebProjectionManager;
    private MediaProjection napTvWebProjection;
    private VirtualDisplay napTvWebVirtualDisplay;
    private ImageReader napTvWebImageReader;
    private HandlerThread napTvWebSystemThread;
    private Handler napTvWebSystemHandler;
    private volatile boolean napTvWebSystemMirrorRequested = false;
    private volatile boolean napTvWebSystemMirrorActive = false;
    private volatile long napTvWebSystemLastFrameMs = 0;
    private volatile long napTvWebSystemFallbackLogMs = 0;
    private volatile boolean napTvWebSystemInFallback = false;
    private volatile int napTvWebSystemFreshStreak = 0;
    private volatile boolean napTvWebResizeInProgress = false;
    private volatile long napTvWebResizeStartedMs = 0;
    private volatile int napTvWebBackingSurfaceFailStreak = 0;
    private volatile int napTvWebSystemWidth = 0;
    private volatile int napTvWebSystemHeight = 0;
    private volatile int napTvWebSystemDpi = 0;
    private volatile String napTvWebPendingScreenUrl = null;
    private volatile long napTvWebYoutubeInAppUntilMs = 0;
    private volatile long napTvWebPeriodicLogMs = 0;
    // BUILD2SK70: mereni celeho tick-to-tick cyklu
    private long napTvWebTickLastStartMs = 0;
    private long napTvWebTickDiagSumMs = 0;
    private int napTvWebTickDiagCount = 0;
    private final Runnable napTvWebFrameTick = new Runnable() {
        @Override public void run() {
            // BUILD2SK70: PixelCopy (~25ms) + H264 zpracovani (~5ms) dohromady
            // nevysvetluji pozorovany cca 65-77ms interval mezi snimky - musi
            // byt jeste dalsi ~35-45ms nekde, co jsem dosud nemeril. Tohle meri
            // PRIMO cely cyklus tick-to-tick (od zacatku jednoho behu run() po
            // zacatek dalsiho) - konecne uvidim, jestli je to planovani/cekani,
            // nebo dalsi neznama prace uvnitr teto metody.
            long tickNow = System.currentTimeMillis();
            if (napTvWebTickLastStartMs > 0) {
                long tickGap = tickNow - napTvWebTickLastStartMs;
                try {
                    if (!napTvWebH264ClientQueues.isEmpty()) {
                        napTvWebTickDiagSumMs += tickGap;
                        napTvWebTickDiagCount++;
                        if (napTvWebTickDiagCount >= 30) {
                            // BUILD2SK81: tenhle log se vola jen kdyz H264 fronta NENI
                            // prazdna (podminka o par radku vyse) - skutecny strop pro
                            // TENTO tick je tedy VZDY napTvWebH264FastTickMs, nikdy stary
                            // tier-based napTvWebFrameDelayMs.
                            // BUILD2SK83: + tier=, aby se dalo primo (bez cross-referencu
                            // s TV_WEB_PERIODIC podle casu) videt, ktera uroven kvality
                            // (0=LOW/1=MEDIUM/2=HIGH) k temhle cislum patri.
                            appendNativeLog("BUILD2SK81 TV_WEB_TICK_AVG n=" + napTvWebTickDiagCount
                                    + " avgTickGapMs=" + (napTvWebTickDiagSumMs / napTvWebTickDiagCount)
                                    + " targetDelayMs=" + napTvWebH264FastTickMs
                                    + " tier=" + napTvWebQualityTier);
                            napTvWebTickDiagCount = 0; napTvWebTickDiagSumMs = 0;
                        }
                    }
                } catch (Throwable ignored) {}
            }
            napTvWebTickLastStartMs = tickNow;
            long extraDelay = 0;
            try {
                // BUILD2SK21: behem vedome prestavby VirtualDisplay (zmena urovne
                // kvality, viz napTvWebResizeSystemMirror) NECHCEME zachytavat vubec -
                // obraz zustane zmrazeny na poslednim dobrem snimku, misto aby na
                // okamzik probliknul fallback WebView zachytavanim. 4s pojistka proti
                // trvalemu zamrznuti, kdyby novy VirtualDisplay z nejakeho duvodu
                // nikdy neposlal prvni snimek.
                if (napTvWebResizeInProgress) {
                    if (System.currentTimeMillis() - napTvWebResizeStartedMs > 4000L) {
                        napTvWebResizeInProgress = false; // vzdavame cekani, pokracuj normalne
                    } else {
                        if (napTvWebRunning) ui.postDelayed(this, 40);
                        return;
                    }
                }
                boolean appCapture = true;
                if (napTvWebRunning && napTvWebSystemMirrorActive) {
                    long age = napTvWebSystemLastFrameMs == 0 ? 999999L : (System.currentTimeMillis() - napTvWebSystemLastFrameMs);
                    // BUILD2SK19: SK16 pridalo 3.5s prah, ale bez hystereze - pri
                    // turbulentnim prechodu (napr. opusteni PS1) mohou system-mirror
                    // snimky chodit nepravidelne kolem prahu, coz zpusobi RYCHLE
                    // KMITANI appCapture tam a zpet (presne "problikavani tam zpet
                    // a zpet", ktere bylo nahlaseno). Oprava: jakmile jednou spadneme
                    // do fallbacku, vyzadujeme NEKOLIK PO SOBE JDOUCICH cerstvych
                    // snimku (ne jen jeden), nez se vratime zpet - klasicka hystereze
                    // proti kmitani prahoveho prepinace.
                    if (age <= 250L) napTvWebSystemFreshStreak++; else napTvWebSystemFreshStreak = 0;
                    if (!napTvWebSystemInFallback) {
                        appCapture = age > 3500L;
                        if (appCapture) napTvWebSystemInFallback = true;
                    } else {
                        appCapture = napTvWebSystemFreshStreak < 3;
                        if (!appCapture) napTvWebSystemInFallback = false;
                    }
                    if (appCapture && System.currentTimeMillis() - napTvWebSystemFallbackLogMs > 5000L) {
                        napTvWebSystemFallbackLogMs = System.currentTimeMillis();
                        appendNativeLog("BUILD2SA13C10 SCREEN_MIRROR_NO_FRAMES_FALLBACK ageMs=" + age);
                    }
                }
                // BUILD2SK41: periodicke logovani stavu (kazde ~2s) - misto
                // pozadovani presneho casovani od uzivatele (nemozne trefit rucne)
                // si appka zaznamenava sama, prubezne, po celou dobu testu. Pri
                // rozboru logu pak uvidime PRESNY stav v okamziku, kdy k
                // zasekavani doslo, aniz by bylo treba cokoli casovat.
                {
                    long nowLog = System.currentTimeMillis();
                    if (nowLog - napTvWebPeriodicLogMs > 2000L) {
                        napTvWebPeriodicLogMs = nowLog;
                        String curUrl = "?";
                        try { curUrl = web == null ? "null" : web.getUrl(); } catch (Throwable ignored) {}
                        napTvWebCurrentUrl = curUrl; // BUILD2SK61: cache pro /status - viz vysvetleni u deklarace pole

                        // BUILD2SK84: battery/CPU/thermal kontext PRIMO v periodickem logu.
                        // readBatteryTempC()/readCpuFreqKHz() uz existovaly (BUILD2RX) a
                        // uz drive proverovaly tepelne hrdlo Segy na S8 - ale JEN dokud
                        // bezelo nativni Sega/Atari jadro (nativeInPlaceEnabled). TV-cast
                        // H264 cesta (PS1, DJ pult, domovska obrazovka, i Sega/Atari pres
                        // TV-cast) timhle vubec nebyla pokryta. Ted je to tady pro VSECHNY
                        // obrazovky, tagovane primo s tier - takze LOW/MEDIUM/HIGH srovnani
                        // uz nemusi byt jen dohad "asi to hrdli", ale primo videt v datech.
                        String powerSave = "na", thermal = "na";
                        try {
                            android.os.PowerManager pm = (android.os.PowerManager) getSystemService(POWER_SERVICE);
                            if (pm != null) {
                                powerSave = String.valueOf(pm.isPowerSaveMode());
                                if (Build.VERSION.SDK_INT >= 29) thermal = napTvWebThermalName(pm.getCurrentThermalStatus());
                            }
                        } catch (Throwable ignored) {}
                        appendNativeLog("BUILD2SK84 TV_WEB_PERIODIC mirror=" + (napTvWebSystemMirrorActive ? "SCREEN" : "APP")
                                + " appCapture=" + appCapture + " seq=" + napTvWebSeq
                                + " bmW=" + (napTvWebBitmap == null ? -1 : napTvWebBitmap.getWidth())
                                + " bmH=" + (napTvWebBitmap == null ? -1 : napTvWebBitmap.getHeight())
                                + " bmDrawW=" + (napTvWebBitmapDraw == null ? -1 : napTvWebBitmapDraw.getWidth())
                                + " bmDrawH=" + (napTvWebBitmapDraw == null ? -1 : napTvWebBitmapDraw.getHeight())
                                + " pcPending=" + napTvWebPixelCopyPending
                                + " gen=" + napTvWebPixelCopyRequestGen
                                // BUILD2SK85: "qJpeg" se ZDE ZALMERNE NEUVADI - napTvWebJpegQuality je
                                // nazev z pred-H264 (MJPEG) éry a kdyz je H264 klient pripojeny (coz
                                // je vzdy tenhle pripad), JPEG komprese se cele PRESKAKUJE (viz
                                // napTvWebPublishBitmap) - to cislo by tu jen matlo, nic nerika o
                                // tom, co skutecne vidis na TV.
                                + " tier=" + napTvWebQualityTier
                                + " battTempC=" + readBatteryTempC()
                                + " cpu0KHz=" + readCpuFreqKHz(0) + " cpu4KHz=" + readCpuFreqKHz(4)
                                + " powerSave=" + powerSave + " thermal=" + thermal
                                + " url=" + curUrl);
                    }
                }
                if (napTvWebRunning && appCapture && rootFrame != null && rootFrame.getWidth() > 0 && rootFrame.getHeight() > 0) {
                    int sw = rootFrame.getWidth(), sh = rootFrame.getHeight();
                    boolean landscape = sw > sh;
                    // BUILD2SK11: DJ obrazovka je staticka UI (jemny text, EQ tahla) - vyhody
                    // je z ostrosti vic nez z vysokeho FPS jako u emulatoru s pohybujici se hrou.
                    // V landscape proto dostane stejnou kvalitu/rozliseni jako portret misto
                    // "rychleho ale mekkeho" profilu, ktery zustava beze zmeny pro vse ostatni.
                    boolean djScreen = false;
                    try { String cu = web == null ? null : web.getUrl(); djScreen = cu != null && cu.contains("/dj/"); } catch (Throwable ignored) {}
                    // BUILD2SK13: PS1 boost z SK12 rozsiren na Segu (/emu_sega/) a Atari
                    // (/emu/, potvrzeno EMU130/ATARI_NET znackami v logu = Atari). Stejne
                    // uz otestovane hodnoty jako PS1 (860px/62) - zadne nove cislo, uzivatel
                    // potvrdil ze plynulost u PS1 zustala stejna, takze stejne zachazeni pro
                    // dalsi dva emulatory nese stejne (nizke) riziko.
                    // BUILD2SK49: draw() opakovane selhaval na ruznem obsahu (Atari, DJ pult,
                    // MP3, a video/log dukaz ted ukazal ze i uvodni stranka po prechodu z
                    // hqLite obrazovky) - vzorec uz neni "tenhle konkretni typ obsahu", je to
                    // "draw() na tomhle WebView neni spolehlivy zpusob zachytavani vubec".
                    // PixelCopy naproti tomu spolehlive fungoval na VSEM, kam byl nasazen.
                    // Dve opravy, ktere driv delaly univerzalni PixelCopy nebezpecnym (SK32/35
                    // katastrofy), uz jsou na miste: SK38 (zahazovani zastaralych vysledku
                    // podle generace) a SK48 (draw() a PixelCopy uz nesdileji jednu bitmapu).
                    // hqLiteScreen je ted VZDY true - PixelCopy pro vsechen obsah, draw() se
                    // pro zachytavani uz vubec nepouziva.
                    boolean hqLiteScreen = true;
                    int[] qv = napTvWebQualityFor(djScreen, hqLiteScreen, landscape);
                    int maxSide = qv[0];
                    napTvWebJpegQuality = qv[1];
                    napTvWebFrameDelayMs = qv[2];
                    napTvWebVideoProfile = (landscape ? (djScreen ? "LANDSCAPE_DJ" : (hqLiteScreen ? "LANDSCAPE_EMU" : "LANDSCAPE_FAST")) : "PORTRAIT") + "_T" + napTvWebQualityTier;
                    float scale = Math.min(1.0f, (float)maxSide / Math.max(sw, sh));
                    int bw = Math.max(2, (int)(sw * scale)), bh = Math.max(2, (int)(sh * scale));
                    // BUILD2SK48: KRITICKY NALEZ - napTvWebBitmap byla SDILENA mezi
                    // PixelCopy (hqLite obsah) A draw() (ostatni obsah). V portretu
                    // maji ruzne obrazovky CASTO stejnou velikost (603x1120 pro vse),
                    // takze se bitmapa NIKDY nepresta vela - a pokud PixelCopy z
                    // PREDCHOZI (hqLite) obrazovky jeste asynchronne zapisovalo do
                    // teto SDILENE bitmapy prave ve chvili, kdy draw() (pro NOVOU,
                    // ne-hqLite obrazovku jako domovska stranka) do TE SAME bitmapy
                    // kreslilo - vysledek byl nepredvidatelny "zavod dvou pisaru do
                    // jedne bitmapy". Presne to odpovida hlasenemu chovani ("skace
                    // do MP3 prehravace misto domu", "pamatuje si Segu jako pilotni
                    // obrazovku"). Oprava: tahle bitmapa (napTvWebBitmap) se ted
                    // pripravuje VYHRADNE kdyz se skutecne pouzije PixelCopy
                    // (hqLiteScreen) - draw() ma od ted SVOJI VLASTNI, oddelenou
                    // bitmapu (napTvWebBitmapDraw, viz napTvWebCaptureByDraw), takze
                    // uz nikdy nemuzou souperit o stejny objekt.
                    if (hqLiteScreen) {
                        if (napTvWebBitmap == null || napTvWebBitmap.getWidth() != bw || napTvWebBitmap.getHeight() != bh) {
                            // BUILD2SK40: stara bitmapa se DRIV NIKDY neuvolnovala
                            // (.recycle()) pred vytvorenim nove - na HIGH urovni jde o
                            // ~7MB na bitmapu (1920x934 ARGB_8888), a pri castych
                            // prechodech mezi obrazovkami (kazda ma jinou velikost) se
                            // tohle mohlo hromadit.
                            if (napTvWebBitmap != null && !napTvWebPixelCopyPending) {
                                try { napTvWebBitmap.recycle(); } catch (Throwable ignored) {}
                            }
                            napTvWebBitmap = Bitmap.createBitmap(bw, bh, Bitmap.Config.ARGB_8888);
                        }
                    }
                    long nowTick = System.currentTimeMillis();
                    boolean didTimeoutFallback = false;
                    if (napTvWebPixelCopyPending && napTvWebPixelCopyPendingAtMs > 0 && nowTick - napTvWebPixelCopyPendingAtMs > 900L) {
                        napTvWebPixelCopyPending = false;
                        napTvWebPixelCopyPendingAtMs = 0;
                        // BUILD2SK22: PS1/Sega/Atari maji nativni GPU vykreslovani,
                        // ktere se pres Canvas.draw() NEZACHYTI VERNE (viz BUILD2SH3
                        // komentar nize - "WebView se pres Canvas.draw nevykresli
                        // verne" - tohle uz drive ZPUSOBOVALO presne tenhle typ
                        // problemu a bylo to znamo, jen draw() zustal jako fallback
                        // presne pro tenhle timeout pripad). Kratsi cooldown (2s
                        // misto 12s) pro nativni obsah, at se PixelCopy zkusi znovu
                        // driv - u ostatniho obsahu (DJ pult, menu) draw() funguje
                        // spolehlive, tam cooldown zustava puvodnich 12s.
                        napTvWebPixelCopyDisabledUntilMs = nowTick + (hqLiteScreen ? 2000L : 12000L);
                        didTimeoutFallback = true;
                        if (nowTick - napTvWebPixelCopyFallbackLogMs > 5000L) {
                            napTvWebPixelCopyFallbackLogMs = nowTick;
                            appendNativeLog("BUILD2SA13C12 TV_WEB_PIXELCOPY_TIMEOUT_FALLBACK_DRAW_COOLDOWN hqLite=" + hqLiteScreen);
                        }
                        if (!hqLiteScreen) { napTvWebCaptureByDraw(bw, bh, scale); napTvWebPixelCopyRequestGen++; }
                        // hqLiteScreen==true: NEVOLAME draw() - radeji zmrazit
                        // posledni dobry PixelCopy snimek nez zobrazit nevery obraz.
                    }
                    // BUILD2SH3: DRIV se pri hrajici hudbe VYPINAL PixelCopy a
                    // prepinalo na draw() -> obraz na projektoru zamrzal a barevne
                    // schema nenaskocilo (WebView se pres Canvas.draw nevykresli verne).
                    // Ted PixelCopy BEZI i pri prehravani (verny obraz), jen mirne
                    // zvednem interval snimku, aby zvuk mel prostor a netrhal se.
                    boolean playerAudioHot = "PLAYER".equals(napTvWebAudioSource)
                            && napTvWebAudioLastPushMs > 0
                            && nowTick - napTvWebAudioLastPushMs < 1500L;
                    boolean pixelCopyAllowed = hqLiteScreen
                            && Build.VERSION.SDK_INT >= 26
                            && napTvWebCopyHandler != null
                            && !napTvWebPixelCopyPending
                            && nowTick >= napTvWebPixelCopyDisabledUntilMs;
                            // BUILD2SK38: primy dukaz z logu (TV_WEB_PIXELCOPY_SLOW
                            // latencyMs=1160 hqLite=FALSE) ukazal, ze PixelCopy na
                            // ne-hqLite obsahu (DJ pult/Atari/uvodni obrazovka) muze
                            // trvat pres vterinu - presne to zpusobovalo hlasene
                            // zasekavani na TECHTO KONKRETNICH obrazovkach (PS1/Sega
                            // v poradku, tam PixelCopy odpovida rychle). Pridano
                            // "hqLiteScreen &&" na zacatek - pro ne-hqLite obsah je
                            // pixelCopyAllowed VZDY false, cimz kod prirozene spadne
                            // do JIZ EXISTUJICI "else if" vetve nize, ktera JIZ
                            // obsahuje "if(!hqLiteScreen) napTvWebCaptureByDraw(...)".
                            // Zadna dalsi zmena struktury - jen tohle jedno misto.
                            // BUILD2SH3: odstraneno "&& !playerAudioHot" - obraz musi
                            // zustat verny i behem hudby.
                    if (playerAudioHot) {
                        // jen lehke uleveni snimkovani, aby audio worker stihal,
                        // ALE stale pres PixelCopy (verny obraz), ne draw.
                        napTvWebVideoProfile = "PLAYER_PIXELCOPY_BAL";
                        napTvWebFrameDelayMs = Math.max(napTvWebFrameDelayMs, landscape ? 90 : 70);
                    }
                    // ===== KROK F: obraz pro TV PRIMO Z JADRA =====
                    // Kdyz bezi nas PS1 obraz, nefotime obrazovku telefonu
                    // (dvoji zvetseni + cerne pruhy + ovladaci prvky), ale
                    // vezmeme framebuffer primo z jadra a JEDNIM krokem ho
                    // roztahneme na cistych 16:9. Ostrejsi a rychlejsi.
                    // OPRAVA: drive bylo podminene existenci ps1GlView (to druhe,
                    // prazdne platno uvnitr appky). Kdyz hra bezi v nativnim okne,
                    // ps1GlView neexistuje - a TV proto snimala okno appky, tedy
                    // jen ovladac bez hry ("Window doesn't have a backing surface").
                    // Snimky z jadra jsou k dispozici vzdy, kdyz bezi PS1 relace.
                    // Zadna podminka navic: proste zkusime vzit snimek z jadra.
                    // Kdyz zadny neni, funkce vrati false a pouzije se zaloha.
                    // (Drive to viselo na priznacich, ktere v ceste pres nativni
                    // okno nemusely platit - a TV pak snimala okno appky, kde je
                    // jen ovladac.)
                    // ZVUK PRO TV: vyzvedneme kopii toho, co se prave prehrava.
                    // Driv se zvuk do TV posilal z Javove zvukove cesty, ta uz
                    // ale nebezi (zvuk obsluhuje nativni OpenSL), takze TV byla
                    // nema. Odbocka v jadre hlavni zvuk nijak nezdrzuje.
                    // ===== B123: PS1 POSILA JAKO DOSUD. USTOUPI JEN TOMU,
                    //             KDO PRAVE OPRAVDU HRAJE. =====
                    // Problem NIKDY nebyl v PS1. Problem byl v tom, ze do
                    // JEDNE zvukove cesty pro TV posilaly DVA zdroje naraz.
                    // Jadro PS1 bezi dal i potom, co uzivatel odejde na Segu
                    // nebo Atari (uz jednou to stalo build u OBRAZU, viz
                    // "ABY TV NEZUSTALA VISET NA POSLEDNIM SNIMKU Z PS1"
                    // v napTvWebCaptureFromCore). Tahle odbocka se proto
                    // dostala ke slovu pri KAZDEM ticku TV a poslala 44100 Hz
                    // jako "PS1", i kdyz hral Sonic.
                    // napTvWebAudioPush pritom pri KAZDE zmene frekvence
                    // nastavuje napTvWebAudioSeq = 0. Sega (48000) a PS1
                    // (44100) se stridaly davku po davce -> sekvence se
                    // nulovala nekolikrat za vterinu -> /audio.raw vracel
                    // prohlizeci PRAZDNOU odpoved -> TICHO na Sege i Atari,
                    // zatimco v telefonu zvuk hral.
                    //
                    // ZAMERNE TO NERESIM OTAZKOU "JE PS1 NA OBRAZOVCE?".
                    // To by znamenalo zaviset na napTvWebCurrentUrl - a kdyby
                    // se ta hodnota jen na chvili opozdila, PS1 by na TV
                    // ztichlo. PS1 je hotove a nesmi se o nej opirat nic
                    // noveho.
                    // Misto toho: PS1 posila UPLNE STEJNE JAKO DOSUD a mlci
                    // jen tehdy, kdyz do zvukove cesty pro TV PRAVE TED
                    // posila NEKDO JINY (posledni cizi davka mladsi nez
                    // 300 ms). Kdyz zadny jiny zdroj nehraje, tahle podminka
                    // je vzdy nepravdiva a chovani je bit po bitu jako v B121.
                    boolean jinyZdrojPravePosila = false;
                    try {
                        String zdroj = napTvWebAudioSource;
                        long odPosledni = System.currentTimeMillis() - napTvWebAudioLastPushMs;
                        jinyZdrojPravePosila = zdroj != null
                                && !"PS1".equals(zdroj)
                                && !"NONE".equals(zdroj)
                                && napTvWebAudioLastPushMs > 0
                                && odPosledni < 300L;
                    } catch (Throwable ignored) {}
                    try {
                        if (tvPs1Pcm == null) tvPs1Pcm = new short[8192];
                        int got = NativePs1CoreBridge.pullTvAudioSafe(tvPs1Pcm);
                        // Vyzvedavat se musi VZDY, i kdyz se neposila - jinak
                        // by odbocka v jadre pretekala a po navratu do PS1 by
                        // na TV slo nekolik vterin stareho zvuku.
                        if (got > 0 && !jinyZdrojPravePosila) {
                            napTvWebAudioPush(tvPs1Pcm, 0, got, 44100, "PS1");
                        }
                    } catch (Throwable ignored) {}

                    boolean gotFromCore = napTvWebCaptureFromCore(bw, bh);
                    if (gotFromCore) {
                        napTvWebPixelCopyPending = false;
                    } else if (!didTimeoutFallback && pixelCopyAllowed) {
                        int[] loc = new int[2];
                        rootFrame.getLocationInWindow(loc);
                        Rect src = new Rect(loc[0], loc[1], loc[0] + sw, loc[1] + sh);
                        napTvWebPixelCopyPending = true;
                        napTvWebPixelCopyPendingAtMs = System.currentTimeMillis();
                        final long requestStartMs = napTvWebPixelCopyPendingAtMs;
                        final long requestGen = ++napTvWebPixelCopyRequestGen;
                        final Bitmap target = napTvWebBitmap;
                        final boolean hqLiteScreenFinal = hqLiteScreen;
                        android.view.PixelCopy.request(getWindow(), src, target, result -> {
                            try {
                                // BUILD2SK34: primy dukaz misto dalsiho hadani - kolik
                                // casu skutecne uplynulo mezi zadanim PixelCopy a jeho
                                // vysledkem (uspesnym i neuspesnym).
                                long latencyMs = System.currentTimeMillis() - requestStartMs;
                                if (latencyMs > 200L) {
                                    appendNativeLog("BUILD2SK34 TV_WEB_PIXELCOPY_SLOW latencyMs=" + latencyMs
                                            + " result=" + result + " hqLite=" + hqLiteScreenFinal + " tier=" + napTvWebQualityTier);
                                }
                                // BUILD2SK68: driv se logovaly jen "pomale" pripady (>200ms) -
                                // ale H264 diagnostika (SK66) ukazala, ze me enkodovani samo
                                // trva jen ~5ms, takze zbytek casu (u ~13fps to je ~70-80ms na
                                // snimek) se musi ztracet NEKDE JINDE. Tohle meri PRUMER pro
                                // UPLNE VSECHNY pozadavky (ne jen ty > 200ms), abych zjistil,
                                // jestli je to prave PixelCopy latence (i "normalni", ne jen
                                // vyjimecne pomala), co dominuje.
                                if (hqLiteScreenFinal) {
                                    napTvWebPcDiagSumMs += latencyMs;
                                    napTvWebPcDiagCount++;
                                    if (napTvWebPcDiagCount >= 30) {
                                        // BUILD2SK83: + tier=, viz vysvetleni u TICK_AVG.
                                        appendNativeLog("BUILD2SK83 TV_WEB_PIXELCOPY_AVG n=" + napTvWebPcDiagCount
                                                + " avgLatencyMs=" + (napTvWebPcDiagSumMs / napTvWebPcDiagCount)
                                                + " tier=" + napTvWebQualityTier);
                                        napTvWebPcDiagCount = 0; napTvWebPcDiagSumMs = 0;
                                    }
                                }
                                // BUILD2SK38: pokud uzivatel mezitim odesel na ne-hqLite
                                // obrazovku (generace se zvysila pri draw() volani), tenhle
                                // pozadavek uz je zastaraly - jeho vysledek ZAHODIME, misto
                                // aby prepsal cerstvy obraz z draw() starym PS1/Sega snimkem.
                                if (requestGen != napTvWebPixelCopyRequestGen) {
                                    appendNativeLog("BUILD2SK38 TV_WEB_PIXELCOPY_STALE_DISCARDED gen=" + requestGen + " current=" + napTvWebPixelCopyRequestGen);
                                    return;
                                }
                                if (result == android.view.PixelCopy.SUCCESS) {
                                    napTvWebPublishBitmap(target, "PIXELCOPY");
                                } else {
                                    appendNativeLog("BUILD2SA13C TV_WEB_PIXELCOPY_FAIL result=" + result + " hqLite=" + hqLiteScreenFinal);
                                    napTvWebPixelCopyDisabledUntilMs = System.currentTimeMillis() + (hqLiteScreenFinal ? 2000L : 5000L);
                                    if (!hqLiteScreenFinal) { ui.post(() -> napTvWebCaptureByDraw(bw, bh, scale)); }
                                }
                            } catch (Throwable t) {
                                appendNativeLog("BUILD2SA13C TV_WEB_PIXELCOPY_ERR " + safeMsg(t));
                                napTvWebPixelCopyDisabledUntilMs = System.currentTimeMillis() + 5000L;
                            } finally {
                                napTvWebPixelCopyPending = false;
                                napTvWebPixelCopyPendingAtMs = 0;
                            }
                        }, napTvWebCopyHandler);
                    } else if (!didTimeoutFallback) {
                        if (!hqLiteScreen) { napTvWebCaptureByDraw(bw, bh, scale); napTvWebPixelCopyRequestGen++; }
                        // hqLiteScreen==true: zmrazit posledni dobry snimek misto
                        // zobrazeni nevereho draw() obrazu (viz komentare vyse).
                        // BUILD2SK38: generace se zvysi PRI KAZDEM draw() volani - tim
                        // se oznaci jakykoli drive vydany (jeste neodpovezeny) hqLite
                        // PixelCopy pozadavek jako zastaraly. Kdyz pozde odpovi (viz
                        // callback nize), jeho vysledek se zahodi misto aby prepsal
                        // cerstvy obraz z draw().
                    }
                }
            } catch (Throwable t) {
                String errMsg = safeMsg(t);
                appendNativeLog("BUILD2SA13C TV_WEB_FRAME_ERR " + errMsg);
                napTvWebPixelCopyPending = false;
                napTvWebPixelCopyPendingAtMs = 0;
                // BUILD2SK24: "Window doesn't have a backing surface!" nastava behem
                // prechodu mezi obrazovkami (napr. opusteni PS1 - okno docasne nema
                // platny povrch, dokud se nova obrazovka neusadi). V logu bylo videt
                // 330x za sebou kazdych ~47ms po dobu pres 20 vterin - appka to
                // zkousela znovu tak rychle, ze si to uzivatel spravne vsiml jako
                // "zamrzlo a problikavalo". Misto normalniho rychleho retry pockame
                // pri TOMHLE konkretnim selhani dele, at ma okno cas se usadit.
                // BUILD2SK25: dalsi log ukazal, ze i s 700ms krokem umi tenhle stav
                // trvat pres 49 vterin (60+ pokusu) - typicky kdyz jde o vetsi
                // systemovou udalost (aktivita se pozastavi/obnovi, ne jen navigace
                // v ramci stranky). Pridana ESKALACE: kazde dalsi selhani v rade
                // prodlouzi dalsi cekani o 400ms, az do stropu 3s - takze dlouhy
                // zaseknuty stav se retryuje mnohem miň casto (odhadem ~3x mene
                // pokusu za stejnou dobu), zatimco kratke, rychle vyresene zaseknuti
                // porad zacne na puvodnich 700ms.
                if (errMsg != null && errMsg.contains("backing surface")) {
                    napTvWebBackingSurfaceFailStreak++;
                    extraDelay = Math.min(3000L, 700L + (long) (napTvWebBackingSurfaceFailStreak - 1) * 400L);
                } else {
                    napTvWebBackingSurfaceFailStreak = 0;
                }
            }
            if (extraDelay == 0) napTvWebBackingSurfaceFailStreak = 0;
            // BUILD2SK39: floor snizen z 35ms na 10ms - Rene chce primo overit,
            // jestli vyssi snimkova frekvence na HIGH neco meni (jeho hypoteza:
            // ne). Puvodni 35ms floor by ztrojnasobene HIGH hodnoty tise osekaval
            // zpet, takze by test nebyl platny - skutecna dosazitelna rychlost je
            // stejne omezena tim, jak dlouho trva zachytit+zkomprimovat snimek,
            // tenhle floor jen urcuje jak agresivne se appka o to snazi.
            // BUILD2SK71: puvodni frameDelayMs (50/40/33ms pro LOW/MEDIUM/HIGH)
            // byl navrzen driv, kdyz cilem bylo FPS OMEZIT na stabilni hodnotu.
            // Data z TICK_AVG diagnostiky (SK70) ukazala, ze skutecny cyklus
            // (~60ms) temer presne odpovida "target delay (50ms) + PixelCopy
            // latence navrch (~10-20ms)" - zadna skryta rezie, appka jen delala
            // presne to, co ji rekl predchozi pozadavek. Ted je cil OPACNY
            // (maximalizovat FPS), takze tohle umele zpozdeni ted brzdi presne
            // to, co ma byt rychlejsi. Pro PS1 s aktivnim H264 klientem: pouzit
            // mnohem mensi strop (5ms) - PixelCopyho vlastni tempo (~20-30ms)
            // se tak stane skutecnym limitujicim faktorem, misto umeleho stropu
            // navrch.
            // BUILD2SK80: BYVALO omezeno jen na PS1. Ted univerzalni - vsechen
            // obsah s aktivnim H264 klientem dostava stejny rychly cyklus, ne
            // jen PS1. Bez tohohle by ostatni obrazovky sice ziskaly H264
            // enkodovani, ale porad by byly brzdene starym umelym zpozdenim.
            long effectiveDelay = Math.max(10, napTvWebFrameDelayMs);
            try {
                if (!napTvWebH264ClientQueues.isEmpty()) {
                    // ===== POZOR: PS1 A SEGA POTREBUJI NECO JINEHO =====
                    // PS1 bere snimek PRIMO Z JADRA (napTvWebCaptureFromCore),
                    // takze mu staci 16 ms - snimek uz je hotovy a jen se
                    // preda enkoderu.
                    // SEGA (a Atari) zadnou takovou cestu nemaji - jejich obraz
                    // se musi SNIMAT Z OKNA aplikace, coz je pomalejsi a musi
                    // se to delat casteji, jinak vznika viditelne zpozdeni
                    // mezi mobilem a televizi.
                    // V B82 jsem tempo nastavil pevne na 16 ms pro vsechno a
                    // tim jsem Segu zpomalil. Ted se pevny strop pouzije JEN
                    // kdyz snimek jde z jadra PS1.
                    // Od B117 bere snimek primo z jadra i SEGA, takze patri
                    // do rychle vetve stejne jako PS1. Do B117 spadala do
                    // pomale (46 ms = 22 snimku za vterinu, i kdyz enkoder
                    // stihal na 3 ms) - odtud zbyle zpozdeni.
                    boolean ps1PrimoZJadra = false;
                    try {
                        String cu0 = (web == null) ? null : web.getUrl();
                        boolean naSegeT = (cu0 != null) && cu0.contains("emu_sega");
                        boolean naPs1T  = (cu0 != null) && cu0.contains("emu_ps1")
                                && (ps1BiosRunning || ps1SessionActive);
                        ps1PrimoZJadra = naPs1T || naSegeT;
                    } catch (Throwable ignored2) {}
                    effectiveDelay = ps1PrimoZJadra
                            ? napTvWebH264FastTickMs               // PS1: 16 ms staci
                            : Math.max(8, napTvWebFrameDelayMs / 2); // Sega/Atari: rychleji
                }
            } catch (Throwable ignored) {}
            // POZNAMKA K TEMPU: smycka bezi na hlavnim vlakne pres
            // ui.postDelayed, takze si rekne o 12 ms a dostane ~42 - ceka
            // na dokresleni obrazovky. Zkousel jsem to resit prednostnim
            // zarazenim (postAtFrontOfQueue) v B119: NEPOMOHLO, tempo
            // zustalo stejne a obraz na TV se dokonce zhorsil.
            // Prehodit smycku na vlastni vlakno NELZE - saha na rootFrame
            // a web.getUrl(), coz jsou pohledy aplikace.
            if (napTvWebRunning) ui.postDelayed(this, effectiveDelay + extraDelay);
        }
    };

    private volatile Bitmap napTvWebBitmapDraw;
    // ===== KROK F: snimek pro TV primo z PS1 jadra =====
    // Misto fotografovani obrazovky telefonu vezme framebuffer jadra
    // a jednim krokem ho roztahne na cely cil (cisty 16:9 bez pruhu
    // a bez ovladacich prvku). Vraci true, kdyz se to povedlo.
    private int[] tvCoreArgb = new int[1024 * 512];
    private Bitmap tvCoreSrcBmp;
    private boolean tvCoreHadFrame = false;
    // Je cely radek/sloupec cerny? (vzorkujeme po nekolika bodech, staci to
    // a je to levne - dela se to na malem originalu, ne na 1280x720)
    private static boolean napTvRowIsBlack(int[] px, int w, int y, int prah) {
        int base = y * w;
        for (int x = 0; x < w; x += 4) {
            int c = px[base + x];
            if (((c >> 16) & 0xFF) > prah || ((c >> 8) & 0xFF) > prah || (c & 0xFF) > prah) return false;
        }
        return true;
    }
    private static boolean napTvColIsBlack(int[] px, int w, int h, int x, int prah) {
        for (int y = 0; y < h; y += 4) {
            int c = px[y * w + x];
            if (((c >> 16) & 0xFF) > prah || ((c >> 8) & 0xFF) > prah || (c & 0xFF) > prah) return false;
        }
        return true;
    }
    private short[] tvPs1Pcm = null;   // odbocka zvuku pro TV
    private double tvSharpSumMs = 0;
    private long tvSharpFrames = 0;
    private long tvFpsT0 = 0;

    private boolean napTvWebCaptureFromCore(int bw, int bh) {
        try {
            // Prima cesta bezi -> javove snimani se VUBEC nedela.
            // Tim odpadnou vsechny tri kopie snimku v Jave (105 MB/s pri
            // 640x480 a 30 snimcich) a procesor zbude emulaci.
            if (tvPrimoBezi) return false;
            if (bw <= 0 || bh <= 0) return false;
            // Podrzeni posledniho snimku ma smysl jen ZA BEHU hry (kratky
            // vypadek pri nacitani). Kdyz uz hra nebezi, drzelo se to donekonecna
            // a na TV zustalo viset posledni logo - dalsi obsah uz se nikdy
            // neukazal. Ted se pri ukoncene hre vzdame a TV ukaze obrazovku appky.
            // ===== TADY BYL TEN OVLADAC NA TV =====
            // Chybelo tu ps1BiosRunning. Pri startu BIOSu bez disku neni
            // ps1SessionActive ani ps1GameWindowOwnsCore, takze se snimek
            // z jadra VUBEC NEVZAL a TV spadla na snimani CELEHO OKNA
            // aplikace - a v tom okne je ovladac. U hry to fungovalo, proto
            // tam ovladac videt nebyl.
            // Navic: snimani okna 1080x2220 kazdy snimek je drahe. Kdyz se
            // bere snimek z jadra (512x240), odpadne to uplne.
            // ===== ABY TV NEZUSTALA VISET NA POSLEDNIM SNIMKU Z PS1 =====
            // Podminka nize se pta jen na to, jestli BEZI JADRO. Jenze jadro
            // bezi dal i potom, co uzivatel z PS1 odejde do Atari nebo Segy -
            // takze si TV porad brala jeho snimek a zustala viset na poslednim
            // obrazku z PlayStation.
            // Ptame se proto i na to, jestli je na obrazovce OPRAVDU PS1.
            // Kdyz ne, TV se vrati ke snimani okna aplikace a ukaze, co ma.
            {
                String u = napTvWebCurrentUrl;
                // Od B117 plati i pro Segu - ta uz taky dava snimek primo
                // z jadra, takze ji sem musime pustit.
                boolean naPs1 = (u != null) && (u.contains("emu_ps1") || u.contains("emu_sega"));
                if (!naPs1) {
                    if (tvCoreHadFrame) appendNativeLog("TV_ZPET_NA_OKNO (odchod z PS1)");
                    tvCoreHadFrame = false;
                    return false;
                }
            }
            // U Segy neplati priznaky PS1 - ta ma vlastni jadro. Kdyz jsme
            // na jeji obrazovce, pustime ji dal a snimek si vezme nize.
            boolean naSege = false;
            try {
                String u2 = (web == null) ? null : web.getUrl();
                naSege = (u2 != null) && u2.contains("emu_sega");
            } catch (Throwable ignored) {}
            if (!naSege && !ps1SessionActive && !ps1GameWindowOwnsCore && !ps1BiosRunning) {
                tvCoreHadFrame = false;
                return false;
            }
            // Nejdriv si PUJCIME snimek, ktery uz vytahla obrazovka telefonu.
            // Setri to jadru praci (drive sahali do jadra dva zajemci zvlast
            // a jadro pak nestihalo delat zvuk - 47 % pokusu naslo prazdno).
            // ===== SNIMEK PRO TV: PS1 NEBO SEGA =====
            // Obe jadra ted davaji hotovy snimek stejnym zpusobem, takze
            // dalsi zpracovani (orez, zvetseni, enkoder) je spolecne.
            // Snimat kvuli Sege okno aplikace uz neni potreba - prave to
            // zpusobovalo zpozdeni mezi mobilem a televizi.
            boolean jeSegaTv = false;
            try {
                String uS = (web == null) ? null : web.getUrl();
                jeSegaTv = (uS != null) && uS.contains("emu_sega");
            } catch (Throwable ignored) {}

            int wh = jeSegaTv ? NativeSegaCoreBridge.grabFrameSafe(tvCoreArgb)
                              : Ps1GlTextureView.borrowFrame(tvCoreArgb);
            if (wh < 0) {
                int need = ((-wh) >> 16) * ((-wh) & 0xFFFF);
                tvCoreArgb = new int[need + 1024];
                wh = jeSegaTv ? NativeSegaCoreBridge.grabFrameSafe(tvCoreArgb)
                              : Ps1GlTextureView.borrowFrame(tvCoreArgb);
            }
            if (wh == 0) {
                // Nic k pujceni (obrazovka zrovna nekresli) - podrzime posledni
                // dobry snimek misto sahani do jadra navic.
                if (tvCoreHadFrame && napTvWebBitmapDraw != null && !napTvWebBitmapDraw.isRecycled()) {
                    napTvWebPublishBitmap(napTvWebBitmapDraw, "CORE_HOLD");
                    return true;
                }
                return false;
            }
            if (wh <= 0 || (wh >> 16) <= 0 || (wh & 0xFFFF) <= 0) {
                // Jadro zrovna nema snimek (typicky pri nacitani hry).
                // Misto propadnuti na fotografovani obrazovky (= cerno a 0 FPS)
                // radeji znovu odevzdame POSLEDNI dobry snimek.
                if (tvCoreHadFrame && napTvWebBitmapDraw != null && !napTvWebBitmapDraw.isRecycled()) {
                    napTvWebPublishBitmap(napTvWebBitmapDraw, "CORE_HOLD");
                    return true;
                }
                return false;
            }
            int sw = wh >> 16, sh = wh & 0xFFFF;

            // ZRYCHLENI: driv se tu projizdelo vsech ~307 000 pixelu kazdy snimek
            // jen kvuli vynuceni pruhlednosti. Ted to same zaridi jeden prikaz
            // setHasAlpha(false) - bitmapa pruhlednost proste ignoruje.
            if (tvCoreSrcBmp == null || tvCoreSrcBmp.getWidth() != sw || tvCoreSrcBmp.getHeight() != sh) {
                if (tvCoreSrcBmp != null) { try { tvCoreSrcBmp.recycle(); } catch (Throwable ignored) {} }
                tvCoreSrcBmp = Bitmap.createBitmap(sw, sh, Bitmap.Config.ARGB_8888);
                try { tvCoreSrcBmp.setHasAlpha(false); } catch (Throwable ignored) {}
            }
            // ===== DOOSTRENI (Laplace) pred zvetsenim =====
            // Delame ho na MALEM originalu (320x240 / 640x480), ne na velkem
            // 1280x720 - je to tim mnohonasobne levnejsi. Uspora navic: rozdil
            // pocitame jen ze zelene slozky (nejlepsi zastupce jasu) a stejny
            // rozdil pak pridame vsem trem barvam. Vysledek oku stejny,
            // prace zhruba tretinova. Cena se meri a vypisuje do logu.
            int nPix = sw * sh;
            long shStart = System.nanoTime();
            // Doostreni stoji 1,6-4,2 ms u hernich scen, ale 7,5 ms u filmovych
            // (640x480). Zvuk uz hladovi (underruns), takze u velkych snimku
            // doostreni preskocime - film je stejne mekky od prirody.
            // ===== DVE ZBYTECNE KOPIE SNIMKU PRYC (B93) =====
            // Drive se tu delalo:
            //   1) arraycopy tvCoreArgb -> tvSharpBuf     (kopie 1)
            //   2) doostrovani bod po bodu                (vypnuto v B92)
            //   3) setPixels tvSharpBuf -> Bitmap         (kopie 2)
            // Doostrovani je pryc, takze tvSharpBuf byla jen kopie originalu
            // a k nicemu. U snimku 640x480 to bylo 307 tisic kopii bodu
            // NAVIC na kazdy snimek, pri 30 snimcich za vterinu 35 MB/s
            // presypani z jedne pameti do druhe.
            // Ted se pracuje primo s tvCoreArgb.
            //
            // POZOR: KDYZ SE SEM BUDE NECO VRACET, MERIT TO. Cesta na TV
            // sdili procesor s emulaci - co se ukousne tady, chybi zvuku.
            tvCoreSrcBmp.setPixels(tvCoreArgb, 0, sw, 0, 0, sw, sh);

            // ===== PEVNYCH 1280x720 = SKUTECNE 16:9 =====
            // Driv se velikost brala z displeje telefonu (1384x672), coz je
            // ale pomer 2,06:1 - ne 16:9! Obraz na TV byl proto roztazeny o
            // ~16 % do sirky (obliceje sirsi) a televize k tomu jeste pridavala
            // pruhy nahore a dole. Ted posilame vzdycky cistych 1280x720:
            // spravny pomer, bez pruhu, nezavisle na otoceni telefonu.
            // ===== ZVETSENI PRESNE NA DVOJNASOBEK =====
            // Bylo 1280x720. Zdroj z her je typicky 640x480, takze:
            //   vodorovne 640 -> 1280 = presne 2x  (kazdy bod na 2x2, ostre)
            //   svisle    480 ->  720 = 1,5x       <- TADY se to kazilo
            // Pri 1,5x pripadnou na kazde dva body tri: jeden se zdvoji,
            // sousedni ne. Vznikne nepravidelny vzor, ktery H.264 spatne
            // komprimuje - a v tmavych plochach z nej delaji kostky.
            // 1280x960 je presne 2x v obou smerech. Kazdy bod PlayStation
            // se zvetsi na ctverec 2x2 a obraz zustane cisty.
            // Prohlizec si to dorovna na svuj pomer sam (CSS), takze se
            // na obrazovce nic nezmeni.
            // POMER STRAN: 1280x720 je 16:9, tedy presne to, co ma dnesni
            // televize i monitor - obraz vyplni obrazovku bez pruhu.
            // (V B103 tu bylo 1280x960 kvuli presnemu zdvojeni 640x480, ale
            //  to je 4:3 a na siroke obrazovce zbyly cerne pruhy po stranach.
            //  Ostrost resi jinak: zdroj se roztahne na sirku bez filtrovani,
            //  takze pixely zustanou ostre i pri jinem pomeru.)
            final int TVW = 1280, TVH = 720;
            if (napTvWebBitmapDraw == null || napTvWebBitmapDraw.getWidth() != TVW
                    || napTvWebBitmapDraw.getHeight() != TVH) {
                if (napTvWebBitmapDraw != null) { try { napTvWebBitmapDraw.recycle(); } catch (Throwable ignored) {} }
                napTvWebBitmapDraw = Bitmap.createBitmap(TVW, TVH, Bitmap.Config.ARGB_8888);
                appendNativeLog("G TV prenos prepnut na pevnych " + TVW + "x" + TVH + " (skutecne 16:9)");
            }
            Canvas cv = new Canvas(napTvWebBitmapDraw);
            cv.drawColor(Color.BLACK);
            // ===== ZVETSENI BEZ ROZMAZANI =====
            // FILTER_BITMAP_FLAG prumeruje sousedni body. U fotky je to
            // spravne, u obrazu z PlayStation ne - ten ma ostre pixely a
            // prumerovanim se rozmaze. Navic rozmazany obraz se hur
            // komprimuje, takze to ubira i datovy tok.
            // Bez filtru se kazdy bod jen zvetsi - presne jak to vypada
            // na skutecne konzoli.
            // ===== VYLADENI PRED KOMPRESI =====
            // Rene mel pravdu se srovnanim se starym videem prevedenym na HD:
            // TAM SE TO DELA PRED KOMPRESI, na originalu, kde je jeste vsechna
            // informace. Ja to v B105 zkusil AZ POTOM, na obraze, kteremu uz
            // detaily chybely - proto to jen zvyraznilo chyby komprese.
            //
            // Tady je to spravne: obraz z jadra je CISTY ORIGINAL (zadny sum,
            // zadna ztrata) a upravuje se JESTE PRED tim, nez ho enkoder
            // zabali. Pocita to grafika telefonu pri kresleni na platno,
            // takze to nestoji skoro nic - v logu ma telefon rezervu 10 ms
            // ze 16.
            //
            // CO SE DELA:
            //  1) mirne vyhlazeni pri zvetseni. Ostre schody se pri kompresi
            //     rozpadaji na kostky (H.264 ma s ostrymi hranami problem),
            //     zatimco mirne mekci obraz se komprimuje CISTEJI - a vysledek
            //     na televizi je pak PARADOXNE ostrejsi.
            //  2) lehke zvyseni kontrastu a sytosti, aby obraz nepusobil
            //     vybledle po tom vyhlazeni.
            // Vysledek: obraz "jako film", ne jako zvetsene pixely.
            Paint pp = new Paint();
            pp.setDither(false);
            if (napTvVyhlazeni) {
                // Jen vyhlazeni pri zvetseni - to umi Canvas levne, je to
                // jedina operace pri kresleni.
                pp.setFilterBitmap(true);
            }
            // ===== BAREVNY FILTR ODSTRANEN - BYL TO TEN DUVOD TRHANI =====
            // V B108 tu byl navic ColorMatrixColorFilter (kontrast a sytost).
            // Vypadalo to lepe, ALE: tohle platno je "new Canvas(bitmapa)",
            // tedy platno V PAMETI, ne na grafice. Barevny filtr se proto
            // pocital PROCESOREM, bod po bodu, u 640x480 tristatisickrat na
            // kazdy snimek. V logu se to projevilo jako avgTickGapMs=96 misto
            // 16 - devet snimku za vterinu misto sedesati, odtud trhani.
            // (Enkoder pritom stihal, avgDrawMs=3.)
            // Kontrast a sytost si nastav v panelu v prohlizeci - tam to
            // pocita grafika PC a telefon to nestoji nic.
            // ===== OREZ CERNYCH OKRAJU =====
            // Hry casto kresli obraz do MENSI plochy, nez je zobrazovaci okno
            // PS1 (typicky 224 radku uvnitr 240) a zbytek nechaji cerny. Ty
            // cerne pasy pak putovaly az na televizi. Najdeme skutecny obsah
            // a roztahneme na celou plochu - odtud "full HD bez pruhu".
            int cropTop = 0, cropBottom = sh - 1, cropLeft = 0, cropRight = sw - 1;
            final int TMAVA = 12;   // co je temnejsi, bereme jako cernou
            while (cropTop < cropBottom && napTvRowIsBlack(tvCoreArgb, sw, cropTop, TMAVA)) cropTop++;
            while (cropBottom > cropTop && napTvRowIsBlack(tvCoreArgb, sw, cropBottom, TMAVA)) cropBottom--;
            while (cropLeft < cropRight && napTvColIsBlack(tvCoreArgb, sw, sh, cropLeft, TMAVA)) cropLeft++;
            while (cropRight > cropLeft && napTvColIsBlack(tvCoreArgb, sw, sh, cropRight, TMAVA)) cropRight--;
            // Pojistka: kdyby byl obraz skoro cely cerny (tma ve hre), neorezavat.
            if (cropBottom - cropTop < sh / 3 || cropRight - cropLeft < sw / 3) {
                cropTop = 0; cropBottom = sh - 1; cropLeft = 0; cropRight = sw - 1;
            }
            Rect srcRect = new Rect(cropLeft, cropTop, cropRight + 1, cropBottom + 1);
            long tKres = System.nanoTime();
            cv.drawBitmap(tvCoreSrcBmp, srcRect, new Rect(0, 0, TVW, TVH), pp);
            // Kolik stoji samotne kresleni (vcetne vyhlazeni). Kdyz to poleze
            // nad ~8 ms, vyhlazeni je prilis drahe a ma se vypnout.
            {
                napTvKresDiagSum += (System.nanoTime() - tKres) / 1000000;
                if (++napTvKresDiagPocet >= 60) {
                    appendNativeLog("TV_KRESLENI prumer=" + (napTvKresDiagSum / napTvKresDiagPocet)
                            + " ms  vyhlazeni=" + (napTvVyhlazeni ? "ZAP" : "VYP"));
                    napTvKresDiagSum = 0; napTvKresDiagPocet = 0;
                }
            }

            // CHYBELO: bitmapu je potreba jeste ODEVZDAT do prenosu.
            // Bez tohohle radku se obraz naplnil, ale TV o nem nevedela
            // a cekala na starou prazdnou bitmapu -> cerna a 0 FPS.
            tvCoreHadFrame = true;
            napTvWebPublishBitmap(napTvWebBitmapDraw, "CORE");
            return true;
        } catch (Throwable t) {
            return false;
        }
    }


    private void napTvWebCaptureByDraw(int bw, int bh, float scale) {
        try {
            // BUILD2SK48: tahle funkce ted pouziva SVOJI VLASTNI bitmapu
            // (napTvWebBitmapDraw), NIKDY sdilenou s PixelCopy cestou
            // (napTvWebBitmap). Driv sdilely jednu bitmapu, coz v portretu (kde
            // ruzne obrazovky casto vychazi na stejnou velikost, takze se bitmapa
            // NIKDY neprestavela) mohlo zpusobit soubeh: PixelCopy z PREDCHOZI
            // hqLite obrazovky (Sega/PS1/MP3/...) jeste asynchronne zapisovalo do
            // bitmapy, zatimco draw() (pro NOVOU, ne-hqLite obrazovku jako domovska
            // stranka) do TE SAME bitmapy kreslilo - nepredvidatelny vysledek,
            // presne odpovidajici hlasenemu "skace do MP3 prehravace misto domu",
            // "pamatuje si Segu jako pilotni obrazovku".
            if (napTvWebBitmapDraw == null || napTvWebBitmapDraw.getWidth() != bw || napTvWebBitmapDraw.getHeight() != bh) {
                if (napTvWebBitmapDraw != null) {
                    // BUILD2SK40: recyklace stare bitmapy pred vytvorenim nove -
                    // tahle bitmapa neni sdilena s PixelCopy, takze recyklace muze
                    // byt VZDY bezpecna (na rozdil od napTvWebBitmap, kde jeste
                    // musime kontrolovat napTvWebPixelCopyPending).
                    try { napTvWebBitmapDraw.recycle(); } catch (Throwable ignored) {}
                }
                napTvWebBitmapDraw = Bitmap.createBitmap(bw, bh, Bitmap.Config.ARGB_8888);
            }
            Canvas cv = new Canvas(napTvWebBitmapDraw);
            cv.drawColor(Color.BLACK);
            cv.save();

            // ===== OREZ NA OKENKO ZRUSEN (bylo v B109-B111) =====
            // Zkousel jsem posilat na TV jen vyrez okenka konzole. Rozbilo to
            // vsechno ostatní: mobil na vysku posilal na web pohled na sirku
            // a naopak, Atari i Sega se orezavaly spatne. Snimani okna ma
            // vlastni logiku pro otoceni a merítko (promenna scale nize),
            // do ktere ten vyrez zasahoval.
            // V B104 to fungovalo spravne - vracim to.
            // Obraz PS1 se na TV posila JINOU cestou (napTvWebCaptureFromCore),
            // ktera bere snimek primo z jadra a okno appky vubec neresi.
            cv.scale(scale, scale);
            // BUILD2SK44: invalidate() pred draw() - pokus vynutit cerstve
            // prekresleni misto zastarale hardwarove vrstvy.
            if (rootFrame != null) rootFrame.invalidate();
            if (rootFrame != null) rootFrame.draw(cv);
            cv.restore();
            napTvWebPublishBitmap(napTvWebBitmapDraw, "DRAW");
        } catch (Throwable t) {
            appendNativeLog("BUILD2SA13C TV_WEB_DRAW_CAPTURE_ERR " + safeMsg(t));
        }
    }

    private void napTvWebPublishBitmap(Bitmap bm, String mode) {
        try {
            // BUILD2SK69: driv se JPEG komprese delala VZDY, i pro PS1 snimky,
            // kdyz H264 uz aktivne bezelo - nikdo MJPEG v tu chvili nesleduje,
            // takze to byla CISTA ZBYTECNA PRACE na kazdem jednom snimku.
            // Ted: pokud je tohle PS1 A H264 klient je pripojeny, JPEG
            // komprese se preskoci UPLNE (napTvWebJpeg proste zustane
            // nezmeneny - stary snimek - dokud se nekdo neodpoji od H264,
            // pak se MJPEG cesta zase prirozene obnovi).
            // BUILD2SK80: BYVALO omezeno jen na PS1 ("isPs1Now &&"). Rene se
            // rozhodl obejit problem neexistujicich kvalitnich hacku uvnitr
            // uzavrenych nativnich jader (Sega/PS1 bridge) tim, ze se
            // UNIFIKUJE cely vystup pres uz overenou H264 cestu - misto
            // MJPEG pro vsechno KROME PS1, ted H264 pro VSECHNO, kde je
            // pripojeny klient. hqLiteScreen je uz univerzalni (SK49), takze
            // tohle prirozene pokryva PS1, Segu, Atari, DJ pult, MP3,
            // domovskou stranku - cokoli, co se prave zachytava.
            String curUrlNow = napTvWebCurrentUrl; // BUILD2SK80: uz se nepouziva pro rozhodnuti (H264 je univerzalni), zustava pro pripadnou budouci diagnostiku
            boolean h264Handling = !napTvWebH264ClientQueues.isEmpty();

            long prevPublishMs = napTvWebLastFrameMs;
            if (!h264Handling) {
                long compressStart = System.currentTimeMillis();
                ByteArrayOutputStream bos = new ByteArrayOutputStream(Math.max(32768, bm.getWidth() * bm.getHeight() / 8));
                bm.compress(Bitmap.CompressFormat.JPEG, Math.max(35, Math.min(94, napTvWebJpegQuality)), bos);
                long compressMs = System.currentTimeMillis() - compressStart;
                napTvWebCompressDiagSumMs += compressMs;
                napTvWebCompressDiagCount++;
                if (napTvWebCompressDiagCount >= 30) {
                    appendNativeLog("BUILD2SK68 TV_WEB_COMPRESS_AVG n=" + napTvWebCompressDiagCount
                            + " avgCompressMs=" + (napTvWebCompressDiagSumMs / napTvWebCompressDiagCount)
                            + " w=" + bm.getWidth() + " h=" + bm.getHeight());
                    napTvWebCompressDiagCount = 0; napTvWebCompressDiagSumMs = 0;
                }
                napTvWebJpeg = bos.toByteArray();
                napTvWebSeq++;
                napTvWebLastFrameMs = System.currentTimeMillis();
                long gapMs = prevPublishMs == 0 ? 0 : (napTvWebLastFrameMs - prevPublishMs);
                // BUILD2SK34: primy dukaz misto dalsiho hadani. Zaznamenej, kdyz
                // samotna JPEG komprese trva podezrele dlouho NEBO kdyz mezi dvema
                // po sobe jdoucimi snimky ubehla podezrele velka mezera - naznacuje
                // zaseknuti NEKDE v ceste, i kdyz zadna vyjimka nenastala (Rene
                // opakovane potvrdil, ze problem je jen na MEDIUM/HIGH a v logu
                // dosud nebyla vidět zadna chyba - tohle by mohlo byt proste fyzicke
                // zpomaleni pri vetsim rozliseni na starsim S8 hardwaru, ne logicka
                // chyba, a takove zpomaleni by se v dosavadnim logu vubec neprojevilo).
                if (compressMs > 200L || gapMs > 400L) {
                    appendNativeLog("BUILD2SK34 TV_WEB_SLOW_FRAME mode=" + mode + " compressMs=" + compressMs
                            + " gapMs=" + gapMs + " w=" + bm.getWidth() + " h=" + bm.getHeight()
                            + " q=" + napTvWebJpegQuality + " tier=" + napTvWebQualityTier);
                }
            } else {
                // H264 obsluhuje - JPEG preskoceno, ale seq/frameMs se pořád
                // aktualizuji (jine casti kodu - napr. /status, stale-detekce -
                // na nich zavisi at uz je aktivni JPEG nebo H264 cesta).
                napTvWebSeq++;
                napTvWebLastFrameMs = System.currentTimeMillis();
            }
            // BUILD2SK57: H.264 stream pro PS1 - beh JEN kdyz (a) aktualni
            // obrazovka je PS1 A (b) aspon jeden klient je pripojeny na
            // /stream.h264 (jinak by se zbytecne plytvalo CPU na enkodovani,
            // ktere nikdo nesleduje).
            if (h264Handling) {
                try { napTvWebH264FeedFrame(bm); } catch (Throwable ignored) {}
            }
            // BUILD2SK67: overeni podezreni - produkuje PS1 emulator (WASM
            // jadro renderujici do <canvas> uvnitr WebView) skutecne NOVY
            // obsah tak rychle, jak se ho snazime zachytit? Pokud emulator
            // sam bezi pomaleji nez muj zachytavaci cyklus, opakovane bych
            // zachytaval a zbytecne enkodoval IDENTICKE snimky - v tom
            // pripade by zadna oprava H264 pipeline nepomohla, protoze
            // bottleneck by byl driv, nez se moje pipeline vubec dostane
            // ke slovu. Levna kontrola: porovnat par vzorkovanych pixelu
            // (ne cely snimek - to by samo pridalo naklad) s minulym
            // snimkem. Beží nezavisle na H264 - i pro MJPEG chceme vedet,
            // jestli zachytavame duplicity.
            try {
                {
                    int bw = bm.getWidth(), bh = bm.getHeight();
                    if (bw > 8 && bh > 8) {
                        long sample = 0;
                        int brightSum = 0; // BUILD2SK143: viz komentar nize
                        for (int si = 0; si < 8; si++) {
                            int sx = (bw * si) / 8, sy = (bh * si) / 8;
                            int px = bm.getPixel(sx, sy);
                            sample = sample * 31 + px;
                            // BUILD2SK143: Rene poslal video s KVANTIFIKOVANYM dukazem -
                            // jednotlive snimky pravidelne (mnohokrat za 14s) propadaji na
                            // ~30% normalniho jasu a hned se vraci - presny "problikavajici
                            // cerny snimek" vzorec. Nevime, jestli se to deje uz TADY (v
                            // bitmape, kterou PixelCopy zachytil) nebo az pozdeji v H264
                            // enkodovani (napTvWebH264FeedFrame). Tohle je NEJDRIVEJSI
                            // misto, kde muzeme jas zmerit - pokud uz TADY vychazi nizko,
                            // problem je v zachytavani (PixelCopy/WebView), ne v enkoderu.
                            int r = (px >> 16) & 0xFF, g = (px >> 8) & 0xFF, b = px & 0xFF;
                            brightSum += (r + g + b) / 3;
                        }
                        int brightAvg = brightSum / 8; // 0-255
                        napTvWebDupCheckCount++;
                        if (sample == napTvWebLastSampleHash) napTvWebDupCheckSame++;
                        napTvWebLastSampleHash = sample;
                        // BUILD2SK143: OKAMZITY log (ne prumerovany) kdyzkoliv je vzorek
                        // podezrele tmavy vuci normalu (~60-90 podle predchozich logu) -
                        // schvalne nizky prah (30), abychom nezachytili jen legitimne
                        // tmava herni sceny, jen ty opravdu vypadajici na "temer cerny
                        // snimek".
                        if (brightAvg < 20) {
                            appendNativeLog("BUILD2SK143 TV_WEB_DARK_FRAME_SPIKE brightAvg=" + brightAvg + " mode=" + mode + " w=" + bw + " h=" + bh);
                        }
                        if (napTvWebDupCheckCount >= 60) {
                            appendNativeLog("BUILD2SK67 TV_WEB_PS1_DUPCHECK sameFrames=" + napTvWebDupCheckSame
                                    + "/" + napTvWebDupCheckCount + " url=" + napTvWebCurrentUrl + " (pokud je toto cislo vysoke, zdroj nevykresluje"
                                    + " novy obsah tak rychle, jak ho zachytavame)");
                            napTvWebDupCheckCount = 0; napTvWebDupCheckSame = 0;
                        }
                    }
                }
            } catch (Throwable ignored) {}
        } catch (Throwable t) {
            appendNativeLog("BUILD2SA13C TV_WEB_PUBLISH_ERR " + safeMsg(t));
        }
    }

    // === BUILD2SK57: H.264 PS1 STREAM - ENKODOVACI PIPELINE ===
    private void napTvWebH264EnsureEncoder(int w, int h) {
        w = w - (w % 2); h = h - (h % 2); // 4:2:0 vyzaduje sude rozmery
        if (w <= 0 || h <= 0) return;
        synchronized (napTvWebH264Lock) {
            if (napTvWebH264Encoder != null && napTvWebH264W == w && napTvWebH264H == h) return;
            napTvWebH264ReleaseEncoderLocked();
            try {
                MediaFormat fmt = MediaFormat.createVideoFormat(MediaFormat.MIMETYPE_VIDEO_AVC, w, h);
                // BUILD2SK75: COLOR_FormatSurface misto YUV420Planar - rikame
                // enkoderu, ze vstup prijde jako Surface (GPU), ne jako rucne
                // pripravene bajty. Enkoder pak RGB->YUV prevod dela SAM,
                // interne, hardwarove - zadna Java smycka po pixelech, zadne
                // soutezeni o CPU s PS1 emulaci. Tohle je presne ta "berlicka",
                // o ktere Rene mel podezreni - puvodni rucni YUV konverze (i
                // kdyz sama o sobe rychla, ~3-5ms) bezela 4x castej po
                // odstraneni umeleho stropu (SK71), a soutezila o CPU s
                // emulatorem samotnym.
                fmt.setInteger(MediaFormat.KEY_COLOR_FORMAT, MediaCodecInfo.CodecCapabilities.COLOR_FormatSurface);

                // ===== PLNY BAREVNY ROZSAH =====
                // H.264 ma ve vychozim stavu "televizni" rozsah: jas jen
                // 16 az 235 misto 0 az 255. Obraz z PlayStation ma plny
                // rozsah, takze se stlacil do uzsiho - a v TMAVYCH PLOCHACH
                // pak zbylo min urovni na jemne stinovani, coz se rozpadlo
                // do kostek. Presne to bylo videt na intru NFS a na Dolby
                // logu, zatimco ostre napisy (MEMORY CARD) byly v poradku -
                // ty maji plny kontrast a rozsah jim nevadi.
                // Tohle je ta hlavni pricina kostek, ne datovy tok.
                if (android.os.Build.VERSION.SDK_INT >= 24) {
                    try {
                        fmt.setInteger(MediaFormat.KEY_COLOR_RANGE,
                                MediaFormat.COLOR_RANGE_FULL);
                        fmt.setInteger(MediaFormat.KEY_COLOR_STANDARD,
                                MediaFormat.COLOR_STANDARD_BT709);
                        fmt.setInteger(MediaFormat.KEY_COLOR_TRANSFER,
                                MediaFormat.COLOR_TRANSFER_SDR_VIDEO);
                    } catch (Throwable ignored) {}
                }
                // BUILD2SK77: podlaha 700000 -> 1800000 (SK76, cileno na LOW) ->
                // nasobitel taky zvysen 4 -> 6 - Rene potvrdil, ze na telefonu
                // samotnem (nekomprimovany zdroj) grafika vypada skvele, takze
                // rozdil, ktery vidi, je nejspis ztrata VERNOSTI pri kompresi/
                // prenosu, ne problem se zdrojem. Vic datoveho toku = mene
                // kompresnich artefaktu pri stejnem rozliseni.
                // ===== DATOVY TOK PRO OSTRY OBRAZ =====
                // Bylo tu w*h*6, coz u 1280x720 dela 5,5 Mbit/s. To je malo
                // pro obraz PlayStation: ma ostre pixely a v pohybu (napr.
                // NFS) detailni textury - H.264 to pri nizkem toku rozmaze.
                // Rene srovnaval s originalni PS1 na CRT a mel pravdu, ze
                // web viewer ma co dohanet.
                // 20 bitu na bod = 18 Mbit/s pri 1280x720. Pres wifi 5 GHz
                // to projde a enkoder to stiha (v logu avgDrawMs=2).
                // 30 bitu na bod = 27 Mbit/s pri 1280x720. Enkoder ma rezervu
                // (v logu avgDrawMs=3 az 4 pri strope 16 ms) a wifi 5 GHz to
                // unese. Kdyby se to kouslo, snizit na w*h*20.
                // 22 bitu na bod = 27 Mbit/s pri 1280x960. Pri presnem
                // zdvojeni se obraz komprimuje LIP (pravidelne ctverce 2x2),
                // takze na stejnou kvalitu staci mensi koeficient nez driv.
                fmt.setInteger(MediaFormat.KEY_BIT_RATE, Math.max(8000000, w * h * 30));
                // Enkoder dostava snimky po ~16 ms (napTvWebH264FastTickMs),
                // tedy 60 za vterinu. Sedi to.
                fmt.setInteger(MediaFormat.KEY_FRAME_RATE, 60);

                // ===== KLICOVY SNIMEK KAZDE 2 VTERINY, NE KAZDOU =====
                // Bylo tu 1 = klicovy snimek KAZDOU VTERINU. Klicovy snimek
                // je asi desetkrat drazsi nez bezny a pri pevnem datovem toku
                // si vezme velkou cast rozpoctu - na snimky kolem nej pak
                // nezbude a ROZPADNOU SE DO KOSTEK. Nejvic je to videt
                // v tmavych plochach (silnice, pozadi), kde ma H.264 malo
                // detailu na schovani chyby.
                fmt.setInteger(MediaFormat.KEY_I_FRAME_INTERVAL, 2);

                // ===== PROMENNY DATOVY TOK =====
                // Ve vychozim stavu drzi enkoder tok konstantni - i kdyz je
                // scena klidna, cpe do ni bity, a kdyz je slozita, uz je
                // nema kde vzit. Promenny tok (VBR) da bity tam, kde jsou
                // potreba: v pohybu a v detailech.
                try {
                    fmt.setInteger(MediaFormat.KEY_BITRATE_MODE,
                        MediaCodecInfo.EncoderCapabilities.BITRATE_MODE_VBR);
                } catch (Throwable ignored) {}
                // BUILD2SK77: Baseline -> Main profil. Baseline byl zvolen driv
                // kvuli "nejsirsi kompatibilite", ale bez konkretniho duvodu, proc
                // by ji appka potrebovala - Chrome (a prakticky kazdy moderni
                // prohlizec) zvladne Main bez problemu. Main pouziva CABAC
                // (pokrocilejsi entropiove kodovani) - citelne lepsi kvalita pri
                // STEJNEM datovem toku, presne cileno na "obraz na telefonu je
                // super, ale po ceste se neco ztrati".
                // POZOR/RIZIKO: Main profil UMOZNUJE B-snimky, o kterych vim (z
                // drivejsiho vyzkumu pri navrhu architektury), ze delaji problemy
                // JMuxer.js/MSE muxovani na klientovi. Zkousim pozadat o
                // nizko-latencni rezim (kde by hardware enkoder typicky B-snimky
                // sam preskocil kvuli jejich vlastni latenci), pokud to API
                // urovne dovoli - neni to zaruka, jen dalsi pojistka.
                if (Build.VERSION.SDK_INT >= 23) {
                    // High profil umi jemnejsi prechody nez Main - pomaha
                    // prave v tmavych plochach, kde vznikaly kostky.
                    try { fmt.setInteger(MediaFormat.KEY_PROFILE,
                            MediaCodecInfo.CodecProfileLevel.AVCProfileHigh); }
                    catch (Throwable ig) {
                        try { fmt.setInteger(MediaFormat.KEY_PROFILE,
                                MediaCodecInfo.CodecProfileLevel.AVCProfileMain); } catch (Throwable ig2) {}
                    }
                }
                if (Build.VERSION.SDK_INT >= 30) {
                    try { fmt.setInteger(MediaFormat.KEY_LATENCY, 0); } catch (Throwable ignored) {}
                }
                MediaCodec enc = MediaCodec.createEncoderByType(MediaFormat.MIMETYPE_VIDEO_AVC);
                enc.configure(fmt, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE);
                // POZOR NA PORADI: createInputSurface() MUSI byt zavolano PO
                // configure() a PRED start() - takhle to vyzaduje Android API.
                android.view.Surface inputSurface = enc.createInputSurface();
                enc.start();
                napTvWebH264Encoder = enc;
                napTvWebH264InputSurface = inputSurface;
                // Okno enkoderu se nativni casti preda AZ NA KONCI teto
                // metody - viz nize. Tady enkoder jeste nemusi bezet.
                napTvWebH264W = w; napTvWebH264H = h;
                napTvWebH264FrameIndex = 0;
                napTvWebH264Generation++;
                // BUILD2SK83: + bitrate=/tier= - drive se muselo dopocitavat rucne z w*h,
                // ted primo v logu (a rovnou i jestli to byla podlaha 1.8M nebo w*h*6).
                // ===== TADY BYL TEN PAD =====
                // Okno enkoderu jsem predaval do C JESTE PRED tim, nez byl
                // enkoder nastartovany. V logu to bylo videt:
                //     23:53:51.547  TV_PRIMO_ZAPNUTO
                //     23:53:51.552  H264_ENCODER_START     <- az POTOM
                // Nativni vlakno zacalo kreslit do enkoderu, ktery jeste
                // nebezel -> pad. Ted se okno preda AZ TED, kdyz uz enkoder
                // opravdu bezi (enc.start() probehlo vyse).
                // ===== PRIMA CESTA NA TV JE VYPNUTA =====
                // Snimek z C primo do enkoderu (B94-B98) shazoval aplikaci
                // pri zapnuti TV. Ctyri pokusy, ctyri pady. Priciny, ktere
                // jsem nasel a opravil (poradi startu enkoderu, format okna,
                // zamek, souběh vlaken) byly skutecne, ale zjevne tam je
                // jeste neco dalsiho - a chovani dvou vlaken s MediaCodec
                // si u sebe overit nedokazu.
                // TV proto jede zase javovou cestou, kde PROKAZATELNE
                // FUNGOVALA (B92). Nativni kod zustava v projektu, jen se
                // nespousti - kdyby ho nekdo chtel dodelat, staci prepnout
                // tenhle priznak a otestovat s telefonem po ruce.
                final boolean POUZIT_PRIMOU_CESTU_NA_TV = false;
                if (POUZIT_PRIMOU_CESTU_NA_TV
                        && (ps1Plocha != null || ps1BiosRunning || ps1SessionActive)) {
                    NativePs1CoreBridge.setTvSurfaceSafe(inputSurface);
                    tvPrimoBezi = true;
                    appendNativeLog("TV_PRIMO_ZAPNUTO");
                }
                appendNativeLog("BUILD2SK83 TV_WEB_H264_ENCODER_START w=" + w + " h=" + h + " gen=" + napTvWebH264Generation
                        + " mode=SURFACE bitrate=" + Math.max(8000000, w * h * 30) + " tier=" + napTvWebQualityTier);   // musi souhlasit s KEY_BIT_RATE vyse!
            } catch (Throwable t) {
                appendNativeLog("BUILD2SK57 TV_WEB_H264_ENCODER_FAIL " + safeMsg(t));
                napTvWebH264Encoder = null;
            }
        }
    }

    private void napTvWebH264ReleaseEncoderLocked() {
        // Enkoder konci (napr. klient spadl na JPEG zalohu). Odpojime
        // nativni kresleni a vratime javovou cestu, at TV nezustane cerna.
        if (tvPrimoBezi) {
            tvPrimoBezi = false;
            try { NativePs1CoreBridge.setTvSurfaceSafe(null); } catch (Throwable ignored) {}
            appendNativeLog("TV_PRIMO_VYPNUTO - vracim javovou cestu");
        }
        if (napTvWebH264Encoder != null) {
            try { napTvWebH264Encoder.stop(); } catch (Throwable ignored) {}
            try { napTvWebH264Encoder.release(); } catch (Throwable ignored) {}
            napTvWebH264Encoder = null;
        }
        if (napTvWebH264InputSurface != null) {
            try { napTvWebH264InputSurface.release(); } catch (Throwable ignored) {}
            napTvWebH264InputSurface = null;
        }
        napTvWebH264W = 0; napTvWebH264H = 0;
    }

    // BUILD2SK64: PUVODNI napTvWebH264FeedFrame delala YUV prevod +
    // enkodovani SYNCHRONNE, primo volane z hlavni zachytavaci smycky
    // (UI vlakno). Rucne psana YUV konverze je cisty Java kod bez
    // hardwaroveho zrychleni - u landscape rozliseni (1384x672 = pres
    // 900 000 pixelu) to realisticky muze trvat desitky milisekund,
    // COZ PRIMO BLOKOVALO dalsi zachytavaci cyklus. Ted: hlavni smycka
    // jen ZKOPIRUJE bitmapu (rychla nativni operace) a preda ji
    // samostatnemu pracovnimu vlaknu - YUV+enkodovani uz nikdy
    // neblokuje zachytavani. Pokud pracovni vlakno nestiha, proste se
    // novejsi snimek prepise pres jeste nezpracovany - stejny princip
    // "zahodit stare, ne cekat", jaky uz MJPEG cesta pouziva.
    private void napTvWebH264FeedFrame(Bitmap bm) {
        if (bm == null) return;
        napTvWebH264EnsureWorker();
        Bitmap copy;
        long t0 = System.nanoTime();
        try {
            Bitmap.Config cfg = bm.getConfig() != null ? bm.getConfig() : Bitmap.Config.ARGB_8888;
            copy = bm.copy(cfg, false);
        } catch (Throwable t) { return; }
        napTvWebH264DiagCopyMs += (System.nanoTime() - t0) / 1000000; // BUILD2SK66: hruby soucet, jen pro diagnostiku - drobna zavodni podminka mezi vlakny je tu prijatelna
        Bitmap old;
        synchronized (napTvWebH264PendingLock) {
            old = napTvWebH264PendingBitmap;
            napTvWebH264PendingBitmap = copy;
        }
        if (old != null) { try { old.recycle(); } catch (Throwable ignored) {} }
    }

    private void napTvWebH264EnsureWorker() {
        if (napTvWebH264WorkerThread != null && napTvWebH264WorkerThread.isAlive()) return;
        napTvWebH264WorkerRunning = true;
        napTvWebH264WorkerThread = new Thread(() -> {
            while (napTvWebH264WorkerRunning) {
                Bitmap bm;
                synchronized (napTvWebH264PendingLock) {
                    bm = napTvWebH264PendingBitmap;
                    napTvWebH264PendingBitmap = null;
                }
                if (bm == null) {
                    try { Thread.sleep(4); } catch (InterruptedException ie) { break; }
                    continue;
                }
                try { napTvWebH264FeedFrameInternal(bm); }
                catch (Throwable t) { appendNativeLog("BUILD2SK64 TV_WEB_H264_WORKER_ERR " + safeMsg(t)); }
                finally { try { bm.recycle(); } catch (Throwable ignored) {} }
            }
        }, "nap-h264-worker");
        napTvWebH264WorkerThread.setDaemon(true);
        napTvWebH264WorkerThread.start();
    }

    // BUILD2SK65: opakovane pouzivane buffery MISTO nove alokace pri
    // KAZDEM snimku. Puvodni kod delal "new int[w*h]" a "new byte[...]"
    // pri kazdem volani - u landscape rozliseni (~900 000 pixelu) to je
    // pres 5 MB odpadu PER SNIMEK. I pri "jen" 20fps je to >100 MB/s
    // odpadu, coz nutne vede k narustajicimu tlaku na garbage collector
    // cim dal hur, jak stream bezi dele - presne to, co jsi popsal jako
    // "casem dochazi k drops FPS". Tyhle buffery pouziva VYHRADNE
    // pracovni H264 vlakno (zadna dalsi synchronizace potreba), a
    // preallokuji se znovu jen kdyz se skutecne zmeni rozmery (coz je
    // vzacne, ne kazdy snimek).
    private int[] napTvWebH264PxBuf = null;
    private byte[] napTvWebH264YuvBuf = null;
    private int napTvWebH264BufW = 0, napTvWebH264BufH = 0;

    private long napTvWebH264DiagFrameCount = 0;
    private long napTvWebH264DiagCopyMs = 0, napTvWebH264DiagPixelsMs = 0, napTvWebH264DiagYuvMs = 0, napTvWebH264DiagDequeueMs = 0, napTvWebH264DiagDrainMs = 0;

    private void napTvWebH264FeedFrameInternal(Bitmap bm) {
        int w = bm.getWidth() - (bm.getWidth() % 2);
        int h = bm.getHeight() - (bm.getHeight() % 2);
        if (w <= 0 || h <= 0) return;
        napTvWebH264EnsureEncoder(w, h);
        MediaCodec enc;
        android.view.Surface inputSurface;
        synchronized (napTvWebH264Lock) { enc = napTvWebH264Encoder; inputSurface = napTvWebH264InputSurface; }
        if (enc == null || inputSurface == null) return;
        try {
            // BUILD2SK75: BYVALO getPixels() + rucni YUV smycka (~3-5ms Java
            // kodu na kazdy snimek, ktery ted po SK71 bezi 4x castej nez
            // predtim - primo souteziv o CPU s PS1 emulaci). TED: bitmapa se
            // vykresli PRIMO na Surface enkoderu pres hardwarove
            // akcelerovany Canvas - GPU dela RGB->YUV prevod sam, interne.
            // Zadna Java smycka po pixelech vubec.
            // Kdyz minuly snimek trval pres strop, tenhle vynechame - enkoder
            // je zahlceny a cekani na nej by zdrzelo celou aplikaci.
            if (napTvWebH264PosledniMs > NAP_TV_H264_STROP_MS) {
                napTvWebH264PosledniMs = 0;          // pristi snimek zase zkusime
                napTvWebH264Vynechano++;
                if ((napTvWebH264Vynechano % 120) == 1) {
                    appendNativeLog("TV_H264_VYNECHAVAM snimky, enkoder nestiha (celkem "
                            + napTvWebH264Vynechano + ")");
                }
                return;
            }
            long t0 = System.nanoTime();
            android.graphics.Canvas canvas = inputSurface.lockHardwareCanvas();
            try {
                canvas.drawBitmap(bm, 0, 0, null);
            } finally {
                inputSurface.unlockCanvasAndPost(canvas);
            }
            long t1 = System.nanoTime();
            napTvWebH264DrainEncoder(enc);
            long t2 = System.nanoTime();
            napTvWebH264PosledniMs = (t1 - t0) / 1000000;
            napTvWebH264DiagPixelsMs += (t1 - t0) / 1000000; // ted: cas kresleni na Surface (byvale "pixels+yuv")
            napTvWebH264DiagDrainMs += (t2 - t1) / 1000000;
            napTvWebH264DiagFrameCount++;
            long totalMs = (t2 - t0) / 1000000;
            if (totalMs > 80) {
                appendNativeLog("BUILD2SK75 TV_WEB_H264_FRAME_SLOW totalMs=" + totalMs
                        + " drawMs=" + ((t1 - t0) / 1000000) + " drainMs=" + ((t2 - t1) / 1000000)
                        + " w=" + w + " h=" + h + " tier=" + napTvWebQualityTier);
            }
            if (napTvWebH264DiagFrameCount >= 30) {
                // BUILD2SK83: + tier= (w/h uz tu byly, tier chybel).
                appendNativeLog("BUILD2SK83 TV_WEB_H264_FRAME_AVG n=" + napTvWebH264DiagFrameCount
                        + " avgDrawMs=" + (napTvWebH264DiagPixelsMs / napTvWebH264DiagFrameCount)
                        + " avgDrainMs=" + (napTvWebH264DiagDrainMs / napTvWebH264DiagFrameCount)
                        + " w=" + w + " h=" + h + " tier=" + napTvWebQualityTier);
                napTvWebH264DiagFrameCount = 0;
                napTvWebH264DiagCopyMs = 0; napTvWebH264DiagPixelsMs = 0; napTvWebH264DiagYuvMs = 0; napTvWebH264DiagDequeueMs = 0; napTvWebH264DiagDrainMs = 0;
            }
        } catch (Throwable t) {
            appendNativeLog("BUILD2SK57 TV_WEB_H264_FEED_ERR " + safeMsg(t));
        }
    }

    // BT.601 studio-range RGB->YUV420 Planar (I420: cely Y, pak cele U, pak
    // cele V). Celociselna aritmetika (bez plovouci carky) kvuli rychlosti -
    // bezi kazdy snimek, u vetsich rozliseni by float verze mohla byt znatelne
    // pomalejsi. BUILD2SK65: ted zapisuje do JIZ EXISTUJICIHO bufferu
    // (predaneho jako parametr) misto aby si sam alokoval novy.
    private void napTvWebH264RgbToYuv420(int[] argb, byte[] out, int w, int h) {
        int uOff = w * h, vOff = w * h + (w * h) / 4;
        for (int j = 0; j < h; j++) {
            int rowBase = j * w;
            for (int i = 0; i < w; i++) {
                int p = argb[rowBase + i];
                int r = (p >> 16) & 0xFF, g = (p >> 8) & 0xFF, b = p & 0xFF;
                int y = ((66 * r + 129 * g + 25 * b + 128) >> 8) + 16;
                out[rowBase + i] = (byte) (y < 0 ? 0 : (y > 255 ? 255 : y));
                if ((j & 1) == 0 && (i & 1) == 0) {
                    int u = ((-38 * r - 74 * g + 112 * b + 128) >> 8) + 128;
                    int v = ((112 * r - 94 * g - 18 * b + 128) >> 8) + 128;
                    int idx = (j / 2) * (w / 2) + (i / 2);
                    out[uOff + idx] = (byte) (u < 0 ? 0 : (u > 255 ? 255 : u));
                    out[vOff + idx] = (byte) (v < 0 ? 0 : (v > 255 ? 255 : v));
                }
            }
        }
    }

    private void napTvWebH264DrainEncoder(MediaCodec enc) {
        MediaCodec.BufferInfo info = new MediaCodec.BufferInfo();
        while (true) {
            int outIdx;
            try { outIdx = enc.dequeueOutputBuffer(info, 0); } catch (Throwable t) { break; }
            if (outIdx == MediaCodec.INFO_TRY_AGAIN_LATER) break;
            if (outIdx == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED) continue;
            if (outIdx < 0) continue;
            try {
                ByteBuffer outBuf = enc.getOutputBuffer(outIdx);
                if (outBuf != null && info.size > 0) {
                    byte[] chunk = new byte[info.size];
                    outBuf.position(info.offset);
                    outBuf.limit(info.offset + info.size);
                    outBuf.get(chunk);
                    napTvWebH264Seq++;
                    napTvWebH264LastFrameMs = System.currentTimeMillis();
                    for (java.util.concurrent.LinkedBlockingQueue<byte[]> q : napTvWebH264ClientQueues) {
                        q.offer(chunk);
                    }
                }
            } finally {
                try { enc.releaseOutputBuffer(outIdx, false); } catch (Throwable ignored) {}
            }
        }
    }

    private void napTvWebWriteH264Stream(OutputStream out, Socket sock) throws IOException {
        String h = "HTTP/1.1 200 OK\r\n"
                + "Content-Type: video/h264\r\n"
                + "Cache-Control: no-store, no-cache, must-revalidate, max-age=0\r\n"
                + "Pragma: no-cache\r\n"
                + "Connection: close\r\n\r\n";
        out.write(h.getBytes("ISO-8859-1"));
        java.util.concurrent.LinkedBlockingQueue<byte[]> myQueue = new java.util.concurrent.LinkedBlockingQueue<>(300);
        napTvWebH264ClientQueues.add(myQueue);
        appendNativeLog("BUILD2SK57 TV_WEB_H264_CLIENT_CONNECT clients=" + napTvWebH264ClientQueues.size());
        try {
            long myGen = -1;
            while (napTvWebRunning && !sock.isClosed()) {
                byte[] chunk;
                try { chunk = myQueue.poll(2000, java.util.concurrent.TimeUnit.MILLISECONDS); }
                catch (InterruptedException ie) { break; }
                if (chunk == null) continue;
                long curGen = napTvWebH264Generation;
                if (myGen == -1) {
                    myGen = curGen; // prvni prijata data - navazat se na aktualni generaci enkoderu
                } else if (curGen != myGen) {
                    // BUILD2SK63: enkoder mezitim restartoval (jina generace =
                    // jine SPS/PPS) - AKTIVNE ukoncit spojeni misto tichy
                    // posilat klientovi nekompatibilni data uprostred streamu.
                    // Klient (SK63 oprava) tohle spravne rozpozna jako konec
                    // streamu a pripoji se znovu s cerstvym JMuxerem.
                    appendNativeLog("BUILD2SK63 TV_WEB_H264_GEN_CHANGE old=" + myGen + " new=" + curGen);
                    break;
                }
                out.write(chunk);
                out.flush();
            }
        } finally {
            napTvWebH264ClientQueues.remove(myQueue);
            if (napTvWebH264ClientQueues.isEmpty()) {
                synchronized (napTvWebH264Lock) { napTvWebH264ReleaseEncoderLocked(); }
            }
            appendNativeLog("BUILD2SK57 TV_WEB_H264_CLIENT_DISCONNECT clients=" + napTvWebH264ClientQueues.size());
        }
    }

    private void napTvWebEnsurePlaceholderFrame(String reason) {
        if (napTvWebJpeg != null) return;
        Bitmap bm = null;
        int oldQ = napTvWebJpegQuality;
        try {
            bm = Bitmap.createBitmap(640, 360, Bitmap.Config.ARGB_8888);
            Canvas cv = new Canvas(bm);
            Paint p = new Paint(Paint.ANTI_ALIAS_FLAG);
            cv.drawColor(Color.BLACK);
            p.setColor(Color.rgb(92, 220, 255));
            p.setTextSize(28);
            p.setFakeBoldText(true);
            cv.drawText("AtariHelp TV WEB CAST", 32, 78, p);
            p.setFakeBoldText(false);
            p.setTextSize(19);
            p.setColor(Color.rgb(210, 235, 245));
            cv.drawText("Cekam na prvni snimek z telefonu...", 32, 122, p);
            p.setTextSize(15);
            p.setColor(Color.rgb(120, 165, 190));
            cv.drawText(reason == null ? "START" : reason, 32, 164, p);
            napTvWebJpegQuality = 72;
            napTvWebPublishBitmap(bm, "PLACEHOLDER");
        } catch (Throwable t) {
            appendNativeLog("BUILD2SA13C11 TV_WEB_PLACEHOLDER_ERR " + safeMsg(t));
        } finally {
            napTvWebJpegQuality = oldQ;
            try { if (bm != null) bm.recycle(); } catch (Throwable ignored) {}
        }
    }

    private float napTvWebClampFloat(double v, float min, float max) {
        if (Double.isNaN(v) || Double.isInfinite(v)) return min;
        return (float)Math.max(min, Math.min(max, v));
    }

    private synchronized String napTvWebSetSoundStateJson(String json) {
        try {
            if (json == null || json.length() == 0) return "TV_WEB_EQ_EMPTY";
            org.json.JSONObject o = new org.json.JSONObject(json);
            org.json.JSONArray eq = o.optJSONArray("eq");
            if (eq != null) {
                for (int i = 0; i < napTvWebEqGains.length && i < eq.length(); i++) {
                    napTvWebEqGains[i] = napTvWebClampFloat(eq.optDouble(i, 0.0), -12f, 12f);
                }
            }
            napTvWebBassGain = napTvWebClampFloat(o.optDouble("bass", napTvWebBassGain), -12f, 12f);
            napTvWebTrebleGain = napTvWebClampFloat(o.optDouble("treble", napTvWebTrebleGain), -12f, 12f);
            napTvWebBalance = napTvWebClampFloat(o.optDouble("balance", napTvWebBalance), -1f, 1f);
            napTvWebVolume = napTvWebClampFloat(o.optDouble("volume", napTvWebVolume), 0f, 1.4f);
            appendNativeLog("BUILD2SA13C18 TV_WEB_SOUND_STATE eq="
                    + napTvWebEqGains[0] + "," + napTvWebEqGains[1] + "," + napTvWebEqGains[2] + "," + napTvWebEqGains[3] + "," + napTvWebEqGains[4]
                    + " bass=" + napTvWebBassGain + " treble=" + napTvWebTrebleGain
                    + " balance=" + napTvWebBalance + " volume=" + napTvWebVolume);
            return "TV_WEB_EQ_OK";
        } catch (Throwable t) {
            appendNativeLog("BUILD2SA13C18 TV_WEB_SOUND_STATE_FAIL " + safeMsg(t));
            return "TV_WEB_EQ_FAIL " + safeMsg(t);
        }
    }

    private synchronized String napTvWebEqJsArray() {
        return "[" + String.format(Locale.US, "%.3f,%.3f,%.3f,%.3f,%.3f",
                napTvWebEqGains[0], napTvWebEqGains[1], napTvWebEqGains[2], napTvWebEqGains[3], napTvWebEqGains[4]) + "]";
    }

    private String napTvWebFloatJs(float v) {
        return String.format(Locale.US, "%.3f", v);
    }

    private void napTvWebOpenYoutubeInApp() {
        final String url = "https://m.youtube.com/";
        napTvWebStart();
        napTvWebYoutubeInAppUntilMs = System.currentTimeMillis() + 60L * 60L * 1000L;
        ui.post(() -> {
            try {
                if (web == null) return;
                stopNativeInPlaceHard("tvWebYoutubeInApp");
                stopPs1SessionHard("tvWebYoutubeInApp");
                applyWebViewVisualMode(url, "tvWebYoutubeInApp");
                web.loadUrl(url);
                appendNativeLog("BUILD2SA13C13 YOUTUBE_IN_APP_OPEN url=" + url);
                napTvWebScheduleYoutubeAudioBridge("openYoutubeInApp");
            } catch (Throwable t) {
                appendNativeLog("BUILD2SA13C13 YOUTUBE_IN_APP_OPEN_FAIL " + safeMsg(t));
            }
        });
    }

    private void napTvWebScheduleYoutubeAudioBridge(String reason) {
        if (web == null) return;
        ui.postDelayed(() -> napTvWebInjectYoutubeAudioBridge(reason + ":900"), 900);
        ui.postDelayed(() -> napTvWebInjectYoutubeAudioBridge(reason + ":2200"), 2200);
        ui.postDelayed(() -> napTvWebInjectYoutubeAudioBridge(reason + ":5200"), 5200);
    }

    private void napTvWebInjectYoutubeAudioBridge(String reason) {
        if (web == null) return;
        try {
            final String eqJs = napTvWebEqJsArray();
            final String bassJs = napTvWebFloatJs(napTvWebBassGain);
            final String trebleJs = napTvWebFloatJs(napTvWebTrebleGain);
            final String balanceJs = napTvWebFloatJs(napTvWebBalance);
            final String volumeJs = napTvWebFloatJs(napTvWebVolume);
            String js =
                    "(function(){try{var AHTV_EQ=" + eqJs
                    + ",AHTV_BASS=" + bassJs
                    + ",AHTV_TREBLE=" + trebleJs
                    + ",AHTV_BAL=" + balanceJs
                    + ",AHTV_VOL=" + volumeJs + ";"
                    + "var AHTV_STATE=window.__AHTV_YT_SOUND_STATE||{eq:(AHTV_EQ||[0,0,0,0,0]).slice(0),bass:AHTV_BASS||0,treble:AHTV_TREBLE||0,balance:AHTV_BAL||0,volume:(AHTV_VOL==null?1:AHTV_VOL)};window.__AHTV_YT_SOUND_STATE=AHTV_STATE;"
                    + "function ahtvClamp(v,min,max){v=parseFloat(v);if(!isFinite(v))v=0;return Math.max(min,Math.min(max,v));}"
                    + "function ahtvSave(){try{if(window.AHTVWEB&&window.AHTVWEB.setSoundState)window.AHTVWEB.setSoundState(JSON.stringify(AHTV_STATE));}catch(_e){}}"
                    + "function ahtvApply(br){try{"
                    + "var st=window.__AHTV_YT_SOUND_STATE||AHTV_STATE;var eg=st.eq||[0,0,0,0,0];"
                    + "if(br.filters){for(var i=0;i<br.filters.length;i++){br.filters[i].gain.value=eg[i]||0;}}"
                    + "if(br.bass)br.bass.gain.value=st.bass||0;"
                    + "if(br.treble)br.treble.gain.value=st.treble||0;"
                    + "if(br.pan)br.pan.pan.value=ahtvClamp(st.balance||0,-1,1);"
                    + "if(br.gain)br.gain.gain.value=ahtvClamp(st.volume==null?1:st.volume,0,1.4);"
                    + "}catch(_e){}}"
                    + "function ahtvInstallControls(){try{"
                    + "if(document.getElementById('ahtvEqBtn'))return;"
                    + "var d=document,stop=function(e){try{e.stopPropagation();}catch(_e){}};"
                    + "var btn=d.createElement('button');btn.id='ahtvEqBtn';btn.type='button';btn.textContent='EQ';"
                    + "btn.style.cssText='position:fixed;right:8px;bottom:84px;z-index:2147483647;background:#06131d;color:#9ee9ff;border:1px solid #71d9ff;border-radius:8px;padding:9px 11px;font:800 13px monospace;box-shadow:0 0 16px #000;opacity:.95;';"
                    + "var panel=d.createElement('div');panel.id='ahtvEqPanel';panel.style.cssText='display:none;position:fixed;left:10px;right:10px;bottom:120px;max-height:62vh;overflow:auto;z-index:2147483647;background:#04121c;color:#d8f6ff;border:2px solid #71d9ff;border-radius:12px;padding:12px 14px;font:700 13px monospace;box-shadow:0 8px 40px rgba(0,0,0,.95);box-sizing:border-box;';"
                    + "var head=d.createElement('div');head.textContent='LIVE YOUTUBE EQ';head.style.cssText='color:#ffe07a;font-size:14px;margin-bottom:8px;';panel.appendChild(head);"
                    + "function row(name,min,max,step,get,set){var w=d.createElement('div');w.style.cssText='margin:8px 0 12px;';var top=d.createElement('div');top.style.cssText='display:flex;justify-content:space-between;align-items:center;margin-bottom:3px;';var l=d.createElement('span');l.textContent=name;l.style.cssText='color:#9fdcff;';var val=d.createElement('span');val.style.cssText='color:#fff;min-width:56px;text-align:right;font-size:14px;';top.appendChild(l);top.appendChild(val);var r=d.createElement('input');r.type='range';r.min=min;r.max=max;r.step=step;r.value=get();r.style.cssText='width:100%;display:block;height:30px;box-sizing:border-box;margin:0;';function upd(){val.textContent=(+r.value).toFixed(step<1?(step<0.1?2:1):0);}upd();r.oninput=function(e){stop(e);set(+r.value);upd();ahtvApply(window.__AHTV_YT_AUDIO_BRIDGE);ahtvSave();};w.appendChild(top);w.appendChild(r);panel.appendChild(w);}"
                    + "row('VOL',0,1.4,.01,function(){return AHTV_STATE.volume==null?1:AHTV_STATE.volume;},function(v){AHTV_STATE.volume=v;});"
                    + "row('BASS',-12,12,.5,function(){return AHTV_STATE.bass||0;},function(v){AHTV_STATE.bass=v;});"
                    + "row('TREBLE',-12,12,.5,function(){return AHTV_STATE.treble||0;},function(v){AHTV_STATE.treble=v;});"
                    + "row('BALANCE',-1,1,.05,function(){return AHTV_STATE.balance||0;},function(v){AHTV_STATE.balance=v;});"
                    + "var bands=[['60',0],['250',1],['1K',2],['4K',3],['16K',4]];for(var bi=0;bi<bands.length;bi++){(function(n,i){row('EQ '+n,-12,12,.5,function(){return(AHTV_STATE.eq||[0,0,0,0,0])[i]||0;},function(v){AHTV_STATE.eq=AHTV_STATE.eq||[0,0,0,0,0];AHTV_STATE.eq[i]=v;});})(bands[bi][0],bands[bi][1]);}"
                    + "var reset=d.createElement('button');reset.type='button';reset.textContent='RESET';reset.style.cssText='width:48%;margin-top:4px;padding:8px;background:#1b2a36;color:#fff;border:1px solid #3f5666;border-radius:6px;font:800 12px monospace;';"
                    + "var close=d.createElement('button');close.type='button';close.textContent='ZAVRIT';close.style.cssText='width:48%;float:right;margin-top:4px;padding:8px;background:#1b2a36;color:#fff;border:1px solid #3f5666;border-radius:6px;font:800 12px monospace;';"
                    + "var orig=d.createElement('button');orig.type='button';orig.textContent='1:1 ORIGINAL';orig.style.cssText='width:100%;margin:6px 0 4px;padding:9px;background:#0d3b1e;color:#c8ffd6;border:1px solid #3fae66;border-radius:6px;font:800 13px monospace;';"
                    + "orig.onclick=function(e){stop(e);AHTV_STATE.eq=[0,0,0,0,0];AHTV_STATE.bass=0;AHTV_STATE.treble=0;AHTV_STATE.balance=0;AHTV_STATE.volume=1;ahtvSave();location.reload();};panel.appendChild(orig);"
                    + "reset.onclick=function(e){stop(e);AHTV_STATE.eq=[0,0,0,0,0];AHTV_STATE.bass=0;AHTV_STATE.treble=0;AHTV_STATE.balance=0;AHTV_STATE.volume=1;ahtvApply(window.__AHTV_YT_AUDIO_BRIDGE);ahtvSave();try{btn.parentNode&&btn.parentNode.removeChild(btn);panel.parentNode&&panel.parentNode.removeChild(panel);}catch(_e){}ahtvInstallControls();};"
                    + "close.onclick=function(e){stop(e);panel.style.display='none';};panel.appendChild(reset);panel.appendChild(close);"
                    + "btn.onclick=function(e){stop(e);var show=panel.style.display==='none';panel.style.display=show?'block':'none';if(show&&window.__ahtvEnsureGraph){window.__ahtvEnsureGraph();}};"
                    + "['click','pointerdown','touchstart','touchmove'].forEach(function(ev){btn.addEventListener(ev,stop,false);panel.addEventListener(ev,stop,false);});"
                    + "(d.body||d.documentElement).appendChild(btn);(d.body||d.documentElement).appendChild(panel);"
                    + "}catch(_e){}}"
                    + "ahtvInstallControls();"
                    // BUILD2SA14: ORIGINAL FIRST - audio graf se NESTAVI hned. YouTube hraje
                    // 100% nedotceny original, dokud uzivatel poprve neotevre EQ panel.
                    + "window.__ahtvEnsureGraph=function(){try{"
                    + "if(window.__AHTV_YT_AUDIO_BRIDGE&&window.__AHTV_YT_AUDIO_BRIDGE.ok)return 'graph_ready';"
                    + "var v=document.querySelector('video');"
                    + "if(!v)return 'no_video';"
                    + "var old=window.__AHTV_YT_AUDIO_BRIDGE;"
                    + "if(old&&old.ok&&old.video===v){ahtvApply(old);return 'updated_live_eq';}"
                    + "var C=window.AudioContext||window.webkitAudioContext;"
                    + "if(!C)return 'no_audio_context';"
                    + "var ctx=window.__AHTV_YT_AUDIO_CTX||new C();window.__AHTV_YT_AUDIO_CTX=ctx;"
                    + "try{if(ctx.resume)ctx.resume();}catch(_r){}"
                    + "var src=v.__ahtvSrc||ctx.createMediaElementSource(v);v.__ahtvSrc=src;"
                    + "var freqs=[60,250,1000,4000,16000],filters=[],node=src;"
                    + "for(var fi=0;fi<freqs.length;fi++){var biq=ctx.createBiquadFilter();biq.type='peaking';biq.frequency.value=freqs[fi];biq.Q.value=1.0;biq.gain.value=0;node.connect(biq);node=biq;filters.push(biq);}"
                    + "var bass=ctx.createBiquadFilter();bass.type='lowshelf';bass.frequency.value=180;bass.gain.value=0;node.connect(bass);node=bass;"
                    + "var treble=ctx.createBiquadFilter();treble.type='highshelf';treble.frequency.value=3500;treble.gain.value=0;node.connect(treble);node=treble;"
                    + "var pan=ctx.createStereoPanner?ctx.createStereoPanner():null,gain=ctx.createGain();gain.gain.value=1;"
                    + "if(pan){node.connect(pan);pan.connect(gain);}else{node.connect(gain);}"
                    + "var tap=ctx.createScriptProcessor(2048,2,2);"
                    + "window.__AHTV_YT_AUDIO_Q=window.__AHTV_YT_AUDIO_Q||[];"
                    + "window.__AHTV_YT_AUDIO_DRAIN=window.__AHTV_YT_AUDIO_DRAIN||setInterval(function(){try{"
                    + "var q=window.__AHTV_YT_AUDIO_Q||[];"
                    + "if(!q.length||!window.AHTVWEB||!window.AHTVWEB.pushYoutubePcm16)return;"
                    + "var n=Math.min(2,q.length);"
                    + "for(var qi=0;qi<n;qi++){var item=q.shift();window.AHTVWEB.pushYoutubePcm16(item.b64,item.rate,item.frames,2);}"
                    + "if(q.length>10)q.splice(0,q.length-10);"
                    + "}catch(_e){}},38);"
                    + "tap.onaudioprocess=function(e){try{"
                    + "var input=e.inputBuffer,output=e.outputBuffer,frames=input.length;"
                    + "var l=input.getChannelData(0),r=input.numberOfChannels>1?input.getChannelData(1):l;"
                    + "for(var c=0;c<output.numberOfChannels;c++){var o=output.getChannelData(c),inp=c===0?l:r;for(var i=0;i<frames;i++)o[i]=inp[i];}"
                    + "var q=window.__AHTV_YT_AUDIO_Q;if(!q)return;"
                    + "var bytes=new Uint8Array(frames*4),peak=0;"
                    + "for(var i=0,j=0;i<frames;i++,j+=4){"
                    + "var lv=Math.max(-1,Math.min(1,l[i])),rv=Math.max(-1,Math.min(1,r[i]));"
                    + "peak=Math.max(peak,Math.abs(lv),Math.abs(rv));"
                    + ""
                    + "var li=lv<0?Math.round(lv*32768):Math.round(lv*32767);"
                    + "var ri=rv<0?Math.round(rv*32768):Math.round(rv*32767);"
                    + "bytes[j]=li&255;bytes[j+1]=(li>>8)&255;bytes[j+2]=ri&255;bytes[j+3]=(ri>>8)&255;"
                    + "}"
                    + "if(peak<0.0005)return;"
                    + "var s='',step=4096;for(var p=0;p<bytes.length;p+=step)s+=String.fromCharCode.apply(null,bytes.subarray(p,p+step));"
                    + "q.push({b64:btoa(s),rate:ctx.sampleRate||44100,frames:frames|0});"
                    + "if(q.length>16)q.splice(0,q.length-16);"
                    + "}catch(_e){}};"
                    + "gain.connect(ctx.destination);var mute=ctx.createGain();mute.gain.value=0;gain.connect(tap);tap.connect(mute);mute.connect(ctx.destination);"
                    + "var br={ok:true,at:Date.now(),video:v,filters:filters,bass:bass,treble:treble,pan:pan,gain:gain};"
                    + "window.__AHTV_YT_AUDIO_BRIDGE=br;ahtvApply(br);ahtvInstallControls();"
                    + "document.addEventListener('click',function(){try{ctx.resume&&ctx.resume();}catch(_e){}},true);"
                    + "return 'ok_live_eq';"
                    + "}catch(e){return 'fail '+(e&&e.message?e.message:e);}};"
                    + "return 'controls_ready_ORIGINAL_SOUND_untouched';"
                    + "}catch(e){return 'fail '+(e&&e.message?e.message:e);}})()";
            web.evaluateJavascript(js, value -> appendNativeLog("BUILD2SA13C18 YOUTUBE_AUDIO_BRIDGE_LIVE_EQ reason=" + reason + " result=" + value));
        } catch (Throwable t) {
            appendNativeLog("BUILD2SA13C18 YOUTUBE_AUDIO_BRIDGE_INJECT_FAIL " + safeMsg(t));
        }
    }

    private void napTvWebRequestSystemMirror() {
        napTvWebRequestSystemMirror(null);
    }

    private void napTvWebRequestSystemMirror(String afterUrl) {
        if (Build.VERSION.SDK_INT < 21) {
            appendNativeLog("BUILD2SA13C9 SCREEN_MIRROR_UNSUPPORTED sdk=" + Build.VERSION.SDK_INT);
            return;
        }
        if (afterUrl != null && afterUrl.trim().length() > 0) {
            napTvWebPendingScreenUrl = afterUrl.trim();
        }
        if (napTvWebSystemMirrorActive) {
            String url = napTvWebPendingScreenUrl;
            napTvWebPendingScreenUrl = null;
            if (url != null && url.length() > 0) {
                ui.postDelayed(() -> openRawExternalBrowserUrl(url), 350);
            }
            return;
        }
        if (napTvWebSystemMirrorRequested) return;
        napTvWebSystemMirrorRequested = true;
        ui.post(() -> {
            try {
                if (napTvWebProjectionManager == null) {
                    napTvWebProjectionManager = (MediaProjectionManager)getSystemService(MEDIA_PROJECTION_SERVICE);
                }
                if (napTvWebProjectionManager == null) throw new RuntimeException("MediaProjectionManager null");
                appendNativeLog("BUILD2SA13C9 SCREEN_MIRROR_PERMISSION_REQUEST");
                startActivityForResult(napTvWebProjectionManager.createScreenCaptureIntent(), PICK_TV_WEB_SCREEN);
            } catch (Throwable t) {
                napTvWebSystemMirrorRequested = false;
                appendNativeLog("BUILD2SA13C9 SCREEN_MIRROR_PERMISSION_FAIL " + safeMsg(t));
            }
        });
    }

    private void napTvWebStartSystemMirror(int resultCode, Intent data) {
        if (Build.VERSION.SDK_INT < 21 || data == null) {
            napTvWebSystemMirrorRequested = false;
            appendNativeLog("BUILD2SA13C9 SCREEN_MIRROR_START_SKIP");
            return;
        }
        try {
            napTvWebReleaseSystemMirror("restart", true);
            if (napTvWebProjectionManager == null) {
                napTvWebProjectionManager = (MediaProjectionManager)getSystemService(MEDIA_PROJECTION_SERVICE);
            }
            napTvWebProjection = napTvWebProjectionManager.getMediaProjection(resultCode, data);
            if (napTvWebProjection == null) throw new RuntimeException("getMediaProjection null");

            DisplayMetrics dm = new DisplayMetrics();
            if (Build.VERSION.SDK_INT >= 17) getWindowManager().getDefaultDisplay().getRealMetrics(dm);
            else getWindowManager().getDefaultDisplay().getMetrics(dm);
            int sw = Math.max(2, dm.widthPixels);
            int sh = Math.max(2, dm.heightPixels);
            boolean landscape = sw > sh;
            // BUILD2SK16: drive natvrdo 960/1120 - VUBEC nerespektovalo zvolenou
            // uroven kvality (LOW/MEDIUM/HIGH), proto byl rozdil mezi urovnemi na
            // PS1/Sega/Atari sotva znatelny (jen JPEG kvalita se menila, rozliseni
            // ne). Rozliseni system mirroru je FIXNI na celou dobu behu (nemuze
            // se za chodu menit pri prepnuti obrazovky), takze pouzivame nejvyssi
            // (DJ-uroven) rozliseni zvolene urovne jako zaklad - kvalita (JPEG) se
            // dal spravne meni za behu podle aktualni obrazovky.
            int[] qv0 = napTvWebQualityFor(true, false, landscape);
            int maxSide = qv0[0];
            float scale = Math.min(1.0f, (float)maxSide / Math.max(sw, sh));
            int cw = Math.max(2, (int)(sw * scale)) & ~1;
            int ch = Math.max(2, (int)(sh * scale)) & ~1;
            int dpi = Math.max(120, dm.densityDpi);

            napTvWebSystemThread = new HandlerThread("nap-tv-web-screen");
            napTvWebSystemThread.start();
            napTvWebSystemHandler = new Handler(napTvWebSystemThread.getLooper());
            napTvWebImageReader = ImageReader.newInstance(cw, ch, PixelFormat.RGBA_8888, 2);
            napTvWebImageReader.setOnImageAvailableListener(reader -> {
                Image img = null;
                try {
                    img = reader.acquireLatestImage();
                    napTvWebHandleSystemImage(img);
                } catch (Throwable t) {
                    appendNativeLog("BUILD2SA13C9 SCREEN_IMAGE_ERR " + safeMsg(t));
                    try { if (img != null) img.close(); } catch (Throwable ignored) {}
                }
            }, napTvWebSystemHandler);

            napTvWebProjection.registerCallback(new MediaProjection.Callback() {
                @Override public void onStop() {
                    napTvWebReleaseSystemMirror("projectionStop", false);
                }
            }, napTvWebSystemHandler);
            napTvWebVirtualDisplay = napTvWebProjection.createVirtualDisplay(
                    "AtariHelp-TV-WEB-SCREEN",
                    cw, ch, dpi,
                    android.hardware.display.DisplayManager.VIRTUAL_DISPLAY_FLAG_AUTO_MIRROR,
                    napTvWebImageReader.getSurface(), null, napTvWebSystemHandler);
            if (napTvWebVirtualDisplay == null) throw new RuntimeException("createVirtualDisplay null");
            napTvWebSystemWidth = cw;
            napTvWebSystemHeight = ch;
            napTvWebSystemDpi = dpi;
            napTvWebSystemLastFrameMs = 0;
            napTvWebSystemFallbackLogMs = 0;
            napTvWebSystemMirrorRequested = false;
            napTvWebSystemMirrorActive = true;
            napTvWebVideoProfile = "SCREEN_FULL";
            appendNativeLog("BUILD2SA13C9 SCREEN_MIRROR_ON src=" + sw + "x" + sh + " cap=" + cw + "x" + ch + " dpi=" + dpi);
        } catch (Throwable t) {
            napTvWebSystemMirrorRequested = false;
            appendNativeLog("BUILD2SA13C9 SCREEN_MIRROR_START_FAIL " + safeMsg(t));
            napTvWebReleaseSystemMirror("startFail", true);
        }
    }

    private void napTvWebHandleSystemImage(Image img) {
        if (img == null) return;
        try {
            long now = System.currentTimeMillis();
            int delay = Math.max(45, napTvWebFrameDelayMs);
            if (now - napTvWebSystemLastFrameMs < delay) return;
            napTvWebSystemLastFrameMs = now;
            napTvWebResizeInProgress = false;
            int w = img.getWidth();
            int h = img.getHeight();
            Image.Plane[] planes = img.getPlanes();
            if (planes == null || planes.length == 0) return;
            ByteBuffer buf = planes[0].getBuffer();
            int pixelStride = planes[0].getPixelStride();
            int rowStride = planes[0].getRowStride();
            if (pixelStride != 4 || rowStride <= 0) {
                appendNativeLog("BUILD2SA13C9 SCREEN_IMAGE_UNSUPPORTED pixelStride=" + pixelStride + " rowStride=" + rowStride);
                return;
            }
            int rowPixels = Math.max(w, rowStride / pixelStride);
            Bitmap padded = Bitmap.createBitmap(rowPixels, h, Bitmap.Config.ARGB_8888);
            buf.rewind();
            padded.copyPixelsFromBuffer(buf);
            Bitmap frame = (rowPixels == w) ? padded : Bitmap.createBitmap(padded, 0, 0, w, h);
            if (frame != padded) padded.recycle();
            boolean djScreenSys = false, hqLiteScreenSys = false;
            try { String cu = web == null ? null : web.getUrl(); djScreenSys = cu != null && cu.contains("/dj/"); hqLiteScreenSys = cu != null && (cu.contains("/emu_ps1/") || cu.contains("/emu_sega/") || cu.contains("/emu/") || cu.contains("/emu_vbxe/") || cu.contains("/dj/") || cu.contains("/player/")); } catch (Throwable ignored) {}
            int[] qvSys = napTvWebQualityFor(djScreenSys, hqLiteScreenSys, w > h);
            napTvWebJpegQuality = qvSys[1];
            napTvWebFrameDelayMs = w > h ? 60 : 55;
            napTvWebVideoProfile = "SCREEN_FULL";
            napTvWebPublishBitmap(frame, "SCREEN");
            frame.recycle();
        } catch (Throwable t) {
            appendNativeLog("BUILD2SA13C9 SCREEN_IMAGE_HANDLE_ERR " + safeMsg(t));
        } finally {
            try { img.close(); } catch (Throwable ignored) {}
        }
    }

    private synchronized void napTvWebReleaseSystemMirror(String why, boolean stopProjection) {
        napTvWebSystemMirrorRequested = false;
        napTvWebSystemMirrorActive = false;
        try { if (napTvWebVirtualDisplay != null) napTvWebVirtualDisplay.release(); } catch (Throwable ignored) {}
        napTvWebVirtualDisplay = null;
        try { if (napTvWebImageReader != null) napTvWebImageReader.close(); } catch (Throwable ignored) {}
        napTvWebImageReader = null;
        MediaProjection mp = napTvWebProjection;
        napTvWebProjection = null;
        if (stopProjection && mp != null) {
            try { mp.stop(); } catch (Throwable ignored) {}
        }
        try { if (napTvWebSystemThread != null) napTvWebSystemThread.quitSafely(); } catch (Throwable ignored) {}
        napTvWebSystemThread = null;
        napTvWebSystemHandler = null;
        napTvWebSystemWidth = 0;
        napTvWebSystemHeight = 0;
        napTvWebSystemFallbackLogMs = 0;
        appendNativeLog("BUILD2SA13C9 SCREEN_MIRROR_OFF why=" + why);
    }

    // BUILD2SK18: prekonfiguruje JEN VirtualDisplay+ImageReader (ktere maji pevnou
    // velikost od sveho vzniku) na novou velikost odpovidajici prave zvolene
    // urovni kvality - BEZ opetovneho zadani o MediaProjection povoleni (to uz
    // mame v napTvWebProjection) a BEZ dotyku zvukoveho potrubi vubec, takze
    // zvuk pri zmene urovne kvality nema duvod se prerusit. Volano pri kazde
    // zmene urovne z /quality endpointu, pokud system mirror prave bezi -
    // zadny "vypni a zapni cast" uz neni potreba.
    private synchronized void napTvWebResizeSystemMirror() {
        if (!napTvWebSystemMirrorActive || napTvWebProjection == null || napTvWebSystemHandler == null) return;
        MediaProjection mp = napTvWebProjection;
        try {
            DisplayMetrics dm = new DisplayMetrics();
            if (Build.VERSION.SDK_INT >= 17) getWindowManager().getDefaultDisplay().getRealMetrics(dm);
            else getWindowManager().getDefaultDisplay().getMetrics(dm);
            int sw = Math.max(2, dm.widthPixels), sh = Math.max(2, dm.heightPixels);
            boolean landscape = sw > sh;
            int[] qv0 = napTvWebQualityFor(true, false, landscape);
            int maxSide = qv0[0];
            float scale = Math.min(1.0f, (float) maxSide / Math.max(sw, sh));
            int cw = Math.max(2, (int) (sw * scale)) & ~1;
            int ch = Math.max(2, (int) (sh * scale)) & ~1;
            if (cw == napTvWebSystemWidth && ch == napTvWebSystemHeight) return; // uz na cilove velikosti
            int dpi = Math.max(120, dm.densityDpi);

            // BUILD2SK21: behem teto vedome, uzivatelem vyvolane prestavby (zmena
            // urovne kvality) NECHCEME padat na WebView fallback vubec - misto toho
            // appka jednoduse zmrazi posledni dobry snimek, dokud novy VirtualDisplay
            // nezacne posilat cerstve (viz napTvWebFrameTick a napTvWebHandleSystemImage).
            napTvWebResizeInProgress = true;
            napTvWebResizeStartedMs = System.currentTimeMillis();
            try { if (napTvWebVirtualDisplay != null) napTvWebVirtualDisplay.release(); } catch (Throwable ignored) {}
            napTvWebVirtualDisplay = null;
            try { if (napTvWebImageReader != null) napTvWebImageReader.close(); } catch (Throwable ignored) {}
            napTvWebImageReader = null;

            napTvWebImageReader = ImageReader.newInstance(cw, ch, PixelFormat.RGBA_8888, 2);
            napTvWebImageReader.setOnImageAvailableListener(reader -> {
                Image img = null;
                try { img = reader.acquireLatestImage(); napTvWebHandleSystemImage(img); }
                catch (Throwable t) {
                    appendNativeLog("BUILD2SK18 SCREEN_IMAGE_ERR " + safeMsg(t));
                    try { if (img != null) img.close(); } catch (Throwable ignored) {}
                }
            }, napTvWebSystemHandler);
            napTvWebVirtualDisplay = mp.createVirtualDisplay(
                    "AtariHelp-TV-WEB-SCREEN",
                    cw, ch, dpi,
                    android.hardware.display.DisplayManager.VIRTUAL_DISPLAY_FLAG_AUTO_MIRROR,
                    napTvWebImageReader.getSurface(), null, napTvWebSystemHandler);
            if (napTvWebVirtualDisplay == null) throw new RuntimeException("resize createVirtualDisplay null");
            napTvWebSystemWidth = cw;
            napTvWebSystemHeight = ch;
            napTvWebSystemLastFrameMs = 0;
            appendNativeLog("BUILD2SK18 SCREEN_MIRROR_RESIZED cap=" + cw + "x" + ch + " tier=" + napTvWebQualityTier);
        } catch (Throwable t) {
            napTvWebResizeInProgress = false; // pojistka - jinak by obraz zustal zmrazeny navzdy pri chybe
            appendNativeLog("BUILD2SK18 SCREEN_MIRROR_RESIZE_FAIL " + safeMsg(t));
        }
    }

    private void napTvWebAudioPush(short[] pcm, int offset, int shorts, int sampleRate, String source) {
        if (!napTvWebRunning || pcm == null || shorts <= 0) return;
        try {
            int start = Math.max(0, offset);
            int limit = Math.min(pcm.length, start + shorts);
            if (((limit - start) & 1) != 0) limit--; // stereo alignment
            if (limit <= start) return;
            synchronized (napTvWebAudioLock) {
                if (sampleRate > 8000 && sampleRate != napTvWebAudioRate) {
                    napTvWebAudioRate = sampleRate;
                    napTvWebAudioSeq = 0;
                    appendNativeLog("BUILD2SA13C TV_WEB_AUDIO_RATE source=" + source + " hz=" + sampleRate);
                }
                if (source != null && !source.equals(napTvWebAudioSource)) {
                    napTvWebAudioSource = source;
                    appendNativeLog("BUILD2SA13C TV_WEB_AUDIO_SOURCE " + source);
                }
                int cap = napTvWebAudioRing.length;
                for (int i = start; i < limit; i++) {
                    short v = pcm[i];
                    int pos = (int)(napTvWebAudioSeq % cap);
                    napTvWebAudioRing[pos] = (byte)(v & 0xFF);
                    napTvWebAudioRing[(pos + 1) % cap] = (byte)((v >> 8) & 0xFF);
                    napTvWebAudioSeq += 2;
                }
                napTvWebAudioLastPushMs = System.currentTimeMillis();
            }
        } catch (Throwable t) {
            appendNativeLog("BUILD2SA13C TV_WEB_AUDIO_PUSH_ERR " + safeMsg(t));
        }
    }

    private void napTvWebAudioPushMonoPcm16Bytes(byte[] pcm, int sampleRate, String source) {
        if (!napTvWebRunning || pcm == null || pcm.length < 2) return;
        try {
            int len = Math.min(pcm.length & ~1, 65536);
            synchronized (napTvWebAudioLock) {
                if (sampleRate > 8000 && sampleRate != napTvWebAudioRate) {
                    napTvWebAudioRate = sampleRate;
                    napTvWebAudioSeq = 0;
                    appendNativeLog("BUILD2SA13C TV_WEB_AUDIO_RATE source=" + source + " hz=" + sampleRate);
                }
                if (source != null && !source.equals(napTvWebAudioSource)) {
                    napTvWebAudioSource = source;
                    appendNativeLog("BUILD2SA13C TV_WEB_AUDIO_SOURCE " + source);
                }
                int cap = napTvWebAudioRing.length;
                for (int i = 0; i < len; i += 2) {
                    byte lo = pcm[i], hi = pcm[i + 1];
                    int pos = (int)(napTvWebAudioSeq % cap);
                    napTvWebAudioRing[pos] = lo;
                    napTvWebAudioRing[(pos + 1) % cap] = hi;
                    napTvWebAudioRing[(pos + 2) % cap] = lo;
                    napTvWebAudioRing[(pos + 3) % cap] = hi;
                    napTvWebAudioSeq += 4;
                }
                napTvWebAudioLastPushMs = System.currentTimeMillis();
            }
        } catch (Throwable t) {
            appendNativeLog("BUILD2SA13C TV_WEB_AUDIO_MONO_PUSH_ERR source=" + source + " " + safeMsg(t));
        }
    }

    private void napTvWebAudioPushStereoPcm16Bytes(byte[] pcm, int sampleRate, String source) {
        if (!napTvWebRunning || pcm == null || pcm.length < 4) return;
        try {
            int len = Math.min(pcm.length & ~3, 65536);
            synchronized (napTvWebAudioLock) {
                if (sampleRate > 8000 && sampleRate != napTvWebAudioRate) {
                    napTvWebAudioRate = sampleRate;
                    napTvWebAudioSeq = 0;
                    appendNativeLog("BUILD2SA13C TV_WEB_AUDIO_RATE source=" + source + " hz=" + sampleRate);
                }
                if (source != null && !source.equals(napTvWebAudioSource)) {
                    napTvWebAudioSource = source;
                    appendNativeLog("BUILD2SA13C TV_WEB_AUDIO_SOURCE " + source);
                }
                int cap = napTvWebAudioRing.length;
                for (int i = 0; i < len; i++) {
                    napTvWebAudioRing[(int)(napTvWebAudioSeq % cap)] = pcm[i];
                    napTvWebAudioSeq++;
                }
                napTvWebAudioLastPushMs = System.currentTimeMillis();
            }
        } catch (Throwable t) {
            appendNativeLog("BUILD2SA13C TV_WEB_AUDIO_STEREO_PUSH_ERR source=" + source + " " + safeMsg(t));
        }
    }

    private long napTvWebQueryLong(String fullPath, String key, long fallback) {
        try {
            int q = fullPath == null ? -1 : fullPath.indexOf('?');
            if (q < 0 || q + 1 >= fullPath.length()) return fallback;
            String[] parts = fullPath.substring(q + 1).split("&");
            for (String part : parts) {
                int eq = part.indexOf('=');
                String k = eq >= 0 ? part.substring(0, eq) : part;
                if (key.equals(k)) return Long.parseLong(eq >= 0 ? part.substring(eq + 1) : "0");
            }
        } catch (Throwable ignored) {}
        return fallback;
    }

    // BUILD2SK59: string varianta pro /clientlog - stejny vzor jako
    // napTvWebQueryLong, jen vraci URL-dekodovany retezec misto cisla.
    private String napTvWebQueryString(String fullPath, String key, String fallback) {
        try {
            int q = fullPath == null ? -1 : fullPath.indexOf('?');
            if (q < 0 || q + 1 >= fullPath.length()) return fallback;
            String[] parts = fullPath.substring(q + 1).split("&");
            for (String part : parts) {
                int eq = part.indexOf('=');
                String k = eq >= 0 ? part.substring(0, eq) : part;
                if (key.equals(k)) {
                    String v = eq >= 0 ? part.substring(eq + 1) : "";
                    return java.net.URLDecoder.decode(v, "UTF-8");
                }
            }
        } catch (Throwable ignored) {}
        return fallback;
    }

    private String napTvWebLocalIp() {
        String fallback = "";
        try {
            Enumeration<NetworkInterface> nets = NetworkInterface.getNetworkInterfaces();
            while (nets != null && nets.hasMoreElements()) {
                NetworkInterface ni = nets.nextElement();
                try { if (!ni.isUp() || ni.isLoopback()) continue; } catch (Throwable ignored) {}
                String n = ni.getName() == null ? "" : ni.getName().toLowerCase(Locale.US);
                Enumeration<InetAddress> addrs = ni.getInetAddresses();
                while (addrs != null && addrs.hasMoreElements()) {
                    InetAddress a = addrs.nextElement();
                    if (a instanceof Inet4Address && !a.isLoopbackAddress()) {
                        String host = a.getHostAddress();
                        if (n.startsWith("wlan") || n.startsWith("swlan") || n.startsWith("ap")) return host;
                        if (fallback.length() == 0) fallback = host;
                    }
                }
            }
        } catch (Throwable t) {
            appendNativeLog("BUILD2SA13C TV_WEB_IP_ERR " + safeMsg(t));
        }
        return fallback.length() > 0 ? fallback : "127.0.0.1";
    }

    private String napTvWebUrl() {
        if (!napTvWebRunning || napTvWebPort <= 0) return "TV_WEB_CAST_OFF";
        return "http://" + napTvWebLocalIp() + ":" + napTvWebPort + "/";
    }

    private synchronized String napTvWebStart() {
        if (napTvWebRunning && napTvWebServer != null) {
            return "TV_WEB_CAST_OK " + napTvWebUrl() + (napTvWebSystemMirrorActive ? " SCREEN" : " APP");
        }
        try {
            ServerSocket ss;
            try { ss = new ServerSocket(8765); }
            catch (Throwable busy) { ss = new ServerSocket(0); }
            napTvWebServer = ss;
            napTvWebPort = ss.getLocalPort();
            napTvWebRunning = true;
            napTvWebPixelCopyPending = false;
            napTvWebPixelCopyPendingAtMs = 0;
            napTvWebPixelCopyFallbackLogMs = 0;
            napTvWebPixelCopyDisabledUntilMs = 0;
            napTvWebLastFrameMs = 0;
            napTvWebEnsurePlaceholderFrame("STARTING");
            synchronized (napTvWebAudioLock) {
                napTvWebAudioSeq = 0;
                napTvWebAudioLastPushMs = 0;
                napTvWebAudioSource = "NONE";
            }
            if (Build.VERSION.SDK_INT >= 26) {
                try {
                    napTvWebCopyThread = new HandlerThread("nap-tv-web-pixelcopy");
                    napTvWebCopyThread.start();
                    napTvWebCopyHandler = new Handler(napTvWebCopyThread.getLooper());
                } catch (Throwable t) {
                    appendNativeLog("BUILD2SA13C TV_WEB_COPY_THREAD_FAIL " + safeMsg(t));
                    napTvWebCopyThread = null;
                    napTvWebCopyHandler = null;
                }
            }
            napTvWebServerThread = new Thread(this::napTvWebAcceptLoop, "nap-tv-web-cast");
            napTvWebServerThread.setDaemon(true);
            napTvWebServerThread.start();
            ui.removeCallbacks(napTvWebFrameTick);
            ui.post(napTvWebFrameTick);
            String url = napTvWebUrl();
            appendNativeLog("BUILD2SA13C13 TV_WEB_CAST_ON url=" + url + " mode=browser_mjpeg_mp3_draw_safe_youtube_in_app audio=PCM16_STEREO");
            // BUILD2SK26: telefon si po chvili necinnosti (nikdo se ho behem
            // sledovani TV nedotyka) sam zamkne obrazovku - to zpusobi
            // "activityPause" (potvrzeno v logu), po kterem nasleduje dlouha
            // "backing surface" smycka (SK24/SK25 ji jen zmirnuji, neresi na
            // koreni). Reseni na koreni: dokud bezi TV cast, drz obrazovku
            // vzhuru (stejny mechanismus jako video prehravace/navigace).
            try { ui.post(() -> { try { getWindow().addFlags(android.view.WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON); } catch (Throwable ignored) {} }); } catch (Throwable ignored) {}
            // BUILD2SK84: pozadat OS o rovnomerny vykon (ne "kratky burst pak sporeni")
            // po celou dobu TV-cast relace - jen HINT, OS smi ignorovat, a NENI to
            // totez co tepelne hrdlo (to je bezpecnostni pojistka hardwaru, kterou appka
            // schvalne nejde a nema obchazet - viz TV_WEB_PERIODIC thermal= log misto).
            try { ui.post(() -> { try { getWindow().setSustainedPerformanceMode(true); } catch (Throwable ignored) {} }); } catch (Throwable ignored) {}
            return "TV_WEB_CAST_OK " + url + " APP";
        } catch (Throwable t) {
            napTvWebRunning = false;
            appendNativeLog("BUILD2SA13C TV_WEB_CAST_START_FAIL " + safeMsg(t));
            return "TV_WEB_CAST_FAIL " + safeMsg(t);
        }
    }

    private synchronized String napTvWebStop(String why) {
        napTvWebRunning = false;
        // BUILD2SK26: uvolni drzeni obrazovky vzhuru - mimo TV cast se telefon
        // ma chovat normalne (nezustavat zbytecne vzhuru a zrat baterku).
        // (drzeni displeje vzhuru se uz NERUSI - appka nesmi usnout nikdy)
        try { ui.post(() -> { try { getWindow().setSustainedPerformanceMode(false); } catch (Throwable ignored) {} }); } catch (Throwable ignored) {}
        napTvWebPixelCopyPending = false;
        napTvWebPixelCopyPendingAtMs = 0;
        napTvWebPixelCopyDisabledUntilMs = 0;
        napTvWebPendingScreenUrl = null;
        napTvWebReleaseSystemMirror(why, true);
        ui.removeCallbacks(napTvWebFrameTick);
        try { if (napTvWebServer != null) napTvWebServer.close(); } catch (Throwable ignored) {}
        try { if (napTvWebCopyThread != null) napTvWebCopyThread.quitSafely(); } catch (Throwable ignored) {}
        napTvWebCopyThread = null;
        napTvWebCopyHandler = null;
        napTvWebServer = null;
        napTvWebPort = 0;
        napTvWebJpeg = null;
        napTvWebLastFrameMs = 0;
        appendNativeLog("BUILD2SA13C TV_WEB_CAST_OFF why=" + why);
        return "TV_WEB_CAST_OFF " + why;
    }

    private void napTvWebAcceptLoop() {
        while (napTvWebRunning) {
            try {
                ServerSocket ss = napTvWebServer;
                if (ss == null) break;
                Socket s = ss.accept();
                new Thread(() -> napTvWebHandleClient(s), "nap-tv-web-client").start();
            } catch (Throwable t) {
                if (napTvWebRunning) appendNativeLog("BUILD2SA13C TV_WEB_ACCEPT_ERR " + safeMsg(t));
                try { Thread.sleep(160); } catch (InterruptedException ignored) {}
            }
        }
    }

    // BUILD2SK91: sdilena logika pro nastaveni kvality - drive existovala jen
    // uvnitr /quality HTTP endpointu (volalo se z LOW/MED/HIGH tlacitek na
    // TV-cast strance v prohlizeci). Rene chtel stejnou volbu primo v appce
    // (ozubene kolecko na uvodni obrazovce) - misto kopirovani teto logiky
    // podruhe ji vytahuji sem, aby HTTP endpoint i novy JS most (AHTvWeb.
    // setQualityTier) volaly PRESNE totez, zadna sance na rozjeti.
    private String napTvWebSetQualityTier(long t) {
        napTvWebQualityTier = (int) Math.max(0, Math.min(2, t));
        try { getSharedPreferences("nap_tv_prefs", MODE_PRIVATE).edit().putInt("quality_tier", napTvWebQualityTier).apply(); } catch (Throwable ignored) {}
        appendNativeLog("BUILD2SK91 TV_WEB_QUALITY_TIER_SET tier=" + napTvWebQualityTier);
        // BUILD2SK18: pokud uz system mirror bezi, prekonfiguruj VirtualDisplay
        // na nove rozliseni HNED - zadny "vypni a zapni cast" jiz neni potreba.
        if (napTvWebSystemMirrorActive) { ui.post(this::napTvWebResizeSystemMirror); }
        return String.valueOf(napTvWebQualityTier);
    }

    private void napTvWebHandleClient(Socket s) {
        try {
            s.setTcpNoDelay(true);
            InputStream in = s.getInputStream();
            OutputStream out = s.getOutputStream();
            byte[] buf = new byte[1200];
            int n = in.read(buf);
            String req = n > 0 ? new String(buf, 0, n, "ISO-8859-1") : "";
            String path = "/";
            int a = req.indexOf(' '), b = a < 0 ? -1 : req.indexOf(' ', a + 1);
            if (a >= 0 && b > a) path = req.substring(a + 1, b);
            String fullPath = path;
            int q = path.indexOf('?');
            if (q >= 0) path = path.substring(0, q);
            if ("/".equals(path) || "/index.html".equals(path)) {
                napTvWebWriteHtml(out);
            } else if ("/frame.jpg".equals(path)) {
                napTvWebWriteFrame(out);
            } else if ("/stream.mjpg".equals(path)) {
                napTvWebWriteMjpeg(out, s);
            } else if ("/stream.h264".equals(path)) {
                napTvWebWriteH264Stream(out, s);
            } else if ("/clientlog".equals(path)) {
                // BUILD2SK59: klient (prohlizec) posila kratke diagnosticke
                // zpravy o prubehu H264/JMuxer inicializace - bez tohohle
                // jsem nemel VUBEC zadnou viditelnost do toho, co se deje
                // na strane prohlizece, kdyz neco selze (SK57 nikdy nedoslo
                // ani k prvnimu pokusu o pripojeni - zadny zpusob, jak
                // rozlisit "klient se nikdy nepokusil" od "klient zkusil a
                // selhal driv, nez server cokoli zaznamenal").
                String msg = napTvWebQueryString(fullPath, "m", "");
                appendNativeLog("BUILD2SK59 TV_WEB_CLIENT " + msg);
                byte[] ok = "ok".getBytes("UTF-8");
                napTvWebHeader(out, "200 OK", "text/plain; charset=utf-8", ok.length, false);
                out.write(ok);
            } else if ("/audio.raw".equals(path)) {
                napTvWebWriteAudioRaw(out, napTvWebQueryLong(fullPath, "after", -1));
            } else if ("/quality".equals(path)) {
                long t = napTvWebQueryLong(fullPath, "tier", napTvWebQualityTier);
                byte[] body = ("tier=" + napTvWebSetQualityTier(t)).getBytes("UTF-8");
                napTvWebHeader(out, "200 OK", "text/plain; charset=utf-8", body.length, true);
                out.write(body);
            } else if ("/status".equals(path)) {
                String curUrl3 = napTvWebCurrentUrl; // BUILD2SK61: bezpecna cache z UI vlakna, viz vysvetleni u deklarace pole
                byte[] body = ("running=" + napTvWebRunning
                        + " seq=" + napTvWebSeq
                        + " mirror=" + (napTvWebSystemMirrorActive ? "SCREEN" : "APP")
                        + " profile=" + napTvWebVideoProfile
                        + " screen=" + napTvWebSystemWidth + "x" + napTvWebSystemHeight
                        + " frameAgeMs=" + (napTvWebLastFrameMs == 0 ? 999999 : (System.currentTimeMillis() - napTvWebLastFrameMs))
                        + " jpegQ=" + napTvWebJpegQuality
                        + " frameDelayMs=" + napTvWebFrameDelayMs
                        + " audioSeq=" + napTvWebAudioSeq
                        + " audioSource=" + napTvWebAudioSource
                        + " audioRate=" + napTvWebAudioRate
                        + " h264Seq=" + napTvWebH264Seq
                        + " url=" + curUrl3 + "\n").getBytes("UTF-8");
                napTvWebHeader(out, "200 OK", "text/plain; charset=utf-8", body.length, false);
                out.write(body);
            } else if ("/vyhlazeni".equals(path)) {
                // Prepinac vyladeni obrazu pred kompresi. Volá ho tlacitko
                // v prohlizeci, at si Rene muze porovnat oba stavy.
                napTvVyhlazeni = !napTvVyhlazeni;
                appendNativeLog("TV_VYLADENI " + (napTvVyhlazeni ? "ZAPNUTO" : "VYPNUTO"));
                byte[] odp = (napTvVyhlazeni ? "1" : "0").getBytes("UTF-8");
                out.write(("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n"
                        + "Access-Control-Allow-Origin: *\r\n"
                        + "Content-Length: " + odp.length + "\r\n\r\n").getBytes("UTF-8"));
                out.write(odp); out.flush();
            } else if ("/log".equals(path)) {
                // BUILD2SK33: log byl driv videt jen zevnitr PS1 obrazovky (nedalo
                // se poslat log z toho, co se deje na jinych obrazovkach - DJ pult,
                // Atari, uvodni stranka). Sdileny ring-buffer (nativeLog) uz
                // existoval, jen nebyl dostupny odjinud - tenhle endpoint ho
                // vystavuje pres HTTP na stejnem serveru, co uz bezi pro TV cast,
                // takze jde otevrit v PC/TV prohlizeci KDYKOLI, nezavisle na tom,
                // ktera nativni obrazovka je zrovna aktivni na telefonu.
                // BUILD2SK82: ring (nativeLog) je omezeny na 100KB - u delsiho testu
                // (vic obrazovek + vic urovni kvality v jedne relaci) stary obsah
                // VYPADAVAL drive, nez se stihlo otevrit /log na konci. Ted se cte
                // PRIMARNE z souboru na disku (napTvWebLogFile), kam appka pise
                // prubezne od onCreate - ring zustava jen jako fallback pro pripad,
                // ze by zapis na disk selhal (napr. uloziste plne).
                byte[] body = null;
                File lf = napTvWebLogFile;
                if (lf != null) {
                    try {
                        byte[] fromFile = napTvWebReadFileBytes(lf);
                        if (fromFile.length > 0) body = fromFile;
                    } catch (Throwable ignored) {}
                }
                if (body == null) {
                    String logText;
                    synchronized (nativeLog) { logText = nativeLog.toString(); }
                    body = logText.getBytes("UTF-8");
                }
                napTvWebHeader(out, "200 OK", "text/plain; charset=utf-8", body.length, true);
                out.write(body);
            } else {
                byte[] body = "not found\n".getBytes("UTF-8");
                napTvWebHeader(out, "404 Not Found", "text/plain; charset=utf-8", body.length, false);
                out.write(body);
            }
            out.flush();
        } catch (Throwable ignored) {
        } finally {
            try { s.close(); } catch (Throwable ignored) {}
        }
    }

    private void napTvWebHeader(OutputStream out, String status, String type, int len, boolean close) throws IOException {
        String h = "HTTP/1.1 " + status + "\r\n"
                + "Content-Type: " + type + "\r\n"
                + "Cache-Control: no-store, no-cache, must-revalidate, max-age=0\r\n"
                + "Pragma: no-cache\r\n"
                + (len >= 0 ? "Content-Length: " + len + "\r\n" : "")
                + "Connection: " + (close ? "close" : "keep-alive") + "\r\n\r\n";
        out.write(h.getBytes("ISO-8859-1"));
    }

    private void napTvWebWriteHtml(OutputStream out) throws IOException {
        String body = "<!doctype html><html><head><meta charset='utf-8'>"
                + "<meta name='viewport' content='width=device-width,initial-scale=1'>"
                + "<title>AtariHelp TV</title><style>"
                + "html,body{margin:0;width:100%;height:100%;background:#000;color:#9fdcff;font:16px monospace;overflow:hidden}"
                + "#v{position:fixed;inset:0;width:100%;height:100%;object-fit:contain;background:#000}"
                + "#s{position:fixed;left:10px;bottom:8px;padding:4px 7px;background:rgba(0,0,0,.55);border-radius:4px}"
                + "#a{position:fixed;right:10px;bottom:8px;padding:8px 10px;background:rgba(10,30,40,.78);border:1px solid #64dfff;border-radius:5px;color:#dfffff;font:700 15px monospace}"
                + "#q{position:fixed;right:10px;bottom:48px;display:flex;gap:4px}"
                + "#q button{padding:7px 9px;background:rgba(10,30,40,.78);border:1px solid #3a6a78;border-radius:5px;color:#9fdcff;font:700 12px monospace}"
                + "#q button.on{background:rgba(20,90,60,.85);border-color:#5aff9a;color:#eaffea}"
                + "</style></head><body><img id='v' alt='AtariHelp TV'><video id='h264v' muted autoplay playsinline style='display:none;position:fixed;inset:0;width:100%;height:100%;object-fit:contain;background:#000'></video><div id='s' style='display:none'>AtariHelp TV WEB CAST</div><button id='a' type='button' style='display:none'>AUDIO OK</button>"
                // ===== RUCNI DOLADENI OBRAZU =====
                // Rene chtel jas a kontrast nastavitelne primo v prohlizeci -
                // ruzne hry maji ruzne tmavy obraz a nastavit to natvrdo v kodu
                // by u jinych her uskodilo. Panel se schova sam po par vterinach
                // a nastaveni se pamatuje mezi spustenimi.
                + "<div id='pan' style='position:fixed;right:10px;bottom:10px;z-index:99;background:rgba(0,0,0,.72);color:#ddd;font:12px sans-serif;padding:8px 10px;border-radius:8px;opacity:0;transition:opacity .25s'>"
                + "OBRAZ<br>jas <input id='sB' type='range' min='60' max='170' value='100'>"
                + "<br>kontrast <input id='sC' type='range' min='60' max='170' value='112'>"   // vychozi mirne vyssi - vyhlazeni obraz zmekci a kontrast to srovna
                + "<br>sytost <input id='sS' type='range' min='60' max='170' value='108'>"
                + "<br><button id='sR' type='button' style='margin-top:4px;padding:3px 8px'>PUVODNI NASTAVENI</button>"
                + "<br><button id='sF' type='button' style='margin-top:4px;padding:3px 8px'>OBRAZ: vyplnit obrazovku</button>"
                + "<br><button id='sV' type='button' style='margin-top:4px;padding:3px 8px'>VYLADENI OBRAZU: zapnuto</button></div>"
                + "<div id='q'><button type='button' data-t='0' style='display:none'>LOW</button><button type='button' data-t='1' style='display:none'>MED</button><button type='button' data-t='2' style='display:none'>HIGH</button><button type='button' id='fs' style='display:none'>\u26f6 FULL</button></div>"
                + "<script>(function(){var AVD=(function(){try{var m=location.search.match(/[?&]av=([0-9.]+)/);if(m)return parseFloat(m[1]);var q=localStorage.getItem('napAvd');return q!==null?parseFloat(q):0.30;}catch(e){return 0.30;}})();function setAvd(x){AVD=Math.max(0,Math.min(2,Math.round(x*100)/100));try{localStorage.setItem('napAvd',AVD);}catch(e){}var o=document.getElementById('avdmsg');if(!o){o=document.createElement('div');o.id='avdmsg';o.style.cssText='position:fixed;left:50%;top:12%;transform:translateX(-50%);background:rgba(0,0,0,.75);color:#0f0;font:20px monospace;padding:8px 16px;border-radius:6px;z-index:99999;pointer-events:none';document.body.appendChild(o);}o.textContent='ZVUK '+Math.round(AVD*1000)+' ms';o.style.display='block';clearTimeout(window._avdT);window._avdT=setTimeout(function(){o.style.display='none';},1200);}var v=document.getElementById('v'),s=document.getElementById('s'),a=document.getElementById('a'),n=0,fb=false,ac=null,g=null,next=0,aseq=0,aon=false,active=[],lastSeq=0,lastSeqT=0,curFps=0,staleTicks=0;" // BUILD2SB1
                + "var h264v=document.getElementById('h264v'),h264Active=false,h264Reader=null,jm=null,h264Loading=false,h264LastFeedMs=0;" // BUILD2SK57+SK73
                + "var pan=document.getElementById('pan'),sB=document.getElementById('sB'),sC=document.getElementById('sC'),sS=document.getElementById('sS'),sR=document.getElementById('sR'),sF=document.getElementById('sF'),panT=0;"
                + "function napF(){var f='brightness('+(sB.value/100)+') contrast('+(sC.value/100)+') saturate('+(sS.value/100)+')';h264v.style.filter=f;v.style.filter=f;"
                + "try{localStorage.setItem('napObraz',sB.value+','+sC.value+','+sS.value);}catch(e){}}"
                + "try{var ul=localStorage.getItem('napObraz');if(ul){var pp=ul.split(',');sB.value=pp[0];sC.value=pp[1];sS.value=pp[2];}}catch(e){}"
                // Pouzit VZDYCKY, i pri prvnim spusteni - jinak by vychozi
                // kontrast a sytost z posuvniku nikdy nezacaly platit.
                + "napF();"
                + "sB.oninput=sC.oninput=sS.oninput=napF;"
                + "sR.onclick=function(){sB.value=100;sC.value=112;sS.value=108;napF();};"
                // ===== OBRAZ SE NESMI OREZAVAT ANI ROZTAHOVAT =====
                // Na web chodi presne to, co je na mobilu - na vysku
                // 720x1336, na sirku 1280x720. Kdyz se pouzil rezim
                // "cover" (vyplnit a co pretece oriznout), obraz na
                // vysku se v sirokem okne prohlizece roztahl a vetsina
                // se ukrojila - Rene to hlasil petkrat.
                // "contain" znamena: vejdi se cely a zachovej pomer.
                // To je jedine spravne - obraz na vysku bude na vysku,
                // po stranach cerno, presne jako na mobilu.
                // Ulozena hodnota z drivejska se ZAHAZUJE (napFillV3),
                // protoze v prohlizecich zustalo "oriznout".
                + "var napFill=1;function napSetFill(){if(!sF)return;var m=napFill?'cover':'contain';h264v.style.objectFit=m;v.style.objectFit=m;"
                + "sF.textContent=napFill?'OBRAZ: vyplnit obrazovku':'OBRAZ: cely, bez orezu';"
                + "try{localStorage.setItem('napFillV3',napFill);}catch(e){}}"
                + "try{var uf=localStorage.getItem('napFillV3');if(uf!==null)napFill=parseInt(uf)||0;}catch(e){}"
                + "napSetFill();if(sF)sF.onclick=function(){napFill=napFill?0:1;napSetFill();};"
                + "var sV=document.getElementById('sV'),napV=1;"
                + "if(sV)sV.onclick=function(){fetch('/vyhlazeni').then(function(r){return r.text();})"
                + ".then(function(t){napV=(t.trim()==='1');sV.textContent='VYLADENI OBRAZU: '+(napV?'zapnuto':'vypnuto');})"
                + ".catch(function(e){clog('vyladeni: '+e);});};"
                + "function panUkaz(){pan.style.opacity='1';clearTimeout(panT);panT=setTimeout(function(){pan.style.opacity='0';},2600);}"
                + "document.addEventListener('mousemove',panUkaz);document.addEventListener('touchstart',panUkaz);panUkaz();"
                + "function label(t){s.textContent='AtariHelp TV WEB CAST [SK60] '+t+' '+new Date().toLocaleTimeString()+' '+curFps+'fps'+(aon?' AUDIO ON':' AUDIO OFF');}"
                + "function clog(m){try{fetch('/clientlog?m='+encodeURIComponent(m));}catch(e){}}" // BUILD2SK59: diagnostika z prohlizece zpet do /log - bez tohohle zadna viditelnost, kdyz H264 cesta selze drive, nez se server vubec dozvi
                + "clog('PAGE LOADED build=SK60 ts='+Date.now());" // BUILD2SK60: OKAMZITY majak hned pri behu skriptu - pokud tohle v /log NENI, novy JS se vubec nespustil (stara zalozka/APK), a je zbytecne hledat chybu dal v H264 toku
                + "var h264CdnTried=0,h264CdnUrls=['https://cdn.jsdelivr.net/npm/jmuxer@2/dist/jmuxer.min.js','https://unpkg.com/jmuxer@2/dist/jmuxer.min.js'];"
                + "function stopH264(){if(h264Active)clog('stopH264');h264Active=false;if(h264Reader){try{h264Reader.cancel();}catch(e){}h264Reader=null;}h264v.style.display='none';v.style.display='';}"
                + "function startH264(){if(h264Active||h264Loading)return;"
                + "if(!window.JMuxer){if(h264CdnTried>=h264CdnUrls.length){clog('JMuxer CDN failed all '+h264CdnTried+' tries');return;}"
                + "h264Loading=true;var url=h264CdnUrls[h264CdnTried];h264CdnTried++;clog('loading JMuxer from '+url);"
                + "var sc=document.createElement('script');sc.src=url;"
                + "sc.onload=function(){clog('JMuxer loaded ok');h264Loading=false;startH264();};"
                + "sc.onerror=function(){clog('JMuxer load FAILED '+url);h264Loading=false;startH264();};"
                + "document.head.appendChild(sc);return;}"
                + "clog('JMuxer ready, starting');v.style.display='none';h264v.style.display='';h264Active=true;h264LastFeedMs=0;"
                + "try{jm=new JMuxer({node:'h264v',mode:'video',flushingTime:0,fps:60,debug:false,onError:function(e){clog('jmuxer onError '+e);}});}"
                + "catch(e){clog('JMuxer ctor threw '+e);h264Active=false;v.style.display='';h264v.style.display='none';return;}"
                + "clog('fetching stream.h264');"
                + "fetch('/stream.h264?'+Date.now()).then(function(r){clog('fetch status '+r.status);h264Reader=r.body.getReader();var first=true;"
                + "function pump(){if(!h264Active)return;h264Reader.read().then(function(res){if(res.done||!h264Active){clog('stream ended, resetting');h264Active=false;h264Reader=null;h264v.style.display='none';v.style.display='';return;}"
                + "var nowMs=Date.now(),dur=h264LastFeedMs?Math.max(4,Math.min(200,nowMs-h264LastFeedMs)):20;h264LastFeedMs=nowMs;if(first){first=false;clog('first chunk bytes='+res.value.length);}try{jm.feed({video:res.value,duration:dur});}catch(e){clog('feed err '+e);}pump();"
                + "}).catch(function(e){clog('read err '+e);});}pump();"
                + "}).catch(function(e){clog('fetch FAILED '+e);h264Active=false;v.style.display='';h264v.style.display='none';});}" // BUILD2SK57+SK59: syrovy H.264 (Annex-B) ze serveru -> JMuxer.js remuxuje na fMP4 primo v prohlizeci -> MSE -> <video>
                + "function fallback(){fb=true;function tick(){v.onload=v.onerror=function(){setTimeout(tick,45)};v.src='/frame.jpg?'+Date.now();if((++n%40)===0)label('JPEG');}tick();}"
                + "function startAudio(){if(aon)return;try{var C=window.AudioContext||window.webkitAudioContext;if(!C){a.textContent='AUDIO NENI';return;}ac=new C({latencyHint:'interactive'});if(ac.resume)ac.resume();g=ac.createGain();g.gain.value=1;g.connect(ac.destination);next=ac.currentTime+AVD;aon=true;a.textContent='AUDIO ON';pollAudio();label(fb?'JPEG':'MJPEG');}catch(e){a.textContent='AUDIO ERR';}}" // BUILD2SB1: jitter polstar 350 ms + master gain pro fady
                + "function cutover(){if(!ac||!g)return;var t=ac.currentTime;try{g.gain.cancelScheduledValues(t);g.gain.setValueAtTime(g.gain.value,t);g.gain.linearRampToValueAtTime(0,t+0.01);}catch(e){}for(var i=0;i<active.length;i++){try{active[i].stop(t+0.012);}catch(e){}}active=[];next=t+AVD;try{g.gain.setValueAtTime(0,next-0.012);g.gain.linearRampToValueAtTime(1,next);}catch(e){}}" // BUILD2SB1: 10ms fade-out/in misto lepeni
                + "async function pollAudio(){if(!aon||!ac)return;try{var r=await fetch('/audio.raw?after='+aseq+'&t='+Date.now(),{cache:'no-store'});var sq=parseInt(r.headers.get('x-nap-audio-seq')||aseq,10);var st=parseInt(r.headers.get('x-nap-audio-start')||aseq,10);var rate=parseInt(r.headers.get('x-nap-audio-rate')||'44100',10);var dis=(r.headers.get('x-nap-audio-discontinuity')||'0')==='1';var ab=await r.arrayBuffer();var expected=aseq;if(!isNaN(sq))aseq=sq;if(ab.byteLength>=4){var now=ac.currentTime;if(dis||next<now+0.04||next>now+AVD+0.5)cutover();var dv=new DataView(ab),frames=Math.floor(ab.byteLength/4),buf=ac.createBuffer(2,frames,rate),L=buf.getChannelData(0),R=buf.getChannelData(1);for(var i=0,p=0;i<frames;i++,p+=4){L[i]=dv.getInt16(p,true)/32768;R[i]=dv.getInt16(p+2,true)/32768;}var src=ac.createBufferSource();src.buffer=buf;src.connect(g);if(next<ac.currentTime+0.02)next=ac.currentTime+AVD;src.start(next);next+=frames/rate;active.push(src);src.onended=function(){var k=active.indexOf(src);if(k>=0)active.splice(k,1);};if(active.length>60)active.splice(0,active.length-60);}}catch(e){}setTimeout(pollAudio,20);}" // BUILD2SB1: planovane buffery (jitter buffer), zadne tvrde resety next, cutover s fadem
                + "a.onclick=startAudio;function toggleFs(){var el=document.documentElement;try{if(!(document.fullscreenElement||document.webkitFullscreenElement)){(el.requestFullscreen||el.webkitRequestFullscreen||el.msRequestFullscreen).call(el);}else{(document.exitFullscreen||document.webkitExitFullscreen||document.msExitFullscreen).call(document);}}catch(e){}}document.addEventListener('click',toggleFs,true);document.addEventListener('keydown',function(e){if(e.key==='ArrowUp'){setAvd(AVD+0.05);e.preventDefault();}else if(e.key==='ArrowDown'){setAvd(AVD-0.05);e.preventDefault();}else if(e.key==='0'){setAvd(0.30);e.preventDefault();}},true);setTimeout(function(){try{startAudio();}catch(e){}},80);var aTry=setInterval(function(){if(aon){clearInterval(aTry);return;}try{startAudio();if(ac&&ac.state==='suspended'&&ac.resume)ac.resume();}catch(e){}},1000);"
                + "(function(){var qs=document.querySelectorAll('#q button');function mark(t){for(var i=0;i<qs.length;i++)qs[i].classList.toggle('on',qs[i].getAttribute('data-t')===(''+t));}"
                + "fetch('/quality').then(function(r){return r.text();}).then(function(t){var m=/tier=(\\d)/.exec(t);mark(m?m[1]:'0');}).catch(function(){});"
                + "for(var i=0;i<qs.length;i++){qs[i].onclick=function(e){var t=e.target.getAttribute('data-t');fetch('/quality?tier='+t).then(function(){mark(t);}).catch(function(){});};}})();"
                + "(function(){var fb=document.getElementById('fs');function isFs(){return !!(document.fullscreenElement||document.webkitFullscreenElement||document.msFullscreenElement);}"
                + "function upd(){fb.textContent=isFs()?'⛶ EXIT':'⛶ FULL';}"
                + "fb.onclick=function(){var el=document.documentElement;try{if(!isFs()){(el.requestFullscreen||el.webkitRequestFullscreen||el.msRequestFullscreen).call(el);}else{(document.exitFullscreen||document.webkitExitFullscreen||document.msExitFullscreen).call(document);}}catch(e){}};"
                + "document.addEventListener('fullscreenchange',upd);document.addEventListener('webkitfullscreenchange',upd);document.addEventListener('msfullscreenchange',upd);upd();})();"
                + "v.onerror=function(){if(!fb)fallback();};v.src='/stream.mjpg?'+Date.now();label('MJPEG');"
                + "function pollFps(){fetch('/status').then(function(r){return r.text();}).then(function(t){var m=/seq=(\\d+)/.exec(t);var m2=/h264Seq=(\\d+)/.exec(t);if(!h264Active&&!h264Loading){clog('pollFps calling startH264 (universal)');startH264();}var useM=h264Active&&m2?m2:m;if(useM){var sq=parseInt(useM[1],10),now=Date.now();if(lastSeqT>0){var dt=(now-lastSeqT)/1000;if(dt>0)curFps=Math.round((sq-lastSeq)/dt*10)/10;}if(sq===lastSeq&&sq>0){staleTicks++;}else{staleTicks=0;}lastSeq=sq;lastSeqT=now;if(staleTicks>=4&&!fb&&!h264Active){staleTicks=0;v.src='/stream.mjpg?'+Date.now();}}label(h264Active?'H264':(fb?'JPEG':'MJPEG'));}).catch(function(e){clog('pollFps fetch err '+e);label(h264Active?'H264':(fb?'JPEG':'MJPEG'));});}" // BUILD2SK45+SK57+SK59: stale-reconnect jen v MJPEG rezimu; "seq" v /status je porad ta sama zachytavaci sekvence i v H264 rezimu
                + "setInterval(pollFps,1000);})();</script></body></html>";
        byte[] b = body.getBytes("UTF-8");
        napTvWebHeader(out, "200 OK", "text/html; charset=utf-8", b.length, false);
        out.write(b);
    }

    private void napTvWebWriteFrame(OutputStream out) throws IOException {
        napTvWebEnsurePlaceholderFrame("FRAME_CONNECT");
        byte[] img = napTvWebJpeg;
        if (img == null) {
            try { Thread.sleep(120); } catch (InterruptedException ignored) {}
            img = napTvWebJpeg;
        }
        if (img == null) {
            byte[] body = "frame not ready\n".getBytes("UTF-8");
            napTvWebHeader(out, "503 Service Unavailable", "text/plain; charset=utf-8", body.length, true);
            out.write(body);
            return;
        }
        napTvWebHeader(out, "200 OK", "image/jpeg", img.length, true);
        out.write(img);
    }

    private void napTvWebWriteAudioRaw(OutputStream out, long after) throws IOException {
        // BUILD2SB1 (fix TV audio chrceni, strana A - streamer):
        // 1) resync uz neskace 0.8 s zpet, ale jen na maly polstar ~0.25 s pred konec ringu
        // 2) KAZDY nespojity strih (resync i orez pres maxBytes) se ohlasi hlavickou
        //    X-NAP-AUDIO-DISCONTINUITY: 1, aby TV klient udelal fade misto tvrdeho lepeni
        byte[] body;
        long start;
        long end;
        int rate;
        long age;
        boolean discontinuity;
        synchronized (napTvWebAudioLock) {
            end = napTvWebAudioSeq & ~3L;
            rate = napTvWebAudioRate;
            age = napTvWebAudioLastPushMs == 0 ? 999999 : (System.currentTimeMillis() - napTvWebAudioLastPushMs);
            int cap = napTvWebAudioRing.length;
            long oldest = Math.max(0, end - cap);
            if (after >= 0 && after >= oldest && after <= end) {
                start = after & ~3L;
                discontinuity = false;
            } else {
                // maly polstar: ~0.25 s pri 16bit stereo (rate*4 bajtu/s -> rate bajtu = 1/4 s)
                long pad = Math.max(8192L, (long) rate) & ~3L;
                start = Math.max(oldest, end - pad) & ~3L;
                discontinuity = true;
            }
            long maxBytes = Math.min(65536L, Math.max(8192L, (rate * 4L) / 3L));
            if (end - start > maxBytes) {
                // klient zaostal vic nez maxBytes: orez = taky nespojity strih, ohlasit
                start = (end - maxBytes) & ~3L;
                discontinuity = true;
            }
            int len = (int)Math.max(0, end - start);
            body = new byte[len];
            for (int i = 0; i < len; i++) {
                body[i] = napTvWebAudioRing[(int)((start + i) % cap)];
            }
        }
        String h = "HTTP/1.1 200 OK\r\n"
                + "Content-Type: application/octet-stream\r\n"
                + "Cache-Control: no-store, no-cache, must-revalidate, max-age=0\r\n"
                + "Pragma: no-cache\r\n"
                + "X-NAP-AUDIO-START: " + start + "\r\n"
                + "X-NAP-AUDIO-SEQ: " + end + "\r\n"
                + "X-NAP-AUDIO-RATE: " + rate + "\r\n"
                + "X-NAP-AUDIO-AGE: " + age + "\r\n"
                + "X-NAP-AUDIO-DISCONTINUITY: " + (discontinuity ? 1 : 0) + "\r\n" // BUILD2SB1
                + "Content-Length: " + body.length + "\r\n"
                + "Connection: close\r\n\r\n";
        out.write(h.getBytes("ISO-8859-1"));
        out.write(body);
    }

    private void napTvWebWriteMjpeg(OutputStream out, Socket sock) throws IOException {
        napTvWebEnsurePlaceholderFrame("MJPEG_CONNECT");
        String h = "HTTP/1.1 200 OK\r\n"
                + "Content-Type: multipart/x-mixed-replace; boundary=napframe\r\n"
                + "Cache-Control: no-store, no-cache, must-revalidate, max-age=0\r\n"
                + "Pragma: no-cache\r\n"
                + "Connection: close\r\n\r\n";
        out.write(h.getBytes("ISO-8859-1"));
        // BUILD2SK42: primy dukaz z /log (SK41 periodicke logovani) ukazal, ze
        // napTvWebSeq roste NEPRETRZITE po celou dobu testu, na VSECH
        // obrazovkach (Atari, DJ pult, uvodni stranka) - zachytavani tedy
        // FUNGUJE. Presto uzivatel hlasi "mrtvy obraz, 0fps". Zavěr: problem
        // neni v zachytavani (to jsem opravoval v SK38-40), ale v DORUCENI -
        // konkretne muze out.write()/out.flush() na TCP socketu BLOKOVAT
        // donekonecna, kdyz sit/prohlizec neni pripraveny cist (slaby WiFi,
        // prohlizec na pozadi apod.) - a tohle bezi na SAMOSTATNEM vlakne,
        // nezavisle na zachytavaci smycce, takze by presne vysvětlovalo
        // "seq roste, ale obraz mrtvy".
        // Reseni: hlidaci vlakno sleduje, jestli zapis skutecne pokracuje - a
        // pokud se pres 5s nepodari zapsat (zatimco jsou nove snimky k
        // dispozici), nasilim zavre socket, coz preruší zablokovany zapis
        // vyjimkou a umozni pripojeni korektne skoncit (prohlizec pak muze
        // znovu pripojit).
        final long[] lastOkMs = { System.currentTimeMillis() };
        final long[] lastWrittenSeq = { -1L };
        Thread watchdog = new Thread(() -> {
            try {
                while (napTvWebRunning && !sock.isClosed()) {
                    Thread.sleep(2000);
                    if (sock.isClosed()) return;
                    long curSeq = napTvWebSeq;
                    long written = lastWrittenSeq[0];
                    long stuckMs = System.currentTimeMillis() - lastOkMs[0];
                    // BUILD2SK42: "zasekle" jen kdyz je NOVY obsah k dispozici
                    // (curSeq != written), ale zaroven pres 8s neprobehl zadny
                    // uspesny zapis. Kdyby appka jen NEMELA co noveho poslat
                    // (staticka obrazovka, zadny novy snimek), to NENI zaseknuti -
                    // takova situace nesmi spustit zabiti zdraveho spojeni.
                    if (curSeq != written && stuckMs > 8000L) {
                        appendNativeLog("BUILD2SK42 TV_WEB_MJPEG_WATCHDOG_KILL stuckMs=" + stuckMs + " curSeq=" + curSeq + " lastWritten=" + written);
                        try { sock.close(); } catch (Throwable ignored) {}
                        return;
                    }
                }
            } catch (InterruptedException ignored) {}
        }, "napTvWebMjpegWatchdog");
        watchdog.setDaemon(true);
        watchdog.start();
        try {
            long last = -1;
            while (napTvWebRunning) {
                byte[] img = napTvWebJpeg;
                long seq = napTvWebSeq;
                if (img != null && seq != last) {
                    out.write(("--napframe\r\nContent-Type: image/jpeg\r\nContent-Length: " + img.length + "\r\n\r\n").getBytes("ISO-8859-1"));
                    out.write(img);
                    out.write("\r\n".getBytes("ISO-8859-1"));
                    out.flush();
                    last = seq;
                    lastOkMs[0] = System.currentTimeMillis();
                    lastWrittenSeq[0] = seq;
                }
                try { Thread.sleep(45); } catch (InterruptedException ignored) {}
            }
        } finally {
            watchdog.interrupt();
        }
    }

    /**
     * BUILD2SA14: rozhrani pro obrazovku HELP, kde se testuje jadro Atari v C++.
     * Puvodni Atari (emu_vbxe) zustava netknute a bezi dal jako dosud - tohle
     * je oddelena vetev, aby se dalo vyvijet bez rizika pro beta verzi.
     */
    /** BUILD2SA21: rozhrani pro uvodni intro. */
    public class AHIntro {
        @JavascriptInterface public void hotovo() {
            napIntroUkazano = true;
            appendNativeLog("BUILD2SA21 INTRO_HOTOVO");
        }
        /** BUILD2SA23: intro pustene rucne z nabidky OPTIONS. */
        @JavascriptInterface public void znovu() {
            appendNativeLog("BUILD2SA23 INTRO_NA_POZADANI");
        }
        /** BUILD2SA27: stazeni Sonica a BIOSu na vyzadani z nabidky OPTIONS. */
        @JavascriptInterface public void stahniSoubory() {
            ui.post(() -> {
                try {
                    NapStahovaniSeSouhlasem.zeptejSeAStahni(MainActivity.this,
                            getPublicAtariHelpDownloadsDir(),
                            zprava -> appendNativeLog("BUILD2SA27 STAZENI " + zprava));
                } catch (Throwable t) {
                    appendNativeLog("BUILD2SA27 STAZENI_CHYBA " + safeMsg(t));
                }
            });
        }
    }

    public class AHAtariCpp {
        @JavascriptInterface public boolean jeNactene() {
            boolean ok;
            try { ok = NativeAtariCoreBridge.isLoaded(); } catch (Throwable t) { ok = false; }
            String err = null;
            try { err = NativeAtariCoreBridge.loadError(); } catch (Throwable ignored) {}
            appendNativeLog("BUILD2SA14 ATARI_CPP_OTEVRENO knihovna="
                    + (ok ? "NACTENA" : "CHYBI") + (err == null ? "" : " duvod=" + err));
            return ok;
        }
        @JavascriptInterface public String samotest() {
            try {
                String r = NativeAtariCoreBridge.runSelfTestSafe();
                // Jeden radek, ktery se da vygrepovat a obsahuje VSECHNO -
                // vcetne ocekavanych hodnot, aby log stal sam o sobe.
                String verdikt = "CHYBA";
                try {
                    boolean cpuOk = r.contains("\"cpuHash\":\"" + NativeAtariCoreBridge.OCEKAVANY_CPU_HASH + "\"")
                            && r.contains("\"cpuInstr\":" + NativeAtariCoreBridge.OCEKAVANO_INSTRUKCI);
                    boolean memOk = r.contains("\"memHash\":\"" + NativeAtariCoreBridge.OCEKAVANY_MEM_HASH + "\"")
                            && r.contains("\"memReads\":" + NativeAtariCoreBridge.OCEKAVANO_CTENI);
                    if (!r.contains("\"chyba\"")) verdikt = (cpuOk && memOk) ? "SEDI" : "NESEDI";
                } catch (Throwable ignored) {}
                appendNativeLog("BUILD2SA14 ATARI_CPP_SELFTEST vysledek=" + verdikt
                        + " ocekavano cpuHash=" + NativeAtariCoreBridge.OCEKAVANY_CPU_HASH
                        + " memHash=" + NativeAtariCoreBridge.OCEKAVANY_MEM_HASH
                        + " cpuInstr=" + NativeAtariCoreBridge.OCEKAVANO_INSTRUKCI
                        + " memReads=" + NativeAtariCoreBridge.OCEKAVANO_CTENI
                        + " | namereno " + r);
                return r;
            } catch (Throwable t) {
                String e = "{\"chyba\":\"" + String.valueOf(t.getMessage()).replace('"', '\'') + "\"}";
                appendNativeLog("BUILD2SA14 ATARI_CPP_SELFTEST vysledek=VYJIMKA " + e);
                return e;
            }
        }
        @JavascriptInterface public String ocekavane() {
            return "{\"cpuHash\":\"" + NativeAtariCoreBridge.OCEKAVANY_CPU_HASH
                 + "\",\"memHash\":\"" + NativeAtariCoreBridge.OCEKAVANY_MEM_HASH
                 + "\",\"cpuInstr\":" + NativeAtariCoreBridge.OCEKAVANO_INSTRUKCI
                 + ",\"memReads\":" + NativeAtariCoreBridge.OCEKAVANO_CTENI + "}";
        }

        /** Cely log teto relace. Nepotrebuje web viewer ani sit. */
        @JavascriptInterface public String log() {
            try {
                synchronized (nativeLog) { return nativeLog.toString(); }
            } catch (Throwable t) { return "log se nepodarilo precist: " + t.getMessage(); }
        }

        /** Jen radky, ktere se tykaji Atari v C++ - kratke, na obrazovku. */
        @JavascriptInterface public String logAtari() {
            try {
                String cely;
                synchronized (nativeLog) { cely = nativeLog.toString(); }
                StringBuilder sb = new StringBuilder();
                for (String r : cely.split("\n")) {
                    if (r.contains("BUILD2SA14") || r.contains("VERZE APKY")
                            || r.contains("napatari") || r.contains("ATARI_CPP")) {
                        sb.append(r).append('\n');
                    }
                }
                return sb.length() == 0 ? "(zatim nic)" : sb.toString();
            } catch (Throwable t) { return "chyba: " + t.getMessage(); }
        }

        /**
         * BUILD2SA18: SKUTECNY stroj - OS z ROM, BASIC, self-test.
         * Nic se tu nevymysli: nabootuje se OS, pocka se na READY,
         * napise se BYE a self-test prevezme stroj. Vse z Reneho ROM.
         */
        @JavascriptInterface public String atariBoot() {
            String r = NativeAtariCoreBridge.bootSafe(600);
            appendNativeLog("BUILD2SA18 ATARI_BOOT " + r);
            return r;
        }
        @JavascriptInterface public String atariDoSelfTestu() {
            // BYE + RETURN -> OS predа rizeni self-testu v ROM
            for (char c : new char[]{'B','Y','E','\n'})
                NativeAtariCoreBridge.keySafe(NativeAtariCoreBridge.kbcode(c), 8);
            NativeAtariCoreBridge.runSafe(400);
            String r = NativeAtariCoreBridge.screenSafe();
            String hlava = r.length() > 200 ? r.substring(0, 200) : r;
            appendNativeLog("BUILD2SA18 ATARI_SELFTEST_VSTUP " + hlava);
            return r;
        }
        @JavascriptInterface public String atariObraz(int snimku) {
            if (snimku > 0) NativeAtariCoreBridge.runSafe(snimku);
            return NativeAtariCoreBridge.screenSafe();
        }
        @JavascriptInterface public String atariKonzole(String ktera) {
            int maska = 7;
            if ("start".equals(ktera))  maska = 6;   // bit0 dolu
            if ("select".equals(ktera)) maska = 5;   // bit1 dolu
            if ("option".equals(ktera)) maska = 3;   // bit2 dolu
            NativeAtariCoreBridge.consolSafe(maska, 12);
            NativeAtariCoreBridge.runSafe(30);
            appendNativeLog("BUILD2SA18 ATARI_KONZOLE " + ktera);
            return NativeAtariCoreBridge.screenSafe();
        }

        /** Vysledek jednoho kroku testu. Rene klepne, ja to mam v logu. */
        @JavascriptInterface public String zapisKrok(int cislo, String popis, boolean ok) {
            try {
                appendNativeLog("BUILD2SA14 TEST_KROK " + cislo
                        + " vysledek=" + (ok ? "V_PORADKU" : "SPATNE")
                        + " co=" + (popis == null ? "" : popis));
                return "{\"ok\":true}";
            } catch (Throwable t) { return "{\"ok\":false}"; }
        }

        /** Kde log lezi na disku - presne cesty, i ta pro pripad padu. */
        @JavascriptInterface public String cestyKLogu() {
            String akt = "(nevytvoren)", pred = "(zadny)", vel = "0";
            try {
                File f = napTvWebLogFileDownloads;
                if (f != null) {
                    akt = f.getAbsolutePath();
                    vel = String.valueOf(f.length());
                    File p = new File(f.getParentFile(), "PS1_LOG_predchozi.txt");
                    if (p.exists()) pred = p.getAbsolutePath() + "  (" + p.length() + " B)";
                }
            } catch (Throwable ignored) {}
            return "{\"aktualni\":\"" + akt.replace('\\', '/') + "\",\"bajtu\":" + vel
                 + ",\"predchozi\":\"" + pred.replace('\\', '/') + "\"}";
        }

        /**
         * BUILD2SA15: ULOZI log do souboru a rovnou otevre odeslani.
         * Schranka sama o sobe nestaci - Rene by pak musel hledat poznamkovy
         * blok, kam ji vlozit. Tady vznikne soubor a hned se nabidne, cim ho
         * poslat. Do schranky se to da taky, jako pojistka.
         */
        @JavascriptInterface public String ulozAOdesli() {
            try {
                final String t;
                synchronized (nativeLog) { t = nativeLog.toString(); }

                // 1) soubor - slozka appky na externim ulozisti jde VZDY
                File cil = null; String chybaZapisu = null;
                try {
                    File dir = getExternalFilesDir(null);
                    if (dir == null) dir = getFilesDir();
                    if (!dir.exists()) dir.mkdirs();
                    String jmeno = "EMU10_LOG_" + new java.text.SimpleDateFormat(
                            "yyyyMMdd_HHmmss", java.util.Locale.US).format(new java.util.Date()) + ".txt";
                    File f = new File(dir, jmeno);
                    try (FileOutputStream fos = new FileOutputStream(f, false)) {
                        fos.write(t.getBytes("UTF-8"));
                        fos.flush();
                    }
                    cil = f;
                } catch (Throwable e) { chybaZapisu = String.valueOf(e.getMessage()); }

                // 2) schranka jako pojistka
                final String txt = t;
                runOnUiThread(new Runnable() { public void run() {
                    try {
                        android.content.ClipboardManager cm =
                            (android.content.ClipboardManager) getSystemService(CLIPBOARD_SERVICE);
                        cm.setPrimaryClip(android.content.ClipData.newPlainText("EMU10 log", txt));
                    } catch (Throwable ignored) {}
                }});

                // 3) hned nabidnout odeslani
                final File odeslat = cil;
                if (odeslat != null) {
                    runOnUiThread(new Runnable() { public void run() {
                        try {
                            // Projekt nema androidx, takze neni FileProvider.
                            // Na Androidu 7+ by file:// hodilo FileUriExposedException,
                            // proto se to hlidani vypne. Je to ustupek, ale funguje
                            // a nepridava do buildu zavislost, ktera by ho mohla shodit.
                            if (Build.VERSION.SDK_INT >= 24) {
                                android.os.StrictMode.setVmPolicy(
                                        new android.os.StrictMode.VmPolicy.Builder().build());
                            }
                            android.content.Intent i = new android.content.Intent(
                                    android.content.Intent.ACTION_SEND);
                            i.setType("text/plain");
                            i.putExtra(android.content.Intent.EXTRA_SUBJECT,
                                    "EMU10 log");
                            i.putExtra(android.content.Intent.EXTRA_STREAM,
                                    android.net.Uri.fromFile(odeslat));
                            i.putExtra(android.content.Intent.EXTRA_TEXT,
                                    "Log z EMU10, soubor: " + odeslat.getAbsolutePath());
                            i.addFlags(android.content.Intent.FLAG_GRANT_READ_URI_PERMISSION);
                            startActivity(android.content.Intent.createChooser(i, "Poslat log"));
                        } catch (Throwable e) {
                            appendNativeLog("BUILD2SA15 LOG_ODESLANI SELHALO duvod=" + e.getMessage());
                        }
                    }});
                }

                appendNativeLog("BUILD2SA15 ATARI_CPP_LOG_ULOZEN znaku=" + t.length()
                        + " soubor=" + (odeslat == null ? ("SELHAL:" + chybaZapisu) : odeslat.getAbsolutePath()));
                return "{\"ok\":" + (odeslat != null) + ",\"znaku\":" + t.length()
                     + ",\"soubor\":\"" + (odeslat == null ? String.valueOf(chybaZapisu)
                        : odeslat.getAbsolutePath()).replace('\\','/').replace('"','\'') + "\"}";
            } catch (Throwable t) {
                return "{\"ok\":false,\"chyba\":\"" + String.valueOf(t.getMessage()).replace('"','\'') + "\"}";
            }
        }
    }

    public class AHTvWeb {
        @JavascriptInterface public String start() { return napTvWebStart(); }
        @JavascriptInterface public String startScreen() {
            String r = napTvWebStart();
            napTvWebRequestSystemMirror();
            return r + " SCREEN_REQUEST";
        }
        @JavascriptInterface public String openWithScreen(String url) {
            String r = napTvWebStart();
            napTvWebRequestSystemMirror(url);
            return r + " SCREEN_OPEN_REQUEST";
        }
        @JavascriptInterface public String youtubeInApp() {
            napTvWebOpenYoutubeInApp();
            return "YOUTUBE_IN_APP";
        }
        @JavascriptInterface public String youtube() {
            return youtubeInApp();
        }
        @JavascriptInterface public String setSoundState(String json) {
            return napTvWebSetSoundStateJson(json);
        }
        @JavascriptInterface public String setPlayerSound(String json) {
            return napTvWebSetSoundStateJson(json);
        }
        @JavascriptInterface public String screen() {
            napTvWebRequestSystemMirror();
            return napTvWebSystemMirrorActive ? "SCREEN_ACTIVE" : "SCREEN_REQUEST";
        }
        @JavascriptInterface public String stop() { return napTvWebStop("js"); }
        @JavascriptInterface public String status() { return napTvWebUrl(); }
        // BUILD2SK91: pro nove nastaveni na uvodni obrazovce (ozubene kolecko) -
        // funguje i kdyz TV-cast HTTP server jeste vubec nebezi (na rozdil od
        // /quality HTTP cesty, kterou pouziva jen LOW/MED/HIGH na strance
        // prohlizece, kdyz uz je cast aktivni) - jde primo do Javy pres JS most,
        // ktery je dostupny na KAZDE strance nacitane v tomhle WebView.
        @JavascriptInterface public String setQualityTier(int tier) {
            return "tier=" + napTvWebSetQualityTier(tier);
        }
        @JavascriptInterface public int getQualityTier() {
            return napTvWebQualityTier;
        }
        @JavascriptInterface public String pushAtariPcm16(String b64, int sampleRate, int frames) {
            try {
                if (!napTvWebRunning) return "TV_WEB_AUDIO_OFF";
                if (b64 == null || b64.length() == 0) return "TV_WEB_AUDIO_EMPTY";
                byte[] data = Base64.decode(b64, Base64.DEFAULT);
                napTvWebAudioPushMonoPcm16Bytes(data, sampleRate, "ATARI");
                return "TV_WEB_AUDIO_ATARI_OK bytes=" + data.length + " frames=" + frames + " hz=" + sampleRate;
            } catch (Throwable t) {
                String e = "TV_WEB_AUDIO_ATARI_FAIL " + safeMsg(t);
                appendNativeLog("BUILD2SA13C " + e);
                return e;
            }
        }
        @JavascriptInterface public String pushPlayerPcm16(String b64, int sampleRate, int frames, int channels) {
            try {
                if (!napTvWebRunning) return "TV_WEB_AUDIO_OFF";
                if (b64 == null || b64.length() == 0) return "TV_WEB_AUDIO_EMPTY";
                byte[] data = Base64.decode(b64, Base64.DEFAULT);
                if (channels <= 1) napTvWebAudioPushMonoPcm16Bytes(data, sampleRate, "PLAYER");
                else napTvWebAudioPushStereoPcm16Bytes(data, sampleRate, "PLAYER");
                return "TV_WEB_AUDIO_PLAYER_OK bytes=" + data.length + " frames=" + frames + " hz=" + sampleRate + " ch=" + channels;
            } catch (Throwable t) {
                String e = "TV_WEB_AUDIO_PLAYER_FAIL " + safeMsg(t);
                appendNativeLog("BUILD2SA13C " + e);
                return e;
            }
        }
        @JavascriptInterface public String pushYoutubePcm16(String b64, int sampleRate, int frames, int channels) {
            try {
                if (!napTvWebRunning) return "TV_WEB_AUDIO_OFF";
                if (b64 == null || b64.length() == 0) return "TV_WEB_AUDIO_EMPTY";
                byte[] data = Base64.decode(b64, Base64.DEFAULT);
                if (channels <= 1) napTvWebAudioPushMonoPcm16Bytes(data, sampleRate, "YOUTUBE");
                else napTvWebAudioPushStereoPcm16Bytes(data, sampleRate, "YOUTUBE");
                return "TV_WEB_AUDIO_YOUTUBE_OK bytes=" + data.length + " frames=" + frames + " hz=" + sampleRate + " ch=" + channels;
            } catch (Throwable t) {
                String e = "TV_WEB_AUDIO_YOUTUBE_FAIL " + safeMsg(t);
                appendNativeLog("BUILD2SA13C16 " + e);
                return e;
            }
        }
    }
    private NativeInPlaceView nativeInPlaceView;
    private boolean nativeInPlaceEnabled = false;
    private String nativeLastRomInfo = "C++ ROM zatim nenactena";
    private String nativeLastStatus = "NATIVE_OFF";
    private int nativeInputEvents = 0;
    private final StringBuilder nativeLog = new StringBuilder();
    // BUILD2SK82: nativeLog (vyse) je in-memory ring omezeny na 100KB (viz
    // appendNativeLog) - u delsiho rucniho testu (Sega, Atari, DJ pult, domu,
    // Sega LOW/MED/HIGH, PS1 HIGH v jedne relaci) starsi radky VYPADNOU drive,
    // nez se stihne otevrit /log na konci - v praxi zbyde jen posledni
    // testovana obrazovka. Reseni: KAZDY radek se navic prubezne zapisuje na
    // disk (soubor cerstvy pri kazdem startu appky, viz napTvWebLogFileInit
    // volane z onCreate) a /log endpoint ted cte PRIMARNE z tohoto souboru
    // (ring zustava jen jako fallback, kdyby zapis na disk selhal) - takze
    // zadna cast testu uz nezmizi, bez ohledu na to, jak dlouho test trva
    // nebo kdy se /log nakonec otevre. Zapis BEZI NA SAMOSTATNEM VLAKNE
    // (fronta + producer/consumer, stejny osvedceny vzor jako H264 feed) -
    // zadne volajici vlakno (UI tick, audio, HTTP) nikdy neceka na disk I/O.
    private final java.util.concurrent.ConcurrentLinkedQueue<String> napTvWebLogFileQueue =
            new java.util.concurrent.ConcurrentLinkedQueue<>();
    private volatile File napTvWebLogFile;
    // Druha kopie logu v Downloads/AtariHelp/PS1_LOG.txt - prezije pad
    // aplikace a jde k ni z telefonu bez pocitace.
    private volatile File napTvWebLogFileDownloads;
    private volatile String napTvWebLogSlozka = "(neurceno)";
    /** BUILD2SA21: intro se ukazuje jen jednou za spusteni aplikace. */
    private static volatile boolean napIntroUkazano = false;
    private volatile boolean napTvWebLogFileWriterStarted = false;
    private volatile boolean napTvWebLogFileCapWarned = false;
    // BUILD2SK82: bezpecnostni strop velikosti log souboru na disku (30MB).
    // Rucni test trva minuty, ne dny - v praxi se nikdy nepriblizi. Je to jen
    // pojistka proti tomu, aby appka pri neceka nekonecne bezici relaci
    // nezaplnila uloziste. Po dosazeni stropu se DALSI zapisy jednoduse
    // prestanou pridavat (soubor zustane platny a citelny az do stropu,
    // misto rizikoveho prepisovani/orezavani zacatku za behu) a zapise se
    // JEDNA viditelna hlaska, misto ticheho zahazovani.
    private static final long napTvWebLogFileCapBytes = 30L * 1024 * 1024;
    private volatile boolean nativeCoreAudioRun = false;
    private volatile int nativeAudioGeneration = 0; // BUILD2RV: kills stale AudioTrack threads after every ROM change; prevents cumulative slowdown.
    private Thread nativeCoreAudioThread;
    private volatile AudioTrack nativeCurrentAudioTrack;
    private volatile int nativeActiveAudioTracks = 0; // BUILD2RV: proof that only one Sega AudioTrack path is alive.
    private ValueCallback<Uri[]> pendingChooser;
    private byte[] pendingGame;
    private String pendingName;
    private int pendingGameInjectSeq = 0;
    private String lastAhGameBridgeUrl = "";
    private long lastAhGameBridgeAtMs = 0L;
    private String pendingBridgeKind;
    private final Handler ui = new Handler(Looper.getMainLooper());
    private volatile int nativeRomLoadGeneration = 0;
    private volatile long nativeViewDrawCounter = 0;
    private volatile long nativeViewDrawCounterAtRomLoad = 0;
    private volatile boolean nativeLandscapeFullVideo = false;
    private volatile long nativeLastRenderCostNs = 0;
    private volatile long nativeRenderPerfWindowStartMs = 0;
    private volatile long nativeRenderPerfWindowFrames = 0;
    private volatile long nativeRenderPerfSlowFrames = 0;
    private volatile String nativePerformanceMode = "HIGH"; // BUILD2RV: SBIRKA selects HIGH QUALITY / LOW PERFORMANCE.
    private volatile long nativeLastSaveLogAtMs = 0; // BUILD2RV: one tap must not create 4 log files.

    private String safeFileName(String name) {
        if (name == null || name.trim().length() == 0) name = "AtariHelp_file.bin";
        String safe = name.replaceAll("[^A-Za-z0-9._-]", "_");
        if (safe.length() > 96) safe = safe.substring(0, 96);
        return safe;
    }

    private File getPublicAtariHelpDownloadsDir() {
        File base = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS);
        File dir = new File(base, "AtariHelp");
        if (!dir.exists()) dir.mkdirs();
        return dir;
    }

    private String mimeForName(String name) {
        String lower = name == null ? "" : name.toLowerCase();
        if (lower.endsWith(".cas")) return "application/octet-stream";
        if (lower.endsWith(".wav")) return "audio/wav";
        if (lower.endsWith(".txt") || lower.endsWith(".log")) return "text/plain";
        return "application/octet-stream";
    }

    private String writeBytesToDownloads(String name, byte[] data) throws IOException {
        String safe = safeFileName(name);

        // Android 10+ spravne pres MediaStore do viditelne slozky Downloads/AtariHelp.
        if (Build.VERSION.SDK_INT >= 29) {
            ContentValues values = new ContentValues();
            values.put(MediaStore.MediaColumns.DISPLAY_NAME, safe);
            values.put(MediaStore.MediaColumns.MIME_TYPE, mimeForName(safe));
            values.put(MediaStore.MediaColumns.RELATIVE_PATH, Environment.DIRECTORY_DOWNLOADS + "/AtariHelp");
            Uri uri = getContentResolver().insert(MediaStore.Downloads.EXTERNAL_CONTENT_URI, values);
            if (uri == null) throw new IOException("MediaStore insert vratil null");
            OutputStream out = getContentResolver().openOutputStream(uri);
            if (out == null) throw new IOException("MediaStore openOutputStream vratil null");
            try { out.write(data); } finally { out.close(); }
            return "Downloads/AtariHelp/" + safe;
        }

        // Android 9 a starsi / NOX: klasicka verejna slozka Downloads/AtariHelp.
        File dir = getPublicAtariHelpDownloadsDir();
        File f = new File(dir, safe);
        int n = 1;
        String base = safe;
        String ext = "";
        int dot = safe.lastIndexOf('.');
        if (dot > 0) { base = safe.substring(0, dot); ext = safe.substring(dot); }
        while (f.exists()) {
            f = new File(dir, base + "_" + n + ext);
            n++;
        }
        FileOutputStream out = new FileOutputStream(f);
        try { out.write(data); } finally { out.close(); }
        return f.getAbsolutePath();
    }

    private String appendNativeDebugToSavedLog(String name, String text) {
        String out = text == null ? "" : text;
        String n = name == null ? "" : name.toLowerCase(Locale.US);
        if (!(n.endsWith(".log") || n.endsWith(".txt") || n.contains("log"))) return out;
        StringBuilder sb = new StringBuilder(out.length() + 4096);
        sb.append(out);
        sb.append("\n\n==== ANDROID MAINACTIVITY LOG BUILD2SA5AH ====\n");
        synchronized (nativeLog) { sb.append(nativeLog.toString()); }
        return sb.toString();
    }

    public class AHSave {
        @JavascriptInterface
        public String save(String name, String text) {
            try {
                String outText = appendNativeDebugToSavedLog(name, text);
                String path = writeBytesToDownloads(name, outText.getBytes("UTF-8"));
                return "DOWNLOADS_OK:" + path;
            } catch (Exception e) {
                try {
                    File dir = getExternalFilesDir(null);
                    if (dir == null) dir = getFilesDir();
                    File f = new File(dir, safeFileName(name));
                    String outText = appendNativeDebugToSavedLog(name, text);
                    FileOutputStream out = new FileOutputStream(f);
                    out.write(outText.getBytes("UTF-8"));
                    out.close();
                    return "FALLBACK_APP_DIR:" + f.getAbsolutePath() + " | DOWNLOADS_CHYBA:" + e.getMessage();
                } catch (Exception e2) {
                    return "CHYBA: " + e.getMessage() + " / fallback: " + e2.getMessage();
                }
            }
        }

        @JavascriptInterface
        public String saveBase64(String name, String b64) {
            String clean = b64 == null ? "" : b64.replaceAll("\\s", "");
            int mod = clean.length() % 4;
            if (mod == 2) clean += "==";
            else if (mod == 3) clean += "=";
            try {
                byte[] data = Base64.decode(clean, Base64.DEFAULT);
                String path = writeBytesToDownloads(name, data);
                return "DOWNLOADS_OK:" + path;
            } catch (Exception e) {
                try {
                    File dir = getExternalFilesDir(null);
                    if (dir == null) dir = getFilesDir();
                    File f = new File(dir, safeFileName(name));
                    byte[] data = Base64.decode(clean, Base64.DEFAULT);
                    FileOutputStream out = new FileOutputStream(f);
                    out.write(data);
                    out.close();
                    return "FALLBACK_APP_DIR:" + f.getAbsolutePath() + " | DOWNLOADS_CHYBA:" + e.getMessage();
                } catch (Exception e2) {
                    return "CHYBA: " + e.getMessage() + " / fallback: " + e2.getMessage();
                }
            }
        }
    }

    private String ps1CleanBiosName(String name) {
        if (name == null) name = "bios.bin";
        return name.toLowerCase(Locale.US).replaceAll("[^a-z0-9._-]", "_");
    }

    private void ps1WriteBytes(File f, byte[] data) throws IOException {
        FileOutputStream fo = new FileOutputStream(f);
        try { fo.write(data); } finally { fo.close(); }
    }

    private String ps1WriteBiosAliases(File dir, String clean, byte[] data) throws IOException {
        if (dir == null || clean == null || data == null || data.length != 524288) return "";
        String alias = "";
        String compact = clean.replace("-", "").replace("_", "");
        String canonical = null;
        if (compact.matches("scph[0-9a-z]+\\.bin")) canonical = compact;
        else if ("psxonpsp660.bin".equals(clean) || "psxonpsp660.bin".equals(compact)) canonical = "PSXONPSP660.bin";
        else if (clean.contains("5500")) canonical = "scph5500.bin";
        else if (clean.contains("5501")) canonical = "scph5501.bin";
        else if (clean.contains("5502")) canonical = "scph5502.bin";
        else if (clean.contains("1001")) canonical = "scph1001.bin";
        else if (clean.contains("7502")) canonical = "scph7502.bin";
        else if (clean.contains("7001")) canonical = "scph7001.bin";
        else if (clean.contains("7003")) canonical = "scph7003.bin";

        if (canonical != null && !canonical.equals(clean)) {
            File cf = new File(dir, canonical);
            ps1WriteBytes(cf, data);
            alias += " alias=" + cf.getAbsolutePath();
        } else if (canonical == null && !clean.matches("scph[0-9a-z]+\\.bin")) {
            File cf = new File(dir, "scph5501.bin");
            ps1WriteBytes(cf, data);
            alias += " alias=" + cf.getAbsolutePath();
        }
        return alias;
    }

    public class AHPS1 {
        @JavascriptInterface
        public String ps1CoreInfo() { return NativePs1CoreBridge.coreInfoSafe(); }
        // BUILD2SA2/SA5P: ulozi BIOS .bin do systemove slozky jadra, pokud ho uzivatel sam vybere.
        @JavascriptInterface
        public String ps1SaveBios(String name, String b64) {
            try {
                if (name == null || b64 == null) return "PS1_BIOS_SAVE_FAIL empty";
                java.io.File dir = new java.io.File(getFilesDir(), "ps1_system");
                if (!dir.exists() && !dir.mkdirs()) return "PS1_BIOS_SAVE_FAIL mkdir";
                String clean = ps1CleanBiosName(name);
                byte[] data = Base64.decode(b64, Base64.DEFAULT);
                java.io.File f = new java.io.File(dir, clean);
                ps1WriteBytes(f, data);
                String alias = ps1WriteBiosAliases(dir, clean, data);
                return "PS1_BIOS_SAVED path=" + f.getAbsolutePath() + alias + " bytes=" + data.length;
            } catch (Throwable t) { return "PS1_BIOS_SAVE_FAIL " + t.getMessage(); }
        }
        // BUILD2SA2: nativni vyber hry -> fd -> /proc/self/fd (700MB bez kopirovani)
        @JavascriptInterface
        public void ps1PickGame() {
            ui.post(() -> {
                try {
                    Intent i = new Intent(Intent.ACTION_GET_CONTENT);
                    i.setType("*/*");
                    i.addCategory(Intent.CATEGORY_OPENABLE);
                    startActivityForResult(Intent.createChooser(i, "PS1: vyber .bin / .iso / .img hry"), PICK_PS1_GAME);
                } catch (Throwable t) { ps1LastBootResult = "PS1_PICK_FAIL " + t.getMessage(); }
            });
        }
        @JavascriptInterface
        public void ps1DownloadAndBoot(String url) {
            startPs1RemoteDownloadAndBoot(url);
        }
        @JavascriptInterface
        public String ps1Status() {
            // Kdyz se otevre obrazovka PS1 a nic nebezi, nastartujeme konzoli
            // BEZ disku - jako kdyz zapnes skutecnou PlayStation: nabehne BIOS
            // a jeho menu (MEMORY CARD / CD PLAYER). Obraz jde do monitoru,
            // protoze stranka si uz snimky sama tahá pres ps1FramePreviewB64().
            ps1MaybeStartBios();
            return NativePs1CoreBridge.statusSafe() + " | lastBoot=" + ps1LastBootResult + " | remote=" + ps1RemoteDownloadStatus;
        }
        @JavascriptInterface
        public String ps1RemoteStatus() { return ps1RemoteDownloadStatus; }
        @JavascriptInterface
        public String ps1RemoteCachePath() {
            try { return ps1RemoteGamesDir().getAbsolutePath(); }
            catch (Throwable t) { return "PS1_CACHE_PATH_FAIL " + safeMsg(t); }
        }
        @JavascriptInterface
        public String ps1ClearRemoteCache() {
            try { return clearPs1RemoteCaches(); }
            catch (Throwable t) { return "PS1_CACHE_CLEAR_FAIL " + safeMsg(t); }
        }
        @JavascriptInterface
        public void ps1Input(String button, boolean down) {
            int id = ps1ButtonId(button);
            if (id >= 0) NativePs1CoreBridge.setInputSafe(id, down);
        }
        private File ps1StateFile() {
            File saveDir = new File(getFilesDir(), "ps1_saves");
            if (!saveDir.exists()) saveDir.mkdirs();
            String base = safeFileName(ps1CurrentGameLabel == null ? "ps1_game" : ps1CurrentGameLabel);
            if (base.endsWith(".bin") || base.endsWith(".iso") || base.endsWith(".img") || base.endsWith(".cue")) {
                int dot = base.lastIndexOf('.');
                if (dot > 0) base = base.substring(0, dot);
            }
            return new File(saveDir, base + ".slot0.state");
        }
        @JavascriptInterface
        public String ps1QuickSave() {
            try {
                File f = ps1StateFile();
                String r = NativePs1CoreBridge.saveStateSafe(f.getAbsolutePath());
                return r;
            } catch (Throwable t) {
                return "PS1_STATE_SAVE_EXCEPTION " + t.getMessage();
            }
        }
        @JavascriptInterface
        public String ps1QuickLoad() {
            try {
                File f = ps1StateFile();
                if (!f.exists()) return "PS1_STATE_LOAD_FAIL missing_file path=" + f.getAbsolutePath();
                String r = NativePs1CoreBridge.loadStateSafe(f.getAbsolutePath());
                return r;
            } catch (Throwable t) {
                return "PS1_STATE_LOAD_EXCEPTION " + t.getMessage();
            }
        }
        @JavascriptInterface
        public String ps1SaveInfo() {
            try {
                File f = ps1StateFile();
                return "PS1_SAVE_SLOT path=" + f.getAbsolutePath() + " exists=" + f.exists() + " bytes=" + (f.exists() ? f.length() : 0);
            } catch (Throwable t) {
                return "PS1_SAVE_SLOT_ERROR " + t.getMessage();
            }
        }
        // BUILD2SA5: nahled obrazu - realne snimky z jadra jako kvalitnejsi JPEG base64.
        // Neni to finalni render (ten pojede pres TextureView v SA2C), ale je to
        // OPRAVDOVY obraz z beziciho jadra, zadny fake.
        // BUILD2SK27: ukazalo se, ze tohle JE zive vykreslovani PS1 (viz emu_ps1/
        // index.html radek ~481 - setInterval kazdych 80ms), na rozdil od Segy,
        // ktera ma vlastni GPU TextureView s kvalitnim hardwarovym zvetsenim.
        // PS1 jadro renderuje na 320x240 (potvrzeno v logu - res=320x240) - tahle
        // funkce driv komprimovala PRESNE tenhle maly snimek do JPEG a nechala
        // prohlizec roztahnout pres CSS az pozdeji. Ted se snimek NEJDRIV kvalitne
        // zvetsi (bilinearni filtr, ne nejblizsi soused) NA 3-nasobek, a az POTOM
        // se komprimuje - JPEG enkoder tak dostane skutecne vic pixelu k praci,
        // coz vypada vyrazne ostreji nez pozdejsi CSS roztazeni maleho JPEGu.
        // BUILD2SK85: DVE veci najednou zjisteny (2026-07-18, Rene): (1) tohle je
        // ZASADNI strop kvality/plynulosti PS1 obrazu (~12fps, JPEG komprimovany
        // JESTE PRED tim, nez cokoli z toho vidi TV-cast H264 pipeline - zadna
        // uroven kvality/bitrate v TV-castu tohle nemuze opravit, protoze uz
        // dostava jen 12x/s aktualizovany, uz jednou zkomprimovany obraz), (2)
        // driv se pri KAZDEM volani (80x/s) alokovaly DVE cerstve bitmapy
        // (320x240 + 960x720 = ~2.8MB) - presne stejna GC-tlak chyba, jakou
        // SK65-66 uz resily u H264 YUV bufferu. Oprava: opakovane pouzivane
        // bitmapy (realokace jen pri zmene rozliseni jadra) - diky tomu je ted
        // BEZPECNE zkratit interval v JS na 40ms (viz emu_ps1/index.html).
        // JPEG komprese samotna zustava (viz komentar vyse - je NUTNA pro prenos
        // pres WebView bridge, poradne odstraneni by znamenalo postavit
        // TextureView cestu jako u Segy - VETSI zmena, zatim NEUDELANA, viz
        // predavaci poznamka).
        // BUILD2SK86: 40ms bylo PRILIS agresivni - zmereny realny log ukazal
        // avgMs~50 (min 33, max 171) PRO JEDNO volani teto funkce, tedy VIC
        // nez samotny 40ms interval. Vysledek: preview smycka bezela prakticky
        // nepretrzite (zadny prostoj mezi volanimi), coz souteziv o CPU
        // stahlo CELOU TV-cast H264 cestu (PixelCopy latence i H264 draw+drain
        // vzrostly 3-4x oproti SK80 zakladu, VSECHNY tri kvalitni urovne PS1
        // razantne pomalejsi - presne to, co Rene nahlasil). JS interval
        // vraceny zpet na 80ms (viz emu_ps1/index.html) - bitmapa reuse (nize)
        // zustava, je to porad cisty prinos, jen sama o sobe nestacila na to,
        // aby byl 40ms bezpecny. Skutecne zrychleni PS1 preview beze skody na
        // TV-castu by vyzadovalo odstranit JPEG kompresi uplne (TextureView
        // pristup jako u Segy) - viz predavaci poznamka, ceka na potvrzeni.
        @JavascriptInterface
        public void ps1SetScreenRect(int l, int t, int w, int h, boolean naSirku) {
            ui.post(() -> ps1PlochaUmisti(l, t, w, h, naSirku));
        }

        @JavascriptInterface
        public String ps1FramePreviewB64() {
            // ===== DRUHA VETEV ZRUSENA =====
            // Obraz kresli NATIVNI PLOCHA primo z jadra. Tahle cesta (JPEG do
            // stranky) bezela vedle ni - v logu "PS1_PREVIEW_AVG" 51x - a brala
            // procesor, ktery pak chybel zvuku, jakmile se pridala TV.
            return "";
        }
        @JavascriptInterface
        public String ps1Stop() {
            return stopPs1SessionHard("jsPs1Stop"); // BUILD2SA5I: one path stops audio + core + fd.
        }
    }
    private int ps1ButtonId(String button) {
        if (button == null) return -1;
        String b = button.toUpperCase(Locale.US);
        if ("CROSS".equals(b) || "X".equals(b)) return 0;       // RetroPad B -> PS1 Cross
        if ("SQUARE".equals(b)) return 1;                       // RetroPad Y -> PS1 Square
        if ("SELECT".equals(b)) return 2;
        if ("START".equals(b)) return 3;
        if ("UP".equals(b)) return 4;
        if ("DOWN".equals(b)) return 5;
        if ("LEFT".equals(b)) return 6;
        if ("RIGHT".equals(b)) return 7;
        if ("CIRCLE".equals(b) || "O".equals(b)) return 8;      // RetroPad A -> PS1 Circle
        if ("TRIANGLE".equals(b)) return 9;                     // RetroPad X -> PS1 Triangle
        if ("L1".equals(b) || "L".equals(b)) return 10;
        if ("R1".equals(b) || "R".equals(b)) return 11;
        if ("L2".equals(b)) return 12;
        if ("R2".equals(b)) return 13;
        return -1;
    }

    private String jsonQuote(String text) {
        if (text == null) text = "";
        StringBuilder sb = new StringBuilder(text.length() + 16);
        sb.append('"');
        for (int i = 0; i < text.length(); i++) {
            char c = text.charAt(i);
            if (c == '"' || c == '\\') sb.append('\\').append(c);
            else if (c == '\n') sb.append("\\n");
            else if (c == '\r') sb.append("\\r");
            else if (c == '\t') sb.append("\\t");
            else if (c < 32) sb.append(' ');
            else sb.append(c);
        }
        sb.append('"');
        return sb.toString();
    }

    private boolean napPlayerIsAudioName(String name) {
        if (name == null) return false;
        String n = name.toLowerCase(Locale.US);
        return n.endsWith(".mp3") || n.endsWith(".wav");
    }

    private boolean napPlayerHasAudioPermission() {
        if (Build.VERSION.SDK_INT < 23) return true;
        try {
            if (Build.VERSION.SDK_INT >= 33) {
                return checkSelfPermission(Manifest.permission.READ_MEDIA_AUDIO) == PackageManager.PERMISSION_GRANTED;
            }
            return checkSelfPermission(Manifest.permission.READ_EXTERNAL_STORAGE) == PackageManager.PERMISSION_GRANTED;
        } catch (Throwable ignored) {
            return true;
        }
    }

    private void napPlayerRequestAudioPermission(String reason) {
        if (Build.VERSION.SDK_INT < 23) return;
        ui.post(() -> {
            try {
                if (napPlayerHasAudioPermission()) return;
                if (Build.VERSION.SDK_INT >= 33) {
                    requestPermissions(new String[]{Manifest.permission.READ_MEDIA_AUDIO}, PICK_AUDIO_PERMISSION);
                } else if (Build.VERSION.SDK_INT < 29) {
                    requestPermissions(new String[]{Manifest.permission.READ_EXTERNAL_STORAGE, Manifest.permission.WRITE_EXTERNAL_STORAGE}, PICK_AUDIO_PERMISSION);
                } else {
                    requestPermissions(new String[]{Manifest.permission.READ_EXTERNAL_STORAGE}, PICK_AUDIO_PERMISSION);
                }
                appendNativeLog("BUILD2SA13C14 PLAYER_AUDIO_PERMISSION_REQUEST reason=" + reason + " sdk=" + Build.VERSION.SDK_INT);
            } catch (Throwable t) {
                appendNativeLog("BUILD2SA13C14 PLAYER_AUDIO_PERMISSION_REQUEST_FAIL " + safeMsg(t));
            }
        });
    }

    private void napPlayerAddAudioItem(ArrayList<NapPlayerAudioItem> out, HashSet<String> seen,
                                       String name, String uri, long size, long modified) {
        if (out == null || seen == null || uri == null || uri.length() == 0) return;
        if (name == null || name.length() == 0) name = "audio";
        if (!napPlayerIsAudioName(name)) return;
        String key = name.toLowerCase(Locale.US) + "|" + size + "|" + modified;
        if (seen.contains(uri) || seen.contains(key)) return;
        seen.add(uri);
        seen.add(key);
        out.add(new NapPlayerAudioItem(name, uri, size, modified));
    }

    private void napPlayerScanAudioDir(File dir, int depth, ArrayList<NapPlayerAudioItem> out, HashSet<String> seen) {
        if (dir == null || depth < 0 || out == null || out.size() >= 500) return;
        try {
            if (!dir.exists() || !dir.isDirectory()) return;
            File[] files = dir.listFiles();
            if (files == null) return;
            for (File f : files) {
                if (f == null || out.size() >= 500) break;
                String n = f.getName();
                if (n == null || n.startsWith(".")) continue;
                if (f.isDirectory()) {
                    napPlayerScanAudioDir(f, depth - 1, out, seen);
                } else if (f.isFile() && napPlayerIsAudioName(n)) {
                    napPlayerAddAudioItem(out, seen, n, Uri.fromFile(f).toString(), f.length(), f.lastModified() / 1000L);
                }
            }
        } catch (Throwable t) {
            appendNativeLog("BUILD2SA13C8 PLAYER_SCAN_DIR_ERR " + safeMsg(t));
        }
    }

    private String napPlayerListLocalAudioJson() {
        ArrayList<NapPlayerAudioItem> items = new ArrayList<>();
        HashSet<String> seen = new HashSet<>();
        try {
            if (!napPlayerHasAudioPermission()) {
                napPlayerRequestAudioPermission("listLocalAudio");
                return "{\"ok\":false,\"needsPermission\":true,\"error\":\"Povol pristup k hudbe v mobilu\",\"items\":[]}";
            }
            String[] projection = new String[] {
                    MediaStore.Audio.Media._ID,
                    MediaStore.Audio.Media.DISPLAY_NAME,
                    MediaStore.Audio.Media.SIZE,
                    MediaStore.Audio.Media.DATE_MODIFIED,
                    MediaStore.Audio.Media.MIME_TYPE
            };
            Cursor c = null;
            try {
                c = getContentResolver().query(MediaStore.Audio.Media.EXTERNAL_CONTENT_URI, projection, null, null, null);
                if (c != null) {
                    int idCol = c.getColumnIndex(MediaStore.Audio.Media._ID);
                    int nameCol = c.getColumnIndex(MediaStore.Audio.Media.DISPLAY_NAME);
                    int sizeCol = c.getColumnIndex(MediaStore.Audio.Media.SIZE);
                    int modCol = c.getColumnIndex(MediaStore.Audio.Media.DATE_MODIFIED);
                    int mimeCol = c.getColumnIndex(MediaStore.Audio.Media.MIME_TYPE);
                    while (c.moveToNext() && items.size() < 500) {
                        long id = idCol >= 0 ? c.getLong(idCol) : -1L;
                        String name = nameCol >= 0 ? c.getString(nameCol) : null;
                        String mime = mimeCol >= 0 ? c.getString(mimeCol) : "";
                        boolean audioMime = mime != null && mime.toLowerCase(Locale.US).startsWith("audio/");
                        if (id < 0 || (!napPlayerIsAudioName(name) && !audioMime)) continue;
                        if (!napPlayerIsAudioName(name)) name = "audio_" + id + ".mp3";
                        long size = sizeCol >= 0 ? c.getLong(sizeCol) : 0L;
                        long modified = modCol >= 0 ? c.getLong(modCol) : 0L;
                        Uri uri = ContentUris.withAppendedId(MediaStore.Audio.Media.EXTERNAL_CONTENT_URI, id);
                        napPlayerAddAudioItem(items, seen, name, uri.toString(), size, modified);
                    }
                }
            } catch (Throwable t) {
                appendNativeLog("BUILD2SA13C8 PLAYER_MEDIASTORE_SCAN_ERR " + safeMsg(t));
            } finally {
                try { if (c != null) c.close(); } catch (Throwable ignored) {}
            }

            try {
                Uri filesUri = MediaStore.Files.getContentUri("external");
                String[] fileProjection = new String[] {
                        MediaStore.Files.FileColumns._ID,
                        MediaStore.Files.FileColumns.DISPLAY_NAME,
                        MediaStore.Files.FileColumns.SIZE,
                        MediaStore.Files.FileColumns.DATE_MODIFIED,
                        MediaStore.Files.FileColumns.MIME_TYPE
                };
                Cursor fc = null;
                try {
                    fc = getContentResolver().query(filesUri, fileProjection, null, null, null);
                    if (fc != null) {
                        int idCol = fc.getColumnIndex(MediaStore.Files.FileColumns._ID);
                        int nameCol = fc.getColumnIndex(MediaStore.Files.FileColumns.DISPLAY_NAME);
                        int sizeCol = fc.getColumnIndex(MediaStore.Files.FileColumns.SIZE);
                        int modCol = fc.getColumnIndex(MediaStore.Files.FileColumns.DATE_MODIFIED);
                        int mimeCol = fc.getColumnIndex(MediaStore.Files.FileColumns.MIME_TYPE);
                        while (fc.moveToNext() && items.size() < 500) {
                            long id = idCol >= 0 ? fc.getLong(idCol) : -1L;
                            String name = nameCol >= 0 ? fc.getString(nameCol) : null;
                            String mime = mimeCol >= 0 ? fc.getString(mimeCol) : "";
                            boolean audioMime = mime != null && mime.toLowerCase(Locale.US).startsWith("audio/");
                            if (id < 0 || (!napPlayerIsAudioName(name) && !audioMime)) continue;
                            if (!napPlayerIsAudioName(name)) name = "audio_file_" + id + ".mp3";
                            long size = sizeCol >= 0 ? fc.getLong(sizeCol) : 0L;
                            long modified = modCol >= 0 ? fc.getLong(modCol) : 0L;
                            Uri uri = ContentUris.withAppendedId(filesUri, id);
                            napPlayerAddAudioItem(items, seen, name, uri.toString(), size, modified);
                        }
                    }
                } finally {
                    try { if (fc != null) fc.close(); } catch (Throwable ignored) {}
                }
            } catch (Throwable t) {
                appendNativeLog("BUILD2SA13C15 PLAYER_MEDIASTORE_FILES_SCAN_ERR " + safeMsg(t));
            }

            napPlayerScanAudioDir(Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS), 2, items, seen);
            napPlayerScanAudioDir(new File(Environment.getExternalStorageDirectory(), "Download"), 2, items, seen);
            napPlayerScanAudioDir(new File(Environment.getExternalStorageDirectory(), "Downloads"), 2, items, seen);
            napPlayerScanAudioDir(Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_MUSIC), 2, items, seen);
            napPlayerScanAudioDir(new File(Environment.getExternalStorageDirectory(), "Music"), 2, items, seen);
            napPlayerScanAudioDir(new File(Environment.getExternalStorageDirectory(), "Documents"), 2, items, seen);
            napPlayerScanAudioDir(new File(Environment.getExternalStorageDirectory(), "Podcasts"), 2, items, seen);
            napPlayerScanAudioDir(new File(Environment.getExternalStorageDirectory(), "Ringtones"), 2, items, seen);
            napPlayerScanAudioDir(new File(Environment.getExternalStorageDirectory(), "AtariHelp"), 3, items, seen);
            napPlayerScanAudioDir(getPublicAtariHelpDownloadsDir(), 2, items, seen);

            Collections.sort(items, new Comparator<NapPlayerAudioItem>() {
                @Override public int compare(NapPlayerAudioItem a, NapPlayerAudioItem b) {
                    long am = a == null ? 0 : a.modified;
                    long bm = b == null ? 0 : b.modified;
                    return am == bm ? 0 : (am < bm ? 1 : -1);
                }
            });

            int limit = Math.min(items.size(), 240);
            StringBuilder sb = new StringBuilder(4096);
            sb.append("{\"ok\":true,\"count\":").append(items.size()).append(",\"items\":[");
            for (int i = 0; i < limit; i++) {
                NapPlayerAudioItem it = items.get(i);
                if (i > 0) sb.append(',');
                sb.append("{\"name\":").append(jsonQuote(it.name))
                        .append(",\"uri\":").append(jsonQuote(it.uri))
                        .append(",\"size\":").append(it.size)
                        .append(",\"modified\":").append(it.modified)
                        .append('}');
            }
            sb.append("]}");
            appendNativeLog("BUILD2SA13C8 PLAYER_LOCAL_AUDIO_LIST count=" + items.size() + " returned=" + limit);
            return sb.toString();
        } catch (Throwable t) {
            appendNativeLog("BUILD2SA13C8 PLAYER_LOCAL_AUDIO_LIST_FAIL " + safeMsg(t));
            return "{\"ok\":false,\"error\":" + jsonQuote(safeMsg(t)) + ",\"items\":[]}";
        }
    }

    public class AHPick {
        @JavascriptInterface
        public void pickGame() {
            ui.post(() -> openBridgePicker("game"));
        }
        @JavascriptInterface
        public void pickAudio() {
            ui.post(() -> openBridgePicker("audio"));
        }
        @JavascriptInterface
        public void pickMp3() {
            ui.post(() -> openBridgePicker("mp3"));
        }
        @JavascriptInterface
        public String listLocalAudio() {
            return napPlayerListLocalAudioJson();
        }
        @JavascriptInterface
        public String requestAudioPermission() {
            napPlayerRequestAudioPermission("js");
            return napPlayerHasAudioPermission() ? "AUDIO_PERMISSION_OK" : "AUDIO_PERMISSION_REQUESTED";
        }
        @JavascriptInterface
        public String playLocalAudio(String name, String uriText) {
            try {
                if (uriText == null || uriText.trim().length() == 0) return "LOCAL_AUDIO_EMPTY_URI";
                final String safeName = (name == null || name.length() == 0) ? "audio" : name;
                final String safeUri = uriText;
                new Thread(() -> {
                    try {
                        Uri uri = Uri.parse(safeUri);
                        byte[] bytes = readUriBytes(uri, 96 * 1024 * 1024);
                        ui.post(() -> injectAudio(safeName, bytes));
                        appendNativeLog("BUILD2SA13C14 PLAYER_LOCAL_AUDIO_LOAD_OK name=" + safeName + " bytes=" + bytes.length + " uri=" + compactUrl(safeUri));
                    } catch (Throwable t) {
                        String msg = "LOCAL AUDIO CHYBA - " + safeMsg(t);
                        appendNativeLog("BUILD2SA13C14 PLAYER_LOCAL_AUDIO_LOAD_FAIL " + msg + " uri=" + compactUrl(safeUri));
                        ui.post(() -> {
                            try { if (web != null) web.evaluateJavascript("AHJAVA_ERROR(" + jsQuote(msg) + ")", null); } catch (Throwable ignored) {}
                        });
                    }
                }, "nap-player-local-audio").start();
                return "LOCAL_AUDIO_LOADING";
            } catch (Throwable t) {
                return "LOCAL_AUDIO_FAIL " + safeMsg(t);
            }
        }
        @JavascriptInterface
        public void openExternalUrl(String url) {
            ui.post(() -> openRawExternalBrowserUrl(url));
        }
        @JavascriptInterface
        public void pickText() {
            ui.post(() -> openBridgePicker("text"));
        }
    }
    private String nowStamp() {
        try { return new SimpleDateFormat("yyyy-MM-dd HH:mm:ss.SSS", Locale.US).format(new Date()); }
        catch (Throwable t) { return String.valueOf(System.currentTimeMillis()); }
    }

    private final java.util.concurrent.atomic.AtomicInteger napTvWebLogFileQueueSize = new java.util.concurrent.atomic.AtomicInteger(0);
    // BUILD2SK95: obranny strop na frontu logu - Rene si vzpomel na starsi
    // problem u Segy, kdy se nekde hromadila pamet a postupne to zpomalilo
    // nejdriv zvuk, pak i obraz (presne popsany vzorec "zhorsuje se to v
    // case"). Sega mela uz drivejsi ochranu proti hromadeni STARYCH VLAKEN
    // (BUILD2RV, nativeAudioGeneration) - to jsem neopakoval spatne (PS1
    // audio uz ma stejnou ochranu, ps1AudioGen, BUILD2SA3B, nezavisle na
    // mne). Ale nasel jsem JINE, VLASTNI riziko: fronta na zapis logu
    // (napTvWebLogFileQueue, SK82) nemela ZADNY strop - kdyby zapisovaci
    // vlakno kdykoli zaostalo za tempem generovani (a me nove
    // PS1_NATIVE_TEXTURE_SLOW logy - viz nize - generovaly AZ 900+ radku
    // za par minut, bez omezeni), fronta mohla teoreticky rust bez konce.
    // Ted: pevny strop (4000 radku, ~pár desitek KB textu) - pri prekroceni
    // se novy radek zahodi (a zapocita), misto aby fronta rostla dal.
    private static final int napTvWebLogFileQueueMax = 4000;
    private final java.util.concurrent.atomic.AtomicLong napTvWebLogFileDropped = new java.util.concurrent.atomic.AtomicLong(0);

    private void appendNativeLog(String line) {
        String stamped = nowStamp() + "  " + (line == null ? "" : line) + "\n";
        synchronized (nativeLog) {
            nativeLog.append(stamped);
            // BUILD2RW: bigger ring (~100 KB) so the 10s PASSIVE_AUDIT_RW rows survive a long S8
            // degradation test. Memory cost is trivial; the point is to SEE what grows over time.
            if (nativeLog.length() > 100000) nativeLog.delete(0, nativeLog.length() - 100000);
        }
        // BUILD2SK82: jen zarad do fronty (O(1), zadne disk I/O na TOMHLE vlakne -
        // muze to byt UI tick, audio vlakno, HTTP handler...) - samotny zapis dela
        // napTvWebLogFileWriterLoop na svem vlastnim vlakne (viz napTvWebLogFileInit).
        // BUILD2SK95: jen pokud jsme pod stropem - viz komentar u konstanty vyse.
        try {
            if (napTvWebLogFileQueueSize.get() < napTvWebLogFileQueueMax) {
                napTvWebLogFileQueue.offer(stamped);
                napTvWebLogFileQueueSize.incrementAndGet();
            } else {
                napTvWebLogFileDropped.incrementAndGet();
            }
        } catch (Throwable ignored) {}
    }

    // BUILD2SK82: vola se jednou z onCreate. Vytvori/zkrati cerstvy log soubor pro
    // TUTO relaci appky (predchozi spusteni se nemicha dohromady se soucasnym) a
    // nastartuje vlakno, ktere frontu prubezne vyprazdnuje na disk.
    private void napTvWebLogFileInit() {
        try {
            // ===== LOG MUSI PREZIT PAD APLIKACE =====
            // Log se posila z BEZICI aplikace pres /log. Kdyz spadne, neni
            // odkud - proto se pise i do Downloads, kam se uzivatel dostane
            // z telefonu bez pocitace, a predchozi relace se NEMAZE, ale
            // preulozi jako "...predchozi.txt". Po padu je tedy k dispozici.
            try {
                File dl = new File(android.os.Environment.getExternalStoragePublicDirectory(
                        android.os.Environment.DIRECTORY_DOWNLOADS), "AtariHelp");
                if (!dl.exists()) dl.mkdirs();

                // BUILD2SA15: Downloads NEMUSI byt zapisovatelne. Na S8 v B128
                // vratilo canWrite() = false, soubor mel 0 B a cesta pro pad
                // nefungovala - pritom se do nej dal psalo, jako by slo o nic.
                // Slozka appky na externim ulozisti je zapisovatelna VZDY a
                // bez jakehokoli opravneni. Bereme ji, kdyz Downloads nejdou.
                boolean dlOk = dl.canWrite();
                if (!dlOk) {
                    try {
                        File alt = getExternalFilesDir(null);
                        if (alt == null) alt = getFilesDir();
                        if (!alt.exists()) alt.mkdirs();
                        dl = alt;
                        dlOk = dl.canWrite();
                    } catch (Throwable ignored) {}
                }

                File akt = new File(dl, "PS1_LOG.txt");
                if (akt.exists()) {
                    File pred = new File(dl, "PS1_LOG_predchozi.txt");
                    if (pred.exists()) pred.delete();
                    akt.renameTo(pred);      // po padu zustane tady
                }
                napTvWebLogFileDownloads = dlOk ? akt : null;

                // Overit zapis SKUTECNYM zapisem, ne jen dotazem na opravneni.
                boolean zapisFunguje = false;
                if (dlOk) {
                    try (FileOutputStream t0 = new FileOutputStream(akt, true)) {
                        t0.write("".getBytes("UTF-8"));
                        zapisFunguje = true;
                    } catch (Throwable ignored) {}
                }
                napTvWebLogSlozka = dl.getAbsolutePath();
                appendNativeLog("BUILD2SA15 LOG_SOUBOR aktualni=" + akt.getAbsolutePath()
                        + " predchozi=" + new File(dl, "PS1_LOG_predchozi.txt").getAbsolutePath()
                        + " zapis_funguje=" + zapisFunguje
                        + (zapisFunguje ? "" : "  <<< LOG SE NEUKLADA, PAD HO NEPREZIJE"));
                if (!zapisFunguje) napTvWebLogFileDownloads = null;
            } catch (Throwable t) {
                napTvWebLogFileDownloads = null;
                appendNativeLog("BUILD2SA15 LOG_SOUBOR SELHAL duvod=" + t.getMessage()
                        + "  <<< LOG SE NEUKLADA, PAD HO NEPREZIJE");
            }

            napTvWebLogFile = new File(getFilesDir(), "nap_tv_session_log.txt");
            try (FileOutputStream fos = new FileOutputStream(napTvWebLogFile, false)) { /* jen vytvor/zkrat na prazdno */ }
        } catch (Throwable t) {
            napTvWebLogFile = null; // /log endpoint pak spadne zpet na in-memory ring
        }
        if (!napTvWebLogFileWriterStarted) {
            napTvWebLogFileWriterStarted = true;
            Thread writer = new Thread(this::napTvWebLogFileWriterLoop, "napTvWebLogFileWriter");
            writer.setDaemon(true);
            writer.start();
        }
    }

    private void napTvWebLogFileWriterLoop() {
        while (true) {
            try {
                String line = napTvWebLogFileQueue.poll();
                if (line == null) {
                    Thread.sleep(50);
                    continue;
                }
                napTvWebLogFileQueueSize.decrementAndGet();
                File f = napTvWebLogFile;
                if (f == null) continue; // init selhala, ring buffer je pojistka
                if (f.length() >= napTvWebLogFileCapBytes) {
                    if (!napTvWebLogFileCapWarned) {
                        napTvWebLogFileCapWarned = true;
                        try (FileOutputStream fos = new FileOutputStream(f, true)) {
                            fos.write(("\n*** BUILD2SK82 LOG_FILE_CAP_REACHED " + napTvWebLogFileCapBytes
                                    + "B - dalsi radky uz se na disk nezapisuji, soubor zustava platny do tohoto bodu ***\n")
                                    .getBytes("UTF-8"));
                        } catch (Throwable ignored) {}
                    }
                    continue; // strop dosazen - dal uz nerosteme, ale nic neorezavame
                }
                // BUILD2SK82: drobne davkovani - pokud fronta mezitim narostla (napr.
                // appka byla chvili na pozadi), vyprazdni vice radku najednou pres
                // jeden otevreny stream, misto open/close pro kazdy jednotlivy radek.
                // BUILD2SK95: strop zvysen z 200 na 500 - rychlejsi zotaveni fronty,
                // kdyby se kdy zpozdila (viz napTvWebLogFileQueueMax vyse).
                StringBuilder batch = new StringBuilder(line);
                String more;
                int batched = 1;
                while (batched < 500 && (more = napTvWebLogFileQueue.poll()) != null) {
                    napTvWebLogFileQueueSize.decrementAndGet();
                    batch.append(more);
                    batched++;
                }
                // ===== PRUBEZNY ZAPIS DO DOWNLOADS VYPNUT =====
                // Zapisoval kazdou davku na sdilene uloziste a brzdil system.
                // Do Downloads se zapisuje UZ JEN PRI PADU (napZapisPadPrimo)
                // - to je jedine misto, kde je to opravdu potreba, protoze
                // /log se posila z bezici aplikace a po padu neni odkud.
                try (FileOutputStream fos = new FileOutputStream(f, true)) {
                    fos.write(batch.toString().getBytes("UTF-8"));
                }
                // BUILD2SK95: viditelnost - pokud jsme nekdy neco zahodili (fronta plna),
                // at je to videt v logu misto tiche ztraty dat, zhruba kazdych 500 zahozenych.
                long dropped = napTvWebLogFileDropped.get();
                if (dropped > 0 && dropped % 500 < batched) {
                    try (FileOutputStream fos2 = new FileOutputStream(f, true)) {
                        fos2.write(("BUILD2SK95 TV_WEB_LOG_QUEUE_DROPPED total=" + dropped + "\n").getBytes("UTF-8"));
                    } catch (Throwable ignored) {}
                }
            } catch (Throwable ignored) {
                try { Thread.sleep(200); } catch (InterruptedException ie) { /* ignore */ }
            }
        }
    }

    // BUILD2SK82: pouzito jen pro /log HTTP odpoved - precte cely soubor jako byty
    // (manualni smycka misto java.nio.file.Files, ktere potrebuje API 26+; appka
    // ma minSdk 24). 25MB citaci strop je jen pojistka pro tenhle jeden HTTP
    // prenos, nezavisly na napTvWebLogFileCapBytes (zapisovy strop souboru).
    private byte[] napTvWebReadFileBytes(File f) throws IOException {
        long len = f.length();
        if (len <= 0) return new byte[0];
        int cap = (int) Math.min(len, 25_000_000L);
        byte[] buf = new byte[cap];
        try (java.io.FileInputStream fis = new java.io.FileInputStream(f)) {
            int off = 0, r;
            while (off < cap && (r = fis.read(buf, off, cap - off)) > 0) off += r;
            if (off == cap) return buf;
            byte[] trimmed = new byte[off];
            System.arraycopy(buf, 0, trimmed, 0, off);
            return trimmed;
        }
    }

    private int mapNativeKey(String btn) {
        if (btn == null) return -1;
        String b = btn.toUpperCase(Locale.US);
        if ("UP".equals(b)) return 0;
        if ("DOWN".equals(b)) return 1;
        if ("LEFT".equals(b)) return 2;
        if ("RIGHT".equals(b)) return 3;
        if ("A".equals(b)) return 4;
        if ("B".equals(b)) return 5;
        if ("C".equals(b)) return 6;
        if ("START".equals(b)) return 7;
        return -1;
    }

    private String buildNativeInPlaceLog() {
        StringBuilder out = new StringBuilder();
        out.append("SEGA C++ IN-PLACE LOG / BUILD2RY\n");
        out.append("AtariHelp.eu EMU-10 BUILD2RY_SEGA_NATIVE_CPP_ONLY_CORE_O3_VDP_LEFT_BLANK_STAGE141\n\n");
        out.append("DEVICE sdk=").append(Build.VERSION.SDK_INT)
           .append(" release=").append(Build.VERSION.RELEASE)
           .append(" brand=").append(Build.BRAND)
           .append(" model=").append(Build.MODEL)
           .append(" cores=").append(Runtime.getRuntime().availableProcessors()).append("\n");
        out.append("nativeInPlaceEnabled=").append(nativeInPlaceEnabled).append("\n");
        out.append("nativeLastStatus=").append(nativeLastStatus).append("\n");
        out.append("nativeInputEvents=").append(nativeInputEvents).append("\n");
        out.append("nativeCoreAudioRun=").append(nativeCoreAudioRun).append("\n");
        out.append("nativeAudioGeneration=").append(nativeAudioGeneration).append("\n");
        out.append("nativePerformanceMode=").append(nativePerformanceMode).append("\n");
        out.append("nativeActiveAudioTracks=").append(nativeActiveAudioTracks).append("\n");
        try { out.append("realCoreStatus=").append(NativeSegaCoreBridge.realCoreStatus().replace('\n',' ')).append("\n"); } catch (Throwable t) { out.append("realCoreStatus=ERROR ").append(safeMsg(t)).append("\n"); }
        out.append("\nROM BLOCK:\n").append(nativeLastRomInfo == null ? "" : nativeLastRomInfo).append("\n\n");
        out.append("EVENTS:\n");
        synchronized (nativeLog) { out.append(nativeLog.toString()); }
        out.append("\nDULEZITE:\n- Tohle porad neni hotovy Sega gameplay.\n");
        out.append("- Toto overuje normalni Sega UI -> Java -> JNI -> C++ -> ROM/input/audio/render/log.\n");
        out.append("- Sega emulace je v BUILD2RV C++ only; Java/WebView wrapper se nespousti; C++ CORE UI tlacitko je odstranene; FM/PSG zeroed RR recovery safe-audit bez hard-drop orezavani; SBIRKA prepina LOW/HIGH vykon; SBIRKA prepina LOW/HIGH vykon; mobile visibility/orientation nesmi vypnout native video; C++ region respektuje ROM header; nulove recty se ignoruji.\n");
        return out.toString();
    }

    private synchronized void startNativeCoreAudioStream() {
        if (nativeCoreAudioRun || (nativeCoreAudioThread != null && nativeCoreAudioThread.isAlive()) || nativeCurrentAudioTrack != null) {
            appendNativeLog("NATIVE_AUDIO_STREAM_PRESTART_HARD_STOP_RV_RR_RECOVERY_SAFE_AUDIT_QT oldGen=" + nativeAudioGeneration);
            stopNativeCoreAudioStream();
        }
        final int audioGen = ++nativeAudioGeneration; // BUILD2RV: one valid AudioTrack writer per ROM session.
        nativeCoreAudioRun = true;
        nativeCoreAudioThread = new Thread(() -> {
            try { android.os.Process.setThreadPriority(android.os.Process.THREAD_PRIORITY_URGENT_AUDIO); } catch (Throwable ignored) {}
            final int sampleRate = 48000;
            final boolean s8NoStarve = (Build.VERSION.SDK_INT <= 28) || ((Build.MODEL == null ? "" : Build.MODEL).toUpperCase(Locale.US).contains("SM-G950"));
            final int chunk = 384; // BUILD2RV: stable stereo chunk; RP 192-frame low-latency cracked; keep 384.
            AudioTrack track = null;
            try {
                int min = AudioTrack.getMinBufferSize(sampleRate, AudioFormat.CHANNEL_OUT_STEREO, AudioFormat.ENCODING_PCM_16BIT);
                // BUILD2RV: S8 RR recovery profile gets a real reservoir; Nox keeps the clean RQ-sized path.
                int wantedFrames = audioFramesForTier();
                int wantedBytes = wantedFrames * 2 * 2;
                // POZOR: min*2 vypada jako zbytecna rezerva, ale zkusil jsem
                // to v B119 snizit na min a zvuk Segy prestal chodit na TV.
                // Nechat tak, jak to je.
                int bufferBytes = Math.max(min > 0 ? min * 2 : 0, wantedBytes);
                AudioTrack.Builder builder = null;
                if (Build.VERSION.SDK_INT >= 21) {
                    builder = new AudioTrack.Builder()
                            .setAudioAttributes(new AudioAttributes.Builder().setUsage(AudioAttributes.USAGE_GAME).setContentType(AudioAttributes.CONTENT_TYPE_SONIFICATION).build())
                            .setAudioFormat(new AudioFormat.Builder().setEncoding(AudioFormat.ENCODING_PCM_16BIT).setSampleRate(sampleRate).setChannelMask(AudioFormat.CHANNEL_OUT_STEREO).build())
                            .setBufferSizeInBytes(bufferBytes)
                            .setTransferMode(AudioTrack.MODE_STREAM);
                    if (Build.VERSION.SDK_INT >= 26) {
                        /* BUILD2RV: do not force Android low-latency profile; on S8/Nox it caused crackle when the core needed a little reservoir. */
                    }
                    track = builder.build();
                } else {
                    track = new AudioTrack(android.media.AudioManager.STREAM_MUSIC, sampleRate, AudioFormat.CHANNEL_OUT_STEREO, AudioFormat.ENCODING_PCM_16BIT, bufferBytes, AudioTrack.MODE_STREAM);
                }
                int setFrames = -1;
                if (Build.VERSION.SDK_INT >= 23) {
                    try { setFrames = track.setBufferSizeInFrames(s8NoStarve ? 8192 : Math.max(3072, chunk * 8)); } catch (Throwable ignored) {}
                }
                int actualFrames = -1;
                if (Build.VERSION.SDK_INT >= 23) { try { actualFrames = track.getBufferSizeInFrames(); } catch (Throwable ignored) {} }
                short[] pcm = new short[chunk * 2];
                nativeCurrentAudioTrack = track;
                nativeActiveAudioTracks = 1;
                appendNativeLog("NATIVE_AUDIO_STREAM_START_RV_RR_RECOVERY_SAFE_AUDIT_QT gen=" + audioGen + " sampleRate=" + sampleRate + " chunk=" + chunk + " minBytes=" + min + " bufferBytes=" + bufferBytes + " setFrames=" + setFrames + " mode=" + nativePerformanceMode + " forcedLowLatency=false actualFrames=" + actualFrames + " s8NoStarve=" + s8NoStarve + " singleAudioPath=YES activeAudioTracks=" + nativeActiveAudioTracks + " bufferMsApprox=" + ((actualFrames > 0 ? actualFrames : (bufferBytes / 4)) * 1000 / sampleRate));

                int prefilled = 0;
                int preLoops = 0;
                long preDeadline = System.currentTimeMillis() + (s8NoStarve ? 900 : 260);
                int prefillTarget = s8NoStarve ? 6144 : 3072;
                while (nativeCoreAudioRun && audioGen == nativeAudioGeneration && prefilled < prefillTarget && System.currentTimeMillis() < preDeadline) {
                    int got = 0;
                    try { got = NativeSegaCoreBridge.pullAudioStereo(pcm, chunk); }
                    catch (Throwable pullErr) { appendNativeLog("NATIVE_AUDIO_PREFILL_PULL_ERROR_RV " + safeMsg(pullErr)); got = 0; }
                    if (got > 0) {
                        int wr = track.write(pcm, 0, pcm.length);
                        if (wr > 0) prefilled += wr;
                    } else {
                        try { Thread.sleep(4); } catch (Throwable ignored) {}
                    }
                    preLoops++;
                }

                if (!nativeCoreAudioRun || audioGen != nativeAudioGeneration) {
                    appendNativeLog("NATIVE_AUDIO_START_CANCELLED_RV staleGen=" + audioGen + " current=" + nativeAudioGeneration);
                    return;
                }
                track.play();
                appendNativeLog("NATIVE_AUDIO_PREFILL_RV_RR_RECOVERY_SAFE_AUDIT_QT gen=" + audioGen + " frames=" + prefilled + " loops=" + preLoops + " playState=" + track.getPlayState());
                int loops = 0;
                int underrunLoops = 0;
                while (nativeCoreAudioRun && audioGen == nativeAudioGeneration) {
                    int got = 0;
                    try { got = NativeSegaCoreBridge.pullAudioStereo(pcm, chunk); }
                    catch (Throwable pullErr) { appendNativeLog("NATIVE_AUDIO_PULL_ERROR_RV " + safeMsg(pullErr)); got = 0; }
                    if (got <= chunk / 4) {
                        underrunLoops++;
                        // BUILD2RV: S8 no-starve waits briefly for native core instead of writing immediate silence crackles.
                        int retries = s8NoStarve ? 5 : 1;
                        for (int rr = 0; rr < retries && got <= chunk / 2 && nativeCoreAudioRun && audioGen == nativeAudioGeneration; rr++) {
                            try { Thread.sleep(s8NoStarve ? 3 : 2); } catch (Throwable ignored) {}
                            try { int got2 = NativeSegaCoreBridge.pullAudioStereo(pcm, chunk); if (got2 > got) got = got2; } catch (Throwable ignored) {}
                        }
                    }
                    if (loops < 16 || loops % 180 == 0) {
                        // BUILD2RW passive: also report the real Android AudioTrack underrun counter (API >= 24).
                        int trackUnderruns = -1;
                        try { if (Build.VERSION.SDK_INT >= 24) trackUnderruns = track.getUnderrunCount(); } catch (Throwable ignored) {}
                        appendNativeLog("NATIVE_AUDIO_PULL_RW_PASSIVE_AUDIT gen=" + audioGen + " got=" + got + " loop=" + loops + " underrunLoops=" + underrunLoops + " audioTrackUnderruns=" + trackUnderruns + " mode=" + nativePerformanceMode);
                    }
                    // BUILD2RV: feed AudioTrack one small full clock chunk. Native produces real samples or true silence only; RR uses balanced FIFO governor, not RP tiny buffer drops or RO huge delay.
                    int framesToWrite = pcm.length;
                    // ===== ZVUK NA TV MUSI JIT DRIV, NEZ SE PREHRAJE =====
                    // track.write(...WRITE_BLOCKING) CEKA, az se zvuk v telefonu
                    // prehraje - trva to tak dlouho, jak je davka dlouha. Kdyz
                    // se na TV posilalo az POTOM, prisel zvuk pozde a televize
                    // ho zahodila. V telefonu pritom hral normalne, takze to
                    // vypadalo, ze zvuk funguje - jen na webu ne.
                    // Davku posilame na TV HNED, jeste pred prehranim.
                    if (framesToWrite > 0) napTvWebAudioPush(pcm, 0, framesToWrite, sampleRate, "SEGA");

                    int off = 0;
                    while (off < framesToWrite && nativeCoreAudioRun && audioGen == nativeAudioGeneration) {
                        int wr;
                        if (Build.VERSION.SDK_INT >= 23) wr = track.write(pcm, off, framesToWrite - off, AudioTrack.WRITE_BLOCKING);
                        else wr = track.write(pcm, off, framesToWrite - off);
                        if (wr <= 0) break;
                        off += wr;
                    }
                    loops++;
                }
            } catch (Throwable t) {
                appendNativeLog("NATIVE_AUDIO_STREAM_ERROR_RV gen=" + audioGen + " " + safeMsg(t));
            } finally {
                try { if (track != null) { track.pause(); track.flush(); track.stop(); track.release(); } } catch (Throwable ignored) {}
                if (nativeCurrentAudioTrack == track) nativeCurrentAudioTrack = null;
                if (nativeActiveAudioTracks > 0) nativeActiveAudioTracks = 0;
                appendNativeLog("NATIVE_AUDIO_STREAM_STOP_RV_RR_RECOVERY_SAFE_AUDIT_QT gen=" + audioGen + " current=" + nativeAudioGeneration + " activeAudioTracks=" + nativeActiveAudioTracks + " singleAudioPath=YES");
            }
        }, "AtariHelpSegaLowLatencyAudioRR_" + audioGen);
        nativeCoreAudioThread.setDaemon(true);
        try { nativeCoreAudioThread.setPriority(Thread.MAX_PRIORITY); } catch (Throwable ignored) {}
        nativeCoreAudioThread.start();
    }

    private synchronized void stopNativeCoreAudioStream() {
        final int stopGen = ++nativeAudioGeneration; // BUILD2RV: invalidate stale AudioTrack writers before starting another ROM.
        nativeCoreAudioRun = false;
        AudioTrack at = nativeCurrentAudioTrack;
        if (at != null) {
            try { at.pause(); } catch (Throwable ignored) {}
            try { at.flush(); } catch (Throwable ignored) {}
            try { at.stop(); } catch (Throwable ignored) {}
            // BUILD2RV: on S8 old AudioTrack.write() could stay blocked after ROM change.
            // Release immediately; the audio thread finally-block tolerates the already released track.
            try { at.release(); } catch (Throwable ignored) {}
            nativeCurrentAudioTrack = null;
        }
        Thread t = nativeCoreAudioThread;
        if (t != null && t.isAlive() && Thread.currentThread() != t) {
            try { t.interrupt(); } catch (Throwable ignored) {}
            try { t.join(1100); } catch (Throwable ignored) {}
            if (t.isAlive()) appendNativeLog("NATIVE_AUDIO_THREAD_STILL_ALIVE_RV invalidatedGen=" + stopGen + " releasedTrack=YES will exit on generation guard");
        }
        if (nativeCoreAudioThread == t) nativeCoreAudioThread = null;
        nativeActiveAudioTracks = 0;
        appendNativeLog("NATIVE_AUDIO_STREAM_STOP_REQUEST_RV_RR_RECOVERY_SAFE_AUDIT_QT gen=" + stopGen + " hardReleaseTrack=" + (at != null) + " activeAudioTracks=" + nativeActiveAudioTracks + " oldAudioKilled=YES javaAudioReleased=" + (at != null) + " singleAudioPath=YES");
    }

    private boolean isSegaNativeOwnerUrl(String url) {
        if (url == null) return false;
        String u = url.toLowerCase(Locale.US);
        // Native C++ view smi zit jen nad normalni Sega strankou. Jakmile WebView jde na Intro/VBXE/Atari/PS1/Web,
        // musi se view i worker vypnout, jinak presne vznikne chaos "Sonic okno vsude".
        return u.startsWith("file:///android_asset/emu_sega/index.html") || u.startsWith("file:///android_asset/emu_sega/");
    }

    private String compactUrl(String url) {
        if (url == null) return "null";
        String u = url.replace("file:///android_asset/", "asset:/");
        return u.length() > 96 ? u.substring(0, 96) : u;
    }

    private static class FetchResult {
        byte[] data;
        String contentType;
        String contentDisposition;
        String via;
        int relayMode = -1;
    }

    private static class SegaExtract {
        byte[] data;
        String name;
    }

    private static class AtariExtract {
        byte[] data;
        String name;
    }

    private boolean isAtariHelpUrl(String url) {
        if (url == null) return false;
        String u = url.trim().toLowerCase(Locale.US);
        if (!(u.startsWith("http://") || u.startsWith("https://"))) return false;
        try {
            Uri uri = Uri.parse(url);
            String host = uri.getHost();
            if (host == null) return false;
            host = host.toLowerCase(Locale.US);
            return host.equals("atarihelp.eu") || host.equals("www.atarihelp.eu") || host.endsWith(".atarihelp.eu");
        } catch (Throwable ignored) {
            return false;
        }
    }

    private boolean isProviderBlockedUrl(String url) {
        // BUILD2SA5AF: web ochrana zpet. AtariHelp/WEDOS jdou pres chrany WebView relay/intercept.
        if (url == null) return false;
        String u = url.trim().toLowerCase(Locale.US);
        if (!(u.startsWith("http://") || u.startsWith("https://"))) return false;
        try {
            Uri uri = Uri.parse(url);
            String host = uri.getHost();
            if (host == null) return false;
            host = host.toLowerCase(Locale.US);
            return host.equals("atarihelp.eu") || host.equals("www.atarihelp.eu") || host.endsWith(".atarihelp.eu")
                    || host.equals("vedos.cz") || host.equals("www.vedos.cz") || host.endsWith(".vedos.cz");
        } catch (Throwable ignored) {
            return false;
        }
    }

    private void applyWebViewVisualMode(String url, String reason) {
        if (web == null) return;
        boolean normalWeb = isProviderBlockedUrl(url);
        String lowUrl = url == null ? "" : url.toLowerCase(Locale.US);
        boolean playerWeb = lowUrl.startsWith("file:///android_asset/player/");
        try { web.setBackgroundColor(normalWeb ? Color.WHITE : Color.TRANSPARENT); } catch (Throwable ignored) {}
        try { if (rootFrame != null) rootFrame.setBackgroundColor(normalWeb ? Color.WHITE : Color.BLACK); } catch (Throwable ignored) {}
        try { web.setLayerType((normalWeb || playerWeb) ? View.LAYER_TYPE_NONE : View.LAYER_TYPE_HARDWARE, null); } catch (Throwable ignored) {}
        if (normalWeb) appendNativeLog("BUILD2SA5AF WEBVIEW_PROTECTED_WEB reason=" + reason + " url=" + compactUrl(url));
    }

    private synchronized boolean markAtariHelpRequestAllowed(String url, String reason) {
        if (isProviderBlockedUrl(url)) {
            appendNativeLog("BUILD2SA5AF PROVIDER_RELAY_REQUEST reason=" + reason + " url=" + compactUrl(url));
            return true;
        }
        if (!isAtariHelpUrl(url)) return true;
        long now = System.currentTimeMillis();
        if (atariHelpBlockedUntilMs > now) {
            showAtariHelpSafetyStop(url, reason, atariHelpBlockedUntilMs - now);
            appendNativeLog("BUILD2SA5M ATARIHELP_REQUEST_BLOCKED_COOLDOWN reason=" + reason + " waitMs=" + (atariHelpBlockedUntilMs - now) + " url=" + compactUrl(url));
            return false;
        }
        boolean userDownload = reason != null && reason.startsWith("download");
        long elapsed = now - atariHelpLastRequestAtMs;
        if (!userDownload && atariHelpLastRequestAtMs > 0 && elapsed < ATARIHELP_MIN_REQUEST_GAP_MS) {
            long waitMs = ATARIHELP_MIN_REQUEST_GAP_MS - elapsed;
            showAtariHelpSafetyStop(url, reason, waitMs);
            appendNativeLog("BUILD2SA5M ATARIHELP_REQUEST_BLOCKED_RATE reason=" + reason + " waitMs=" + waitMs + " url=" + compactUrl(url));
            return false;
        }
        atariHelpLastRequestAtMs = now;
        appendNativeLog("BUILD2SA5M ATARIHELP_REQUEST_ALLOWED reason=" + reason + " url=" + compactUrl(url));
        return true;
    }

    private void loadAtariHelpGuarded(String url, String reason) {
        if (web == null || url == null) return;
        if (isProviderBlockedUrl(url)) {
            applyWebViewVisualMode(url, "relayLoad");
            appendNativeLog("BUILD2SA5AA ATARIHELP_RELAY_LOAD reason=" + reason + " url=" + compactUrl(url));
            web.loadUrl(url);
            return;
        }
        if (!markAtariHelpRequestAllowed(url, reason)) return;
        applyWebViewVisualMode(url, reason);
        web.loadUrl(url);
    }

    private void showAtariHelpSafetyStop(final String url, final String reason, final long waitMs) {
        Runnable r = () -> {
            if (web == null) return;
            applyWebViewVisualMode(url, "safetyStop");
            long sec = Math.max(1L, (waitMs + 999L) / 1000L);
            String html = "<!doctype html><html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'>"
                    + "<style>body{background:#fff;color:#111;font-family:sans-serif;padding:22px;line-height:1.45}h1{font-size:22px}.btn{display:block;margin:10px 0;padding:12px 14px;border:1px solid #111;border-radius:6px;background:#111;color:#fff;text-decoration:none;text-align:center;font-weight:700}.alt{background:#fff;color:#111}.warn{padding:10px;border-left:4px solid #b80;background:#fff7e6}code{word-break:break-all}</style></head>"
                    + "<body><h1>AtariHelp ochranna brzda</h1>"
                    + "<div class='warn'>Aplikace ted neposila dalsi pozadavky na web, aby nezhorsila IP ban / rate limit.</div>"
                    + "<p><b>URL:</b> <code>" + escapeHtml(url) + "</code></p>"
                    + "<p><b>Duvod:</b> <code>" + escapeHtml(reason) + "</code></p>"
                    + "<p>Zkusit znovu nejdrive za " + sec + " s.</p>"
                    + "<a class='btn alt' href='#' onclick='try{AHPICK.pickGame();}catch(e){}return false;'>Vybrat ZIP z telefonu</a>"
                    + "</body></html>";
            try { web.loadDataWithBaseURL("about:blank", html, "text/html", "UTF-8", null); } catch (Throwable ignored) {}
        };
        if (isUiThread()) r.run(); else ui.post(r);
    }

    private void showAtariHelpLoadError(String url, String detail) {
        if (!isProviderBlockedUrl(url) || web == null) return;
        showAtariHelpProviderBridge(url, "loadError", detail);
    }

    private void configureGameHttpConnection(HttpURLConnection c, String url) {
        if (c == null) return;
        try { c.setConnectTimeout(20000); } catch (Throwable ignored) {}
        try { c.setReadTimeout(45000); } catch (Throwable ignored) {}
        if (isAtariHelpUrl(url)) {
            try { c.setRequestProperty("User-Agent", ATARIHELP_BROWSER_UA); } catch (Throwable ignored) {}
            try { c.setRequestProperty("Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,application/zip,application/octet-stream,*/*;q=0.8"); } catch (Throwable ignored) {}
            try { c.setRequestProperty("Accept-Language", "cs-CZ,cs;q=0.9,en;q=0.8"); } catch (Throwable ignored) {}
            try { c.setRequestProperty("Referer", "https://atarihelp.eu/?page_id=207"); } catch (Throwable ignored) {}
        }
    }

    private String providerRelayUrl(String url, int mode) throws IOException {
        String enc = java.net.URLEncoder.encode(url, "UTF-8");
        if (mode == 0) return "https://proxy.cors.sh/" + url;
        if (mode == 1) return "https://api.allorigins.win/raw?url=" + enc;
        return "https://corsproxy.io/?url=" + enc;
    }

    private FetchResult fetchViaProviderRelay(String url, int maxBytes, String reason) throws IOException {
        IOException last = null;
        for (int i = 0; i < 3; i++) {
            String relay = providerRelayUrl(url, i);
            HttpURLConnection c = null;
            try {
                c = (HttpURLConnection) new URL(relay).openConnection();
                c.setInstanceFollowRedirects(true);
                c.setConnectTimeout(18000);
                c.setReadTimeout(50000);
                try { c.setRequestProperty("User-Agent", ATARIHELP_BROWSER_UA); } catch (Throwable ignored) {}
                try { c.setRequestProperty("Accept", "*/*"); } catch (Throwable ignored) {}
                if (i == 0) {
                    try { c.setRequestProperty("Origin", "https://atarihelp.eu"); } catch (Throwable ignored) {}
                    try { c.setRequestProperty("Referer", "https://atarihelp.eu/"); } catch (Throwable ignored) {}
                }
                int code = c.getResponseCode();
                if (code < 200 || code >= 400) throw new IOException("relay HTTP " + code + " via=" + relay);
                FetchResult out = new FetchResult();
                out.data = readStreamLimited(c.getInputStream(), maxBytes);
                out.contentType = c.getContentType();
                out.contentDisposition = c.getHeaderField("Content-Disposition");
                out.via = relay;
                out.relayMode = i;
                if (out.data == null || out.data.length == 0) throw new IOException("relay empty via=" + relay);
                appendNativeLog("BUILD2SA5AA PROVIDER_RELAY_OK reason=" + reason + " mode=" + i + " bytes=" + out.data.length + " via=" + compactUrl(relay) + " target=" + compactUrl(url));
                return out;
            } catch (IOException ex) {
                last = ex;
                appendNativeLog("BUILD2SA5AA PROVIDER_RELAY_FAIL reason=" + reason + " try=" + i + " err=" + safeMsg(ex) + " target=" + compactUrl(url));
            } finally {
                try { if (c != null) c.disconnect(); } catch (Throwable ignored) {}
            }
        }
        throw last == null ? new IOException("relay failed") : last;
    }

    private FetchResult fetchUrlBytes(String url, int maxBytes, String reason) throws IOException {
        if (isProviderBlockedUrl(url)) return fetchViaProviderRelay(url, maxBytes, reason);
        HttpURLConnection c = (HttpURLConnection) new URL(url).openConnection();
        c.setInstanceFollowRedirects(true);
        configureGameHttpConnection(c, url);
        c.connect();
        int code = c.getResponseCode();
        if (code < 200 || code >= 400) throw new IOException("HTTP " + code + " " + c.getResponseMessage());
        FetchResult out = new FetchResult();
        out.data = readStreamLimited(c.getInputStream(), maxBytes);
        out.contentType = c.getContentType();
        out.contentDisposition = c.getHeaderField("Content-Disposition");
        out.via = url;
        try { c.disconnect(); } catch (Throwable ignored) {}
        return out;
    }

    private byte[] readStreamLimited(InputStream in, int maxBytes) throws IOException {
        ByteArrayOutputStream bos = new ByteArrayOutputStream();
        byte[] buf = new byte[16384];
        int n;
        while ((n = in.read(buf)) > 0 && bos.size() < maxBytes) {
            int take = Math.min(n, maxBytes - bos.size());
            if (take > 0) bos.write(buf, 0, take);
            if (take < n) break;
        }
        try { in.close(); } catch (Throwable ignored) {}
        return bos.toByteArray();
    }

    private String responseMime(String url, String contentType, boolean mainFrame) {
        if (contentType != null) {
            int semi = contentType.indexOf(';');
            String mime = (semi >= 0 ? contentType.substring(0, semi) : contentType).trim();
            if (mime.length() > 0) return mime;
        }
        String guess = null;
        try { guess = java.net.URLConnection.guessContentTypeFromName(Uri.parse(url).getPath()); } catch (Throwable ignored) {}
        if (guess != null && guess.length() > 0) return guess;
        return mainFrame ? "text/html" : "application/octet-stream";
    }

    private WebResourceResponse interceptProviderBlockedResource(String url, boolean mainFrame) {
        if (!isProviderBlockedUrl(url)) return null;
        try {
            FetchResult r = fetchViaProviderRelay(url, mainFrame ? 4 * 1024 * 1024 : 12 * 1024 * 1024, mainFrame ? "webMain" : "webResource");
            String mime = responseMime(url, r.contentType, mainFrame);
            String enc = (mime.startsWith("text/") || mime.contains("javascript") || mime.contains("json") || mime.contains("xml")) ? "UTF-8" : null;
            return new WebResourceResponse(mime, enc, new java.io.ByteArrayInputStream(r.data));
        } catch (Throwable t) {
            appendNativeLog("BUILD2SA5AA PROVIDER_INTERCEPT_FAIL main=" + mainFrame + " err=" + safeMsg(t) + " url=" + compactUrl(url));
            return null;
        }
    }

    private WebResourceResponse htmlResponse(String html) {
        try {
            return new WebResourceResponse("text/html", "UTF-8", new java.io.ByteArrayInputStream(html.getBytes("UTF-8")));
        } catch (Throwable t) {
            return null;
        }
    }

    private WebResourceResponse interceptMainFrameGameNavigation(final String url) {
        if (url == null || (!isGameUrl(url, null, null) && !shouldRouteAsPs1Download(url, "mainFrameGameNav"))) return null;
        appendNativeLog("BUILD2SA5AQ MAINFRAME_GAME_NAV route url=" + compactUrl(url));
        ui.post(() -> {
            try {
                if (shouldRouteAsSegaDownload(url)) downloadAndRunSegaArchive(url);
                else if (shouldRouteAsPs1Download(url, "mainFrameGameNav")) downloadAndRunPs1Remote(url, "mainFrameGameNav");
                else if (hasSegaExtension(url)) downloadAndRunSega(url);
                else downloadAndRun(url);
            } catch (Throwable t) {
                appendNativeLog("BUILD2SA5AQ MAINFRAME_GAME_NAV_FAIL " + safeMsg(t));
            }
        });
        return htmlResponse("<!doctype html><html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'>"
                + "<style>body{font-family:sans-serif;background:#fff;color:#111;padding:24px;line-height:1.4}code{word-break:break-all}</style></head>"
                + "<body><h1>Spoustim hru...</h1><p><code>" + escapeHtml(url) + "</code></p></body></html>");
    }

    private void showAtariHelpProviderBridge(final String url, final String reason, final String detail) {
        Runnable r = () -> {
            if (web == null) return;
            applyWebViewVisualMode(url, "providerBridge");
            String html = "<!doctype html><html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'>"
                    + "<style>body{background:#fff;color:#111;font-family:sans-serif;padding:22px;line-height:1.45}h1{font-size:22px;margin:0 0 12px}.box{padding:12px;border-left:4px solid #111;background:#f3f3f3;margin:12px 0}.warn{border-left-color:#b80;background:#fff7e6}.btn{display:block;margin:10px 0;padding:12px 14px;border:1px solid #111;border-radius:6px;background:#111;color:#fff;text-decoration:none;text-align:center;font-weight:700}.alt{background:#fff;color:#111}input{box-sizing:border-box;width:100%;padding:12px;border:1px solid #777;border-radius:6px;font:16px sans-serif}code{word-break:break-all}</style></head>"
                    + "<body><h1>AtariHelp lokalni bridge</h1>"
                    + "<div class='box warn'>Provider blokuje nebo zavira spojeni na AtariHelp/WEDOS. Appka zkusi relay a necha aktivni herni bridge.</div>"
                    + "<p><b>URL:</b> <code>" + escapeHtml(url) + "</code></p>"
                    + "<p><b>Duvod:</b> <code>" + escapeHtml(reason) + " / " + escapeHtml(detail) + "</code></p>"
                    + "<a class='btn' href='#' onclick='try{AHNET.openGames();}catch(e){}return false;'>Zkusit znovu pres relay</a>"
                    + "<a class='btn alt' href='#' onclick='try{AHPICK.pickGame();}catch(e){}return false;'>Vybrat hru z telefonu</a>"
                    + "<div class='box'><b>Primy neblokovany odkaz</b><br><input id='u' placeholder='https://mirror.example/hra.zip'>"
                    + "<a class='btn' href='#' onclick='var u=document.getElementById(\"u\").value;if(u){try{AHNET.runGameUrl(u);}catch(e){location.href=u;}}return false;'>Spustit z odkazu</a></div>"
                    + "</body></html>";
            try { web.loadDataWithBaseURL("file:///android_asset/atarihelp_bridge.html", html, "text/html", "UTF-8", null); } catch (Throwable ignored) {}
            appendNativeLog("BUILD2SA5AA ATARIHELP_PROVIDER_BRIDGE reason=" + reason + " detail=" + detail + " url=" + compactUrl(url));
        };
        if (isUiThread()) r.run(); else ui.post(r);
    }

    private void showWebDownloadError(final String msg) {
        Runnable r = () -> {
            try {
                String current = web == null ? null : web.getUrl();
                appendNativeLog("BUILD2SA5AB WEB_DOWNLOAD_ERROR page=" + compactUrl(current) + " msg=" + msg);
                if (web != null && current != null && current.startsWith(EMU_URL)) {
                    web.postDelayed(() -> web.evaluateJavascript("AHJAVA_ERROR(" + jsQuote(msg) + ")", null), 300);
                }
            } catch (Throwable ignored) {}
        };
        if (isUiThread()) r.run(); else ui.post(r);
    }

    private String escapeHtml(String s) {
        if (s == null) return "";
        return s.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;").replace("\"", "&quot;").replace("'", "&#39;");
    }

    private void stopNativeIfLeavingSega(String url, String source) {
        if (!nativeInPlaceEnabled && !nativeCoreAudioRun && nativeInPlaceView == null) return;
        if (isSegaNativeOwnerUrl(url)) return;
        stopNativeInPlaceHard(source + ":" + compactUrl(url));
    }

    private boolean isPs1OwnerUrl(String url) {
        if (url == null) return false;
        String u = url.toLowerCase(Locale.US);
        return u.startsWith("file:///android_asset/emu_ps1/index.html") || u.startsWith("file:///android_asset/emu_ps1/");
    }

    private void stopPs1IfLeaving(String url, String source) {
        if (!ps1RemoteDownloadActive && !ps1BootActive && !ps1SessionActive && ps1CurrentAudioTrack == null && ps1AudioThread == null) return;
        if (isPs1OwnerUrl(url)) return;
        stopPs1SessionHard(source + ":" + compactUrl(url));
    }

    // BUILD2SK31: sdilena obranna JS-cistici funkce - volana z KAZDEHO mista, kde
    // se PS1 jadro zastavuje (hlavni odchod ze stranky, mazani cache behem hry,
    // "boot dokoncen az po odchodu" edge-case), aby se predesla stara PS1
    // predview obrazek nezustaval ukazovan na zadne dalsi obrazovce.
    private void ps1ClearJsPreview() {
        try {
            if (web != null) {
                web.evaluateJavascript(
                    "try{if(window.__napPs1FramePoll){clearInterval(window.__napPs1FramePoll);window.__napPs1FramePoll=null;}"
                    + "var im=document.getElementById('ps1Screen');if(im){im.src='';im.style.display='none';}}catch(e){}",
                    null);
            }
        } catch (Throwable ignored) {}
    }
    private synchronized String stopPs1SessionHard(String reason) {
        boolean hadSession = ps1BootActive || ps1SessionActive || ps1CurrentAudioTrack != null || ps1AudioThread != null;
        boolean hadRemoteDownload = ps1RemoteDownloadActive;
        if (!hadSession && !hadRemoteDownload) return "PS1_ALREADY_STOPPED";
        ps1LifecycleGen++;
        ps1RemoteDownloadActive = false;
        if (hadRemoteDownload) ps1RemoteDownloadStatus = "PS1_REMOTE_CANCELLED reason=" + reason;
        ps1BootActive = false;
        ps1SessionActive = false;
        stopPs1Audio();
        String r;
        try {
            r = NativePs1CoreBridge.stopSafe();
        } catch (Throwable t) {
            r = "PS1_STOP_EXCEPTION " + safeMsg(t);
        }
        closePs1GamePfdQuietly();
        ps1ClearJsPreview();
        ps1DeactivateNativeView();
        ps1LastBootResult = "PS1_STOPPED " + reason;
        appendNativeLog("BUILD2SA5I PS1_SESSION_STOP reason=" + reason + " core=" + (r == null ? "null" : r.replace('\n', ' ')));
        return r;
    }

    // aktivace/deaktivace obrazu PS1 (OpenGL cesta, ps1GlEnable/ps1GlDisable)
    // trida) - stejny idempotentni vzor jako Sega AHNative.enableInPlace /
    // removeNativeViewOnUi. Vzdy bezi na UI vlakne (rootFrame.addView vyzaduje
    // UI vlakno) - volajici muze byt na libovolnem vlakne (boot bezi na
    // pozadi), proto ui.post().

    // ================= VYKONNOSTNI TRIDA ZARIZENI =================
    //  0 = LOW    (stara/slabsi zarizeni, napr. S8 s Androidem 9)
    //  1 = MEDIUM (stredni rada)
    //  2 = HIGH   (moderni telefony, rok az dva stare)
    //
    //  Driv bylo v kodu natvrdo "kdyz je to S8, udelej tohle" - coz je
    //  spatne, aplikace ma bezet i na jinych telefonech. Ted se trida
    //  urci sama podle vykonu, a da se prepsat rucne z menu.
    // ==============================================================
    private int napPerfTier = -1;

    private int perfTier() {
        if (napPerfTier >= 0) return napPerfTier;
        try {
            android.content.SharedPreferences sp = getSharedPreferences("nap_perf", MODE_PRIVATE);
            int saved = sp.getInt("tier", -1);
            if (saved >= 0 && saved <= 2) { napPerfTier = saved; return napPerfTier; }
        } catch (Throwable ignored) {}
        int tier = 1;
        try {
            int cores = Runtime.getRuntime().availableProcessors();
            long ramMb = 0;
            try {
                android.app.ActivityManager am = (android.app.ActivityManager) getSystemService(ACTIVITY_SERVICE);
                android.app.ActivityManager.MemoryInfo mi = new android.app.ActivityManager.MemoryInfo();
                am.getMemoryInfo(mi);
                ramMb = mi.totalMem / (1024 * 1024);
            } catch (Throwable ignored) {}
            int sdk = Build.VERSION.SDK_INT;
            if (sdk <= 28 || cores <= 4 || (ramMb > 0 && ramMb < 3500)) tier = 0;
            else if (sdk >= 31 && cores >= 8 && ramMb >= 5500) tier = 2;
            else tier = 1;
            appendNativeLog("L vykonnostni trida: " + (tier == 0 ? "LOW" : tier == 1 ? "MEDIUM" : "HIGH")
                    + " (Android " + sdk + ", " + cores + " jader, " + ramMb + " MB RAM)");
        } catch (Throwable ignored) {}
        napPerfTier = tier;
        return tier;
    }

    /** Rucni prepnuti tridy z menu: 0=LOW, 1=MEDIUM, 2=HIGH. Projevi se po restartu hry. */
    public void setPerfTier(int t) {
        if (t < 0 || t > 2) return;
        napPerfTier = t;
        try { getSharedPreferences("nap_perf", MODE_PRIVATE).edit().putInt("tier", t).apply(); } catch (Throwable ignored) {}
        appendNativeLog("L vykonnostni trida rucne nastavena: " + (t == 0 ? "LOW" : t == 1 ? "MEDIUM" : "HIGH"));
    }

    /** Kolik vzorku zvuku drzet dopredu. Min = mensi zpozdeni, ale vetsi riziko praskani. */
    private int audioFramesForTier() {
        // ===== VELIKOST ZASOBNIKU ZVUKU - NESAHAT =====
        // V B119 jsem to zkusil snizit na polovinu (11 misto 21 ms), abych
        // zkratil zpozdeni zvuku. VYSLEDEK: zvuk Segy prestal chodit na TV.
        // Duvod: mensi zasobnik znamena kratsi davky a jina casovani, na
        // kterych zavisi odbocka zvuku pro televizi.
        // VRACENO NA PUVODNI HODNOTY. Zpozdeni zvuku se musi resit jinde,
        // ne velikosti zasobniku.
        switch (perfTier()) {
            case 2:  return 1024;   // HIGH   ~21 ms
            case 1:  return 2048;   // MEDIUM ~43 ms
            default: return 4096;   // LOW    ~85 ms
        }
    }

    private void ps1ActivateNativeView() {
        // Obraz PS1 kresli OpenGL (ps1GlEnable). Stara cesta pres lockCanvas
        // (trida NativePs1InPlaceView) byla pomala, 31-57 ms na snimek, a od
        // KROKU D se nespoustela - stala tu jen jako mrtvy kod za "if (true)
        // return;". Smazana i s tridou, aby uz nikdo nehledal, ktera plocha
        // vlastne kresli.
        ui.post(() -> ps1GlEnable());
    }

    private void ps1DeactivateNativeView() {
        ps1GlDisable();
    }

    private void closePs1GamePfdQuietly() {
        synchronized (MainActivity.class) {
            try {
                if (ps1GamePfd != null) {
                    ps1GamePfd.close();
                    ps1GamePfd = null;
                }
            } catch (Throwable ignored) {}
        }
    }

    private boolean isUiThread() {
        return Looper.myLooper() == Looper.getMainLooper();
    }

    private void removeNativeViewOnUi(final String reason) {
        final NativeInPlaceView old = nativeInPlaceView;
        nativeInPlaceView = null;
        Runnable r = () -> {
            try {
                if (old != null) {
                    try { old.stop(); } catch (Throwable ignored) {}
                    try { old.setVisibility(View.GONE); } catch (Throwable ignored) {}
                    try { if (old.getParent() instanceof ViewGroup) ((ViewGroup) old.getParent()).removeView(old); } catch (Throwable t) { appendNativeLog("NATIVE_VIEW_REMOVE_UI_ERR_RV " + safeMsg(t)); }
                }
                appendNativeLog("NATIVE_VIEW_REMOVE_UI_OK_RV reason=" + reason);
            } catch (Throwable t) {
                appendNativeLog("NATIVE_VIEW_REMOVE_UI_FAIL_RV reason=" + reason + " " + safeMsg(t));
            }
        };
        if (isUiThread()) r.run(); else ui.post(r);
    }

    private synchronized String stopNativeInPlaceHard(String reason) {
        if (!nativeInPlaceEnabled && !nativeCoreAudioRun && nativeInPlaceView == null) {
            nativeLastStatus = "NATIVE_ALREADY_OFF " + reason;
            return nativeLastStatus;
        }
        StringBuilder res = new StringBuilder();
        try { appendNativeLog("NATIVE_LIFECYCLE_STOP_BEGIN reason=" + reason); } catch (Throwable ignored) {}
        nativeRomLoadGeneration++; // BUILD2RV: cancel stale delayed audio/render watchdogs when leaving Sega/Atari/VBXE.
        nativeInPlaceEnabled = false;
        try { stopNativeCoreAudioStream(); nativeCurrentAudioTrack = null; res.append("audioStop=OK "); } catch (Throwable t) { res.append("audioStop=ERR:").append(safeMsg(t)).append(' '); }
        try {
            for (int i = 0; i < 8; i++) NativeSegaCoreBridge.setInput(i, false);
        } catch (Throwable ignored) {}
        try {
            String nativeStop = NativeSegaCoreBridge.shutdown();
            res.append("nativeStop=").append(nativeStop == null ? "null" : nativeStop.replace('\n', ' ')).append(' ');
        } catch (Throwable t) { res.append("nativeStop=ERR:").append(safeMsg(t)).append(' '); }
        try {
            removeNativeViewOnUi(reason);
            res.append("viewRemovePostedUI=YES");
        } catch (Throwable t) { res.append("viewRemovePostedUI=ERR:").append(safeMsg(t)); }
        nativeLastStatus = "NATIVE_OFF_LIFECYCLE_STOP " + reason + " " + res;
        appendNativeLog(nativeLastStatus);
        return nativeLastStatus;
    }

    public class AHNative {
        @JavascriptInterface
        public String enableInPlace() {
            try {
                final String build = NativeSegaCoreBridge.buildString();
                nativeLastStatus = "NATIVE_IN_PLACE_OK " + build.replace('\n', ' ');
                appendNativeLog("ENABLE_IN_PLACE " + nativeLastStatus);
                ui.post(() -> {
                    try {
                        if (rootFrame == null || web == null) return;
                        // BUILD2RV: enableInPlace is now idempotent. Rotation/resize/visibility on S8 called it many times;
                        // recreating TextureView each time slowly killed mobile video. Reuse the existing native view when it is alive.
                        if (nativeInPlaceView != null && nativeInPlaceView.getParent() == rootFrame) {
                            nativeInPlaceEnabled = true;
                            try { nativeInPlaceView.setAlpha(1f); nativeInPlaceView.setTranslationX(0f); nativeInPlaceView.setTranslationY(0f); } catch (Throwable ignored) {}
                            try { nativeInPlaceView.start(); nativeInPlaceView.forceRedrawOnce(); } catch (Throwable ignored) {}
                            appendNativeLog("NATIVE_VIEW_REUSE_RV parent=rootFrame noRecreate=YES");
                            return;
                        }
                        if (nativeInPlaceView != null) {
                            try { nativeInPlaceView.stop(); } catch (Throwable ignored) {}
                            try { if (nativeInPlaceView.getParent() != null) ((ViewGroup) nativeInPlaceView.getParent()).removeView(nativeInPlaceView); } catch (Throwable ignored) {}
                            nativeInPlaceView = null;
                            appendNativeLog("NATIVE_VIEW_RECREATE_RV oldViewRemoved=YES detachedOrBadParent");
                        }
                        nativeInPlaceView = new NativeInPlaceView(MainActivity.this);
                        nativeInPlaceView.setClickable(false);
                        nativeInPlaceView.setEnabled(false);
                        nativeInPlaceView.setFocusable(false);
                        nativeInPlaceView.setFocusableInTouchMode(false);
                        // BUILD2RV: never show native view before JS sends a valid monitor/landscape rect.
                        nativeInPlaceView.setVisibility(View.INVISIBLE);
                        try { nativeInPlaceView.setLayerType(View.LAYER_TYPE_HARDWARE, null); } catch (Throwable ignored) {}
                        rootFrame.addView(nativeInPlaceView, new FrameLayout.LayoutParams(1, 1));
                        appendNativeLog("NATIVE_VIEW_REATTACH_RV parent=rootFrame freshView=YES hiddenUntilValidRect=YES");
                        nativeInPlaceEnabled = true;
                        nativeInPlaceView.setAlpha(1f);
                        nativeInPlaceView.setTranslationX(0f);
                        nativeInPlaceView.setTranslationY(0f);
                    } catch (Throwable t) {
                        nativeLastStatus = "NATIVE_VIEW_ERROR " + safeMsg(t);
                        appendNativeLog(nativeLastStatus);
                    }
                });
                return nativeLastStatus;
            } catch (Throwable t) {
                nativeLastStatus = "NATIVE_ENABLE_ERROR " + safeMsg(t);
                appendNativeLog(nativeLastStatus);
                return nativeLastStatus;
            }
        }

        @JavascriptInterface
        public String setNativeRect(final int x, final int y, final int w, final int h) {
            try {
                ui.post(() -> {
                    try {
                        if (nativeInPlaceView == null || rootFrame == null) return;
                        // BUILD2RV: Samsung/WebView can send a transient 0x0 rect when returning from landscape.
                        // Ignoring that rect prevents the native video from becoming black after portrait return.
                        if (w < 120 || h < 80) { appendNativeLog("SET_RECT_SKIP_SMALL_RV x=" + x + " y=" + y + " w=" + w + " h=" + h); return; }
                        int ww = Math.max(120, w);
                        int hh = Math.max(80, h);
                        FrameLayout.LayoutParams lp = new FrameLayout.LayoutParams(ww, hh);
                        lp.leftMargin = Math.max(0, x);
                        lp.topMargin = Math.max(0, y);
                        nativeInPlaceView.setLayoutParams(lp);
                        nativeInPlaceView.setVisibility(nativeInPlaceEnabled ? View.VISIBLE : View.GONE);
                        if (nativeInPlaceEnabled) nativeInPlaceView.start();
                        boolean landscapeFull = ww > (hh * 1.12f) && x <= 4 && y <= 4;
                        nativeLandscapeFullVideo = landscapeFull;
                        if (landscapeFull && web != null) {
                            // BUILD2RV: on real phones the C++ native view is the video layer, WebView is only transparent controls/log.
                            // This gives full-screen Sonic/Aladdin with transparent joystick/buttons over the picture.
                            try { nativeInPlaceView.setZ(0f); } catch (Throwable ignored) {}
                            try { web.setZ(10f); } catch (Throwable ignored) {}
                            try { rootFrame.bringChildToFront(web); } catch (Throwable ignored) {}
                            appendNativeLog("NATIVE_Z_ORDER_RV landscapeWebControlsOverNative=YES topLabelsHidden=YES");
                        } else {
                            try { if (web != null) web.setZ(0f); } catch (Throwable ignored) {}
                            try { nativeInPlaceView.setZ(20f); } catch (Throwable ignored) {}
                            nativeInPlaceView.bringToFront();
                        }
                        nativeInPlaceView.requestLayout();
                        nativeInPlaceView.invalidate();
                        appendNativeLog("SET_RECT_OK_RV x=" + Math.max(0, x) + " y=" + Math.max(0, y) + " w=" + ww + " h=" + hh + " visible=" + nativeInPlaceEnabled + " landscapeFull=" + landscapeFull);
                    } catch (Throwable t) { appendNativeLog("SET_RECT_ERROR " + safeMsg(t)); }
                });
                return "RECT_OK x=" + x + " y=" + y + " w=" + w + " h=" + h;
            } catch (Throwable t) {
                return "RECT_ERROR " + safeMsg(t);
            }
        }

        @JavascriptInterface
        public String loadRomBase64(String name, String b64) {
            try {
                if (b64 == null) b64 = "";
                String clean = b64.replaceAll("\\s", "");
                long decodeStart = System.currentTimeMillis();
                byte[] data = Base64.decode(clean, Base64.DEFAULT);
                long decodeMs = System.currentTimeMillis() - decodeStart;
                long t0 = System.currentTimeMillis();
                String info = NativeSegaCoreBridge.romInfo(data);
                long dt = System.currentTimeMillis() - t0;
                try {
                    String rt = NativeSegaCoreBridge.configureRuntime(Build.VERSION.SDK_INT, Runtime.getRuntime().availableProcessors(), Build.MODEL == null ? "" : Build.MODEL);
                    appendNativeLog("NATIVE_RUNTIME_CONFIG_RV " + rt);
                } catch (Throwable rtErr) {
                    appendNativeLog("NATIVE_RUNTIME_CONFIG_RV_ERROR " + safeMsg(rtErr));
                }

                // BUILD2RV: audio starts only after visible native frame; zeroed balanced audio clock/music timing lives in C++ core.
                // Tvrdý fresh start pred kazdou ROM brani stavu: Atari 130XE -> Sega -> nova ROM -> cerna obrazovka + zvuk.
                final int loadGen = ++nativeRomLoadGeneration;
                appendNativeLog("FRESH_ROM_GENERATION_RV gen=" + loadGen + " oldAudioGen=" + nativeAudioGeneration + " oldDraw=" + nativeViewDrawCounter);
                stopNativeCoreAudioStream();
                try { NativeSegaCoreBridge.shutdown(); appendNativeLog("FRESH_ROM_HARD_STOP_BEFORE_LOAD_RV gen=" + loadGen); } catch (Throwable ignored) {}
                nativeViewDrawCounterAtRomLoad = nativeViewDrawCounter;
                nativeRenderPerfWindowStartMs = 0; nativeRenderPerfWindowFrames = 0; nativeRenderPerfSlowFrames = 0;
                String realCore = NativeSegaCoreBridge.realCoreLoadRom(data);
                // Obraz Segy jde od B117 PRIMO na plochu pres OpenGL ES,
                // stejne jako u PS1. Doted se snimal z okna aplikace, coz
                // zpusobovalo viditelne zpozdeni proti mobilu.
                ui.post(() -> segaPlochaZapni());

                nativeLastRomInfo = "ROM: " + safeFileName(name) + "\n" + info + "\n\nREAL CORE SLOT:\n" + realCore;
                nativeLastStatus = "ROM_REAL_CORE_LOAD_READY bytes=" + data.length + " decodeMs=" + decodeMs + " parserMs=" + dt;
                appendNativeLog("ROM_REAL_CORE_LOAD_READY name=" + safeFileName(name) + " bytes=" + data.length + " decodeMs=" + decodeMs + " parserMs=" + dt + " gen=" + loadGen);
                appendNativeLog("REAL_CORE_RENDER_ACTIVE_RV after ROM load audio=WAIT_FRAME_AND_VIEW_DRAW");
                forceNativeViewRedrawBurst("afterRomLoad_RV");
                scheduleNativeAudioAfterFrameAndViewDraw(name, data, loadGen, 1);
                scheduleNativeRenderWatchdog(name, data, loadGen, 1);
                schedulePassiveAuditRW(loadGen, 1); // BUILD2RW: passive 10s audit rows, measure only
                appendNativeLog("PASSIVE_AUDIT_RX_START gen=" + loadGen + " intervalMs=10000 changesNothing=YES");
                return nativeLastStatus + "\n" + info + "\n\nREAL CORE SLOT:\n" + realCore;
            } catch (Throwable t) {
                nativeLastStatus = "ROM_TO_CPP_ERROR " + safeMsg(t);
                appendNativeLog(nativeLastStatus);
                return nativeLastStatus;
            }
        }

        @JavascriptInterface
        public String coreStep() {
            try {
                appendNativeLog("CORE_STEP_REQUEST");
                String res = NativeSegaCoreBridge.realCoreStep();
                nativeLastStatus = res == null ? "CORE_STEP_NULL" : res;
                appendNativeLog("CORE_STEP_RESULT " + nativeLastStatus.replace('\n', ' '));
                return nativeLastStatus;
            } catch (Throwable t) {
                String e = "CORE_STEP_ERROR " + safeMsg(t);
                nativeLastStatus = e;
                appendNativeLog(e);
                return e;
            }
        }

        @JavascriptInterface
        public String input(String btn, boolean down) {
            try {
                int key = mapNativeKey(btn);
                if (key < 0) return "INPUT_UNKNOWN " + btn;
                NativeSegaCoreBridge.setInput(key, down);
                nativeInputEvents++;
                String st = NativeSegaCoreBridge.inputStatus();
                if (nativeInputEvents <= 16 || nativeInputEvents % 20 == 0) appendNativeLog("INPUT " + btn + " " + (down ? "DOWN" : "UP") + " count=" + nativeInputEvents + " " + st);
                return st;
            } catch (Throwable t) {
                String e = "INPUT_ERROR " + safeMsg(t);
                appendNativeLog(e);
                return e;
            }
        }

        @JavascriptInterface
        public String disableInPlace(String reason) {
            return stopNativeInPlaceHard(reason == null ? "js" : reason);
        }

        @JavascriptInterface
        public String status() {
            try { return NativeSegaCoreBridge.realCoreStatus() + "\nviewDrawCounter=" + nativeViewDrawCounter + "\nviewEnabled=" + nativeInPlaceEnabled + "\nperfMode=" + nativePerformanceMode; }
            catch (Throwable t) { return "STATUS_ERROR " + safeMsg(t); }
        }


        @JavascriptInterface
        public String setPerformanceMode(String mode) {
            try {
                String m = mode == null ? "HIGH" : mode.trim().toUpperCase(Locale.US);
                if (!m.startsWith("LOW")) m = "HIGH"; else m = "LOW";
                nativePerformanceMode = m;
                String nativePerf = "nativePerf=not-called";
                try { NativeSegaCoreBridge.configureRuntime(Build.VERSION.SDK_INT, Runtime.getRuntime().availableProcessors(), Build.MODEL == null ? "" : Build.MODEL); } catch (Throwable ignored) {}
                try { nativePerf = NativeSegaCoreBridge.setPerformanceMode(m); } catch (Throwable nt) { nativePerf = "nativePerfError=" + safeMsg(nt); }
                appendNativeLog("NATIVE_PERF_MODE_RV mode=" + nativePerformanceMode + " sdk=" + Build.VERSION.SDK_INT + " " + nativePerf);
                if (nativeInPlaceView != null) {
                    try { nativeInPlaceView.forceRedrawOnce(); } catch (Throwable ignored) {}
                }
                return "NATIVE_PERF_MODE_OK_RV mode=" + nativePerformanceMode + " " + nativePerf;
            } catch (Throwable t) {
                return "NATIVE_PERF_MODE_ERROR_RV " + safeMsg(t);
            }
        }

        @JavascriptInterface
        public String audioTest() {
            try {
                int sampleRate = 48000;
                int samples = 16000;
                short[] pcm = new short[samples];
                NativeSegaCoreBridge.makeAudioTone(pcm, sampleRate, 440.0);
                AudioTrack track;
                if (Build.VERSION.SDK_INT >= 21) {
                    track = new AudioTrack.Builder()
                            .setAudioAttributes(new AudioAttributes.Builder().setUsage(AudioAttributes.USAGE_GAME).setContentType(AudioAttributes.CONTENT_TYPE_SONIFICATION).build())
                            .setAudioFormat(new AudioFormat.Builder().setEncoding(AudioFormat.ENCODING_PCM_16BIT).setSampleRate(sampleRate).setChannelMask(AudioFormat.CHANNEL_OUT_MONO).build())
                            .setBufferSizeInBytes(samples * 2)
                            .setTransferMode(AudioTrack.MODE_STATIC)
                            .build();
                } else {
                    track = new AudioTrack(android.media.AudioManager.STREAM_MUSIC, sampleRate, AudioFormat.CHANNEL_OUT_MONO, AudioFormat.ENCODING_PCM_16BIT, samples * 2, AudioTrack.MODE_STATIC);
                }
                int written = track.write(pcm, 0, pcm.length);
                track.play();
                appendNativeLog("AUDIO_TEST_PLAY sampleRate=" + sampleRate + " written=" + written + " state=" + track.getPlayState());
                ui.postDelayed(() -> { try { track.stop(); track.release(); } catch (Throwable ignored) {} }, 850);
                return "AUDIO_TEST_OK sampleRate=" + sampleRate + " written=" + written;
            } catch (Throwable t) {
                String e = "AUDIO_TEST_ERROR " + safeMsg(t);
                appendNativeLog(e);
                return e;
            }
        }

        @JavascriptInterface
        public String saveLog() {
            try {
                long now = System.currentTimeMillis();
                if (now - nativeLastSaveLogAtMs < 2200) {
                    appendNativeLog("SAVE_LOG_DEDUP_RV ignored deltaMs=" + (now - nativeLastSaveLogAtMs));
                    return "SAVE_LOG_DEDUP_RV";
                }
                nativeLastSaveLogAtMs = now;
                String fn = "AtariHelp_SEGA_CPP_INPLACE_LOG_BUILD2SA2_" + new SimpleDateFormat("yyyyMMdd_HHmmss", Locale.US).format(new Date()) + ".txt";
                String path = writeBytesToDownloads(fn, buildNativeInPlaceLog().getBytes("UTF-8"));
                appendNativeLog("SAVE_LOG_OK " + path);
                return "SAVE_LOG_OK " + path;
            } catch (Throwable t) {
                String e = "SAVE_LOG_ERROR " + safeMsg(t);
                appendNativeLog(e);
                return e;
            }
        }
    }

    private String safeMsg(Throwable t) { return t == null ? "null" : (t.getMessage() == null ? t.toString() : t.getMessage()); }

    private void forceNativeViewRedrawBurst(final String reason) {
        ui.post(() -> {
            try {
                if (nativeInPlaceView != null) {
                    nativeInPlaceView.setVisibility(nativeInPlaceEnabled ? View.VISIBLE : View.GONE);
                    if (!nativeLandscapeFullVideo) nativeInPlaceView.bringToFront();
                    nativeInPlaceView.start();
                    nativeInPlaceView.forceRedrawOnce();
                    appendNativeLog("NATIVE_TEXTURE_REDRAW_BURST " + reason);
                }
            } catch (Throwable t) { appendNativeLog("NATIVE_VIEW_REDRAW_BURST_ERROR " + safeMsg(t)); }
        });
        // BUILD2RV: keep only a short burst; long delayed bursts from old ROMs added work after multiple ROM loads.
        ui.postDelayed(() -> { try { if (nativeInPlaceView != null) nativeInPlaceView.forceRedrawOnce(); } catch (Throwable ignored) {} }, 120);
        ui.postDelayed(() -> { try { if (nativeInPlaceView != null) nativeInPlaceView.forceRedrawOnce(); } catch (Throwable ignored) {} }, 420);
    }

    private void scheduleNativeAudioAfterFrameAndViewDraw(final String romName, final byte[] romData, final int gen, final int attempt) {
        final int delay = attempt <= 1 ? 220 : 320;
        ui.postDelayed(() -> {
            try {
                if (gen != nativeRomLoadGeneration) { appendNativeLog("NATIVE_AUDIO_WAIT_CANCELLED_RV staleGen=" + gen + " current=" + nativeRomLoadGeneration); return; }
                String st = NativeSegaCoreBridge.realCoreStatus();
                boolean hasFrame = st != null && st.indexOf("frameReady=YES") >= 0 && st.indexOf("frameCounter=0") < 0;
                boolean viewReady = nativeInPlaceEnabled && nativeInPlaceView != null && nativeInPlaceView.getParent() != null
                        && nativeInPlaceView.getVisibility() == View.VISIBLE && nativeViewDrawCounter > nativeViewDrawCounterAtRomLoad;
                appendNativeLog("NATIVE_AUDIO_WAIT_FRAME_VIEW_RV attempt=" + attempt + " gen=" + gen + " hasFrame=" + hasFrame + " viewReady=" + viewReady + " draw=" + nativeViewDrawCounter + "/" + nativeViewDrawCounterAtRomLoad + " " + (st == null ? "null" : st.replace('\n',' ').substring(0, Math.min(430, st.length()))));
                if (hasFrame && viewReady) {
                    startNativeCoreAudioStream();
                    appendNativeLog("NATIVE_AUDIO_START_AFTER_FRAME_VIEW_RV name=" + safeFileName(romName) + " gen=" + gen + " audioProfile=QT_RR_RECOVERY_SAFE_AUDIT");
                    return;
                }
                if (attempt < 10 && nativeInPlaceEnabled) {
                    scheduleNativeAudioAfterFrameAndViewDraw(romName, romData, gen, attempt + 1);
                    return;
                }
                stopNativeCoreAudioStream();
                nativeLastStatus = "NATIVE_AUDIO_NOT_STARTED_NO_VISIBLE_FRAME_RV name=" + safeFileName(romName);
                appendNativeLog(nativeLastStatus);
                forceNativeViewRedrawBurst("audioNoVisibleFrame_RV");
            } catch (Throwable t) {
                appendNativeLog("NATIVE_AUDIO_WAIT_FRAME_VIEW_RV_ERROR " + safeMsg(t));
            }
        }, delay);
    }

    // BUILD2RX passive: battery temperature (sticky intent, no permission) + current CPU freq.
    // RW data showed S8 degradation with FLAT heap/GC => prime suspect is thermal throttling.
    // These two read-only values will prove or kill that theory.
    private String readBatteryTempC() {
        try {
            Intent i = registerReceiver(null, new IntentFilter(Intent.ACTION_BATTERY_CHANGED));
            if (i == null) return "na";
            int t = i.getIntExtra(BatteryManager.EXTRA_TEMPERATURE, -1);
            if (t < 0) return "na";
            return String.format(Locale.US, "%.1f", t / 10.0);
        } catch (Throwable t) { return "err"; }
    }
    private String readCpuFreqKHz(int cpu) {
        try {
            File f = new File("/sys/devices/system/cpu/cpu" + cpu + "/cpufreq/scaling_cur_freq");
            if (!f.canRead()) return "na";
            java.io.BufferedReader br = new java.io.BufferedReader(new java.io.FileReader(f));
            String line = br.readLine();
            br.close();
            return line == null ? "na" : line.trim();
        } catch (Throwable t) { return "na"; }
    }
    // BUILD2SK84: citelne jmeno pro PowerManager.getCurrentThermalStatus() (API 29+,
    // volajici uz hlida SDK_INT). Konstanty samotne jsou dostupne na compileSdk bez
    // ohledu na minSdk - jen samotne VOLANI getCurrentThermalStatus() potrebuje strazit.
    private String napTvWebThermalName(int status) {
        switch (status) {
            case android.os.PowerManager.THERMAL_STATUS_NONE: return "NONE";
            case android.os.PowerManager.THERMAL_STATUS_LIGHT: return "LIGHT";
            case android.os.PowerManager.THERMAL_STATUS_MODERATE: return "MODERATE";
            case android.os.PowerManager.THERMAL_STATUS_SEVERE: return "SEVERE";
            case android.os.PowerManager.THERMAL_STATUS_CRITICAL: return "CRITICAL";
            case android.os.PowerManager.THERMAL_STATUS_EMERGENCY: return "EMERGENCY";
            case android.os.PowerManager.THERMAL_STATUS_SHUTDOWN: return "SHUTDOWN";
            default: return "UNKNOWN(" + status + ")";
        }
    }
    private String pickField(String flat, String key) {
        try {
            int i = flat.indexOf(key);
            if (i < 0) return "na";
            int end = flat.indexOf(' ', i);
            if (end < 0) end = Math.min(flat.length(), i + 40);
            return flat.substring(i + key.length(), end);
        } catch (Throwable t) { return "na"; }
    }

    // BUILD2RW PASSIVE AUDIT: every 10s log one row with Java heap, native heap, GC, AudioTrack
    // underruns, generations and the C++ audit block. It only reads and logs; it never changes
    // region, FIFO, clocks, gain or render. Goal: after 10-15 min on S8 the log itself shows
    // WHAT grows (heap? FIFO backlog? underruns? pull cost?) before we touch any fix.
    private void schedulePassiveAuditRW(final int gen, final int tick) {
        ui.postDelayed(() -> {
            try {
                if (gen != nativeRomLoadGeneration || !nativeInPlaceEnabled) {
                    appendNativeLog("PASSIVE_AUDIT_RX_STOP gen=" + gen + " current=" + nativeRomLoadGeneration + " enabled=" + nativeInPlaceEnabled);
                    return;
                }
                Runtime rt = Runtime.getRuntime();
                long jUsedKb = (rt.totalMemory() - rt.freeMemory()) / 1024;
                long jTotalKb = rt.totalMemory() / 1024;
                long jMaxKb = rt.maxMemory() / 1024;
                long nAllocKb = 0, nTotalKb = 0;
                try { nAllocKb = Debug.getNativeHeapAllocatedSize() / 1024; nTotalKb = Debug.getNativeHeapSize() / 1024; } catch (Throwable ignored) {}
                String gcCount = "na", gcTime = "na";
                try {
                    if (Build.VERSION.SDK_INT >= 23) {
                        gcCount = Debug.getRuntimeStat("art.gc.gc-count");
                        gcTime = Debug.getRuntimeStat("art.gc.gc-time");
                    }
                } catch (Throwable ignored) {}
                int trackUnderruns = -1;
                try {
                    AudioTrack at = nativeCurrentAudioTrack;
                    if (at != null && Build.VERSION.SDK_INT >= 24) trackUnderruns = at.getUnderrunCount();
                } catch (Throwable ignored) {}
                String st;
                try { st = NativeSegaCoreBridge.realCoreStatus(); } catch (Throwable t) { st = "coreStatusErr=" + safeMsg(t); }
                String flat = st == null ? "null" : st.replace('\n', ' ');
                // BUILD2RX: RW truncation cut coreAvgMs off the row; extract the key fields explicitly.
                String coreAvg = pickField(flat, "coreAvgMs=");
                String coreMax = pickField(flat, "coreMaxMs=");
                String stress = pickField(flat, "sceneStress=");
                String backlog = pickField(flat, "audioBacklog=");
                appendNativeLog("PASSIVE_AUDIT_RX tick=" + tick + " gen=" + gen
                        + " upMin=" + ((tick * 10) / 60) + "." + String.format(Locale.US, "%02d", (tick * 10) % 60)
                        + " batteryTempC=" + readBatteryTempC()
                        + " cpu0KHz=" + readCpuFreqKHz(0) + " cpu4KHz=" + readCpuFreqKHz(4)
                        + " coreAvgMs=" + coreAvg + " coreMaxMs=" + coreMax + " sceneStress=" + stress + " audioBacklog=" + backlog
                        + " javaHeapUsedKB=" + jUsedKb + " javaHeapTotalKB=" + jTotalKb + " javaHeapMaxKB=" + jMaxKb
                        + " nativeHeapAllocKB=" + nAllocKb + " nativeHeapTotalKB=" + nTotalKb
                        + " gcCount=" + gcCount + " gcTimeMs=" + gcTime
                        + " audioTrackUnderruns=" + trackUnderruns
                        + " activeAudioTracks=" + nativeActiveAudioTracks
                        + " audioGen=" + nativeAudioGeneration + " romGen=" + nativeRomLoadGeneration
                        + " drawCounter=" + nativeViewDrawCounter
                        + " lastRenderCostMs=" + (nativeLastRenderCostNs / 1000000.0)
                        + " perfMode=" + nativePerformanceMode
                        + " core=" + flat.substring(0, Math.min(1400, flat.length())));
                schedulePassiveAuditRW(gen, tick + 1);
            } catch (Throwable t) {
                appendNativeLog("PASSIVE_AUDIT_RX_ERROR " + safeMsg(t));
            }
        }, 10000);
    }

    private void scheduleNativeRenderWatchdog(final String romName, final byte[] romData, final int gen, final int attempt) {
        final int delay = attempt <= 1 ? 1350 : 2600;
        ui.postDelayed(() -> {
            try {
                if (gen != nativeRomLoadGeneration) { appendNativeLog("NATIVE_RENDER_WATCHDOG_CANCELLED_RV staleGen=" + gen + " current=" + nativeRomLoadGeneration); return; }
                String st = NativeSegaCoreBridge.realCoreStatus();
                boolean hasFrame = st != null && st.indexOf("frameReady=YES") >= 0 && st.indexOf("frameCounter=0") < 0;
                boolean viewReady = nativeInPlaceEnabled && nativeInPlaceView != null && nativeInPlaceView.getParent() != null
                        && nativeInPlaceView.getVisibility() == View.VISIBLE && nativeViewDrawCounter > nativeViewDrawCounterAtRomLoad;
                appendNativeLog("NATIVE_RENDER_WATCHDOG_RV attempt=" + attempt + " gen=" + gen + " hasFrame=" + hasFrame + " viewReady=" + viewReady + " draw=" + nativeViewDrawCounter + "/" + nativeViewDrawCounterAtRomLoad + " " + (st == null ? "null" : st.replace('\n',' ').substring(0, Math.min(520, st.length()))));
                if (hasFrame && viewReady) {
                    forceNativeViewRedrawBurst("watchdogFrameViewOK_RV");
                    return;
                }
                if (attempt <= 1 && romData != null && romData.length > 0 && nativeInPlaceEnabled) {
                    appendNativeLog("NATIVE_RENDER_WATCHDOG_FRESH_RELOAD_RV name=" + safeFileName(romName) + " bytes=" + romData.length + " gen=" + gen);
                    try { stopNativeCoreAudioStream(); } catch (Throwable ignored) {}
                    try { NativeSegaCoreBridge.shutdown(); } catch (Throwable ignored) {}
                    nativeViewDrawCounterAtRomLoad = nativeViewDrawCounter;
                    String reload = NativeSegaCoreBridge.realCoreLoadRom(romData);
                    ui.post(() -> segaPlochaZapni());
                    nativeLastStatus = "NATIVE_RENDER_WATCHDOG_RELOAD_RV " + (reload == null ? "null" : reload.replace('\n',' '));
                    appendNativeLog(nativeLastStatus.substring(0, Math.min(900, nativeLastStatus.length())));
                    forceNativeViewRedrawBurst("watchdogFreshReload_RV");
                    scheduleNativeAudioAfterFrameAndViewDraw(romName, romData, gen, 1);
                    scheduleNativeRenderWatchdog(romName, romData, gen, attempt + 1);
                    return;
                }
                if (!hasFrame || !viewReady) {
                    stopNativeCoreAudioStream();
                    nativeLastStatus = "NATIVE_RENDER_NO_VISIBLE_FRAME_AUDIO_STOPPED_RV name=" + safeFileName(romName);
                    appendNativeLog(nativeLastStatus);
                    forceNativeViewRedrawBurst("watchdogNoVisibleFrameAudioStopped_RV");
                }
            } catch (Throwable t) {
                appendNativeLog("NATIVE_RENDER_WATCHDOG_RV_ERROR " + safeMsg(t));
            }
        }, delay);
    }

    // ============================================================
    //  KROK C2: plynuly PS1 obraz pres TextureView + vlastni OpenGL.
    //
    //  Dve opravy oproti C1b:
    //   1) VYPNE starou zobrazovaci cestu (ps1DeactivateNativeView),
    //      ktera bezela neviditelne pod nami a zrala procesor
    //      (v logu PS1_NATIVE_TEXTURE_SLOW_AVG avgCostMs=31-57).
    //      Tim se uvolni vykon a obraz je plne plynuly.
    //   2) TextureView misto GLSurfaceView -> TV cast obraz ZASE VIDI
    //      (GLSurfaceView je samostatna vrstva, PixelCopy ji nezachyti
    //      a posilal na TV cernou plochu, brightAvg=0).
    //
    //  Prepinac: klik na logo NaP zapne, dalsi klik vypne.
    // ============================================================
    // Ps1GlTextureView uz se pro obraz nepouziva - kresli nativni plocha.
    // Trida zustava kvuli borrowFrame(), ktery pouziva TV.


    // Zapnout nas plynuly OpenGL obraz (hlavni zobrazovaci cesta pro PS1).
    private android.view.SurfaceView ps1Plocha = null;
    private java.util.Timer plochaHlidac = null;

    /** HLIDAC, ABY OBRAZ PS1 NELEZL DO CELE APLIKACE.
     *  Plocha lezi NAD strankou (jinak by ji na vysku zakryla neprusvitna
     *  grafika konzole), takze mimo obrazovku PS1 by prosvitala vsude -
     *  presne to je ten "duch BIOSu v cele apce".
     *  Drive tohle sledovani viselo uvnitr smycky pro TV, takze BEZ ZAPNUTE
     *  TV se vubec nespoustelo. Ted bezi samostatne, vzdycky. */
    private void plochaHlidacStart() {
        if (plochaHlidac != null) return;
        plochaHlidac = new java.util.Timer("ps1-plocha-hlidac", true);
        plochaHlidac.schedule(new java.util.TimerTask() {
            @Override public void run() {
                try { ui.post(() -> plochaZkontroluj()); } catch (Throwable ignored) {}
            }
        }, 300, 300);
    }

    private void plochaHlidacStop() {
        try { if (plochaHlidac != null) plochaHlidac.cancel(); } catch (Throwable ignored) {}
        plochaHlidac = null;
    }

    private void plochaZkontroluj() {
        // Sega ma vlastni plochu - schovat ji, kdyz nejsme na jeji obrazovce,
        // jinak by obraz prosvital do zbytku aplikace (stejny problem jako
        // mela PS1).
        try {
            android.view.SurfaceView sp = segaPlocha;
            if (sp != null) {
                String us = null;
                try { if (web != null) us = web.getUrl(); } catch (Throwable ignored) {}
                boolean jeSega = (us != null) && us.contains("emu_sega");
                int chciS = jeSega ? View.VISIBLE : View.INVISIBLE;
                if (sp.getVisibility() != chciS) {
                    sp.setVisibility(chciS);
                    appendNativeLog("SEGA_PLOCHA_" + (jeSega ? "ZOBRAZENA" : "SCHOVANA"));
                }
            }
        } catch (Throwable ignored) {}
        try {
            android.view.SurfaceView pl = ps1Plocha;
            if (pl == null) return;
            String u = null;
            try { if (web != null) u = web.getUrl(); } catch (Throwable ignored) {}
            boolean jePs1 = (u != null) && u.contains("emu_ps1");
            int chci = jePs1 ? View.VISIBLE : View.INVISIBLE;
            if (pl.getVisibility() != chci && (jePs1 || plochaW > 0)) {
                pl.setVisibility(chci);
                appendNativeLog("PLOCHA_" + (jePs1 ? "ZOBRAZENA" : "SCHOVANA")
                        + " (obrazovka " + (jePs1 ? "PS1" : String.valueOf(u)) + ")");
            }
        } catch (Throwable ignored) {}
    }

    // Vychozi obdelnik okenka konzole na vysku. Stranka ho hned upresni
    // (PLOCHA_MISTO_ZE_STRANKY), ale nez to udela, at uz je obraz videt.
    private int plochaL = -1, plochaT = -1, plochaW = -1, plochaH = -1;
    // NA VYSKU MUSI BYT PLOCHA NAHORE UZ OD PRVNI CHVILE.
    // Kdyz zacinala dole, na vysku ji zakryla neprusvitna grafika konzole
    // a obraz naskocil az potom, co stranka nahlasila obdelnik - to je az
    // 400 ms cerna. PS1 se spousti na vysku, takze vychozi stav je "nahore"
    // a prestavba se udela jen pri otoceni na sirku.
    private boolean plochaNaSirku = false, plochaZOrderNahore = true;

    /** Stranka rekne, kde ma obraz byt. Na VYSKU musi plocha lezet NAD
     *  strankou (grafika konzole je neprusvitna a jinak obraz zakryje),
     *  na SIRKU pod ni (stranka je pruhledna a ovladac ma byt nad obrazem).
     *  Vysku vrstvy nelze menit za behu - proto se plocha postavi znovu. */
    /** Postavi plochu tam, kam ji stranka hlasi.
     *  NA VYSKU: obraz lezi v okenku konzole a grafika konzole je
     *    NEPRUSVITNA - plocha proto musi byt NAD strankou. Je mala
     *    (642x533) a mimo obrazovku PS1 se schovava, takze uz nikam
     *    neprosvita.
     *  NA SIRKU: stranka je pruhledna, plocha je POD ni pres cely vyrez
     *    a ovladac zustava nad obrazem.
     *  Vysku vrstvy nelze menit za behu, proto se plocha pri ZMENE OTOCENI
     *  postavi znovu - ale jen tehdy, kdyz se otoceni opravdu zmenilo. */
    private void ps1PlochaUmisti(int l, int t, int w, int hh, boolean naSirku) {
        if (rootFrame == null || w <= 0 || hh <= 0) return;
        boolean stejne = (ps1Plocha != null) && (plochaW == w) && (plochaH == hh)
                && (plochaL == l) && (plochaT == t) && (plochaNaSirku == naSirku);
        if (stejne) return;

        // POZOR: rozhodovat podle VRSTVY, ne podle zmeny otoceni. Plocha
        // vznikne s vrstvou "dole"; kdyz pak prijde prvni hlaseni na vysku,
        // otoceni se NEZMENILO (obojí false) a vrstva by zustala dole -
        // obraz by na vysku zustal schovany pod neprusvitnou grafikou konzole.
        boolean chceNahore = !naSirku;
        boolean prestavet  = (ps1Plocha == null) || (plochaZOrderNahore != chceNahore);
        plochaL = l; plochaT = t; plochaW = w; plochaH = hh; plochaNaSirku = naSirku;

        try {
            if (prestavet) {
                plochaZOrderNahore = chceNahore;
                if (ps1Plocha != null) ps1GlDisable();
                ps1GlEnable();                      // rozmery si vezme z plochaL..H
                if (ps1Plocha != null) ps1Plocha.setVisibility(View.VISIBLE);
                appendNativeLog("PLOCHA_POSTAVENA_ZNOVU " + l + "," + t + " " + w + "x" + hh
                        + (naSirku ? " (na sirku, pod strankou)" : " (na vysku, nad strankou)"));
                return;
            }
            FrameLayout.LayoutParams lp = new FrameLayout.LayoutParams(w, hh);
            lp.leftMargin = l; lp.topMargin = t;
            ps1Plocha.setLayoutParams(lp);
            ps1Plocha.requestLayout();
            if (ps1Plocha.getVisibility() != View.VISIBLE) ps1Plocha.setVisibility(View.VISIBLE);
            appendNativeLog("PLOCHA_UMISTENA " + l + "," + t + " " + w + "x" + hh);
        } catch (Throwable e) {
            appendNativeLog("PLOCHA_UMISTENA_CHYBA " + safeMsg(e));
        }
    }

    /** PLOCHA, NA KTEROU JADRO KRESLI PRIMO - bez JPEG, bez base64.
     *  Prevzato z overene cesty v eglrender (jadro -> pixely -> GL textura ->
     *  obrazovka). Jediny rozdil: plochu dostane z aplikace, ne ze
     *  samostatneho okna. Lezi POD webovou strankou, takze ovladac zustava
     *  nad obrazem. Ma vlastni EGL kontext - nic se s jadrem nesdili, prave
     *  sdileni bylo to, co nikdy nefungovalo. */
    private void ps1GlEnable() {
        try {
            if (ps1Plocha != null || rootFrame == null) return;
            android.view.SurfaceView sv = new android.view.SurfaceView(MainActivity.this);
            sv.setClickable(false);
            sv.setEnabled(false);
            sv.setFocusable(false);
            // Na vysku musi byt plocha NAD strankou, jinak ji zakryje
            // neprusvitna grafika konzole. Nastavuje se PRED vytvorenim.
            // Na VYSKU musi byt plocha NAD strankou (grafika konzole je
            // neprusvitna), na SIRKU pod ni (ovladac ma byt nad obrazem).
            // Prosakovani do zbytku aplikace resi schovavani nize.
            sv.setZOrderOnTop(plochaZOrderNahore);
            // Dokud nevime, KAM plocha patri, at ji neni videt - jinak by
            // pres celou obrazovku prekryla skrin konzole.
            if (plochaW <= 0) sv.setVisibility(View.INVISIBLE);
            final android.view.SurfaceView tato = sv;
            sv.getHolder().addCallback(new android.view.SurfaceHolder.Callback() {
                @Override public void surfaceCreated(android.view.SurfaceHolder h) {
                    if (ps1Plocha != tato) return;      // hlaseni od stare plochy
                    appendNativeLog("PLOCHA_VYTVORENA - predavam ji jadru");
                    NativePs1CoreBridge.setDisplaySurfaceSafe(h.getSurface());
                }
                @Override public void surfaceChanged(android.view.SurfaceHolder h, int f, int w, int hh) {
                    // Znovu NEPRIPOJOVAT. Pri otoceni prisly surfaceCreated
                    // i surfaceChanged tesne po sobe, kreslici vlakna se
                    // prekotne stridala a po otoceni uz nekreslilo nic.
                    appendNativeLog("PLOCHA_ZMENENA " + w + "x" + hh + " (kresli dal)");
                }
                @Override public void surfaceDestroyed(android.view.SurfaceHolder h) {
                    // Kdyz se plocha prestavuje, prijde tohle hlaseni od STARE
                    // plochy az POTOM, co uz bezi nova - a odpojilo by ji.
                    if (ps1Plocha != null && ps1Plocha != tato) {
                        appendNativeLog("PLOCHA_ZRUSENA (stara, nova uz bezi - neodpojuji)");
                        return;
                    }
                    appendNativeLog("PLOCHA_ZRUSENA");
                    NativePs1CoreBridge.setDisplaySurfaceSafe(null);
                }
            });
            FrameLayout.LayoutParams lp;
            if (plochaW > 0) {
                lp = new FrameLayout.LayoutParams(plochaW, plochaH);
                lp.leftMargin = plochaL; lp.topMargin = plochaT;
            } else {
                lp = new FrameLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
                                                 ViewGroup.LayoutParams.MATCH_PARENT);
            }
            rootFrame.addView(sv, 0, lp);
            ps1Plocha = sv;
            plochaHlidacStart();
            appendNativeLog("PS1_OBRAZ_PRIMO_ZAPNUT (bez JPEG)");
        } catch (Throwable t) {
            appendNativeLog("PS1_OBRAZ_PRIMO_CHYBA " + safeMsg(t));
        }
    }

    // Vypnout nas obraz (pri ukonceni hry / odchodu z aplikace).
    private void ps1GlDisable() {
        final android.view.SurfaceView old = ps1Plocha;
        ps1Plocha = null;
        Runnable r = () -> {
            try {
                NativePs1CoreBridge.setDisplaySurfaceSafe(null);
                if (old != null && old.getParent() instanceof ViewGroup)
                    ((ViewGroup) old.getParent()).removeView(old);
                plochaHlidacStop();
                appendNativeLog("PS1_OBRAZ_PRIMO_VYPNUT");
            } catch (Throwable ignored) {}
        };
        if (isUiThread()) r.run(); else ui.post(r);
    }

    // Tlacitko na logu NaP: kdyz obraz bezi, prepina POMER STRAN
    // 4:3 <-> 16:9 (pro televizi). Kdyz nebezi, zapne ho.
    private boolean ps1Wide = false;
    private void togglePs1Gl() {
        if (ps1Plocha == null) { ps1GlEnable(); return; }
        ps1Wide = !ps1Wide;
        // Pomer stran resi nativni plocha sama podle rozmeru snimku.
        // Prepinac zustava, at je co ohlasit; realny ucinek prijde spolu
        // s dalsim krokem (vyrez 16:9 primo v kreslici smycce).
        appendNativeLog("E pomer stran: " + (ps1Wide ? "16:9 (siroky)" : "4:3 (puvodni PS1)"));
        try {
            android.widget.Toast.makeText(MainActivity.this,
                ps1Wide ? "16:9" : "4:3", android.widget.Toast.LENGTH_SHORT).show();
        } catch (Throwable ignored) {}
    }

    // Most pro pripadne spousteni z menu:  AHRENDER.togglePs1()
    public class AHRENDER {
        /** Vykonnostni trida: 0=LOW (stara zarizeni), 1=MEDIUM, 2=HIGH (moderni).
         *  Volat z menu:  AHRENDER.setPerf(1)   Projevi se pri dalsim spusteni hry. */
        @JavascriptInterface
        public String setPerf(int t) {
            try { setPerfTier(t); return "PERF_OK " + t; }
            catch (Throwable e) { return "PERF_FAIL " + e.getMessage(); }
        }

        /** Zpet na automaticke urceni tridy. Volat z menu: AHRENDER.clearPerf() */
        @JavascriptInterface
        public String clearPerf() {
            try {
                napPerfTier = -1;
                getSharedPreferences("nap_perf", MODE_PRIVATE).edit().remove("tier").apply();
                appendNativeLog("L vykonnostni trida: zpet na AUTO");
                return "PERF_AUTO";
            } catch (Throwable e) { return "PERF_FAIL " + e.getMessage(); }
        }

        /** Vrati aktualni tridu (0/1/2). Volat z menu:  AHRENDER.getPerf() */
        @JavascriptInterface
        public int getPerf() {
            try { return perfTier(); } catch (Throwable e) { return 0; }
        }

        @JavascriptInterface
        public String togglePs1() {
            try {
                ui.post(() -> togglePs1Gl());
                return "AHRENDER_TOGGLE_OK";
            } catch (Throwable t) {
                return "AHRENDER_TOGGLE_FAIL " + t.getMessage();
            }
        }
    }

    private class NativeInPlaceView extends TextureView implements TextureView.SurfaceTextureListener {
        private final Paint paint = new Paint();
        private int frame = 0;
        private static final int SRC_W = 320;
        private static final int SRC_H = 224;
        private int[] argb = new int[SRC_W * SRC_H];
        private Bitmap bitmap;
        private volatile boolean running = false;
        private Thread renderThread;

        NativeInPlaceView(Activity a) {
            super(a);
            try { setOpaque(true); } catch (Throwable ignored) {}
            try { paint.setFilterBitmap(false); paint.setDither(false); } catch (Throwable ignored) {}
            setSurfaceTextureListener(this);
        }

        void start() {
            running = true;
            startRenderThreadIfReady("start");
        }

        void stop() {
            running = false;
            Thread t = renderThread;
            renderThread = null;
            if (t != null && t.isAlive() && Thread.currentThread() != t) {
                try { t.interrupt(); } catch (Throwable ignored) {}
                try { t.join(220); } catch (Throwable ignored) {}
            }
        }

        void forceRedrawOnce() {
            if (!running) running = true;
            startRenderThreadIfReady("force");
        }

        private void startRenderThreadIfReady(String reason) {
            if (!isAvailable()) return;
            Thread t = renderThread;
            if (t != null && t.isAlive()) return;
            renderThread = new Thread(() -> renderLoop(reason), "AtariHelpSegaTextureRG");
            renderThread.setDaemon(true);
            try { renderThread.setPriority(Build.VERSION.SDK_INT <= 28 ? Thread.NORM_PRIORITY : Thread.NORM_PRIORITY + 1); } catch (Throwable ignored) {}
            renderThread.start();
            appendNativeLog("NATIVE_TEXTURE_THREAD_START_RV reason=" + reason + " view=" + getWidth() + "x" + getHeight());
        }

        private void renderLoop(String reason) {
            long next = System.nanoTime();
            int noSurfaceLoops = 0;
            while (running) {
                if (!isAvailable()) {
                    if (++noSurfaceLoops > 20) break;
                    try { Thread.sleep(8); } catch (Throwable ignored) {}
                    continue;
                }
                noSurfaceLoops = 0;
                long startNs = System.nanoTime();
                Canvas c = null;
                try {
                    c = lockCanvas();
                    if (c != null) drawTextureFrame(c);
                } catch (Throwable t) {
                    appendNativeLog("NATIVE_TEXTURE_RENDER_ERROR_RV " + safeMsg(t));
                } finally {
                    try { if (c != null) unlockCanvasAndPost(c); } catch (Throwable t) { appendNativeLog("NATIVE_TEXTURE_UNLOCK_ERROR_RV " + safeMsg(t)); }
                }
                long cost = System.nanoTime() - startNs;
                nativeLastRenderCostNs = cost;
                // BUILD2RV: SBIRKA performance switch. HIGH keeps full-speed render for Nox/new phones.
                // LOW intentionally lowers only the render presentation pace on S8/A12; core/audio timing stays real.
                boolean lowMode = "LOW".equals(nativePerformanceMode);
                long period = lowMode ? 33333333L : 16666667L;
                if (lowMode && Build.VERSION.SDK_INT <= 28 && cost > 22000000L) period = 40000000L;
                next += period;
                long wait = next - System.nanoTime();
                if (wait > 1000000L) {
                    try { Thread.sleep(wait / 1000000L, (int)(wait % 1000000L)); } catch (Throwable ignored) {}
                } else if (wait < -period) {
                    next = System.nanoTime();
                    try { Thread.yield(); } catch (Throwable ignored) {}
                }
            }
            appendNativeLog("NATIVE_TEXTURE_THREAD_STOP_RV reason=" + reason + " drawCounter=" + nativeViewDrawCounter + " lastCostMs=" + (nativeLastRenderCostNs / 1000000.0));
        }

        private void drawTextureFrame(Canvas canvas) {
            int w = Math.max(16, getWidth());
            int h = Math.max(16, getHeight());
            if (bitmap == null || bitmap.getWidth() != SRC_W || bitmap.getHeight() != SRC_H) {
                argb = new int[SRC_W * SRC_H];
                bitmap = Bitmap.createBitmap(SRC_W, SRC_H, Bitmap.Config.ARGB_8888);
            }
            try {
                NativeSegaCoreBridge.renderPattern(SRC_W, SRC_H, frame++, argb);
                bitmap.setPixels(argb, 0, SRC_W, 0, 0, SRC_W, SRC_H);
                canvas.drawColor(Color.BLACK);
                Rect dst;
                if (nativeLandscapeFullVideo) {
                    // BUILD2RV: never crop the 320x224 game image. User reported Sonic lives/HUD cut off in landscape.
                    // Use aspect-fit with a small bottom/top safety margin; black side bars are acceptable, missing HUD is not.
                    float safeW = (float) w;
                    float safeH = (float) h * 0.94f;
                    float scale = Math.min(safeW / (float) SRC_W, safeH / (float) SRC_H);
                    int dw = Math.max(1, Math.round(SRC_W * scale));
                    int dh = Math.max(1, Math.round(SRC_H * scale));
                    int dx = Math.max(0, (w - dw) / 2);
                    int dy = Math.max(0, (h - dh) / 2);
                    dst = new Rect(dx, dy, dx + dw, dy + dh);
                } else {
                    dst = new Rect(0, 0, w, h);
                }
                Rect src = new Rect(0, 0, SRC_W, SRC_H); // BUILD2RV: left-edge is fixed in C++ scanline boundary mapping, not by Java crop.
                canvas.drawBitmap(bitmap, src, dst, paint);
                nativeViewDrawCounter++;
                long nowMs = System.currentTimeMillis();
                if (nativeRenderPerfWindowStartMs == 0) nativeRenderPerfWindowStartMs = nowMs;
                nativeRenderPerfWindowFrames++;
                if (nativeLastRenderCostNs > 22000000L) nativeRenderPerfSlowFrames++;
                if (nowMs - nativeRenderPerfWindowStartMs >= 5000) {
                    appendNativeLog("NATIVE_RENDER_PERF_RV frames5s=" + nativeRenderPerfWindowFrames + " slow22ms=" + nativeRenderPerfSlowFrames + " lastCostMs=" + (nativeLastRenderCostNs / 1000000.0) + " android=" + Build.VERSION.SDK_INT + " perfMode=" + nativePerformanceMode + " landscapeFull=" + nativeLandscapeFullVideo);
                    nativeRenderPerfWindowStartMs = nowMs;
                    nativeRenderPerfWindowFrames = 0;
                    nativeRenderPerfSlowFrames = 0;
                }
                if (nativeViewDrawCounter <= 4 || nativeViewDrawCounter % 300 == 0) {
                    appendNativeLog("NATIVE_TEXTURE_FRAME_RV count=" + nativeViewDrawCounter + " view=" + w + "x" + h + " gameDst=" + dst.toShortString() + " srcCrop=0px leftBoundaryShift=YES perfMode=" + nativePerformanceMode + " landscapeFull=" + nativeLandscapeFullVideo + " costMs=" + (nativeLastRenderCostNs / 1000000.0));
                }
            } catch (Throwable t) {
                try { canvas.drawColor(Color.rgb(20, 0, 0)); } catch (Throwable ignored) {}
                appendNativeLog("NATIVE_TEXTURE_FRAME_ERROR_RV " + safeMsg(t));
            }
        }

        @Override public void onSurfaceTextureAvailable(SurfaceTexture surface, int width, int height) { startRenderThreadIfReady("surfaceAvailable"); }
        @Override public void onSurfaceTextureSizeChanged(SurfaceTexture surface, int width, int height) { forceRedrawOnce(); }
        @Override public boolean onSurfaceTextureDestroyed(SurfaceTexture surface) { stop(); return true; }
        @Override public void onSurfaceTextureUpdated(SurfaceTexture surface) { }
    }

    // BUILD2SK87: PS1 protejsek NativeInPlaceView (vyse) - grabne snimek PRIMO
    // z jadra (NativePs1CoreBridge.grabFrameSafe, uz drive overene v
    // tvRenderLoop) a nakresli ho na Surface teto TextureView. ZADNY JPEG,
    // ZADNY base64, ZADNY WebView round-trip - presne to, co puvodni
    // ps1FramePreviewB64() (SK27, pak SK85/86) nemohlo nikdy dosahnout, protoze
    // JPEG komprese samotna stoji ~50ms/snimek na tomhle hardware bez ohledu
    // na frekvenci volani.
    //
    // Na rozdil od Segy nema PS1 pevne SRC_W/SRC_H - rozliseni jadra se muze
    // behem hry menit (v logu videno 256x240, 320x240, 640x480) - buffer/
    // bitmapa se prealokuje znovu jen kdyz se rozliseni skutecne zmeni.
    //
    // ROZSAH TETO PRVNI VERZE: AKTIVNI JEN V LANDSCAPE. Portret ma vlastni
    // "skin" dekoraci (viz emu_ps1/index.html .stage/.skin, procentualni
    // pozicovani #psMonitor) - presne umisteni by chtelo bud replikovat tuhle
    // matematiku, nebo (jako Sega) pridat JS->Java rect-reporting bridge
    // (setNativeRect ekvivalent) - ANI JEDNU cestu jsem nemel jak overit bez
    // rizika, ze se portret rozbije, a tenhle balicek uz obsahuje dost
    // velkych zmen najednou. Landscape 16:9-stred-fit matematika nize je
    // PRESNA kopie CSS #psMonitor pravidla (@media orientation:landscape) -
    // video tak sedi presne tam, kde je dotykove tlacitko. Portret zustava na
    // puvodnim JPEG preview (viz orientation guard v emu_ps1/index.html),
    // beze zmeny chovani/rizika.

    public class AHNet {
        @JavascriptInterface
        public void openGames() {
            // BUILD2SA5J: AtariHelp sbirka musi zustat uvnitr WebView.
            // Jen tak muze klik na ZIP/XEX/GEN projit pres AHNET.runGameUrl() a rovnou spustit emu.
            ui.post(() -> {
                showAtariNetGamesBridge();
            });
        }
        @JavascriptInterface
        public void openGamesWeb() {
            ui.post(() -> loadAtariHelpGuarded("https://atarihelp.eu/?page_id=21", "openGamesWeb"));
        }
        @JavascriptInterface
        public void openPs1Games() {
            ui.post(() -> loadAtariHelpGuarded(PS1_GOOGLE_GAMES_URL, "openPs1Games"));
        }
        @JavascriptInterface
        public void openInBrowser(String url) {
            ui.post(() -> openRawExternalBrowserUrl(url));
        }
        @JavascriptInterface
        public void runGameUrl(String url) {
            ui.post(() -> {
                if (url == null || url.length() == 0) return;
                routeGameDownloadUrl(url, "AHNET_RUN_GAME_URL");
            });
        }
    }


    private void addAtariNetGame(StringBuilder sb, String title, String zipUrl) {
        String safeTitle = escapeHtml(title == null ? "Atari XEX" : title);
        String bridgeUrl = bridgeGameHref(zipUrl);
        String safeUrl = escapeHtml(bridgeUrl);
        String jsUrl = jsQuote(zipUrl == null ? "" : zipUrl);
        sb.append("<a class='game' href='").append(safeUrl).append("' onclick=\"try{AHNET.runGameUrl(")
                .append(jsUrl).append(");return false;}catch(e){return true;}\">")
                .append(safeTitle).append("</a>");
    }

    private String bridgeGameHref(String url) {
        try {
            return "ahgame://run?url=" + java.net.URLEncoder.encode(url == null ? "" : url, "UTF-8");
        } catch (Throwable ignored) {
            return "ahgame://run?url=";
        }
    }

    private boolean isAhGameBridgeUrl(String url) {
        if (url == null) return false;
        String u = url.trim().toLowerCase(Locale.US);
        return u.startsWith("ahgame://");
    }

    private String ahGameBridgeTarget(String url) {
        try {
            Uri u = Uri.parse(url);
            String target = u.getQueryParameter("url");
            return target == null ? "" : target.trim();
        } catch (Throwable ignored) {
            return "";
        }
    }

    private void routeGameDownloadUrl(String url, String source) {
        if (url == null || url.trim().length() == 0) {
            appendNativeLog("BUILD2SA5AH ROUTE_GAME_SKIP_EMPTY source=" + source);
            return;
        }
        String target = url.trim();
        appendNativeLog("BUILD2SA5AQ ROUTE_GAME source=" + source + " url=" + compactUrl(target));
        if (shouldRouteAsSegaDownload(target)) downloadAndRunSegaArchive(target); // BUILD2SA5AQ: Sega ZIP ma prednost pred PS1 ZIP.
        else if (shouldRouteAsPs1Download(target, source)) downloadAndRunPs1Remote(target, source);
        else if (hasSegaExtension(target)) downloadAndRunSega(target); // BUILD2SA2
        else downloadAndRun(target);
    }

    private boolean handleAhGameBridgeUrl(String url) {
        if (!isAhGameBridgeUrl(url)) return false;
        final String target = ahGameBridgeTarget(url);
        long now = System.currentTimeMillis();
        if (target.length() > 0 && target.equals(lastAhGameBridgeUrl) && now - lastAhGameBridgeAtMs < 1200L) {
            appendNativeLog("BUILD2SA5AH AHGAME_DUPLICATE_IGNORED url=" + compactUrl(target));
            return true;
        }
        lastAhGameBridgeUrl = target;
        lastAhGameBridgeAtMs = now;
        appendNativeLog("BUILD2SA5AH AHGAME_BRIDGE url=" + compactUrl(target));
        ui.post(() -> routeGameDownloadUrl(target, "AHGAME_BRIDGE"));
        return true;
    }

    private WebResourceResponse interceptAhGameBridgeNavigation(String url) {
        if (!isAhGameBridgeUrl(url)) return null;
        handleAhGameBridgeUrl(url);
        return htmlResponse("<!doctype html><html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'>"
                + "<style>body{font-family:sans-serif;background:#fff;color:#111;padding:24px;line-height:1.4}</style></head>"
                + "<body><h1>Spoustim hru...</h1></body></html>");
    }

    private void showAtariNetGamesBridge() {
        if (web == null) return;
        StringBuilder sb = new StringBuilder(12000);
        sb.append("<!doctype html><html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'>");
        sb.append("<style>");
        sb.append("body{margin:0;background:#050505;color:#f4ead2;font-family:Arial,sans-serif;padding:18px 14px 28px}");
        sb.append("h1{font-size:22px;margin:6px 0 14px;text-align:center;letter-spacing:.04em}");
        sb.append(".bar{display:flex;gap:8px;justify-content:center;margin:0 0 14px;flex-wrap:wrap}");
        sb.append(".cmd{border:1px solid #9e854c;color:#f4ead2;background:#14120c;border-radius:6px;padding:9px 12px;text-decoration:none;font-weight:700}");
        sb.append(".grid{display:grid;grid-template-columns:1fr;gap:9px;max-width:620px;margin:0 auto}");
        sb.append(".game{display:block;border:1px solid #8b743f;background:linear-gradient(180deg,#201b12,#0d0c09);color:#fff0c0;text-decoration:none;border-radius:6px;padding:13px 14px;font-size:17px;font-weight:700;box-shadow:inset 0 0 0 1px rgba(255,255,255,.05)}");
        sb.append(".game:active{background:#3a2f16;color:#fff}");
        sb.append("</style></head><body>");
        sb.append("<h1>Atari XEX</h1>");
        sb.append("<div class='bar'><a class='cmd' href='#' onclick='try{AHNET.openGamesWeb();}catch(e){}return false;'>WEB</a></div>");
        sb.append("<div class='grid'>");
        addAtariNetGame(sb, "Atari Galactic Chase XEX", "https://atarihelp.eu/wp-content/uploads/2026/06/galactic_chase.zip");
        addAtariNetGame(sb, "Atari Moon Patrol XEX", "https://atarihelp.eu/wp-content/uploads/2026/06/moon_patrol.zip");
        addAtariNetGame(sb, "Atari Pac Man XEX", "https://atarihelp.eu/wp-content/uploads/2026/06/de_re_pac-man.zip");
        addAtariNetGame(sb, "Atari Donkey Kong XEX", "https://atarihelp.eu/wp-content/uploads/2026/06/donkey_kong.zip");
        addAtariNetGame(sb, "Atari Death Race XEX", "https://atarihelp.eu/wp-content/uploads/2026/06/death_race.zip");
        addAtariNetGame(sb, "Atari Super Cobra XEX", "https://atarihelp.eu/wp-content/uploads/2026/06/Super-Cobra.zip");
        addAtariNetGame(sb, "Atari Pitstop 2 XEX", "https://atarihelp.eu/wp-content/uploads/2026/06/pitstop_ii.zip");
        addAtariNetGame(sb, "Atari River Raid XEX", "https://atarihelp.eu/wp-content/uploads/2026/06/River-Raid.zip");
        addAtariNetGame(sb, "Atari NaP Pitt-Kitt XEX", "https://atarihelp.eu/wp-content/uploads/2026/06/PiTT-KiTT-NaP-Final.zip");
        addAtariNetGame(sb, "Atari Bugi Bugi 3 XEX", "https://atarihelp.eu/wp-content/uploads/2026/06/bugi_bugi_3.zip");
        addAtariNetGame(sb, "Atari International Karate XEX", "https://atarihelp.eu/wp-content/uploads/2026/06/international_karate_enhanced_version_2014.zip");
        addAtariNetGame(sb, "Atari Tennis XEX", "https://atarihelp.eu/wp-content/uploads/2026/06/atari_tennis.zip");
        addAtariNetGame(sb, "Atari Bruce Lee XEX", "https://atarihelp.eu/wp-content/uploads/2026/06/Bruce-Lee.zip");
        addAtariNetGame(sb, "Atari Alley Cat XEX", "https://atarihelp.eu/wp-content/uploads/2026/06/Alley-Cat.zip");
        addAtariNetGame(sb, "Atari Bruce Lee's Return XEX", "https://atarihelp.eu/wp-content/uploads/2026/06/Bruce-Lees-Return.zip");
        addAtariNetGame(sb, "Atari Ghostbusters XEX", "https://atarihelp.eu/wp-content/uploads/2026/06/Ghostbusters.zip");
        addAtariNetGame(sb, "Atari Dawn Raider XEX", "https://atarihelp.eu/wp-content/uploads/2026/06/dawn_raider.zip");
        addAtariNetGame(sb, "Atari Mario Bros XEX", "https://atarihelp.eu/wp-content/uploads/2026/06/mario_bros._xe_arcade.zip");
        addAtariNetGame(sb, "Atari Montezuma's Revenge XEX", "https://atarihelp.eu/wp-content/uploads/2026/06/Montezumas-Revenge-.zip");
        addAtariNetGame(sb, "Atari Shaft Raider XEX", "https://atarihelp.eu/wp-content/uploads/2026/06/shaft_raider.zip");
        addAtariNetGame(sb, "Atari Sidewinder XEX", "https://atarihelp.eu/wp-content/uploads/2026/06/sidewinder.zip");
        addAtariNetGame(sb, "Atari Vanguard XEX", "https://atarihelp.eu/wp-content/uploads/2026/06/Vanguard.zip");
        addAtariNetGame(sb, "Atari Cross-Country Road Race XEX", "https://atarihelp.eu/wp-content/uploads/2026/06/The-Great-American-Cross-Country-Road-Race.zip");
        addAtariNetGame(sb, "Atari Activision Decathlon XEX", "https://atarihelp.eu/wp-content/uploads/2026/06/The-Activision-Decathlon.zip");
        addAtariNetGame(sb, "Atari Popeye Arcade XEX", "https://atarihelp.eu/wp-content/uploads/2026/06/Popeye-Arcade-Version.zip");
        addAtariNetGame(sb, "Atari Ninja Title XEX", "https://atarihelp.eu/wp-content/uploads/2026/06/Ninja-Title-Version.zip");
        addAtariNetGame(sb, "Atari Patent Pole Position XEX", "https://atarihelp.eu/wp-content/uploads/2026/06/Patent-Pole-Position.zip");
        addAtariNetGame(sb, "Atari Bros XEX", "https://atarihelp.eu/wp-content/uploads/2026/06/Bros.zip");
        addAtariNetGame(sb, "Atari Arkanoid XEX", "https://atarihelp.eu/wp-content/uploads/2026/06/Arkanoid.zip");
        addAtariNetGame(sb, "Atari Star Wars XEX", "https://atarihelp.eu/wp-content/uploads/2026/06/Starwars.zip");
        sb.append("</div></body></html>");
        try {
            applyWebViewVisualMode("file:///android_asset/atari_xex_bridge.html", "atariNetBridge");
            web.loadDataWithBaseURL("file:///android_asset/atari_xex_bridge.html", sb.toString(), "text/html", "UTF-8", null);
            appendNativeLog("BUILD2SA5AF ATARI_NET_BRIDGE_OPEN protectedWeb=ON games=30");
        } catch (Throwable t) {
            appendNativeLog("BUILD2SA5AF ATARI_NET_BRIDGE_FAIL " + safeMsg(t));
        }
    }

    private boolean isYoutubeUrl(String url) {
        if (url == null) return false;
        String u = url.trim().toLowerCase(Locale.US);
        if (!(u.startsWith("http://") || u.startsWith("https://"))) return false;
        try {
            Uri uri = Uri.parse(url);
            String host = uri.getHost();
            if (host == null) return false;
            host = host.toLowerCase(Locale.US);
            return host.equals("youtube.com")
                    || host.equals("www.youtube.com")
                    || host.equals("m.youtube.com")
                    || host.endsWith(".youtube.com")
                    || host.equals("youtu.be")
                    || host.endsWith(".youtu.be");
        } catch (Throwable ignored) {
            return u.contains("youtube.com") || u.contains("youtu.be");
        }
    }

    // BUILD2GH: normalni WWW odkazy nesmi spadnout do emulator NET loaderu.
    // BUILD2SA13C13: YouTube zustava v appce, aby ho TV WEB CAST videl.
    // Facebook stale jde ven, aby nespadl do game/download routeru.
    private boolean isExternalBrowserUrl(String url) {
        if (url == null) return false;
        String u = url.trim().toLowerCase();
        if (!(u.startsWith("http://") || u.startsWith("https://"))) return false;
        try {
            Uri uri = Uri.parse(url);
            String host = uri.getHost();
            if (host == null) return false;
            host = host.toLowerCase();

            if (isYoutubeUrl(url)) return false;
            if (host.equals("facebook.com") || host.equals("www.facebook.com") || host.endsWith(".facebook.com")) return true;
        } catch (Throwable ignored) {}
        return false;
    }

    private boolean openRawExternalBrowserUrl(String url) {
        if (url == null || url.trim().length() == 0) return false;
        if (isProviderBlockedUrl(url)) {
            loadAtariHelpGuarded(url, "openBrowserRelay");
            return true;
        }
        try {
            Intent i = new Intent(Intent.ACTION_VIEW, Uri.parse(url));
            i.addCategory(Intent.CATEGORY_BROWSABLE);
            i.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            startActivity(i);
            return true;
        } catch (Throwable ignored) {
            return false;
        }
    }

    private boolean openExternalBrowserUrl(String url) {
        if (!isExternalBrowserUrl(url)) return false;
        try { openRawExternalBrowserUrl(url); } catch (Throwable ignored) {
            // DULEZITE: i kdyz by browser Intent selhal,
            // nevracet false, jinak by to sebral emulator loader.
        }
        return true;
    }

    private boolean hasGameExtension(String value) {
        if (value == null) return false;
        String v = value.toLowerCase();
        int q = v.indexOf('?'); if (q >= 0) v = v.substring(0, q);
        int h = v.indexOf('#'); if (h >= 0) v = v.substring(0, h);
        return v.endsWith(".zip") || v.endsWith(".xex") || v.endsWith(".atr")
                || v.endsWith(".com") || v.endsWith(".exe");
    }


    // ===================================================================
    //  KLAVESNICE A JOYSTICK - NEJRYCHLEJSI CESTA STISKU
    //
    //  Dotyk na displeji jde pres WebView, ktery si udalosti drzi ve
    //  vlastni fronte. Klavesnice a joystick jdou naopak PRIMO sem, takze
    //  se bit preda do jadra hned - zadne mezikroky.
    //
    //  Cisla tlacitek jsou RetroPad, stejne jako u dotyku:
    //    0=X(kriz)  1=O(kolecko)  2=ctverec  3=trojuhelnik
    //    4=nahoru 5=dolu 6=vlevo 7=vpravo
    //    8=SELECT 9=START  10=L1 11=R1  12=L2 13=R2
    // ===================================================================
    private static int napTlacitkoPodleKlavesy(int kod) {
        switch (kod) {
            // --- joystick / gamepad (Xbox, PS, 8BitDo a dalsi) ---
            case KeyEvent.KEYCODE_BUTTON_A:      return 0;   // Xbox A -> kriz
            case KeyEvent.KEYCODE_BUTTON_B:      return 1;   // Xbox B -> kolecko
            case KeyEvent.KEYCODE_BUTTON_X:      return 2;   // Xbox X -> ctverec
            case KeyEvent.KEYCODE_BUTTON_Y:      return 3;   // Xbox Y -> trojuhelnik
            case KeyEvent.KEYCODE_DPAD_UP:       return 4;
            case KeyEvent.KEYCODE_DPAD_DOWN:     return 5;
            case KeyEvent.KEYCODE_DPAD_LEFT:     return 6;
            case KeyEvent.KEYCODE_DPAD_RIGHT:    return 7;
            case KeyEvent.KEYCODE_BUTTON_SELECT: return 8;
            case KeyEvent.KEYCODE_BUTTON_MODE:   return 8;   // nektere padi
            case KeyEvent.KEYCODE_BUTTON_START:  return 9;
            case KeyEvent.KEYCODE_BUTTON_L1:     return 10;
            case KeyEvent.KEYCODE_BUTTON_R1:     return 11;
            case KeyEvent.KEYCODE_BUTTON_L2:     return 12;
            case KeyEvent.KEYCODE_BUTTON_R2:     return 13;

            // --- klavesnice (i bluetooth) ---
            case KeyEvent.KEYCODE_DPAD_CENTER:
            case KeyEvent.KEYCODE_SPACE:         return 0;   // kriz
            case KeyEvent.KEYCODE_ENTER:         return 9;   // START
            case KeyEvent.KEYCODE_SHIFT_RIGHT:   return 8;   // SELECT
            case KeyEvent.KEYCODE_W:             return 4;
            case KeyEvent.KEYCODE_S:             return 5;
            case KeyEvent.KEYCODE_A:             return 6;
            case KeyEvent.KEYCODE_D:             return 7;
            case KeyEvent.KEYCODE_J:             return 0;   // kriz
            case KeyEvent.KEYCODE_K:             return 1;   // kolecko
            case KeyEvent.KEYCODE_H:             return 2;   // ctverec
            case KeyEvent.KEYCODE_U:             return 3;   // trojuhelnik
            case KeyEvent.KEYCODE_Q:             return 10;  // L1
            case KeyEvent.KEYCODE_E:             return 11;  // R1
            case KeyEvent.KEYCODE_1:             return 12;  // L2
            case KeyEvent.KEYCODE_3:             return 13;  // R2
            default: return -1;
        }
    }

    private boolean napPs1Ovladani() {
        return ps1BiosRunning || ps1SessionActive;
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (napPs1Ovladani() && event.getRepeatCount() == 0) {
            int t = napTlacitkoPodleKlavesy(keyCode);
            if (t >= 0) {
                NativePs1CoreBridge.setButtonSafe(t, true);
                return true;                 // spotrebovano, dal neposilat
            }
        }
        return super.onKeyDown(keyCode, event);
    }

    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event) {
        if (napPs1Ovladani()) {
            int t = napTlacitkoPodleKlavesy(keyCode);
            if (t >= 0) {
                NativePs1CoreBridge.setButtonSafe(t, false);
                return true;
            }
        }
        return super.onKeyUp(keyCode, event);
    }

    /** Analogova packa a smerovy krizek joysticku. */
    @Override
    public boolean onGenericMotionEvent(MotionEvent event) {
        try {
            if (napPs1Ovladani()
                    && (event.getSource() & InputDevice.SOURCE_JOYSTICK) == InputDevice.SOURCE_JOYSTICK
                    && event.getAction() == MotionEvent.ACTION_MOVE) {
                float x = event.getAxisValue(MotionEvent.AXIS_X);
                float y = event.getAxisValue(MotionEvent.AXIS_Y);
                // Smerovy krizek na joysticku chodi jako HAT osa - pripocteme.
                float hx = event.getAxisValue(MotionEvent.AXIS_HAT_X);
                float hy = event.getAxisValue(MotionEvent.AXIS_HAT_Y);
                if (Math.abs(hx) > 0.5f) x = hx;
                if (Math.abs(hy) > 0.5f) y = hy;
                NativePs1CoreBridge.setStickSafe((int) (x * 32767), (int) (y * 32767));
                return true;
            }
        } catch (Throwable ignored) {}
        return super.onGenericMotionEvent(event);
    }

    @SuppressLint({"SetJavaScriptEnabled", "AddJavascriptInterface"})
    @Override

    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        // BUILD2SK82: prvni vec ze vseho - zalozit cerstvy log soubor pro TUHLE
        // relaci a nastartovat vlakno, co ho prubezne plni (viz appendNativeLog).
        // Musi byt PRED vsim ostatnim, aby /log opravdu zachytil "od zacatku".
        // APPKA NIKDY NESMI USNOUT. Drive se displej drzel vzhuru jen po dobu
        // TV castu; ted platí vzdycky - hrajes/koukas a telefon nesmi zhasnout.
        try { getWindow().addFlags(android.view.WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON); } catch (Throwable ignored) {}
        napTvWebLogFileInit();
        // VERZE AUTOMATICKY Z BUILDU - nesmi se rozejit s tim, co je nainstalovane.
        // (Drive jsem verzi psal do logu rucne v nativnim kodu a zapomnel ji
        // prepsat, takze log hlasil starou verzi a hledala se chyba v necem,
        // co uz davno bylo opravene. Tohle se rozejit nemuze.)
        try {
            android.content.pm.PackageInfo pi =
                getPackageManager().getPackageInfo(getPackageName(), 0);
            appendNativeLog("VERZE APKY = " + pi.versionName + " (code " + pi.versionCode + ")");
        } catch (Throwable t) {
            appendNativeLog("VERZE APKY = nezjistena: " + safeMsg(t));
        }
        // BUILD2SK99: rekni PS1 nativnimu kodu, kam ma zapisovat pohotovostni
        // diagnostiku (napr. gpu-gles/EGL inicializace) - primo do STEJNEHO
        // souboru, co uz napTvWebLogFileInit prave zalozil. Musi byt AZ PO
        // napTvWebLogFileInit (soubor uz musi existovat) a PRED jakymkoli PS1
        // bootem (coz v praxi vzdy je, boot je uzivatelska akce o dost pozdeji).
        try { if (napTvWebLogFile != null) NativePs1CoreBridge.setDiagLogPathSafe(napTvWebLogFile.getAbsolutePath()); } catch (Throwable ignored) {}
        // BUILD2SK18: obnov ulozenou volbu kvality TV mirroru z minula (drive se
        // pri kazdem znovuotevreni appky vracela na LOW - ted si to appka pamatuje).
        try { napTvWebQualityTier = getSharedPreferences("nap_tv_prefs", MODE_PRIVATE).getInt("quality_tier", 0); } catch (Throwable ignored) {}
        napPlayerRequestAudioPermission("startup");
        web = new WebView(this);
        // BUILD2RV: WebView must be transparent in landscape; HTML is controls-only over native C++ video.
        try { web.setBackgroundColor(Color.TRANSPARENT); } catch (Throwable ignored) {}
        try { web.setLayerType(View.LAYER_TYPE_HARDWARE, null); } catch (Throwable ignored) {}
        WebSettings s = web.getSettings();
        s.setJavaScriptEnabled(true);
        s.setDomStorageEnabled(true);
        try { s.setUserAgentString(ATARIHELP_BROWSER_UA); } catch (Throwable ignored) {}
        s.setAllowFileAccess(true);
        s.setAllowUniversalAccessFromFileURLs(true);
        s.setAllowContentAccess(true);
        s.setMediaPlaybackRequiresUserGesture(false);
        try { s.setLoadsImagesAutomatically(true); } catch (Throwable ignored) {}
        try { s.setBlockNetworkImage(false); } catch (Throwable ignored) {}
        try { s.setBlockNetworkLoads(false); } catch (Throwable ignored) {}
        if (Build.VERSION.SDK_INT >= 21) {
            try { s.setMixedContentMode(WebSettings.MIXED_CONTENT_ALWAYS_ALLOW); } catch (Throwable ignored) {}
        }
        web.addJavascriptInterface(new AHSave(), "AHSAVE");
        web.addJavascriptInterface(new AHPick(), "AHPICK");
        web.addJavascriptInterface(new AHPS1(), "AHPS1"); // BUILD2SA1
        tvSetupDisplayListener(); // BUILD2SB1: TV vystup pro celou appku
        web.addJavascriptInterface(new AHNet(), "AHNET");
        web.addJavascriptInterface(new AHNative(), "AHNATIVE");
        web.addJavascriptInterface(new AHRENDER(), "AHRENDER"); // KROK C1: plynuly PS1 obraz pres OpenGL
        web.addJavascriptInterface(new AHTvWeb(), "AHTVWEB"); // BUILD2SA13C: browser-based TV cast fallback
        web.addJavascriptInterface(new AHAtariCpp(), "AHATARICPP"); // BUILD2SA14: jadro Atari v C++ (obrazovka HELP)
        web.addJavascriptInterface(new AHIntro(), "AHINTRO");       // BUILD2SA21: uvodni intro
        web.setWebChromeClient(new WebChromeClient() {
            // BUILD2SH2: zachyt JS console (jen nase SH2 stream diagnostika) do
            // nativniho logu, aby slo poslat log pri ladeni zvuku na projektor.
            @Override
            public boolean onConsoleMessage(android.webkit.ConsoleMessage cm) {
                try {
                    String m = cm.message();
                    if (m != null && (m.contains("SH2") || m.startsWith("[NAP DJ]") || m.startsWith("[NAP PLAYER]"))) {
                        appendNativeLog("JSLOG " + m);
                    }
                } catch (Throwable ignored) {}
                return super.onConsoleMessage(cm);
            }
            @Override
            public boolean onShowFileChooser(WebView v, ValueCallback<Uri[]> cb,
                                             FileChooserParams params) {
                if (pendingChooser != null) pendingChooser.onReceiveValue(null);
                pendingChooser = cb;
                Intent i = new Intent(Intent.ACTION_GET_CONTENT);
                i.addCategory(Intent.CATEGORY_OPENABLE);
                i.setType("*/*");
                startActivityForResult(Intent.createChooser(i, "Vyber XEX / ATR / ZIP"), PICK_FILE);
                return true;
            }
        });
        web.setWebViewClient(new WebViewClient() {
            @Override
            public void onPageStarted(WebView v, String url, Bitmap favicon) {
                super.onPageStarted(v, url, favicon);
                // BUILD2SA14: bez tohohle radku neni v logu videt, KTERE
                // obrazovky Rene pri testu prosel - a pak se nedaji jeho
                // odpovedi opret o nic jineho nez o jeho slovo.
                try { appendNativeLog("BUILD2SA14 OBRAZOVKA url=" + url); } catch (Throwable ignored) {}
                applyWebViewVisualMode(url, "onPageStarted");
                stopNativeIfLeavingSega(url, "onPageStarted");
                stopPs1IfLeaving(url, "onPageStarted");
            }

            @Override
            public boolean shouldOverrideUrlLoading(WebView v, String url) {
                if (handleAhGameBridgeUrl(url)) return true;
                if (openExternalBrowserUrl(url)) return true;
                if (handleMaybeGameUrl(url)) return true;
                if (isProviderBlockedUrl(url)) return false;
                return false;
            }
            @Override
            public boolean shouldOverrideUrlLoading(WebView v, WebResourceRequest request) {
                if (request != null && request.getUrl() != null) {
                    String url = request.getUrl().toString();
                    if (handleAhGameBridgeUrl(url)) return true;
                    if (openExternalBrowserUrl(url)) return true;
                    if (handleMaybeGameUrl(url)) return true;
                    if (isProviderBlockedUrl(url)) return false;
                    return false;
                }
                return false;
            }
            @Override
            public void onPageFinished(WebView v, String url) {
                applyWebViewVisualMode(url, "onPageFinished");
                stopNativeIfLeavingSega(url, "onPageFinished");
                stopPs1IfLeaving(url, "onPageFinished");
                if (isYoutubeUrl(url)) napTvWebScheduleYoutubeAudioBridge("onPageFinished");
                if (pendingGame != null && url != null && url.startsWith(EMU_URL)) {
                    schedulePendingAtariGameInjection("onPageFinished");
                }
                if (url != null && url.toLowerCase().contains("atarihelp.eu")) {
                    injectGameLinkBridge();
                }
            }
            @Override
            public WebResourceResponse shouldInterceptRequest(WebView v, String url) {
                WebResourceResponse bridge = interceptAhGameBridgeNavigation(url);
                if (bridge != null) return bridge;
                WebResourceResponse rr = interceptProviderBlockedResource(url, false);
                return rr != null ? rr : super.shouldInterceptRequest(v, url);
            }
            @Override
            public WebResourceResponse shouldInterceptRequest(WebView v, WebResourceRequest request) {
                if (Build.VERSION.SDK_INT >= 21 && request != null && request.getUrl() != null) {
                    String reqUrl = request.getUrl().toString();
                    if (request.isForMainFrame()) {
                        WebResourceResponse bridge = interceptAhGameBridgeNavigation(reqUrl);
                        if (bridge != null) return bridge;
                        WebResourceResponse game = interceptMainFrameGameNavigation(reqUrl);
                        if (game != null) return game;
                    }
                    WebResourceResponse rr = interceptProviderBlockedResource(reqUrl, request.isForMainFrame());
                    if (rr != null) return rr;
                }
                return super.shouldInterceptRequest(v, request);
            }
            @Override
            public void onReceivedError(WebView v, int errorCode, String description, String failingUrl) {
                super.onReceivedError(v, errorCode, description, failingUrl);
                String current = v == null ? null : v.getUrl();
                if (current == null || current.equals(failingUrl)) {
                    showAtariHelpLoadError(failingUrl, "code=" + errorCode + " " + description);
                }
            }
            @Override
            public void onReceivedError(WebView v, WebResourceRequest request, WebResourceError error) {
                super.onReceivedError(v, request, error);
                if (Build.VERSION.SDK_INT >= 23 && request != null && request.isForMainFrame() && request.getUrl() != null) {
                    CharSequence d = error == null ? "" : error.getDescription();
                    showAtariHelpLoadError(request.getUrl().toString(), String.valueOf(d));
                }
            }
            @Override
            public void onReceivedHttpError(WebView v, WebResourceRequest request, WebResourceResponse errorResponse) {
                super.onReceivedHttpError(v, request, errorResponse);
                if (Build.VERSION.SDK_INT >= 21 && request != null && request.isForMainFrame() && request.getUrl() != null && isProviderBlockedUrl(request.getUrl().toString())) {
                    int code = errorResponse == null ? 0 : errorResponse.getStatusCode();
                    String reason = errorResponse == null ? "" : errorResponse.getReasonPhrase();
                    if (code >= 400) showAtariHelpLoadError(request.getUrl().toString(), "HTTP " + code + " " + reason);
                }
            }
        });
        web.setDownloadListener((url, userAgent, contentDisposition, mimetype, contentLength) -> {
            if (shouldRouteAsPs1Download(url, "downloadListener")) {
                downloadAndRunPs1Remote(url, "downloadListenerPs1");
                return;
            }
            if (openExternalBrowserUrl(url)) return;
            if (isGameUrl(url, contentDisposition, mimetype)) {
                if (shouldRouteAsSegaDownload(url)) downloadAndRunSegaArchive(url); // BUILD2SA5AB
                else if (hasSegaExtension(url)) downloadAndRunSega(url);
                else downloadAndRun(url);
            } else if (isProviderBlockedUrl(url)) {
                loadAtariHelpGuarded(url, "downloadListenerPageRelay");
            } else {
                try {
                    Intent i = new Intent(Intent.ACTION_VIEW, Uri.parse(url));
                    i.addCategory(Intent.CATEGORY_BROWSABLE);
                    startActivity(i);
                } catch (Exception ignored) {}
            }
        });
        rootFrame = new FrameLayout(this);
        // BUILD2SA13: TV vystup - hlidat pripojene displeje po celou dobu behu
        try {
            napDisplayManager = (android.hardware.display.DisplayManager) getSystemService(DISPLAY_SERVICE);
            if (napDisplayManager != null) {
                napDisplayManager.registerDisplayListener(napTvListener, ui);
                ui.postDelayed(() -> napTvUpdatePresentation(), 1200);
            }
        } catch (Throwable t) { appendNativeLog("BUILD2SA13 TV_INIT_ERR " + safeMsg(t)); }
        try { rootFrame.setBackgroundColor(Color.BLACK); } catch (Throwable ignored) {}
        rootFrame.addView(web, new FrameLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));

        // KROK F: testovaci tlacitko na logu ZRUSENO - prekryvalo tlacitko
        // pro vyskoceni z PS1. Pomer stran se ted prepina sam podle otoceni.

        setContentView(rootFrame);
        // BUILD2SA21: uvodni intro pro sraz klanu KKT. Ukaze se jen jednou
        // za spusteni aplikace - po nem uz se jde rovnou do rozcestniku,
        // aby to pri navratu z her neotravovalo.
        {
            // BUILD2SA28: PORADI. Nejdriv otazka na stazeni, AZ POTOM intro.
            //
            // V B137 se otazka objevila PRES uz bezici intro - film hral
            // pod dialogem, dobehl a skocil do menu, aniz uzivatel stihl
            // cokoli odklepnout. Bylo to zmatecne a byla to moje chyba
            // v poradi, ne v te otazce samotne.
            final String menu  = "file:///android_asset/index.html";
            final String intro = "file:///android_asset/intro/index.html";

            // POJISTKA: intro se smi spustit jen jednou, ale MUSI se
            // spustit vzdycky. Kdyby dialog z jakehokoli duvodu nenaskocil,
            // zustala by prazdna obrazovka a uzivatel by se nikam nedostal.
            final boolean[] uzJede = { false };
            final Runnable spustIntro = () -> {
                if (uzJede[0]) return;
                uzJede[0] = true;
                final String cil = napIntroUkazano ? menu : intro;
                applyWebViewVisualMode(cil, "startup");
                appendNativeLog("BUILD2SA28 START url=" + cil
                        + " intro=" + (napIntroUkazano ? "PRESKOCENO" : "SPOUSTIM"));
                web.loadUrl(cil);
            };

            if (NapStahovaniSeSouhlasem.uzZeptano(MainActivity.this)) {
                spustIntro.run();
            } else {
                // Nez se uzivatel rozhodne, drzime prazdnou obrazovku -
                // at pod dialogem nic nebezi a nic mu neutika.
                applyWebViewVisualMode(menu, "startup");
                web.loadUrl("about:blank");
                appendNativeLog("BUILD2SA28 START ptam-se-na-stazeni");
                ui.postDelayed(() -> {
                    try {
                        NapStahovaniSeSouhlasem.zeptejSeAStahni(MainActivity.this,
                                getPublicAtariHelpDownloadsDir(),
                                zprava -> {
                                    appendNativeLog("BUILD2SA27 STAZENI " + zprava);
                                    spustIntro.run();     // az ted jede film
                                });
                    } catch (Throwable t) {
                        appendNativeLog("BUILD2SA27 STAZENI_CHYBA " + safeMsg(t));
                        spustIntro.run();
                    }
                }, 400);
                // kdyby se do dvou minut nic nestalo, jedeme dal sami
                ui.postDelayed(() -> {
                    if (!uzJede[0]) {
                        appendNativeLog("BUILD2SA28 POJISTKA - dialog nedobehl, spoustim intro");
                        spustIntro.run();
                    }
                }, 120000);
            }
        }
    }

    private void openBridgePicker(String kind) {
        pendingBridgeKind = kind;
        Intent i = new Intent(Intent.ACTION_GET_CONTENT);  // NOX je s ACTION_GET_CONTENT spolehlivejsi nez ACTION_OPEN_DOCUMENT
        i.addCategory(Intent.CATEGORY_OPENABLE);
        i.setType("*/*");
        i.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
        if ("mp3".equals(kind)) {
            i.setType("audio/*");
            i.putExtra(Intent.EXTRA_MIME_TYPES, new String[]{"audio/mpeg", "audio/mp3", "audio/wav", "audio/x-wav", "audio/*", "application/octet-stream"});
            i.putExtra("android.content.extra.SHOW_ADVANCED", true);
            i.putExtra(Intent.EXTRA_ALLOW_MULTIPLE, true);
            i.putExtra(Intent.EXTRA_LOCAL_ONLY, true);
            startActivityForResult(Intent.createChooser(i, "MP3 PRIDAT - vyber pisnicku nebo vice pisnicek"), PICK_BRIDGE);
        } else if ("audio".equals(kind)) {
            i.putExtra(Intent.EXTRA_MIME_TYPES, new String[]{"audio/*", "audio/wav", "audio/x-wav", "audio/mpeg", "application/octet-stream"});
            startActivityForResult(Intent.createChooser(i, "VLOZIT KAZETU - vyber WAV / MP3 / CAS z mobilu"), PICK_BRIDGE);
        } else if ("text".equals(kind)) {
            i.putExtra(Intent.EXTRA_MIME_TYPES, new String[]{"text/*", "application/octet-stream", "*/*"});
            startActivityForResult(Intent.createChooser(i, "Vyber BASIC / Turbo BASIC TXT z mobilu"), PICK_BRIDGE);
        } else {
            i.putExtra(Intent.EXTRA_MIME_TYPES, new String[]{"application/zip", "application/octet-stream", "application/x-msdos-program", "*/*"});
            startActivityForResult(Intent.createChooser(i, "Vyber XEX / ATR / ZIP z mobilu"), PICK_BRIDGE);
        }
    }

    private String getDisplayName(Uri uri) {
        try (Cursor c = getContentResolver().query(uri, null, null, null, null)) {
            if (c != null && c.moveToFirst()) {
                int idx = c.getColumnIndex(OpenableColumns.DISPLAY_NAME);
                if (idx >= 0) {
                    String n = c.getString(idx);
                    if (n != null && n.length() > 0) return n;
                }
            }
        } catch (Exception ignored) {}
        String p = uri.getLastPathSegment();
        return (p == null || p.length() == 0) ? "vybrany_soubor" : p;
    }

    private byte[] readUriBytes(Uri uri, int maxBytes) throws Exception {
        InputStream in;
        if (uri != null && "file".equalsIgnoreCase(uri.getScheme())) {
            in = new java.io.FileInputStream(new File(uri.getPath()));
        } else {
            in = getContentResolver().openInputStream(uri);
        }
        if (in == null) throw new Exception("Nelze otevrit soubor");
        ByteArrayOutputStream bos = new ByteArrayOutputStream();
        byte[] buf = new byte[16384];
        int n;
        while ((n = in.read(buf)) > 0) {
            bos.write(buf, 0, n);
            if (bos.size() > maxBytes) { in.close(); throw new Exception("Soubor je moc velky"); }
        }
        in.close();
        return bos.toByteArray();
    }

    private boolean isGameUrl(String url, String contentDisposition, String mimetype) {
        // BUILD2GH FIX:
        // NIKDY nehledat ".com" v cele URL, protoze facebook.com / youtube.com
        // by se pak povazoval za Atari COM soubor.
        try {
            if (url != null) {
                Uri uri = Uri.parse(url);
                String path = uri.getPath();
                if (hasGameExtension(path)) return true;
            }
        } catch (Throwable ignored) {}

        // Content-Disposition muze obsahovat skutecny filename souboru.
        if (contentDisposition != null) {
            String cd = contentDisposition.toLowerCase();
            int p = cd.indexOf("filename=");
            if (p >= 0) {
                String n = cd.substring(p + 9).replace("\"", "").replace("'", "").trim();
                int semi = n.indexOf(';');
                if (semi >= 0) n = n.substring(0, semi).trim();
                if (hasGameExtension(n)) return true;
            }
        }

        // MIME bereme jen jako slabou pomucku pro ZIP/octet, ne pro .com domenu.
        String mt = mimetype == null ? "" : mimetype.toLowerCase();
        if (mt.contains("application/zip")) return true;

        return false;
    }

    // BUILD2SA2: hry ze SBIRKY se musi routovat podle typu - Sega ROM do emu_sega,
    // ne do Atari. Atari cesta zustava pro xex/atr/com/exe/zip beze zmeny.
    private boolean hasSegaExtension(String value) {
        if (value == null) return false;
        String v = value.toLowerCase();
        int q = v.indexOf('?'); if (q >= 0) v = v.substring(0, q);
        int h = v.indexOf('#'); if (h >= 0) v = v.substring(0, h);
        return v.endsWith(".gen") || v.endsWith(".md") || v.endsWith(".smd") || v.endsWith(".sms") || v.endsWith(".68k") || v.endsWith(".sgd");
    }

    private boolean hasPs1RemoteExtension(String value) {
        if (value == null) return false;
        String v = value.toLowerCase(Locale.US);
        int q = v.indexOf('?'); if (q >= 0) v = v.substring(0, q);
        int h = v.indexOf('#'); if (h >= 0) v = v.substring(0, h);
        return v.endsWith(".cue") || v.endsWith(".bin") || v.endsWith(".iso") || v.endsWith(".img")
                || v.endsWith(".pbp") || v.endsWith(".chd");
    }

    private boolean hasZipExtension(String value) {
        if (value == null) return false;
        String v = value.toLowerCase(Locale.US);
        int q = v.indexOf('?'); if (q >= 0) v = v.substring(0, q);
        int h = v.indexOf('#'); if (h >= 0) v = v.substring(0, h);
        return v.endsWith(".zip");
    }

    private boolean isGoogleDriveUrl(String url) {
        if (url == null) return false;
        String u = url.trim().toLowerCase(Locale.US);
        if (!(u.startsWith("http://") || u.startsWith("https://"))) return false;
        try {
            Uri uri = Uri.parse(url);
            String host = uri.getHost();
            if (host == null) return false;
            host = host.toLowerCase(Locale.US);
            return host.equals("drive.google.com") || host.endsWith(".drive.google.com")
                    || host.equals("drive.usercontent.google.com") || host.endsWith(".drive.usercontent.google.com")
                    || host.equals("docs.google.com") || host.endsWith(".docs.google.com");
        } catch (Throwable ignored) {
            return u.contains("drive.google.com") || u.contains("drive.usercontent.google.com");
        }
    }

    private String googleDriveFileId(String url) {
        if (url == null) return null;
        try {
            Uri uri = Uri.parse(url);
            String id = uri.getQueryParameter("id");
            if (id != null && id.trim().length() > 0) return id.trim();
            String path = uri.getPath();
            if (path != null) {
                java.util.regex.Matcher m = java.util.regex.Pattern.compile("/d/([^/]+)").matcher(path);
                if (m.find()) return m.group(1);
                m = java.util.regex.Pattern.compile("/file/d/([^/]+)").matcher(path);
                if (m.find()) return m.group(1);
            }
        } catch (Throwable ignored) {}
        java.util.regex.Matcher m = java.util.regex.Pattern.compile("/d/([^/?#]+)").matcher(url);
        if (m.find()) return m.group(1);
        m = java.util.regex.Pattern.compile("[?&]id=([^&#]+)").matcher(url);
        if (m.find()) return m.group(1);
        return null;
    }

    private String ps1ResolveRemoteDownloadUrl(String url) throws IOException {
        if (!isGoogleDriveUrl(url)) return url;
        String id = googleDriveFileId(url);
        if (id == null || id.length() == 0) throw new IOException("Google Drive odkaz nema file id");
        String enc = java.net.URLEncoder.encode(id, "UTF-8");
        return "https://drive.usercontent.google.com/download?id=" + enc + "&export=download&confirm=t";
    }

    private void downloadAndRunPs1Remote(final String url, final String reason) {
        appendNativeLog("BUILD2SA5AM PS1_REMOTE_ROUTE reason=" + reason + " url=" + compactUrl(url));
        ui.post(() -> {
            try {
                String cur = web == null ? null : web.getUrl();
                if (web != null && (cur == null || !isPs1OwnerUrl(cur))) {
                    web.loadUrl(PS1_URL);
                }
            } catch (Throwable t) {
                appendNativeLog("BUILD2SA5AM PS1_REMOTE_OPEN_UI_FAIL " + safeMsg(t));
            }
        });
        startPs1RemoteDownloadAndBoot(url);
    }

    private boolean isPs1CollectionContext() {
        try {
            String cur = web == null ? null : web.getUrl();
            if (cur == null) return false;
            String u = cur.toLowerCase(Locale.US);
            return u.startsWith(PS1_URL) || u.contains("page_id=1048");
        } catch (Throwable ignored) {
            return false;
        }
    }

    private boolean isPs1RouteSource(String source) {
        if (source == null) return false;
        String s = source.toLowerCase(Locale.US);
        return s.contains("ps1") || s.contains("remoteurl");
    }

    private boolean shouldRouteAsPs1Download(String url, String source) {
        if (url == null) return false;
        boolean ps1Context = isPs1CollectionContext() || isPs1RouteSource(source);
        if (!ps1Context) return false;
        return isGoogleDriveUrl(url) || hasPs1RemoteExtension(url) || hasZipExtension(url);
    }

    private boolean hasAtariPayloadExtension(String value) {
        if (value == null) return false;
        String v = value.toLowerCase();
        int q = v.indexOf('?'); if (q >= 0) v = v.substring(0, q);
        int h = v.indexOf('#'); if (h >= 0) v = v.substring(0, h);
        return v.endsWith(".xex") || v.endsWith(".atr") || v.endsWith(".com") || v.endsWith(".exe");
    }

    private boolean isSegaCollectionContext() {
        try {
            String cur = web == null ? null : web.getUrl();
            if (cur == null) return false;
            String u = cur.toLowerCase(Locale.US);
            return u.startsWith(SEGA_URL) || u.contains("page_id=1003");
        } catch (Throwable ignored) {
            return false;
        }
    }

    private boolean shouldRouteAsSegaDownload(String url) {
        if (hasSegaExtension(url)) return true;
        if (!isGameUrl(url, null, null)) return false;
        return isSegaCollectionContext();
    }

    private SegaExtract extractSegaRomFromMaybeZip(String name, byte[] data) {
        if (data == null || data.length == 0) return null;
        if (hasSegaExtension(name)) {
            SegaExtract out = new SegaExtract();
            out.data = data;
            out.name = (name == null || name.length() == 0) ? "sega_game.md" : name;
            return out;
        }
        try {
            java.util.zip.ZipInputStream zi = new java.util.zip.ZipInputStream(new java.io.ByteArrayInputStream(data));
            java.util.zip.ZipEntry ze;
            while ((ze = zi.getNextEntry()) != null) {
                String en = ze.getName() == null ? "" : ze.getName();
                if (hasSegaExtension(en)) {
                    ByteArrayOutputStream ro = new ByteArrayOutputStream();
                    byte[] rb = new byte[16384];
                    int rn;
                    while ((rn = zi.read(rb)) > 0 && ro.size() < 16 * 1024 * 1024) {
                        ro.write(rb, 0, rn);
                    }
                    zi.close();
                    SegaExtract out = new SegaExtract();
                    out.data = ro.toByteArray();
                    int sl = en.lastIndexOf('/');
                    out.name = sl >= 0 ? en.substring(sl + 1) : en;
                    return out;
                }
                zi.closeEntry();
            }
            zi.close();
        } catch (Throwable ignored) {}
        return null;
    }

    private AtariExtract extractAtariPayloadFromMaybeZip(String name, byte[] data) {
        if (data == null || data.length == 0) return null;
        if (hasAtariPayloadExtension(name)) {
            AtariExtract out = new AtariExtract();
            out.data = data;
            out.name = (name == null || name.length() == 0) ? "atari_game.xex" : name;
            return out;
        }
        try {
            java.util.zip.ZipInputStream zi = new java.util.zip.ZipInputStream(new java.io.ByteArrayInputStream(data));
            java.util.zip.ZipEntry ze;
            AtariExtract atrFallback = null;
            while ((ze = zi.getNextEntry()) != null) {
                String en = ze.getName() == null ? "" : ze.getName();
                if (hasAtariPayloadExtension(en)) {
                    ByteArrayOutputStream ro = new ByteArrayOutputStream();
                    byte[] rb = new byte[16384];
                    int rn;
                    while ((rn = zi.read(rb)) > 0 && ro.size() < 16 * 1024 * 1024) {
                        ro.write(rb, 0, rn);
                    }
                    AtariExtract out = new AtariExtract();
                    out.data = ro.toByteArray();
                    int sl = en.lastIndexOf('/');
                    out.name = sl >= 0 ? en.substring(sl + 1) : en;
                    if (en.toLowerCase(Locale.US).endsWith(".atr")) {
                        atrFallback = out;
                    } else {
                        zi.close();
                        return out;
                    }
                }
                zi.closeEntry();
            }
            zi.close();
            if (atrFallback != null) return atrFallback;
        } catch (Throwable ignored) {}
        return null;
    }

    private void openSegaRomBytes(byte[] data, String name, String reason) {
        if (data == null || data.length == 0 || web == null) return;
        pendingSegaGame = data;
        pendingSegaName = (name == null || name.length() == 0) ? "sega_game.md" : name;
        String cur = web.getUrl();
        if (cur != null && cur.startsWith(SEGA_URL)) injectPendingSegaGame();
        else {
            web.loadUrl(SEGA_URL);
            web.postDelayed(MainActivity.this::injectPendingSegaGame, 1800);
        }
        appendNativeLog("BUILD2SA5AB SEGA_ROUTE_OPEN reason=" + reason + " name=" + pendingSegaName + " bytes=" + data.length);
    }

    private void downloadAndRunSega(final String url) {
        new Thread(() -> {
            try {
                if (!markAtariHelpRequestAllowed(url, "downloadSega")) return;
                FetchResult fetched = fetchUrlBytes(url, 16 * 1024 * 1024, "downloadSega");
                final String cdName = fetched.contentDisposition;
                final byte[] dataArr = fetched.data;
                final String name = guessDownloadName(url, cdName);
                appendNativeLog("BUILD2SA5AB SEGA_WEB_ROM_DOWNLOADED name=" + name + " bytes=" + dataArr.length + " via=" + compactUrl(fetched.via));
                ui.post(() -> openSegaRomBytes(dataArr, name, "rawSega"));
            } catch (Exception ex) {
                appendNativeLog("BUILD2SA5AB SEGA_WEB_ROM_FAIL " + safeMsg(ex));
                showWebDownloadError("SEGA SBIRKA: download selhal - " + safeMsg(ex));
            }
        }).start();
    }

    private void downloadAndRunSegaArchive(final String url) {
        new Thread(() -> {
            try {
                if (!markAtariHelpRequestAllowed(url, "downloadSegaArchive")) return;
                FetchResult fetched = fetchUrlBytes(url, 16 * 1024 * 1024, "downloadSegaArchive");
                final String cdName = fetched.contentDisposition;
                final byte[] dataArr = fetched.data;
                final String name = guessDownloadName(url, cdName);
                final SegaExtract sega = extractSegaRomFromMaybeZip(name, dataArr);
                if (sega == null || sega.data == null || sega.data.length == 0) {
                    throw new IOException("Sega ZIP neobsahuje .gen/.md/.smd/.sms ROM: " + name);
                }
                appendNativeLog("BUILD2SA5AB SEGA_ARCHIVE_DOWNLOADED zip=" + name + " rom=" + sega.name + " romBytes=" + sega.data.length + " via=" + compactUrl(fetched.via));
                ui.post(() -> openSegaRomBytes(sega.data, sega.name, "segaArchive"));
            } catch (Exception ex) {
                appendNativeLog("BUILD2SA5AB SEGA_ARCHIVE_FAIL " + safeMsg(ex));
                showWebDownloadError("SEGA SBIRKA: download selhal - " + safeMsg(ex));
            }
        }).start();
    }

    private void setPs1RemoteStatus(String status) {
        ps1RemoteDownloadStatus = status == null ? "" : status;
        appendNativeLog("BUILD2SA5AP " + ps1RemoteDownloadStatus);
    }

    private boolean canUsePs1CacheDir(File dir) {
        if (dir == null) return false;
        File probe = null;
        try {
            if (!dir.exists() && !dir.mkdirs()) return false;
            probe = new File(dir, ".nap_ps1_write_test");
            FileOutputStream out = new FileOutputStream(probe);
            try { out.write(1); } finally { out.close(); }
            return probe.exists();
        } catch (Throwable t) {
            appendNativeLog("BUILD2SA5AP PS1_CACHE_DIR_WRITE_FAIL path=" + (dir == null ? "null" : dir.getAbsolutePath()) + " err=" + safeMsg(t));
            return false;
        } finally {
            try { if (probe != null && probe.exists()) probe.delete(); } catch (Throwable ignored) {}
        }
    }

    private File ps1PrivateRemoteGamesDir() {
        File dir = null;
        try { dir = getExternalFilesDir("ps1_games"); } catch (Throwable ignored) {}
        if (dir == null) dir = new File(getFilesDir(), "ps1_games");
        return dir;
    }

    private File ps1RemoteGamesDir() throws IOException {
        File publicDir = null;
        try { publicDir = new File(getPublicAtariHelpDownloadsDir(), "PS1"); } catch (Throwable ignored) {}
        if (canUsePs1CacheDir(publicDir)) return publicDir;

        File dir = ps1PrivateRemoteGamesDir();
        if (!dir.exists() && !dir.mkdirs()) throw new IOException("nejde vytvorit PS1 slozka: " + dir.getAbsolutePath());
        appendNativeLog("BUILD2SA5AP PS1_CACHE_FALLBACK_APP_PRIVATE path=" + dir.getAbsolutePath());
        return dir;
    }

    private boolean deleteTree(File f) {
        if (f == null || !f.exists()) return true;
        File[] kids = f.listFiles();
        if (kids != null) {
            for (File k : kids) deleteTree(k);
        }
        return f.delete();
    }

    private String clearPs1RemoteCaches() throws IOException {
        if (ps1RemoteDownloadActive) {
            String busy = "PS1_CACHE_CLEAR_BUSY download prave bezi, pockej na dokonceni";
            setPs1RemoteStatus(busy);
            return busy;
        }
        // BUILD2SA5AR: drive tlacitko pri BEZICI hre jen tise ohlasilo BUSY a Rene
        // si myslel, ze cache smazal. Ted se hra napred korektne zastavi a maze se.
        if (ps1BootActive || ps1SessionActive) {
            try { NativePs1CoreBridge.stopSafe(); } catch (Throwable ignored) {}
            ps1BootActive = false;
            ps1SessionActive = false;
            ps1ClearJsPreview();
            appendNativeLog("BUILD2SA5AR PS1_CACHE_CLEAR_AUTOSTOP hra zastavena kvuli mazani cache");
        }
        int targets = 0;
        StringBuilder sb = new StringBuilder();
        File publicDir = null;
        try { publicDir = new File(getPublicAtariHelpDownloadsDir(), "PS1"); } catch (Throwable ignored) {}
        if (publicDir != null && publicDir.exists()) {
            targets++;
            boolean ok = deleteTree(publicDir);
            sb.append("downloads=").append(ok ? "OK" : "FAIL").append(":").append(publicDir.getAbsolutePath()).append(" ");
        }
        File privateDir = ps1PrivateRemoteGamesDir();
        if (privateDir != null && privateDir.exists()) {
            targets++;
            boolean ok = deleteTree(privateDir);
            sb.append("private=").append(ok ? "OK" : "FAIL").append(":").append(privateDir.getAbsolutePath()).append(" ");
        }
        if (targets == 0) sb.append("nic ke smazani");
        String res = "PS1_CACHE_CLEAR " + sb.toString().trim();
        setPs1RemoteStatus(res);
        return res;
    }

    private String ps1StableHash(String value) {
        try {
            java.security.MessageDigest md = java.security.MessageDigest.getInstance("SHA-1");
            byte[] dig = md.digest((value == null ? "" : value).getBytes("UTF-8"));
            StringBuilder sb = new StringBuilder();
            for (byte b : dig) {
                String h = Integer.toHexString(b & 0xff);
                if (h.length() < 2) sb.append('0');
                sb.append(h);
            }
            return sb.toString();
        } catch (Throwable ignored) {
            return Integer.toHexString(value == null ? 0 : value.hashCode());
        }
    }

    private String ps1RemoteCacheKey(String url) {
        String id = isGoogleDriveUrl(url) ? googleDriveFileId(url) : null;
        if (id != null && id.length() > 0) return safeFileName("gdrive_" + id);
        return safeFileName("url_" + ps1StableHash(url));
    }

    private File ps1RemoteCacheDir(String url) throws IOException {
        File root = ps1RemoteGamesDir();
        File dir = new File(root, ps1RemoteCacheKey(url));
        if (!dir.exists() && !dir.mkdirs()) throw new IOException("nejde vytvorit PS1 cache: " + dir.getAbsolutePath());
        return dir;
    }

    private File ps1RemoteCacheMarker(File dir) {
        return new File(dir, "_ps1_boot.name");
    }

    private boolean ps1CachedCueLooksComplete(File cueFile) {
        InputStream in = null;
        try {
            in = new java.io.FileInputStream(cueFile);
            byte[] cueBytes = readStreamLimited(in, 1024 * 1024);
            java.util.List<String> refs = ps1CueReferencedFiles(cueBytes);
            if (refs == null || refs.isEmpty()) return false;
            File dir = cueFile.getParentFile();
            for (String ref : refs) {
                File f = new File(dir, safeFileName(ref));
                if (!f.exists() || !f.isFile() || f.length() <= 0L) return false;
            }
            return true;
        } catch (Throwable t) {
            appendNativeLog("BUILD2SA5AP PS1_REMOTE_CACHE_CUE_CHECK_FAIL " + safeMsg(t));
            return false;
        } finally {
            try { if (in != null) in.close(); } catch (Throwable ignored) {}
        }
    }

    private File ps1ReadCachedBootFile(File dir) {
        InputStream in = null;
        try {
            File marker = ps1RemoteCacheMarker(dir);
            if (!marker.exists() || !marker.isFile()) return null;
            in = new java.io.FileInputStream(marker);
            String name = new String(readStreamLimited(in, 4096), "UTF-8").trim();
            if (name.length() == 0 || name.indexOf('/') >= 0 || name.indexOf('\\') >= 0) return null;
            File boot = new File(dir, safeFileName(name));
            if (!boot.exists() || !boot.isFile() || boot.length() <= 0L) return null;
            if (isPs1CueName(boot.getName()) && !ps1CachedCueLooksComplete(boot)) return null;
            return boot;
        } catch (Throwable t) {
            appendNativeLog("BUILD2SA5AP PS1_REMOTE_CACHE_READ_FAIL " + safeMsg(t));
            return null;
        } finally {
            try { if (in != null) in.close(); } catch (Throwable ignored) {}
        }
    }

    private void ps1WriteCacheMarker(File dir, File bootFile) {
        try {
            if (dir == null || bootFile == null) return;
            ps1WriteBytes(ps1RemoteCacheMarker(dir), bootFile.getName().getBytes("UTF-8"));
            appendNativeLog("BUILD2SA5AP PS1_REMOTE_CACHE_STORE boot=" + bootFile.getName() + " dir=" + dir.getName());
        } catch (Throwable t) {
            appendNativeLog("BUILD2SA5AP PS1_REMOTE_CACHE_STORE_FAIL " + safeMsg(t));
        }
    }

    private boolean isPs1RemoteImageName(String name) {
        if (name == null) return false;
        String n = name.toLowerCase(Locale.US);
        return n.endsWith(".bin") || n.endsWith(".iso") || n.endsWith(".img")
                || n.endsWith(".pbp") || n.endsWith(".chd") || n.endsWith(".cue") || n.endsWith(".zip");
    }

    private boolean isPs1CueName(String name) {
        return name != null && name.toLowerCase(Locale.US).endsWith(".cue");
    }

    private boolean isPs1ZipName(String name) {
        return name != null && name.toLowerCase(Locale.US).endsWith(".zip");
    }

    private String zipLeafName(String name) {
        if (name == null) return "ps1_game.bin";
        String n = name.replace("\\", "/");
        int slash = n.lastIndexOf('/');
        return slash >= 0 ? n.substring(slash + 1) : n;
    }

    private boolean isPs1ZipPayloadName(String name) {
        if (name == null) return false;
        String n = name.toLowerCase(Locale.US);
        return n.endsWith(".cue") || n.endsWith(".bin") || n.endsWith(".iso") || n.endsWith(".img")
                || n.endsWith(".pbp") || n.endsWith(".chd");
    }

    private boolean isPs1ZipPrimaryName(String name) {
        if (name == null) return false;
        String n = name.toLowerCase(Locale.US);
        return n.endsWith(".iso") || n.endsWith(".img") || n.endsWith(".bin") || n.endsWith(".pbp") || n.endsWith(".chd");
    }

    private java.util.List<String> ps1CueReferencedFiles(byte[] cueBytes) throws IOException {
        java.util.LinkedHashSet<String> refs = new java.util.LinkedHashSet<String>();
        if (cueBytes == null) return new java.util.ArrayList<String>();
        String text = new String(cueBytes, "ISO-8859-1");
        String[] lines = text.split("\\r?\\n");
        for (String line : lines) {
            if (line == null) continue;
            String t = line.trim();
            if (!t.toUpperCase(Locale.US).startsWith("FILE ")) continue;
            String ref = "";
            int a = t.indexOf('"');
            int b = a >= 0 ? t.indexOf('"', a + 1) : -1;
            if (a >= 0 && b > a) {
                ref = t.substring(a + 1, b);
            } else {
                String[] parts = t.split("\\s+");
                if (parts.length >= 2) ref = parts[1];
            }
            ref = ref == null ? "" : ref.trim();
            String low = ref.toLowerCase(Locale.US);
            if (ref.length() > 0 && (low.endsWith(".bin") || low.endsWith(".img") || low.endsWith(".iso") || low.endsWith(".wav"))) {
                refs.add(ref);
            }
        }
        return new java.util.ArrayList<String>(refs);
    }

    private String ps1CueWithSafeLocalNames(byte[] cueBytes, java.util.List<String> refs) throws IOException {
        String text = new String(cueBytes, "ISO-8859-1");
        if (refs == null) return text;
        for (String ref : refs) {
            String safe = safeFileName(ref);
            text = text.replace("\"" + ref + "\"", "\"" + safe + "\"");
            text = text.replace("FILE " + ref, "FILE " + safe);
        }
        return text;
    }

    private String ps1ResolveRelativeUrl(String baseUrl, String ref) throws IOException {
        String clean = ref == null ? "" : ref.trim().replace("\\", "/");
        clean = clean.replace(" ", "%20");
        return new URL(new URL(baseUrl), clean).toString();
    }

    private byte[] readZipEntryLimited(java.util.zip.ZipFile zip, java.util.zip.ZipEntry entry, int maxBytes) throws IOException {
        InputStream in = zip.getInputStream(entry);
        try {
            return readStreamLimited(in, maxBytes);
        } finally {
            try { in.close(); } catch (Throwable ignored) {}
        }
    }

    private java.util.zip.ZipEntry findZipEntry(java.util.zip.ZipFile zip, String ref, String cueEntryName) {
        if (zip == null || ref == null) return null;
        String wanted = ref.replace("\\", "/").trim();
        String wantedLower = wanted.toLowerCase(Locale.US);
        String wantedLeaf = zipLeafName(wanted).toLowerCase(Locale.US);
        String cueDir = "";
        if (cueEntryName != null) {
            String c = cueEntryName.replace("\\", "/");
            int slash = c.lastIndexOf('/');
            if (slash >= 0) cueDir = c.substring(0, slash + 1).toLowerCase(Locale.US);
        }
        java.util.Enumeration<? extends java.util.zip.ZipEntry> en = zip.entries();
        while (en.hasMoreElements()) {
            java.util.zip.ZipEntry ze = en.nextElement();
            if (ze == null || ze.isDirectory()) continue;
            String name = ze.getName() == null ? "" : ze.getName().replace("\\", "/");
            String low = name.toLowerCase(Locale.US);
            if (low.equals(wantedLower) || low.equals(cueDir + wantedLower) || low.endsWith("/" + wantedLower)) return ze;
            if (zipLeafName(low).equals(wantedLeaf)) return ze;
        }
        return null;
    }

    private File extractZipEntryToFile(java.util.zip.ZipFile zip, java.util.zip.ZipEntry entry, File dir, String safeName) throws IOException {
        File out = new File(dir, safeFileName(safeName == null ? zipLeafName(entry.getName()) : safeName));
        File part = new File(dir, out.getName() + ".part");
        try { if (part.exists()) part.delete(); } catch (Throwable ignored) {}
        try { if (out.exists()) out.delete(); } catch (Throwable ignored) {}
        InputStream in = zip.getInputStream(entry);
        FileOutputStream fos = new FileOutputStream(part);
        byte[] buf = new byte[65536];
        long total = 0L;
        long lastStatusAt = 0L;
        try {
            int n;
            while ((n = in.read(buf)) >= 0) {
                if (n == 0) continue;
                total += n;
                if (total > PS1_REMOTE_MAX_BYTES) throw new IOException("ZIP entry prekrocil limit " + formatMb(PS1_REMOTE_MAX_BYTES));
                fos.write(buf, 0, n);
                long now = System.currentTimeMillis();
                if (now - lastStatusAt > 1200L) {
                    lastStatusAt = now;
                    ps1RemoteDownloadStatus = "PS1_REMOTE_ZIP_EXTRACT " + formatMb(total) + " " + out.getName();
                }
            }
        } finally {
            try { fos.close(); } catch (Throwable ignored) {}
            try { in.close(); } catch (Throwable ignored) {}
        }
        if (total <= 0L) throw new IOException("ZIP entry je prazdny: " + entry.getName());
        if (!part.renameTo(out)) throw new IOException("nejde prejmenovat .part na " + out.getName());
        appendNativeLog("BUILD2SA5AM PS1_ZIP_EXTRACT_ENTRY name=" + out.getName() + " bytes=" + total);
        return out;
    }

    private File extractPs1RemoteZip(File zipFile, File dir) throws IOException {
        java.util.zip.ZipFile zip = new java.util.zip.ZipFile(zipFile);
        try {
            // BUILD2SA9: LOAD GAME s BIOS ZIPem se drive pokusil BIOS spustit jako hru
            // (viselo na 92 %). Ted BIOS pozname, nainstalujeme a rekneme to poctive.
            int biosN = 0;
            java.util.Enumeration<? extends java.util.zip.ZipEntry> enB = zip.entries();
            java.io.File biosSys = new java.io.File(getFilesDir(), "ps1_system");
            while (enB.hasMoreElements()) {
                java.util.zip.ZipEntry zb = enB.nextElement();
                if (zb == null || zb.isDirectory()) continue;
                String nb = zipLeafName(zb.getName()).toLowerCase(Locale.US);
                if (nb.startsWith("scph") && nb.endsWith(".bin") && zb.getSize() == 524288) {
                    if (!biosSys.exists()) biosSys.mkdirs();
                    java.io.InputStream ib = zip.getInputStream(zb);
                    java.io.FileOutputStream ob = new java.io.FileOutputStream(new java.io.File(biosSys, nb));
                    byte[] bb = new byte[16384]; int nn;
                    while ((nn = ib.read(bb)) > 0) ob.write(bb, 0, nn);
                    ob.close(); ib.close(); biosN++;
                }
            }
            if (biosN > 0) {
                appendNativeLog("BUILD2SA9 PS1_BIOS_INSTALLED_FROM_LOADGAME count=" + biosN);
                throw new IOException("BIOS NAINSTALOVAN (" + biosN + " souboru) - tohle byl BIOS, ne hra. Ted spust HRU a SONY logo pojede.");
            }
            java.util.zip.ZipEntry cue = null;
            java.util.zip.ZipEntry primary = null;
            java.util.Enumeration<? extends java.util.zip.ZipEntry> en = zip.entries();
            while (en.hasMoreElements()) {
                java.util.zip.ZipEntry ze = en.nextElement();
                if (ze == null || ze.isDirectory()) continue;
                String name = ze.getName();
                if (!isPs1ZipPayloadName(name)) continue;
                if (cue == null && isPs1CueName(name)) cue = ze;
                // BUILD2SA5AS: TEKKEN FIX - zip s vice .bin bez .cue: prvni entry byl
                // klidne 28MB AUDIO track (Track 3) => hra se sekla. Bereme NEJVETSI
                // payload (datovy track); "track 1" ma prednost pri stejne velikosti.
                if (isPs1ZipPrimaryName(name)) {
                    if (primary == null) primary = ze;
                    else {
                        long a = ze.getSize(), b = primary.getSize();
                        String ln = name.toLowerCase(Locale.US);
                        boolean t1 = ln.contains("track 1") || ln.contains("track 01") || ln.contains("track_1") || ln.contains("track1");
                        if (a > b || (a == b && t1)) primary = ze;
                    }
                }
            }
            if (cue != null) {
                byte[] cueBytes = readZipEntryLimited(zip, cue, 1024 * 1024);
                java.util.List<String> refs = ps1CueReferencedFiles(cueBytes);
                if (refs == null || refs.isEmpty()) throw new IOException("ZIP CUE nema FILE radky: " + cue.getName());
                setPs1RemoteStatus("PS1_REMOTE_ZIP_CUE " + zipLeafName(cue.getName()) + " files=" + refs.size());
                for (String ref : refs) {
                    java.util.zip.ZipEntry refEntry = findZipEntry(zip, ref, cue.getName());
                    if (refEntry == null) throw new IOException("ZIP CUE odkaz nenalezen: " + ref);
                    extractZipEntryToFile(zip, refEntry, dir, safeFileName(ref));
                }
                File cueOut = new File(dir, safeFileName(zipLeafName(cue.getName())));
                String cueText = ps1CueWithSafeLocalNames(cueBytes, refs);
                ps1WriteBytes(cueOut, cueText.getBytes("ISO-8859-1"));
                appendNativeLog("BUILD2SA5AM PS1_ZIP_CUE_READY name=" + cueOut.getName() + " files=" + refs.size());
                return cueOut;
            }
            if (primary != null) {
                // BUILD2SA7: MULTI-BIN bez .cue (treba 10 tracku) - rozbalime VSECHNY
                // biny a .cue SI VYROBIME: nejvetsi/track1 = datovy MODE2/2352,
                // ostatni AUDIO podle cisla tracku. Hra pak jede i s CD hudbou.
                java.util.List<java.util.zip.ZipEntry> bins = new java.util.ArrayList<>();
                java.util.Enumeration<? extends java.util.zip.ZipEntry> en3 = zip.entries();
                while (en3.hasMoreElements()) {
                    java.util.zip.ZipEntry z3 = en3.nextElement();
                    if (z3 == null || z3.isDirectory()) continue;
                    if (isPs1ZipPrimaryName(z3.getName()) && z3.getName().toLowerCase(Locale.US).endsWith(".bin")) bins.add(z3);
                }
                if (bins.size() > 1) {
                    final java.util.zip.ZipEntry dataTrack = primary;
                    java.util.Collections.sort(bins, (a, b) -> {
                        if (a == dataTrack) return -1; if (b == dataTrack) return 1;
                        return zipLeafName(a.getName()).compareToIgnoreCase(zipLeafName(b.getName()));
                    });
                    StringBuilder cueSb = new StringBuilder();
                    int trackNo = 1;
                    for (java.util.zip.ZipEntry bz : bins) {
                        String leaf = safeFileName(zipLeafName(bz.getName()));
                        extractZipEntryToFile(zip, bz, dir, leaf);
                        cueSb.append("FILE \"").append(leaf).append("\" BINARY\n");
                        if (trackNo == 1) cueSb.append("  TRACK 01 MODE2/2352\n    INDEX 01 00:00:00\n");
                        else cueSb.append(String.format(Locale.US, "  TRACK %02d AUDIO\n    INDEX 01 00:00:00\n", trackNo));
                        trackNo++;
                    }
                    File synth = new File(dir, "_ps1_synth.cue");
                    ps1WriteBytes(synth, cueSb.toString().getBytes("ISO-8859-1"));
                    setPs1RemoteStatus("PS1_REMOTE_ZIP_MULTIBIN tracks=" + bins.size() + " cue=SYNTETIZOVANY (vc. CD hudby)");
                    appendNativeLog("BUILD2SA7 PS1_CUE_SYNTH tracks=" + bins.size());
                    return synth;
                }
                setPs1RemoteStatus("PS1_REMOTE_ZIP_PRIMARY " + zipLeafName(primary.getName()) + " size=" + formatMb(Math.max(0, primary.getSize())));
                return extractZipEntryToFile(zip, primary, dir, zipLeafName(primary.getName()));
            }
            throw new IOException("ZIP neobsahuje PS1 .cue/.bin/.iso/.img/.pbp/.chd");
        } finally {
            try { zip.close(); } catch (Throwable ignored) {}
        }
    }

    private long contentLengthLong(HttpURLConnection c) {
        if (c == null) return -1L;
        try {
            String h = c.getHeaderField("Content-Length");
            if (h == null || h.trim().length() == 0) return -1L;
            return Long.parseLong(h.trim());
        } catch (Throwable ignored) {
            return -1L;
        }
    }

    private String formatMb(long bytes) {
        if (bytes < 0) return "?MB";
        return String.format(Locale.US, "%.1fMB", bytes / 1048576.0);
    }

    private File downloadPs1RemoteCompanionFile(String fileUrl, String safeName, File dir) throws IOException {
        HttpURLConnection c = null;
        File part = null;
        try {
            c = (HttpURLConnection) new URL(fileUrl).openConnection();
            c.setInstanceFollowRedirects(true);
            configureGameHttpConnection(c, fileUrl);
            try { c.setConnectTimeout(22000); } catch (Throwable ignored) {}
            try { c.setReadTimeout(70000); } catch (Throwable ignored) {}
            try { c.setRequestProperty("User-Agent", ATARIHELP_BROWSER_UA); } catch (Throwable ignored) {}
            try { c.setRequestProperty("Accept", "application/octet-stream,*/*"); } catch (Throwable ignored) {}
            c.connect();
            int code = c.getResponseCode();
            if (code < 200 || code >= 400) throw new IOException("HTTP " + code + " " + c.getResponseMessage());
            long expected = contentLengthLong(c);
            if (expected > PS1_REMOTE_MAX_BYTES) throw new IOException("soubor je moc velky: " + formatMb(expected));
            File out = new File(dir, safeName);
            part = new File(dir, safeName + ".part");
            try { if (part.exists()) part.delete(); } catch (Throwable ignored) {}
            try { if (out.exists()) out.delete(); } catch (Throwable ignored) {}
            InputStream in = c.getInputStream();
            FileOutputStream fos = new FileOutputStream(part);
            byte[] buf = new byte[65536];
            long total = 0L;
            long lastStatusAt = 0L;
            try {
                int n;
                while ((n = in.read(buf)) >= 0) {
                    if (n == 0) continue;
                    total += n;
                    if (total > PS1_REMOTE_MAX_BYTES) throw new IOException("prekrocen limit " + formatMb(PS1_REMOTE_MAX_BYTES));
                    fos.write(buf, 0, n);
                    long now = System.currentTimeMillis();
                    if (now - lastStatusAt > 1200L) {
                        lastStatusAt = now;
                        String progress = expected > 0
                                ? (Math.min(99L, (total * 100L) / expected) + "% " + formatMb(total) + "/" + formatMb(expected))
                                : formatMb(total);
                        ps1RemoteDownloadStatus = "PS1_REMOTE_CUE_FILE " + progress + " " + safeName;
                    }
                }
            } finally {
                try { fos.close(); } catch (Throwable ignored) {}
                try { in.close(); } catch (Throwable ignored) {}
            }
            if (total <= 0L) throw new IOException("stazeny soubor je prazdny: " + safeName);
            if (!part.renameTo(out)) throw new IOException("nejde prejmenovat .part na " + out.getName());
            appendNativeLog("BUILD2SA5AK PS1_REMOTE_CUE_FILE_READY name=" + safeName + " bytes=" + total);
            return out;
        } finally {
            try { if (c != null) c.disconnect(); } catch (Throwable ignored) {}
            try { if (part != null && part.exists()) part.delete(); } catch (Throwable ignored) {}
        }
    }

    // BUILD2SA5AR: ENOSPC oprava. Kazda hra = zip (~700MB) + rozbaleny bin (~700MB)
    // a cache drzela VSECHNY drivejsi hry => druha hra uz se na S8 nevesla.
    private long ps1PurgeOtherRemoteGames(File keepDir) {
        long freed = 0L;
        try {
            File[] roots = new File[] { new File(getPublicAtariHelpDownloadsDir(), "PS1"), ps1PrivateRemoteGamesDir() };
            for (File root : roots) {
                if (root == null || !root.exists()) continue;
                File[] kids = root.listFiles();
                if (kids == null) continue;
                for (File k : kids) {
                    if (keepDir != null && k.getAbsolutePath().equals(keepDir.getAbsolutePath())) continue;
                    freed += dirSizeBytes(k);
                    deleteTree(k);
                }
            }
        } catch (Throwable t) { appendNativeLog("BUILD2SA5AR PS1_AUTO_CLEAN_ERR " + safeMsg(t)); }
        if (freed > 0) appendNativeLog("BUILD2SA5AR PS1_CACHE_AUTO_CLEAN freed=" + formatMb(freed));
        return freed;
    }
    private long dirSizeBytes(File f) {
        if (f == null || !f.exists()) return 0L;
        if (f.isFile()) return f.length();
        long sum = 0L; File[] kids = f.listFiles();
        if (kids != null) for (File k : kids) sum += dirSizeBytes(k);
        return sum;
    }
    private void ps1EnsureFreeSpace(File dir, long contentLength) throws IOException {
        long need = contentLength > 0 ? (contentLength * 2 + 200L * 1024 * 1024) : 1600L * 1024 * 1024;
        long usable = 0L;
        try { usable = dir.getUsableSpace(); } catch (Throwable ignored) {}
        if (usable > 0 && usable < need) {
            throw new IOException("MALO MISTA v telefonu: potreba ~" + formatMb(need) + ", volne " + formatMb(usable)
                    + ". PS1 cache byla prave automaticky uklizena - uvolni jeste misto v telefonu a zkus znovu.");
        }
        appendNativeLog("BUILD2SA5AR PS1_SPACE_CHECK need=" + formatMb(need) + " usable=" + formatMb(usable));
    }
    // ============================================================================
    // BUILD2SB1: VLASTNI TV VYSTUP (Presentation API) - infrastruktura pro CELOU
    // appku. Kdyz je telefon pripojeny k externimu displeji (HDMI/DeX kabel nebo
    // bezdratove pripojeni), appka na TV kresli VLASTNI cistou obrazovku - jen
    // hru, zadne UI - a telefon slouzi jako ovladac. Efektivnejsi nez zrcadleni
    // celeho telefonu. SB1 zapojuje PS1 (ma cisty frame API); Sega = dalsi krok,
    // WebView emulatory (Atari pilot) = vlastni technika, v planu.
    // ============================================================================
    private android.app.Presentation tvPresentation = null;
    private android.view.SurfaceView tvSurface = null;
    private volatile boolean tvLoopRunning = false;
    private int[] tvFrameBuf = new int[1024 * 512];
    private void tvSetupDisplayListener() {
        try {
            android.hardware.display.DisplayManager dm =
                    (android.hardware.display.DisplayManager) getSystemService(DISPLAY_SERVICE);
            if (dm == null) return;
            dm.registerDisplayListener(new android.hardware.display.DisplayManager.DisplayListener() {
                public void onDisplayAdded(int id) { ui.post(() -> tvMaybeStart()); }
                public void onDisplayRemoved(int id) { ui.post(() -> tvStop("display_removed")); }
                public void onDisplayChanged(int id) {}
            }, ui);
            tvMaybeStart();
        } catch (Throwable t) { appendNativeLog("BUILD2SB1 TV_LISTENER_ERR " + safeMsg(t)); }
    }
    private void tvMaybeStart() {
        try {
            if (tvPresentation != null) return;
            android.hardware.display.DisplayManager dm =
                    (android.hardware.display.DisplayManager) getSystemService(DISPLAY_SERVICE);
            if (dm == null) return;
            android.view.Display[] disp = dm.getDisplays(android.hardware.display.DisplayManager.DISPLAY_CATEGORY_PRESENTATION);
            if (disp == null || disp.length == 0) return;
            tvPresentation = new android.app.Presentation(this, disp[0]);
            tvSurface = new android.view.SurfaceView(tvPresentation.getContext());
            tvPresentation.setContentView(tvSurface);
            tvPresentation.show();
            appendNativeLog("BUILD2SB1 TV_OUTPUT_START display=" + disp[0].getName());
            tvLoopRunning = true;
            new Thread(this::tvRenderLoop, "nap-tv-render").start();
        } catch (Throwable t) { appendNativeLog("BUILD2SB1 TV_START_ERR " + safeMsg(t)); }
    }
    private void tvStop(String why) {
        tvLoopRunning = false;
        try { if (tvPresentation != null) tvPresentation.dismiss(); } catch (Throwable ignored) {}
        tvPresentation = null; tvSurface = null;
        appendNativeLog("BUILD2SB1 TV_OUTPUT_STOP why=" + why);
    }
    private void tvRenderLoop() {
        android.graphics.Paint paint = new android.graphics.Paint(android.graphics.Paint.FILTER_BITMAP_FLAG);
        android.graphics.Bitmap bm = null;
        int lastW = 0, lastH = 0;
        long frames = 0;
        while (tvLoopRunning) {
            try {
                android.view.SurfaceView sv = tvSurface;
                if (sv == null) break;
                int wh = NativePs1CoreBridge.grabFrameSafe(tvFrameBuf);
                if (wh < 0) { int need = ((-wh) >> 16) * ((-wh) & 0xFFFF); tvFrameBuf = new int[need + 1024]; wh = NativePs1CoreBridge.grabFrameSafe(tvFrameBuf); }
                if (wh > 0) {
                    int w = wh >> 16, h = wh & 0xFFFF;
                    if (bm == null || w != lastW || h != lastH) { bm = android.graphics.Bitmap.createBitmap(w, h, android.graphics.Bitmap.Config.ARGB_8888); lastW = w; lastH = h; }
                    bm.setPixels(tvFrameBuf, 0, w, 0, 0, w, h);
                    android.view.SurfaceHolder holder = sv.getHolder();
                    android.graphics.Canvas c = holder.lockCanvas();
                    if (c != null) {
                        c.drawColor(0xFF000000);
                        float scale = Math.min((float) c.getWidth() / w, (float) c.getHeight() / h);
                        int dw = (int) (w * scale), dh = (int) (h * scale);
                        int dx = (c.getWidth() - dw) / 2, dy = (c.getHeight() - dh) / 2;
                        c.drawBitmap(bm, null, new Rect(dx, dy, dx + dw, dy + dh), paint);
                        holder.unlockCanvasAndPost(c);
                        if (++frames % 600 == 0) appendNativeLog("BUILD2SB1 TV_FRAMES " + frames);
                    }
                    Thread.sleep(16); // ~60 fps strop
                } else {
                    Thread.sleep(100); // nic nebezi - setrit CPU
                }
            } catch (Throwable t) { try { Thread.sleep(250); } catch (InterruptedException ignored) {} }
        }
    }
    // ================================================================
    //  KROK C: spusteni PS1 pres nativni EGL renderer.
    //
    //  core_ps1.c hleda hru v  filesDir/ps1/  a BIOS v  filesDir/ps1/bios/.
    //  Aplikace je ale mela jinde (hru v cache slozce, BIOS v ps1_system).
    //  Tahle metoda je pred spustenim rendereru pripravi PRESNE tam, kam
    //  core_ps1 kouka - hru i .bin vedle .cue, a BIOS. Az pak spusti
    //  aktivitu. Zadna zmena core_ps1 - jen se mu podstrci to, co ceka.
    // ================================================================
    private void stageOneFile(java.io.File src, java.io.File dstDir) throws java.io.IOException {
        if (src == null || !src.exists() || !src.isFile()) return;
        java.io.File dst = new java.io.File(dstDir, src.getName());
        if (dst.exists() && dst.length() == src.length()) return; // uz tam je
        try (java.io.InputStream in = new java.io.FileInputStream(src);
             java.io.OutputStream out = new java.io.FileOutputStream(dst)) {
            byte[] buf = new byte[65536];
            int n;
            while ((n = in.read(buf)) > 0) out.write(buf, 0, n);
        }
    }

    private void launchEglPs1Activity(String gamePath) {
        try {
            java.io.File filesDir = getFilesDir();
            java.io.File ps1Dir  = new java.io.File(filesDir, "ps1");
            java.io.File biosDir = new java.io.File(ps1Dir, "bios");
            ps1Dir.mkdirs();
            biosDir.mkdirs();

            // core_ps1 bere PRVNI .cue ve slozce - kdyby tu zustala minula
            // hra, vzal by ji misto nove. Smazat stare herni soubory (BIOS
            // podslozku necham, ta se prepisuje zvlast).
            java.io.File[] old = ps1Dir.listFiles();
            if (old != null) {
                for (java.io.File f : old) {
                    if (f.isFile()) { try { f.delete(); } catch (Throwable ignored) {} }
                }
            }

            // 1) Hra: .cue + vsechny sourozence ze stejne slozky (.bin, .img...)
            //    core_ps1 bere prvni .cue v ps1Dir; .cue odkazuje na .bin vedle.
            java.io.File game = (gamePath != null) ? new java.io.File(gamePath) : null;
            if (game != null && game.exists()) {
                java.io.File srcDir = game.getParentFile();
                if (srcDir != null) {
                    java.io.File[] siblings = srcDir.listFiles();
                    if (siblings != null) {
                        for (java.io.File f : siblings) {
                            if (f.isFile()) stageOneFile(f, ps1Dir);
                        }
                    }
                }
                appendNativeLog("KROKC EGL_STAGE_GAME dir=" + ps1Dir.getAbsolutePath()
                        + " z=" + (srcDir != null ? srcDir.getAbsolutePath() : "?"));
            }

            // 2) BIOS: z ps1_system, kam ho appka instaluje, do ps1/bios
            java.io.File biosSrc = new java.io.File(filesDir, "ps1_system");
            java.io.File[] biosFiles = biosSrc.listFiles();
            if (biosFiles != null) {
                for (java.io.File f : biosFiles) {
                    if (f.isFile()) stageOneFile(f, biosDir);
                }
                appendNativeLog("KROKC EGL_STAGE_BIOS do=" + biosDir.getAbsolutePath());
            }

            // ===== JEDNA CESTA PRO BIOS I HRU =====
            // Drive se tady otviralo SAMOSTATNE OKNO (NativeActivity) na sirku.
            // To bylo to druhe platno: hra se v nem kreslila mimo monitor,
            // v portretu mezitim zustal viset BIOS a ovladac tam nebyl.
            // Ted se hra spousti do TEHOZ monitoru jako BIOS - stejne jadro,
            // stejny odber snimku, stejna obrazovka.
            String cestaKeHre = "";
            java.io.File[] nalezene = ps1Dir.listFiles();
            if (nalezene != null) {
                for (java.io.File f : nalezene) {
                    if (!f.isFile()) continue;
                    String n = f.getName().toLowerCase(java.util.Locale.US);
                    if (n.endsWith(".cue")) { cestaKeHre = f.getAbsolutePath(); break; }
                    if (cestaKeHre.isEmpty() && (n.endsWith(".chd") || n.endsWith(".pbp")
                            || n.endsWith(".iso") || n.endsWith(".img") || n.endsWith(".bin"))) {
                        cestaKeHre = f.getAbsolutePath();
                    }
                }
            }
            ps1GameWindowOwnsCore = false;  // jadro zustava tady, u monitoru
            ps1BiosRunning = false;
            java.io.File sysDirH  = new java.io.File(getFilesDir(), "ps1_system");
            java.io.File saveDirH = new java.io.File(getFilesDir(), "ps1_saves");
            if (!sysDirH.exists())  sysDirH.mkdirs();
            if (!saveDirH.exists()) saveDirH.mkdirs();
            ps1EnsureBios(sysDirH);
            ps1LastBootResult = NativePs1CoreBridge.bootGameSafe(
                    sysDirH.getAbsolutePath(), saveDirH.getAbsolutePath(), cestaKeHre);
            ps1SessionActive = ps1LastBootResult != null
                    && ps1LastBootResult.startsWith("PS1_HRA_OK");
            appendNativeLog("PS1_HRA_DO_MONITORU cesta=" + cestaKeHre
                    + " vysledek=" + ps1LastBootResult);
        } catch (Throwable t) {
            appendNativeLog("KROKC EGL_PS1_LAUNCH_FAIL " + safeMsg(t) + " - padam na puvodni cestu");
            ps1ActivateNativeView();
        }
    }

    private void startPs1RemoteDownloadAndBoot(final String rawUrl) {
        final String url = rawUrl == null ? "" : rawUrl.trim();
        if (!(url.startsWith("http://") || url.startsWith("https://"))) {
            setPs1RemoteStatus("PS1_REMOTE_FAIL bad_url");
            ps1LastBootResult = "PS1_REMOTE_FAIL bad_url";
            return;
        }
        final int downloadGen;
        synchronized (this) {
            if (ps1RemoteDownloadActive) {
                ps1RemoteDownloadStatus = "PS1_REMOTE_BUSY";
                return;
            }
            downloadGen = ++ps1LifecycleGen;
            ps1RemoteDownloadActive = true;
            ps1RemoteDownloadStatus = "PS1_REMOTE_START " + compactUrl(url);
        }
        new Thread(() -> {
            HttpURLConnection c = null;
            File part = null;
            try {
                String downloadUrl = ps1ResolveRemoteDownloadUrl(url);
                boolean googleDrive = isGoogleDriveUrl(url);
                File dir = ps1RemoteCacheDir(url);
                File cached = ps1ReadCachedBootFile(dir);
                if (cached != null) {
                    synchronized (MainActivity.this) {
                        if (downloadGen != ps1LifecycleGen || !ps1RemoteDownloadActive) {
                            setPs1RemoteStatus("PS1_REMOTE_CANCELLED_BEFORE_CACHE " + cached.getName());
                            return;
                        }
                    }
                    setPs1RemoteStatus("PS1_REMOTE_CACHE_HIT " + cached.getName() + " path=" + cached.getAbsolutePath());
                    bootPs1FileOnCurrentThread(cached, cached.getName(), "remoteCache");
                    return;
                }
                ps1PurgeOtherRemoteGames(dir); // BUILD2SA5AR: pred stazenim uklidit VSECHNY stare hry
                setPs1RemoteStatus("PS1_REMOTE_CONNECT " + compactUrl(url) + (googleDrive ? " via=google_drive" : "") + " path=" + dir.getAbsolutePath());
                c = (HttpURLConnection) new URL(downloadUrl).openConnection();
                c.setInstanceFollowRedirects(true);
                configureGameHttpConnection(c, downloadUrl);
                try { c.setConnectTimeout(22000); } catch (Throwable ignored) {}
                try { c.setReadTimeout(70000); } catch (Throwable ignored) {}
                try { c.setRequestProperty("User-Agent", ATARIHELP_BROWSER_UA); } catch (Throwable ignored) {}
                try { c.setRequestProperty("Accept", "application/octet-stream,application/x-cd-image,application/x-chd,*/*"); } catch (Throwable ignored) {}
                c.connect();
                int code = c.getResponseCode();
                if (code < 200 || code >= 400) throw new IOException("HTTP " + code + " " + c.getResponseMessage());
                String contentType = c.getContentType();
                long expectedLen = -1L;
                try { expectedLen = c.getContentLengthLong(); } catch (Throwable ignored) {}
                ps1EnsureFreeSpace(dir, expectedLen); // BUILD2SA5AR: srozumitelna hlaska misto ENOSPC v pulce
                String name = safeFileName(guessDownloadName(downloadUrl, c.getHeaderField("Content-Disposition")));
                if (!isPs1RemoteImageName(name)) {
                    if (googleDrive) {
                        throw new IOException("Google Drive nevratil PS1 soubor. Nastav soubor na 'Kdokoli s odkazem - viewer' a na Drive dej .zip/.bin/.iso/.img/.pbp/.chd, ne .7z. contentType=" + contentType);
                    }
                    throw new IOException("PS1 URL musi koncit na .zip/.cue/.bin/.iso/.img/.pbp/.chd; ted je: " + name);
                }
                long expected = contentLengthLong(c);
                if (expected > PS1_REMOTE_MAX_BYTES) throw new IOException("soubor je moc velky: " + formatMb(expected));

                if (isPs1CueName(name)) {
                    if (googleDrive) {
                        throw new IOException("Google Drive .cue potrebuje vedle sebe i .bin soubory, ale Drive odkaz ukazuje jen jeden soubor. Pouzij radeji .iso/.chd/.pbp nebo primo .bin.");
                    }
                    byte[] cueBytes = readStreamLimited(c.getInputStream(), 1024 * 1024);
                    if (cueBytes == null || cueBytes.length == 0) throw new IOException("CUE je prazdne");
                    java.util.List<String> refs = ps1CueReferencedFiles(cueBytes);
                    if (refs == null || refs.isEmpty()) throw new IOException("CUE neobsahuje FILE radky pro .bin/.img/.iso");
                    setPs1RemoteStatus("PS1_REMOTE_CUE " + name + " files=" + refs.size());
                    for (String ref : refs) {
                        String safeRef = safeFileName(ref);
                        String refUrl = ps1ResolveRelativeUrl(downloadUrl, ref);
                        downloadPs1RemoteCompanionFile(refUrl, safeRef, dir);
                    }
                    File cueOut = new File(dir, name);
                    String cueText = ps1CueWithSafeLocalNames(cueBytes, refs);
                    ps1WriteBytes(cueOut, cueText.getBytes("ISO-8859-1"));
                    synchronized (MainActivity.this) {
                        if (downloadGen != ps1LifecycleGen || !ps1RemoteDownloadActive) {
                            setPs1RemoteStatus("PS1_REMOTE_CANCELLED_AFTER_CUE " + name);
                            return;
                        }
                    }
                    setPs1RemoteStatus("PS1_REMOTE_READY " + name + " cueFiles=" + refs.size() + " path=" + cueOut.getAbsolutePath());
                    ps1WriteCacheMarker(dir, cueOut);
                    bootPs1FileOnCurrentThread(cueOut, name, "remoteCueUrl");
                    return;
                }
                File out = new File(dir, name);
                part = new File(dir, name + ".part");
                try { if (part.exists()) part.delete(); } catch (Throwable ignored) {}
                try { if (out.exists()) out.delete(); } catch (Throwable ignored) {}

                InputStream in = c.getInputStream();
                FileOutputStream fos = new FileOutputStream(part);
                byte[] buf = new byte[65536];
                long total = 0L;
                long lastStatusAt = 0L;
                try {
                    int n;
                    while ((n = in.read(buf)) >= 0) {
                        if (n == 0) continue;
                        total += n;
                        if (total > PS1_REMOTE_MAX_BYTES) throw new IOException("prekrocen limit " + formatMb(PS1_REMOTE_MAX_BYTES));
                        fos.write(buf, 0, n);
                        long now = System.currentTimeMillis();
                        if (now - lastStatusAt > 1200L) {
                            lastStatusAt = now;
                            String progress = expected > 0
                                    ? (Math.min(99L, (total * 100L) / expected) + "% " + formatMb(total) + "/" + formatMb(expected))
                                    : formatMb(total);
                            ps1RemoteDownloadStatus = "PS1_REMOTE_DOWNLOAD " + progress + " " + name;
                        }
                    }
                } finally {
                    try { fos.close(); } catch (Throwable ignored) {}
                    try { in.close(); } catch (Throwable ignored) {}
                }
                if (total <= 0L) throw new IOException("stazeny soubor je prazdny");
                if (!part.renameTo(out)) throw new IOException("nejde prejmenovat .part na " + out.getName());

                synchronized (MainActivity.this) {
                    if (downloadGen != ps1LifecycleGen || !ps1RemoteDownloadActive) {
                        setPs1RemoteStatus("PS1_REMOTE_CANCELLED_AFTER_DOWNLOAD " + name);
                        return;
                    }
                }
                if (isPs1ZipName(name)) {
                    setPs1RemoteStatus("PS1_REMOTE_ZIP_READY " + name + " bytes=" + total);
                    File bootFile = extractPs1RemoteZip(out, dir);
                    try { long zb = out.length(); if (out.delete()) appendNativeLog("BUILD2SA5AR PS1_ZIP_DELETED_AFTER_EXTRACT freed=" + formatMb(zb)); } catch (Throwable ignored) {} // BUILD2SA5AR
                    try { out.delete(); } catch (Throwable ignored) {}
                    setPs1RemoteStatus("PS1_REMOTE_READY " + bootFile.getName() + " fromZip=" + name + " path=" + bootFile.getAbsolutePath());
                    ps1WriteCacheMarker(dir, bootFile);
                    bootPs1FileOnCurrentThread(bootFile, bootFile.getName(), "remoteZipUrl");
                    return;
                }
                setPs1RemoteStatus("PS1_REMOTE_READY " + name + " bytes=" + total + " path=" + out.getAbsolutePath());
                ps1WriteCacheMarker(dir, out);
                bootPs1FileOnCurrentThread(out, name, "remoteUrl");
            } catch (Throwable t) {
                // BUILD2SA10: instalace BIOSu neni chyba - nesmi se ukazat cerveny banner.
                if (safeMsg(t) != null && safeMsg(t).startsWith("BIOS NAINSTALOVAN")) {
                    ps1LastBootResult = "PS1_BIOS_INSTALLED " + safeMsg(t);
                    setPs1RemoteStatus("PS1_BIOS_INSTALLED " + safeMsg(t));
                    return;
                }
                ps1LastBootResult = "PS1_REMOTE_FAIL " + safeMsg(t);
                setPs1RemoteStatus(ps1LastBootResult);
                try { if (part != null && part.exists()) part.delete(); } catch (Throwable ignored) {}
            } finally {
                try { if (c != null) c.disconnect(); } catch (Throwable ignored) {}
                synchronized (MainActivity.this) {
                    ps1RemoteDownloadActive = false;
                }
            }
        }, "nap-ps1-remote-download").start();
    }

    // BUILD2SA7: BIOS AUTO-ADOPT. Audit ukazal sysdirFiles=[prazdna] => jadro jelo
    // na vestavene nahrade (zadne SONY logo/znelka). Rene-proof reseni: BIOS ZIP
    // stazeny z atarihelp.eu do Download/AtariHelp si appka sama najde a adoptuje.
    private void ps1EnsureBios(File sysDir) {
        try {
            File[] have = sysDir.listFiles();
            if (have != null) for (File f : have) {
                String n = f.getName().toLowerCase(Locale.US);
                if (n.startsWith("scph") && n.endsWith(".bin") && f.length() == 524288) return; // BIOS uz je
            }
            File root = getPublicAtariHelpDownloadsDir();
            File[] scan = new File[] { root, new File(root, "BIOS"), new File(root, "PS1_BIOS") };
            int adopted = 0;
            for (File d : scan) {
                if (d == null || !d.isDirectory()) continue;
                File[] kids = d.listFiles();
                if (kids == null) continue;
                for (File k : kids) {
                    String n = k.getName().toLowerCase(Locale.US);
                    if (k.isFile() && n.startsWith("scph") && n.endsWith(".bin") && k.length() == 524288) {
                        ps1CopyFile(k, new File(sysDir, n)); adopted++;
                    } else if (k.isFile() && n.endsWith(".zip") && n.contains("bios") && k.length() < 8L * 1024 * 1024) {
                        java.util.zip.ZipFile z = new java.util.zip.ZipFile(k);
                        try {
                            java.util.Enumeration<? extends java.util.zip.ZipEntry> en2 = z.entries();
                            while (en2.hasMoreElements()) {
                                java.util.zip.ZipEntry ze2 = en2.nextElement();
                                if (ze2 == null || ze2.isDirectory()) continue;
                                String zn = zipLeafName(ze2.getName()).toLowerCase(Locale.US);
                                if (zn.startsWith("scph") && zn.endsWith(".bin") && ze2.getSize() == 524288) {
                                    java.io.InputStream in2 = z.getInputStream(ze2);
                                    java.io.FileOutputStream fo2 = new java.io.FileOutputStream(new File(sysDir, zn));
                                    byte[] b2 = new byte[16384]; int n2;
                                    while ((n2 = in2.read(b2)) > 0) fo2.write(b2, 0, n2);
                                    fo2.close(); in2.close(); adopted++;
                                }
                            }
                        } finally { try { z.close(); } catch (Throwable ignored) {} }
                    }
                }
            }
            if (adopted > 0) { appendNativeLog("BUILD2SA7 PS1_BIOS_ADOPTED count=" + adopted + " -> " + sysDir.getAbsolutePath()); return; }
            // BUILD2SA12: NULA KLIKU - appka si BIOS sama stahne z Reneho stranek.
            // 1) precte stranku page_id=1048 a najde na ni BIOS ZIP odkaz
            // 2) zkusi zname primé cesty jako zalohu
            java.util.List<String> cand = new java.util.ArrayList<>();
            // BUILD2SA12B: primy odkaz od Reneho (7.7.2026) - prvni volba, nejrychlejsi.
            cand.add("https://atarihelp.eu/wp-content/uploads/2026/07/PS1-BIOS_.zip");
            try {
                HttpURLConnection pc = (HttpURLConnection) new URL("https://atarihelp.eu/?page_id=1048").openConnection();
                pc.setConnectTimeout(8000); pc.setReadTimeout(8000);
                pc.setRequestProperty("User-Agent", "Mozilla/5.0 (Linux; Android 9) AtariHelpEMU10");
                java.io.InputStream pin = pc.getInputStream();
                ByteArrayOutputStream pbo = new ByteArrayOutputStream();
                byte[] pb = new byte[16384]; int pn2;
                while ((pn2 = pin.read(pb)) > 0 && pbo.size() < 512 * 1024) pbo.write(pb, 0, pn2);
                pin.close();
                String html = pbo.toString("UTF-8");
                java.util.regex.Matcher m = java.util.regex.Pattern.compile("(?i)(href|src)=[\"']([^\"']*\\.zip)[\"']").matcher(html);
                while (m.find()) {
                    String u2 = m.group(2);
                    if (!u2.toLowerCase(Locale.US).contains("bios")) continue;
                    if (u2.startsWith("//")) u2 = "https:" + u2;
                    else if (u2.startsWith("/")) u2 = "https://atarihelp.eu" + u2;
                    cand.add(u2);
                }
            } catch (Throwable pt) { appendNativeLog("BUILD2SA12 PS1_BIOS_PAGE_SCAN_FAIL " + safeMsg(pt)); }
            cand.add("https://atarihelp.eu/wp-content/uploads/2026/07/_PS1_-_BIOS_.ZIP");
            for (String bu : cand) {
                try {
                    appendNativeLog("BUILD2SA12 PS1_BIOS_AUTO_DL_TRY " + bu);
                    HttpURLConnection bc = (HttpURLConnection) new URL(bu).openConnection();
                    bc.setConnectTimeout(10000); bc.setReadTimeout(20000);
                    bc.setInstanceFollowRedirects(true);
                    bc.setRequestProperty("User-Agent", "Mozilla/5.0 (Linux; Android 9) AtariHelpEMU10");
                    java.io.InputStream in3 = bc.getInputStream();
                    ByteArrayOutputStream bo3 = new ByteArrayOutputStream();
                    byte[] b3 = new byte[16384]; int n3;
                    while ((n3 = in3.read(b3)) > 0 && bo3.size() < 8 * 1024 * 1024) bo3.write(b3, 0, n3);
                    in3.close();
                    byte[] zipData = bo3.toByteArray();
                    int inst = 0;
                    java.util.zip.ZipInputStream zi3 = new java.util.zip.ZipInputStream(new java.io.ByteArrayInputStream(zipData));
                    java.util.zip.ZipEntry ze3;
                    while ((ze3 = zi3.getNextEntry()) != null) {
                        if (ze3.isDirectory()) continue;
                        String zn3 = zipLeafName(ze3.getName()).toLowerCase(Locale.US);
                        if (zn3.startsWith("scph") && zn3.endsWith(".bin")) {
                            ByteArrayOutputStream eo = new ByteArrayOutputStream();
                            byte[] eb = new byte[16384]; int en4;
                            while ((en4 = zi3.read(eb)) > 0 && eo.size() <= 524288) eo.write(eb, 0, en4);
                            if (eo.size() == 524288) {
                                java.io.FileOutputStream ef = new java.io.FileOutputStream(new File(sysDir, zn3));
                                eo.writeTo(ef); ef.close(); inst++;
                            }
                        }
                        zi3.closeEntry();
                    }
                    zi3.close();
                    if (inst > 0) {
                        try { java.io.FileOutputStream kf2 = new java.io.FileOutputStream(new File(getPublicAtariHelpDownloadsDir(), "_PS1_-_BIOS_.ZIP")); kf2.write(zipData); kf2.close(); } catch (Throwable ignored) {}
                        appendNativeLog("BUILD2SA12 PS1_BIOS_AUTO_DOWNLOADED url=" + bu + " count=" + inst);
                        setPs1RemoteStatus("PS1_BIOS_AUTO_INSTALLED (" + inst + ") ze stranek atarihelp.eu - SONY logo pojede");
                        return;
                    }
                } catch (Throwable dt) { appendNativeLog("BUILD2SA12 PS1_BIOS_AUTO_DL_FAIL " + bu + " " + safeMsg(dt)); }
            }
            appendNativeLog("BUILD2SA7 PS1_BIOS_MISSING auto-stazeni nevyslo - stahni BIOS ZIP z atarihelp.eu do Download/AtariHelp nebo pres LOAD GAME");
        } catch (Throwable t) { appendNativeLog("BUILD2SA7 PS1_BIOS_ADOPT_ERR " + safeMsg(t)); }
    }
    private void ps1CopyFile(File src, File dst) throws IOException {
        java.io.FileInputStream in = new java.io.FileInputStream(src);
        java.io.FileOutputStream out2 = new java.io.FileOutputStream(dst);
        byte[] b = new byte[65536]; int n;
        while ((n = in.read(b)) > 0) out2.write(b, 0, n);
        out2.close(); in.close();
    }
    private void bootPs1FileOnCurrentThread(File gameFile, String label, String reason) {
        int bootGen = 0;
        try {
            if (gameFile == null || !gameFile.exists() || !gameFile.isFile()) {
                throw new IOException("PS1 soubor neexistuje");
            }
            closePs1GamePfdQuietly();
            synchronized (MainActivity.this) {
                bootGen = ++ps1LifecycleGen;
                ps1BootActive = true;
                ps1SessionActive = false;
            }
            java.io.File sysDir = new java.io.File(getFilesDir(), "ps1_system");
            java.io.File saveDir = new java.io.File(getFilesDir(), "ps1_saves");
            if (!sysDir.exists()) sysDir.mkdirs();
            if (!saveDir.exists()) saveDir.mkdirs();
            ps1CurrentGameLabel = safeFileName(label == null ? gameFile.getName() : label);
            stopPs1Audio();
            ps1LastBootResult = "PS1_REMOTE_BOOTING " + ps1CurrentGameLabel;
            appendNativeLog("BUILD2SA5AK PS1_REMOTE_BOOT reason=" + reason + " name=" + ps1CurrentGameLabel + " bytes=" + gameFile.length() + " path=" + gameFile.getAbsolutePath());
            appendNativeLog("BUILD2SA5AK PS1_BIOS_AUDIT " + ps1BiosAudit(sysDir));
            ps1EnsureBios(sysDir); // BUILD2SA7
            // KROK C: obraz PS1 jede pres nativni EGL renderer (core_ps1.c
            // si jadro i hru nacte sam pres libretro). Java uz jadro
            // nebootuje ani netaha obraz. Zvuk zatim po staru (v core_ps1
            // je audio_cb prazdne - prijde jako dalsi krok).
            ps1LastBootResult = "PS1_BOOT_OK EGL";
            boolean stillWanted = bootGen == ps1LifecycleGen && ps1BootActive;
            ps1BootActive = false;
            if (stillWanted) {
                ps1SessionActive = true;
                setPs1RemoteStatus("PS1_REMOTE_BOOT_OK " + ps1CurrentGameLabel);
                final String gp = gameFile.getAbsolutePath();
                ui.post(() -> launchEglPs1Activity(gp));
            } else {
                ps1SessionActive = false;
                stopPs1Audio();
                // CESTA A: sem se dostaneme, kdyz uz o boot neni zajem
                // (bootGen se zmenil nebo ps1BootActive vypnuto). Uklid
                // probehne vzdy - drive tu byla podminka if(ok), ale
                // promenna ok patrila stare bootSafe ceste, ktera uz tu neni.
                try { NativePs1CoreBridge.stopSafe(); } catch (Throwable ignored) {}
                ps1ClearJsPreview();
                ps1LastBootResult = "PS1_BOOT_CANCELLED_AFTER_LEAVE";
                setPs1RemoteStatus(ps1LastBootResult);
            }
        } catch (Throwable t) {
            ps1BootActive = false;
            ps1SessionActive = false;
            stopPs1Audio();
            ps1LastBootResult = "PS1_REMOTE_BOOT_EXCEPTION " + safeMsg(t);
            setPs1RemoteStatus(ps1LastBootResult);
        }
    }

    // BUILD2SA3/SA5P: PS1 zvuk - dedikovane vlakno, Sega-style 384f chunks,
    // retry misto okamziteho ticha, generation guard a hard release pri prepnuti.
    private synchronized void startPs1Audio() {
        stopPs1Audio(); // BUILD2SA3B: pred novou hrou zabit stare vlakno a uvolnit stary AudioTrack

        // ==============================================================
        //  DVA ZVUKOVE VYSTUPY NARAZ - tohle byla ta chyba.
        //  V ceste A si zvuk PS1 obsluhuje nativni OpenSL prehravac primo
        //  v jadre (nap_sl_open(), otevira se pri kazdem bootu PS1). Tenhle
        //  Javovy AudioTrack byl PUVODNI cesta a nikdo ho nevypnul - takze
        //  bezely oba a oba drzely otevreny zvukovy vystup. Javovy uz navic
        //  nema odkud brat vzorky (jadro je posila do vlastni fronty), takze
        //  jen dokola podteka a zabira zvukovy vystup.
        //  Odtud kousani, ktere zavisi na zatezi - dve cesty se hadaji.
        //  Zvuk PS1 obsluhuje JEDEN vystup: nativni.
        // ==============================================================
        appendNativeLog("PS1_AUDIO_JAVA_VYPNUTO duvod=zvuk obsluhuje nativni OpenSL (drive bezely oba naraz)");
        ps1CurrentAudioTrack = null;
        ps1AudioThread = null;
    }

    // PUVODNI Javova zvukova cesta - ponechana jen pro pripad navratu, NEVOLA SE.
    @SuppressWarnings("unused")
    private synchronized void startPs1AudioJavaStara_NEPOUZIVA_SE() {
        final int gen = ++ps1AudioGen;
        ps1AudioThread = new Thread(() -> {
            AudioTrack at = null;
            try {
                try { android.os.Process.setThreadPriority(android.os.Process.THREAD_PRIORITY_URGENT_AUDIO); } catch (Throwable ignored) {}
                final int sampleRate = 44100;
                final boolean s8NoStarve = (Build.VERSION.SDK_INT <= 28) || ((Build.MODEL == null ? "" : Build.MODEL).toUpperCase(Locale.US).contains("SM-G950"));
                final int chunkFrames = 384;
                int min = AudioTrack.getMinBufferSize(sampleRate, AudioFormat.CHANNEL_OUT_STEREO, AudioFormat.ENCODING_PCM_16BIT);
                // ZPOZDENI ZVUKU: driv 8192 vzorku = ~186 ms zvuku dopredu (proto to
                // "ujizdeni", ktere Rene slysi i na mobilu). Zmenseno na polovinu.
                // Kdyby zvuk zacal praskat, vratime zpet - je to vymena
                // "zpozdeni" za "vypadky".
                int wantedFrames = audioFramesForTier();
                int bufBytes = Math.max(min > 0 ? min * 2 : 0, wantedFrames * 2 * 2);
                if (Build.VERSION.SDK_INT >= 21) {
                    at = new AudioTrack.Builder()
                            .setAudioAttributes(new AudioAttributes.Builder().setUsage(AudioAttributes.USAGE_GAME).setContentType(AudioAttributes.CONTENT_TYPE_MUSIC).build())
                            .setAudioFormat(new AudioFormat.Builder().setEncoding(AudioFormat.ENCODING_PCM_16BIT).setSampleRate(sampleRate).setChannelMask(AudioFormat.CHANNEL_OUT_STEREO).build())
                            .setBufferSizeInBytes(bufBytes)
                            .setTransferMode(AudioTrack.MODE_STREAM)
                            .build();
                } else {
                    at = new AudioTrack(android.media.AudioManager.STREAM_MUSIC, sampleRate, AudioFormat.CHANNEL_OUT_STEREO, AudioFormat.ENCODING_PCM_16BIT, bufBytes, AudioTrack.MODE_STREAM);
                }
                int setFrames = -1;
                if (Build.VERSION.SDK_INT >= 23) {
                    try { setFrames = at.setBufferSizeInFrames(wantedFrames); } catch (Throwable ignored) {}
                }
                try {
                    int fr = (setFrames > 0 ? setFrames : wantedFrames);
                    appendNativeLog("L zvuk PS1: zasobnik " + fr + " vzorku = "
                            + (fr * 1000 / sampleRate) + " ms zpozdeni");
                } catch (Throwable ignored) {}
                ps1CurrentAudioTrack = at;
                short[] buf = new short[chunkFrames * 2];
                int prefillFrames = 0;
                // BUILD2SK20: puvodne 6144 snimku (~139ms typickeho zpozdeni pred
                // prvnim zvukem) - nahlaseno jako FIXNI (ne rostouci) zpozdeni zvuku
                // za obrazem na S8, bez praskani pri soucasnem nastaveni. Snizeno na
                // 4096 (~93ms) - stale bezpecnostni polstar proti praskani, jen mensi.
                // NEDOTCENO: wantedFrames/bufBytes o par radku vyse (velikost bufferu
                // pro BEZICI prehravani, resi odolnost proti vypadkum BEHEM hrani, ne
                // startovni zpozdeni - to je jina vec, nez ktera se resi tady).
                int prefillTarget = s8NoStarve ? 4096 : 3072;
                long prefillDeadline = System.currentTimeMillis() + (s8NoStarve ? 650 : 320);
                while (gen == ps1AudioGen && prefillFrames < prefillTarget && System.currentTimeMillis() < prefillDeadline) {
                    int got = NativePs1CoreBridge.pullAudioSafe(buf, chunkFrames);
                    if (got > 0) {
                        softenPs1Pcm(buf, got * 2);
                        prefillFrames += writePs1AudioTrack(at, buf, got * 2, gen) / 2;
                    } else {
                        try { Thread.sleep(2); } catch (InterruptedException ignored) { break; }
                    }
                }
                at.play();
                int actualFrames = 0;
                try { if (Build.VERSION.SDK_INT >= 23) actualFrames = at.getBufferSizeInFrames(); } catch (Throwable ignored) {}
                appendNativeLog("BUILD2SA5P PS1_AUDIO_START gen=" + gen + " bufBytes=" + bufBytes + " minBytes=" + min + " setFrames=" + setFrames + " actualFrames=" + actualFrames + " prefillFrames=" + prefillFrames + " prefillTarget=" + prefillTarget + " gain=7/8 chunk=" + chunkFrames + " s8NoStarve=" + s8NoStarve + " retryNoSilence=ON previewThrottle=80ms cdAsync=ON biosAudit=ON");
                int idle = 0;
                int writes = 0;
                int loops = 0;
                int underrunLoops = 0;
                while (gen == ps1AudioGen) {
                    int got = NativePs1CoreBridge.pullAudioSafe(buf, chunkFrames);
                    loops++;
                    if (got <= chunkFrames / 4) {
                        underrunLoops++;
                        int retries = s8NoStarve ? 5 : 2;
                        for (int rr = 0; rr < retries && got <= chunkFrames / 2 && gen == ps1AudioGen; rr++) {
                            try { Thread.sleep(s8NoStarve ? 3 : 2); } catch (InterruptedException ignored) { break; }
                            int got2 = NativePs1CoreBridge.pullAudioSafe(buf, chunkFrames);
                            if (got2 > got) got = got2;
                        }
                    }
                    if (got > 0) {
                        softenPs1Pcm(buf, got * 2);
                        int wrote = writePs1AudioTrack(at, buf, got * 2, gen);
                        idle = 0;
                        if (wrote > 0) writes++;
                        if (writes <= 6 || writes % 240 == 0) {
                            int underruns = -1;
                            try { if (Build.VERSION.SDK_INT >= 24) underruns = at.getUnderrunCount(); } catch (Throwable ignored) {}
                            appendNativeLog("BUILD2SA5P PS1_AUDIO_WRITE gen=" + gen + " gotFrames=" + got + " wroteShorts=" + wrote + " writes=" + writes + " loops=" + loops + " underrunLoops=" + underrunLoops + " underruns=" + underruns);
                        }
                    }
                    else {
                        idle++;
                        if (idle > 5000) break;
                        try { Thread.sleep(2); } catch (InterruptedException ignored) { break; }
                    }
                }
            } catch (Throwable t) {
                appendNativeLog("BUILD2SA5P PS1_AUDIO_ERROR " + t.getMessage());
            } finally {
                if (at != null) {
                    try { at.pause(); } catch (Throwable ignored) {}
                    try { at.flush(); } catch (Throwable ignored) {}
                    try { at.stop(); } catch (Throwable ignored) {}
                    try { at.release(); } catch (Throwable ignored) {}
                }
                if (ps1CurrentAudioTrack == at) ps1CurrentAudioTrack = null;
                appendNativeLog("BUILD2SA5P PS1_AUDIO_STOP gen=" + gen + " current=" + ps1AudioGen);
            }
        }, "nap-ps1-audio");
        ps1AudioThread.start();
    }
    private synchronized void stopPs1Audio() {
        final int gen = ++ps1AudioGen;
        AudioTrack at = ps1CurrentAudioTrack;
        ps1CurrentAudioTrack = null;
        if (at != null) {
            try { at.pause(); } catch (Throwable ignored) {}
            try { at.flush(); } catch (Throwable ignored) {}
            try { at.stop(); } catch (Throwable ignored) {}
            try { at.release(); } catch (Throwable ignored) {}
        }
        Thread t = ps1AudioThread;
        if (t != null && t != Thread.currentThread()) {
            try { t.interrupt(); } catch (Throwable ignored) {}
            try { t.join(250); } catch (Throwable ignored) {}
        }
        if (ps1AudioThread == t) ps1AudioThread = null;
        appendNativeLog("BUILD2SA5P PS1_AUDIO_STOP_REQUEST gen=" + gen + " hadTrack=" + (at != null));
    }
    private int writePs1AudioTrack(AudioTrack at, short[] pcm, int shorts, int gen) {
        if (at == null || pcm == null || shorts <= 0) return 0;
        int limit = Math.min(shorts, pcm.length);
        int off = 0;
        while (off < limit && gen == ps1AudioGen) {
            int wr = Build.VERSION.SDK_INT >= 23 ? at.write(pcm, off, limit - off, AudioTrack.WRITE_BLOCKING) : at.write(pcm, off, limit - off);
            if (wr <= 0) break;
            off += wr;
        }
        if (off > 0) napTvWebAudioPush(pcm, 0, off, 44100, "PS1");
        return off;
    }
    private void softenPs1Pcm(short[] pcm, int shorts) {
        if (pcm == null) return;
        int n = Math.min(shorts, pcm.length);
        for (int i = 0; i < n; i++) pcm[i] = (short)((pcm[i] * 7) / 8);
    }
    private void injectPendingSegaGame() {
        try {
            if (pendingSegaGame == null || pendingSegaName == null) return;
            String b64 = Base64.encodeToString(pendingSegaGame, Base64.NO_WRAP);
            String js = "try{napInjectRomBase64(" + jsQuote(pendingSegaName) + "," + jsQuote(b64) + ");}catch(e){console.log('napInject fail '+e);}";
            web.evaluateJavascript(js, null);
            appendNativeLog("BUILD2SA2 SEGA_WEB_ROM_INJECTED name=" + pendingSegaName + " bytes=" + pendingSegaGame.length);
            pendingSegaGame = null; pendingSegaName = null;
        } catch (Throwable t) { appendNativeLog("BUILD2SA2 SEGA_WEB_ROM_INJECT_FAIL " + t.getMessage()); }
    }
    private boolean handleMaybeGameUrl(String url) {
        if (shouldRouteAsSegaDownload(url)) {
            appendNativeLog("BUILD2SA5AQ HANDLE_GAME_URL route=segaArchive url=" + compactUrl(url));
            downloadAndRunSegaArchive(url);
            return true;
        } // BUILD2SA5AB: Sega ZIP nesmi spadnout do 130XE
        if (shouldRouteAsPs1Download(url, "handleMaybeGameUrl")) {
            appendNativeLog("BUILD2SA5AQ HANDLE_GAME_URL route=ps1Remote url=" + compactUrl(url));
            downloadAndRunPs1Remote(url, "handleMaybeGameUrl");
            return true;
        }
        if (openExternalBrowserUrl(url)) return true;
        if (hasSegaExtension(url)) {
            appendNativeLog("BUILD2SA5AQ HANDLE_GAME_URL route=segaRaw url=" + compactUrl(url));
            downloadAndRunSega(url);
            return true;
        } // BUILD2SA2: Sega ma prednost
        if (isGameUrl(url, null, null)) {
            appendNativeLog("BUILD2SA5AQ HANDLE_GAME_URL route=atari url=" + compactUrl(url));
            downloadAndRun(url);
            return true;
        }
        return false;
    }

    private String guessDownloadName(String url, String contentDisposition) {
        try {
            if (contentDisposition != null) {
                String cd = contentDisposition;
                int p = cd.toLowerCase().indexOf("filename=");
                if (p >= 0) {
                    String n = cd.substring(p + 9).replace("\"", "").replace("'", "").trim();
                    int semi = n.indexOf(';');
                    if (semi >= 0) n = n.substring(0, semi).trim();
                    if (n.length() > 0) return n;
                }
            }
            Uri u = Uri.parse(url);
            String path = u.getLastPathSegment();
            if (path != null && path.length() > 0) return URLDecoder.decode(path, "UTF-8");
        } catch (Exception ignored) {}
        return "stazeno_z_webu.zip";
    }

    private void injectGameLinkBridge() {
        String js = "(function(){"
                + "if(window.__AH_GAME_BRIDGE)return;window.__AH_GAME_BRIDGE=1;"
                + "document.addEventListener('click',function(e){"
                + "var a=e.target;while(a&&a.tagName!=='A')a=a.parentElement;if(!a||!a.href)return;"
                + "var h=a.href;"
                + "if(/drive\\.google\\.com|drive\\.usercontent\\.google\\.com/i.test(h)||/\\.(xex|zip|atr|com|exe|gen|md|smd|sms|68k|sgd|cue|bin|iso|img|pbp|chd)([?#].*)?$/i.test(h)||/\\.(xex|zip|atr|com|exe|gen|md|smd|sms|68k|sgd|cue|bin|iso|img|pbp|chd)/i.test(h)){"
                + "e.preventDefault();e.stopPropagation();try{AHNET.runGameUrl(h);}catch(err){location.href=h;}"
                + "}"
                + "},true);"
                + "document.addEventListener('click',function(e){"
                + "if(e.defaultPrevented)return;"
                + "var n=e.target,fig=null;while(n&&n!==document){if(n.tagName==='FIGURE'||(n.className&&String(n.className).indexOf('wp-block-image')>=0)){fig=n;break;}n=n.parentElement;}"
                + "if(!fig)return;var links=fig.getElementsByTagName('a');for(var i=0;i<links.length;i++){var h=links[i].href||'';"
                + "if(/drive\\.google\\.com|drive\\.usercontent\\.google\\.com/i.test(h)||/\\.(xex|zip|atr|com|exe|gen|md|smd|sms|68k|sgd|cue|bin|iso|img|pbp|chd)([?#].*)?$/i.test(h)||/\\.(xex|zip|atr|com|exe|gen|md|smd|sms|68k|sgd|cue|bin|iso|img|pbp|chd)/i.test(h)){"
                + "e.preventDefault();e.stopPropagation();try{AHNET.runGameUrl(h);}catch(err){location.href=h;}return;"
                + "}}"
                + "},true);"
                + "})();";
        web.evaluateJavascript(js, null);
    }

    private void downloadAndRun(final String url) {
        new Thread(() -> {
            try {
                if (!markAtariHelpRequestAllowed(url, "downloadGame")) return;
                FetchResult fetched = fetchUrlBytes(url, 16 * 1024 * 1024, "downloadGame");
                final String cdName = fetched.contentDisposition;
                final byte[] data = fetched.data;
                final String name = guessDownloadName(url, cdName);
                appendNativeLog("BUILD2SA5AF WEB_GAME_DOWNLOADED name=" + name + " bytes=" + data.length + " via=" + compactUrl(fetched.via));
                // BUILD2SA2B: Reneho web umi hostovat jen ZIPy. Kouknem DOVNITR zipu:
                // kdyz je uvnitr Sega ROM (.gen/.md/.smd/.sms), rozbalime a posleme
                // do EMU SEGA. Jinak jede stara Atari cesta beze zmeny.
                // BUILD2SA8: klik na BIOS ZIP na strankach = automaticka instalace.
                // scph*.bin se rovnou adoptuji do systemove slozky jadra a kopie zipu
                // jde do Download/AtariHelp (prezije preinstalaci; SA7 auto-adopt ji najde).
                int biosCount = 0;
                try {
                    java.util.zip.ZipInputStream bzz = new java.util.zip.ZipInputStream(new java.io.ByteArrayInputStream(data));
                    java.util.zip.ZipEntry be;
                    java.io.File biosSys = new java.io.File(getFilesDir(), "ps1_system");
                    while ((be = bzz.getNextEntry()) != null) {
                        if (be.isDirectory()) continue;
                        String bn = be.getName();
                        int bsl = bn.lastIndexOf('/'); if (bsl >= 0) bn = bn.substring(bsl + 1);
                        bn = bn.toLowerCase(Locale.US);
                        if (bn.startsWith("scph") && bn.endsWith(".bin")) {
                            if (!biosSys.exists()) biosSys.mkdirs();
                            java.io.ByteArrayOutputStream bo = new java.io.ByteArrayOutputStream();
                            byte[] bb = new byte[16384]; int bnn;
                            while ((bnn = bzz.read(bb)) > 0 && bo.size() <= 524288) bo.write(bb, 0, bnn);
                            if (bo.size() == 524288) {
                                java.io.FileOutputStream bf = new java.io.FileOutputStream(new java.io.File(biosSys, bn));
                                bo.writeTo(bf); bf.close(); biosCount++;
                            }
                        }
                        bzz.closeEntry();
                    }
                    bzz.close();
                } catch (Throwable ignored) {}
                if (biosCount > 0) {
                    try {
                        java.io.File keep = new java.io.File(getPublicAtariHelpDownloadsDir(), safeFileName(name));
                        java.io.FileOutputStream kf = new java.io.FileOutputStream(keep);
                        kf.write(data); kf.close();
                    } catch (Throwable ignored) {}
                    final int bc = biosCount;
                    appendNativeLog("BUILD2SA8 PS1_BIOS_INSTALLED_FROM_WEB count=" + bc + " zip=" + name);
                    ui.post(() -> { try { setPs1RemoteStatus("PS1_BIOS_INSTALLED count=" + bc + " - SONY logo pojede u dalsi hry"); } catch (Throwable ignored) {} });
                    return;
                }
                final SegaExtract sega = extractSegaRomFromMaybeZip(name, data);
                if (sega != null && sega.data != null && sega.data.length > 0) {
                    appendNativeLog("BUILD2SA5AF ZIP_CONTAINS_SEGA name=" + sega.name + " bytes=" + sega.data.length + " -> EMU_SEGA");
                    ui.post(() -> openSegaRomBytes(sega.data, sega.name, "genericZipInspect"));
                    return;
                }
                final AtariExtract atari = extractAtariPayloadFromMaybeZip(name, data);
                final byte[] atariData = (atari != null && atari.data != null && atari.data.length > 0) ? atari.data : data;
                final String atariName = (atari != null && atari.name != null && atari.name.length() > 0) ? atari.name : name;
                if (atari != null && atari.data != null && atari.data.length > 0) {
                    appendNativeLog("BUILD2SA5AF ZIP_CONTAINS_ATARI name=" + atari.name + " bytes=" + atari.data.length + " -> EMU_130XE");
                }
                ui.post(() -> {
                    queueAtariGameFor130xe(atariName, atariData, "netDownload");
                });
            } catch (Exception ex) {
                ui.post(() -> {
                    try {
                        appendNativeLog("BUILD2SA5AF WEB_GAME_DOWNLOAD_FAIL noEmuFallback " + safeMsg(ex));
                        final String msg = ex.getMessage() == null ? "neznamá chyba" : ex.getMessage();
                        String curErr = web == null ? null : web.getUrl();
                        if (curErr != null && curErr.startsWith(EMU_URL)) {
                            web.postDelayed(() -> web.evaluateJavascript("AHJAVA_ERROR(" + jsQuote("NET HRY: download selhal - " + msg) + ")", null), 500);
                        }
                    } catch (Exception ignored) {}
                });
            }
        }).start();
    }

    private void queueAtariGameFor130xe(String name, byte[] data, String reason) {
        if (data == null || data.length == 0 || web == null) {
            appendNativeLog("BUILD2SA5AG EMU130_QUEUE_SKIP_EMPTY reason=" + reason + " name=" + name);
            return;
        }
        pendingGame = data;
        pendingName = (name == null || name.length() == 0) ? "atarihelp_game.xex" : name;
        appendNativeLog("BUILD2SA5AG EMU130_QUEUE reason=" + reason + " name=" + pendingName + " bytes=" + pendingGame.length);
        String cur = web.getUrl();
        if (cur != null && cur.startsWith(EMU_URL)) {
            schedulePendingAtariGameInjection(reason + ":alreadyOn130xe");
        } else {
            appendNativeLog("BUILD2SA5AJ EMU130_OPEN_ONCE reason=" + reason + " from=" + compactUrl(cur) + " settleMs=" + ATARI_NET_OPEN_SETTLE_MS);
            web.loadUrl(EMU_URL);
            final int seq = ++pendingGameInjectSeq;
            appendNativeLog("BUILD2SA5AJ EMU130_INJECT_DELAYED_AFTER_OPEN reason=" + reason + " seq=" + seq + " name=" + pendingName + " bytes=" + pendingGame.length);
            ui.postDelayed(() -> tryInjectPendingAtariGame(seq, reason + ":afterOpenDelay", 0), ATARI_NET_OPEN_SETTLE_MS);
            ui.postDelayed(() -> commitPendingAtariGameInjection(seq, reason + ":lateDirectFallback"), ATARI_NET_INJECT_FALLBACK_MS);
        }
    }

    private void schedulePendingAtariGameInjection(final String reason) {
        if (pendingGame == null || web == null) return;
        final int seq = ++pendingGameInjectSeq;
        appendNativeLog("BUILD2SA5AG EMU130_INJECT_SCHEDULE reason=" + reason + " seq=" + seq + " name=" + pendingName + " bytes=" + pendingGame.length);
        ui.postDelayed(() -> tryInjectPendingAtariGame(seq, reason, 0), ATARI_NET_INJECT_RETRY_MS);
        ui.postDelayed(() -> fallbackInjectPendingAtariGame(seq, reason), ATARI_NET_INJECT_FALLBACK_MS);
    }

    private void tryInjectPendingAtariGame(final int seq, final String reason, final int attempt) {
        if (seq != pendingGameInjectSeq || pendingGame == null || web == null) return;
        String cur = web.getUrl();
        if (cur == null || !cur.startsWith(EMU_URL)) {
            if (attempt < ATARI_NET_INJECT_MAX_ATTEMPTS) {
                if (attempt == 0 || attempt == 8 || attempt == 20 || attempt == 36) {
                    appendNativeLog("BUILD2SA5AJ EMU130_WAIT_URL reason=" + reason + " attempt=" + attempt + " cur=" + compactUrl(cur));
                }
                ui.postDelayed(() -> tryInjectPendingAtariGame(seq, reason, attempt + 1), ATARI_NET_INJECT_RETRY_MS);
            } else {
                appendNativeLog("BUILD2SA5AJ EMU130_URL_TIMEOUT_TRY_DIRECT reason=" + reason + " cur=" + compactUrl(cur));
                commitPendingAtariGameInjection(seq, reason + ":urlTimeoutDirect");
            }
            return;
        }
        try {
            web.evaluateJavascript("(typeof window.AHRECV_BEGIN==='function'&&typeof window.AHRECV_PART==='function'&&typeof window.AHRECV_END==='function')", value -> {
                if (seq != pendingGameInjectSeq || pendingGame == null || web == null) return;
                boolean ready = "true".equals(String.valueOf(value)) || "\"true\"".equals(String.valueOf(value));
                if (ready) {
                    appendNativeLog("BUILD2SA5AH EMU130_INJECT_READY_SETTLE reason=" + reason + " attempt=" + attempt + " name=" + pendingName + " bytes=" + pendingGame.length + " settleMs=" + ATARI_NET_INJECT_SETTLE_MS);
                    ui.postDelayed(() -> commitPendingAtariGameInjection(seq, reason + ":readySettled"), ATARI_NET_INJECT_SETTLE_MS);
                    return;
                }
                if (attempt < ATARI_NET_INJECT_MAX_ATTEMPTS) {
                    if (attempt == 0 || attempt == 8 || attempt == 20) {
                        appendNativeLog("BUILD2SA5AG EMU130_INJECT_WAIT reason=" + reason + " attempt=" + attempt + " ready=" + value);
                    }
                    ui.postDelayed(() -> tryInjectPendingAtariGame(seq, reason, attempt + 1), ATARI_NET_INJECT_RETRY_MS);
                } else {
                    appendNativeLog("BUILD2SA5AG EMU130_INJECT_TIMEOUT reason=" + reason + " name=" + pendingName + " bytes=" + pendingGame.length);
                }
            });
        } catch (Throwable t) {
            if (attempt < ATARI_NET_INJECT_MAX_ATTEMPTS) {
                ui.postDelayed(() -> tryInjectPendingAtariGame(seq, reason, attempt + 1), ATARI_NET_INJECT_RETRY_MS);
            } else {
                appendNativeLog("BUILD2SA5AG EMU130_INJECT_JS_ERROR reason=" + reason + " " + safeMsg(t));
            }
        }
    }

    private void commitPendingAtariGameInjection(final int seq, final String reason) {
        if (seq != pendingGameInjectSeq || pendingGame == null || web == null) return;
        String cur = web.getUrl();
        boolean allowDirect = reason != null && reason.toLowerCase(Locale.US).contains("direct");
        if (cur == null || !cur.startsWith(EMU_URL)) {
            appendNativeLog("BUILD2SA5AJ EMU130_COMMIT_URL_MISMATCH reason=" + reason + " cur=" + compactUrl(cur) + " allowDirect=" + allowDirect);
            if (!allowDirect) return;
        }
        try {
            byte[] data = pendingGame;
            String name = pendingName;
            appendNativeLog("BUILD2SA5AH EMU130_INJECT_COMMIT reason=" + reason + " name=" + name + " bytes=" + data.length);
            injectGame(name, data);
            pendingGame = null;
            pendingName = null;
        } catch (Throwable t) {
            appendNativeLog("BUILD2SA5AH EMU130_INJECT_COMMIT_ERROR reason=" + reason + " " + safeMsg(t));
        }
    }

    private void fallbackInjectPendingAtariGame(final int seq, final String reason) {
        appendNativeLog("BUILD2SA5AH EMU130_INJECT_FALLBACK_CHECK reason=" + reason + " seq=" + seq);
        commitPendingAtariGameInjection(seq, reason + ":fallback");
    }

    private String jsQuote(String text) {
        if (text == null) text = "";
        return "'" + text.replace("\\", "\\\\").replace("'", "\\'").replace("\n", " ").replace("\r", " ") + "'";
    }

    private void injectGame(String name, byte[] data) {
        appendNativeLog("BUILD2SA5AG EMU130_INJECT_SEND name=" + name + " bytes=" + (data == null ? 0 : data.length));
        web.evaluateJavascript("AHRECV_BEGIN(" + jsQuote(name) + ")", null);
        String b64 = Base64.encodeToString(data, Base64.NO_WRAP);
        int parts = 0;
        for (int i = 0; i < b64.length(); i += 262144) {
            String part = b64.substring(i, Math.min(i + 262144, b64.length()));
            web.evaluateJavascript("AHRECV_PART('" + part + "')", null);
            parts++;
        }
        web.evaluateJavascript("AHRECV_END()", null);
        appendNativeLog("BUILD2SA5AG EMU130_INJECT_SENT name=" + name + " parts=" + parts + " b64Chars=" + b64.length());
    }

    private void injectAudio(String name, byte[] data) {
        web.evaluateJavascript("AHLOCAL_AUDIO_BEGIN(" + jsQuote(name) + ")", null);
        String b64 = Base64.encodeToString(data, Base64.NO_WRAP);
        for (int i = 0; i < b64.length(); i += 262144) {
            String part = b64.substring(i, Math.min(i + 262144, b64.length()));
            web.evaluateJavascript("AHLOCAL_AUDIO_PART('" + part + "')", null);
        }
        web.evaluateJavascript("AHLOCAL_AUDIO_END()", null);
    }

    private void injectMp3Uri(String name, Uri uri) {
        if (web == null || uri == null) return;
        try {
            try {
                grantUriPermission(getPackageName(), uri, Intent.FLAG_GRANT_READ_URI_PERMISSION);
            } catch (Throwable ignored) {}
            String js = "if(typeof AHLOCAL_MP3_URI==='function'){AHLOCAL_MP3_URI("
                    + jsQuote(name) + "," + jsQuote(uri.toString()) + ");}"
                    + "else if(typeof AHJAVA_ERROR==='function'){AHJAVA_ERROR('MP3 player bridge neni pripraveny');}";
            web.evaluateJavascript(js, null);
            appendNativeLog("BUILD2SA13C7 PLAYER_MP3_URI name=" + name + " uri=" + compactUrl(uri.toString()));
        } catch (Throwable t) {
            appendNativeLog("BUILD2SA13C7 PLAYER_MP3_URI_FAIL " + safeMsg(t));
        }
    }

    private void injectText(String name, byte[] data) {
        web.evaluateJavascript("AHLOCAL_TEXT_BEGIN(" + jsQuote(name) + ")", null);
        String b64 = Base64.encodeToString(data, Base64.NO_WRAP);
        for (int i = 0; i < b64.length(); i += 262144) {
            String part = b64.substring(i, Math.min(i + 262144, b64.length()));
            web.evaluateJavascript("AHLOCAL_TEXT_PART('" + part + "')", null);
        }
        web.evaluateJavascript("AHLOCAL_TEXT_END()", null);
    }

    private String ps1BiosAudit(File sysDir) {
        try {
            if (sysDir == null) return "sysdir=null";
            StringBuilder sb = new StringBuilder();
            sb.append("sysdir=").append(sysDir.getAbsolutePath());
            if (!sysDir.exists()) return sb.append(" missing").toString();
            String[] known = new String[]{
                    "PSXONPSP660.bin", "psxonpsp660.bin",
                    "scph101.bin", "scph7001.bin", "scph5501.bin", "scph1001.bin",
                    "scph5500.bin", "scph5502.bin", "scph7003.bin", "scph7502.bin"
            };
            int found = 0;
            StringBuilder knownFound = new StringBuilder();
            for (String name : known) {
                File f = new File(sysDir, name);
                if (f.exists() && f.isFile()) {
                    found++;
                    if (knownFound.length() > 0) knownFound.append(",");
                    knownFound.append(f.getName()).append(":").append(f.length());
                }
            }
            File[] bins = sysDir.listFiles(new java.io.FilenameFilter() {
                @Override public boolean accept(File dir, String name) {
                    return name != null && name.toLowerCase(Locale.US).endsWith(".bin");
                }
            });
            sb.append(" knownFound=").append(found);
            if (knownFound.length() > 0) sb.append(" known=").append(knownFound);
            sb.append(" binCount=").append(bins == null ? -1 : bins.length);
            if (bins != null && bins.length > 0) {
                sb.append(" bins=");
                int max = Math.min(8, bins.length);
                for (int i = 0; i < max; i++) {
                    if (i > 0) sb.append(",");
                    sb.append(bins[i].getName()).append(":").append(bins[i].length());
                }
                if (bins.length > max) sb.append(",...");
            }
            return sb.toString();
        } catch (Throwable t) {
            return "audit_error=" + safeMsg(t);
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode == PICK_AUDIO_PERMISSION) {
            boolean ok = napPlayerHasAudioPermission();
            appendNativeLog("BUILD2SA13C14 PLAYER_AUDIO_PERMISSION_RESULT ok=" + ok + " sdk=" + Build.VERSION.SDK_INT);
            if (web != null) {
                try {
                    web.evaluateJavascript("try{if(typeof AHREFRESH_LOCAL_LIBRARY==='function')AHREFRESH_LOCAL_LIBRARY();}catch(e){}", null);
                } catch (Throwable ignored) {}
            }
        }
    }

    @Override
    protected void onActivityResult(int req, int res, Intent data) {
        super.onActivityResult(req, res, data);
        if (req == PICK_TV_WEB_SCREEN) {
            String pendingScreenUrl = napTvWebPendingScreenUrl;
            napTvWebPendingScreenUrl = null;
            if (res == RESULT_OK && data != null) {
                napTvWebStartSystemMirror(res, data);
                if (pendingScreenUrl != null && pendingScreenUrl.length() > 0 && napTvWebSystemMirrorActive) {
                    ui.postDelayed(() -> openRawExternalBrowserUrl(pendingScreenUrl), 650);
                } else if (pendingScreenUrl != null && pendingScreenUrl.length() > 0) {
                    appendNativeLog("BUILD2SA13C12 SCREEN_MIRROR_PENDING_URL_NOT_OPENED mirrorActive=false");
                }
            } else {
                napTvWebSystemMirrorRequested = false;
                appendNativeLog("BUILD2SA13C9 SCREEN_MIRROR_PERMISSION_DENIED res=" + res);
            }
            return;
        }
        if (req == PICK_PS1_GAME) { // BUILD2SA2
            if (res != RESULT_OK || data == null || data.getData() == null) { ps1LastBootResult = "PS1_PICK_CANCELLED"; return; }
            final Uri uri = data.getData();
            final String pickedName = safeFileName(getDisplayName(uri));
            new Thread(() -> {
                int bootGen = 0;
                try {
                    android.os.ParcelFileDescriptor pfd = getContentResolver().openFileDescriptor(uri, "r");
                    if (pfd == null) { ps1LastBootResult = "PS1_PICK_FAIL pfd=null"; return; }
                    synchronized (MainActivity.class) {
                        try { if (ps1GamePfd != null) ps1GamePfd.close(); } catch (Throwable ignored) {}
                        ps1GamePfd = pfd;
                    }
                    synchronized (MainActivity.this) {
                        bootGen = ++ps1LifecycleGen;
                        ps1BootActive = true;
                        ps1SessionActive = false;
                    }
                    String fdPath = "/proc/self/fd/" + pfd.getFd();
                    java.io.File sysDir = new java.io.File(getFilesDir(), "ps1_system");
                    java.io.File saveDir = new java.io.File(getFilesDir(), "ps1_saves");
                    if (!sysDir.exists()) sysDir.mkdirs();
                    if (!saveDir.exists()) saveDir.mkdirs();
                    ps1CurrentGameLabel = pickedName;
                    stopPs1Audio(); // BUILD2SA3B: cisty audio restart pri prepnuti PS1 hry
                    ps1LastBootResult = "PS1_BOOTING...";
                    appendNativeLog("BUILD2SA5P PS1_BIOS_AUDIT " + ps1BiosAudit(sysDir));
                    ps1EnsureBios(sysDir); // BUILD2SA7
                    // Drive se tu volalo bootSafe(), ktere hru sice nahralo, ale
                    // NEPRIPRAVILO grafiku, zvuk ani vlakno emulace - proto po
                    // stisku ISO CD zustala na obrazovce viset predchozi vec
                    // (menu BIOSu). Ted jde hra tou samou cestou jako BIOS.
                    ps1LastBootResult = NativePs1CoreBridge.bootGameSafe(
                            sysDir.getAbsolutePath(), saveDir.getAbsolutePath(), fdPath);
                    appendNativeLog("PS1_HRA_DO_MONITORU cesta=" + fdPath
                            + " vysledek=" + ps1LastBootResult);
                    boolean ok = ps1LastBootResult != null && ps1LastBootResult.startsWith("PS1_HRA_OK");
                    boolean stillWanted = ok && bootGen == ps1LifecycleGen && ps1BootActive;
                    ps1BootActive = false;
                    if (stillWanted) {
                        ps1SessionActive = true;
                        startPs1Audio(); // BUILD2SA3
                        ps1ActivateNativeView();
                    } else {
                        ps1SessionActive = false;
                        stopPs1Audio();
                        if (ok) {
                            try { NativePs1CoreBridge.stopSafe(); } catch (Throwable ignored) {}
                            ps1ClearJsPreview();
                            closePs1GamePfdQuietly();
                            ps1LastBootResult = "PS1_BOOT_CANCELLED_AFTER_LEAVE";
                        }
                    }
                } catch (Throwable t) {
                    ps1BootActive = false;
                    ps1SessionActive = false;
                    stopPs1Audio();
                    closePs1GamePfdQuietly();
                    ps1LastBootResult = "PS1_BOOT_EXCEPTION " + safeMsg(t);
                }
            }).start();
            return;
        }
        if (req == PICK_FILE && pendingChooser != null) {
            Uri[] out = (res == RESULT_OK && data != null && data.getData() != null)
                    ? new Uri[]{ data.getData() } : null;
            pendingChooser.onReceiveValue(out);
            pendingChooser = null;
            return;
        }
        if (req == PICK_BRIDGE) {
            if (res == RESULT_OK && data != null) {
                try {
                    int max = ("audio".equals(pendingBridgeKind) || "mp3".equals(pendingBridgeKind)) ? 64 * 1024 * 1024 : ("text".equals(pendingBridgeKind) ? 2 * 1024 * 1024 : 16 * 1024 * 1024);
                    if ("mp3".equals(pendingBridgeKind)) {
                        web.evaluateJavascript("if(typeof AHLOCAL_MP3_PLAYLIST_BEGIN==='function')AHLOCAL_MP3_PLAYLIST_BEGIN()", null);
                        ClipData clip = data.getClipData();
                        if (clip != null && clip.getItemCount() > 0) {
                            for (int k = 0; k < clip.getItemCount(); k++) {
                                Uri uri = clip.getItemAt(k).getUri();
                                if (uri == null) continue;
                                String name = getDisplayName(uri);
                                injectMp3Uri(name, uri);
                            }
                        } else if (data.getData() != null) {
                            Uri uri = data.getData();
                            String name = getDisplayName(uri);
                            injectMp3Uri(name, uri);
                        }
                        web.evaluateJavascript("if(typeof AHLOCAL_MP3_PLAYLIST_END==='function')AHLOCAL_MP3_PLAYLIST_END()", null);
                    } else if (data.getData() != null) {
                        Uri uri = data.getData();
                        String name = getDisplayName(uri);
                        byte[] bytes = readUriBytes(uri, max);
                        if ("audio".equals(pendingBridgeKind)) injectAudio(name, bytes);
                        else if ("text".equals(pendingBridgeKind)) injectText(name, bytes);
                        else injectGame(name, bytes);
                    }
                } catch (Exception e) {
                    web.evaluateJavascript("AHJAVA_ERROR(" + jsQuote(e.getMessage()) + ")", null);
                }
            }
            pendingBridgeKind = null;
        }
    }

    @Override
    public void onBackPressed() {
        if (web != null && web.canGoBack()) {
            stopNativeInPlaceHard("backPressedBeforeGoBack");
            stopPs1SessionHard("backPressedBeforeGoBack");
            web.goBack();
        }
        else super.onBackPressed();
    }

    @Override
    protected void onPause() {
        super.onPause();
        if (ps1GameWindowOwnsCore) {
            // Jadro si prevzalo okno hry - appka ho NESMI zastavovat.
            appendNativeLog("PS1_PAUZA_APPKY jadro patri oknu hry, nesaham na nej");
        } else {
            stopNativeInPlaceHard("activityPause");
            stopPs1SessionHard("activityPause");
        }
        if (web != null) web.onPause();
    }

    @Override
    protected void onDestroy() {
        try { napTvWebStop("activityDestroy"); } catch (Throwable ignored) {} // BUILD2SA13C
        try { if (napTvPresentation != null) napTvPresentation.dismiss(); } catch (Throwable ignored) {} // BUILD2SA13
        try { if (napDisplayManager != null) napDisplayManager.unregisterDisplayListener(napTvListener); } catch (Throwable ignored) {}
        stopNativeInPlaceHard("activityDestroy");
        stopPs1SessionHard("activityDestroy");
        super.onDestroy();
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (web != null) web.onResume();
        // ====== NÁVRAT Z OKNA HRY ======
        // Kdyz se sem vracime a PS1 relace je jeste "zapnuta", znamena to, ze
        // se okno hry zavrelo (jadro uz se vypnulo samo pres core_shutdown).
        // Appka si ale dosud drzela stav dal: myslela si, ze PS1 porad bezi,
        // zustala na ovladaci obrazovce a slo to spravit jen restartem appky.
        // Tady to uklidime - vratime se do vychoziho stavu PS1 obrazovky.
        if (ps1GameWindowOwnsCore) {
            // ====== POZOR: NESAHAT NA JADRO ======
            // Jadro si vypina okno hry samo (core_shutdown). Kdyz jsme sem
            // volali stopPs1SessionHard(), zacala appka zastavovat TOTEZ
            // vlakno emulace ve stejnou chvili jako okno hry - obe strany na
            // sebe cekaly a appka ZTUHLA. V logu to bylo videt na poradi:
            //   21:30:57.214  PS1_UKLID_PO_NAVRATU   (appka zastavuje)
            //   21:30:57.222  PS1_SESSION_STOP
            //                 CESTA_A VYPINAM JADRO  (a teprve ted okno hry)
            // Uklidime tedy JEN stav na strane appky, jadra se nedotkneme.
            ps1GameWindowOwnsCore = false;
            appendNativeLog("PS1_UKLID_PO_NAVRATU duvod=zavreno okno hry (jadro si vypina samo)");
            ps1BootActive = false;
            ps1SessionActive = false;
            try { stopPs1Audio(); } catch (Throwable ignored) {}
            try { closePs1GamePfdQuietly(); } catch (Throwable ignored) {}
            try { ps1ClearJsPreview(); } catch (Throwable ignored) {}
            try { ps1DeactivateNativeView(); } catch (Throwable ignored) {}
            tvCoreHadFrame = false;   // TV uz nema drzet posledni snimek hry
            ps1BiosRunning = false;   // po navratu se BIOS nastartuje znovu
            ps1LastBootResult = "PS1_STOPPED navrat z okna hry";
        } else if (ps1SessionActive || ps1BootActive) {
            appendNativeLog("PS1_UKLID_PO_NAVRATU duvod=zbyla relace appky");
            try { stopPs1SessionHard("uklid pri navratu"); } catch (Throwable ignored) {}
        }
        // ====== OTACENI SE NESMI ZAMYKAT ======
        // Drive jsem tady vynucoval portret (aby se po navratu z hry PS1
        // neskoncilo na ovladaci obrazovce). Jenze orientace se nastavuje
        // pro CELOU aplikaci - a tim se zamklo i Atari, Sega a DJ pult,
        // ktere se pak nedaly pretocit na sirku. Moje chyba.
        // Necháváme orientaci volnou; telefon si rozhoduje sam.
    }
}
