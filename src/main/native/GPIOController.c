#include <jni.h>
#include <stdio.h>
#include <wiringPi.h>

#include "com_wattu_sidpi_GPIOController.h"

JNIEXPORT jint JNICALL Java_com_wattu_sidpi_GPIOController_wiringPiSetup (JNIEnv *env, jobject thisObj) {
	if (wiringPiSetupGpio () < 0) {
		//fprintf (stderr, "Unable to setup wiringPi: %s\n", strerror (errno));
		return -1;
	}

   return 0;
}

JNIEXPORT void JNICALL Java_com_wattu_sidpi_GPIOController_pinMode  (JNIEnv *env, jobject thisObj, jint pin, jint mode) {
	pinMode(pin,mode);
}

/*
 * Class:     com_wattu_sidpi_GPIOController
 * Method:    digitalWrite
 * Signature: (II)V
 */
JNIEXPORT void JNICALL Java_com_wattu_sidpi_GPIOController_digitalWrite (JNIEnv *env, jobject thisObj, jint pin, jint value) {
	digitalWrite(pin,value);
}

/*
 * Class:     com_wattu_sidpi_GPIOController
 * Method:    digitalRead
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_com_wattu_sidpi_GPIOController_digitalRead (JNIEnv *env, jobject thisObj, jint pin) {
	return (jint) digitalRead(pin);
}

/*
 * Class:     com_wattu_sidpi_GPIOController
 * Method:    gpioClockSet
 * Signature: (II)V
 */
JNIEXPORT void JNICALL Java_com_wattu_sidpi_GPIOController_gpioClockSet
  (JNIEnv *env, jobject thisObj, jint pin, jint freq) {
	gpioClockSet(pin,freq);
}

JNIEXPORT jint JNICALL Java_com_wattu_sidpi_GPIOController_readByte (JNIEnv *env, jobject thisObj) {
	int data = 0;

	digitalWrite(8,1);
	digitalWrite(25,0);
	//Java_com_wattu_sidpi_GPIOController_sidWait();
	data = digitalRead(2)
		| (digitalRead(3) << 1)
		| (digitalRead(17) << 2)
		| (digitalRead(27) << 3)
		| (digitalRead(22) << 4)
		| (digitalRead(10) << 5)
		| (digitalRead(9) << 6)
		| (digitalRead(11) << 7);
	digitalWrite(25,1);

	return (jint) data;
}

/*
 * Class:     com_wattu_sidpi_GPIOController
 * Method:    writeByte
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_wattu_sidpi_GPIOController_writeByte (JNIEnv *env, jobject thisObj, jint data) {
	digitalWrite(8,0);
	digitalWrite(2, (data & 1));
	digitalWrite(3, (data & 2) >> 1);
	digitalWrite(17, (data & 4) >> 2);
	digitalWrite(27, (data & 8) >> 3);
	digitalWrite(22, (data & 16) >> 4);
	digitalWrite(10, (data & 32) >> 5);
	digitalWrite(9, (data & 64) >> 6);
	digitalWrite(11, (data & 128) >> 7);
	digitalWrite(25,0);
	//Java_com_wattu_sidpi_GPIOController_sidWait();
	digitalWrite(25,1);

}

