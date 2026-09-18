/*
 * main_menu.c - 3x2 tile grid (480x320) + navigation callbacks
 *
 * PORT NOTES
 *  - (void*)i  ->  (void*)(intptr_t)i     (safe int <-> pointer casting)
 *  - LV_SCR_LOAD_ANIM_* -> LV_SCREEN_LOAD_ANIM_*
 *  - create_*() no longer calls lv_screen_load(); only this file loads screens.
 *  - Added go_back_to_settings_event_cb for the settings sub-screens.
 */
#include "ui.h"

/* Back to the main menu */
void go_back_event_cb(lv_event_t * e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    lv_screen_load_anim(main_screen, LV_SCREEN_LOAD_ANIM_MOVE_RIGHT, 300, 0, false);
}

/* Back to the settings menu (used by settings sub-screens) */
void go_back_to_settings_event_cb(lv_event_t * e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    if (!screen_settings) create_settings_screen();
    lv_screen_load_anim(screen_settings, LV_SCREEN_LOAD_ANIM_MOVE_RIGHT, 300, 0, false);
}

/* One handler for all 6 tiles; user_data carries the tile index */
void menu_button_event_cb(lv_event_t * e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;

    int screen_id = (int)(intptr_t)lv_event_get_user_data(e);
    lv_obj_t * target_screen = NULL;

    /* Pattern: build a screen only the FIRST time, then reuse it. */
    switch (screen_id) {
        case 0:
            if (!screen_type) create_type_screen();
            target_screen = screen_type;
            break;
        case 1:
            if (!screen_file_manager) create_file_manager_screen();
            target_screen = screen_file_manager;
            break;
        case 2:
            if (!screen_settings) create_settings_screen();
            target_screen = screen_settings;
            break;
        case 3:
            if (!screen_test_files) create_test_files_screen();
            target_screen = screen_test_files;
            break;
        case 4:
            if (!screen_finger) create_finger_screen();
            target_screen = screen_finger;
            break;
        case 5:
            if (!screen_about) create_about_screen();
            target_screen = screen_about;
            break;
        default:
            LV_LOG_ERROR("Invalid menu selection %d", screen_id);
            break;
    }

    if (target_screen) {
        lv_screen_load_anim(target_screen, LV_SCREEN_LOAD_ANIM_MOVE_LEFT, 300, 0, false);
    }
}

void create_main_menu_screen(void)
{
    main_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(main_screen, lv_color_hex(0xEEEEEE), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(main_screen, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_remove_flag(main_screen, LV_OBJ_FLAG_SCROLLABLE);

    static int32_t col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static int32_t row_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};

    lv_obj_set_layout(main_screen, LV_LAYOUT_GRID);
    lv_obj_set_grid_dsc_array(main_screen, col_dsc, row_dsc);
    lv_obj_set_style_pad_all(main_screen, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_row(main_screen, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_column(main_screen, 10, LV_PART_MAIN);

    static const char * labels[]  = {"Type", "Folder", "Settings", "Test Files", "Lock", "About"};
    static const char * symbols[] = {LV_SYMBOL_LIST, LV_SYMBOL_DIRECTORY, LV_SYMBOL_SETTINGS,
                                     LV_SYMBOL_PLAY, LV_SYMBOL_LOOP, LV_SYMBOL_WARNING};

    for (int i = 0; i < 6; i++) {
        lv_obj_t * tile = lv_button_create(main_screen);
        lv_obj_set_style_radius(tile, 12, 0);
        lv_obj_set_style_bg_color(tile, lv_color_hex(0x001F3F), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(tile, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(tile, lv_color_hex(0x003366), 0);
        lv_obj_set_style_border_width(tile, 2, 0);
        lv_obj_set_style_pad_all(tile, 8, 0);

        lv_obj_set_grid_cell(tile,
                             LV_GRID_ALIGN_STRETCH, i % 3, 1,
                             LV_GRID_ALIGN_STRETCH, i / 3, 1);

        lv_obj_add_event_cb(tile, menu_button_event_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);

        lv_obj_t * icon_label = lv_label_create(tile);
        lv_label_set_text(icon_label, symbols[i]);
        lv_obj_set_style_text_color(icon_label, lv_color_white(), LV_PART_MAIN);
        lv_obj_set_style_text_font(icon_label, &lv_font_montserrat_48, 0);
        lv_obj_align(icon_label, LV_ALIGN_CENTER, 0, -15);

        lv_obj_t * text_label = lv_label_create(tile);
        lv_label_set_text(text_label, labels[i]);
        lv_obj_set_style_text_color(text_label, lv_color_white(), LV_PART_MAIN);
        lv_obj_set_style_text_font(text_label, &lv_font_montserrat_16, 0);
        lv_obj_align_to(text_label, icon_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 6);
    }
}
