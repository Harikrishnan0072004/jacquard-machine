#include "ui.h"

#define FB_TOP_BAR_H 56

static int active_fb_idx = 0;
static int saved_fb_idx  = 0;
static lv_obj_t *fb_radio_cont;

int ui_get_front_back(void) { return saved_fb_idx; }

static void fb_radio_event_cb(lv_event_t *e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    ui_radio_select(fb_radio_cont, idx);
    active_fb_idx = idx;
}

static void fb_back_event_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    active_fb_idx = saved_fb_idx;                    /* discard an unsaved choice */
    ui_radio_select(fb_radio_cont, saved_fb_idx);
    ui_goto(screen_settings, false);                 /* back to Settings, not the main menu */
}

static void fb_ok_event_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    saved_fb_idx = active_fb_idx;
    LV_LOG_USER("Front/Back saved = %d", saved_fb_idx);
    /* TODO: tell the machine code here */
    ui_goto(screen_settings, false);
}

void create_settings_front_back_screen(void)
{
    screen_settings_front_back = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen_settings_front_back, lv_color_hex(0x001F3F), 0);
    lv_obj_set_style_bg_opa(screen_settings_front_back, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(screen_settings_front_back, 0, 0);
    lv_obj_remove_flag(screen_settings_front_back, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(screen_settings_front_back, LV_SCROLLBAR_MODE_OFF);

    /* ---- top bar ---- */
    lv_obj_t *top_bar = lv_obj_create(screen_settings_front_back);
    lv_obj_set_size(top_bar, lv_pct(100), FB_TOP_BAR_H);
    lv_obj_align(top_bar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(top_bar, lv_color_hex(0x002B55), 0);
    lv_obj_set_style_border_width(top_bar, 0, 0);
    lv_obj_set_style_radius(top_bar, 0, 0);
    lv_obj_set_style_pad_all(top_bar, 8, 0);
    lv_obj_set_flex_flow(top_bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(top_bar, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(top_bar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *back_btn = lv_button_create(top_bar);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(0x1E90FF), 0);
    lv_obj_set_style_radius(back_btn, 10, 0);
    lv_obj_add_event_cb(back_btn, fb_back_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *back_lbl = lv_label_create(back_btn);
    lv_label_set_text(back_lbl, LV_SYMBOL_LEFT " Back");
    lv_obj_set_style_text_color(back_lbl, lv_color_white(), 0);
    lv_obj_center(back_lbl);

    lv_obj_t *title = lv_label_create(top_bar);
    lv_label_set_text(title, "Front / Back Selection");
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_22, 0);

    lv_obj_t *ok_btn = lv_button_create(top_bar);
    lv_obj_set_style_bg_color(ok_btn, lv_color_hex(0x1E90FF), 0);
    lv_obj_set_style_radius(ok_btn, 10, 0);
    lv_obj_add_event_cb(ok_btn, fb_ok_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *ok_lbl = lv_label_create(ok_btn);
    lv_label_set_text(ok_lbl, "OK");
    lv_obj_set_style_text_color(ok_lbl, lv_color_white(), 0);
    lv_obj_center(ok_lbl);

    /* ---- radio container: exactly the space under the top bar ---- */
    fb_radio_cont = lv_obj_create(screen_settings_front_back);
    lv_obj_set_size(fb_radio_cont, lv_pct(100), UI_VER_RES - FB_TOP_BAR_H);
    lv_obj_align(fb_radio_cont, LV_ALIGN_TOP_MID, 0, FB_TOP_BAR_H);
    lv_obj_set_style_bg_opa(fb_radio_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(fb_radio_cont, 0, 0);
    lv_obj_set_style_pad_all(fb_radio_cont, 20, 0);
    lv_obj_set_flex_flow(fb_radio_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(fb_radio_cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(fb_radio_cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(fb_radio_cont, LV_SCROLLBAR_MODE_OFF);

    const char *options[] = {"Front Left", "Front Right", "Back Left", "Back Right"};
    for (int i = 0; i < 4; i++) {
        ui_radio_row_create(fb_radio_cont, options[i], i, lv_pct(100), 44, fb_radio_event_cb);
    }
    active_fb_idx = saved_fb_idx;
    ui_radio_select(fb_radio_cont, saved_fb_idx);
}
