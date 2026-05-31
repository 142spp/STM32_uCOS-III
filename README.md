# STM32F429ZI uC/OS-III GCC Project

STM32F429ZI Nucleo 보드에서 uC/OS-III를 올려 실행하는 bare-metal 펌웨어 프로젝트입니다. CMake로 `arm-none-eabi-gcc` 크로스 빌드를 수행하며, 결과물은 ELF/BIN/HEX로 생성됩니다.

## 구성

```text
app/
  config/      uC/OS-III, uC-CPU, uC-LIB, STM32 StdPeriph 설정
  src/         main(), 애플리케이션 태스크, USART3 직렬 입출력
platform/
  bsp/         보드 초기화, SysTick, LED, 인터럽트 핸들러
  cmsis/       STM32F4 CMSIS 헤더
  startup/     벡터 테이블, 링커 스크립트, syscall/printf 보조 코드
  stm32f4xx_stdperiph/
               STM32F4 Standard Peripheral Library
external/
  uCOS-III/    Micrium uC/OS-III 커널 및 Cortex-M4 GNU 포트
  uC-CPU/      Micrium CPU 추상화 계층
  uC-LIB/      Micrium 공용 라이브러리
tools/build/   CMake ARM GCC toolchain 파일
```

## 동작 개요

- 타깃: `STM32F429ZI`
- 클럭: BSP/System 코드 기준 HSE 8 MHz, PLL 168 MHz 설정
- RTOS: uC/OS-III
- LED: PB0, PB7, PB14를 USART 명령으로 on/off/toggle/blink 제어
- UART: Nucleo ST-LINK VCP의 USART3 PD8(TX), PD9(RX), 115200 8N1
- 직렬 명령: `led1 on`, `led2 blink`, `all off`, `status`, `help`

## 필요 도구

Ubuntu/WSL 기준:

```bash
sudo apt update
sudo apt install cmake ninja-build gcc-arm-none-eabi binutils-arm-none-eabi openocd gdb-multiarch
```

필수 빌드 도구:

- `cmake` 3.20 이상
- `ninja` 또는 Make
- `arm-none-eabi-gcc`
- `arm-none-eabi-objcopy`
- `arm-none-eabi-size`

플래시/디버그 도구:

- `openocd`
- ST-LINK 연결
- VS Code 디버깅 시 Cortex-Debug 확장과 `gdb-multiarch`

## 빌드

Ninja 사용:

```bash
cmake -S . -B build/ninja -G Ninja -DCMAKE_TOOLCHAIN_FILE=tools/build/arm-none-eabi-gcc.cmake
cmake --build build/ninja
```

Makefiles 사용:

```bash
cmake -S . -B build/make -DCMAKE_TOOLCHAIN_FILE=tools/build/arm-none-eabi-gcc.cmake
cmake --build build/make
```

주요 산출물:

```text
build/ninja/f429zi_os3_gcc.elf
build/ninja/f429zi_os3_gcc.bin
build/ninja/f429zi_os3_gcc.hex
build/ninja/f429zi_os3_gcc.map
```

## 플래시

ST-LINK가 연결되어 있고 OpenOCD가 보드를 볼 수 있을 때:

```bash
cmake --build build/ninja --target flash
```

VS Code에서는 `Terminal -> Run Task... -> CMake: flash`를 실행합니다.

수동 실행:

```bash
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \
  -c "program build/ninja/f429zi_os3_gcc.elf verify reset exit"
```

## 실행 확인

1. 보드에 펌웨어를 플래시합니다.
2. ST-LINK VCP를 115200 8N1로 엽니다.
3. 터미널에 `HW#5 USART LED Command Control` 배너와 프롬프트가 출력되는지 확인합니다.
4. `led1 on`, `led2 blink`, `all off`, `status` 명령으로 LED와 응답을 확인합니다.

예:

```bash
picocom -b 115200 /dev/ttyACM0
```

장치 이름은 환경에 따라 `/dev/ttyACM1`, `/dev/ttyUSB0` 등으로 달라질 수 있습니다.

## 테스트 상태

현재 저장소에는 호스트에서 실행되는 유닛 테스트나 시뮬레이터 테스트가 없습니다. 현실적인 검증 단계는 다음 순서입니다.

```bash
cmake -S . -B build/ninja -G Ninja -DCMAKE_TOOLCHAIN_FILE=tools/build/arm-none-eabi-gcc.cmake
cmake --build build/ninja
cmake --build build/ninja --target flash
```

그 다음 실제 보드에서 LED 토글과 USART3 송수신을 확인해야 합니다.

## 디버그

VS Code에서 Cortex-Debug 확장을 설치한 뒤 `Run and Debug -> Debug STM32F429ZI` 구성을 실행합니다. `.vscode/launch.json`은 `build/ninja/f429zi_os3_gcc.elf`와 `gdb-multiarch`를 사용하도록 설정되어 있습니다.
