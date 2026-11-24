#ifndef _UI_DEMO_H_
#define _UI_DEMO_H_

#ifdef __cplusplus
extern "C" {
#endif

/* 初始化触摸测试菜单（独立屏幕，不影响系统 UI） */
void ui_demo_init(void);

/* 切换到测试菜单屏幕 */
void ui_demo_show(void);

/* 切回系统默认屏幕 */
void ui_demo_hide(void);

/* 更新状态文字 */
void ui_demo_set_text(const char* text);

#ifdef __cplusplus
}
#endif

#endif /* _UI_DEMO_H_ */
