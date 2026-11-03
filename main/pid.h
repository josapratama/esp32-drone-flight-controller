#pragma once
#include <Arduino.h>

struct PIDConfig {
  float kp, ki, kd;
  float iMax;   // anti windup limit
  float outMax; // output limit
  float dFilterHz; // PT1 Low Pass Filter cutoff frequency (Hz)
};

class PID {
public:
  PID(float kp, float ki, float kd, float iMax, float outMax, float dFilterHz = 35.0f);

  float  compute(float setpoint, float actual, float dt);
  void   reset();
  void   setGains(float kp, float ki, float kd);
  void   setDFilterHz(float hz);
  
  float  getKp() const { return _kp; }
  float  getKi() const { return _ki; }
  float  getKd() const { return _kd; }
  float  getLastOutput() const { return _output; }

private:
  float  _kp, _ki, _kd;
  float  _iMax, _outMax;
  float  _prevError;
  float  _prevActual;
  float  _integral;
  float  _lastD;
  float  _dFilterHz;
  float  _output;
  bool   _firstRun;
};
