file(GLOB_RECURSE FONTS_SOURCES fonts/*.c)
file(GLOB_RECURSE IMAGES_SOURCES images/*.c)

idf_component_register(SRCS "main.c" ${FONTS_SOURCES} ${IMAGES_SOURCES}
                    INCLUDE_DIRS ".")