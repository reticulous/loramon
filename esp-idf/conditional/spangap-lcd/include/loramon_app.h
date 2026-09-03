/**
 * loramon_app.h — "LoRaMon": the on-device LoRa airtime/signal monitor as a
 * boot-registered Service (an LcdApp).
 *
 * Per-radio tabs and a row of window pills; one plot carrying both directions,
 * each frame a bar spanning its time-on-air placed at its power on a dBm axis.
 * The full description of what is drawn and why is at the top of
 * loramon_lcd.cpp.
 *
 * Source of truth is storage — the `lora.<n>.packets.<ms>` subtree iface-lora
 * publishes — so this app reads what the browser window reads. Compiled only
 * under conditional/spangap-lcd/, so it exists only when the lcd straddle is
 * staged (the `when:` gate on the straddle.yaml services entry).
 */
#pragma once

#include "lcd_app.h"   /* LcdApp (a Service) */
#include "lvgl.h"      /* lv_obj_t */

class LoraMonApp : public LcdApp {
public:
    LoraMonApp();
    void onCreate(lv_obj_t* root) override;
    void onShow() override;
    void onHide() override;
    void onClose() override;
};
