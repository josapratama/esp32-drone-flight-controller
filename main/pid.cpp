#include "pid.h"

PID::PID(float kp, float ki, float kd, float iMax, float outMax, float dFilterHz)
  : _kp(kp), _ki(ki), _kd(kd),
    _iMax(iMax), _outMax(outMax),
    _prevError(0), _prevActual(0),
    _integral(0), _lastD(0),
    _dFilterHz(dFilterHz),
    _output(0), _firstRun(true) {}

float PID::compute(float setpoint, float actual, float dt) {
  if (dt <= 0.0f) return 0.0f;

  float error = setpoint - actual;

  // Integral dengan anti-windup
  _integral += error * dt;
  _integral  = constrain(_integral, -_iMax, _iMax);

  // Derivative-on-Measurement (menghindari derivative kick saat stick digerakkan cepat)
  float rawDerivative = 0.0f;
  if (!_firstRun) {
    rawDerivative = -(actual - _prevActual) / dt;
  }
  _firstRun = false;

  // 1st-order Low-Pass Filter (PT1) pada D-term untuk meredam noise getaran motor
  float dTerm = rawDerivative;
  if (_dFilterHz > 0.0f) {
    float rc = 1.0f / (2.0f * 3.14159265f * _dFilterHz);
    float alpha = dt / (rc + dt);
    dTerm = _lastD + alpha * (rawDerivative - _lastD);
    _lastD = dTerm;
  }

  // Hitung output PID
  _output = (_kp * error) + (_ki * _integral) + (_kd * dTerm);
  _output = constrain(_output, -_outMax, _outMax);

  _prevError  = error;
  _prevActual = actual;
  return _output;
}

void PID::reset() {
  _prevError  = 0.0f;
  _prevActual = 0.0f;
  _integral   = 0.0f;
  _lastD      = 0.0f;
  _output     = 0.0f;
  _firstRun   = true;
}

void PID::setGains(float kp, float ki, float kd) {
  _kp = kp;
  _ki = ki;
  _kd = kd;
}

void PID::setDFilterHz(float hz) {
  _dFilterHz = hz;
}
