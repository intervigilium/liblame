/* Lame.java
   A port of LAME for Android

   Copyright (c) 2010 Ethan Chen

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

package net.sourceforge.lame;

import java.io.BufferedInputStream;
import java.io.IOException;
import java.io.InputStream;

public class Lame {
    private static final int MP3_BUFFER_SIZE = 1024;
    public static final int LAME_PRESET_DEFAULT = 0;
    public static final int LAME_PRESET_MEDIUM = 1;
    public static final int LAME_PRESET_STANDARD = 2;
    public static final int LAME_PRESET_EXTREME = 3;

    private static final String LAME_LIB = "lame";

    static {
        System.loadLibrary(LAME_LIB);
    }

    public static native int initializeEncoder(int sampleRate, int numChannels);

    public static native void setEncoderPreset(int preset);

    public static native int encode(short[] leftChannel,
            short[] rightChannel, int channelSamples, byte[] mp3Buffer,
            int bufferSize);

    public static native int flushEncoder(byte[] mp3Buffer, int bufferSize);

    public static native int closeEncoder();

    public static native int initializeDecoder();

    public static native int getDecoderSampleRate();

    public static native int getDecoderChannels();

    public static native int getDecoderDelay();

    public static native int getDecoderPadding();

    public static native int getDecoderTotalFrames();

    public static native int getDecoderFrameSize();

    public static native int getDecoderBitrate();

    public static int configureDecoder(BufferedInputStream input) throws IOException {
        int size = 100;
        byte[] buf = new byte[size];

        do {
            size = input.read(buf);
            if (nativeConfigureDecoder(buf, size) == 0) {
                return 0;
            }
        } while(size > 0);
        return -1;
    }

    private static native int nativeConfigureDecoder(byte[] inputBuffer, int bufferSize);

    public static int decodeFrame(InputStream input,
            short[] pcmLeft, short[] pcmRight) throws IOException {
        int len = 0;
        int samplesRead = 0;
        byte[] buf = new byte[MP3_BUFFER_SIZE];

        // check for buffered data
        samplesRead = nativeDecodeFrame(buf, len, pcmLeft, pcmRight);
        if (samplesRead != 0) {
            return samplesRead;
        }
        while (true) {
            len = input.read(buf);
            if (len == -1) {
                // finished reading input buffer, check for buffered data
                samplesRead = nativeDecodeFrame(buf, len, pcmLeft, pcmRight);
                break;
            }
            samplesRead = nativeDecodeFrame(buf, len, pcmLeft, pcmRight);
            if (samplesRead > 0) {
                break;
            }
        }
        return samplesRead;
    }

    private static native int nativeDecodeFrame(byte[] inputBuffer, int bufferSize,
            short[] pcmLeft, short[] pcmRight);

    public static native int closeDecoder();
}