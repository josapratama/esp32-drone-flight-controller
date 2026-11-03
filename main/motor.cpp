#include "motor.h"
#include "config.h"
#include "driver/mcpwm_prelude.h"

// MCPWM handles — 4 motor, masing-masing independen
static mcpwm_cmpr_handle_t cmp[4] = {};
static mcpwm_gen_handle_t  gen[4] = {};
static mcpwm_timer_handle_t timers[4] = {};

static int   pwm[4] = {ESC_MIN, ESC_MIN, ESC_MIN, ESC_MIN};
static bool  armed  = false;

// Kirim sinyal PWM dalam microseconds ke motor index (0-3)
static void mcpwmWrite(int idx, int us) {
  mcpwm_comparator_set_compare_value(cmp[idx], (uint32_t)us);
}

// Setup satu motor: idx=0..3, gpio=pin number
static void setupMotorPin(int idx, int gpio) {
  mcpwm_timer_config_t timer_config = {
    .group_id      = idx / 2,      // group 0 untuk M1&M2, group 1 untuk M3&M4
    .clk_src       = MCPWM_TIMER_CLK_SRC_DEFAULT,
    .resolution_hz = 1000000,      // 1MHz → 1 tick = 1 microsecond
    .count_mode    = MCPWM_TIMER_COUNT_MODE_UP,
    .period_ticks  = 2500,         // 2500us = 400Hz (High-Rate ESC PWM, latensi 2.5ms vs 20ms standar servo)
  };
  mcpwm_new_timer(&timer_config, &timers[idx]);

  mcpwm_oper_handle_t oper = NULL;
  mcpwm_operator_config_t oper_config = {
    .group_id = idx / 2,
  };
  mcpwm_new_operator(&oper_config, &oper);
  mcpwm_operator_connect_timer(oper, timers[idx]);

  mcpwm_comparator_config_t cmp_config = {
    .flags = { .update_cmp_on_tez = true },
  };
  mcpwm_new_comparator(oper, &cmp_config, &cmp[idx]);

  mcpwm_generator_config_t gen_config = {
    .gen_gpio_num = gpio,
  };
  mcpwm_new_generator(oper, &gen_config, &gen[idx]);

  // HIGH saat counter = 0, LOW saat counter = compare value
  mcpwm_generator_set_action_on_timer_event(gen[idx],
    MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP,
                                  MCPWM_TIMER_EVENT_EMPTY,
                                  MCPWM_GEN_ACTION_HIGH));
  mcpwm_generator_set_action_on_compare_event(gen[idx],
    MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP,
                                    cmp[idx],
                                    MCPWM_GEN_ACTION_LOW));

  mcpwm_comparator_set_compare_value(cmp[idx], ESC_ARM);
  mcpwm_timer_enable(timers[idx]);
  mcpwm_timer_start_stop(timers[idx], MCPWM_TIMER_START_NO_STOP);
}

void motorInit() {
  setupMotorPin(0, PIN_M1);
  setupMotorPin(1, PIN_M2);
  setupMotorPin(2, PIN_M3);
  setupMotorPin(3, PIN_M4);

  motorStop();
  Serial.println("[MOTOR] Init OK (MCPWM 400Hz High-Rate)");
}

void motorArm() {
  if (armed) return;
  for (int i = 0; i < 4; i++) mcpwmWrite(i, ESC_MIN);
  delay(500);
  armed = true;
  Serial.println("[MOTOR] Armed");
}

void motorDisarm() {
  motorStop();
  armed = false;
  Serial.println("[MOTOR] Disarmed");
}

bool motorIsArmed() {
  return armed;
}

void motorWrite(float throttle, float roll, float pitch, float yaw, float thrustMultiplier) {
  if (!armed) return;

  // throttle: 0.0 ~ 1.0 (dengan kompensasi sag baterai jika ada)
  float compThrottle = constrain(throttle * thrustMultiplier, 0.0f, 1.0f);

  // Map throttle ke microseconds
  float thr = ESC_IDLE + compThrottle * (ESC_MAX - ESC_IDLE);

  // Motor mixing Quad X
  // Dilihat dari atas:
  //   M1 FL CCW: +throttle + roll + pitch + yaw
  //   M2 FR CW:  +throttle - roll + pitch - yaw
  //   M3 RL CW:  +throttle + roll - pitch - yaw
  //   M4 RR CCW: +throttle - roll - pitch + yaw

  float out1 = thr + roll + pitch + yaw;
  float out2 = thr - roll + pitch - yaw;
  float out3 = thr + roll - pitch - yaw;
  float out4 = thr - roll - pitch + yaw;

  // Clamp ke range ESC
  pwm[0] = (int)constrain(out1, ESC_MIN, ESC_MAX);
  pwm[1] = (int)constrain(out2, ESC_MIN, ESC_MAX);
  pwm[2] = (int)constrain(out3, ESC_MIN, ESC_MAX);
  pwm[3] = (int)constrain(out4, ESC_MIN, ESC_MAX);

  mcpwmWrite(0, pwm[0]);
  mcpwmWrite(1, pwm[1]);
  mcpwmWrite(2, pwm[2]);
  mcpwmWrite(3, pwm[3]);
}

void motorStop() {
  pwm[0] = pwm[1] = pwm[2] = pwm[3] = ESC_MIN;
  for (int i = 0; i < 4; i++) mcpwmWrite(i, ESC_MIN);
}

int motorGetPWM(int index) {
  if (index < 0 || index > 3) return 0;
  return pwm[index];
}
