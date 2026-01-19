idf_component_register(
    SRCS "boot_screen.c" "unicode.c"
    INCLUDE_DIRS "."
    REQUIRES display_HAL font_render
EMBED_FILES "Ubuntu-R.ttf"
)