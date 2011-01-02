/* Lame.java
   A port of LAME for Android

   Copyright (c) 2010 Ethan Chen

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
/*****************************************************************************/
#include "lame-interface.h"
#include "lame.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <android/log.h>

static lame_global_flags *lame_context;
static hip_t hip_context;
static mp3data_struct *mp3data;
static int enc_delay, enc_padding;

JNIEXPORT jint JNICALL Java_net_sourceforge_lame_Lame_initializeEncoder
  (JNIEnv *env, jclass class, jint sampleRate, jint numChannels)
{
  if (!lame_context) {
    lame_context = lame_init();
    if (lame_context) {
      lame_set_in_samplerate(lame_context, sampleRate);
      lame_set_num_channels(lame_context, numChannels);
      int ret = lame_init_params(lame_context);
      __android_log_print(ANDROID_LOG_DEBUG, "liblame.so", "initialized lame with code %d", ret);
      return ret;
    }
  }
  return -1;
}


JNIEXPORT void JNICALL Java_net_sourceforge_lame_Lame_setEncoderPreset
  (JNIEnv *env, jclass class, jint preset)
{
  // set to vbr_mtrh for fast, vbr_rh for slower
  switch (preset) {
    case net_sourceforge_lame_Lame_LAME_PRESET_MEDIUM:
      lame_set_VBR_q(lame_context, 4);
      lame_set_VBR(lame_context, vbr_rh);
      break;
    case net_sourceforge_lame_Lame_LAME_PRESET_STANDARD:
      lame_set_VBR_q(lame_context, 2);
      lame_set_VBR(lame_context, vbr_rh);
      break;
    case net_sourceforge_lame_Lame_LAME_PRESET_EXTREME:
      lame_set_VBR_q(lame_context, 0);
      lame_set_VBR(lame_context, vbr_rh);
      break;
    case net_sourceforge_lame_Lame_LAME_PRESET_DEFAULT:
    default:
      break;
  }
}


JNIEXPORT jint JNICALL Java_net_sourceforge_lame_Lame_encode
  (JNIEnv *env, jclass class, jshortArray leftChannel, jshortArray rightChannel,
		  jint channelSamples, jbyteArray mp3Buffer, jint bufferSize)
{
  int encoded_samples;
  short *left_buf, *right_buf;
  unsigned char *mp3_buf;

  left_buf = (*env)->GetShortArrayElements(env, leftChannel, NULL);
  right_buf = (*env)->GetShortArrayElements(env, rightChannel, NULL);
  mp3_buf = (*env)->GetByteArrayElements(env, mp3Buffer, NULL);

  encoded_samples = lame_encode_buffer(lame_context, left_buf, right_buf, channelSamples, mp3_buf, bufferSize);

  // mode 0 means free left/right buf, write changes back to left/rightChannel
  (*env)->ReleaseShortArrayElements(env, leftChannel, left_buf, 0);
  (*env)->ReleaseShortArrayElements(env, rightChannel, right_buf, 0);

  if (encoded_samples < 0) {
    // don't propagate changes back up if we failed
    (*env)->ReleaseByteArrayElements(env, mp3Buffer, mp3_buf, JNI_ABORT);
    return -1;
  }

  (*env)->ReleaseByteArrayElements(env, mp3Buffer, mp3_buf, 0);
  return encoded_samples;
}


JNIEXPORT jint JNICALL Java_net_sourceforge_lame_Lame_flushEncoder
  (JNIEnv *env, jclass class, jbyteArray mp3Buffer, jint bufferSize)
{
  // call lame_encode_flush when near the end of pcm buffer
  int num_bytes;
  unsigned char *mp3_buf;

  mp3_buf = (*env)->GetByteArrayElements(env, mp3Buffer, NULL);

  num_bytes = lame_encode_flush(lame_context, mp3_buf, bufferSize);
  if (num_bytes < 0) {
    // some kind of error occurred, don't propagate changes to buffer
	(*env)->ReleaseByteArrayElements(env, mp3Buffer, mp3_buf, JNI_ABORT);
    return num_bytes;
  }

  (*env)->ReleaseByteArrayElements(env, mp3Buffer, mp3_buf, 0);
  return num_bytes;
}


JNIEXPORT jint JNICALL Java_net_sourceforge_lame_Lame_closeEncoder
  (JNIEnv *env, jclass class)
{
  if (lame_context) {
    int ret = lame_close(lame_context);
    lame_context = NULL;
    __android_log_print(ANDROID_LOG_DEBUG, "liblame.so", "freed lame with code %d", ret);
    return ret;
  }
  return -1;
}


JNIEXPORT jint JNICALL Java_net_sourceforge_lame_Lame_initializeDecoder
  (JNIEnv *env, jclass class)
{
  if (!hip_context) {
    hip_context = hip_decode_init();
    if (hip_context) {
      mp3data = (mp3data_struct *) malloc(sizeof(mp3data_struct));
      memset(mp3data, 0, sizeof(mp3data_struct));
      enc_delay = -1;
      enc_padding = -1;
      return 0;
    }
  }
  return -1;
}


JNIEXPORT jint JNICALL Java_net_sourceforge_lame_Lame_nativeConfigureDecoder
  (JNIEnv *env, jclass class, jbyteArray mp3Buffer, jint bufferSize)
{
  int ret = -1;
  short left_buf[1152], right_buf[1152];
  unsigned char *mp3_buf;

  if (mp3data) {
    mp3_buf = (*env)->GetByteArrayElements(env, mp3Buffer, NULL);
    ret = hip_decode1_headersB(hip_context, mp3_buf, bufferSize,
        left_buf, right_buf, mp3data, &enc_delay, &enc_padding);
    if (mp3data->header_parsed) {
      mp3data->totalframes = mp3data->nsamp / mp3data->framesize;
      ret = 0;
      __android_log_print(ANDROID_LOG_DEBUG, "liblame.so", "decoder configured successfully");
      __android_log_print(ANDROID_LOG_DEBUG, "liblame.so", "sample rate: %d, channels: %d", mp3data->samplerate, mp3data->stereo);
      __android_log_print(ANDROID_LOG_DEBUG, "liblame.so", "bitrate: %d, frame size: %d", mp3data->bitrate, mp3data->framesize);
    } else {
        ret = -1;
    }
    (*env)->ReleaseByteArrayElements(env, mp3Buffer, mp3_buf, 0);
  }

  return ret;
}


JNIEXPORT jint JNICALL Java_net_sourceforge_lame_Lame_getDecoderChannels
  (JNIEnv *env, jclass class)
{
  return mp3data->stereo;
}


JNIEXPORT jint JNICALL Java_net_sourceforge_lame_Lame_getDecoderSampleRate
  (JNIEnv *env, jclass class)
{
  return mp3data->samplerate;
}


JNIEXPORT jint JNICALL Java_net_sourceforge_lame_Lame_getDecoderDelay
  (JNIEnv *env, jclass class)
{
  return enc_delay;
}


JNIEXPORT jint JNICALL Java_net_sourceforge_lame_Lame_getDecoderPadding
  (JNIEnv *env, jclass class)
{
  return enc_padding;
}


JNIEXPORT jint JNICALL Java_net_sourceforge_lame_Lame_getDecoderTotalFrames
  (JNIEnv *env, jclass class)
{
  return mp3data->totalframes;
}


JNIEXPORT jint JNICALL Java_net_sourceforge_lame_Lame_getDecoderFrameSize
  (JNIEnv *env, jclass class)
{
  return mp3data->framesize;
}


JNIEXPORT jint JNICALL Java_net_sourceforge_lame_Lame_getDecoderBitrate
  (JNIEnv *env, jclass class)
{
  return mp3data->bitrate;
}


JNIEXPORT jint JNICALL Java_net_sourceforge_lame_Lame_nativeDecodeFrame
  (JNIEnv *env, jclass class, jbyteArray mp3Buffer, jint bufferSize,
		  jshortArray rightChannel, jshortArray leftChannel)
{
  int samples_read;
  short *left_buf, *right_buf;
  unsigned char *mp3_buf;

  left_buf = (*env)->GetShortArrayElements(env, leftChannel, NULL);
  right_buf = (*env)->GetShortArrayElements(env, rightChannel, NULL);
  mp3_buf = (*env)->GetByteArrayElements(env, mp3Buffer, NULL);

  samples_read = hip_decode1_headers(hip_context, mp3_buf, bufferSize, left_buf, right_buf, mp3data);

  (*env)->ReleaseByteArrayElements(env, mp3Buffer, mp3_buf, 0);

  if (samples_read < 0) {
    // some sort of error occurred, don't propagate changes to buffers
	(*env)->ReleaseShortArrayElements(env, leftChannel, left_buf, JNI_ABORT);
	(*env)->ReleaseShortArrayElements(env, rightChannel, right_buf, JNI_ABORT);
    return samples_read;
  }

  (*env)->ReleaseShortArrayElements(env, leftChannel, left_buf, 0);
  (*env)->ReleaseShortArrayElements(env, rightChannel, right_buf, 0);

  return samples_read;
}


JNIEXPORT jint JNICALL Java_net_sourceforge_lame_Lame_closeDecoder
  (JNIEnv *env, jclass class)
{
  if (hip_context) {
    int ret = hip_decode_exit(hip_context);
    hip_context = NULL;
    free(mp3data);
    mp3data = NULL;
    enc_delay = -1;
    enc_padding = -1;
	return ret;
  }
  return -1;
}
