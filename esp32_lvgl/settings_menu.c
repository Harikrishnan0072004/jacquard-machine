/*
 * settings_menu.c - Password, Card count, Invert design, Test mode, Lock, About
 *
 * FIXES vs simulator version
 *  1. MEMORY LEAK: OK/Cancel called create_settings_screen() again every time
 *     -> a new settings screen per press. Now we just LOAD the existing one.
 *  2. WRONG CALLBACK TYPE: create_settings_screen (void f(void)) was passed as
 *     an event callback (needs void f(lv_event_t*)). Replaced.
 *  3. SHADOWED GLOBALS: create_about_screen() and create_test_files_screen()
 *     used LOCAL variables, so the globals stayed NULL and a new screen was
 *     built on every visit. Now the globals are assigned.
 *  4. Keypad screens are DELETED when you leave them (auto_del = true) and the
 *     global pointer is reset in an LV_EVENT_DELETE handler. They rebuild fresh
 *     (empty text field) next time, and use no RAM while hidden.
 *  5. Test-mode rows: child objects made non-clickable so taps reach the row.
 *  6. lv_btnmatrix_* -> lv_buttonmatrix_*, clear_flag -> remove_flag.
 */
#include "ui.h"
#include <stdio.h>
#include <string.h>

#define PRODUCT_ABOUT "A Jacquard machine is a device that automates the creation of complex, \
intricate patterns in woven fabric by precisely controlling individual \
warp threads.\n\nOriginally controlled by a series of punched cards, modern \
versions use electronic controls to allow for detailed designs like brocade, \
damask, and other ornamental patterns."

/* =====================================================================
 *  Helpers shared by the two keypad screens
 * ===================================================================== */

/* When a screen object is deleted, set the global pointer that referred to it
 * back to NULL. user_data = address of that global pointer. */
static void screen_deleted_cb(lv_event_t * e)
{
    lv_obj_t ** owner = (lv_obj_t **)lv_event_get_user_data(e);
    if (owner) *owner = NULL;
}

/* Leave a keypad screen and free it after the animation */
static void leave_keypad_screen(void)
{
    if (!screen_settings) create_settings_screen();
    lv_screen_load_anim(screen_settings, LV_SCREEN_LOAD_ANIM_MOVE_RIGHT, 300, 0, true);
}

static void keypad_back_btn_cb(lv_event_t * e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    leave_keypad_screen();
}

static const char * num_map[] = {
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

/* Builds the common layout: title, text area, numeric keyboard, back button.
 * Returns the text area; keyboard is returned through kb_out. */
static lv_obj_t * build_keypad_screen(lv_obj_t * scr, const char * title_txt,
                                      const char * placeholder, uint32_t max_len,
                                      bool password, lv_event_cb_t kb_cb)
{
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x001F3F), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t * title = lv_label_create(scr);
    lv_label_set_text(title, title_txt);
    lv_obj_set_style_text_color(title, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 18);

    lv_obj_t * ta = lv_textarea_create(scr);
    lv_obj_set_width(ta, lv_pct(90));
    lv_obj_align(ta, LV_ALIGN_TOP_MID, 0, 60);
    lv_textarea_set_placeholder_text(ta, placeholder);
    lv_textarea_set_one_line(ta, true);
    lv_textarea_set_max_length(ta, max_len);
    lv_textarea_set_password_mode(ta, password);
    lv_textarea_set_accepted_chars(ta, "0123456789");

    lv_obj_t * kb = lv_keyboard_create(scr);
    lv_obj_set_size(kb, lv_pct(100), lv_pct(55));
    lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_keyboard_set_map(kb, LV_KEYBOARD_MODE_USER_1, num_map, num_ctrl_map);
    lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_USER_1);
    lv_keyboard_set_textarea(kb, ta);
    lv_obj_add_event_cb(kb, kb_cb, LV_EVENT_VALUE_CHANGED, ta);  /* ta passed as user_data */

    lv_obj_t * back_btn = lv_button_create(scr);
    lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_add_event_cb(back_btn, keypad_back_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t * back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT " Back");
    lv_obj_center(back_label);

    return ta;
}

/* Returns the text of the key that was just pressed, or NULL */
static const char * pressed_key_text(lv_event_t * e)
{
    lv_obj_t * kb = (lv_obj_t *)lv_event_get_current_target(e);
    uint32_t id = lv_buttonmatrix_get_selected_button(kb);
    if (id == LV_BUTTONMATRIX_BUTTON_NONE) return NULL;
    return lv_buttonmatrix_get_button_text(kb, id);
}

/* =====================================================================
 *  New password screen
 * ===================================================================== */
static void password_kb_event_cb(lv_event_t * e)
{
    const char * txt = pressed_key_text(e);
    if (txt == NULL) return;

    lv_obj_t * ta = (lv_obj_t *)lv_event_get_user_data(e);

    if (strcmp(txt, LV_SYMBOL_OK) == 0) {
        LV_LOG_USER("New password entered (%d digits)", (int)strlen(lv_textarea_get_text(ta)));
        /* TODO: store in NVS (Preferences library) */
        leave_keypad_screen();
    } else if (strcmp(txt, LV_SYMBOL_CLOSE) == 0) {
        leave_keypad_screen();
    }
}

void create_settings_password_screen(void)
{
    screen_settings_password = lv_obj_create(NULL);
    lv_obj_add_event_cb(screen_settings_password, screen_deleted_cb,
                        LV_EVENT_DELETE, &screen_settings_password);

    build_keypad_screen(screen_settings_password, "Enter New Password",
                        "Enter only digits...", 8, true, password_kb_event_cb);
}

/* =====================================================================
 *  Card count screen
 * ===================================================================== */
static void card_count_kb_event_cb(lv_event_t * e)
{
    const char * txt = pressed_key_text(e);
    if (txt == NULL) return;

    lv_obj_t * ta = (lv_obj_t *)lv_event_get_user_data(e);

    if (strcmp(txt, LV_SYMBOL_OK) == 0) {
        LV_LOG_USER("Card count set to: %s", lv_textarea_get_text(ta));
        /* TODO: save the value (NVS / config struct) */
        leave_keypad_screen();
    } else if (strcmp(txt, LV_SYMBOL_CLOSE) == 0) {
        leave_keypad_screen();
    }
}

void create_settings_card_count_screen(void)
{
    screen_settings_card_count = lv_obj_create(NULL);
    lv_obj_add_event_cb(screen_settings_card_count, screen_deleted_cb,
                        LV_EVENT_DELETE, &screen_settings_card_count);

    build_keypad_screen(screen_settings_card_count, "Set Number of Cards",
                        "Enter count (digits only)...", 4, false, card_count_kb_event_cb);
}

/* =====================================================================
 *  Invert design (screen kept for later; the settings row uses a switch)
 * ===================================================================== */
void create_settings_invert_design_screen(void)
{
    screen_settings_invert_design = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen_settings_invert_design, lv_color_hex(0x001F3F), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen_settings_invert_design, LV_OPA_COVER, LV_PART_MAIN);

    lv_obj_t * title = lv_label_create(screen_settings_invert_design);
    lv_label_set_text(title, "Invert Design (On/Off)");
    lv_obj_set_style_text_color(title, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

    lv_obj_t * switch_label = lv_label_create(screen_settings_invert_design);
    lv_label_set_text(switch_label, "Invert Design");
    lv_obj_set_style_text_color(switch_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(switch_label, LV_ALIGN_LEFT_MID, 20, 0);

    lv_obj_t * sw = lv_switch_create(screen_settings_invert_design);
    lv_obj_align(sw, LV_ALIGN_RIGHT_MID, -20, 0);
    lv_obj_add_event_cb(sw, invert_design_switch_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_t * back_btn = lv_button_create(screen_settings_invert_design);
    lv_obj_add_event_cb(back_btn, go_back_to_settings_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_t * back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT " Back");
}

void invert_design_switch_event_cb(lv_event_t * e)
{
    lv_obj_t * sw = (lv_obj_t *)lv_event_get_target(e);
    bool on = lv_obj_has_state(sw, LV_STATE_CHECKED);

    if (screen_settings) {
        lv_obj_set_style_bg_color(screen_settings,
                                  lv_color_hex(on ? 0x000000 : 0x001F3F), LV_PART_MAIN);
    }
    LV_LOG_USER("Invert design: %s", on ? "ON" : "OFF");
    /* TODO: persist state */
}

/* =====================================================================
 *  TEST MODE screen (menu tile "Test Files")
 * ===================================================================== */
#define TM_ALL_DOWN 0
#define TM_ALL_UP   1
#define TM_PLAIN    2

static lv_obj_t * selected_test_row = NULL;
static int selected_test_id = -1;

static void testmode_set_selected(lv_obj_t * row)
{
    if (!row) return;

    if (selected_test_row && selected_test_row != row) {
        lv_obj_t * old_radio = lv_obj_get_child(selected_test_row, 0);
        lv_obj_t * old_dot   = lv_obj_get_child(old_radio, 0);
        lv_obj_add_flag(old_dot, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_bg_color(selected_test_row, lv_color_hex(0x004A9F), 0);
    }

    lv_obj_t * radio = lv_obj_get_child(row, 0);
    lv_obj_t * dot   = lv_obj_get_child(radio, 0);
    lv_obj_remove_flag(dot, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_bg_color(row, lv_color_hex(0x0077D7), 0);

    selected_test_row = row;
}

static void testmode_radio_event_cb(lv_event_t * e)
{
    /* current_target = the object this callback was attached to (the row) */
    lv_obj_t * row = (lv_obj_t *)lv_event_get_current_target(e);
    selected_test_id = (int)(intptr_t)lv_event_get_user_data(e);
    testmode_set_selected(row);
    printf("[TestMode] Selected = %d\n", selected_test_id);
}

void create_test_files_screen(void)
{
    screen_test_files = lv_obj_create(NULL);           /* FIX: assign the global */
    lv_obj_remove_style_all(screen_test_files);
    lv_obj_set_style_bg_color(screen_test_files, lv_color_hex(0x001F3F), 0);
    lv_obj_set_style_bg_opa(screen_test_files, LV_OPA_COVER, 0);

    lv_obj_t * title = lv_label_create(screen_test_files);
    lv_label_set_text(title, "Test Mode");
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 16);

    lv_obj_t * cont = lv_obj_create(screen_test_files);
    lv_obj_remove_style_all(cont);
    lv_obj_set_size(cont, lv_pct(90), lv_pct(70));
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(cont, 14, 0);
    lv_obj_align(cont, LV_ALIGN_CENTER, 0, 20);
    lv_obj_set_scroll_dir(cont, LV_DIR_NONE);

    struct { const char * label; uint8_t id; } tm_items[] = {
        {"All Down", TM_ALL_DOWN},
        {"All Up",   TM_ALL_UP},
        {"Plain",    TM_PLAIN},
    };

    for (int i = 0; i < 3; i++) {
        lv_obj_t * row = lv_button_create(cont);
        lv_obj_remove_style_all(row);
        lv_obj_set_width(row, lv_pct(100));
        lv_obj_set_height(row, 48);
        lv_obj_set_style_bg_color(row, lv_color_hex(0x004A9F), 0);
        lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(row, 12, 0);
        lv_obj_set_style_pad_all(row, 10, 0);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        lv_obj_t * radio = lv_obj_create(row);
        lv_obj_remove_style_all(radio);
        lv_obj_remove_flag(radio, LV_OBJ_FLAG_CLICKABLE);   /* FIX: let taps reach row */
        lv_obj_set_size(radio, 22, 22);
        lv_obj_set_style_radius(radio, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(radio, lv_color_white(), 0);
        lv_obj_set_style_bg_opa(radio, LV_OPA_70, 0);

        lv_obj_t * dot = lv_obj_create(radio);
        lv_obj_remove_style_all(dot);
        lv_obj_remove_flag(dot, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_size(dot, 12, 12);
        lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(dot, lv_color_hex(0xFFD700), 0);
        lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
        lv_obj_center(dot);
        lv_obj_add_flag(dot, LV_OBJ_FLAG_HIDDEN);

        lv_obj_t * lbl = lv_label_create(row);
        lv_label_set_text(lbl, tm_items[i].label);
        lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_20, 0);
        lv_obj_set_style_pad_left(lbl, 10, 0);

        lv_obj_add_event_cb(row, testmode_radio_event_cb, LV_EVENT_CLICKED,
                            (void *)(intptr_t)tm_items[i].id);

        if (selected_test_id == tm_items[i].id) {
            testmode_set_selected(row);
        }
    }

    /* Back */
    lv_obj_t * back_btn = lv_button_create(screen_test_files);
    lv_obj_remove_style_all(back_btn);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(0x003366), 0);
    lv_obj_set_style_bg_opa(back_btn, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(back_btn, 10, 0);
    lv_obj_set_style_pad_all(back_btn, 8, 0);
    lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_add_event_cb(back_btn, go_back_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t * back_lbl = lv_label_create(back_btn);
    lv_label_set_text(back_lbl, LV_SYMBOL_LEFT " Back");
    lv_obj_set_style_text_color(back_lbl, lv_color_white(), 0);
    lv_obj_center(back_lbl);

    /* OK */
    lv_obj_t * ok_btn = lv_button_create(screen_test_files);
    lv_obj_remove_style_all(ok_btn);
    lv_obj_set_style_bg_color(ok_btn, lv_color_hex(0x009900), 0);
    lv_obj_set_style_bg_opa(ok_btn, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(ok_btn, 10, 0);
    lv_obj_set_style_pad_all(ok_btn, 8, 0);
    lv_obj_align(ok_btn, LV_ALIGN_TOP_RIGHT, -10, 10);
    lv_obj_add_event_cb(ok_btn, go_back_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t * ok_lbl = lv_label_create(ok_btn);
    lv_label_set_text(ok_lbl, "OK " LV_SYMBOL_OK);
    lv_obj_set_style_text_color(ok_lbl, lv_color_white(), 0);
    lv_obj_center(ok_lbl);
}

/* =====================================================================
 *  LOCK screen (menu tile "Lock")
 * ===================================================================== */
void create_finger_screen(void)
{
    screen_finger = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen_finger, lv_color_hex(0x001F3F), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen_finger, LV_OPA_COVER, LV_PART_MAIN);

    lv_obj_t * title = lv_label_create(screen_finger);
    lv_label_set_text(title, "Lock");
    lv_obj_set_style_text_color(title, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

    lv_obj_t * back_btn = lv_button_create(screen_finger);
    lv_obj_add_event_cb(back_btn, go_back_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_t * back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT " Back");
}

/* =====================================================================
 *  ABOUT screen
 * ===================================================================== */
void create_about_screen(void)
{
    screen_about = lv_obj_create(NULL);                 /* FIX: was a local variable */
    lv_obj_set_style_bg_color(screen_about, lv_color_hex(0x001F3F), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen_about, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(screen_about, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t * title = lv_label_create(screen_about);
    lv_label_set_text(title, "About");
    lv_obj_set_style_text_color(title, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 15);

    lv_obj_t * info_label = lv_label_create(screen_about);
    lv_label_set_text(info_label, PRODUCT_ABOUT);
    lv_label_set_long_mode(info_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(info_label, lv_pct(90));
    lv_obj_align_to(info_label, title, LV_ALIGN_OUT_BOTTOM_MID, 0, 15);
    lv_obj_set_style_text_color(info_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(info_label, &lv_font_montserrat_18, 0);

    lv_obj_t * back_btn = lv_button_create(screen_about);
    lv_obj_add_event_cb(back_btn, go_back_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_t * back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT " Back");
}
