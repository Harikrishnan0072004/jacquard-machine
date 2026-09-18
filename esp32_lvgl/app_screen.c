/*
 * app_screen.c - owns all global screen pointers + app entry point
 *
 * PORT NOTES
 *  - Removed Linux-only headers (unistd.h, pwd.h, dirent.h).
 *  - Each global is DEFINED exactly once here (with = NULL).
 *    Other files only DECLARE them through ui.h (extern).
 */
#include "ui.h"

lv_obj_t * main_screen                   = NULL;

lv_obj_t * screen_type                   = NULL;
lv_obj_t * screen_file_manager           = NULL;
lv_obj_t * screen_settings               = NULL;
lv_obj_t * screen_test_files             = NULL;
lv_obj_t * screen_finger                 = NULL;
lv_obj_t * screen_about                  = NULL;

lv_obj_t * screen_settings_password      = NULL;
lv_obj_t * screen_settings_card_count    = NULL;
lv_obj_t * screen_settings_invert_design = NULL;
lv_obj_t * screen_settings_front_back    = NULL;

void my_app_init(void)
{
    create_boot_screen();
}
