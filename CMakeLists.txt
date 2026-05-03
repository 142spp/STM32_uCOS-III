cmake_minimum_required(VERSION 3.20)

project(nucleo_f429zi_example C ASM)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(APP_DIR "${CMAKE_CURRENT_LIST_DIR}/app")
set(PLATFORM_DIR "${CMAKE_CURRENT_LIST_DIR}/platform")
set(EXTERNAL_DIR "${CMAKE_CURRENT_LIST_DIR}/external")
set(LINKER_SCRIPT "${PLATFORM_DIR}/startup/stm32f4_flash.ld")

file(GLOB APP_SOURCES CONFIGURE_DEPENDS
    "${APP_DIR}/src/*.c"
)

file(GLOB BSP_SOURCES CONFIGURE_DEPENDS
    "${PLATFORM_DIR}/bsp/*.c"
)

file(GLOB STARTUP_SOURCES CONFIGURE_DEPENDS
    "${PLATFORM_DIR}/startup/*.c"
    "${PLATFORM_DIR}/startup/*.s"
)

file(GLOB STM32_SPL_SOURCES CONFIGURE_DEPENDS
    "${PLATFORM_DIR}/stm32f4xx_stdperiph/STM32F4xx/src/*.c"
)

file(GLOB UC_LIB_SOURCES CONFIGURE_DEPENDS
    "${EXTERNAL_DIR}/uC-LIB/*.c"
    "${EXTERNAL_DIR}/uC-LIB/Ports/ARM-Cortex-M4/GNU/*.s"
)

file(GLOB UC_CPU_SOURCES CONFIGURE_DEPENDS
    "${EXTERNAL_DIR}/uC-CPU/*.c"
    "${EXTERNAL_DIR}/uC-CPU/ARM-Cortex-M4/GNU/*.c"
    "${EXTERNAL_DIR}/uC-CPU/ARM-Cortex-M4/GNU/*.s"
)

file(GLOB UCOS_SOURCES CONFIGURE_DEPENDS
    "${EXTERNAL_DIR}/uCOS-III/Source/*.c"
    "${EXTERNAL_DIR}/uCOS-III/Ports/ARM-Cortex-M4/Generic/GNU/*.c"
    "${EXTERNAL_DIR}/uCOS-III/Ports/ARM-Cortex-M4/Generic/GNU/*.S"
)

add_executable(f429zi_os3_gcc.elf
    ${APP_SOURCES}
    ${BSP_SOURCES}
    ${STARTUP_SOURCES}
    ${STM32_SPL_SOURCES}
    ${UC_LIB_SOURCES}
    ${UC_CPU_SOURCES}
    ${UCOS_SOURCES}
)

target_include_directories(f429zi_os3_gcc.elf PRIVATE
    "${APP_DIR}/src"
    "${APP_DIR}/config"
    "${PLATFORM_DIR}/bsp"
    "${PLATFORM_DIR}/cmsis"
    "${PLATFORM_DIR}/stm32f4xx_stdperiph/STM32F4xx/inc"
    "${EXTERNAL_DIR}/uC-CPU"
    "${EXTERNAL_DIR}/uC-CPU/ARM-Cortex-M4/GNU"
    "${EXTERNAL_DIR}/uC-LIB"
    "${EXTERNAL_DIR}/uC-LIB/Ports/ARM-Cortex-M4/GNU"
    "${EXTERNAL_DIR}/uCOS-III/Source"
    "${EXTERNAL_DIR}/uCOS-III/Ports/ARM-Cortex-M4/Generic/GNU"
)

target_compile_definitions(f429zi_os3_gcc.elf PRIVATE
    STM32F429xx
    USE_STDPERIPH_DRIVER
)

target_compile_options(f429zi_os3_gcc.elf PRIVATE
    -mcpu=cortex-m4
    -mthumb
    -mfpu=fpv4-sp-d16
    -mfloat-abi=hard
    -ffunction-sections
    -fdata-sections
    -Wall
    -Wextra
    -Wno-unused-parameter
    -Wno-missing-field-initializers
)

target_link_options(f429zi_os3_gcc.elf PRIVATE
    -mcpu=cortex-m4
    -mthumb
    -mfpu=fpv4-sp-d16
    -mfloat-abi=hard
    "-T${LINKER_SCRIPT}"
    -Wl,--gc-sections
    -Wl,-Map=f429zi_os3_gcc.map
)

add_custom_command(TARGET f429zi_os3_gcc.elf POST_BUILD
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:f429zi_os3_gcc.elf>
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:f429zi_os3_gcc.elf> f429zi_os3_gcc.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:f429zi_os3_gcc.elf> f429zi_os3_gcc.hex
)

add_custom_target(flash
    COMMAND openocd -f interface/stlink.cfg -f target/stm32f4x.cfg
            -c "program $<TARGET_FILE:f429zi_os3_gcc.elf> verify reset exit"
    DEPENDS f429zi_os3_gcc.elf
    USES_TERMINAL
)
