/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "esp_log.h"
#include "bsp/esp-bsp.h"
#include "lvgl.h"
#include "esp_lvgl_port.h"
#include "tinyusb.h"
#include "class/hid/hid_device.h"

static const char *TAG = "example";
#define TUSB_DESC_TOTAL_LEN      (TUD_CONFIG_DESC_LEN + CFG_TUD_HID * TUD_HID_DESC_LEN)

LV_FONT_DECLARE(symbols)

/*******************************************************************************
* Types definitions
*******************************************************************************/
typedef struct {
    const char * name;
    const void * image;
    uint8_t keycode[6];
} app_keymap_button_t;

/*******************************************************************************
* Variables
*******************************************************************************/

/* Count of the buttons */
#define APP_BUTTONS_ROWS    3
#define APP_BUTTONS_COLS    4

/*
 * Symbols from https://fontawesome.com/v5/search
 * Translate from unicode to UTF-8: https://www.utf8-chartable.de/unicode-utf8-table.pl?start=62060&number=1
*/
#define APP_SYMBOL_REC          "\xEF\x84\x91" /* 61713, 0xf111 */ 
#define APP_SYMBOL_MUTE         "\xEF\x9A\xA9" /* 63145, 0xf6a9 */ 
#define APP_SYMBOL_VIDEO        "\xEF\x80\xBD" /* 61501, 0xf03d */ 
#define APP_SYMBOL_VIDEO_SLASH  "\xEF\x93\xA2" /* 62690, 0xf4e2 */
#define APP_SYMBOL_TV           "\xEF\x89\xAC" /* 62060, 0xf26c */
#define APP_SYMBOL_MIC          "\xEF\x84\xB0" /* 61744, 0xf130 */
#define APP_SYMBOL_MIC_SLASH    "\xEF\x84\xB1" /* 61745, 0xf131 */
#define APP_SYMBOL_MIC1         "\xEF\x8F\x89" /* 62409, 0xf3c9 */
#define APP_SYMBOL_MIC1_SLASH   "\xEF\x94\xB9" /* 62777, 0xf539 */
#define APP_SYMBOL_HEADPHONES   "\xEF\x96\x8F" /* 62863, 0xf58f */
#define APP_SYMBOL_HEADPHONES1  "\xEF\x80\xA5" /* 61477, 0xf025 */
#define APP_SYMBOL_YOUTUBE      "\xEF\x85\xA7" /* 61799, 0xf167 */

/* Images */
LV_IMG_DECLARE(img_camera)

/* Definition of the buttons */
static const app_keymap_button_t app_buttons_matrix[APP_BUTTONS_ROWS][APP_BUTTONS_COLS] = {
    /* Row 1 */
    {
        /* TEXT,        IMAGE,      KEYS (max 6) */
        {"PLAY",    LV_SYMBOL_PLAY,    {HID_KEY_A}}, 
        {"STOP",    LV_SYMBOL_STOP,    {HID_KEY_B}}, 
        {"MIC",     APP_SYMBOL_MIC,    {HID_KEY_D}}, 
        {"MIC",     APP_SYMBOL_MIC_SLASH,{HID_KEY_D}}, 
    },
    /* Row 2 */
    {
        /* TEXT,        IMAGE,      KEYS (max 6) */
        {"MUTE",    APP_SYMBOL_MUTE,        {HID_KEY_1}}, 
        {"VOL-",    LV_SYMBOL_VOLUME_MID,   {HID_KEY_3}}, 
        {"VOL+",    LV_SYMBOL_VOLUME_MAX,   {HID_KEY_4}},
        {"SHUFFLE", LV_SYMBOL_SHUFFLE,      {HID_KEY_ALT_LEFT,HID_KEY_TAB}}, 
    },
    /* Row 3 */
    {
        /* TEXT,        IMAGE,      KEYS (max 6) */
        {"CAMERA 1", APP_SYMBOL_VIDEO,       {HID_KEY_ALT_LEFT,HID_KEY_CONTROL_LEFT,HID_KEY_1}}, 
        {"CAMERA 2", APP_SYMBOL_VIDEO_SLASH, {HID_KEY_ALT_LEFT,HID_KEY_CONTROL_LEFT,HID_KEY_2}}, 
        {"CAMERA 3", &img_camera,            {HID_KEY_ALT_LEFT,HID_KEY_CONTROL_LEFT,HID_KEY_3}},
        {"TV",       APP_SYMBOL_TV,          {HID_KEY_ALT_LEFT,HID_KEY_CONTROL_LEFT,HID_KEY_4}}
    },
};

/**
 * @brief String descriptor
 */
const char* hid_string_descriptor[5] = {
    (char[]){0x09, 0x04},   // 0: is supported language is English (0x0409)
    "Espressif",            // 1: Manufacturer
    "ESP Streaming Deck",   // 2: Product
    "123456",               // 3: Serials, should use chip ID
    "Streaming Deck",       // 4: HID
};

/**
 * @brief HID report descriptor
 *
 * In this example we implement Keyboard + Mouse HID device,
 * so we must define both report descriptors
 */
const uint8_t hid_report_descriptor[] = {
    TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(HID_ITF_PROTOCOL_KEYBOARD) )
};

/**
 * @brief Configuration descriptor
 *
 * This is a simple configuration descriptor that defines 1 configuration and 1 HID interface
 */
static const uint8_t hid_configuration_descriptor[] = {
    // Configuration number, interface count, string index, total length, attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(1, 1, 0, TUSB_DESC_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),

    // Interface number, string index, boot protocol, report descriptor len, EP In address, size & polling interval
    TUD_HID_DESCRIPTOR(0, 4, false, sizeof(hid_report_descriptor), 0x81, 16, 10),
};

static lv_disp_t *display;
/*******************************************************************************
* Private functions
*******************************************************************************/

// Invoked when received GET HID REPORT DESCRIPTOR request
// Application return pointer to descriptor, whose contents must exist long enough for transfer to complete
uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance)
{
    // We use only one interface and one HID report descriptor, so we can ignore parameter 'instance'
    return hid_report_descriptor;
}

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen)
{
  (void) instance;
  (void) report_id;
  (void) report_type;
  (void) buffer;
  (void) reqlen;

  return 0;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize)
{
}

static void btn_event_cb(lv_event_t * e)
{
    app_keymap_button_t * keybtn = (app_keymap_button_t *)lv_event_get_user_data(e);
    tud_hid_keyboard_report(HID_ITF_PROTOCOL_KEYBOARD, 0, keybtn->keycode);
    vTaskDelay(pdMS_TO_TICKS(50));
    tud_hid_keyboard_report(HID_ITF_PROTOCOL_KEYBOARD, 0, NULL);
}

static void app_lvgl_add_button(lv_obj_t *screen, const app_keymap_button_t * keybtn, uint16_t width, uint16_t height)
{
    assert(keybtn);

    lv_obj_t * btn = lv_btn_create(screen);
    lv_obj_set_size(btn, width, height);
    lv_obj_set_style_radius(btn, 0, 0);

    if (keybtn->image) {
        lv_obj_t * img = lv_img_create(btn);
	    lv_obj_set_style_text_font(img, &symbols, 0);
        lv_img_set_src(img, keybtn->image);
	    lv_obj_align(img, LV_ALIGN_CENTER, 0, 0);
    }

    lv_obj_t * lbl = lv_label_create(btn);
    if (keybtn->name) {
        lv_label_set_text_static(lbl, keybtn->name);
    } else {
        lv_label_set_text_static(lbl, "");
    }

	lv_obj_align(lbl, LV_ALIGN_BOTTOM_MID, 0, 0);

    lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_CLICKED, (void*)keybtn);
}

static void app_lvgl_show(void)
{
    lv_obj_t *scr = lv_scr_act();
    uint16_t btn_width = (BSP_LCD_H_RES / APP_BUTTONS_COLS)-15;
    uint16_t btn_height = (BSP_LCD_V_RES / APP_BUTTONS_ROWS)-15;

    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(scr, 255, 0);
    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(scr, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    for (int r=0; r<APP_BUTTONS_ROWS; r++) {
        for (int c=0; c<APP_BUTTONS_COLS; c++) {
            app_lvgl_add_button(scr, &app_buttons_matrix[r][c], btn_width, btn_height);
        }
    }
}

void app_main(void)
{    
    /* Initialize I2C (for touch and audio) */
    bsp_i2c_init();
    
    /* Initialize display and LVGL */
    display = bsp_display_start();

    /* Set display brightness to 100% */
    bsp_display_backlight_on();

    ESP_LOGI(TAG, "USB initialization");
    const tinyusb_config_t tusb_cfg = {
        .device_descriptor = NULL,
        .string_descriptor = hid_string_descriptor,
        .string_descriptor_count = sizeof(hid_string_descriptor) / sizeof(hid_string_descriptor[0]),
        .external_phy = false,
        .configuration_descriptor = hid_configuration_descriptor,
    };
    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));
    ESP_LOGI(TAG, "USB initialization DONE");
    
    app_lvgl_show();

    ESP_LOGI(TAG, "Project on GitHub: https://github.com/espzav/ESP-Streaming-Deck");
}
