#include "ui.h"

/* ---- screen pointers: defined ONCE here, declared extern in ui.h ---- */
lv_obj_t *main_screen;
lv_obj_t *screen_type, *screen_file_manager, *screen_settings;
lv_obj_t *screen_test_files, *screen_finger, *screen_about;
lv_obj_t *screen_settings_password, *screen_settings_card_count;
lv_obj_t *screen_settings_invert_design, *screen_settings_front_back;

/* Entry point, called from setup() after the display is ready */
void my_app_init(void)
{
    create_boot_screen();
}

/* Slide to a screen. auto_del = false: screens stay alive and are reused. */
void ui_goto(lv_obj_t *scr, bool forward)
{
    if (!scr) return;
    lv_screen_load_anim(scr,
                        forward ? LV_SCR_LOAD_ANIM_MOVE_LEFT : LV_SCR_LOAD_ANIM_MOVE_RIGHT,
                        UI_NAV_ANIM_MS, 0, false);
}

/* Radio group = container -> rows -> [radio button (child 0) with dot in user_data, label] */
void ui_radio_select(lv_obj_t *cont, int idx)
{
    if (!cont) return;
    uint32_t n = lv_obj_get_child_count(cont);
    for (uint32_t i = 0; i < n; i++) {
        lv_obj_t *row = lv_obj_get_child(cont, (int32_t)i);
        lv_obj_t *btn = lv_obj_get_child(row, 0);
        lv_obj_t *dot = (lv_obj_t *)lv_obj_get_user_data(btn);
        if ((int)i == idx) {
            lv_obj_add_state(btn, LV_STATE_CHECKED);
            lv_obj_remove_flag(dot, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_remove_state(btn, LV_STATE_CHECKED);
            lv_obj_add_flag(dot, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

/* One radio row. Same look as your original type / front_back / test-mode rows. */
lv_obj_t *ui_radio_row_create(lv_obj_t *cont, const char *text, int idx,
                              int32_t row_w, int32_t row_h, lv_event_cb_t cb)
{
    lv_obj_t *row = lv_obj_create(cont);
    lv_obj_set_size(row, row_w, row_h);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(row, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t *btn = lv_button_create(row);
    lv_obj_set_size(btn, 24, 24);
    lv_obj_set_style_radius(btn, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(btn, lv_color_white(), 0);
    lv_obj_set_style_border_color(btn, lv_color_hex(0x0050A0), 0);
    lv_obj_set_style_border_width(btn, 3, 0);
    lv_obj_add_flag(btn, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, (void *)(intptr_t)idx);

    lv_obj_t *dot = lv_obj_create(btn);
    lv_obj_set_size(dot, 12, 12);
    lv_obj_center(dot);
    lv_obj_set_style_bg_color(dot, lv_color_hex(0x0050A0), 0);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_add_flag(dot, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_add_flag(dot, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_user_data(btn, dot);           /* button remembers its dot */

    lv_obj_t *lbl = lv_label_create(row);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_20, 0);
    lv_obj_set_style_pad_left(lbl, 12, 0);

    return btn;
}
