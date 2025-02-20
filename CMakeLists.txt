cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(felk_ble_app)

target_sources(app PRIVATE src/main.c)

target_sources(app PRIVATE
    src/common.c
    src/remote.c
    src/pwm_f.c
    src/adc_f.c
)

zephyr_library_include_directories(library)