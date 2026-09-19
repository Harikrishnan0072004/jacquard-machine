#include "ui.h"
#include <stdlib.h>
#include <string.h>

/* About screen text */
#define PRODUCT_ABOUT "A Jacquard machine is a device that automates the creation of complex, \
intricate patterns in woven fabric by precisely controlling individual \
warp threads. \n\nOriginally controlled by a series of punched cards, modern \
versions use electronic controls to allow for detailed designs like brocade, \
damask, and other ornamental patterns"

/* Numeric keypad layout shared by Password and Card Count.
   Keys: 1-9, 0, backspace, OK (accept), X (cancel). */
static const char *num_map[] = {
    "1", "2", "3", LV_SYMBOL_BACKSPACE, "\n",
    "4", "5", "6", LV_SYMBOL_OK, "\n",
    "7", "8", "9", LV_SYMBOL_CLOSE, "\n",
    "0", ""
};
static const lv_buttonmatrix_ctrl_t num_ctrl_map[] = {
    LV_BUTTONMATRIX_CTRL_CLICK_TRIG, LV_BUTTONMATRIX_CTRL_CLICK_TRIG, LV_BUTTONMATRIX_CTRL_CLICK_TRIG, LV_BUTTONMATRIX_CTRL_CLICK_TRIG,
    LV_BUTTONMATRIX_CTRL_CLICK_TRIG, LV_BUTTONMATRIX_CTRL_CLICK_TRIG, LV_BUTTONMATRIX_CTRL_CLICK_TRIG, LV_BUTTONMATRIX_CTRL_CLICK_TRIG,
    LV_BUTTONMATRIX_CTRL_CLICK_TRIG, LV_BUTTONMATRIX_CTRL_CLICK_TRIG, LV_BUTTONMATRIX_CTRL_CLICK_TRIG, LV_BUTTONMATRIX_CTRL_CLICK_TRIG,
    LV_BUTTONMATRIX_CTRL_CLICK_TRIG
};

/* Text of the key that was just pressed (NULL if none) */
static const char *pressed_key(lv_event_t *e)
{
    lv_obj_t *kb = lv_event_get_target(e);
    return lv_buttonmatrix_get_button_text(kb, lv_buttonmatrix_get_selected_button(kb));
}

/* ======================= PASSWORD ======================= */
static lv_obj_t *password_ta;

static void password_kb_event_cb(lv_event_t *e)
{
    const char *txt = pressed_key(e);
    if (txt == NULL) return;

    if (strcmp(txt, LV_SYMBOL_OK) == 0) {
        /* Do not log the password itself. */
        LV_LOG_USER("New password entered, %d digits", (int)strlen(lv_textarea_get_text(password_ta)));
        /* TODO: hand lv_textarea_get_text(password_ta) to your storage code */
        lv_textarea_set_text(password_ta, "");
        ui_goto(screen_settings, false);
    } else if (strcmp(txt, LV_SYMBOL_CLOSE) == 0) {
        lv_textarea_set_text(password_ta, "");
        ui_goto(screen_settings, false);
    }
}

void create_settings_password_screen(void)
{
    screen_settings_password = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen_settings_password, lv_color_hex(0x001F3F), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen_settings_password, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_remove_flag(screen_settings_password, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(screen_settings_password);
    lv_label_set_text(title, "Enter New Password");
    lv_obj_set_style_text_color(title, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    password_ta = lv_textarea_create(screen_settings_password);
    lv_obj_set_width(password_ta, lv_pct(90));
    lv_obj_align(password_ta, LV_ALIGN_TOP_MID, 0, 50);
    lv_textarea_set_placeholder_text(password_ta, "Enter only digits...");
    lv_textarea_set_max_length(password_ta, 8);
    lv_textarea_set_one_line(password_ta, true);
    lv_textarea_set_password_mode(password_ta, true);
    lv_textarea_set_accepted_chars(password_ta, "0123456789");

    lv_obj_t *kb = lv_keyboard_create(screen_settings_password);
    lv_obj_set_size(kb, lv_pct(100), lv_pct(50));
    lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_USER_1);
    lv_keyboard_set_map(kb, LV_KEYBOARD_MODE_USER_1, num_map, num_ctrl_map);
    lv_keyboard_set_textarea(kb, password_ta);
    lv_obj_add_event_cb(kb, password_kb_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_t *back_btn = lv_button_create(screen_settings_password);
    lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_add_event_cb(back_btn, go_settings_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *back_lbl = lv_label_create(back_btn);
    lv_label_set_text(back_lbl, LV_SYMBOL_LEFT " Back");
    lv_obj_center(back_lbl);
}

/* ======================= CARD COUNT ======================= */
static lv_obj_t *card_count_ta;
static int saved_card_count = 0;

int ui_get_card_count(void) { return saved_card_count; }

static void card_count_kb_event_cb(lv_event_t *e)
{
    const char *txt = pressed_key(e);
    if (txt == NULL) return;

    if (strcmp(txt, LV_SYMBOL_OK) == 0) {
        saved_card_count = atoi(lv_textarea_get_text(card_count_ta));
        LV_LOG_USER("Card count set to %d", saved_card_count);
        /* TODO: save to NVS/config and tell the machine code */
        lv_textarea_set_text(card_count_ta, "");
        ui_goto(screen_settings, false);
    } else if (strcmp(txt, LV_SYMBOL_CLOSE) == 0) {
        lv_textarea_set_text(card_count_ta, "");
        ui_goto(screen_settings, false);
    }
}

void create_settings_card_count_screen(void)
{
    screen_settings_card_count = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen_settings_card_count, lv_color_hex(0x001F3F), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen_settings_card_count, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_remove_flag(screen_settings_card_count, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(screen_settings_card_count);
    lv_label_set_text(title, "Set Number of Cards");
    lv_obj_set_style_text_color(title, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    card_count_ta = lv_textarea_create(screen_settings_card_count);
    lv_obj_set_width(card_count_ta, lv_pct(90));
    lv_obj_align(card_count_ta, LV_ALIGN_TOP_MID, 0, 50);
    lv_textarea_set_placeholder_text(card_count_ta, "Enter count (digits only)...");
    lv_textarea_set_one_line(card_count_ta, true);
    lv_textarea_set_accepted_chars(card_count_ta, "0123456789");
    lv_textarea_set_max_length(card_count_ta, 4);

    lv_obj_t *kb = lv_keyboard_create(screen_settings_card_count);
    lv_obj_set_size(kb, lv_pct(100), lv_pct(50));
    lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_USER_1);
    lv_keyboard_set_map(kb, LV_KEYBOARD_MODE_USER_1, num_map, num_ctrl_map);
    lv_keyboard_set_textarea(kb, card_count_ta);
    lv_obj_add_event_cb(kb, card_count_kb_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_t *back_btn = lv_button_create(screen_settings_card_count);
    lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_add_event_cb(back_btn, go_settings_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *back_lbl = lv_label_create(back_btn);
    lv_label_set_text(back_lbl, LV_SYMBOL_LEFT " Back");
    lv_obj_center(back_lbl);
}

/* ======================= INVERT DESIGN ======================= */
/* In the Settings list this is a switch on its own row. The separate screen below is kept
   (your original) but is not reachable from the menu. */
void invert_design_switch_event_cb(lv_event_t *e)
{
    lv_obj_t *sw = lv_event_get_target(e);
    bool on = lv_obj_has_state(sw, LV_STATE_CHECKED);

    if (screen_settings) {
        lv_obj_set_style_bg_color(screen_settings,
                                  lv_color_hex(on ? 0x000000 : 0x001F3F), LV_PART_MAIN);
    }
    /* TODO: persist the state / tell the machine code */
}

void create_settings_invert_design_screen(void)
{
    screen_settings_invert_design = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen_settings_invert_design, lv_color_hex(0x001F3F), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen_settings_invert_design, LV_OPA_COVER, LV_PART_MAIN);

    lv_obj_t *title = lv_label_create(screen_settings_invert_design);
    lv_label_set_text(title, "Invert Design (On/Off)");
    lv_obj_set_style_text_color(title, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

    lv_obj_t *sw_label = lv_label_create(screen_settings_invert_design);
    lv_label_set_text(sw_label, "Invert Design");
    lv_obj_set_style_text_color(sw_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(sw_label, LV_ALIGN_LEFT_MID, 20, 0);

    lv_obj_t *sw = lv_switch_create(screen_settings_invert_design);
    lv_obj_align(sw, LV_ALIGN_RIGHT_MID, -20, 0);
    lv_obj_add_event_cb(sw, invert_design_switch_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_t *back_btn = lv_button_create(screen_settings_invert_design);
    lv_obj_add_event_cb(back_btn, go_settings_event_cb, LV_EVENT_CLICKED, NULL);   /* was create_settings_screen: wrong signature */
    lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_t *back_lbl = lv_label_create(back_btn);
    lv_label_set_text(back_lbl, LV_SYMBOL_LEFT "Back");
}

/* ======================= TEST MODE ======================= */
#define TM_ALL_DOWN 0
#define TM_ALL_UP   1
#define TM_PLAIN    2

static int active_testmode_idx = 0;   /* highlighted now */
static int saved_testmode_idx  = 0;   /* committed by OK */
static lv_obj_t *test_radio_cont;

int ui_get_test_mode(void) { return saved_testmode_idx; }

static void testmode_radio_event_cb(lv_event_t *e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    ui_radio_select(test_radio_cont, idx);
    active_testmode_idx = idx;
}

static void testmode_back_event_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    active_testmode_idx = saved_testmode_idx;               /* discard an unsaved choice */
    ui_radio_select(test_radio_cont, saved_testmode_idx);
    ui_goto(main_screen, false);
}

static void testmode_ok_event_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    saved_testmode_idx = active_testmode_idx;
    LV_LOG_USER("TestMode saved = %d", saved_testmode_idx);
    /* TODO: save to NVS and tell the machine code */
    ui_goto(main_screen, false);
}

void create_test_files_screen(void)
{
    screen_test_files = lv_obj_create(NULL);      /* was a local variable: global stayed NULL and it leaked */
    lv_obj_set_style_bg_color(screen_test_files, lv_color_hex(0x001F3F), 0);
    lv_obj_set_style_bg_opa(screen_test_files, LV_OPA_COVER, 0);
    lv_obj_remove_flag(screen_test_files, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(screen_test_files);
    lv_label_set_text(title, "Test Mode");
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 16);

    test_radio_cont = lv_obj_create(screen_test_files);
    lv_obj_set_size(test_radio_cont, lv_pct(90), lv_pct(60));
    lv_obj_align_to(test_radio_cont, title, LV_ALIGN_OUT_BOTTOM_MID, 0, 20);
    lv_obj_set_style_bg_opa(test_radio_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(test_radio_cont, 0, 0);
    lv_obj_set_flex_flow(test_radio_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(test_radio_cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(test_radio_cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(test_radio_cont, LV_SCROLLBAR_MODE_OFF);

    const char *options[] = {"All Down", "All Up", "Plain"};
    for (int i = 0; i < 3; i++) {
        ui_radio_row_create(test_radio_cont, options[i], i, lv_pct(90), 40, testmode_radio_event_cb);
    }
    active_testmode_idx = saved_testmode_idx;
    ui_radio_select(test_radio_cont, saved_testmode_idx);

    lv_obj_t *back_btn = lv_button_create(screen_test_files);
    lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_add_event_cb(back_btn, testmode_back_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *back_lbl = lv_label_create(back_btn);
    lv_label_set_text(back_lbl, LV_SYMBOL_LEFT "Back");
    lv_obj_center(back_lbl);

    lv_obj_t *ok_btn = lv_button_create(screen_test_files);
    lv_obj_align(ok_btn, LV_ALIGN_TOP_RIGHT, -10, 10);
    lv_obj_add_event_cb(ok_btn, testmode_ok_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *ok_lbl = lv_label_create(ok_btn);
    lv_label_set_text(ok_lbl, "OK");
    lv_obj_center(ok_lbl);
}

/* ======================= LOCK (was "finger") ======================= */
void create_finger_screen(void)
{
    screen_finger = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen_finger, lv_color_hex(0x001F3F), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen_finger, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_remove_flag(screen_finger, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(screen_finger);
    lv_label_set_text(title, "Lock");
    lv_obj_set_style_text_color(title, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

    lv_obj_t *back_btn = lv_button_create(screen_finger);
    lv_obj_add_event_cb(back_btn, go_back_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_t *back_lbl = lv_label_create(back_btn);
    lv_label_set_text(back_lbl, LV_SYMBOL_LEFT "Back");
}

/* ======================= ABOUT ======================= */
void create_about_screen(void)
{
    screen_about = lv_obj_create(NULL);           /* was "lv_obj_t *screen_about" (shadowed the global) */
    lv_obj_set_style_bg_color(screen_about, lv_color_hex(0x001F3F), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen_about, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(screen_about, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t *title = lv_label_create(screen_about);
    lv_label_set_text(title, "About");
    lv_obj_set_style_text_color(title, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 15);

    lv_obj_t *info = lv_label_create(screen_about);
    lv_label_set_text(info, PRODUCT_ABOUT);
    lv_label_set_long_mode(info, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(info, lv_pct(90));
    lv_obj_align_to(info, title, LV_ALIGN_OUT_BOTTOM_MID, 0, 15);
    lv_obj_set_style_text_color(info, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(info, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_align(info, LV_TEXT_ALIGN_AUTO, LV_PART_MAIN);

    lv_obj_t *back_btn = lv_button_create(screen_about);
    lv_obj_add_event_cb(back_btn, go_back_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_t *back_lbl = lv_label_create(back_btn);
    lv_label_set_text(back_lbl, LV_SYMBOL_LEFT "Back");
}
