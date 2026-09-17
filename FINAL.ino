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
#include <string.h>

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

/* External 24C32 EEPROM (Drop Box rev 1.0, "MEMORY SYSTEM", U6), on the
 * same I2C1 bus as the LCD. A0/A1/A2 read as grounded on the board ->
 * 0x50. CONFIRM with i2c_scan() before trusting this -- if it comes up
 * wrong, check the address pins. WP must be tied LOW (write-enabled);
 * if it is tied high the chip still ACKs every write and eeprom_self_test()
 * below is what catches that, not endTransmission(). */
#define EEPROM_I2C_ADDR    0x50
#define EEPROM_PAGE_BYTES  32     // 24C32 page size -- writes must not cross this
#define EEPROM_TOTAL_BYTES 4096   // 24C32 = 32Kbit

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
#define MAX_BLOCKS          100
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

/* Defined here (not down by the functions that use it) so it is visible
 * at the top of the file, where the Arduino IDE inserts its auto-generated
 * function prototypes -- a struct defined AFTER that point makes any
 * prototype referencing it fail with "does not name a type". */
struct RunStateRecord {
  uint32_t magic;
  uint32_t seq;
  int32_t  pick_index;
  long     picks_woven;
  uint8_t  state;
  uint8_t  _pad[3];
  uint32_t checksum;
};

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
 *   ST_JAM --(switch cleared)--> ST_RUN
 * Only two states. The machine runs continuously; jog (A/B), goto-pick-1
 * (0) and the menu (#) all work live from ST_RUN. ST_JAM is the one
 * exception -- a real safety stop, not an operator convenience -- and
 * it auto-resumes ST_RUN the instant SEN_1 clears.
 * =================================================================== */
enum MachineState { ST_RUN, ST_JAM };
MachineState machine_state = ST_RUN;

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
      Serial.printf("[KEY] '%c' pressed  (state=%d)\n", stable, machine_state);
      return stable;                            // fire on PRESS
    }
    return 0;                                   // release: nothing
  }

  /* Key is being HELD. */
  if (allow_repeat && stable != 0 && (int32_t)(t - next_repeat) >= 0) {
    next_repeat = t + KEY_REPEAT_RATE_MS;
    Serial.printf("[KEY] '%c' repeat   (state=%d)\n", stable, machine_state);
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
  if (!EEPROM.commit()) {  // Erase-And-rewrite that  
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

/* =============== RUN-STATE PERSISTENCE (EXTERNAL 24C32) =============
 * The DESIGN (block list) is rewritten rarely -- flash is fine for that
 * and is left alone above. The RUN POSITION changes every pick, which
 * would wear out a flash sector fast. This chip is rated for it, but
 * only per PAGE, and only with the writes SPREAD OUT -- so this is a
 * circular log, not a fixed address: each save goes to the next of 127
 * pages (the 128th is reserved as self-test scratch, see below) and
 * carries a rising sequence number. On boot every page is read back and
 * the highest-sequence record that still passes its checksum wins --
 * that also means a save torn by a mid-write power loss is simply
 * ignored in favor of the last good one, instead of resuming garbage.
 *
 * machine_state is recorded but deliberately NOT restored as ST_RUN in
 * runstate_load_latest() -- see the comment in setup(). A loom should
 * never come back up moving on its own just because power returned.
 * =================================================================== */
#define RUN_STATE_MAGIC       0x54524E31UL   // 'TRN1'
#define RUN_STATE_SLOT_COUNT  ((EEPROM_TOTAL_BYTES / EEPROM_PAGE_BYTES) - 1)  // 127; last page = self-test scratch
#define RUN_STATE_TEST_ADDR   (RUN_STATE_SLOT_COUNT * EEPROM_PAGE_BYTES)      // 4064

uint32_t run_state_next_slot = 0;
uint32_t run_state_next_seq  = 1;
bool     run_state_dirty     = false;

void mark_run_state_dirty() { run_state_dirty = true; }

bool eeprom_write_page(uint16_t addr, const uint8_t *data, uint8_t len) {
  I2C_BUS.beginTransmission(EEPROM_I2C_ADDR);
  I2C_BUS.write((uint8_t)(addr >> 8));
  I2C_BUS.write((uint8_t)(addr & 0xFF));
  I2C_BUS.write(data, len);
  if (I2C_BUS.endTransmission() != 0) return false;
  delay(5);                 // tWR internal write cycle, datasheet max ~5ms
  return true;
}

bool eeprom_read_bytes(uint16_t addr, uint8_t *data, uint8_t len) {
  I2C_BUS.beginTransmission(EEPROM_I2C_ADDR);
  I2C_BUS.write((uint8_t)(addr >> 8));
  I2C_BUS.write((uint8_t)(addr & 0xFF));
  if (I2C_BUS.endTransmission(false) != 0) return false;   // repeated start, no stop
  if (I2C_BUS.requestFrom(EEPROM_I2C_ADDR, (int)len) != len) return false;
  for (uint8_t i = 0; i < len; i++) data[i] = I2C_BUS.read();
  return true;
}

/* WP-high failure mode: the chip ACKs the write transaction normally and
 * only silently drops the memory update. A bare endTransmission() check
 * cannot see that -- only a read-back comparison can. Uses the reserved
 * scratch page, never touches a real run-state slot. */
bool eeprom_self_test() {
  uint8_t pattern[8] = { 0xA5, 0x5A, 0x3C, 0xC3, 0x00, 0xFF, 0x11, 0xEE };
  uint8_t readback[8] = {0};

  if (!eeprom_write_page(RUN_STATE_TEST_ADDR, pattern, sizeof(pattern))) {
    Serial.println("[EEPROM] self-test FAILED: no ACK -- check wiring/address");
    return false;
  }
  if (!eeprom_read_bytes(RUN_STATE_TEST_ADDR, readback, sizeof(readback))) {
    Serial.println("[EEPROM] self-test FAILED: read-back error");
    return false;
  }
  if (memcmp(pattern, readback, sizeof(pattern)) != 0) {
    Serial.println("[EEPROM] self-test FAILED: data mismatch -- is WP tied high?");
    return false;
  }
  Serial.println("[EEPROM] self-test OK (24C32 present, writable)");
  return true;
}

uint32_t runstate_checksum(const RunStateRecord &rec) {
  const uint8_t *bytes = reinterpret_cast<const uint8_t *>(&rec);
  uint32_t hash = 2166136261UL;   // FNV-1a, same scheme as design_storage_checksum
  for (size_t i = 0; i < offsetof(RunStateRecord, checksum); i++) {
    hash ^= bytes[i];
    hash *= 16777619UL;
  }
  return hash;
}

bool runstate_save() {
  RunStateRecord rec;
  rec.magic       = RUN_STATE_MAGIC;
  rec.seq         = run_state_next_seq;
  rec.pick_index  = current_pick_index;
  rec.picks_woven = picks_woven;
  rec.state       = (uint8_t)machine_state;
  rec._pad[0] = rec._pad[1] = rec._pad[2] = 0;
  rec.checksum    = runstate_checksum(rec);

  uint16_t addr = (uint16_t)(run_state_next_slot * EEPROM_PAGE_BYTES);
  bool ok = eeprom_write_page(addr, reinterpret_cast<const uint8_t *>(&rec), sizeof(rec));
  if (!ok) {
    Serial.println("[EEPROM] run-state save FAILED (I2C error)");
    return false;
  }
  run_state_next_seq++;
  run_state_next_slot = (run_state_next_slot + 1) % RUN_STATE_SLOT_COUNT;
  return true;
}

/* Restores current_pick_index / picks_woven only. machine_state is read
 * into the record but the caller (setup()) decides what to do with it --
 * intentionally not applied here. */
bool runstate_load_latest() {
  RunStateRecord best = {};
  bool have_best = false;
  uint32_t best_slot = 0;

  for (uint32_t s = 0; s < RUN_STATE_SLOT_COUNT; s++) {
    RunStateRecord rec;
    if (!eeprom_read_bytes((uint16_t)(s * EEPROM_PAGE_BYTES),
                            reinterpret_cast<uint8_t *>(&rec), sizeof(rec)))
      continue;
    if (rec.magic != RUN_STATE_MAGIC) continue;
    if (rec.checksum != runstate_checksum(rec)) continue;
    if (!have_best || rec.seq > best.seq) { best = rec; have_best = true; best_slot = s; }
  }

  if (!have_best) {
    Serial.println("[EEPROM] no valid run-state record found; starting fresh");
    run_state_next_slot = 0;
    run_state_next_seq  = 1;
    return false;
  }

  current_pick_index = best.pick_index;
  picks_woven         = best.picks_woven;
  run_state_next_slot = (best_slot + 1) % RUN_STATE_SLOT_COUNT;
  run_state_next_seq  = best.seq + 1;
  Serial.printf("[EEPROM] resumed at pick %d, %ld woven (slot %lu, seq %lu)\n",
                current_pick_index + 1, picks_woven,
                (unsigned long)best_slot, (unsigned long)best.seq);
  return true;
}

/* =================== PROGRAMMING UI STATE MACHINE ===================
 * BLOCKING ON PURPOSE, with an operator standing at the panel to drive
 * it. Reachable from ST_RUN via the menu (#) -- nothing here stops the
 * loom mechanically, it only stops loop() from tracking SEN_2 pick
 * advances while this is open, same caveat as run_menu()/design_view().
 * On an ESP32-S3 with WiFi up you could NOT do this -- you would have
 * to fold it into loop() as more states.
 * =================================================================== */
enum EntryStage { ASK_BOX, ASK_PICKS };

void program_design() {
  EntryStage stage = ASK_BOX;
  int box = 0, picks = 0;
  bool dirty = true;
  char line[LCD_COLS + 1];

  /* '*' at ASK_BOX cancels out of here entirely -- back up the design
   * that's currently live before clearing spec_count below, so cancel
   * can put it back exactly as it was instead of leaving the machine
   * on an empty/partial design. */
  int previous_count = spec_count;
  PatternBlock previous_spec[MAX_BLOCKS];
  for (int i = 0; i < previous_count; i++) previous_spec[i] = design_spec[i];

  digitalWrite(LED_A, LOW);
  digitalWrite(LED_B, LOW);
  display_box_number(-1);

  spec_count = 0;
  Serial.println("\n[PROGRAM MODE] 1-4=box  0-9=picks  #=confirm  *=cancel");

  while (true) {
    if (dirty) {
      if (stage == ASK_BOX) {
        lcd_print_line(0, "-- PROGRAM DESIGN --");
        snprintf(line, sizeof(line), "Block %d of %d",
                 spec_count + 1, MAX_BLOCKS);
        lcd_print_line(1, line);
        snprintf(line, sizeof(line), "Total picks: %d", design_total_picks());
        lcd_print_line(2, line);
        lcd_print_line(3, "BOX? 1-4 #RUN *BACK");
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
        spec_count = previous_count;
        for (int i = 0; i < spec_count; i++) design_spec[i] = previous_spec[i];
        Serial.println("[PROGRAM MODE] cancelled -- previous design kept");
        lcd_print_line(0, "CANCELLED");
        lcd_print_line(1, "Previous design kept");
        lcd_print_line(2, "");
        lcd_print_line(3, "");
        delay(900);
        return;
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
  lcd_print_line(3, "");
  delay(1500);
}

/* ==================== IN-RUN DESIGN MENU (key '#') ===================
 * BLOCKING, same as program_design(): while this is open, loop() never
 * reaches the SEN_2 pick-advance check, so any picks the loom makes
 * while the menu is open go uncounted. There is no state to gate this
 * on anymore -- machine_state stays ST_RUN the whole time; it is on the
 * operator to keep it brief.
 * =================================================================== */
void design_view() {
  char line[LCD_COLS + 1];
  int page = 0;
  int page_count = (spec_count + 1) / 2;
  if (page_count < 1) page_count = 1;
  bool dirty = true;

  while (true) {
    if (dirty) {
      lcd_print_line(0, "-- CURRENT DESIGN --");

      int first = page * 2;
      if (first < spec_count) {
        snprintf(line, sizeof(line), "B%02d Box%d x %3d", first + 1,
                 design_spec[first].box, design_spec[first].picks);
        lcd_print_line(1, line);
      } else {
        lcd_print_line(1, "");
      }

      if (first + 1 < spec_count) {
        snprintf(line, sizeof(line), "B%02d Box%d x %3d", first + 2,
                 design_spec[first + 1].box, design_spec[first + 1].picks);
        lcd_print_line(2, line);
      } else {
        lcd_print_line(2, "");
      }

      snprintf(line, sizeof(line), "Pg %d/%d  B<< A>> *Back",
               page + 1, page_count);
      lcd_print_line(3, line);
      dirty = false;
    }

    char k = keypad_get_key();
    if (k == '*') return;
    else if (k == 'A' && page + 1 < page_count) { page++; dirty = true; }
    else if (k == 'B' && page > 0)               { page--; dirty = true; }
  }
}

void run_menu() {
  bool dirty = true;

  while (true) {
    if (dirty) {
      lcd_print_line(0, "-- MENU --");
      lcd_print_line(1, "1 Current design");
      lcd_print_line(2, "2 New design");
      lcd_print_line(3, "3 (soon)   *=Back");
      dirty = false;
    }

    char k = keypad_get_key();
    if (k == '*') return;

    else if (k == '1') {
      design_view();
      dirty = true;
    }
    else if (k == '2') {
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
      mark_run_state_dirty();
      update_outputs(current_pick_index);
      return;
    }
    else if (k == '3') {
      ui_error("Coming soon");
      dirty = true;
    }
  }
}

/* ============================ RUN OUTPUTS =========================== */
void lcd_show_status(int pattern_index) {
  char line[LCD_COLS + 1];
  int box = SHUTTLE_PATTERN[pattern_index];

  /* Top-right corner: "R*" -- always running here, ST_JAM has its own
   * screen (lcd_show_jam()), so reaching this function means RUN. */
  char box_str[LCD_COLS + 1];
  snprintf(box_str, sizeof(box_str), "BOX %d", box);
  snprintf(line, sizeof(line), "%-18sR*", box_str);
  lcd_print_line(0, line);

  snprintf(line, sizeof(line), "SET  %3d/%-3d",
           SHUTTLE_SET_POS[pattern_index], SHUTTLE_SET_LEN[pattern_index]);
  lcd_print_line(1, line);

  snprintf(line, sizeof(line), "PICK %3d/%-3d",
           pattern_index + 1, total_picks);
  lcd_print_line(2, line);

  /* Preview the block that starts right after this one ends, not just
   * "how much woven so far". remaining_in_block = picks still left in
   * THIS block (0 on the block's last pick), so pattern_index + that +
   * 1 lands exactly on the first pick of the next block. wrap_pick()
   * handles the case where the current block is the design's last --
   * "next" is then the first block of the design repeating. */
  int remaining_in_block = SHUTTLE_SET_LEN[pattern_index] - SHUTTLE_SET_POS[pattern_index];
  int next_index = wrap_pick(pattern_index + remaining_in_block + 1);
  int next_box   = SHUTTLE_PATTERN[next_index];
  int next_len   = SHUTTLE_SET_LEN[next_index];
  snprintf(line, sizeof(line), "NEXT BOX %d : %d", next_box, next_len);
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

/* ------------------- DATA_1 / DATA_2 COIL PULSE ----------------------
 * LED_A/LED_B don't just drive indicator LEDs -- on the real box
 * mechanism they energise a coil that pulls a rod.
 *
 * TIMING RULE: a coil that charges for a box transition stays ON for
 * exactly one pick -- the entry pick -- and releases the instant the
 * NEXT pick is reached, whether or not that next pick is still the
 * same box. This is event-based, not time-based: there is no timer,
 * so a coil charged right before the machine stalls (paused/jammed
 * with no next pick coming) stays energised until a next pick actually
 * arrives. update_outputs() is the single funnel every "here is the
 * new current pick" event goes through (real pick advances, jogs, menu
 * returns, jam-clear resume), so it is the one place that needs to
 * enforce this -- see the release-then-maybe-recharge sequence there.
 * ------------------------------------------------------------------- */
void coil_pulse(uint8_t pin)   { digitalWrite(pin, HIGH); }
void coil_release(uint8_t pin) { digitalWrite(pin, LOW);  }

/* -1 = no box driven yet -- position unknown at power-up, so the very
 * first update_outputs() call always pulses for real instead of being
 * skipped as "no change". Left alone by the jam handler on purpose
 * (see the comment there): a jam clearing back into the SAME box must
 * not look like a fresh box change. */
int last_output_box = -1;

/* Which coil(s) actually need to fire for a given box-to-box move.
 * This is the real mechanism, not an arbitrary binary code: 1<->2 and
 * 3<->4 are single-linkage steps (one coil moves the mechanism across
 * that boundary), 2<->3 needs both linkages. Only these three defined
 * pairs ever pulse anything -- a jump that skips a box (1<->3, 1<->4,
 * 2<->4), or the very first drive after power-up (from_box == -1), is
 * not expected to occur in a real design, so both coils are just left
 * released rather than guessed at. */
void coil_apply_transition(int from_box, int to_box) {
  int lo = from_box < to_box ? from_box : to_box;
  int hi = from_box < to_box ? to_box   : from_box;

  bool pulse_a, pulse_b;
  if      (lo == 1 && hi == 2) { pulse_a = true;  pulse_b = false; }
  else if (lo == 2 && hi == 3) { pulse_a = true;  pulse_b = true;  }
  else if (lo == 3 && hi == 4) { pulse_a = false; pulse_b = true;  }
  else                         { pulse_a = false; pulse_b = false; } // undefined jump: no pulse

  if (pulse_a) coil_pulse(LED_A); else coil_release(LED_A);
  if (pulse_b) coil_pulse(LED_B); else coil_release(LED_B);
}

void update_outputs(int pattern_index) {
  int box_number = SHUTTLE_PATTERN[pattern_index];
  display_box_number(box_number);

  /* Release first: whatever charged on the PREVIOUS call has now lived
   * exactly one pick, so its time is up the moment this pick is
   * reached -- regardless of whether the box changed. Only then decide
   * whether THIS pick's box change needs a fresh charge. Coalesced
   * into one call so a coil that needs to stay on for a fresh
   * transition (e.g. two consecutive box-3 entries) doesn't visibly
   * chatter -- release and re-pulse happen back-to-back with no delay
   * between them. */
  coil_release(LED_A);
  coil_release(LED_B);

  if (box_number != last_output_box) {
    coil_apply_transition(last_output_box, box_number);
    last_output_box = box_number;
  }

  lcd_show_status(pattern_index);
}

/* ======================= PICK FINDING (JOG) =========================
 * delta -1 backs one pick out, +1 steps one on. Called live from
 * ST_RUN (A/B), by operator request -- NOT from ST_JAM, which is
 * handled separately.
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
  mark_run_state_dirty();

  Serial.printf("[JOG %s] now at pick %d/%d (BOX %d)\n",
                delta < 0 ? "BACK" : "FWD ",
                current_pick_index + 1, total_picks,
                SHUTTLE_PATTERN[current_pick_index]);

  update_outputs(current_pick_index);
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
  eeprom_self_test();          // confirms the 24C32 is present AND writable (catches WP tied high)
  keypad_init();

  Serial.println("\n=================================================");
  Serial.println("  TilT LOOM ENGINE -- 4x4 KEYPAD + CD4511 + 20x4 ");
  Serial.println("=================================================");

  EEPROM.begin(DESIGN_STORAGE_BYTES);
  if (!design_load_flash()) {
    design_load_default();
    Serial.println("[FLASH] No valid saved design; using default");
  }

  if (!decompress_pattern()) {
    lcd_print_line(0, "DESIGN ERROR");
    lcd_print_line(1, "Empty or too long");
    Serial.println("[FATAL] Design invalid. Halting.");
    while (true) { }
  }

  Serial.printf("[SYSTEM] Decompressed: %d picks loaded.\n", total_picks);
  Serial.println("[KEYS] B=back  A=fwd  0=goto 1  #=menu\n");

  /* Resume position after a power cut -- only the pick position/count
   * carry over, and only if it still fits the design that just got
   * loaded (e.g. after a saved position was made for a different
   * design). machine_state itself is never read back from the saved
   * record: every boot goes straight into ST_RUN, no keypress needed. */
  if (runstate_load_latest()) {
    if (current_pick_index < 0 || current_pick_index >= total_picks) {
      Serial.println("[EEPROM] saved pick index doesn't fit this design; resetting to pick 1");
      current_pick_index = 0;
      picks_woven        = 0;
    }
  } else {
    current_pick_index = 0;
    picks_woven         = 0;
  }

  machine_state          = ST_RUN;
  virtual_crank_angle     = 0;
  pick_incremented_flag   = true;      // don't eat a pick on the very first pass
  last_switch_state       = digitalRead(ENCODER_SWITCH);
  mark_run_state_dirty();
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
    case ST_RUN: hb_pattern(40, 960);  break;   // quick blip
    case ST_JAM: hb_pattern(120, 120); break;   // urgent
  }

  /* ---- SAFETY IS CHECKED EVERY PASS, IN EVERY STATE ---------------- */
  if (digitalRead(SAFETY_SWITCH) == LOW && machine_state != ST_JAM) {   // POLARITY
    machine_state = ST_JAM;
    /* Immediate safety cutoff -- a jam can't wait for the next pick
     * event, which is what update_outputs() normally waits for. */
    coil_release(LED_A);
    coil_release(LED_B);
    /* last_output_box is deliberately left alone -- a jam is not a box
     * change. If we reset it to -1 here, the next update_outputs() call
     * (on jam clear) would see box_number != last_output_box and fire a
     * fresh coil pulse into the SAME box it was already on. Coils pulse
     * only on an actual box change; jam/clear must not trigger one. */
    display_box_number(-1);          // -> BCD 15 -> blank
    lcd_show_jam();
    mark_run_state_dirty();
    Serial.printf("[ALARM] SHUTTLE JAM at pick %d. Clear SEN_1 input.\n",
                  current_pick_index + 1);
  }

  switch (machine_state) {

  /* ========================= STATE: JAM ============================ */
  case ST_JAM:
    if (digitalRead(SAFETY_SWITCH) == HIGH) {                 // POLARITY
      /* Straight back to RUN -- the jam message was already on screen
       * while machine_state == ST_JAM; once the switch clears, just
       * resume, no operator keypress and no extra screen needed. */
      machine_state          = ST_RUN;
      virtual_crank_angle    = 0;
      pick_incremented_flag  = true;      // don't eat a pick resuming mid-cycle
      last_switch_state      = digitalRead(ENCODER_SWITCH);
      mark_run_state_dirty();
      Serial.println("[SYSTEM] Jam cleared. Resuming RUN.");
      update_outputs(current_pick_index);
    }
    break;

  /* ========================= STATE: RUN ============================ */
  case ST_RUN: {
    char k = keypad_get_key_repeat();      // repeat ON -- A/B can be held to jog while running
    if (k == '#') {
      /* BLOCKING -- loop() never reaches the pick-advance check below
       * while run_menu() is open (see the comment above it). Reset the
       * crank/switch state on return so resuming doesn't eat or double
       * up a pick. */
      run_menu();
      virtual_crank_angle   = 0;
      pick_incremented_flag = true;
      last_degree_ms        = millis();
      last_switch_state     = digitalRead(ENCODER_SWITCH);
      Serial.printf("[SYSTEM] Menu closed -- resuming RUN at pick %d\n",
                    current_pick_index + 1);
      update_outputs(current_pick_index);
      break;
    }
    else if (k == 'B') { jog_pick(-1); break; }   // B = decrement
    else if (k == 'A') { jog_pick(+1); break; }   // A = increment
    else if (k == '0') {
      current_pick_index    = 0;
      virtual_crank_angle   = 0;
      pick_incremented_flag = true;
      mark_run_state_dirty();
      Serial.println("[JOG] jumped to pick 1");
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
        mark_run_state_dirty();
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
        mark_run_state_dirty();
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

  /* One write per loop() pass, at most -- every state change above just
   * sets the flag, so several changes made in the same pass (e.g. a jam
   * clearing straight into a pick jog) still cost only one EEPROM write. */
  if (run_state_dirty) {
    if (runstate_save()) run_state_dirty = false;
    // else: left dirty, retried next loop() pass
  }
}
//git test push chage