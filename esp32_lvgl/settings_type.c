/*
 * settings_type.c - "Type" screen: Single / Dual mode radio selection
 *
 * PORT NOTES
 *  - Duplicate forward declarations removed, extern block replaced by ui.h
 *  - (void*)99 -> (void*)(intptr_t)99
 *  - Inner dot made non-clickable, clear_* -> remove_*
 *  - No lv_screen_load() here (main_menu.c loads it)
 */
#include "ui.h"

static void type_radio_event_cb(lv_event_t * e);
static void type_selection_event_cb(lv_event_t * e);

static int active_type_selection_idx = 0;

#define TYPE_OK_ACTION 99

void create_type_screen(void)
{
    screen_type = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen_type, lv_color_hex(0x001F3F), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen_type, LV_OPA_COVER, LV_PART_MAIN);

    /* Title */
    lv_obj_t * title = lv_label_create(screen_type);
    lv_label_set_text(title, "Select Type Option");
    lv_obj_set_style_text_color(title, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 15);

    /* Radio container */
    lv_obj_t * radio_cont = lv_obj_create(screen_type);
    lv_obj_set_size(radio_cont, lv_pct(90), lv_pct(50));
    lv_obj_center(radio_cont);
    lv_obj_set_style_bg_opa(radio_cont, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(radio_cont, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(radio_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(radio_cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(radio_cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(radio_cont, LV_SCROLLBAR_MODE_OFF);

    static const char * options[] = {"Single Mode", "Dual Mode"};
    lv_obj_t * radio_btns[2];

    for (int i = 0; i < 2; i++) {
        lv_obj_t * row = lv_obj_create(radio_cont);
        lv_obj_set_size(row, lv_pct(90), 48);
        lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_width(row, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(row, 4, LV_PART_MAIN);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_scrollbar_mode(row, LV_SCROLLBAR_MODE_OFF);

        /* Radio circle */
        radio_btns[i] = lv_button_create(row);
        lv_obj_set_size(radio_btns[i], 28, 28);
        lv_obj_set_style_radius(radio_btns[i], LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(radio_btns[i], lv_color_white(), 0);
        lv_obj_set_style_border_color(radio_btns[i], lv_color_hex(0x0050A0), 0);
        lv_obj_set_style_border_width(radio_btns[i], 3, 0);
        lv_obj_set_style_pad_all(radio_btns[i], 0, 0);
        lv_obj_add_flag(radio_btns[i], LV_OBJ_FLAG_CHECKABLE);
        lv_obj_add_event_cb(radio_btns[i], type_radio_event_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);

        /* Inner dot */
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

        /* Label */
        lv_obj_t * label = lv_label_create(row);
        lv_label_set_text(label, options[i]);
        lv_obj_set_style_text_color(label, lv_color_white(), LV_PART_MAIN);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_20, 0);
        lv_obj_set_style_pad_left(label, 10, 0);
    }

    /* Default: Single */
    active_type_selection_idx = 0;
    lv_obj_add_state(radio_btns[0], LV_STATE_CHECKED);
    lv_obj_remove_flag((lv_obj_t *)lv_obj_get_user_data(radio_btns[0]), LV_OBJ_FLAG_HIDDEN);

    /* Back */
    lv_obj_t * back_btn = lv_button_create(screen_type);
    lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(0x003366), LV_PART_MAIN);
    lv_obj_set_style_radius(back_btn, 8, LV_PART_MAIN);
    lv_obj_add_event_cb(back_btn, go_back_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t * back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT " Back");
    lv_obj_set_style_text_color(back_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_center(back_label);

    /* OK */
    lv_obj_t * ok_btn = lv_button_create(screen_type);
    lv_obj_align(ok_btn, LV_ALIGN_TOP_RIGHT, -10, 10);
    lv_obj_set_style_bg_color(ok_btn, lv_color_hex(0x0050A0), LV_PART_MAIN);
    lv_obj_set_style_radius(ok_btn, 8, LV_PART_MAIN);
    lv_obj_add_event_cb(ok_btn, type_selection_event_cb, LV_EVENT_CLICKED,
                        (void *)(intptr_t)TYPE_OK_ACTION);
    lv_obj_t * ok_label = lv_label_create(ok_btn);
    lv_label_set_text(ok_label, "Ok");
    lv_obj_set_style_text_color(ok_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_center(ok_label);
}

static void type_radio_event_cb(lv_event_t * e)
{
    lv_obj_t * clicked = (lv_obj_t *)lv_event_get_current_target(e);
    int idx = (int)(intptr_t)lv_event_get_user_data(e);

    /* button -> row -> radio_cont */
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

    active_type_selection_idx = idx;
}

static void type_selection_event_cb(lv_event_t * e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;

    int action_id = (int)(intptr_t)lv_event_get_user_data(e);
    if (action_id == TYPE_OK_ACTION) {
        LV_LOG_USER("%s selected", active_type_selection_idx == 0 ? "Single Mode" : "Dual Mode");
        lv_screen_load_anim(main_screen, LV_SCREEN_LOAD_ANIM_MOVE_RIGHT, 300, 0, false);
    }
}
