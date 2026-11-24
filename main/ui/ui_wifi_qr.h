#ifndef _UI_WIFI_QR_H_
#define _UI_WIFI_QR_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 显示配网二维码屏幕（两个二维码）：
 *   左：WiFi 连接码  WIFI:T:WPA;S:<ssid>;P:;;
 *   右：配置页 URL    http://192.168.4.1
 *
 * @return true  = 已显示二维码
 *         false = 屏幕太小或未处于配网模式，调用方应回退到文字提示
 */
bool ui_wifi_qr_show(void);

/** 隐藏配网二维码屏幕（配网成功后调用） */
void ui_wifi_qr_hide(void);

#ifdef __cplusplus
}
#endif

#endif /* _UI_WIFI_QR_H_ */
