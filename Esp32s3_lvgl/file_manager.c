#include "ui.h"

/*
 * Folder screen: UI only for now.
 *
 * Your Linux version used opendir/readdir, getenv("HOME") and getpwuid(). None of that exists
 * on the ESP32. Whether your board has an SD card slot (or you want LittleFS) is UNKNOWN,
 * so this screen shows a placeholder instead of files.
 *
 * To make it real later:
 *   1. Mount the storage in a .cpp file (SD / SD_MMC / LittleFS).
 *   2. Expose a plain C function, e.g. int storage_list(const char *path, cb, ctx).
 *   3. Call it from fm_populate() and add one list button per entry.
 *   4. Do NOT strdup() a name per button (your old code leaked those). Keep the current path in
 *      one static buffer and build the child path from the button's own label text.
 */

static lv_obj_t *file_list;

static void fm_populate(void)
{
    lv_obj_clean(file_list);
    lv_list_add_text(file_list, "No storage connected yet");
}

void create_file_manager_screen(void)
{
    screen_file_manager = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen_file_manager, lv_color_hex(0x001F3F), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen_file_manager, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_remove_flag(screen_file_manager, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(screen_file_manager);
    lv_label_set_text(title, "Folder");
    lv_obj_set_style_text_color(title, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 15);

    lv_obj_t *back_btn = lv_button_create(screen_file_manager);
    lv_obj_add_event_cb(back_btn, go_back_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_t *back_lbl = lv_label_create(back_btn);
    lv_label_set_text(back_lbl, LV_SYMBOL_LEFT "Back");
    lv_obj_set_style_text_color(back_lbl, lv_color_white(), LV_PART_MAIN);

    file_list = lv_list_create(screen_file_manager);
    lv_obj_set_size(file_list, lv_pct(90), lv_pct(75));
    lv_obj_align(file_list, LV_ALIGN_BOTTOM_MID, 0, -10);

    fm_populate();
}
