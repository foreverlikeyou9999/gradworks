/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class com_intel_vpg_TrackingContext */

#ifndef _Included_com_intel_vpg_TrackingContext
#define _Included_com_intel_vpg_TrackingContext
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     com_intel_vpg_TrackingContext
 * Method:    nativeInit
 * Signature: ([III[D)V
 */
JNIEXPORT void JNICALL Java_com_intel_vpg_TrackingContext_nativeInit
  (JNIEnv *, jobject, jintArray, jint, jint, jdoubleArray);

/*
 * Class:     com_intel_vpg_TrackingContext
 * Method:    nativeTrack
 * Signature: ([D[III[III[FI[F)D
 */
JNIEXPORT jdouble JNICALL Java_com_intel_vpg_TrackingContext_nativeTrack
  (JNIEnv *, jobject, jdoubleArray, jintArray, jint, jint, jintArray, jint, jint, jfloatArray, jint, jfloatArray);

/*
 * Class:     com_intel_vpg_TrackingContext
 * Method:    nativeResample
 * Signature: ([III[F[III)Z
 */
JNIEXPORT jboolean JNICALL Java_com_intel_vpg_TrackingContext_nativeResample
  (JNIEnv *, jobject, jintArray, jint, jint, jfloatArray, jintArray, jint, jint);

#ifdef __cplusplus
}
#endif
#endif
