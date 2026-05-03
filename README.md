# STM32F429ZI uC/OS-III GCC Project

This is a cleaned WSL/GCC-oriented STM32F429ZI project.

```text
app/        Application tasks and project-specific config
platform/   STM32F429ZI board/chip support, startup, CMSIS, and StdPeriph
external/   External Micrium code: uC/OS-III, uC-CPU, and uC-LIB
tools/      Build helper files
build/      Generated CMake/Ninja output
```

## Build

From this project root:

```bash
cmake -S . -B build/ninja -G Ninja -DCMAKE_TOOLCHAIN_FILE=tools/build/arm-none-eabi-gcc.cmake
cmake --build build/ninja
```

If Ninja is not installed, use the default Makefiles generator:

```bash
cmake -S . -B build/make -DCMAKE_TOOLCHAIN_FILE=tools/build/arm-none-eabi-gcc.cmake
cmake --build build/make
```

The main output is:

```text
build/ninja/f429zi_os3_gcc.elf
```

## Flash

With ST-LINK attached to WSL through usbipd:

```bash
cmake --build build/ninja --target flash
```

Or manually:

```bash
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \
  -c "program build/ninja/f429zi_os3_gcc.elf verify reset exit"
```

