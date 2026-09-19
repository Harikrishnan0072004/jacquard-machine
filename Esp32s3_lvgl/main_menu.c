#include "ui.h"

/* Back to the main menu (used by Type, Folder, Test, Lock, About) */
void go_back_event_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    ui_goto(main_screen, false);
}

/* One handler for all 6 buttons. user data = button id, cast through intptr_t. */
static void menu_button_event_cb(lv_event_t *e)
{
    intptr_t id = (intptr_t)lv_event_get_user_data(e);
    lv_obj_t *target = NULL;

    switch (id) {
        case 0:  if (!screen_type)         create_type_screen();         target = screen_type;         break;
        case 1:  if (!screen_file_manager) create_file_manager_screen(); target = screen_file_manager; break;
        case 2:  if (!screen_settings)     create_settings_screen();     target = screen_settings;     break;
        case 3:  if (!screen_test_files)   create_test_files_screen();   target = screen_test_files;   break;
        case 4:  if (!screen_finger)       create_finger_screen();       target = screen_finger;       break;
        case 5:  if (!screen_about)        create_about_screen();        target = screen_about;        break;
        default: LV_LOG_ERROR("Invalid menu id");                        break;
    }
    ui_goto(target, true);
}

/* Main menu: 3 columns x 2 rows grid (480x320) */
void create_main_menu_screen(void)
{
    static const int32_t col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static const int32_t row_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};

    main_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(main_screen, lv_color_hex(0xEEEEEE), LV_PART_MAIN);
    lv_obj_remove_flag(main_screen, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_layout(main_screen, LV_LAYOUT_GRID);
    lv_obj_set_grid_dsc_array(main_screen, col_dsc, row_dsc);
    lv_obj_set_style_pad_all(main_screen, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_column(main_screen, 8, LV_PART_MAIN);   /* gap between columns */
    lv_obj_set_style_pad_row(main_screen, 8, LV_PART_MAIN);      /* gap between rows    */

    const char *labels[]  = {"Type", "Folder", "Settings", "Test Files", "Lock", "About"};
    const char *symbols[] = {LV_SYMBOL_LIST, LV_SYMBOL_DIRECTORY, LV_SYMBOL_SETTINGS,
                             LV_SYMBOL_PLAY, LV_SYMBOL_LOOP, LV_SYMBOL_WARNING};

    for (int i = 0; i < 6; i++) {
        lv_obj_t *btn = lv_button_create(main_screen);
        lv_obj_set_style_radius(btn, 12, 0);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x001F3F), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(btn, lv_color_hex(0x003366), 0);
        lv_obj_set_style_border_width(btn, 2, 0);
        lv_obj_set_style_pad_all(btn, 8, 0);

        lv_obj_set_grid_cell(btn,
                             LV_GRID_ALIGN_STRETCH, i % 3, 1,
                             LV_GRID_ALIGN_STRETCH, i / 3, 1);

        lv_obj_add_event_cb(btn, menu_button_event_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);

        lv_obj_t *icon = lv_label_create(btn);
        lv_label_set_text(icon, symbols[i]);
        lv_obj_set_style_text_color(icon, lv_color_white(), LV_PART_MAIN);
        lv_obj_set_style_text_font(icon, &lv_font_montserrat_48, 0);
        lv_obj_align(icon, LV_ALIGN_CENTER, 0, -15);

        lv_obj_t *text = lv_label_create(btn);
        lv_label_set_text(text, labels[i]);
        lv_obj_set_style_text_color(text, lv_color_white(), LV_PART_MAIN);
        lv_obj_set_style_text_font(text, &lv_font_montserrat_16, 0);
        lv_obj_align_to(text, icon, LV_ALIGN_OUT_BOTTOM_MID, 0, 6);
    }

    /* Replace the boot screen. auto_del = true frees it. Safe here: the boot timer is
       already deleted and its fade animations (about 1.8 s) finished long before 5 s. */
    lv_screen_load_anim(main_screen, LV_SCR_LOAD_ANIM_FADE_ON, 300, 0, true);
}
