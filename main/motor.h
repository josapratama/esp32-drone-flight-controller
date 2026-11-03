#pragma once
#include <Arduino.h>

void  motorInit();
void  motorArm();
void  motorDisarm();
void  motorWrite(float throttle, float roll, float pitch, float yaw, float thrustMultiplier = 1.0f);
void  motorStop();
int   motorGetPWM(int index);   // debug: baca PWM motor 0-3
bool  motorIsArmed();
