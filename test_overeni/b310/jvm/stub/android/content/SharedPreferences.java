package android.content;
/** B310 test: nahrada Androidu - jen to, co pouziva NapPostup. */
public interface SharedPreferences {
    interface Editor {
        Editor putBoolean(String k, boolean v);
        Editor putInt(String k, int v);
        Editor putLong(String k, long v);
        void apply();
    }
    boolean getBoolean(String k, boolean def);
    int getInt(String k, int def);
    long getLong(String k, long def);
    Editor edit();
}
