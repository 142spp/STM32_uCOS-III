# HW#5 수행 보고서

- 이름:
- 학번:
- 제출 파일: `app_1.c`, `app_2.c`
- 동작 영상 링크:

## 1. 과제 목표

이번 HW#5의 목표는 제공받은 `app.c` starter code를 기반으로 STM32F429ZI 보드에서 uC/OS-III task를 사용하여 USART 입력과 LED 제어를 구현하는 것이었다. 과제는 두 단계로 나뉘어 있었다.

첫 번째 소과제는 USART로 입력한 문자를 다시 시리얼 터미널에 출력하는 echo 기능과 LED blink task를 구현하는 것이었다. 두 번째 소과제는 USART로 문자열 명령을 입력받아 LED1, LED2, LED3을 각각 `on`, `off`, `blink` 상태로 제어하는 command 기능을 구현하는 것이었다.

## 2. 개발 환경 구축 및 확인

처음에는 프로젝트의 빌드와 플래시가 가능한지부터 확인하였다. 이 프로젝트는 CMake 기반으로 구성되어 있었고, ARM Cortex-M용 크로스 컴파일러인 `arm-none-eabi-gcc`를 사용해야 했다. 일반 PC용 `gcc`나 `clang`으로는 STM32 펌웨어를 직접 빌드할 수 없기 때문에, 실제 펌웨어 빌드는 `arm-none-eabi-gcc`를 사용하고, `clangd`는 VS Code에서 코드 분석과 자동완성용으로 사용하는 구조로 정리하였다.

빌드는 다음 명령으로 수행하였다.

```bash
cmake --build build/ninja
```

플래시는 VS Code task에서 `flash` target을 실행하도록 구성되어 있었고, OpenOCD와 ST-LINK를 사용하였다. 처음 플래시할 때는 OpenOCD에서 ST-LINK 접근 권한 문제가 발생했다.

```text
Error: libusb_open() failed with LIBUSB_ERROR_ACCESS
Error: open failed
```

이 문제는 Linux에서 일반 사용자가 ST-LINK USB 장치에 접근할 권한이 없어서 발생한 것이었다. udev rule과 사용자 권한 설정을 적용한 뒤 OpenOCD가 정상적으로 보드에 접근할 수 있게 되었고, 이후 VS Code task를 통해 빌드와 플래시를 수행할 수 있었다.

또한 시리얼 통신을 위해 보드의 virtual COM port를 확인하였다. 현재 환경에서는 STM32 ST-LINK의 serial port가 `/dev/ttyACM0`로 잡혔고, 안정적인 경로로는 `/dev/serial/by-id/...` 형태의 장치 경로를 사용할 수 있었다. 시리얼 모니터 설정은 115200 baud, 8 data bits, no parity, 1 stop bit로 맞추었다.

## 3. 5-1 구현 과정

5-1에서는 먼저 USART task와 LED task를 별도로 생성하였다. 과제에서 USART task와 LED task를 분리하라고 요구했기 때문에, 하나의 task 안에서 모든 동작을 처리하지 않고 `AppTaskCreate()`에서 두 task를 생성하였다.

USART echo는 polling 방식으로 구현하였다. `USART_GetFlagStatus(USART3, USART_FLAG_RXNE)`를 확인하여 문자가 들어왔을 때만 `USART_ReceiveData()`로 읽고, 읽은 문자를 다시 `USART_SendData()`로 송신하였다.

```c
ch = UsartGetChar();
if (ch != '\0') {
    UsartPutChar(ch);
}
```

LED blink는 LED task에서 주기적으로 LED 상태를 반전시키는 방식으로 구현하였다. 첫 번째 소과제에서는 복잡한 명령 처리 없이 LED가 주기적으로 on/off 되는지만 확인하면 되었기 때문에, `led_on` 변수를 두고 500 ms마다 상태를 바꾸도록 만들었다.

이 단계에서 USART echo와 LED blink가 동시에 동작하는지 확인하였다. 두 기능이 별도 task에서 동작하므로 LED가 blink 중이어도 USART 입력이 처리되어야 하고, 반대로 USART 입력 중에도 LED blink가 멈추지 않아야 한다.

## 4. USART 초기화 위치 문제 해결

개발 중 시리얼 터미널에 출력되는 글자가 깨지는 문제가 있었다. 처음에는 baudrate 설정 문제로 생각했지만, USART3 초기화 시점과 시스템 클럭 초기화 순서가 원인이었다.

초기 코드 구조에서는 `main()`에서 peripheral 설정을 먼저 하고 이후 `BSP_Init()`에서 보드와 클럭 관련 초기화를 수행한다. USART baudrate는 peripheral clock을 기준으로 계산되므로, clock 설정이 안정되기 전에 USART를 초기화하면 실제 baudrate가 의도한 값과 달라져 문자가 깨질 수 있다.

따라서 USART3 초기화를 `main()`이 아니라 `AppTaskStart()` 안에서 `BSP_Init()`과 `BSP_Tick_Init()` 이후에 수행하도록 변경하였다.

```c
static void AppTaskStart(void *p_arg)
{
    BSP_Init();
    BSP_Tick_Init();

    Setup_Usart3();

    AppTaskCreate();
}
```

이후 시리얼 출력이 정상적으로 표시되었다.

## 5. 5-2 구현 과정

두 번째 소과제에서는 USART로 LED 제어 명령을 입력받아 처리해야 했다. 과제에서 요구한 명령 형식은 다음과 같았다.

| 명령 예시 | 동작 |
|---|---|
| `led3on` | LED3 on |
| `led2off` | LED2 off |
| `led1blink3` | LED1을 3초 주기로 blink |
| `reset` | 모든 LED off |

명령 처리를 위해 USART task는 입력 문자를 하나씩 받아 command buffer에 저장하고, 매 입력마다 현재까지 입력된 문자열을 parser에 전달하도록 구현하였다.

```c
CmdBuffer[CmdLen++] = ch;
CmdBuffer[CmdLen] = '\0';

UsartPutChar(ch);
HandleCommand(CmdBuffer);
```

처음에는 명령이 완전히 입력된 뒤 Enter를 눌렀을 때 처리하는 방식도 고려했지만, 현재 구현에서는 한 글자씩 입력될 때마다 parser를 호출하는 방식으로 작성하였다. 그래서 parser 내부에서는 아직 명령이 완성되지 않은 상태와 잘못된 명령을 구분해야 했다.

예를 들어 `led1off`를 입력하는 중간 단계인 `led1o`, `led1of`는 아직 완성된 명령이 아니므로 바로 wrong command로 처리하면 안 된다. 반면 명령 형식과 맞지 않는 문자열은 buffer를 초기화해야 한다.

## 6. 문자열 비교와 명령 길이 처리

명령 parser를 구현하면서 `Str_Cmp_N()`의 동작을 확인하였다. `Str_Cmp_N(a, b, n)`은 두 문자열의 앞에서부터 `n`글자를 비교하는 함수이므로, 전체 명령이 완성되었는지 확인하지 않으면 prefix만 맞아도 같은 명령처럼 판단될 수 있다.

예를 들어 `led1of`까지 입력된 상태에서 `"off"`와 앞 2글자만 비교하면 `"of"`가 일치하므로, 잘못하면 아직 완성되지 않은 명령을 `off` 명령으로 처리할 수 있다. 이를 막기 위해 `on`, `off`, `blink` 명령마다 기대 길이를 함께 확인하였다.

```c
if (Str_Cmp_N(cmd + 4, "off", cmdlen - 4) == 0) {
    if (cmdlen != 7) break;
    ...
}
```

`reset`은 길이가 5인 명령이고, `ledNon`, `ledNoff`, `ledNblinkT`는 각각 길이가 다르기 때문에 command length를 기준으로 명령 완성 여부를 판단하였다.

## 7. Task 간 공유 변수와 Critical Section

과제에서는 USART task와 LED task 간 communication을 global variable로 구현하고, shared variable 접근 시 critical section을 사용하라고 요구하였다. 이에 따라 LED 상태를 나타내는 global variable을 두었다.

```c
static ledcmd LedMode[LED_COUNT + 1];
static int    LedPeriod[LED_COUNT + 1];
static char   CmdBuffer[CMD_BUF_SIZE];
static int    CmdLen;
```

USART task 또는 command parser는 입력 명령에 따라 `LedMode[]`, `LedPeriod[]`를 변경하고, LED task는 주기적으로 이 값을 읽어 실제 LED를 제어한다. 두 task가 같은 변수를 접근하므로 `OS_CRITICAL_ENTER()`와 `OS_CRITICAL_EXIT()`로 보호하였다.

LED task에서는 critical section 안에서 공유 값을 local variable로 복사한 뒤, 실제 LED on/off 처리는 critical section 밖에서 수행하였다.

```c
OS_CRITICAL_ENTER();
mode = LedMode[led];
period = LedPeriod[led];
OS_CRITICAL_EXIT();
```

이렇게 한 이유는 critical section의 시간을 짧게 유지하면서도 LED task가 한 번의 판단에 필요한 mode와 period 값을 안정적으로 읽도록 하기 위해서이다.

## 8. Interrupt 방식 검토 후 Task 기반 방식 선택

처음에는 USART 수신을 interrupt 방식으로 구현하는 것도 고려하였다. 하지만 interrupt 방식으로 구현하면 실제 문자 수신 처리는 ISR에서 이루어지고, `AppTaskUsart()`에는 할 일이 거의 없어지는 문제가 있었다. 과제 요구사항은 USART task와 LED task를 별도로 생성하고 task 간 communication을 구현하는 것이므로, 최종적으로는 USART task가 직접 polling 방식으로 입력을 확인하고 command parser를 호출하는 구조로 작성하였다.

이 방식은 과제 요구사항인 task 기반 구조를 더 직접적으로 만족한다. 또한 ISR 안에서 문자열 parsing이나 USART 출력 같은 무거운 처리를 하지 않아도 되므로 구현과 디버깅이 단순해졌다.

## 9. Enter 처리와 시리얼 출력

시리얼 터미널에서 Enter를 눌렀을 때 줄바꿈이 제대로 보이도록 `'\r'`, `'\n'`을 처리하였다. 터미널에서는 줄바꿈을 위해 `\r\n`을 출력해야 정상적으로 다음 줄의 맨 앞으로 이동하는 경우가 많기 때문에, Enter 입력 시 다음과 같이 처리하였다.

```c
if (ch == '\r' || ch == '\n') {
    CmdLen = 0;
    UsartPrint("\r\n");
    continue;
}
```

이를 통해 명령 입력 후 Enter를 눌렀을 때 시리얼 모니터에서 새 줄로 이동하도록 하였다.

## 10. 최종 동작

최종적으로 구현한 `app_2.c`는 다음 기능을 수행한다.

| 입력 | 결과 |
|---|---|
| `led1on` | LED1 켜짐 |
| `led2off` | LED2 꺼짐 |
| `led3blink1` | LED3이 1초 주기로 blink |
| `led1blink3` | LED1이 3초 주기로 blink |
| `reset` | LED1, LED2, LED3 모두 꺼짐 |
| 잘못된 명령 | `Wrong Command` 출력 후 buffer 초기화 |

빌드는 다음 명령으로 확인하였다.

```bash
cmake --build build/ninja
```

빌드 결과 정상적으로 ELF가 생성되었다.

```text
text    data    bss     dec     hex
15612   12      13452   29076   7194
```

## 11. 느낀 점 및 정리

이번 과제에서는 단순히 LED를 켜고 끄는 것보다, RTOS 환경에서 task를 분리하고 task 간 shared variable을 안전하게 사용하는 점이 중요했다. 특히 USART 입력은 한 글자씩 들어오기 때문에, 명령이 아직 완성되지 않은 상태와 실제로 잘못된 명령을 구분하는 parser 로직이 필요했다.

또한 peripheral 초기화 순서에 따라 USART baudrate가 달라질 수 있다는 점을 확인하였다. `BSP_Init()` 이후 USART를 초기화하도록 수정하면서 시리얼 출력 깨짐 문제를 해결할 수 있었다.

최종적으로 USART task는 입력 수신과 command parsing을 담당하고, LED task는 global command state를 읽어 LED를 제어하는 구조로 구현하였다. 이를 통해 과제에서 요구한 USART echo, LED blink, LED command control, critical section 적용을 모두 수행하였다.
