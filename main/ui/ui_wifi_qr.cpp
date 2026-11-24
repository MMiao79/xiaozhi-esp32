#include "ui_wifi_qr.h"

#include "lvgl.h"
#include "wifi_manager.h"
#include "display.h"
#include "board.h"

#include "qrcodegen.h"

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>

static lv_obj_t* qr_screen = nullptr;
static lv_obj_t* prev_screen = nullptr;

/* 统一画布尺寸（保证两个二维码视觉一致） */
#define QR_CANVAS_PX   92

static uint8_t s_qr_temp[xzq_BUFFER_LEN_MAX];
static uint8_t s_qr_code[xzq_BUFFER_LEN_MAX];
static uint16_t* s_qr_buf[2] = { nullptr, nullptr };
static int s_qr_buf_size[2] = { 0, 0 };

static lv_obj_t* draw_qr(lv_obj_t* parent, const char* text, int slot, int x, int y)
{
    if (text == nullptr || strlen(text) == 0) return nullptr;

    /* 强制 version 5~10，保证两个二维码模块数一致（视觉大小相同） */
    if (!xzq_encodeText(text, s_qr_temp, s_qr_code,
                        xzq_Ecc_MEDIUM, 5, 10,
                        xzq_Mask_AUTO, false)) {
        return nullptr;
    }

    int modules = xzq_getSize(s_qr_code);
    int quiet = 2;
    int total = modules + quiet * 2;

    int px = QR_CANVAS_PX;
    int scale = px / total;
    if (scale < 1) scale = 1;
    int qr_px = total * scale;
    int off = (px - qr_px) / 2;

    size_t need = (size_t)px * px * sizeof(uint16_t);
    if (s_qr_buf[slot] == nullptr || s_qr_buf_size[slot] < (int)need) {
        if (s_qr_buf[slot]) free(s_qr_buf[slot]);
        s_qr_buf[slot] = (uint16_t*)malloc(need);
        s_qr_buf_size[slot] = need;
    }
    if (s_qr_buf[slot] == nullptr) return nullptr;

    for (int py = 0; py < px; py++) {
        for (int pxx = 0; pxx < px; pxx++) {
            bool dark = false;
            int rx = pxx - off;
            int ry = py - off;
            if (rx >= 0 && ry >= 0 && rx < qr_px && ry < qr_px) {
                int mx = rx / scale - quiet;
                int my = ry / scale - quiet;
                if (mx >= 0 && my >= 0 && mx < modules && my < modules) {
                    dark = xzq_getModule(s_qr_code, mx, my);
                }
            }
            s_qr_buf[slot][py * px + pxx] = dark ? 0x0000 : 0xFFFF;
        }
    }

    lv_obj_t* canvas = lv_canvas_create(parent);
    lv_canvas_set_buffer(canvas, s_qr_buf[slot], px, px, LV_COLOR_FORMAT_RGB565);
    lv_obj_set_pos(canvas, x, y);
    return canvas;
}

/* 带边框的容器 */
static lv_obj_t* make_box(lv_obj_t* parent, int x, int y, int w, int h, uint32_t border_color)
{
    lv_obj_t* box = lv_obj_create(parent);
    lv_obj_set_pos(box, x, y);
    lv_obj_set_size(box, w, h);
    lv_obj_set_style_bg_opa(box, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_color(box, lv_color_hex(border_color), 0);
    lv_obj_set_style_border_width(box, 1, 0);
    lv_obj_set_style_radius(box, 4, 0);
    lv_obj_set_style_pad_all(box, 0, 0);
    lv_obj_set_scrollbar_mode(box, LV_SCROLLBAR_MODE_OFF);
    return box;
}

static lv_obj_t* make_label(lv_obj_t* parent, const char* text, uint32_t color, int x, int y)
{
    lv_obj_t* l = lv_label_create(parent);
    lv_label_set_text(l, text);
    lv_obj_set_style_text_color(l, lv_color_hex(color), 0);
    lv_obj_set_pos(l, x, y);
    return l;
}

bool ui_wifi_qr_show(void)
{
    auto* disp = Board::GetInstance().GetDisplay();
    if (disp == nullptr || disp->width() < 200 || disp->height() < 160) return false;

    auto& wifi = WifiManager::GetInstance();
    if (!wifi.IsConfigMode()) return false;

    std::string ssid = wifi.GetApSsid();
    std::string url  = wifi.GetApWebUrl();
    if (ssid.empty() || url.empty()) return false;

    char wifi_qr[160];
    snprintf(wifi_qr, sizeof(wifi_qr), "WIFI:T:nopass;S:%s;P:;;", ssid.c_str());

    if (qr_screen == nullptr) {
        qr_screen = lv_obj_create(nullptr);
    } else {
        lv_obj_clean(qr_screen);
    }
    lv_obj_set_style_bg_color(qr_screen, lv_color_hex(0x0B1220), 0);
    lv_obj_set_style_bg_opa(qr_screen, LV_OPA_COVER, 0);

    /* ===== 顶部标题栏（带边框） ===== */
    lv_obj_t* title_box = make_box(qr_screen, 4, 2, 312, 34, 0x38BDF8);
    make_label(title_box, "WiFi 配网模式", 0x38BDF8, 4, 3);
    make_label(title_box, "设备未联网，请按 1-2-3 操作", 0x94A3B8, 4, 17);

    /* ===== 左框：步骤 1 ===== */
    lv_obj_t* box1 = make_box(qr_screen, 4, 40, 152, 150, 0x2ECC71);
    make_label(box1, "1 扫码连热点", 0x2ECC71, 4, 2);
    draw_qr(box1, wifi_qr, 0, 30, 18);
    make_label(box1, ssid.c_str(), 0xF1C40F, 4, 116);
    make_label(box1, "（手机自动连上）", 0x64748B, 4, 130);

    /* ===== 右框：步骤 2 ===== */
    lv_obj_t* box2 = make_box(qr_screen, 164, 40, 152, 150, 0x2ECC71);
    make_label(box2, "2 扫码开配置页", 0x2ECC71, 4, 2);
    draw_qr(box2, url.c_str(), 1, 30, 18);
    make_label(box2, url.c_str(), 0xF1C40F, 4, 116);
    make_label(box2, "（手机浏览器打开）", 0x64748B, 4, 130);

    /* ===== 底部：步骤 3 + 说明 ===== */
    lv_obj_t* box3 = make_box(qr_screen, 4, 194, 312, 42, 0x475569);
    make_label(box3, "3 在配置页选你家WiFi并输密码", 0x2ECC71, 4, 2);
    make_label(box3, "   ota_url 留空！完成后自动连服务器，无需重刷", 0x94A3B8, 4, 14);
    make_label(box3, "   连按5次BOOT键可随时重新配网", 0x64748B, 4, 26);

    prev_screen = lv_screen_active();
    lv_screen_load(qr_screen);
    return true;
}

void ui_wifi_qr_hide(void)
{
    if (prev_screen != nullptr && prev_screen != qr_screen) {
        lv_screen_load(prev_screen);
    }
}
