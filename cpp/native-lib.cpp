#include <jni.h>
#include <string>
#include <android/log.h>
#include <android/native_window_jni.h>
#include "video_decoder.h"
//
// Created by tang on 2024/6/13.
//
using namespace std;
extern "C"
JNIEXPORT jint JNICALL
Java_com_example_androidplayer_Player_nativePlay(JNIEnv *env, jobject thiz, jstring file,
                                                 jobject surface) {
    // TODO: implement nativePlay()
    const char*url=env->GetStringUTFChars(file,JNI_FALSE);
//    a_decode();
//    v_decode((char*)url);
    ANativeWindow * nativeWindow=ANativeWindow_fromSurface(env,surface);
    play(nativeWindow,(char*)url);
    return 0;

}
extern "C"
JNIEXPORT void JNICALL
Java_com_example_androidplayer_Player_nativePause(JNIEnv *env, jobject thiz, jboolean p) {
    // TODO: implement nativePause()
    isStart = !p;
}
extern "C"
JNIEXPORT jint JNICALL
Java_com_example_androidplayer_Player_nativeSeek(JNIEnv *env, jobject thiz, jdouble position) {
    // TODO: implement nativeSeek()
    seek(position);
    return 1;
}
extern "C"
JNIEXPORT jint JNICALL
Java_com_example_androidplayer_Player_nativeStop(JNIEnv *env, jobject thiz) {
    // TODO: implement nativeStop()
    isStart = false;
    STOP = true;
    return 0;
}
extern "C"
JNIEXPORT jint JNICALL
Java_com_example_androidplayer_Player_nativeSetSpeed(JNIEnv *env, jobject thiz, jfloat speed) {
    // TODO: implement nativeSetSpeed()
    return 0;
}
extern "C"
JNIEXPORT jdouble JNICALL
Java_com_example_androidplayer_Player_nativeGetPosition(JNIEnv *env, jobject thiz) {
    // TODO: implement nativeGetPosition()
    return current_time;
}
extern "C"
JNIEXPORT jdouble JNICALL
Java_com_example_androidplayer_Player_nativeGetDuration(JNIEnv *env, jobject thiz) {
    // TODO: implement nativeGetDuration()
    return total_time;
}

//test
//extern "C"
//JNIEXPORT void JNICALL
//Java_com_example_androidplayer_Player_nativeTest(JNIEnv *env, jobject thiz, jstring file) {
//    // TODO: implement nativeTest()
//    const char*url=env->GetStringUTFChars(file,JNI_FALSE);
//    __android_log_print(ANDROID_LOG_INFO, "native-log", "%s",url);
//    test((char*)url);
//}