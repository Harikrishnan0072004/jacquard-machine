/* =====================================================================
 * TilT LOOM DESIGN ENGINE  --  ARDUINO PORT
 * Board: Raspberry Pi Pico  (arduino-pico core by Earle Philhower)
 *
 * WHY THIS FILE LOOKS DIFFERENT FROM THE SDK VERSION
 *   1. No main(). The Arduino core owns main() and calls setup() then
 *      loop() forever. Fighting that is what broke the last build.
 *   2. No stdio_init_all(). The core already brought stdio up, so the
 *      header poisons that call with a static_assert on purpose.
 *   3. No sleep_ms() inside loop(). loop() must RETURN. On the Pico a
 *      long delay is merely rude; on an ESP32-S3 it starves FreeRTOS
 *      and eventually trips the task watchdog. Build the habit now.
 *   4. SDK calls swapped for Arduino calls (pinMode / digitalWrite /
 *      Wire). That is the real prize: change the pin numbers at the top
 *      and this same file runs on an S3.
 *
 * PIN MAP
 *   BCD A/B/C/D : GP22,26,27,28  -> CD4511 -> 7-seg (A = LSB)
 *                 (board nets D1..D4 on header J15)
 *   LCD I2C     : GP6 SDA, GP7 SCL   20x4 @ 0x27
 *   KEYPAD ROWS : GP8,9,10,11
 *   KEYPAD COLS : GP12,13,20,21
 *   SAFETY      : GP19   (SEN_1, opto-isolated)
 *   ENCODER     : GP18   (SEN_2, opto-isolated)
 *   LED A/B     : GP14,15  (board nets DATA_1 / DATA_2)
 *   FREE        : GP2,3,4,5 (SD SPI intact), GP16, GP17
 * ===================================================================== */

#include <Wire.h>
#include <EEPROM.h>
#include <stddef.h>

/* ============================ HARDWARE ============================== */
/* Mapped to the TilT Drop Box carrier board, rev 1.0.
 *   GP19 = SEN_1, GP18 = SEN_2 -> 817B optocoupler field inputs.
 *   These are NOT bare GPIO. Each channel has an opto, a 4.7k pull-up,
 *   100R series, a 100nF filter and a status LED. That is a galvanically
 *   isolated input, built for a switch on a long cable next to a motor.
 *   The loom's electrical noise never reaches the RP2040 die.
 *
 *   POLARITY WARNING: an 817B pulls its output LOW when its LED
 *   conducts. Depending on how the field side is wired, "switch closed"
 *   may read HIGH, not LOW. Verify with a meter before trusting the
 *   jam detection -- a safety input that reads backwards is worse than
 *   no safety input at all. If inverted, flip the two comparisons in
 *   loop() marked POLARITY.
 */
#define SAFETY_SWITCH  19   // SEN_1, opto-isolated
#define ENCODER_SWITCH 18   // SEN_2, opto-isolated

/* Board nets DATA_1 / DATA_2. See the warning in the header block. */
#define LED_A          14   // binary bit 0
#define LED_B          15   // binary bit 1

/* ---------------- BCD -> 7-SEGMENT DECODER (CD4511) -----------------
 * A is the LSB:  GP2=1  GP3=2  GP4=4  GP5=8
 * The 4511 has ACTIVE-HIGH outputs -- it SOURCES current -- so it pairs
 * with a COMMON CATHODE display. (A 74LS47 is the mirror image: active
 * low, sinks current, needs common anode.)
 *
 * BLANKING COSTS ZERO PINS: feed the decoder 10..15 and it shows
 * nothing, because those are not valid BCD codes. We send 15.
 * ------------------------------------------------------------------- */
/* --- WHICH PINS -----------------------------------------------------
 * Board nets D1..D4 on header J15 (the ESP module socket):
 *      D1 = GP22   D2 = GP26   D3 = GP27   D4 = GP28
 *
 * Four contiguous pins, nothing driving them from the board side, and
 * GP2-GP5 stay with the SD card. This is the clean answer.
 *
 * TWO COSTS, both real:
 *   1. J15 must stay EMPTY. Plug an ESP module in later and it fights
 *      the decoder for these lines.
 *   2. GP26/27/28 are the RP2040's ONLY ADC pins. Spending all three on
 *      digital output means this board can never read an analog sensor
 *      -- no pot for speed, no thermistor, no load cell. GP22 is plain
 *      digital and costs nothing; the other three are the expensive
 *      ones. Spend flexible pins last; here we had no choice.
 * ------------------------------------------------------------------- */
const uint8_t BCD_PINS[4] = { 22, 26, 27, 28 };   // A, B, C, D  (A = LSB)

#define BCD_BLANK 15

#define I2C_BUS  Wire1          // GP6/GP7 are i2c1, NOT i2c0
#define I2C_SDA  6
#define I2C_SCL  7
#define I2C_HZ   400000UL
#define LCD_ADDR 0x27            // some backpacks are 0x3F. SCAN IT.

#define LCD_BIT_RS 0x01
#define LCD_BIT_RW 0x02
#define LCD_BIT_EN 0x04
#define LCD_BIT_BL 0x08          // backlight

#define LCD_COLS 20
#define LCD_ROWS 4

/* --- 4x4 MATRIX KEYPAD --- */
#define KP_ROWS 4
#define KP_COLS 4
#define KEY_DEBOUNCE_MS     25
#define KEY_REPEAT_DELAY_MS 500
#define KEY_REPEAT_RATE_MS  120

const uint8_t KP_ROW_PINS[KP_ROWS] = {  13,  11, 9, 8 };
const uint8_t KP_COL_PINS[KP_COLS] = { 20, 21, 12, 10 };

const char KEYMAP[KP_ROWS][KP_COLS] = {
   {'1', '2', '3', '4'},  
  {'5', '6', '7', '8'},  
  {'9', '0', '*', 'L'}, 
  {'B', 'R', '#', 'A'}   

 
};
/* ============================ RUNTIME CFG =========================== */
#define ENABLE_AUTO_SIMULATION 0
#define SIMULATION_RPM         60

/* ============================ DESIGN DATA =========================== */
#define MAX_BLOCKS          32
#define MAX_PICKS_TOTAL    512
#define MAX_PICKS_PER_BLOCK 999

struct PatternBlock {   //structure stores one block
  int box;   
  int picks;
};

/* EEPROM on the Pico is emulated in flash. Store one complete design with
 * a signature and checksum so a first boot or interrupted write is safe. */
#define DESIGN_STORAGE_MAGIC   0x544C5431UL
#define DESIGN_STORAGE_VERSION 1

struct DesignStorage {
  uint32_t magic;
  uint16_t version;
  uint16_t count;
  PatternBlock blocks[MAX_BLOCKS];
  uint32_t checksum;
};

const size_t DESIGN_STORAGE_BYTES = sizeof(DesignStorage);

const PatternBlock DEFAULT_SPEC[] = {
  {1, 80}, {2, 4}, {1, 4}, {2, 2}, {3, 30}, {2, 2},{4,10},
  {1, 2},  {2, 2}, {3, 30}, {2, 2}, {1, 4},  {2, 4},{4, 20}
};
const int DEFAULT_SPEC_COUNT = sizeof(DEFAULT_SPEC) / sizeof(DEFAULT_SPEC[0]); //number of element

PatternBlock design_spec[MAX_BLOCKS];
int spec_count = 0;

int SHUTTLE_PATTERN[MAX_PICKS_TOTAL];
int SHUTTLE_SET_POS[MAX_PICKS_TOTAL];
int SHUTTLE_SET_LEN[MAX_PICKS_TOTAL];
int total_picks = 0;

/* TWO counters, TWO different jobs:
 *   current_pick_index : WHERE AM I. Always 0..total_picks-1.
 *   picks_woven        : HOW MUCH CLOTH. Goes DOWN on pick-back,
 *                        because those picks get re-woven. */
int  current_pick_index    = 0;
long picks_woven           = 0;

bool last_switch_state     = true;
int  virtual_crank_angle   = 0;
bool pick_incremented_flag = false;

/* Was a local in main(). Now file scope, because setup() computes it
 * and loop() uses it, and they are separate functions. */
uint32_t ms_per_degree  = 3;
uint32_t last_degree_ms = 0;

/* ========================= MACHINE STATE ============================
 *   ST_JAM --(switch cleared)--> ST_PAUSED <--('5')--> ST_RUN
 * ST_JAM never exits straight to ST_RUN: after a jam the weaver must
 * pick back to the broken end first, and that needs PAUSED.
 * =================================================================== */
enum MachineState { ST_RUN, ST_PAUSED, ST_JAM };
MachineState machine_state = ST_PAUSED;

/* ====================== HEARTBEAT / DIAGNOSTIC LED ==================
 * GP25 is the Pico module's own LED. It is INSIDE the module, so no
 * carrier-board net can conflict with it -- which is exactly why it is
 * the right pin for "am I alive?".
 *
 * A heartbeat is not decoration. Without it, three very different
 * faults all look identical from outside: code crashed, code stuck in
 * a blocking call, and code running fine but serial not connected.
 * The blink rate tells them apart with no PC attached at all.
 *
 *   fast flutter  : still in boot_menu(), waiting for a keypress
 *   slow 1 Hz     : PAUSED
 *   quick blip    : RUNNING
 *   double flash  : JAM
 * =================================================================== */
#define HB_LED LED_BUILTIN        // GP25 on a Pico

uint32_t hb_last = 0;
uint8_t  hb_step = 0;

void hb_pattern(uint16_t on_ms, uint16_t off_ms) {
  uint32_t now = millis();
  bool lit = digitalRead(HB_LED);
  uint32_t want = lit ? on_ms : off_ms;
  if ((uint32_t)(now - hb_last) >= want) {
    hb_last = now;
    digitalWrite(HB_LED, !lit);
  }
}

/* Blocking blink, used only in setup() before the main loop exists. */
void hb_blink(int times, int ms) {
  for (int i = 0; i < times; i++) {
    digitalWrite(HB_LED, HIGH); delay(ms);
    digitalWrite(HB_LED, LOW);  delay(ms);
  }
}

/* ---------------------- I2C BUS SCANNER -----------------------------
 * Prints every address that ACKs. If the LCD is silent this tells you
 * instantly whether the problem is the ADDRESS (found 0x3F, not 0x27),
 * the WIRING (nothing found at all), or your driver code (found 0x27
 * and it still does not print).
 * ------------------------------------------------------------------- */
void i2c_scan() {
  Serial.println("[I2C] scanning...");
  int found = 0;
  for (uint8_t a = 1; a < 127; a++) {
    I2C_BUS.beginTransmission(a);
    if (I2C_BUS.endTransmission() == 0) {
      Serial.printf("[I2C]   device at 0x%02X\n", a);
      found++;
    }
  }
  if (found == 0)
    Serial.println("[I2C]   NOTHING FOUND -- check SDA/SCL wiring and power");
  else
    Serial.printf("[I2C]   %d device(s). LCD_ADDR is set to 0x%02X\n",
                  found, LCD_ADDR);
}

/* ==================== BCD DECODER DRIVE ============================= */
void bcd_init() {
  for (int i = 0; i < 4; i++) pinMode(BCD_PINS[i], OUTPUT);
  for (int i = 0; i < 4; i++)
    digitalWrite(BCD_PINS[i], (BCD_BLANK >> i) & 1);   // power up blank
}

/* 1..4 shows a box. Anything else (e.g. -1) blanks the display. */
void display_box_number(int box) {
  int code = (box >= 0 && box <= 9) ? box : BCD_BLANK;
  for (int i = 0; i < 4; i++)
    digitalWrite(BCD_PINS[i], (code >> i) & 1);        // bit i -> A,B,C,D
}

/* ====================== LCD DRIVER (I2C / PCF8574) ==================
 * Deliberately NOT using LiquidCrystal_I2C. Driving the PCF8574 by hand
 * is how you learn what that library hides: 4-bit nibbles, an enable
 * pulse, and the HD44780 wake-up dance.
 * =================================================================== */
uint8_t lcd_backlight = LCD_BIT_BL;

void lcd_i2c_write(uint8_t data) {
  I2C_BUS.beginTransmission(LCD_ADDR);
  I2C_BUS.write(data);
  I2C_BUS.endTransmission();
}

void lcd_pulse_enable(uint8_t data) {
  lcd_i2c_write(data | LCD_BIT_EN);
  delayMicroseconds(1);
  lcd_i2c_write(data & ~LCD_BIT_EN);
  delayMicroseconds(50);
}

void lcd_write4(uint8_t nibble, uint8_t mode) {
  lcd_pulse_enable(((nibble & 0x0F) << 4) | mode | lcd_backlight);
}

void lcd_send(uint8_t value, bool is_data) {
  uint8_t mode = is_data ? LCD_BIT_RS : 0x00;
  lcd_write4(value >> 4,   mode);
  lcd_write4(value & 0x0F, mode);
}

void lcd_command(uint8_t cmd) { lcd_send(cmd, false); }
void lcd_char(char c)         { lcd_send((uint8_t)c, true); }

void lcd_clear() {
  lcd_command(0x01);
  delay(2);
}

/* --- 20x4 DDRAM ADDRESSING: THE CLASSIC TRAP ------------------------
 * The HD44780 has ONE 80-byte RAM in two 40-byte halves, at 0x00 and
 * 0x40. A "20x4" panel is those same two halves, each folded at the
 * 20-character mark:
 *      row 0 -> 0x00      row 1 -> 0x40
 *      row 2 -> 0x14      row 3 -> 0x54      (0x14 == 20 decimal)
 * Rows 0 and 2 are physically ONE 40-char line; rows 1 and 3 the other.
 * Guess {0x00,0x40,0x80,0xC0} and row 2 collides with the command
 * range -- the single most-Googled HD44780 bug.
 * ------------------------------------------------------------------- */
void lcd_set_cursor(uint8_t col, uint8_t row) {
  static const uint8_t row_offsets[] = { 0x00, 0x40, 0x14, 0x54 };
  if (row >= LCD_ROWS) row = LCD_ROWS - 1;
  lcd_command(0x80 | (col + row_offsets[row]));
}

void lcd_backlight_set(bool on) {
  lcd_backlight = on ? LCD_BIT_BL : 0x00;
  lcd_i2c_write(lcd_backlight);
}

void lcd_init() {

  /* setSDA/setSCL return false if the pin cannot be routed to THIS bus.
   * Must be called BEFORE begin() -- after begin() they are ignored. */
  bool ok_sda = I2C_BUS.setSDA(I2C_SDA);
  bool ok_scl = I2C_BUS.setSCL(I2C_SCL);
  if (!ok_sda || !ok_scl)
    Serial.printf("[I2C] BAD PIN: SDA %s, SCL %s -- wrong bus object?\n",
                  ok_sda ? "ok" : "REJECTED", ok_scl ? "ok" : "REJECTED");

  I2C_BUS.begin();
  I2C_BUS.setClock(I2C_HZ);

  delay(50);                  // HD44780 needs >40ms after power-up

  /* Wake-up: three 0x03 nibbles force 8-bit mode from any unknown state,
   * then 0x02 switches to 4-bit. Timings are datasheet minimums. */
  lcd_write4(0x03, 0); delay(5);
  lcd_write4(0x03, 0); delayMicroseconds(150);
  lcd_write4(0x03, 0); delayMicroseconds(150);
  lcd_write4(0x02, 0);

  lcd_command(0x28);   // 4-bit, 2-line, 5x8 font
  lcd_command(0x0C);   // display on, cursor off, blink off
  lcd_command(0x06);   // entry mode: increment, no shift
  lcd_clear();
}

void lcd_print_line(uint8_t row, const char *str) {
  lcd_set_cursor(0, row);
  uint8_t i = 0;
  while (str[i] != '\0' && i < LCD_COLS) { lcd_char(str[i]); i++; }
  while (i < LCD_COLS) { lcd_char(' '); i++; }   // pad, so no ghosts
}

const char* box_label(int box) {
  switch (box) {
    case 1: return "Base Level";
    case 2: return "Lift +1";
    case 3: return "Lift +2";
    case 4: return "Full Lift";
    default: return "";
  }
}

/* ========================= KEYPAD DRIVER ============================
 * Rows idle as INPUTS (Hi-Z) and only become OUTPUT during their own
 * scan slot. That is why two keys pressed at once cannot short a driven
 * HIGH row into a driven LOW row. Most tutorial code gets this wrong
 * and survives on luck.
 *
 * Repeat is opt-in PER CALL, not global: auto-repeat is right for a jog
 * key and wrong for data entry -- holding '8' while typing "80" must
 * not produce 8888888. Same hardware, different policy.
 * =================================================================== */
void keypad_init() {
  for (int r = 0; r < KP_ROWS; r++) {
    pinMode(KP_ROW_PINS[r], OUTPUT);
    digitalWrite(KP_ROW_PINS[r], LOW);    // latch a 0 in the output reg
    pinMode(KP_ROW_PINS[r], INPUT);       // then float it
  }
  for (int c = 0; c < KP_COLS; c++)
    pinMode(KP_COL_PINS[c], INPUT_PULLUP);  // idle HIGH
}

char keypad_scan_raw() {
  for (int r = 0; r < KP_ROWS; r++) {
    pinMode(KP_ROW_PINS[r], OUTPUT);      // direction IS the enable
    delayMicroseconds(5);
    for (int c = 0; c < KP_COLS; c++) {
      if (digitalRead(KP_COL_PINS[c]) == LOW) {
        pinMode(KP_ROW_PINS[r], INPUT);
        return KEYMAP[r][c];
      }
    }
    pinMode(KP_ROW_PINS[r], INPUT);
  }
  return 0;
}

char keypad_read(bool allow_repeat) {
  static char     stable      = 0;
  static char     candidate   = 0;
  static uint32_t settle_at   = 0;
  static uint32_t next_repeat = 0;

  char now = keypad_scan_raw();
  uint32_t t = millis();

  if (now != candidate) {                       // changed: restart debounce
    candidate = now;
    settle_at = t + KEY_DEBOUNCE_MS;
    return 0;
  }
  /* Subtract-then-compare, never t < settle_at. This is the rollover-
   * safe form: millis() wraps after ~49 days and a naive comparison
   * would freeze the keypad forever at that moment. */
  if ((int32_t)(t - settle_at) < 0) return 0;   // still bouncing

  if (candidate != stable) {                    // debounced transition
    stable = candidate;
    if (stable != 0) {
      next_repeat = t + KEY_REPEAT_DELAY_MS;
      return stable;                            // fire on PRESS
    }
    return 0;                                   // release: nothing
  }

  /* Key is being HELD. */
  if (allow_repeat && stable != 0 && (int32_t)(t - next_repeat) >= 0) {
    next_repeat = t + KEY_REPEAT_RATE_MS;
    return stable;
  }
  return 0;
}

char keypad_get_key()        { return keypad_read(false); }  // menus, data entry
char keypad_get_key_repeat() { return keypad_read(true);  }  // jogging

char keypad_wait_key() {
  char k;
  while ((k = keypad_get_key()) == 0) { /* spin */ }
  return k;
}

/* ========================= DESIGN HANDLING ========================== */
void design_load_default() {
  spec_count = 0;
  for (int i = 0; i < DEFAULT_SPEC_COUNT && i < MAX_BLOCKS; i++)
    design_spec[spec_count++] = DEFAULT_SPEC[i];
}

int design_total_picks() {
  int sum = 0;
  for (int i = 0; i < spec_count; i++) sum += design_spec[i].picks;
  return sum;
}

uint32_t design_storage_checksum(const DesignStorage &stored) {
  const uint8_t *bytes = reinterpret_cast<const uint8_t *>(&stored);
  uint32_t hash = 2166136261UL;  // FNV-1a
  for (size_t i = 0; i < offsetof(DesignStorage, checksum); i++) {
    hash ^= bytes[i];
    hash *= 16777619UL;
  }
  return hash;
}

bool design_save_flash() {
  DesignStorage stored = {};
  stored.magic = DESIGN_STORAGE_MAGIC;
  stored.version = DESIGN_STORAGE_VERSION;
  stored.count = spec_count;
  for (int i = 0; i < spec_count; i++)
    stored.blocks[i] = design_spec[i];
  stored.checksum = design_storage_checksum(stored);

  EEPROM.put(0, stored);
  if (!EEPROM.commit()) {
    Serial.println("[FLASH] Save failed");
    return false;
  }
  Serial.printf("[FLASH] Design saved (%d blocks, %d picks)\n",
                spec_count, design_total_picks());
  return true;
}

bool design_load_flash() {
  DesignStorage stored = {};
  EEPROM.get(0, stored);

  if (stored.magic != DESIGN_STORAGE_MAGIC ||
      stored.version != DESIGN_STORAGE_VERSION ||
      stored.count == 0 || stored.count > MAX_BLOCKS ||
      stored.checksum != design_storage_checksum(stored)) {
    return false;
  }

  int total = 0;
  for (int i = 0; i < stored.count; i++) {
    if (stored.blocks[i].box < 1 || stored.blocks[i].box > 4 ||
        stored.blocks[i].picks <= 0 ||
        stored.blocks[i].picks > MAX_PICKS_PER_BLOCK) {
      return false;
    }
    total += stored.blocks[i].picks;
    if (total > MAX_PICKS_TOTAL) return false;
  }

  spec_count = stored.count;
  for (int i = 0; i < spec_count; i++)
    design_spec[i] = stored.blocks[i];

  Serial.printf("[FLASH] Design loaded (%d blocks, %d picks)\n",
                spec_count, design_total_picks());
  return true;
}

/* RAM for speed: expand the compressed block list once, so the run loop
 * never has to walk it. 3 arrays x 512 ints ~= 6 KB of the Pico's 264 KB.
 * On an ATmega328 (2 KB) this approach would be impossible. */
bool decompress_pattern() {
  int index = 0;
  for (int i = 0; i < spec_count; i++) {
    for (int p = 0; p < design_spec[i].picks; p++) {
      if (index >= MAX_PICKS_TOTAL) { total_picks = index; return false; }
      SHUTTLE_PATTERN[index] = design_spec[i].box;
      SHUTTLE_SET_POS[index] = p + 1;
      SHUTTLE_SET_LEN[index] = design_spec[i].picks;
      index++;
    }
  }
  total_picks = index;
  return total_picks > 0;
}

/* --- THE SAFE MODULO -------------------------------------------------
 * C truncates toward zero, so the remainder takes the sign of the
 * DIVIDEND:  -1 % 166 == -1  (Python would give 165). Without the fix
 * below, one press of '4' at pick 1 indexes SHUTTLE_PATTERN[-1] --
 * reading memory before the array. It does not crash cleanly: it feeds
 * garbage to display_box_number() and looks like a 7-seg glitch.
 * ------------------------------------------------------------------- */
int wrap_pick(int i) {
  if (total_picks <= 0) return 0;
  int r = i % total_picks;
  if (r < 0) r += total_picks;
  return r;
}

void design_print_serial() {
  Serial.printf("\n--- DESIGN (%d blocks, %d picks) ---\n",
                spec_count, design_total_picks());
  for (int i = 0; i < spec_count; i++)
    Serial.printf("  Blk %2d : BOX %d x %3d picks  (%s)\n",
                  i + 1, design_spec[i].box, design_spec[i].picks,
                  box_label(design_spec[i].box));
  Serial.println("-----------------------------------");
}

/* Errors get row 3 to themselves. On the old 16x2 an error had to
 * overwrite the prompt on row 1, so the operator lost the very
 * instruction he had just failed to follow. */
void ui_error(const char *msg) {
  lcd_print_line(3, msg);
  Serial.printf("[INPUT REJECTED] %s\n", msg);
  delay(900);
}

/* =================== PROGRAMMING UI STATE MACHINE ===================
 * BLOCKING ON PURPOSE. This runs only when the loom is stopped and an
 * operator is standing at the panel, so nothing time-critical is
 * waiting. On an ESP32-S3 with WiFi up you could NOT do this -- you
 * would have to fold it into loop() as more states.
 * =================================================================== */
enum EntryStage { ASK_BOX, ASK_PICKS };

void program_design() {
  EntryStage stage = ASK_BOX;
  int box = 0, picks = 0;
  bool dirty = true;
  char line[LCD_COLS + 1];

  digitalWrite(LED_A, LOW);
  digitalWrite(LED_B, LOW);
  display_box_number(-1);

  spec_count = 0;
  Serial.println("\n[PROGRAM MODE] 1-4=box  0-9=picks  #=confirm  *=back");

  while (true) {
    if (dirty) {
      if (stage == ASK_BOX) {
        lcd_print_line(0, "-- PROGRAM DESIGN --");
        snprintf(line, sizeof(line), "Block %d of %d",
                 spec_count + 1, MAX_BLOCKS);
        lcd_print_line(1, line);
        snprintf(line, sizeof(line), "Total picks: %d", design_total_picks());
        lcd_print_line(2, line);
        lcd_print_line(3, "BOX? 1-4  #RUN *DEL");
      } else {
        snprintf(line, sizeof(line), "BOX %d  %s", box, box_label(box));
        lcd_print_line(0, line);
        lcd_print_line(1, "How many picks?");
        snprintf(line, sizeof(line), "> %d_", picks);
        lcd_print_line(2, line);
        lcd_print_line(3, "#=OK  *=CLR");
      }
      dirty = false;
    }

    char k = keypad_get_key();
    if (k == 0) continue;

    if (stage == ASK_BOX) {
      if (k >= '1' && k <= '4') {
        box = k - '0'; picks = 0; stage = ASK_PICKS; dirty = true;
      } else if (k == '#') {
        if (spec_count == 0) { ui_error("EMPTY - add a block"); dirty = true; }
        else break;
      } else if (k == '*') {
        if (spec_count > 0) { spec_count--; dirty = true; }
        else { ui_error("Nothing to delete"); dirty = true; }
      } else { ui_error("BOX must be 1-4"); dirty = true; }
    }
    else {
      if (k >= '0' && k <= '9') {
        int next = picks * 10 + (k - '0');
        if (next <= MAX_PICKS_PER_BLOCK) { picks = next; dirty = true; }
        else { ui_error("Max 999 picks"); dirty = true; }
      }
      else if (k == '#') {
        if (picks == 0) {
          ui_error("Picks must be > 0");
        } else if (spec_count >= MAX_BLOCKS) {
          ui_error("Block limit reached");
        } else if (design_total_picks() + picks > MAX_PICKS_TOTAL) {
          snprintf(line, sizeof(line), "Over %d pick limit", MAX_PICKS_TOTAL);
          ui_error(line);
        } else {
          design_spec[spec_count].box   = box;
          design_spec[spec_count].picks = picks;
          spec_count++;
          Serial.printf("  stored Blk %2d : BOX %d x %d\n", spec_count, box, picks);
          stage = ASK_BOX;
        }
        dirty = true;
      }
      else if (k == '*') {
        if (picks > 0) picks = 0; else stage = ASK_BOX;
        dirty = true;
      }
    }
  }

  design_print_serial();
  if (!design_save_flash())
    lcd_print_line(3, "FLASH SAVE FAILED");
  lcd_print_line(0, "DESIGN LOADED");
  snprintf(line, sizeof(line), "Blocks : %d", spec_count);
  lcd_print_line(1, line);
  snprintf(line, sizeof(line), "Picks  : %d", design_total_picks());
  lcd_print_line(2, line);
  lcd_print_line(3, "Press 5 to run");
  delay(1500);
}

/* ============================ RUN OUTPUTS =========================== */
void lcd_show_status(int pattern_index) {
  char line[LCD_COLS + 1];
  int box = SHUTTLE_PATTERN[pattern_index];

  snprintf(line, sizeof(line), "BOX %d", box);
  lcd_print_line(0, line);

  snprintf(line, sizeof(line), "SET  %3d/%-3d",
           SHUTTLE_SET_POS[pattern_index], SHUTTLE_SET_LEN[pattern_index]);
  lcd_print_line(1, line);

  snprintf(line, sizeof(line), "PICK %3d/%-3d",
           pattern_index + 1, total_picks);
  lcd_print_line(2, line);

  if (machine_state == ST_PAUSED)
    snprintf(line, sizeof(line), "STOP 4<< 6>> 5=RUN");
  else
    snprintf(line, sizeof(line), "RUN  TOTAL %ld", picks_woven);
  lcd_print_line(3, line);
}

void lcd_show_jam() {
  char line[LCD_COLS + 1];
  lcd_print_line(0, "**  SHUTTLE JAM  **");
  lcd_print_line(1, "MACHINE STOPPED");
  snprintf(line, sizeof(line), "Held at pick %d", current_pick_index + 1);
  lcd_print_line(2, line);
  lcd_print_line(3, "Clear SEN_1 input");
}

void update_outputs(int pattern_index) {
  int box_number = SHUTTLE_PATTERN[pattern_index];
  display_box_number(box_number);
  switch (box_number) {
    case 1: digitalWrite(LED_A, LOW);  digitalWrite(LED_B, LOW);  break;
    case 2: digitalWrite(LED_A, HIGH); digitalWrite(LED_B, LOW);  break;
    case 3: digitalWrite(LED_A, LOW);  digitalWrite(LED_B, HIGH); break;
    case 4: digitalWrite(LED_A, HIGH); digitalWrite(LED_B, HIGH); break;
  }
  lcd_show_status(pattern_index);
}

/* ======================= PICK FINDING (JOG) =========================
 * delta -1 backs one pick out, +1 steps one on. Only ever called from
 * ST_PAUSED: shifting a box while the shuttle is in flight would smash
 * the shuttle. That is why jog is state-gated, not just guarded.
 * =================================================================== */
void jog_pick(int delta) {
  if (total_picks <= 0) return;

  current_pick_index = wrap_pick(current_pick_index + delta);

  /* Backed-out picks get re-woven, so un-count them. Clamped at 0 --
   * you cannot un-weave cloth you never made. */
  picks_woven += delta;
  if (picks_woven < 0) picks_woven = 0;

  /* Reset the crank so resuming does not immediately eat a pick. */
  virtual_crank_angle   = 0;
  pick_incremented_flag = true;

  Serial.printf("[JOG %s] now at pick %d/%d (BOX %d)\n",
                delta < 0 ? "BACK" : "FWD ",
                current_pick_index + 1, total_picks,
                SHUTTLE_PATTERN[current_pick_index]);

  update_outputs(current_pick_index);
}

/* ============================== BOOT MENU =========================== */
void boot_menu() {
  char line[LCD_COLS + 1];
  lcd_print_line(0, "TilT LOOM ENGINE");
  snprintf(line, sizeof(line), "Saved design: %d pk", design_total_picks());
  lcd_print_line(1, line);
  lcd_print_line(2, "* = PROGRAM new");
  lcd_print_line(3, "# = RUN saved");

  /* Re-print the prompt every 3 s. If you open the serial monitor late
   * -- which is normal, because the USB port re-enumerates after an
   * upload -- you still see it instead of an empty window. The old
   * version printed once and then sat silent forever. */
  uint32_t last_nag = 0;
  while (true) {
    hb_pattern(80, 80);                 // fast flutter = waiting for a key

    if (millis() - last_nag > 3000) {
      last_nag = millis();
      Serial.println("[BOOT] Waiting for keypad: '*' = PROGRAM, '#' = RUN");
    }

    char k = keypad_get_key();
    if (k != 0)   Serial.printf("[BOOT] key pressed: '%c'\n", k);
    if (k == '*') { digitalWrite(HB_LED, LOW); program_design(); return; }
    if (k == '#') { digitalWrite(HB_LED, LOW); design_print_serial(); return; }
  }
}

/* ================================ SETUP ============================= */
void setup() {
  Serial.begin(115200);

  uint32_t t0 = millis();
  while (!Serial && millis() - t0 < 2000) { }

  pinMode(HB_LED, OUTPUT);
  hb_blink(3, 120);            // 3 flashes = setup() reached, code is running

  pinMode(LED_A, OUTPUT);
  pinMode(LED_B, OUTPUT);
  pinMode(ENCODER_SWITCH, INPUT_PULLUP);
  pinMode(SAFETY_SWITCH,  INPUT_PULLUP);

  bcd_init();
  lcd_init();
  i2c_scan();                  // tells you the real LCD address
  keypad_init();

  Serial.println("\n=================================================");
  Serial.println("  TilT LOOM ENGINE -- 4x4 KEYPAD + CD4511 + 20x4 ");
  Serial.println("=================================================");

  EEPROM.begin(DESIGN_STORAGE_BYTES);
  if (!design_load_flash()) {
    design_load_default();
    Serial.println("[FLASH] No valid saved design; using default");
  }
  boot_menu();

  if (!decompress_pattern()) {
    lcd_print_line(0, "DESIGN ERROR");
    lcd_print_line(1, "Empty or too long");
    Serial.println("[FATAL] Design invalid. Halting.");
    while (true) { }
  }

  Serial.printf("[SYSTEM] Decompressed: %d picks loaded.\n", total_picks);
  Serial.println("[KEYS] 5=run/pause  4=back  6=fwd  0=goto 1  *=reprogram\n");

  current_pick_index = 0;
  picks_woven        = 0;
  machine_state      = ST_PAUSED;     // start stopped; weaver presses 5
  update_outputs(current_pick_index);

  ms_per_degree = (uint32_t)((60.0f / SIMULATION_RPM) * 1000.0f / 360.0f + 0.5f);
  if (ms_per_degree < 1) ms_per_degree = 1;
  last_degree_ms = millis();
}

/* ================================ LOOP =============================
 * Every path through this function RETURNS. No delay() anywhere in the
 * run path -- the crank simulation is driven by comparing millis()
 * against a deadline instead. That is the difference between "works"
 * and "works while also doing something else".
 * =================================================================== */
void loop() {
  /* Heartbeat first, so it keeps beating no matter which branch runs. */
  switch (machine_state) {
    case ST_RUN:    hb_pattern(40, 960);  break;   // quick blip
    case ST_PAUSED: hb_pattern(500, 500); break;   // slow 1 Hz
    case ST_JAM:    hb_pattern(120, 120); break;   // urgent
  }

  /* ---- SAFETY IS CHECKED EVERY PASS, IN EVERY STATE ---------------- */
  if (digitalRead(SAFETY_SWITCH) == LOW && machine_state != ST_JAM) {   // POLARITY
    machine_state = ST_JAM;
    digitalWrite(LED_A, LOW);
    digitalWrite(LED_B, LOW);
    display_box_number(-1);          // -> BCD 15 -> blank
    lcd_show_jam();
    Serial.printf("[ALARM] SHUTTLE JAM at pick %d. Clear SEN_1 input.\n",
                  current_pick_index + 1);
  }

  switch (machine_state) {

  /* ========================= STATE: JAM ============================ */
  case ST_JAM:
    if (digitalRead(SAFETY_SWITCH) == HIGH) {                 // POLARITY
      /* Exit to PAUSED, never straight to RUN. */
      machine_state     = ST_PAUSED;
      last_switch_state = digitalRead(ENCODER_SWITCH);
      Serial.println("[SYSTEM] Jam cleared. PAUSED -- use 4/6 to pick find.");
      update_outputs(current_pick_index);
    }
    break;

  /* ======================== STATE: PAUSED ========================== */
  case ST_PAUSED: {
    char k = keypad_get_key_repeat();      // jogging -> repeat ON
    if (k == '4') jog_pick(-1);
    else if (k == '6') jog_pick(+1);
    else if (k == '0') {
      current_pick_index    = 0;
      virtual_crank_angle   = 0;
      pick_incremented_flag = true;
      Serial.println("[JOG] jumped to pick 1");
      update_outputs(current_pick_index);
    }
    else if (k == '5') {
      machine_state         = ST_RUN;
      virtual_crank_angle   = 0;
      pick_incremented_flag = true;        // don't eat a pick on resume
      last_degree_ms        = millis();
      Serial.printf("[SYSTEM] RUN from pick %d\n", current_pick_index + 1);
      update_outputs(current_pick_index);
    }
    else if (k == '*') {
      program_design();
      if (!decompress_pattern()) {
        lcd_print_line(0, "DESIGN ERROR");
        lcd_print_line(1, "Using default");
        delay(1200);
        design_load_default();
        decompress_pattern();
      }
      current_pick_index    = 0;
      picks_woven           = 0;
      virtual_crank_angle   = 0;
      pick_incremented_flag = true;
      last_switch_state     = digitalRead(ENCODER_SWITCH);
      update_outputs(current_pick_index);
    }
    break;
  }

  /* ========================= STATE: RUN ============================ */
  case ST_RUN: {
    char k = keypad_get_key();             // running -> repeat OFF
    if (k == '5') {
      machine_state = ST_PAUSED;
      Serial.printf("[SYSTEM] PAUSED at pick %d. 4=back 6=fwd 5=resume\n",
                    current_pick_index + 1);
      update_outputs(current_pick_index);
      break;
    }

#if ENABLE_AUTO_SIMULATION
    /* Crank simulation, non-blocking. The old sleep_ms(ms_per_degree)
     * became this deadline check: advance one degree only when enough
     * time has passed, then fall straight out of loop(). */
    uint32_t now = millis();
    if ((uint32_t)(now - last_degree_ms) >= ms_per_degree) {
      last_degree_ms += ms_per_degree;

      virtual_crank_angle++;
      if (virtual_crank_angle >= 360) virtual_crank_angle = 0;

      if (virtual_crank_angle == 240) {          // dwell window
        Serial.printf("Angle:240 | Pick %d/%d -> BOX %d\n",
                      current_pick_index + 1, total_picks,
                      SHUTTLE_PATTERN[current_pick_index]);
        update_outputs(current_pick_index);
      }
      if (virtual_crank_angle == 0 && !pick_incremented_flag) {
        current_pick_index = wrap_pick(current_pick_index + 1);
        picks_woven++;
        pick_incremented_flag = true;
      }
      if (virtual_crank_angle > 10) pick_incremented_flag = false;
    }
#else
    /* --- MANUAL BUTTON MODE (GP18 / SEN_2) --- */
    bool cur = (digitalRead(ENCODER_SWITCH) == HIGH);
    if (last_switch_state == true && cur == false) {
      delay(30);                                  // crude debounce
      if (digitalRead(ENCODER_SWITCH) == LOW) {
        current_pick_index = wrap_pick(current_pick_index + 1);
        picks_woven++;
        Serial.printf("Pick complete | %d/%d\n",
                      current_pick_index + 1, total_picks);
        update_outputs(current_pick_index);
      }
    }
    last_switch_state = cur;
#endif
    break;
  }
  }
}
