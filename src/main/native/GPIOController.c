#include <jni.h>
#include <stdio.h>
#include <wiringPi.h>

#include "com_wattu_sidpi_GPIOController.h"

JNIEXPORT jint JNICALL Java_com_wattu_sidpi_GPIOController_wiringPiSetup (JNIEnv *env, jobject thisObj) {
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
JNIEXPORT void JNICALL Java_com_wattu_sidpi_GPIOController_clockSpeed
  (JNIEnv * env, jobject obj, jint pin, jint speed) {
	pinMode(pin,com_wattu_sidpi_GPIOController_MODE_CLOCK);
	gpioClockSet(pin,speed);
}

/*
 * Class:     com_wattu_sidpi_GPIOController
 * Method:    setPins
 * Signature: ([I[I)V
 */
JNIEXPORT void JNICALL Java_com_wattu_sidpi_GPIOController_setPins
  (JNIEnv *env, jobject obj, jintArray pins, jintArray vals) {
	int i=0;

	for(i =0;i< sizeOf(pins);i++) {
		pinMode(pins[i],com_wattu_sidpi_GPIOController_MODE_OUT);
		digitalWrite(pins[i],vals[i]);
	}
}

/*
 * Class:     com_wattu_sidpi_GPIOController
 * Method:    setPin
 * Signature: (II)V
 */
JNIEXPORT void JNICALL Java_com_wattu_sidpi_GPIOController_setPin
  (JNIEnv *env , jobject obj, jint pin, jint value) {
	pinMode(pin,com_wattu_sidpi_GPIOController_MODE_OUT);
	digitalWrite(pin,value);

}

/*
 * Class:     com_wattu_sidpi_GPIOController
 * Method:    getPin
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_com_wattu_sidpi_GPIOController_getPin
  (JNIEnv *env, jobject obj, jint pin) {
	pinMode(pin,com_wattu_sidpi_GPIOController_MODE_IN);
	return (jint) digitalRead(pin);
}

/*
 * Class:     com_wattu_sidpi_GPIOController
 * Method:    getPins
 * Signature: ([I)[I
 */
JNIEXPORT jintArray JNICALL Java_com_wattu_sidpi_GPIOController_getPins
  (JNIEnv *env, jobject obj, jintArray pins) {
	return (jintArray) 0L;
}

/*
 * Class:     com_wattu_sidpi_GPIOController
 * Method:    startClock
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_wattu_sidpi_GPIOController_startClock
  (JNIEnv *env, jobject obj, jint pin) {

}

/*
 * Class:     com_wattu_sidpi_GPIOController
 * Method:    stopClock
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_wattu_sidpi_GPIOController_stopClock
  (JNIEnv *env, jobject obj, jint pin) {

}
