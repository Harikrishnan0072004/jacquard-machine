/*
 * boot_screen.c - arc progress + fading titles
 *
 * PORT NOTES
 *  - lv_timer_del -> lv_timer_delete, lv_anim_set_time -> lv_anim_set_duration
 *  - When progress hits 100%, the main menu is loaded with auto_del = true,
 *    so the boot screen is FREED (it is never shown again - why keep it in RAM?)
 *  - Arc made non-clickable so a touch cannot drag the progress value.
 */
#include "ui.h"

static lv_obj_t * boot_arc;

static void boot_progress_cb(lv_timer_t * timer)
{
    int32_t value = lv_arc_get_value(boot_arc);
    if (value < 100) {
        lv_arc_set_value(boot_arc, value + 2);
    } else {
        lv_timer_delete(timer);
        create_main_menu_screen();
        /* last arg true = delete the boot screen after the animation */
        lv_screen_load_anim(main_screen, LV_SCREEN_LOAD_ANIM_FADE_IN, 400, 0, true);
    }
}

static void boot_label_timer_cb(lv_timer_t * t)
{
    lv_obj_t * label = (lv_obj_t *)lv_timer_get_user_data(t);
    if (label == NULL) return;

    int32_t value = lv_arc_get_value(boot_arc);
    char buf[8];
    lv_snprintf(buf, sizeof(buf), "%d%%", (int)value);
    lv_label_set_text(label, buf);

    if (value >= 100) {
        lv_timer_delete(t);
    }
}

/* Signature matches lv_anim_exec_xcb_t exactly, so no cast is needed */
static void fade_in_anim_cb(void * var, int32_t v)
{
    lv_obj_set_style_opa((lv_obj_t *)var, (lv_opa_t)v, LV_PART_MAIN);
}

static void start_fade(lv_obj_t * obj, uint32_t delay_ms)
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
    lv_obj_t * screen_boot = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen_boot, lv_color_hex(0x001F3F), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen_boot, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_remove_flag(screen_boot, LV_OBJ_FLAG_SCROLLABLE);

    /* Company name */
    lv_obj_t * label_company = lv_label_create(screen_boot);
    lv_label_set_text(label_company, "Tilt Pvt Ltd");
    lv_obj_set_style_text_color(label_company, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(label_company, &lv_font_montserrat_24, 0);
    lv_obj_align(label_company, LV_ALIGN_CENTER, 0, -100);
    lv_obj_set_style_opa(label_company, LV_OPA_TRANSP, LV_PART_MAIN);

    /* Product name */
    lv_obj_t * label_product = lv_label_create(screen_boot);
    lv_label_set_text(label_product, "Jacquard Machine");
    lv_obj_set_style_text_color(label_product, lv_color_hex(0xFFD700), LV_PART_MAIN);
    lv_obj_set_style_text_font(label_product, &lv_font_montserrat_32, 0);
    lv_obj_align(label_product, LV_ALIGN_CENTER, 0, -55);
    lv_obj_set_style_opa(label_product, LV_OPA_TRANSP, LV_PART_MAIN);

    /* Circular progress arc (moved slightly for 320 px height) */
    boot_arc = lv_arc_create(screen_boot);
    lv_obj_set_size(boot_arc, 120, 120);
    lv_obj_align(boot_arc, LV_ALIGN_CENTER, 0, 60);
    lv_arc_set_bg_angles(boot_arc, 0, 360);
    lv_arc_set_rotation(boot_arc, 270);
    lv_arc_set_range(boot_arc, 0, 100);
    lv_arc_set_value(boot_arc, 0);
    lv_arc_set_mode(boot_arc, LV_ARC_MODE_NORMAL);
    lv_obj_remove_flag(boot_arc, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_set_style_arc_color(boot_arc, lv_color_hex(0x003366), LV_PART_MAIN);
    lv_obj_set_style_arc_width(boot_arc, 10, LV_PART_MAIN);
    lv_obj_set_style_arc_color(boot_arc, lv_color_hex(0xFFD700), LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(boot_arc, 10, LV_PART_INDICATOR);
    lv_obj_remove_style(boot_arc, NULL, LV_PART_KNOB);

    /* Percentage label inside the arc */
    lv_obj_t * progress_label = lv_label_create(boot_arc);
    lv_label_set_text(progress_label, "0%");
    lv_obj_set_style_text_color(progress_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_center(progress_label);

    lv_screen_load(screen_boot);

    start_fade(label_company, 0);
    start_fade(label_product, 800);

    lv_timer_create(boot_progress_cb, 100, NULL);
    lv_timer_create(boot_label_timer_cb, 100, progress_label);
}
