idf_component_register(
    SRCS
        "src/ssd1306.c"
        "src/ssd1306_font.c"
        "src/ssd1306_images.c"
    INCLUDE_DIRS "include"
    REQUIRES driver 
)
