#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_freertos_hooks.h"
#include "freertos/semphr.h"
#include "GUI.h"
#include "display_HAL.h"
#include "GUI_frontend.h"
#include "LVGL/lvgl.h"

/*********************
 *      DEFINES
 *********************/
#define LV_TICK_PERIOD_MS 10
#define DISP_BUF_SIZE   240*20 // Horizontal Res * 20 vertical pixels

/**********************
 *  STATIC PROTOTYPES
 **********************/
static lv_disp_drv_t disp_drv;

/**********************
 *  STATIC VARIABLES
 **********************/
static SemaphoreHandle_t xGuiSemaphore;

/**********************
*   GLOBAL FUNCTIONS
**********************/

void GUI_init(void){

    xGuiSemaphore = xSemaphoreCreateMutex();

    // LVGL Initialization
    lv_init();

    // Screen Buffer initialization - LVGL v8 API
    static EXT_RAM_ATTR lv_color_t buf1[DISP_BUF_SIZE];
    static lv_disp_draw_buf_t draw_buf;
    lv_disp_draw_buf_init(&draw_buf, buf1, NULL, DISP_BUF_SIZE);

    // Initialize LVGL display and attach the flush function
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = 240;
    disp_drv.ver_res = 240;
    disp_drv.flush_cb = display_HAL_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    // Note: With LV_TICK_CUSTOM enabled in lv_conf.h, we use esp_timer_get_time()
    // directly, so no need for lv_tick_inc() or a tick timer

    // Init menu graphic user interface
    GUI_frontend();
}

// Async refresh
void GUI_refresh(){
    for(int i = 0; i < 3; i++) lv_disp_flush_ready(&disp_drv);
}

void GUI_async_message(){
    async_battery_alert(false);
}

void GUI_task(void *arg){
    GUI_init();
    
    while (1) {
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    
    printf("delete\r\n");
    //A task should NEVER return
    vTaskDelete(NULL);
}