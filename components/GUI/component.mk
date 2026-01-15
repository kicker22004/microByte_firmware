ifndef LV_CONF_INCLUDE_SIMPLE
CFLAGS += -DLV_CONF_INCLUDE_SIMPLE
endif

COMPONENT_SRCDIRS := . \
        LVGL/src \
        LVGL/src/core \
        LVGL/src/draw \
        LVGL/src/draw/sw \
        LVGL/src/extra \
        LVGL/src/extra/layouts \
        LVGL/src/extra/layouts/flex \
        LVGL/src/extra/layouts/grid \
        LVGL/src/extra/themes \
        LVGL/src/extra/themes/basic \
        LVGL/src/extra/themes/default \
        LVGL/src/extra/themes/mono \
        LVGL/src/extra/widgets \
        LVGL/src/extra/widgets/animimg \
        LVGL/src/extra/widgets/calendar \
        LVGL/src/extra/widgets/chart \
        LVGL/src/extra/widgets/colorwheel \
        LVGL/src/extra/widgets/imgbtn \
        LVGL/src/extra/widgets/keyboard \
        LVGL/src/extra/widgets/led \
        LVGL/src/extra/widgets/list \
        LVGL/src/extra/widgets/menu \
        LVGL/src/extra/widgets/meter \
        LVGL/src/extra/widgets/msgbox \
        LVGL/src/extra/widgets/span \
        LVGL/src/extra/widgets/spinbox \
        LVGL/src/extra/widgets/spinner \
        LVGL/src/extra/widgets/tabview \
        LVGL/src/extra/widgets/tileview \
        LVGL/src/extra/widgets/win \
        LVGL/src/extra/others \
        LVGL/src/extra/others/snapshot \
        LVGL/src/extra/others/monkey \
        LVGL/src/extra/others/gridnav \
        LVGL/src/extra/others/fragment \
        LVGL/src/extra/others/imgfont \
        LVGL/src/extra/others/msg \
        LVGL/src/extra/others/ime \
        LVGL/src/font \
        LVGL/src/hal \
        LVGL/src/misc \
        LVGL/src/widgets

COMPONENT_ADD_INCLUDEDIRS := . LVGL LVGL/src
