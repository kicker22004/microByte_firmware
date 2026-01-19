idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES 
	GUI
	lvgl
	display_HAL
	system_configuration
	OTA
	GBC	
	NES
	SMS
	boot_screen
	sd_storage
	battery
	sound
	user_input
	LED
	backlight_ctrl
)
