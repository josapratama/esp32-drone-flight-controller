/*
 * TEST 6 - SPIN MOTOR SATU PER SATU
 * Cek arah rotasi tiap motor sebelum pasang propeller.
 * LEPAS PROPELLER sebelum menjalankan program ini!
 *
 * Menggunakan MCPWM (Motor Control PWM) bawaan ESP32-S3.
 * Tidak butuh library tambahan, setiap channel benar-benar independen.
 *
 * ESC Mapping (ESP32-S3):
 *   M1 Front Left  CCW → GPIO 4
 *   M2 Front Right CW  → GPIO 5
 *   M3 Rear Left   CW  → GPIO 6
 *   M4 Rear Right  CCW → GPIO 7
 *
 * Arah rotasi yang benar (dilihat dari atas):
 *   M1 (FL) → CCW (berlawanan jarum jam)
 *   M2 (FR) → CW  (searah jarum jam)
 *   M3 (RL) → CW  (searah jarum jam)
 *   M4 (RR) → CCW (berlawanan jarum jam)
 *
 * Cara pakai:
 *   Kirim perintah via Serial Monitor (115200 baud):
 *   1  → spin M1 (Front Left)
 *   2  → spin M2 (Front Right)
 *   3  → spin M3 (Rear Left)
 *   4  → spin M4 (Rear Right)
 *   0  → stop semua
 *   s  → spin semua sekaligus (throttle rendah)
 */

#include "driver/mcpwm_prelude.h"

#define PIN_M1  4
#define PIN_M2  5
#define PIN_M3  6
#define PIN_M4  7

#define THR_MIN   1000   // microseconds
#define THR_MAX   2000
#define THR_ARM    700
#define THR_TEST  1150   // throttle test rendah

// MCPWM handles
mcpwm_cmpr_handle_t cmp[4] = {};
mcpwm_gen_handle_t  gen[4] = {};

// Kirim sinyal PWM dalam microseconds ke motor index (0-3)
void motorWrite(int idx, int us) {
  mcpwm_comparator_set_compare_value(cmp[idx], us);
}

void setupMotorPin(int idx, int gpio) {
  // Timer (1 timer per 2 operator, tapi MCPWM punya 3 timer × 2 operator = 6 operator)
  // Kita pakai 1 timer, 4 operator terpisah
  mcpwm_oper_handle_t oper = NULL;

  mcpwm_timer_handle_t timer = NULL;
  mcpwm_timer_config_t timer_config = {
    .group_id      = idx / 2,   // group 0 untuk M1&M2, group 1 untuk M3&M4
    .clk_src       = MCPWM_TIMER_CLK_SRC_DEFAULT,
    .resolution_hz = 1000000,   // 1MHz → 1 tick = 1 microsecond
    .count_mode    = MCPWM_TIMER_COUNT_MODE_UP,
    .period_ticks  = 20000,     // 20ms = 50Hz (standar ESC)
  };
  mcpwm_new_timer(&timer_config, &timer);

  mcpwm_operator_config_t oper_config = {
    .group_id = idx / 2,
  };
  mcpwm_new_operator(&oper_config, &oper);
  mcpwm_operator_connect_timer(oper, timer);

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
    MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH));
  mcpwm_generator_set_action_on_compare_event(gen[idx],
    MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, cmp[idx], MCPWM_GEN_ACTION_LOW));

  mcpwm_comparator_set_compare_value(cmp[idx], THR_ARM);
  mcpwm_timer_enable(timer);
  mcpwm_timer_start_stop(timer, MCPWM_TIMER_START_NO_STOP);
}

void armAll() {
  for (int i = 0; i < 4; i++) motorWrite(i, THR_ARM);
  delay(1000);
  for (int i = 0; i < 4; i++) motorWrite(i, THR_MIN);
  delay(1000);
}

void stopAll() {
  for (int i = 0; i < 4; i++) motorWrite(i, THR_MIN);
}

void printMenu() {
  Serial.println("──────────────────────────────");
  Serial.println("1 → Spin M1 Front Left  (harusnya CCW)");
  Serial.println("2 → Spin M2 Front Right (harusnya CW)");
  Serial.println("3 → Spin M3 Rear Left   (harusnya CW)");
  Serial.println("4 → Spin M4 Rear Right  (harusnya CCW)");
  Serial.println("0 → Stop semua");
  Serial.println("s → Spin semua (throttle rendah)");
  Serial.println("──────────────────────────────");
}

void setup() {
  Serial.begin(115200);
  delay(500);

  setupMotorPin(0, PIN_M1);
  setupMotorPin(1, PIN_M2);
  setupMotorPin(2, PIN_M3);
  setupMotorPin(3, PIN_M4);

  Serial.println("=== TEST MOTOR SATU PER SATU (MCPWM) ===");
  Serial.println("PASTIKAN PROPELLER SUDAH DILEPAS!");
  Serial.println("Arming ESC...");

  armAll();

  Serial.println("ESC siap.");
  printMenu();
}

void loop() {
  if (Serial.available()) {
    char cmd = Serial.read();
    while (Serial.available()) Serial.read(); // flush

    stopAll();

    switch (cmd) {
      case '1':
        Serial.println("Spin M1 - Front Left (CCW)...");
        motorWrite(0, THR_TEST);
        delay(3000);
        stopAll();
        Serial.println("Stop. Arah putaran sudah benar? (CCW = berlawanan jarum jam)");
        break;

      case '2':
        Serial.println("Spin M2 - Front Right (CW)...");
        motorWrite(1, THR_TEST);
        delay(3000);
        stopAll();
        Serial.println("Stop. Arah putaran sudah benar? (CW = searah jarum jam)");
        break;

      case '3':
        Serial.println("Spin M3 - Rear Left (CW)...");
        motorWrite(2, THR_TEST);
        delay(3000);
        stopAll();
        Serial.println("Stop. Arah putaran sudah benar? (CW = searah jarum jam)");
        break;

      case '4':
        Serial.println("Spin M4 - Rear Right (CCW)...");
        motorWrite(3, THR_TEST);
        delay(3000);
        stopAll();
        Serial.println("Stop. Arah putaran sudah benar? (CCW = berlawanan jarum jam)");
        break;

      case '0':
        Serial.println("Semua motor STOP.");
        break;

      case 's': case 'S':
        Serial.println("Spin semua motor (throttle rendah)...");
        for (int i = 0; i < 4; i++) motorWrite(i, THR_TEST);
        delay(3000);
        stopAll();
        Serial.println("Stop.");
        break;

      default:
        Serial.println("Perintah tidak dikenal.");
        break;
    }

    printMenu();
  }
}
