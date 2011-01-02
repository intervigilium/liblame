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
        int id3Length, aidLength;
        byte[] buf = new byte[size];

        if (input.read(buf, 0, 4) != 4) {
            return -1;
        }
        if (isId3Header(buf)) {
            // ID3 header found, skip past it
            if (input.read(buf, 0, 6) != 6) {
                return -1;
            }
            buf[2] &= 0x7F;
            buf[3] &= 0x7F;
            buf[4] &= 0x7F;
            buf[5] &= 0x7F;
            id3Length = (((((buf[2] << 7) + buf[3]) << 7) + buf[4]) << 7) + buf[5];
            input.skip(id3Length);
            if (input.read(buf, 0, 4) != 4) {
                return -1;
            }
        }
        if (isAidHeader(buf)) {
            // AID header found, skip past it too
            if (input.read(buf, 0, 2) != 2) {
                return -1;
            }
            aidLength = buf[0] + 256 * buf[1];
            input.skip(aidLength);
            if (input.read(buf, 0, 4) != 4) {
                return -1;
            }
        }
        while (!isMp123SyncWord(buf)) {
         // search for MP3 syncword one byte at a time
            for (int i = 0; i < 3; i++) {
                buf[i] = buf[i + 1];
            }
            buf[3] = (byte) input.read();
        }

        do {
            size = input.read(buf);
            if (nativeConfigureDecoder(buf, size) == 0) {
                return 0;
            }
        } while(size > 0);
        return -1;
    }

    private static boolean isId3Header(byte[] buf) {
        return (buf[0] == 'I' &&
                buf[1] == 'D' &&
                buf[2] == '3');
    }

    private static boolean isAidHeader(byte[] buf) {
        return (buf[0] == 'A' &&
                buf[1] == 'i' &&
                buf[2] == 'D' &&
                buf[3] == '\1');
    }

    private static boolean isMp123SyncWord(byte[] buf) {
        // function taken from LAME to identify MP3 syncword
        char[] abl2 = new char[] { 0, 7, 7, 7, 0, 7, 0, 0, 0, 0, 0, 8, 8, 8, 8, 8 };
        if ((buf[0] & 0xFF) != 0xFF) {
            return false;
        }
        if ((buf[1] & 0xE0) != 0xE0) {
            return false;
        }
        if ((buf[1] & 0x18) == 0x08) {
            return false;
        }
        if ((buf[1] & 0x06) == 0x00) {
            // not layer I/II/III
            return false;
        }
        if ((buf[2] & 0xF0) == 0xF0) {
            // bad bitrate
            return false;
        }
        if ((buf[2] & 0x0C) == 0x0C) {
            // bad sample frequency
            return false;
        }
        if ((buf[1] & 0x18) == 0x18 &&
                (buf[1] & 0x06) == 0x04 &&
                (abl2[buf[2] >> 4] & (1 << (buf[3] >> 6))) != 0) {
            return false;
        }
        if ((buf[3] & 0x03) == 2) {
            // reserved emphasis mode (?)
            return false;
        }
        return true;
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