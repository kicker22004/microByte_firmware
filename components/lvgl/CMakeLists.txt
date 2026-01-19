file(GLOB_RECURSE SOURCES "src/*.c")
idf_component_register(
    SRCS ${SOURCES}
    INCLUDE_DIRS "." "src"
)
target_compile_definitions(${COMPONENT_LIB} PUBLIC LV_CONF_INCLUDE_SIMPLE=1)