/* =====================================================================
 * CD4511  --  BCD to 7-segment latch / decoder / driver
 * Wokwi custom chip. Wokwi has no built-in 4511 part, so we build one.
 *
 * This file IS the truth table that used to live in your firmware as
 * DIGIT_PATTERNS[5][7]. That is the whole point of the exercise: the
 * lookup did not disappear, it MOVED. On real hardware it moves into
 * a 40-cent piece of silicon; here it moves into a chip model.
 *
 * BEHAVIOUR (matches the real datasheet):
 *   LT (Lamp Test, active LOW)  : LOW  -> all 7 segments ON
 *   BI (Blank Input, active LOW): LOW  -> all 7 segments OFF
 *   LE (Latch Enable)           : HIGH -> freeze the last BCD value
 *   inputs 0..9                 -> that digit
 *   inputs 10..15               -> BLANK  <-- we exploit this
 *
 * Outputs are ACTIVE HIGH (the chip SOURCES current), which is why it
 * must be paired with a COMMON CATHODE display. A 74LS47 is the mirror
 * image: active low, sinks current, needs common anode.
 * ===================================================================== */

#include "wokwi-api.h"
#include <stdlib.h>
#include <stdint.h>

/* Bit 0 = segment a ... bit 6 = segment g */
static const uint8_t PATTERNS[16] = {
  0x3F, /* 0 : a b c d e f   */
  0x06, /* 1 :   b c         */
  0x5B, /* 2 : a b   d e   g */
  0x4F, /* 3 : a b c d     g */
  0x66, /* 4 :   b c     f g */
  0x6D, /* 5 : a   c d   f g */
  0x7D, /* 6 : a   c d e f g */
  0x07, /* 7 : a b c         */
  0x7F, /* 8 : all           */
  0x6F, /* 9 : a b c d   f g */
  0x00, /* 10..15 : BLANK    */
  0x00,
  0x00,
  0x00,
  0x00,
  0x00
};

typedef struct {
  pin_t in[4];      /* A B C D  -- A is LSB */
  pin_t lt, bi, le;
  pin_t seg[7];     /* QA .. QG */
  uint8_t latched;  /* value held while LE is HIGH */
} chip_state_t;

static void update(void *user_data) {
  chip_state_t *chip = (chip_state_t *)user_data;

  /* --- BI has priority over everything: blank wins --- */
  if (pin_read(chip->bi) == LOW) {
    for (int i = 0; i < 7; i++) pin_write(chip->seg[i], LOW);
    return;
  }

  /* --- LT: lamp test, light everything --- */
  if (pin_read(chip->lt) == LOW) {
    for (int i = 0; i < 7; i++) pin_write(chip->seg[i], HIGH);
    return;
  }

  /* --- LE LOW = transparent, LE HIGH = hold last value --- */
  if (pin_read(chip->le) == LOW) {
    uint8_t v = 0;
    for (int i = 0; i < 4; i++) {
      if (pin_read(chip->in[i]) == HIGH) v |= (1u << i);
    }
    chip->latched = v;
  }

  uint8_t pattern = PATTERNS[chip->latched & 0x0F];
  for (int i = 0; i < 7; i++) {
    pin_write(chip->seg[i], (pattern >> i) & 1 ? HIGH : LOW);
  }
}

/* Wokwi's pin_change callback is handed THREE arguments: user_data, the
 * pin that changed, and its new value. update() re-reads every input on
 * each call, so it does not care which pin woke it up -- but the types
 * must still match exactly or the compiler rejects the assignment.
 * (void) casts kill the unused-parameter warnings. */
static void on_pin_change(void *user_data, pin_t pin, uint32_t value) {
  (void)pin;
  (void)value;
  update(user_data);
}

void chip_init(void) {
  chip_state_t *chip = (chip_state_t *)calloc(1, sizeof(chip_state_t));

  chip->in[0] = pin_init("A", INPUT);
  chip->in[1] = pin_init("B", INPUT);
  chip->in[2] = pin_init("C", INPUT);
  chip->in[3] = pin_init("D", INPUT);

  chip->lt = pin_init("LT", INPUT_PULLUP);   /* idle HIGH = inactive */
  chip->bi = pin_init("BI", INPUT_PULLUP);   /* idle HIGH = inactive */
  chip->le = pin_init("LE", INPUT_PULLDOWN); /* idle LOW  = transparent */

  chip->seg[0] = pin_init("QA", OUTPUT);
  chip->seg[1] = pin_init("QB", OUTPUT);
  chip->seg[2] = pin_init("QC", OUTPUT);
  chip->seg[3] = pin_init("QD", OUTPUT);
  chip->seg[4] = pin_init("QE", OUTPUT);
  chip->seg[5] = pin_init("QF", OUTPUT);
  chip->seg[6] = pin_init("QG", OUTPUT);

  chip->latched = 0x0F;   /* power up blank, not showing a stale 0 */

  const pin_watch_config_t cfg = {
    .edge = BOTH,
    .pin_change = on_pin_change,
    .user_data = chip,
  };

  for (int i = 0; i < 4; i++) pin_watch(chip->in[i], &cfg);
  pin_watch(chip->lt, &cfg);
  pin_watch(chip->bi, &cfg);
  pin_watch(chip->le, &cfg);

  update(chip);
}