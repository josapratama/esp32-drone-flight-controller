/*
 * TEST 2 - KALIBRASI ESC (MCPWM)
 * Kalibrasi throttle range semua ESC sekaligus.
 * LEPAS PROPELLER sebelum menjalankan program ini!
 *
 * ESC Mapping (ESP32-S3):
 *   M1 Front Left  CCW → GPIO 4
 *   M2 Front Right CW  → GPIO 5
 *   M3 Rear Left   CW  → GPIO 6
 *   M4 Rear Right  CCW → GPIO 7
 *
 * Urutan kalibrasi:
 *   1. Upload program ini (JANGAN colok LiPo dulu)
 *   2. Buka Serial Monitor 115200
 *   3. Ikuti instruksi di Serial Monitor
 */

#include "driver/mcpwm_prelude.h"
#include <Arduino.h>

// ── Pin ──────────────────────────────────────────
#define PIN_M1  4
#define PIN_M2  5
#define PIN_M3  6
#define PIN_M4  7

// ── Throttle range ───────────────────────────────
#define THR_MAX  2000
#define THR_MIN  1000

// ── MCPWM handles ────────────────────────────────
static mcpwm_cmpr_handle_t cmp[4] = {};
static mcpwm_timer_handle_t timers[4] = {};

// Kirim sinyal µs ke motor index 0–3
static void pwmWrite(int idx, int us) {
  mcpwm_comparator_set_compare_value(cmp[idx], (uint32_t)us);
}

// Setup satu channel MCPWM
static void setupPin(int idx, int gpio) {
  mcpwm_timer_config_t timer_cfg = {
    .group_id      = idx / 2,
    .clk_src       = MCPWM_TIMER_CLK_SRC_DEFAULT,
    .resolution_hz = 1000000,   // 1 tick = 1 µs
    .count_mode    = MCPWM_TIMER_COUNT_MODE_UP,
    .period_ticks  = 20000,     // 20ms = 50Hz
  };
  mcpwm_new_timer(&timer_cfg, &timers[idx]);

  mcpwm_oper_handle_t oper = NULL;
  mcpwm_operator_config_t oper_cfg = { .group_id = idx / 2 };
  mcpwm_new_operator(&oper_cfg, &oper);
  mcpwm_operator_connect_timer(oper, timers[idx]);

  mcpwm_comparator_config_t cmp_cfg = {
    .flags = { .update_cmp_on_tez = true }
  };
  mcpwm_new_comparator(oper, &cmp_cfg, &cmp[idx]);

  mcpwm_gen_handle_t gen = NULL;
  mcpwm_generator_config_t gen_cfg = { .gen_gpio_num = gpio };
  mcpwm_new_generator(oper, &gen_cfg, &gen);

  mcpwm_generator_set_action_on_timer_event(gen,
    MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP,
                                  MCPWM_TIMER_EVENT_EMPTY,
                                  MCPWM_GEN_ACTION_HIGH));
  mcpwm_generator_set_action_on_compare_event(gen,
    MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP,
                                    cmp[idx],
                                    MCPWM_GEN_ACTION_LOW));

  mcpwm_comparator_set_compare_value(cmp[idx], THR_MIN);
  mcpwm_timer_enable(timers[idx]);
  mcpwm_timer_start_stop(timers[idx], MCPWM_TIMER_START_NO_STOP);
}

// Kirim nilai yang sama ke semua 4 motor
void setAll(int us) {
  for (int i = 0; i < 4; i++) pwmWrite(i, us);
}

// Tunggu user tekan ENTER di Serial Monitor
void waitEnter(const char* msg) {
  Serial.println(msg);
  Serial.println(">>> Tekan ENTER untuk lanjut...");
  while (!Serial.available());
  while (Serial.available()) Serial.read();
  Serial.println();
}

// ─────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(500);

  // Init semua 4 channel MCPWM
  setupPin(0, PIN_M1);
  setupPin(1, PIN_M2);
  setupPin(2, PIN_M3);
  setupPin(3, PIN_M4);

  Serial.println("=====================================");
  Serial.println("      KALIBRASI ESC — MCPWM");
  Serial.println("=====================================");
  Serial.println("!! PASTIKAN PROPELLER SUDAH DILEPAS !!");
  Serial.println("!! JANGAN COLOK LIPO DULU            !!");
  Serial.println();

  // ── STEP 1: Kirim MAX, baru colok LiPo ──────
  Serial.println("[STEP 1] Mengirim sinyal MAXIMUM (2000µs)...");
  setAll(THR_MAX);
  Serial.println("Sinyal MAX sudah dikirim ke semua ESC.");
  Serial.println();
  waitEnter("[STEP 1] Sekarang COLOK LIPO → tunggu bunyi bip pertama dari ESC");

  // ── STEP 2: Turun ke MIN ─────────────────────
  Serial.println("[STEP 2] Mengirim sinyal MINIMUM (1000µs)...");
  setAll(THR_MIN);
  Serial.println("Tunggu bunyi kalibrasi dari ESC (biasanya 2–3 bip)...");
  waitEnter("[STEP 2] Setelah bunyi kalibrasi selesai, tekan ENTER");

  // ── Selesai ──────────────────────────────────
  Serial.println("====================================");
  Serial.println("Kalibrasi selesai!");
  Serial.println("Masuk mode test — ketik 0–100 untuk throttle");
  Serial.println("  0   = motor berhenti (1000µs)");
  Serial.println("  10  = throttle rendah");
  Serial.println("  50  = throttle setengah");
  Serial.println("  100 = full throttle (2000µs)");
  Serial.println("====================================");
}

// ─────────────────────────────────────────────────
void loop() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    int val = input.toInt();
    val = constrain(val, 0, 100);

    int us = map(val, 0, 100, THR_MIN, THR_MAX);
    setAll(us);

    Serial.print("Throttle: ");
    Serial.print(val);
    Serial.print("% → ");
    Serial.print(us);
    Serial.print(" µs");
    if (val == 0)   Serial.print("  (motor berhenti)");
    if (val == 100) Serial.print("  (full throttle)");
    Serial.println();
  }
}
