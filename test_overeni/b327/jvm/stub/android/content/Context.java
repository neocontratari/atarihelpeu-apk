package android.content;
/** B326 / B327 test: nahrada Androidu - jen to, co pouziva NapJazyk. */
public abstract class Context {
    public static final int MODE_PRIVATE = 0;
    public abstract Context getApplicationContext();
    public abstract SharedPreferences getSharedPreferences(String jmeno, int rezim);
}
