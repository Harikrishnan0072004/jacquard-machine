#include "ui.h"

/* One timer drives both the arc and the label (was two timers sharing boot_arc). */
typedef struct {
    lv_obj_t *arc;
    lv_obj_t *label;
} boot_ctx_t;

static boot_ctx_t ctx;   /* static: must outlive create_boot_screen() */

static void fade_in_anim_cb(void *var, int32_t v)
{
    lv_obj_set_style_opa((lv_obj_t *)var, (lv_opa_t)v, LV_PART_MAIN);
}

static void boot_timer_cb(lv_timer_t *t)
{
    boot_ctx_t *c = (boot_ctx_t *)lv_timer_get_user_data(t);

    int32_t value = lv_arc_get_value(c->arc) + 2;
    if (value > 100) value = 100;

    lv_arc_set_value(c->arc, value);
    lv_label_set_text_fmt(c->label, "%d%%", (int)value);

    if (value >= 100) {
        lv_timer_delete(t);           /* stop the timer first */
        create_main_menu_screen();    /* loads the menu and deletes this boot screen */
    }
}

static void fade_in(lv_obj_t *obj, uint32_t delay_ms)
{
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_values(&a, 0, 255);
    lv_anim_set_duration(&a, 1000);
    lv_anim_set_delay(&a, delay_ms);
    lv_anim_set_exec_cb(&a, fade_in_anim_cb);
    lv_anim_start(&a);
}

void create_boot_screen(void)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x001F3F), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *company = lv_label_create(scr);
    lv_label_set_text(company, "Tilt Pvt Ltd");
    lv_obj_set_style_text_color(company, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(company, &lv_font_montserrat_24, 0);
    lv_obj_align(company, LV_ALIGN_CENTER, 0, -80);
    lv_obj_set_style_opa(company, LV_OPA_TRANSP, LV_PART_MAIN);

    lv_obj_t *product = lv_label_create(scr);
    lv_label_set_text(product, "Jacquard Machine");
    lv_obj_set_style_text_color(product, lv_color_hex(0xFFD700), LV_PART_MAIN);
    lv_obj_set_style_text_font(product, &lv_font_montserrat_32, 0);
    lv_obj_align(product, LV_ALIGN_CENTER, 0, -30);
    lv_obj_set_style_opa(product, LV_OPA_TRANSP, LV_PART_MAIN);

    ctx.arc = lv_arc_create(scr);
    lv_obj_set_size(ctx.arc, 120, 120);
    lv_obj_align(ctx.arc, LV_ALIGN_CENTER, 0, 70);
    lv_arc_set_bg_angles(ctx.arc, 0, 360);
    lv_arc_set_rotation(ctx.arc, 270);
    lv_arc_set_range(ctx.arc, 0, 100);
    lv_arc_set_value(ctx.arc, 0);
    lv_arc_set_mode(ctx.arc, LV_ARC_MODE_NORMAL);
    lv_obj_set_style_arc_color(ctx.arc, lv_color_hex(0x003366), LV_PART_MAIN);
    lv_obj_set_style_arc_width(ctx.arc, 10, LV_PART_MAIN);
    lv_obj_set_style_arc_color(ctx.arc, lv_color_hex(0xFFD700), LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(ctx.arc, 10, LV_PART_INDICATOR);
    lv_obj_remove_style(ctx.arc, NULL, LV_PART_KNOB);
    lv_obj_remove_flag(ctx.arc, LV_OBJ_FLAG_CLICKABLE);

    ctx.label = lv_label_create(ctx.arc);
    lv_label_set_text(ctx.label, "0%");
    lv_obj_set_style_text_color(ctx.label, lv_color_white(), LV_PART_MAIN);
    lv_obj_center(ctx.label);

    lv_screen_load(scr);

    fade_in(company, 0);
    fade_in(product, 800);

    lv_timer_create(boot_timer_cb, 100, &ctx);
}
