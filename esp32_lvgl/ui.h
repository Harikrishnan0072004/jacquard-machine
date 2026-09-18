/*
 * ui.h - shared declarations for the Jacquard UI
 *
 * WHY THIS FILE EXISTS
 *  - Before: every .c file had its own copy of "extern ..." lines, and some
 *    functions were called without any declaration (error on new GCC).
 *  - Now: ONE place declares everything. Every .c file includes "ui.h".
 *  - The extern "C" block lets the .ino (compiled as C++) call these C functions.
 */
#ifndef UI_H
#define UI_H

#include "lvgl.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---------- Screen pointers (defined once in app_screen.c) ---------- */
extern lv_obj_t * main_screen;

extern lv_obj_t * screen_type;
extern lv_obj_t * screen_file_manager;
extern lv_obj_t * screen_settings;
extern lv_obj_t * screen_test_files;
extern lv_obj_t * screen_finger;
extern lv_obj_t * screen_about;

extern lv_obj_t * screen_settings_password;
extern lv_obj_t * screen_settings_card_count;
extern lv_obj_t * screen_settings_invert_design;
extern lv_obj_t * screen_settings_front_back;

/* ---------- Entry point (called from the .ino) ---------- */
void my_app_init(void);

/* ---------- Screen builders ----------
 * Rule used everywhere: a create_*() function only BUILDS the screen.
 * The caller decides when and how to load it (with animation). */
void create_boot_screen(void);
void create_main_menu_screen(void);

void create_type_screen(void);
void create_file_manager_screen(void);
void create_settings_screen(void);
void create_test_files_screen(void);
void create_finger_screen(void);
void create_about_screen(void);

void create_settings_password_screen(void);
void create_settings_card_count_screen(void);
void create_settings_invert_design_screen(void);
void create_settings_front_back_screen(void);

/* ---------- Shared event callbacks ---------- */
void go_back_event_cb(lv_event_t * e);              /* -> main menu     */
void go_back_to_settings_event_cb(lv_event_t * e);  /* -> settings menu */
void menu_button_event_cb(lv_event_t * e);
void settings_menu_event_cb(lv_event_t * e);
void invert_design_switch_event_cb(lv_event_t * e);

#ifdef __cplusplus
}
#endif

#endif /* UI_H */
