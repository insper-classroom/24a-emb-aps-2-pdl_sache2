add_executable(main
        hc05.c
        main.c
        controller.c
)

target_link_libraries(main pico_stdlib hardware_adc hardware_pwm freertos)
pico_add_extra_outputs(main)
