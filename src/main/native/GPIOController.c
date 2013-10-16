#include <jni.h>
#include <stdio.h>
#include <wiringPi.h>

#include "com_wattu_sidpi_impl_GPIOControllerImpl.h"

JNIEXPORT jint JNICALL Java_com_wattu_sidpi_impl_GPIOControllerImpl_wiringPiSetup (JNIEnv *env, jobject thisObj) {
	if (wiringPiSetupGpio () < 0) {
		return -1;
	}

   return 0;
}

/*
 * Class:     com_wattu_sidpi_GPIOController
 * Method:    clockSpeed
 * Signature: (II)V
 */
JNIEXPORT void JNICALL Java_com_wattu_sidpi_impl_GPIOControllerImpl_clockSpeed
  (JNIEnv * env, jobject obj, jint pin, jint speed) {
	pinMode(pin,com_wattu_sidpi_impl_GPIOControllerImpl_MODE_CLOCK);
	gpioClockSet(pin,speed);
}

/*
 * Class:     com_wattu_sidpi_GPIOController
 * Method:    setPins
 * Signature: ([I[I)V
 */
JNIEXPORT void JNICALL Java_com_wattu_sidpi_impl_GPIOControllerImpl_setPins
  (JNIEnv *env, jobject obj, jintArray pins, jintArray vals) {
	 // Step 1: Convert the incoming JNI jintarray to C's jint[]
		int i;
		jint *pins_c = (*env)->GetIntArrayElements(env, pins, NULL);
		jsize length = (*env)->GetArrayLength(env, pins);
		jint *vals_c = (*env)->GetIntArrayElements(env, vals, NULL);

	    for (i = 0; i < length; i++) {
	    	pinMode(pins_c[i],com_wattu_sidpi_impl_GPIOControllerImpl_MODE_OUT);
	    	digitalWrite(pins_c[i],vals_c[i]);
	    }
}


/*
 * Class:     com_wattu_sidpi_GPIOController
 * Method:    setPin
 * Signature: (II)V
 */
JNIEXPORT void JNICALL Java_com_wattu_sidpi_impl_GPIOControllerImpl_setPin
  (JNIEnv *env , jobject obj, jint pin, jint value) {
	pinMode(pin,com_wattu_sidpi_impl_GPIOControllerImpl_MODE_OUT);
	digitalWrite(pin,value);

}

/*
 * Class:     com_wattu_sidpi_GPIOController
 * Method:    getPin
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_com_wattu_sidpi_impl_GPIOControllerImpl_getPin
  (JNIEnv *env, jobject obj, jint pin) {
	pinMode(pin,com_wattu_sidpi_impl_GPIOControllerImpl_MODE_IN);
	return (jint) digitalRead(pin);
}

/*
 * Class:     com_wattu_sidpi_GPIOController
 * Method:    getPins
 * Signature: ([I)[I
 */
JNIEXPORT jintArray JNICALL Java_com_wattu_sidpi_impl_GPIOControllerImpl_getPins
  (JNIEnv *env, jobject obj, jintArray pins) {

	int i;
	jint *pins_c = (*env)->GetIntArrayElements(env, pins, NULL);
	jsize length = (*env)->GetArrayLength(env, pins);

	jintArray dataJNI = (*env)->NewIntArray(env,length);
	jint *data = (*env)->GetIntArrayElements(env,dataJNI,NULL);


	if(NULL == dataJNI) return NULL;

    for (i = 0; i < length; i++) {
    	pinMode(pins_c[i],com_wattu_sidpi_impl_GPIOControllerImpl_MODE_IN);
    	data[i] = digitalRead(pins_c[i]);
    }
	return dataJNI;
}

/*
 * Class:     com_wattu_sidpi_GPIOController
 * Method:    startClock
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_wattu_sidpi_impl_GPIOControllerImpl_startClock
  (JNIEnv *env, jobject obj, jint pin) {

}

/*
 * Class:     com_wattu_sidpi_GPIOController
 * Method:    stopClock
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_wattu_sidpi_impl_GPIOControllerImpl_stopClock
  (JNIEnv *env, jobject obj, jint pin) {

}
