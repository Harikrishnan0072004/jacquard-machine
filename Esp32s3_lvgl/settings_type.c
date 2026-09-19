#include "ui.h"

static int active_type_idx = 0;   /* what is highlighted now       */
static int saved_type_idx  = 0;   /* what OK committed             */
static lv_obj_t *type_radio_cont;

int ui_get_type_mode(void) { return saved_type_idx; }

static void type_radio_event_cb(lv_event_t *e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    ui_radio_select(type_radio_cont, idx);
    active_type_idx = idx;
}

static void type_back_event_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    active_type_idx = saved_type_idx;                 /* discard an unsaved choice */
    ui_radio_select(type_radio_cont, saved_type_idx);
    ui_goto(main_screen, false);
}

static void type_ok_event_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    saved_type_idx = active_type_idx;
    LV_LOG_USER("Type mode saved = %d (0=Single, 1=Dual)", saved_type_idx);
    /* TODO: tell the machine code here */
    ui_goto(main_screen, false);
}

void create_type_screen(void)
{
    screen_type = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen_type, lv_color_hex(0x001F3F), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen_type, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_remove_flag(screen_type, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(screen_type);
    lv_label_set_text(title, "Select Type Option");
    lv_obj_set_style_text_color(title, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 15);

    type_radio_cont = lv_obj_create(screen_type);
    lv_obj_set_size(type_radio_cont, lv_pct(90), lv_pct(50));
    lv_obj_center(type_radio_cont);
    lv_obj_set_style_bg_opa(type_radio_cont, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(type_radio_cont, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(type_radio_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(type_radio_cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(type_radio_cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(type_radio_cont, LV_SCROLLBAR_MODE_OFF);

    const char *options[] = {"Single Mode", "Dual Mode"};
    for (int i = 0; i < 2; i++) {
        ui_radio_row_create(type_radio_cont, options[i], i, lv_pct(90), 40, type_radio_event_cb);
    }
    active_type_idx = saved_type_idx;
    ui_radio_select(type_radio_cont, saved_type_idx);

    lv_obj_t *back_btn = lv_button_create(screen_type);
    lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(0x003366), LV_PART_MAIN);
    lv_obj_set_style_radius(back_btn, 8, LV_PART_MAIN);
    lv_obj_add_event_cb(back_btn, type_back_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *back_lbl = lv_label_create(back_btn);
    lv_label_set_text(back_lbl, LV_SYMBOL_LEFT " Back");
    lv_obj_set_style_text_color(back_lbl, lv_color_white(), LV_PART_MAIN);
    lv_obj_center(back_lbl);

    lv_obj_t *ok_btn = lv_button_create(screen_type);
    lv_obj_align(ok_btn, LV_ALIGN_TOP_RIGHT, -10, 10);
    lv_obj_set_style_bg_color(ok_btn, lv_color_hex(0x0050A0), LV_PART_MAIN);
    lv_obj_set_style_radius(ok_btn, 8, LV_PART_MAIN);
    lv_obj_add_event_cb(ok_btn, type_ok_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *ok_lbl = lv_label_create(ok_btn);
    lv_label_set_text(ok_lbl, "Ok");
    lv_obj_set_style_text_color(ok_lbl, lv_color_white(), LV_PART_MAIN);
    lv_obj_center(ok_lbl);
}
