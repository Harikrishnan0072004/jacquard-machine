/*
 * file_manager.c - ESP32 placeholder
 *
 * The simulator version used Linux APIs (opendir, getpwuid, $HOME).
 * The ESP32 has no Linux home folder, so for now this screen only shows
 * a message. In Phase 5 it will list files from an SD card.
 *
 * Also note: the old version called strdup() for every file and never
 * freed it - a leak that grew each time you opened a folder.
 */
#include "ui.h"

void create_file_manager_screen(void)
{
    screen_file_manager = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen_file_manager, lv_color_hex(0x001F3F), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen_file_manager, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_remove_flag(screen_file_manager, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t * title = lv_label_create(screen_file_manager);
    lv_label_set_text(title, "Folder");
    lv_obj_set_style_text_color(title, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 15);

    lv_obj_t * back_btn = lv_button_create(screen_file_manager);
    lv_obj_add_event_cb(back_btn, go_back_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_t * back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT " Back");
    lv_obj_set_style_text_color(back_label, lv_color_white(), LV_PART_MAIN);

    lv_obj_t * list = lv_list_create(screen_file_manager);
    lv_obj_set_size(list, lv_pct(90), lv_pct(72));
    lv_obj_align(list, LV_ALIGN_BOTTOM_MID, 0, -10);

    lv_list_add_text(list, "Storage");
    lv_list_add_button(list, LV_SYMBOL_SD_CARD, "SD card not configured yet");
    lv_list_add_button(list, LV_SYMBOL_WARNING, "File browsing comes in Phase 5");
}
