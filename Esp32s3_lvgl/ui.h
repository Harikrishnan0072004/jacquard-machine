#pragma once
/* Shared header: replaces the six copies of the extern list. */
#include <lvgl.h>
#ifdef __cplusplus
extern "C" {
#endif

#define UI_HOR_RES      480
#define UI_VER_RES      320
#define UI_NAV_ANIM_MS  300   /* screen slide time; set 0 to switch instantly if it looks choppy */

/* ---- screens (defined once in app_screen.c) ---- */
extern lv_obj_t *main_screen;
extern lv_obj_t *screen_type, *screen_file_manager, *screen_settings;
extern lv_obj_t *screen_test_files, *screen_finger, *screen_about;
extern lv_obj_t *screen_settings_password, *screen_settings_card_count;
extern lv_obj_t *screen_settings_invert_design, *screen_settings_front_back;

/* ---- app_screen.c : entry point + shared helpers ---- */
void my_app_init(void);
void ui_goto(lv_obj_t *scr, bool forward);          /* slide to a screen (forward = left, back = right) */
void ui_radio_select(lv_obj_t *cont, int idx);      /* check row idx, uncheck the others */
lv_obj_t *ui_radio_row_create(lv_obj_t *cont, const char *text, int idx,
                              int32_t row_w, int32_t row_h, lv_event_cb_t cb);

/* ---- one create_* per screen (they only BUILD the screen, they never load it) ---- */
void create_boot_screen(void);            /* boot_screen.c    */
void create_main_menu_screen(void);       /* main_menu.c      */
void create_type_screen(void);            /* settings_type.c  */
void create_file_manager_screen(void);    /* file_manager.c   */
void create_settings_screen(void);        /* settings.c       */
void create_settings_front_back_screen(void);   /* front_back.c */
void create_settings_password_screen(void);     /* settings_menu.c */
void create_settings_card_count_screen(void);
void create_settings_invert_design_screen(void);
void create_test_files_screen(void);
void create_finger_screen(void);
void create_about_screen(void);

/* ---- callbacks shared between files ---- */
void go_back_event_cb(lv_event_t *e);            /* main_menu.c : back to main menu */
void go_settings_event_cb(lv_event_t *e);        /* settings.c  : back to settings  */
void invert_design_switch_event_cb(lv_event_t *e);   /* settings_menu.c */

/* ---- read-only hooks for your machine code (call from the same task as lv_timer_handler) ---- */
int ui_get_type_mode(void);     /* 0 = Single, 1 = Dual                          */
int ui_get_front_back(void);    /* 0 FrontLeft, 1 FrontRight, 2 BackLeft, 3 BackRight */
int ui_get_test_mode(void);     /* 0 All Down, 1 All Up, 2 Plain                 */
int ui_get_card_count(void);

#ifdef __cplusplus
}
#endif
