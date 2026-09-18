/*
 * front_back.c - Settings > Front/Back Selection (4 radio options)
 *
 * PORT NOTES
 *  - Back / OK now return to SETTINGS (this is a settings sub-screen)
 *  - Unused static functions removed (they only caused warnings)
 *  - Radio container height computed with flex_grow instead of reading
 *    object heights before LVGL has done the layout
 *  - lv_coord_t -> int32_t, clear_* -> remove_*
 *  - No lv_screen_load() here (settings.c loads it)
 */
#include "ui.h"

static int active_fb_selection_idx = 0;

static void front_back_radio_event_cb(lv_event_t * e)
{
    lv_obj_t * clicked = (lv_obj_t *)lv_event_get_current_target(e);
    int idx = (int)(intptr_t)lv_event_get_user_data(e);

    lv_obj_t * parent_cont = lv_obj_get_parent(lv_obj_get_parent(clicked));
    uint32_t row_count = lv_obj_get_child_count(parent_cont);

    for (uint32_t i = 0; i < row_count; i++) {
        lv_obj_t * row = lv_obj_get_child(parent_cont, (int32_t)i);
        lv_obj_t * btn = lv_obj_get_child(row, 0);
        lv_obj_t * dot = (lv_obj_t *)lv_obj_get_user_data(btn);
        lv_obj_remove_state(btn, LV_STATE_CHECKED);
        lv_obj_add_flag(dot, LV_OBJ_FLAG_HIDDEN);
    }

    lv_obj_add_state(clicked, LV_STATE_CHECKED);
    lv_obj_remove_flag((lv_obj_t *)lv_obj_get_user_data(clicked), LV_OBJ_FLAG_HIDDEN);

    active_fb_selection_idx = idx;
    LV_LOG_USER("Front/Back option %d selected", idx);
}

static lv_obj_t * make_top_button(lv_obj_t * parent, const char * text, lv_event_cb_t cb)
{
    lv_obj_t * btn = lv_button_create(parent);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x1E90FF), 0);
    lv_obj_set_style_radius(btn, 10, 0);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t * lbl = lv_label_create(btn);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
    lv_obj_center(lbl);
    return btn;
}

void create_settings_front_back_screen(void)
{
    /* Screen = vertical flex: [top bar][radio container fills the rest] */
    screen_settings_front_back = lv_obj_create(NULL);
    lv_obj_t * scr = screen_settings_front_back;
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x001F3F), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(scr, 0, 0);
    lv_obj_set_style_pad_row(scr, 0, 0);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(scr, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);

    /* Top bar */
    lv_obj_t * top_bar = lv_obj_create(scr);
    lv_obj_set_size(top_bar, lv_pct(100), 56);
    lv_obj_set_style_bg_color(top_bar, lv_color_hex(0x002B55), 0);
    lv_obj_set_style_border_width(top_bar, 0, 0);
    lv_obj_set_style_radius(top_bar, 0, 0);
    lv_obj_set_style_pad_all(top_bar, 8, 0);
    lv_obj_remove_flag(top_bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(top_bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(top_bar, LV_FLEX_ALIGN_SPACE_BETWEEN,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    make_top_button(top_bar, LV_SYMBOL_LEFT " Back", go_back_to_settings_event_cb);

    lv_obj_t * title = lv_label_create(top_bar);
    lv_label_set_text(title, "Front / Back Selection");
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_22, 0);

    make_top_button(top_bar, "OK", go_back_to_settings_event_cb);

    /* Radio container: takes all remaining height */
    lv_obj_t * radio_cont = lv_obj_create(scr);
    lv_obj_set_width(radio_cont, lv_pct(100));
    lv_obj_set_flex_grow(radio_cont, 1);
    lv_obj_set_style_bg_opa(radio_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(radio_cont, 0, 0);
    lv_obj_set_style_pad_all(radio_cont, 20, 0);
    lv_obj_set_style_pad_row(radio_cont, 8, 0);
    lv_obj_set_flex_flow(radio_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(radio_cont, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(radio_cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(radio_cont, LV_SCROLLBAR_MODE_OFF);

    static const char * options[] = {"Front Left", "Front Right", "Back Left", "Back Right"};
    lv_obj_t * radio_btns[4];

    for (int i = 0; i < 4; i++) {
        lv_obj_t * row = lv_obj_create(radio_cont);
        lv_obj_set_width(row, lv_pct(100));
        lv_obj_set_height(row, 44);
        lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(row, 0, 0);
        lv_obj_set_style_pad_all(row, 4, 0);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);

        radio_btns[i] = lv_button_create(row);
        lv_obj_set_size(radio_btns[i], 28, 28);
        lv_obj_set_style_radius(radio_btns[i], LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(radio_btns[i], lv_color_white(), 0);
        lv_obj_set_style_border_color(radio_btns[i], lv_color_hex(0x0050A0), 0);
        lv_obj_set_style_border_width(radio_btns[i], 3, 0);
        lv_obj_set_style_pad_all(radio_btns[i], 0, 0);
        lv_obj_add_flag(radio_btns[i], LV_OBJ_FLAG_CHECKABLE);
        lv_obj_add_event_cb(radio_btns[i], front_back_radio_event_cb,
                            LV_EVENT_CLICKED, (void *)(intptr_t)i);

        lv_obj_t * dot = lv_obj_create(radio_btns[i]);
        lv_obj_set_size(dot, 12, 12);
        lv_obj_center(dot);
        lv_obj_set_style_bg_color(dot, lv_color_hex(0x0050A0), 0);
        lv_obj_set_style_border_width(dot, 0, 0);
        lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
        lv_obj_remove_flag(dot, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_flag(dot, LV_OBJ_FLAG_IGNORE_LAYOUT);
        lv_obj_add_flag(dot, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_user_data(radio_btns[i], dot);

        lv_obj_t * lbl = lv_label_create(row);
        lv_label_set_text(lbl, options[i]);
        lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_20, 0);
        lv_obj_set_style_pad_left(lbl, 12, 0);
    }

    /* Restore the previous choice (screen is rebuilt only once, but be safe) */
    lv_obj_add_state(radio_btns[active_fb_selection_idx], LV_STATE_CHECKED);
    lv_obj_remove_flag((lv_obj_t *)lv_obj_get_user_data(radio_btns[active_fb_selection_idx]),
                       LV_OBJ_FLAG_HIDDEN);
}
