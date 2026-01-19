# components/font_render/CMakeLists.txt
set(FREETYPE_SRCS
    "freetype2/src/base/ftsystem.c"
    "freetype2/src/base/ftinit.c"
    "freetype2/src/base/ftdebug.c"
    "freetype2/src/base/ftbase.c"
    "freetype2/src/truetype/truetype.c"
    "freetype2/src/sfnt/sfnt.c"
    "freetype2/src/smooth/smooth.c"
    "font_render.c"
)

idf_component_register(
    SRCS ${FREETYPE_SRCS}
    INCLUDE_DIRS "." "include" "freetype2/include"
)

target_compile_definitions(${COMPONENT_LIB} PRIVATE 
    FT2_BUILD_LIBRARY 
    IS_LITTLE_ENDIAN
)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-unused-function)