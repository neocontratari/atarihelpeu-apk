package android.media;
/** B306 test: nahrada pro javac (kdyby se zaloha spustila, nic nehraje). */
public class AudioTrack {
    public static final int MODE_STREAM = 1;
    public static int getMinBufferSize(int sr, int ch, int enc) { return 4096; }
    public static class Builder {
        public Builder setAudioAttributes(AudioAttributes a) { return this; }
        public Builder setAudioFormat(AudioFormat f) { return this; }
        public Builder setBufferSizeInBytes(int b) { return this; }
        public Builder setTransferMode(int m) { return this; }
        public AudioTrack build() { return new AudioTrack(); }
    }
    public void play() {}
    public void pause() {}
    public void flush() {}
    public void release() {}
    public int write(short[] d, int o, int n) { return n; }
    public int getPlaybackHeadPosition() { return 0; }
}
