#include "ui.h"

/* Back to the Settings screen (used by the settings sub-screens) */
void go_settings_event_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    ui_goto(screen_settings, false);
}

/* Row click -> open the matching sub-screen. user data = row id. */
static void settings_menu_event_cb(lv_event_t *e)
{
    intptr_t id = (intptr_t)lv_event_get_user_data(e);
    lv_obj_t *target = NULL;

    switch (id) {
        case 1: if (!screen_settings_password)     create_settings_password_screen();
                target = screen_settings_password;     break;
        case 2: if (!screen_settings_card_count)   create_settings_card_count_screen();
                target = screen_settings_card_count;   break;
        case 3: /* Invert Design is a switch on its own row, not a screen (see below) */ break;
        case 4: if (!screen_settings_front_back)   create_settings_front_back_screen();
                target = screen_settings_front_back;   break;
        default: LV_LOG_ERROR("Invalid settings id %d", (int)id); break;
    }
    ui_goto(target, true);
}

void create_settings_screen(void)
{
    screen_settings = lv_obj_create(NULL);
    lv_obj_remove_style_all(screen_settings);
    lv_obj_set_style_bg_color(screen_settings, lv_color_hex(0x001F3F), 0);
    lv_obj_set_style_bg_opa(screen_settings, LV_OPA_COVER, 0);
    lv_obj_remove_flag(screen_settings, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(screen_settings);
    lv_label_set_text(title, "System Settings");
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 16);

    lv_obj_t *menu_cont = lv_obj_create(screen_settings);
    lv_obj_remove_style_all(menu_cont);
    lv_obj_set_size(menu_cont, lv_pct(90), lv_pct(70));
    lv_obj_align(menu_cont, LV_ALIGN_CENTER, 0, 12);
    lv_obj_set_flex_flow(menu_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(menu_cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(menu_cont, 8, 0);

    static const struct {
        const char *icon;
        const char *text;
        uint8_t id;
    } menu_items[] = {
        {LV_SYMBOL_EDIT,    "New Password",         1},
        {LV_SYMBOL_FILE,    "Number of Cards",      2},
        {LV_SYMBOL_REFRESH, "Invert Design",        3},
        {LV_SYMBOL_SHUFFLE, "Front/Back Selection", 4},
    };
    const size_t n_items = sizeof(menu_items) / sizeof(menu_items[0]);

    for (size_t i = 0; i < n_items; ++i) {
        lv_obj_t *row_btn = lv_button_create(menu_cont);
        lv_obj_remove_style_all(row_btn);
        lv_obj_set_width(row_btn, lv_pct(100));
        lv_obj_set_height(row_btn, 42);
        lv_obj_set_style_bg_color(row_btn, lv_color_hex(0x004080), 0);
        lv_obj_set_style_bg_opa(row_btn, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(row_btn, 10, 0);
        lv_obj_set_style_pad_all(row_btn, 8, 0);
        lv_obj_set_flex_flow(row_btn, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row_btn, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        lv_obj_t *left = lv_obj_create(row_btn);
        lv_obj_remove_style_all(left);
        lv_obj_set_flex_flow(left, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(left, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_flex_grow(left, 1);
        lv_obj_set_width(left, lv_pct(100));
        lv_obj_set_height(left, LV_SIZE_CONTENT);
        lv_obj_remove_flag(left, LV_OBJ_FLAG_CLICKABLE);   /* let the row receive the click */

        lv_obj_t *icon = lv_label_create(left);
        lv_label_set_text(icon, menu_items[i].icon);
        lv_obj_set_style_text_color(icon, lv_color_white(), 0);
        lv_obj_set_style_text_font(icon, &lv_font_montserrat_20, 0);

        lv_obj_t *lbl = lv_label_create(left);
        lv_label_set_text(lbl, menu_items[i].text);
        lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18, 0);
        lv_obj_set_style_pad_left(lbl, 10, 0);

        lv_obj_t *right = lv_obj_create(row_btn);
        lv_obj_remove_style_all(right);
        lv_obj_set_flex_flow(right, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(right, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_width(right, LV_SIZE_CONTENT);
        lv_obj_set_height(right, LV_SIZE_CONTENT);
        lv_obj_remove_flag(right, LV_OBJ_FLAG_CLICKABLE);  /* the switch inside stays clickable */

        if (menu_items[i].id == 3) {
            lv_obj_t *sw = lv_switch_create(right);
            lv_obj_set_size(sw, 48, 26);
            lv_obj_add_event_cb(sw, invert_design_switch_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
        } else {
            lv_obj_add_event_cb(row_btn, settings_menu_event_cb, LV_EVENT_CLICKED,
                                (void *)(intptr_t)menu_items[i].id);
        }
    }

    lv_obj_t *back_btn = lv_button_create(screen_settings);
    lv_obj_remove_style_all(back_btn);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(0x003366), 0);
    lv_obj_set_style_bg_opa(back_btn, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(back_btn, 8, 0);
    lv_obj_set_style_pad_all(back_btn, 6, 0);
    lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_add_event_cb(back_btn, go_back_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *back_lbl = lv_label_create(back_btn);
    lv_label_set_text(back_lbl, LV_SYMBOL_LEFT " Back");
    lv_obj_set_style_text_color(back_lbl, lv_color_white(), 0);
    lv_obj_center(back_lbl);
}
