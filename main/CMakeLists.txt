idf_component_register(SRCS "main_app.c" "wifi_driver_utils.c" "mqtt_driver_utils.c" "main_gui.c" "mhz19.c" "DHT22.c"
                    INCLUDE_DIRS ".")
target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-format")
