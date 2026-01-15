/*********************
 *      LIBRARIES
 *********************/
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"

#include "lvgl.h"

#include "GUI.h"
#include "system_manager.h"
#include "sd_storage.h"
#include "user_input.h"
#include "sound_driver.h"
#include "backlight_ctrl.h"
#include "battery.h"

/*********************
 *   ICONS IMAGES
 *********************/

/* Emulator lib icons */
#include "icons/emulators_icon.h"
#include "icons/gameboycolor_icon.h"
#include "icons/gameboy_icon.h"
#include "icons/nes_icon.h"
#include "icons/snes_icon.h"
#include "icons/sega_icon.h"
#include "icons/nogame_icon.h"
#include "icons/GG_icon.h"

/* Wi-Fi lib manager icons */
#include "icons/ext_application_icon.h"

/* Configuration icons */
#include "icons/settings_icon.h"

/*********************
 *      DEFINES
 *********************/


/**********************
 *  STATIC PROTOTYPES
 **********************/

// Tasks
static void user_input_task(lv_indev_drv_t * indev_drv, lv_indev_data_t * data);
static void battery_status_task(lv_timer_t * timer);

// Display resolution
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 135

// Emulators menu
static void emulators_menu(lv_obj_t * parent);
static void emulators_list_cb(lv_event_t * e);
static void emulator_list_event(lv_event_t * e);
static void game_execute_cb(lv_event_t * e);
static void game_menu_cb(lv_event_t * e);
static void msgbox_no_game_cb(lv_event_t * e);
static void msgbox_no_app_cb(lv_event_t * e);
static void game_list_cb(lv_event_t * e);

// On game menu
static void on_game_menu(void);
static void slider_volume_cb(lv_event_t * e);
static void slider_brightness_cb(lv_event_t * e);
static void list_game_menu_cb(lv_event_t * e);

// External app menu
static void external_app_menu(lv_obj_t * parent);
static void external_app_cb(lv_event_t * e);
static void app_execute_cb(lv_event_t * e);

// Configuration menu
void config_menu(lv_obj_t * parent);
static void configuration_cb(lv_event_t * e);
static void config_option_cb(lv_event_t * e);
static void mbox_config_cb(lv_event_t * e);
static void fw_update_cb(lv_event_t * e);

//Extra functions
static void GUI_theme_color(uint8_t color_selected);

/**********************
 *  GLOBAL VARIABLES
 **********************/
uint8_t tab_num = 0;

uint32_t btn_left_time = 0;
uint32_t btn_right_time = 0;
uint32_t btn_up_time = 0;
uint32_t btn_down_time = 0;
uint32_t btn_a_time = 0;
uint32_t btn_b_time = 0;
uint32_t btn_menu_time = 0;

#define bouncing_time 7 //MS to wait to avoid bouncing on button selection

uint8_t emulator_selected = 0x00;

bool sub_menu = false;

// Theme: 0 = light, 1 = dark
uint32_t GUI_THEME = 0;

struct BATTERY_STATUS management;


/**********************
 *  LVGL groups and objects
 **********************/

static lv_group_t * group_interact; // Group of interactive objects

static lv_indev_t * kb_indev;

// Notification bar container objects
static lv_obj_t * notification_cont;
static lv_obj_t * battery_bar;
static lv_obj_t * battery_label;
static lv_obj_t * WIFI_label;
static lv_obj_t * BT_label;
static lv_obj_t * SD_label;
static lv_obj_t * Charging_label;

// Main menu objects

static lv_obj_t * tab_main_menu;
static lv_obj_t * tab_emulators;
static lv_obj_t * tab_ext_app_manager;
static lv_obj_t * tab_bt_controller;
static lv_obj_t * tab_config;

// Emulators menu objects

static lv_obj_t * list_emulators_main;
static lv_obj_t * btn_emulator_lib;
static lv_obj_t * container_header_game_icon;
static lv_obj_t * list_game_emulator;
static lv_obj_t * list_game_options;
static lv_obj_t * mbox_game_options;
//Buttons of the game initialization list
static lv_obj_t * btn_game_new;
static lv_obj_t * btn_game_resume;
static lv_obj_t * btn_game_delete;

// On-game menu objects

static lv_obj_t * list_on_game;
static lv_obj_t * mbox_volume;
static lv_obj_t * mbox_brightness;

// External app menu objects

static lv_obj_t * btn_ext_app;
static lv_obj_t * list_external_app;


// Configuration menu objects

static lv_obj_t * config_btn;
static lv_obj_t * list_config;
static lv_obj_t * list_fw_update;
static lv_obj_t * mbox_about;
static lv_obj_t * mbox_color;

// Store the currently selected button text for callbacks
static char current_btn_text[256];
static char current_game_name[256];

static const char *TAG = "GUI_frontend";

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void GUI_frontend(void){

    group_interact = lv_group_create();

    //  Initalize keypad task to control user input.
    static lv_indev_drv_t kb_drv;  // Must be static to persist!
    lv_indev_drv_init(&kb_drv);
    kb_drv.type = LV_INDEV_TYPE_KEYPAD;
    kb_drv.read_cb = user_input_task;
    kb_indev = lv_indev_drv_register(&kb_drv);
    lv_indev_set_group(kb_indev, group_interact);

    /* Device State Bar */
    //This bar shows the battery status, if the SD card is attached
    // or if any wireless communication is active.
    notification_cont = lv_obj_create(lv_layer_top());
    lv_obj_set_size(notification_cont, 240, 30);
    lv_obj_align(notification_cont, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_clear_flag(notification_cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(notification_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(notification_cont, 0, 0);
    lv_obj_set_style_pad_all(notification_cont, 0, 0);

    /* Battery Bar - top right like original */
    battery_bar = lv_bar_create(notification_cont);
    lv_obj_set_style_bg_color(battery_bar, lv_color_hex(0x0CC62D), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(battery_bar, lv_color_hex(0x0CC62D), LV_PART_MAIN);
    lv_obj_set_style_border_color(battery_bar, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_border_width(battery_bar, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(battery_bar, 4, LV_PART_MAIN);
    lv_obj_set_style_radius(battery_bar, 4, LV_PART_INDICATOR);
    lv_obj_set_size(battery_bar, 50, 20);
    lv_obj_align(battery_bar, LV_ALIGN_RIGHT_MID, -5, 0);

    battery_label = lv_label_create(battery_bar);
    lv_obj_align(battery_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_font(battery_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(battery_label, lv_color_black(), 0);

    //To show the battery level before the first message arrives to the queue
    uint8_t battery_aux = battery_get_percentage();
    char battery_level[8];
    sprintf(battery_level, "%i", battery_aux);
    lv_label_set_text(battery_label, battery_level);
    lv_bar_set_value(battery_bar, battery_aux, LV_ANIM_OFF);

    // This timer checks every second if a new battery message is on the queue
    lv_timer_create(battery_status_task, 1000, NULL);

    /* SD card connected icon - top left */
    SD_label = lv_label_create(notification_cont);
    lv_label_set_text(SD_label, LV_SYMBOL_SD_CARD);
    lv_obj_align(SD_label, LV_ALIGN_LEFT_MID, 5, 0);
    // Check if is connected
    if(!sd_mounted()) lv_obj_add_flag(SD_label, LV_OBJ_FLAG_HIDDEN);

    //Wi-Fi Status Icon
    WIFI_label = lv_label_create(notification_cont);
    lv_label_set_text(WIFI_label, LV_SYMBOL_WIFI);
    lv_obj_align(WIFI_label, LV_ALIGN_LEFT_MID, 50, 0);
    lv_obj_add_flag(WIFI_label, LV_OBJ_FLAG_HIDDEN);

    //Bluetooth Status Icon
    BT_label = lv_label_create(notification_cont);
    lv_label_set_text(BT_label, LV_SYMBOL_BLUETOOTH);
    lv_obj_align(BT_label, LV_ALIGN_LEFT_MID, 75, 0);
    lv_obj_add_flag(BT_label, LV_OBJ_FLAG_HIDDEN);

    //Charging Status Icon
    Charging_label = lv_label_create(notification_cont);
    lv_label_set_text(Charging_label, LV_SYMBOL_CHARGE);
    lv_obj_align(Charging_label, LV_ALIGN_LEFT_MID, 190, 0);
    lv_obj_add_flag(Charging_label, LV_OBJ_FLAG_HIDDEN);


    /* Tab menu shows the different sub menu:
        - Emulator library.
        - Wi-Fi Library Manager.
        - Bluetooth controller.
        - Configuration
    */

    tab_main_menu = lv_tabview_create(lv_scr_act(), LV_DIR_TOP, 0);
    lv_obj_set_size(tab_main_menu, 240, 240);
    lv_obj_set_pos(tab_main_menu, 0, 0);
    lv_obj_set_style_pad_all(tab_main_menu, 0, 0);
    lv_obj_set_style_border_width(tab_main_menu, 0, 0);
    
    // Get the tab content area and remove its padding
    lv_obj_t * tab_content = lv_tabview_get_content(tab_main_menu);
    lv_obj_set_style_pad_all(tab_content, 0, 0);
    lv_obj_set_style_border_width(tab_content, 0, 0);

    tab_emulators = lv_tabview_add_tab(tab_main_menu, "Emulators");
    tab_ext_app_manager = lv_tabview_add_tab(tab_main_menu, "External Application");
    tab_config = lv_tabview_add_tab(tab_main_menu, "Configuration");

    // Remove padding and borders from tab content for fullscreen
    lv_obj_set_style_pad_all(tab_emulators, 0, 0);
    lv_obj_set_style_pad_all(tab_ext_app_manager, 0, 0);
    lv_obj_set_style_pad_all(tab_config, 0, 0);
    lv_obj_set_style_border_width(tab_emulators, 0, 0);
    lv_obj_set_style_border_width(tab_ext_app_manager, 0, 0);
    lv_obj_set_style_border_width(tab_config, 0, 0);

    //Set the saved color configuration
    uint8_t theme_color = system_get_config(SYS_GUI_COLOR);
    GUI_theme_color(theme_color);

    emulators_menu(tab_emulators);
    external_app_menu(tab_ext_app_manager);
    config_menu(tab_config);
}


/**********************
 *   STATIC FUNCTIONS
 **********************/

/* Emulator menu function */

static void emulators_menu(lv_obj_t * parent){
    // Creation of the button with image on the main menu.

    btn_emulator_lib = lv_btn_create(parent);
    lv_obj_align(btn_emulator_lib, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_size(btn_emulator_lib, 200, 200);
    
    // Make button circular with colored border like original
    lv_obj_set_style_radius(btn_emulator_lib, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(btn_emulator_lib, 4, 0);
    lv_obj_set_style_border_color(btn_emulator_lib, lv_palette_main(LV_PALETTE_PINK), 0);
    lv_obj_set_style_border_opa(btn_emulator_lib, LV_OPA_COVER, 0);

    // Set the image and the sub label
    lv_obj_t * console_image = lv_img_create(btn_emulator_lib);
    lv_img_set_src(console_image, &emulators_icon);
    lv_obj_align(console_image, LV_ALIGN_CENTER, 0, -10);
    
    lv_obj_t * label = lv_label_create(btn_emulator_lib);
    lv_label_set_text(label, "Emulators");
    lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, -20);

    lv_obj_add_event_cb(btn_emulator_lib, emulators_list_cb, LV_EVENT_ALL, NULL);

    // Add to the available objects group to manipulate it.
    lv_group_add_obj(group_interact, btn_emulator_lib);
}

static void emulators_game_menu(char * game_name){
    // Placeholder for future implementation
}

/* On game menu function */

static void on_game_menu(void){
    sub_menu = true;

    list_on_game = lv_list_create(lv_layer_top());
    lv_obj_set_size(list_on_game, 240, 240);
    lv_obj_align(list_on_game, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t * list_btn;

    list_btn = lv_list_add_btn(list_on_game, LV_SYMBOL_HOME, "Resume Game");
    lv_obj_add_event_cb(list_btn, list_game_menu_cb, LV_EVENT_ALL, NULL);

    list_btn = lv_list_add_btn(list_on_game, LV_SYMBOL_SAVE, "Save Game");
    lv_obj_add_event_cb(list_btn, list_game_menu_cb, LV_EVENT_ALL, NULL);

    list_btn = lv_list_add_btn(list_on_game, LV_SYMBOL_VOLUME_MAX, "Volume");
    lv_obj_add_event_cb(list_btn, list_game_menu_cb, LV_EVENT_ALL, NULL);

    list_btn = lv_list_add_btn(list_on_game, LV_SYMBOL_IMAGE, "Brightness");
    lv_obj_add_event_cb(list_btn, list_game_menu_cb, LV_EVENT_ALL, NULL);

    list_btn = lv_list_add_btn(list_on_game, LV_SYMBOL_CLOSE, "Exit");
    lv_obj_add_event_cb(list_btn, list_game_menu_cb, LV_EVENT_ALL, NULL);

    lv_group_add_obj(group_interact, list_on_game);
    lv_group_focus_obj(list_on_game);
}


/* Wi-Fi library manager function */

static void external_app_menu(lv_obj_t * parent){

    btn_ext_app = lv_btn_create(parent);
    lv_obj_align(btn_ext_app, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_size(btn_ext_app, 200, 200);
    
    // Make button circular with colored border
    lv_obj_set_style_radius(btn_ext_app, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(btn_ext_app, 4, 0);
    lv_obj_set_style_border_color(btn_ext_app, lv_palette_main(LV_PALETTE_PINK), 0);
    lv_obj_set_style_border_opa(btn_ext_app, LV_OPA_COVER, 0);

    lv_obj_t * wifi_lib_image = lv_img_create(btn_ext_app);
    lv_img_set_src(wifi_lib_image, &ext_application_icon);
    lv_obj_align(wifi_lib_image, LV_ALIGN_CENTER, 0, -10);

    lv_obj_t * label = lv_label_create(btn_ext_app);
    lv_label_set_text(label, "External Apps");
    lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(label, 160);
    lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, -20);

    lv_obj_add_event_cb(btn_ext_app, external_app_cb, LV_EVENT_ALL, NULL);
    lv_group_add_obj(group_interact, btn_ext_app);
}

/* System Configuration main function */

void config_menu(lv_obj_t * parent){

    config_btn = lv_btn_create(parent);
    lv_obj_align(config_btn, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_size(config_btn, 200, 200);
    
    // Make button circular with colored border
    lv_obj_set_style_radius(config_btn, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(config_btn, 4, 0);
    lv_obj_set_style_border_color(config_btn, lv_palette_main(LV_PALETTE_PINK), 0);
    lv_obj_set_style_border_opa(config_btn, LV_OPA_COVER, 0);
    
    lv_obj_t * wifi_lib_image = lv_img_create(config_btn);
    lv_img_set_src(wifi_lib_image, &settings_icon);
    lv_obj_align(wifi_lib_image, LV_ALIGN_CENTER, 0, -10);
    
    lv_obj_t * label = lv_label_create(config_btn);
    lv_label_set_text(label, "Configuration");
    lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, -20);

    lv_obj_add_event_cb(config_btn, configuration_cb, LV_EVENT_ALL, NULL);
    lv_group_add_obj(group_interact, config_btn);
}


/**********************
 *  CALL-BACK FUNCTIONS
 **********************/

// Helper function to get button text from list button in v8
static const char * get_list_btn_text(lv_obj_t * btn) {
    lv_obj_t * label = lv_obj_get_child(btn, 0);
    if(label == NULL) {
        label = lv_obj_get_child(btn, 1);  // Sometimes label is second child after icon
    }
    if(label && lv_obj_check_type(label, &lv_label_class)) {
        return lv_label_get_text(label);
    }
    // Try finding label in children
    for(int i = 0; i < (int)lv_obj_get_child_cnt(btn); i++) {
        lv_obj_t * child = lv_obj_get_child(btn, i);
        if(lv_obj_check_type(child, &lv_label_class)) {
            return lv_label_get_text(child);
        }
    }
    return "";
}

/* Emulator menu call-back functions */

static void game_list_cb(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);

    // Get the list of available games for each console and show a header with the icon of the console
    if(code == LV_EVENT_CLICKED){
        const char * btn_text = get_list_btn_text(obj);

        // Remove emulator buttons from group before proceeding
        uint32_t child_cnt = lv_obj_get_child_cnt(list_emulators_main);
        for(uint32_t i = 0; i < child_cnt; i++) {
            lv_obj_t * child = lv_obj_get_child(list_emulators_main, i);
            lv_group_remove_obj(child);
        }

        container_header_game_icon = lv_obj_create(lv_layer_top());
        lv_obj_align(container_header_game_icon, LV_ALIGN_TOP_LEFT, 0, 0);
        lv_obj_set_size(container_header_game_icon, 240, 40);
        lv_obj_clear_flag(container_header_game_icon, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t * console_label = lv_label_create(container_header_game_icon);
        lv_obj_t * console_image = lv_img_create(container_header_game_icon);

        lv_obj_set_style_text_font(console_label, &lv_font_montserrat_20, 0);

        // Check which console was selected and create the title
        if(strcmp(btn_text, "NES") == 0){
            emulator_selected = NES;
            lv_label_set_text(console_label, "NES");
            lv_obj_align(console_label, LV_ALIGN_CENTER, 25, 0);
            lv_img_set_src(console_image, &NES_icon);
            lv_obj_align(console_image, LV_ALIGN_CENTER, -40, 0);
            lv_obj_add_flag(list_emulators_main, LV_OBJ_FLAG_HIDDEN);
            ESP_LOGI(TAG, "Selected NES");
        }
        else if(strcmp(btn_text, "SNES") == 0){
            emulator_selected = SNES;
            lv_label_set_text(console_label, "SNES");
            lv_obj_align(console_label, LV_ALIGN_CENTER, 25, 0);
            lv_img_set_src(console_image, &snes_icon);
            lv_obj_align(console_image, LV_ALIGN_CENTER, -40, 0);
            lv_obj_add_flag(list_emulators_main, LV_OBJ_FLAG_HIDDEN);
            ESP_LOGI(TAG, "Selected SNES");
        }
        else if(strcmp(btn_text, "GameBoy") == 0){
            emulator_selected = GAMEBOY;
            lv_label_set_text(console_label, "GameBoy");
            lv_obj_align(console_label, LV_ALIGN_CENTER, 25, 0);
            lv_img_set_src(console_image, &gameboy_icon);
            lv_obj_align(console_image, LV_ALIGN_CENTER, -40, 0);
            lv_obj_add_flag(list_emulators_main, LV_OBJ_FLAG_HIDDEN);
            ESP_LOGI(TAG, "Selected GameBoy");
        }
        else if(strcmp(btn_text, "GameBoy Color") == 0){
            emulator_selected = GAMEBOY_COLOR;
            lv_label_set_text(console_label, "GameBoy Color");
            lv_obj_align(console_label, LV_ALIGN_CENTER, 20, 0);
            lv_img_set_src(console_image, &gameboycolor_icon);
            lv_obj_align(console_image, LV_ALIGN_CENTER, -75, 0);
            lv_obj_add_flag(list_emulators_main, LV_OBJ_FLAG_HIDDEN);
            ESP_LOGI(TAG, "Selected GameBoy Color");
        }
        else if(strcmp(btn_text, "Master System") == 0){
            emulator_selected = SMS;
            lv_label_set_text(console_label, "Master System");
            lv_obj_align(console_label, LV_ALIGN_CENTER, 25, 0);
            lv_img_set_src(console_image, &sega_icon);
            lv_obj_align(console_image, LV_ALIGN_CENTER, -75, 0);
            lv_obj_add_flag(list_emulators_main, LV_OBJ_FLAG_HIDDEN);
            ESP_LOGI(TAG, "Selected Sega MasterSystem");
        }
        else if(strcmp(btn_text, "Game Gear") == 0){
            emulator_selected = GG;
            lv_label_set_text(console_label, "Game Gear");
            lv_obj_align(console_label, LV_ALIGN_CENTER, 25, 0);
            lv_img_set_src(console_image, &GG_icon);
            lv_obj_align(console_image, LV_ALIGN_CENTER, -75, 0);
            lv_obj_add_flag(list_emulators_main, LV_OBJ_FLAG_HIDDEN);
            ESP_LOGI(TAG, "Selected Sega Game Gear");
        }

        // Get the game list of each console.
        char *game_list[100];
        uint8_t games_num = sd_game_list(game_list, emulator_selected);

        ESP_LOGI(TAG, "Found %i games", games_num);

        // Print the list of games or show a message if any game is available
        if(games_num > 0){
            list_game_emulator = lv_list_create(lv_layer_top());
            lv_obj_set_size(list_game_emulator, 220, 200);
            lv_obj_align(list_game_emulator, LV_ALIGN_CENTER, 0, 20);

            lv_obj_t * first_btn = NULL;

            // Add a button for each game
            for(int i = 0; i < games_num; i++){
                lv_obj_t * game_btn = lv_list_add_btn(list_game_emulator, NULL, game_list[i]);
                lv_obj_set_height(game_btn, 40);
                lv_obj_add_event_cb(game_btn, game_menu_cb, LV_EVENT_ALL, NULL);
                lv_group_add_obj(group_interact, game_btn);
                if(i == 0) first_btn = game_btn;
                free(game_list[i]);
            }

            if(first_btn) lv_group_focus_obj(first_btn);
        }
        else{
            lv_obj_del(container_header_game_icon);

            // Remove emulator buttons from group while showing popup
            uint32_t child_cnt = lv_obj_get_child_cnt(list_emulators_main);
            for(uint32_t i = 0; i < child_cnt; i++) {
                lv_obj_t * child = lv_obj_get_child(list_emulators_main, i);
                lv_group_remove_obj(child);
            }

            // Create simple message box
            lv_obj_t * mbox = lv_obj_create(lv_layer_top());
            lv_obj_set_size(mbox, 200, 150);
            lv_obj_align(mbox, LV_ALIGN_CENTER, 0, 0);

            lv_obj_t * label = lv_label_create(mbox);
            lv_label_set_text(label, "Oops! No games available.");
            lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 10);

            lv_obj_t * nogame_image = lv_img_create(mbox);
            lv_img_set_src(nogame_image, &nogame_icon);
            lv_obj_align(nogame_image, LV_ALIGN_CENTER, 0, 10);

            lv_obj_add_event_cb(mbox, msgbox_no_game_cb, LV_EVENT_ALL, NULL);
            lv_group_add_obj(group_interact, mbox);
            lv_group_focus_obj(mbox);

            lv_obj_set_style_bg_opa(lv_layer_top(), LV_OPA_70, 0);
            lv_obj_set_style_bg_color(lv_layer_top(), lv_color_hex(0x808080), 0);
            lv_obj_add_flag(lv_layer_top(), LV_OBJ_FLAG_CLICKABLE);
        }
    }
    else if(code == LV_EVENT_CANCEL){
        // This is called when canceling from the emulator list
        sub_menu = false;
        // Remove all children from group before deleting
        uint32_t child_cnt = lv_obj_get_child_cnt(list_emulators_main);
        for(uint32_t i = 0; i < child_cnt; i++) {
            lv_obj_t * child = lv_obj_get_child(list_emulators_main, i);
            lv_group_remove_obj(child);
        }
        lv_obj_del(list_emulators_main);

        // Restore main menu buttons to group
        lv_group_add_obj(group_interact, btn_emulator_lib);
        lv_group_add_obj(group_interact, btn_ext_app);
        lv_group_add_obj(group_interact, config_btn);
        lv_group_focus_obj(btn_emulator_lib);
    }
}

static void msgbox_no_game_cb(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);

    if(code == LV_EVENT_CANCEL) {
        // Remove mbox from group
        lv_group_remove_obj(obj);

        // Delete the mbox
        lv_obj_del(obj);

        // Clear the layer top styling
        lv_obj_set_style_bg_opa(lv_layer_top(), LV_OPA_TRANSP, 0);
        lv_obj_clear_flag(lv_layer_top(), LV_OBJ_FLAG_CLICKABLE);

        // Show emulator list again and re-add buttons to group
        lv_obj_clear_flag(list_emulators_main, LV_OBJ_FLAG_HIDDEN);
        uint32_t child_cnt = lv_obj_get_child_cnt(list_emulators_main);
        lv_obj_t * first_btn = NULL;
        for(uint32_t i = 0; i < child_cnt; i++) {
            lv_obj_t * child = lv_obj_get_child(list_emulators_main, i);
            lv_group_add_obj(group_interact, child);
            if(i == 0) first_btn = child;
        }
        if(first_btn) lv_group_focus_obj(first_btn);
    }
}

static void msgbox_no_app_cb(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);

    if(code == LV_EVENT_CANCEL) {
        // Remove mbox from group
        lv_group_remove_obj(obj);

        // Delete the mbox
        lv_obj_del(obj);

        // Clear the layer top styling
        lv_obj_set_style_bg_opa(lv_layer_top(), LV_OPA_TRANSP, 0);
        lv_obj_clear_flag(lv_layer_top(), LV_OBJ_FLAG_CLICKABLE);

        // Focus back on external app button
        lv_group_focus_obj(btn_ext_app);
    }
}

// Cancel handler for emulators list
static void emulators_list_cancel_cb(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_CANCEL){
        sub_menu = false;
        // Remove all children from group before deleting
        uint32_t child_cnt = lv_obj_get_child_cnt(list_emulators_main);
        for(uint32_t i = 0; i < child_cnt; i++) {
            lv_obj_t * child = lv_obj_get_child(list_emulators_main, i);
            lv_group_remove_obj(child);
        }
        lv_obj_del(list_emulators_main);

        // Restore main menu buttons to group
        lv_group_add_obj(group_interact, btn_emulator_lib);
        lv_group_add_obj(group_interact, btn_ext_app);
        lv_group_add_obj(group_interact, config_btn);
        lv_group_focus_obj(btn_emulator_lib);
    }
}

static void emulators_list_cb(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);

    // Create a list with the available emulators
    if(code == LV_EVENT_CLICKED){
        sub_menu = true;

        // Remove main menu buttons from group to prevent wrap-around navigation
        lv_group_remove_obj(btn_emulator_lib);
        lv_group_remove_obj(btn_ext_app);
        lv_group_remove_obj(config_btn);

        list_emulators_main = lv_list_create(lv_layer_top());
        lv_obj_set_size(list_emulators_main, 240, 240);
        lv_obj_align(list_emulators_main, LV_ALIGN_CENTER, 0, 0);
        // Make list opaque to cover main menu
        lv_obj_set_style_bg_opa(list_emulators_main, LV_OPA_COVER, 0);

        lv_obj_t * emulator_NES_btn = lv_list_add_btn(list_emulators_main, &NES_icon, "NES");
        lv_obj_set_height(emulator_NES_btn, 40);
        lv_obj_add_event_cb(emulator_NES_btn, game_list_cb, LV_EVENT_ALL, NULL);
        lv_group_add_obj(group_interact, emulator_NES_btn);

        lv_obj_t * emulator_GB_btn = lv_list_add_btn(list_emulators_main, &gameboy_icon, "GameBoy");
        lv_obj_set_height(emulator_GB_btn, 40);
        lv_obj_add_event_cb(emulator_GB_btn, game_list_cb, LV_EVENT_ALL, NULL);
        lv_group_add_obj(group_interact, emulator_GB_btn);

        lv_obj_t * emulator_GBC_btn = lv_list_add_btn(list_emulators_main, &gameboycolor_icon, "GameBoy Color");
        lv_obj_set_height(emulator_GBC_btn, 40);
        lv_obj_add_event_cb(emulator_GBC_btn, game_list_cb, LV_EVENT_ALL, NULL);
        lv_group_add_obj(group_interact, emulator_GBC_btn);

        lv_obj_t * emulator_SMS_btn = lv_list_add_btn(list_emulators_main, &sega_icon, "Master System");
        lv_obj_set_height(emulator_SMS_btn, 40);
        lv_obj_add_event_cb(emulator_SMS_btn, game_list_cb, LV_EVENT_ALL, NULL);
        lv_group_add_obj(group_interact, emulator_SMS_btn);

        lv_obj_t * emulator_GG_btn = lv_list_add_btn(list_emulators_main, &GG_icon, "Game Gear");
        lv_obj_set_height(emulator_GG_btn, 40);
        lv_obj_add_event_cb(emulator_GG_btn, game_list_cb, LV_EVENT_ALL, NULL);
        lv_group_add_obj(group_interact, emulator_GG_btn);

        // Add cancel handler to the list itself
        lv_obj_add_event_cb(list_emulators_main, emulators_list_cancel_cb, LV_EVENT_CANCEL, NULL);

        lv_group_focus_obj(emulator_NES_btn);
    }
}

static void game_menu_cb(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);

    if(code == LV_EVENT_CLICKED) {
        const char * game_name = get_list_btn_text(obj);
        strncpy(current_game_name, game_name, sizeof(current_game_name) - 1);

        lv_obj_add_flag(container_header_game_icon, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(list_emulators_main, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(list_game_emulator, LV_OBJ_FLAG_HIDDEN);

        // Create game options container
        mbox_game_options = lv_obj_create(lv_layer_top());
        lv_obj_set_size(mbox_game_options, 230, 200);
        lv_obj_align(mbox_game_options, LV_ALIGN_CENTER, 0, 0);

        lv_obj_t * title_label = lv_label_create(mbox_game_options);
        lv_label_set_text(title_label, current_game_name);
        lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 5);
        lv_label_set_long_mode(title_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_set_width(title_label, 210);

        lv_obj_add_flag(lv_layer_top(), LV_OBJ_FLAG_CLICKABLE);

        list_game_options = lv_list_create(mbox_game_options);
        lv_obj_set_size(list_game_options, 210, 140);
        lv_obj_align(list_game_options, LV_ALIGN_BOTTOM_MID, 0, -5);

        btn_game_new = lv_list_add_btn(list_game_options, LV_SYMBOL_HOME, "New Game");
        lv_obj_set_height(btn_game_new, 35);
        lv_obj_add_event_cb(btn_game_new, game_execute_cb, LV_EVENT_ALL, NULL);
        lv_group_add_obj(group_interact, btn_game_new);

        if(sd_sav_exist((char *)game_name, emulator_selected)){
            btn_game_resume = lv_list_add_btn(list_game_options, LV_SYMBOL_SAVE, "Resume Game");
            lv_obj_set_height(btn_game_resume, 35);
            lv_obj_add_event_cb(btn_game_resume, game_execute_cb, LV_EVENT_ALL, NULL);
            lv_group_add_obj(group_interact, btn_game_resume);

            btn_game_delete = lv_list_add_btn(list_game_options, LV_SYMBOL_CLOSE, "Delete Save Data");
            lv_obj_set_height(btn_game_delete, 35);
            lv_obj_add_event_cb(btn_game_delete, game_execute_cb, LV_EVENT_ALL, NULL);
            lv_group_add_obj(group_interact, btn_game_delete);
        }

        lv_group_focus_obj(btn_game_new);
    }
    else if(code == LV_EVENT_CANCEL){
        // Remove game buttons from group before deleting
        uint32_t child_cnt = lv_obj_get_child_cnt(list_game_emulator);
        for(uint32_t i = 0; i < child_cnt; i++) {
            lv_obj_t * child = lv_obj_get_child(list_game_emulator, i);
            lv_group_remove_obj(child);
        }

        // Delete the list of games and the header icon
        lv_obj_del(container_header_game_icon);
        lv_obj_del(list_game_emulator);

        // Show emulator list again and re-add buttons to group
        lv_obj_clear_flag(list_emulators_main, LV_OBJ_FLAG_HIDDEN);
        child_cnt = lv_obj_get_child_cnt(list_emulators_main);
        lv_obj_t * first_btn = NULL;
        for(uint32_t i = 0; i < child_cnt; i++) {
            lv_obj_t * child = lv_obj_get_child(list_emulators_main, i);
            lv_group_add_obj(group_interact, child);
            if(i == 0) first_btn = child;
        }
        if(first_btn) lv_group_focus_obj(first_btn);
    }
}

static void game_execute_cb(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);

    if(code == LV_EVENT_CLICKED) {
        struct SYSTEM_MODE emulator;
        const char * btn_text = get_list_btn_text(obj);

        if(strcmp(btn_text, "New Game") == 0){
            ESP_LOGI(TAG, "Loading: %s", current_game_name);

            emulator.mode = MODE_GAME;
            emulator.load_save_game = 0;
            emulator.status = 1;
            emulator.console = emulator_selected;
            strcpy(emulator.game_name, current_game_name);

            if(xQueueSend(modeQueue, &emulator, (TickType_t)10) != pdPASS){
                ESP_LOGE(TAG, "Error sending game execution queue");
            }

            on_game_menu();
        }
        else if(strcmp(btn_text, "Resume Game") == 0){
            ESP_LOGI(TAG, "Loading: %s", current_game_name);

            emulator.mode = MODE_GAME;
            emulator.load_save_game = 1;
            emulator.status = 1;
            emulator.console = emulator_selected;
            strcpy(emulator.game_name, current_game_name);

            if(xQueueSend(modeQueue, &emulator, (TickType_t)10) != pdPASS){
                ESP_LOGE(TAG, "Error sending game execution queue");
            }
            on_game_menu();
        }
        else if(strcmp(btn_text, "Delete Save Data") == 0){
            sd_sav_remove(current_game_name, emulator_selected);
            //Update the list of options
            lv_obj_del(btn_game_delete);
            lv_obj_del(btn_game_resume);
        }
    }
    else if(code == LV_EVENT_CANCEL) {
        // Remove game option buttons from group
        lv_group_remove_obj(btn_game_new);
        if(btn_game_resume) lv_group_remove_obj(btn_game_resume);
        if(btn_game_delete) lv_group_remove_obj(btn_game_delete);

        // Show hidden elements again
        lv_obj_clear_flag(container_header_game_icon, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(list_emulators_main, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(list_game_emulator, LV_OBJ_FLAG_HIDDEN);

        lv_obj_del(mbox_game_options);

        // Re-add game buttons to group and focus
        uint32_t child_cnt = lv_obj_get_child_cnt(list_game_emulator);
        lv_obj_t * first_btn = NULL;
        for(uint32_t i = 0; i < child_cnt; i++) {
            lv_obj_t * child = lv_obj_get_child(list_game_emulator, i);
            lv_group_add_obj(group_interact, child);
            if(i == 0) first_btn = child;
        }
        if(first_btn) lv_group_focus_obj(first_btn);
    }
}


/* On game menu call-back functions */

static void slider_volume_cb(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * slider = lv_event_get_target(e);

    if(code == LV_EVENT_VALUE_CHANGED) {
        ESP_LOGI(TAG, "Volumen set: %i", (int)lv_slider_get_value(slider));
        audio_volume_set(lv_slider_get_value(slider));
    }
    else if(code == LV_EVENT_CANCEL){
        lv_obj_del(mbox_volume);
    }
}

static void slider_brightness_cb(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * slider = lv_event_get_target(e);

    if(code == LV_EVENT_VALUE_CHANGED) {
        ESP_LOGI(TAG, "Brightness set: %i", (int)lv_slider_get_value(slider));
        backlight_set(lv_slider_get_value(slider));
    }
    else if(code == LV_EVENT_CANCEL){
        lv_obj_del(mbox_brightness);
    }
}

static void list_game_menu_cb(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);

    if(code == LV_EVENT_CLICKED){
        struct SYSTEM_MODE emulator;
        const char * btn_text = get_list_btn_text(obj);

        if(strcmp(btn_text, "Resume Game") == 0){
            printf("Resume game\r\n");
            emulator.mode = MODE_GAME;
            emulator.status = 0;

            if(xQueueSend(modeQueue, &emulator, (TickType_t)10) != pdPASS){
                ESP_LOGE(TAG, "modeQueue send error");
            }
        }
        else if(strcmp(btn_text, "Save Game") == 0){
            emulator.mode = MODE_SAVE_GAME;
            emulator.console = emulator_selected;

            if(xQueueSend(modeQueue, &emulator, (TickType_t)10) != pdPASS){
                ESP_LOGE(TAG, "modeQueue send error");
            }
        }
        else if(strcmp(btn_text, "Volume") == 0){
            mbox_volume = lv_obj_create(lv_layer_top());
            lv_obj_set_size(mbox_volume, 220, 100);
            lv_obj_align(mbox_volume, LV_ALIGN_CENTER, 0, 0);

            lv_obj_t * title = lv_label_create(mbox_volume);
            lv_label_set_text(title, "Volume");
            lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 5);

            lv_group_add_obj(group_interact, mbox_volume);
            lv_group_focus_obj(mbox_volume);

            lv_obj_set_style_bg_opa(lv_layer_top(), LV_OPA_70, 0);
            lv_obj_set_style_bg_color(lv_layer_top(), lv_color_hex(0x808080), 0);
            lv_obj_add_flag(lv_layer_top(), LV_OBJ_FLAG_CLICKABLE);

            lv_obj_t * slider = lv_slider_create(mbox_volume);
            lv_obj_align(slider, LV_ALIGN_CENTER, 0, 10);
            lv_obj_set_width(slider, 180);
            lv_slider_set_range(slider, 0, 100);
            // Get volume
            uint8_t volume = audio_volume_get();
            lv_slider_set_value(slider, volume, LV_ANIM_OFF);

            lv_obj_add_event_cb(slider, slider_volume_cb, LV_EVENT_ALL, NULL);

            lv_group_add_obj(group_interact, slider);
            lv_group_focus_obj(slider);
        }
        else if(strcmp(btn_text, "Brightness") == 0){
            mbox_brightness = lv_obj_create(lv_layer_top());
            lv_obj_set_size(mbox_brightness, 220, 100);
            lv_obj_align(mbox_brightness, LV_ALIGN_CENTER, 0, 0);

            lv_obj_t * title = lv_label_create(mbox_brightness);
            lv_label_set_text(title, "Brightness");
            lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 5);

            lv_group_add_obj(group_interact, mbox_brightness);
            lv_group_focus_obj(mbox_brightness);

            lv_obj_set_style_bg_opa(lv_layer_top(), LV_OPA_70, 0);
            lv_obj_set_style_bg_color(lv_layer_top(), lv_color_hex(0x808080), 0);
            lv_obj_add_flag(lv_layer_top(), LV_OBJ_FLAG_CLICKABLE);

            lv_obj_t * slider = lv_slider_create(mbox_brightness);
            lv_obj_align(slider, LV_ALIGN_CENTER, 0, 10);
            lv_obj_set_width(slider, 180);
            lv_slider_set_range(slider, 0, 100);

            uint8_t brightness_level = backlight_get();
            lv_slider_set_value(slider, brightness_level, LV_ANIM_ON);

            lv_obj_add_event_cb(slider, slider_brightness_cb, LV_EVENT_ALL, NULL);

            lv_group_add_obj(group_interact, slider);
            lv_group_focus_obj(slider);
        }
        else if(strcmp(btn_text, "Exit") == 0){
            emulator.mode = MODE_OUT;

            if(xQueueSend(modeQueue, &emulator, (TickType_t)10) != pdPASS){
                ESP_LOGE(TAG, "modeQueue send error");
            }
        }
    }
    else if(code == LV_EVENT_CANCEL){
        // Pushed button B, so we want to close the menu
        struct SYSTEM_MODE emulator;
        emulator.mode = MODE_GAME;
        emulator.status = 0;

        if(xQueueSend(modeQueue, &emulator, (TickType_t)10) != pdPASS){
            ESP_LOGE(TAG, "SYSTEM Mode queue send error");
        }
    }
}


/* External app call-back functions */

static void external_app_cb(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_CLICKED){
        char *app_list[100];
        uint8_t app_num = sd_app_list(app_list, false);

        ESP_LOGI(TAG, "Found %i applications", app_num);

        if(app_num > 0){
            //Create a list of applications
            sub_menu = true;
            list_external_app = lv_list_create(lv_layer_top());
            lv_obj_set_size(list_external_app, 210, 200);
            lv_obj_align(list_external_app, LV_ALIGN_CENTER, 0, 10);

            // Add a button for each app
            for(int i = 0; i < app_num; i++){
                lv_obj_t * app_btn = lv_list_add_btn(list_external_app, NULL, app_list[i]);
                lv_group_add_obj(group_interact, app_btn);
                lv_obj_add_event_cb(app_btn, app_execute_cb, LV_EVENT_ALL, NULL);
                free(app_list[i]);
            }
            lv_group_add_obj(group_interact, list_external_app);
            lv_group_focus_obj(list_external_app);
        }
        else{
            // Show a message
            lv_obj_t * mbox = lv_obj_create(lv_layer_top());
            lv_obj_set_size(mbox, 200, 150);
            lv_obj_align(mbox, LV_ALIGN_CENTER, 0, 0);

            lv_obj_t * label = lv_label_create(mbox);
            lv_label_set_text(label, "No apps available.");
            lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 10);

            lv_obj_t * nogame_image = lv_img_create(mbox);
            lv_img_set_src(nogame_image, &nogame_icon);
            lv_obj_align(nogame_image, LV_ALIGN_CENTER, 0, 10);

            lv_obj_add_event_cb(mbox, msgbox_no_app_cb, LV_EVENT_ALL, NULL);
            lv_group_add_obj(group_interact, mbox);
            lv_group_focus_obj(mbox);

            lv_obj_set_style_bg_opa(lv_layer_top(), LV_OPA_70, 0);
            lv_obj_set_style_bg_color(lv_layer_top(), lv_color_hex(0x808080), 0);
            lv_obj_add_flag(lv_layer_top(), LV_OBJ_FLAG_CLICKABLE);
        }
    }
}

static void app_execute_cb(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);

    if(code == LV_EVENT_CLICKED){
        const char * app_name = get_list_btn_text(obj);
        ESP_LOGI(TAG, "Loading: %s", app_name);

        lv_obj_t * mbox = lv_obj_create(lv_layer_top());
        lv_obj_set_size(mbox, 200, 150);
        lv_obj_align(mbox, LV_ALIGN_CENTER, 0, -60);

        lv_obj_t * title = lv_label_create(mbox);
        lv_label_set_text(title, "Loading:");
        lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 5);

        lv_obj_t * label1 = lv_label_create(mbox);
        lv_label_set_text(label1, app_name);
        lv_obj_align(label1, LV_ALIGN_TOP_MID, 0, 25);

        lv_obj_set_style_bg_opa(lv_layer_top(), LV_OPA_70, 0);
        lv_obj_set_style_bg_color(lv_layer_top(), lv_color_hex(0x808080), 0);
        lv_obj_add_flag(lv_layer_top(), LV_OBJ_FLAG_CLICKABLE);

        lv_group_focus_obj(mbox);
        lv_group_focus_freeze(group_interact, false);

        lv_obj_t * preload = lv_spinner_create(mbox, 1000, 60);
        lv_obj_set_size(preload, 80, 80);
        lv_obj_align(preload, LV_ALIGN_CENTER, 0, 20);

        struct SYSTEM_MODE ext_app;
        ext_app.mode = MODE_EXT_APP;
        strcpy(ext_app.game_name, app_name);

        if(xQueueSend(modeQueue, &ext_app, (TickType_t)10) != pdPASS){
            ESP_LOGE(TAG, " External app queue send fail");
        }

        lv_obj_del(list_external_app);
    }
    else if(code == LV_EVENT_CANCEL){
        sub_menu = false;
        lv_obj_del(list_external_app);
        lv_group_focus_obj(btn_ext_app);
    }
}

/* Configuration menu cb function */

static void configuration_cb(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_CLICKED){
        sub_menu = true;

        // Remove main menu buttons from group to prevent wrap-around navigation
        lv_group_remove_obj(btn_emulator_lib);
        lv_group_remove_obj(btn_ext_app);
        lv_group_remove_obj(config_btn);

        list_config = lv_list_create(lv_layer_top());
        lv_obj_set_size(list_config, 240, 240);
        lv_obj_align(list_config, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_bg_opa(list_config, LV_OPA_COVER, 0);

        lv_obj_t * list_btn;
        lv_obj_t * first_btn = NULL;

        list_btn = lv_list_add_btn(list_config, LV_SYMBOL_SETTINGS, "About this device");
        lv_obj_set_height(list_btn, 40);
        lv_obj_add_event_cb(list_btn, config_option_cb, LV_EVENT_ALL, NULL);
        lv_group_add_obj(group_interact, list_btn);
        first_btn = list_btn;

        list_btn = lv_list_add_btn(list_config, LV_SYMBOL_DOWNLOAD, "Update firmware");
        lv_obj_set_height(list_btn, 40);
        lv_obj_add_event_cb(list_btn, config_option_cb, LV_EVENT_ALL, NULL);
        lv_group_add_obj(group_interact, list_btn);

        //System config options
        list_btn = lv_list_add_btn(list_config, LV_SYMBOL_EYE_OPEN, "Brightness");
        lv_obj_set_height(list_btn, 40);
        lv_obj_add_event_cb(list_btn, config_option_cb, LV_EVENT_ALL, NULL);
        lv_group_add_obj(group_interact, list_btn);

        list_btn = lv_list_add_btn(list_config, LV_SYMBOL_IMAGE, "GUI Color Mode");
        lv_obj_set_height(list_btn, 40);
        lv_obj_add_event_cb(list_btn, config_option_cb, LV_EVENT_ALL, NULL);
        lv_group_add_obj(group_interact, list_btn);

        list_btn = lv_list_add_btn(list_config, LV_SYMBOL_VOLUME_MAX, "Volume");
        lv_obj_set_height(list_btn, 40);
        lv_obj_add_event_cb(list_btn, config_option_cb, LV_EVENT_ALL, NULL);
        lv_group_add_obj(group_interact, list_btn);

        if(system_get_config(SYS_STATE_SAV_BTN) != 1)
            list_btn = lv_list_add_btn(list_config, LV_SYMBOL_SAVE, "Enable Button State Save");
        else
            list_btn = lv_list_add_btn(list_config, LV_SYMBOL_SAVE, "Disable Button State Save");
        lv_obj_set_height(list_btn, 40);
        lv_obj_add_event_cb(list_btn, config_option_cb, LV_EVENT_ALL, NULL);
        lv_group_add_obj(group_interact, list_btn);

        //System info options
        list_btn = lv_list_add_btn(list_config, LV_SYMBOL_BATTERY_FULL, "Battery Status");
        lv_obj_set_height(list_btn, 40);
        lv_obj_add_event_cb(list_btn, config_option_cb, LV_EVENT_ALL, NULL);
        lv_group_add_obj(group_interact, list_btn);

        list_btn = lv_list_add_btn(list_config, LV_SYMBOL_SD_CARD, "SD card Status");
        lv_obj_set_height(list_btn, 40);
        lv_obj_add_event_cb(list_btn, config_option_cb, LV_EVENT_ALL, NULL);
        lv_group_add_obj(group_interact, list_btn);

        lv_group_focus_obj(first_btn);
    }
}

static void config_option_cb(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);

    if(code == LV_EVENT_CLICKED){
        const char * btn_text = get_list_btn_text(obj);

        if(strcmp(btn_text, "About this device") == 0){
            mbox_about = lv_obj_create(lv_layer_top());
            lv_obj_set_size(mbox_about, 220, 160);
            lv_obj_align(mbox_about, LV_ALIGN_CENTER, 0, -30);

            lv_obj_t * title = lv_label_create(mbox_about);
            lv_label_set_text(title, "microByte");
            lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 5);

            char *aux_text = malloc(256);
            lv_obj_t * label_hw_info = lv_label_create(mbox_about);
            sprintf(aux_text, "\u2022 CPU: %s\n\u2022 RAM: %i MB \n\u2022 Flash: %i MB", cpu_version, RAM_size, FLASH_size);
            lv_label_set_text(label_hw_info, aux_text);
            lv_obj_align(label_hw_info, LV_ALIGN_TOP_LEFT, 10, 30);
            free(aux_text);

            aux_text = malloc(256);
            lv_obj_t * label_fw_version = lv_label_create(mbox_about);
            sprintf(aux_text, "v%s", app_version);
            lv_label_set_text(label_fw_version, aux_text);
            lv_obj_align(label_fw_version, LV_ALIGN_BOTTOM_MID, 0, -10);
            free(aux_text);

            lv_group_add_obj(group_interact, mbox_about);
            lv_obj_add_event_cb(mbox_about, mbox_config_cb, LV_EVENT_ALL, NULL);
            lv_group_focus_obj(mbox_about);
        }
        else if(strcmp(btn_text, "Update firmware") == 0){
            char fw_update_name[30][100];
            uint8_t fw_update_num = sd_app_list(fw_update_name, true);

            ESP_LOGI(TAG, "Found %i updates", fw_update_num);

            if(fw_update_num > 0){
                list_fw_update = lv_list_create(lv_layer_top());
                lv_obj_set_size(list_fw_update, 210, 210);
                lv_obj_align(list_fw_update, LV_ALIGN_CENTER, 0, 23);

                for(int i = 0; i < fw_update_num; i++){
                    lv_obj_t * update_btn = lv_list_add_btn(list_fw_update, NULL, fw_update_name[i]);
                    lv_group_add_obj(group_interact, update_btn);
                    lv_obj_add_event_cb(update_btn, fw_update_cb, LV_EVENT_ALL, NULL);
                }
                lv_group_add_obj(group_interact, list_fw_update);
                lv_group_focus_obj(list_fw_update);
            }
        }
        else if(strcmp(btn_text, "Brightness") == 0){
            mbox_brightness = lv_obj_create(lv_layer_top());
            lv_obj_set_size(mbox_brightness, 220, 100);
            lv_obj_align(mbox_brightness, LV_ALIGN_CENTER, 0, 0);

            lv_obj_t * title = lv_label_create(mbox_brightness);
            lv_label_set_text(title, "Brightness");
            lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 5);

            lv_group_add_obj(group_interact, mbox_brightness);
            lv_group_focus_obj(mbox_brightness);

            lv_obj_t * slider = lv_slider_create(mbox_brightness);
            lv_obj_align(slider, LV_ALIGN_CENTER, 0, 10);
            lv_obj_set_width(slider, 180);
            lv_slider_set_range(slider, 0, 100);

            uint8_t brightness_level = backlight_get();
            lv_slider_set_value(slider, brightness_level, LV_ANIM_ON);

            lv_obj_add_event_cb(slider, slider_brightness_cb, LV_EVENT_ALL, NULL);

            lv_group_add_obj(group_interact, slider);
            lv_group_focus_obj(slider);
        }
        else if(strcmp(btn_text, "GUI Color Mode") == 0){
            if(GUI_THEME == 0){
                system_save_config(SYS_GUI_COLOR, 1);
                GUI_theme_color(1);
            }
            else{
                system_save_config(SYS_GUI_COLOR, 0);
                GUI_theme_color(0);
            }
        }
        else if(strcmp(btn_text, "Volume") == 0){
            mbox_volume = lv_obj_create(lv_layer_top());
            lv_obj_set_size(mbox_volume, 220, 100);
            lv_obj_align(mbox_volume, LV_ALIGN_CENTER, 0, 0);

            lv_obj_t * title = lv_label_create(mbox_volume);
            lv_label_set_text(title, "Volume");
            lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 5);

            lv_group_add_obj(group_interact, mbox_volume);
            lv_group_focus_obj(mbox_volume);

            lv_obj_t * slider = lv_slider_create(mbox_volume);
            lv_obj_align(slider, LV_ALIGN_CENTER, 0, 10);
            lv_obj_set_width(slider, 180);
            lv_slider_set_range(slider, 0, 100);
            uint8_t volume = audio_volume_get();
            lv_slider_set_value(slider, volume, LV_ANIM_OFF);

            lv_obj_add_event_cb(slider, slider_volume_cb, LV_EVENT_ALL, NULL);

            lv_group_add_obj(group_interact, slider);
            lv_group_focus_obj(slider);
        }
        else if(strcmp(btn_text, "Enable Button State Save") == 0){
            // Find label child and update text
            for(int i = 0; i < (int)lv_obj_get_child_cnt(obj); i++) {
                lv_obj_t * child = lv_obj_get_child(obj, i);
                if(lv_obj_check_type(child, &lv_label_class)) {
                    lv_label_set_text(child, "Disable Button State Save");
                    break;
                }
            }
            system_save_config(SYS_STATE_SAV_BTN, 1);
        }
        else if(strcmp(btn_text, "Disable Button State Save") == 0){
            for(int i = 0; i < (int)lv_obj_get_child_cnt(obj); i++) {
                lv_obj_t * child = lv_obj_get_child(obj, i);
                if(lv_obj_check_type(child, &lv_label_class)) {
                    lv_label_set_text(child, "Enable Button State Save");
                    break;
                }
            }
            system_save_config(SYS_STATE_SAV_BTN, 0);
        }
        else if(strcmp(btn_text, "Battery Status") == 0){
            lv_obj_t * mbox_battery = lv_obj_create(lv_layer_top());
            lv_obj_set_size(mbox_battery, 220, 160);
            lv_obj_align(mbox_battery, LV_ALIGN_CENTER, 0, -30);

            lv_obj_t * title = lv_label_create(mbox_battery);
            lv_label_set_text(title, "Battery Status");
            lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 5);

            char spec_text[256];
            sprintf(spec_text, "\u2022 Total Capacity: %imAh\n\u2022 Voltage: %imV", 500, management.voltage);
            lv_obj_t * label_battery_info = lv_label_create(mbox_battery);
            lv_label_set_text(label_battery_info, spec_text);
            lv_obj_align(label_battery_info, LV_ALIGN_TOP_LEFT, 10, 30);

            lv_obj_t * bar_battery = lv_bar_create(mbox_battery);
            lv_obj_set_size(bar_battery, 180, 20);
            lv_obj_align(bar_battery, LV_ALIGN_BOTTOM_MID, 0, -20);

            if(management.voltage >= 4165){
                lv_obj_t * label_battery_bar = lv_label_create(bar_battery);
                lv_label_set_text(label_battery_bar, "Charged");
                lv_obj_align(label_battery_bar, LV_ALIGN_CENTER, 0, 0);

                lv_obj_set_style_bg_color(bar_battery, lv_color_hex(0x25CDF6), LV_PART_INDICATOR);
                lv_bar_set_value(bar_battery, 100, LV_ANIM_ON);
            }
            else{
                lv_obj_t * label_battery_bar = lv_label_create(bar_battery);
                char battery_level[8];
                sprintf(battery_level, "%i", management.percentage);
                lv_label_set_text(label_battery_bar, battery_level);
                lv_obj_align(label_battery_bar, LV_ALIGN_CENTER, 0, 0);

                lv_obj_set_style_bg_color(bar_battery, lv_color_hex(0x0CC62D), LV_PART_INDICATOR);
                lv_bar_set_value(bar_battery, management.percentage, LV_ANIM_ON);
            }
            lv_obj_add_event_cb(mbox_battery, mbox_config_cb, LV_EVENT_ALL, NULL);
            lv_group_add_obj(group_interact, mbox_battery);
            lv_group_focus_obj(mbox_battery);
        }
        else if(strcmp(btn_text, "SD card Status") == 0){
            lv_obj_t * mbox_SD = lv_obj_create(lv_layer_top());
            lv_obj_set_size(mbox_SD, 220, 140);
            lv_obj_align(mbox_SD, LV_ALIGN_CENTER, 0, -30);

            lv_obj_t * title = lv_label_create(mbox_SD);
            lv_label_set_text(title, "SD Card Information");
            lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 5);

            if(sd_mounted()){
                lv_obj_t * label1 = lv_label_create(mbox_SD);
                char spec_text[256];
                sprintf(spec_text, "\u2022 SD Name: %s\n\u2022 Size: %i MB \n\u2022 Speed: %i Khz\n\u2022 Type: %s",
                    sd_card_info.card_name, sd_card_info.card_size, sd_card_info.card_speed,
                    (sd_card_info.card_type & SDIO) ? "SDIO" : "SDHC/SDXC");
                lv_label_set_text(label1, spec_text);
                lv_obj_align(label1, LV_ALIGN_TOP_LEFT, 10, 30);
            }
            else{
                lv_obj_t * label1 = lv_label_create(mbox_SD);
                lv_label_set_text(label1, "SD card not available");
                lv_obj_align(label1, LV_ALIGN_CENTER, 0, 0);
            }

            lv_obj_add_event_cb(mbox_SD, mbox_config_cb, LV_EVENT_ALL, NULL);
            lv_group_add_obj(group_interact, mbox_SD);
            lv_group_focus_obj(mbox_SD);
        }
    }
    else if(code == LV_EVENT_CANCEL){
        sub_menu = false;
        // Remove all children from group before deleting
        uint32_t child_cnt = lv_obj_get_child_cnt(list_config);
        for(uint32_t i = 0; i < child_cnt; i++) {
            lv_obj_t * child = lv_obj_get_child(list_config, i);
            lv_group_remove_obj(child);
        }
        lv_obj_del(list_config);

        // Restore main menu buttons to group
        lv_group_add_obj(group_interact, btn_emulator_lib);
        lv_group_add_obj(group_interact, btn_ext_app);
        lv_group_add_obj(group_interact, config_btn);
        lv_group_focus_obj(config_btn);
    }
}

static void fw_update_cb(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);

    if(code == LV_EVENT_CLICKED){
        const char * fw_name = get_list_btn_text(obj);
        ESP_LOGI(TAG, "FW update: %s", fw_name);

        lv_obj_t * mbox = lv_obj_create(lv_layer_top());
        lv_obj_set_size(mbox, 200, 150);
        lv_obj_align(mbox, LV_ALIGN_CENTER, 0, -60);

        lv_obj_t * title = lv_label_create(mbox);
        lv_label_set_text(title, "Firmware update:");
        lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 5);

        lv_obj_t * label1 = lv_label_create(mbox);
        lv_label_set_text(label1, fw_name);
        lv_obj_align(label1, LV_ALIGN_TOP_MID, 0, 25);

        lv_obj_set_style_bg_opa(lv_layer_top(), LV_OPA_70, 0);
        lv_obj_set_style_bg_color(lv_layer_top(), lv_color_hex(0x808080), 0);
        lv_obj_add_flag(lv_layer_top(), LV_OBJ_FLAG_CLICKABLE);

        lv_group_focus_obj(mbox);
        lv_group_focus_freeze(group_interact, false);

        lv_obj_t * preload = lv_spinner_create(mbox, 1000, 60);
        lv_obj_set_size(preload, 80, 80);
        lv_obj_align(preload, LV_ALIGN_CENTER, 0, 20);

        struct SYSTEM_MODE fw_update;
        fw_update.mode = MODE_UPDATE;
        strcpy(fw_update.game_name, fw_name);

        if(xQueueSend(modeQueue, &fw_update, (TickType_t)10) != pdPASS){
            ESP_LOGE(TAG, "Error sending update execution queue");
        }
    }
    else if(code == LV_EVENT_CANCEL){
        lv_obj_del(list_fw_update);
    }
}

static void mbox_config_cb(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);

    if(code == LV_EVENT_CANCEL){
        lv_obj_del(obj);
    }
}

static void GUI_theme_color(uint8_t color_selected){
    GUI_THEME = color_selected;

    // In LVGL v8, use the default theme with dark mode option
    lv_theme_t * theme = lv_theme_default_init(
        lv_disp_get_default(),
        lv_palette_main(LV_PALETTE_BLUE),
        lv_palette_main(LV_PALETTE_RED),
        color_selected ? true : false,  // dark mode
        LV_FONT_DEFAULT
    );
    lv_disp_set_theme(lv_disp_get_default(), theme);
}

/****** External Async functions ***********/
void async_battery_alert(bool game_mode){
    lv_obj_t * mbox_battery_Alert = lv_obj_create(lv_scr_act());
    lv_obj_set_size(mbox_battery_Alert, 200, 120);
    lv_obj_align(mbox_battery_Alert, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t * title = lv_label_create(mbox_battery_Alert);
    lv_label_set_text(title, "Battery Alert");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    lv_obj_t * mbox_battery = lv_label_create(mbox_battery_Alert);
    lv_label_set_text(mbox_battery, "Battery level below 10%");
    lv_obj_align(mbox_battery, LV_ALIGN_CENTER, 0, 0);

    lv_group_add_obj(group_interact, mbox_battery_Alert);
    lv_group_focus_obj(mbox_battery_Alert);
}

/**********************
 *   TASKS FUNCTIONS
 **********************/

static void battery_status_task(lv_timer_t * timer){
    // This task check every minute if the battery level has change.

    if(xQueueReceive(batteryQueue, &management, (TickType_t)0)){
        if(management.voltage >= 4190){
            lv_obj_clear_flag(Charging_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_style_bg_color(battery_bar, lv_color_hex(0x25CDF6), LV_PART_INDICATOR);
            lv_bar_set_value(battery_bar, 100, LV_ANIM_OFF);
            lv_label_set_text(battery_label, "USB");
        }
        else{
            lv_obj_add_flag(Charging_label, LV_OBJ_FLAG_HIDDEN);
            if(management.percentage == 1){
                lv_label_set_text(battery_label, LV_SYMBOL_WARNING);
                lv_bar_set_value(battery_bar, 100, LV_ANIM_OFF);
            }
            else{
                char battery_level[8];
                sprintf(battery_level, "%i", management.percentage);
                lv_label_set_text(battery_label, battery_level);
                lv_bar_set_value(battery_bar, management.percentage, LV_ANIM_OFF);
            }
            lv_obj_align(battery_label, LV_ALIGN_CENTER, 0, 0);

            if(management.percentage <= 25){
                lv_obj_set_style_bg_color(battery_bar, lv_color_hex(0xC61D0C), LV_PART_INDICATOR);
            }
            else{
                lv_obj_set_style_bg_color(battery_bar, lv_color_hex(0x0CC62D), LV_PART_INDICATOR);
            }
        }
    }
}

static void user_input_task(lv_indev_drv_t * indev_drv, lv_indev_data_t * data){
    // Get the status of the multiplexer driver
    uint16_t inputs_value = input_read();

    data->state = LV_INDEV_STATE_RELEASED;

    if(!((inputs_value >> 0) & 0x01)){
        // Button Down pushed
        uint32_t actual_time = xTaskGetTickCount() / portTICK_PERIOD_MS;
        if((actual_time - btn_down_time) > bouncing_time){
            data->state = LV_INDEV_STATE_PRESSED;
            data->key = LV_KEY_NEXT;
            btn_down_time = actual_time;
        }
    }

    if(!((inputs_value >> 1) & 0x01)){
        // Button Left pushed
        uint32_t actual_time = xTaskGetTickCount() / portTICK_PERIOD_MS;
        if(sub_menu){
            data->state = LV_INDEV_STATE_PRESSED;
            data->key = LV_KEY_LEFT;
            btn_right_time = actual_time;
        }
        else{
            if((actual_time - btn_right_time) > bouncing_time){
                if(tab_num > 0){
                    tab_num--;
                    data->state = LV_INDEV_STATE_PRESSED;
                    data->key = LV_KEY_PREV;
                    lv_tabview_set_act(tab_main_menu, tab_num, LV_ANIM_ON);
                }
                btn_right_time = actual_time;
            }
        }
    }

    if(!((inputs_value >> 2) & 0x01)){
        // Button up pushed
        uint32_t actual_time = xTaskGetTickCount() / portTICK_PERIOD_MS;
        if((actual_time - btn_up_time) > bouncing_time){
            data->state = LV_INDEV_STATE_PRESSED;
            data->key = LV_KEY_PREV;
            btn_up_time = actual_time;
        }
    }

    if(!((inputs_value >> 3) & 0x01)){
        // Button right pushed
        uint32_t actual_time = xTaskGetTickCount() / portTICK_PERIOD_MS;
        if(sub_menu){
            data->state = LV_INDEV_STATE_PRESSED;
            data->key = LV_KEY_RIGHT;
            btn_right_time = actual_time;
        }
        else{
            if((actual_time - btn_right_time) > bouncing_time){
                if(tab_num < 2){
                    tab_num++;
                    data->state = LV_INDEV_STATE_PRESSED;
                    data->key = LV_KEY_NEXT;
                    lv_tabview_set_act(tab_main_menu, tab_num, LV_ANIM_ON);
                }
                btn_right_time = actual_time;
            }
        }
    }

    if(!((inputs_value >> 11) & 0x01)){
        // Button menu pushed
        uint32_t actual_time = xTaskGetTickCount() / portTICK_PERIOD_MS;
        if((actual_time - btn_menu_time) > bouncing_time){
            struct SYSTEM_MODE emulator;
            emulator.mode = MODE_GAME;
            emulator.status = 0;

            if(xQueueSend(modeQueue, &emulator, (TickType_t)10) != pdPASS){
                ESP_LOGE(TAG, "Button menu queue send error");
            }
            btn_menu_time = actual_time;
        }
    }

    if(!((inputs_value >> 8) & 0x01)){
        // Button B pushed
        uint32_t actual_time = xTaskGetTickCount() / portTICK_PERIOD_MS;
        if((actual_time - btn_b_time) > bouncing_time){
            data->state = LV_INDEV_STATE_PRESSED;
            data->key = LV_KEY_ESC;
            btn_b_time = actual_time;
        }
    }

    if(!((inputs_value >> 9) & 0x01)){
        // Button A pushed
        uint32_t actual_time = xTaskGetTickCount() / portTICK_PERIOD_MS;
        if((actual_time - btn_a_time) > bouncing_time){
            data->state = LV_INDEV_STATE_PRESSED;
            data->key = LV_KEY_ENTER;
            btn_a_time = actual_time;
        }
    }
}