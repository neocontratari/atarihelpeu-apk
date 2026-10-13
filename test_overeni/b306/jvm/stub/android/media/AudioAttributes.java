package android.media;
/** B306 test: nahrada pro javac (zaloha AudioTrack se v testu nespousti). */
public class AudioAttributes {
    public static final int USAGE_GAME = 14;
    public static final int CONTENT_TYPE_MUSIC = 2;
    public static class Builder {
        public Builder setUsage(int u) { return this; }
        public Builder setContentType(int c) { return this; }
        public AudioAttributes build() { return new AudioAttributes(); }
    }
}
