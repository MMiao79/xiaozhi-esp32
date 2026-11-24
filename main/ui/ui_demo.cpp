#include "ui_demo.h"

#include "lvgl.h"
#include "board.h"
#include "audio/audio_codec.h"
#include "boards/common/backlight.h"

#include <stdio.h>

static lv_obj_t* demo_screen = nullptr;
static lv_obj_t* prev_screen = nullptr;
static lv_obj_t* volume_label = nullptr;
static lv_obj_t* brightness_label = nullptr;
static lv_obj_t* touch_label = nullptr;
static lv_obj_t* status_label = nullptr;

static int s_volume = 70;
static int s_brightness = 80;

/* ---------- helpers ---------- */

static lv_obj_t* make_btn(lv_obj_t* parent, const char* text, lv_event_cb_t cb,
                          lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_coord_t h)
{
    lv_obj_t* btn = lv_btn_create(parent);
    lv_obj_set_pos(btn, x, y);
    lv_obj_set_size(btn, w, h);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x1E3A5F), 0);
    lv_obj_set_style_border_color(btn, lv_color_hex(0x38BDF8), 0);
    lv_obj_set_style_border_width(btn, 1, 0);
    lv_obj_set_style_radius(btn, 6, 0);

    lv_obj_t* lab = lv_label_create(btn);
    lv_label_set_text(lab, text);
    lv_obj_set_style_text_color(lab, lv_color_hex(0xE2E8F0), 0);
    lv_obj_center(lab);

    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, nullptr);
    return btn;
}

static void refresh_status(void)
{
    char buf[64];
    if (volume_label) {
        snprintf(buf, sizeof(buf), "音量: %d%%", s_volume);
        lv_label_set_text(volume_label, buf);
    }
    if (brightness_label) {
        snprintf(buf, sizeof(buf), "亮度: %d%%", s_brightness);
        lv_label_set_text(brightness_label, buf);
    }
}

/* ---------- 事件回调 ---------- */

static void on_vol_up(lv_event_t*)
{
    s_volume = (s_volume + 10 > 100) ? 100 : s_volume + 10;
    Board::GetInstance().GetAudioCodec()->SetOutputVolume(s_volume);
    refresh_status();
    if (status_label) lv_label_set_text(status_label, "音量 +10");
}

static void on_vol_down(lv_event_t*)
{
    s_volume = (s_volume - 10 < 0) ? 0 : s_volume - 10;
    Board::GetInstance().GetAudioCodec()->SetOutputVolume(s_volume);
    refresh_status();
    if (status_label) lv_label_set_text(status_label, "音量 -10");
}

static void on_bri_up(lv_event_t*)
{
    s_brightness = (s_brightness + 20 > 100) ? 100 : s_brightness + 20;
    auto bl = Board::GetInstance().GetBacklight();
    if (bl) bl->SetBrightness((uint8_t)s_brightness, false);
    refresh_status();
    if (status_label) lv_label_set_text(status_label, "亮度 +20");
}

static void on_bri_down(lv_event_t*)
{
    s_brightness = (s_brightness - 20 < 0) ? 0 : s_brightness - 20;
    auto bl = Board::GetInstance().GetBacklight();
    if (bl) bl->SetBrightness((uint8_t)s_brightness, false);
    refresh_status();
    if (status_label) lv_label_set_text(status_label, "亮度 -20");
}

static void on_back(lv_event_t*)
{
    ui_demo_hide();
}

/* 触摸坐标轮询（每 100ms） */
static void touch_timer_cb(lv_timer_t*)
{
    lv_indev_t* indev = lv_indev_get_next(nullptr);
    while (indev) {
        if (lv_indev_get_type(indev) == LV_INDEV_TYPE_POINTER) {
            lv_point_t p;
            lv_indev_get_point(indev, &p);
            static lv_point_t last = {-1, -1};
            if (p.x != last.x || p.y != last.y) {
                last = p;
                if (touch_label) {
                    char buf[48];
                    snprintf(buf, sizeof(buf), "触摸: x=%d y=%d", (int)p.x, (int)p.y);
                    lv_label_set_text(touch_label, buf);
                }
            }
            break;
        }
        indev = lv_indev_get_next(indev);
    }
}

/* ---------- 公共接口 ---------- */

void ui_demo_init(void)
{
    if (demo_screen != nullptr) {
        return;
    }

    demo_screen = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(demo_screen, lv_color_hex(0x0B1220), 0);
    lv_obj_set_style_bg_opa(demo_screen, LV_OPA_COVER, 0);

    /* 标题 */
    lv_obj_t* title = lv_label_create(demo_screen);
    lv_label_set_text(title, "触摸测试菜单");
    lv_obj_set_style_text_color(title, lv_color_hex(0x38BDF8), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 6);

    /* 音量 +/- */
    make_btn(demo_screen, "音量 +", on_vol_up, 6, 34, 100, 40);
    make_btn(demo_screen, "音量 -", on_vol_down, 112, 34, 100, 40);

    /* 亮度 +/- */
    make_btn(demo_screen, "亮度 +", on_bri_up, 6, 80, 100, 40);
    make_btn(demo_screen, "亮度 -", on_bri_down, 112, 80, 100, 40);

    /* 当前值 */
    volume_label = lv_label_create(demo_screen);
    lv_obj_set_style_text_color(volume_label, lv_color_hex(0x94A3B8), 0);
    lv_obj_set_pos(volume_label, 220, 40);

    brightness_label = lv_label_create(demo_screen);
    lv_obj_set_style_text_color(brightness_label, lv_color_hex(0x94A3B8), 0);
    lv_obj_set_pos(brightness_label, 220, 86);

    /* 返回 */
    make_btn(demo_screen, "返回主界面", on_back, 6, 132, 206, 34);

    /* 触摸坐标 */
    touch_label = lv_label_create(demo_screen);
    lv_label_set_text(touch_label, "触摸: 未检测");
    lv_obj_set_style_text_color(touch_label, lv_color_hex(0x2ECC71), 0);
    lv_obj_set_pos(touch_label, 6, 176);

    /* 状态提示 */
    status_label = lv_label_create(demo_screen);
    lv_label_set_text(status_label, "等待操作...");
    lv_obj_set_style_text_color(status_label, lv_color_hex(0xF1C40F), 0);
    lv_obj_set_pos(status_label, 6, 204);

    refresh_status();

    /* 触摸坐标轮询定时器 */
    lv_timer_create(touch_timer_cb, 100, nullptr);
}

void ui_demo_show(void)
{
    if (demo_screen == nullptr) {
        ui_demo_init();
    }
    if (demo_screen != nullptr) {
        prev_screen = lv_screen_active();
        lv_screen_load(demo_screen);
    }
}

void ui_demo_hide(void)
{
    if (prev_screen != nullptr && prev_screen != demo_screen) {
        lv_screen_load(prev_screen);
    }
}

void ui_demo_set_text(const char* text)
{
    if (status_label != nullptr && text != nullptr) {
        lv_label_set_text(status_label, text);
    }
}
