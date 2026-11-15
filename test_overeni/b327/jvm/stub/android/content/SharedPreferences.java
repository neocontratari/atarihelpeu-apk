package android.content;
/** B326 / B327 test: nahrada Androidu - jen to, co pouziva NapJazyk. */
public interface SharedPreferences {
    interface Editor {
        Editor putString(String k, String v);
        void apply();
    }
    String getString(String k, String def);
    Editor edit();
}
