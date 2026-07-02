Theremin-like Instrument (STM32F401 and VL6180X)

Overview
This project implements a Theremin-like instrument using an STM32 Nucleo-F401RE and a VL6180X proximity sensor. The distance of the hand from the sensor controls the note frequency, and the user button on the Nucleo changes the waveform (sine / triangle / square / saw).
The audio signal is generated as PWM, then converted to an analog-like audio signal using a low-pass RC filter, and finally sent to a powered speaker.

Demo Videos
- [Final project demonstration](https://youtu.be/_6XN4k_lHYg)
- [PWM control with hand distance](https://youtu.be/aEjlqiktlC4)
- [YouTube playlist](https://www.youtube.com/playlist?list=PLb6JbbSLS7Bw)

Requirements
- Board: Nucleo-F401RE (STM32F401)
- Sensor: VL6180X shield/module (I2C address 0x29)
- Software: STM32CubeIDE (full CubeIDE project is provided)
- Optional: PuTTY/TeraTerm for UART debug prints (USART2)
- Hardware (audio output):
  - RC filter components used: 100 Ω, 3.3 kΩ x2, 10 nF x2, 1–10 µF, 10 kΩ
  - Powered speaker with AUX input (3.5 mm)

Project Structure
- Core/Src/main.c
  Main application logic: sensor reading, filtering, distance-to-frequency mapping, waveform switching.
- Core/Src/audio.c  and  Core/Inc/audio.h
  - DDS + wavetable (sine/tri/saw/square)
- Core/Src/vl6180x.c  and  Core/Inc/vl6180x.h
  - small VL6180X driver (HAL I2C mem read/write, init, single shot range)
- Core/Src/* and Core/Inc/*
  HAL/CubeMX generated initialization and peripheral configuration files.
- Core/Src/syscalls.c
  - `_write()` retarget, so `printf()` goes to USART2
  - timer tick updates PWM duty (CCR)
- Core/Src/stm32f4xx_it.c
  - timer interrupt callback for audio sample update

Pin connections and mapping
I2C for sensor :
- I2C1_SCL = PB8
- I2C1_SDA = PB9
- VL6180X I2C address (7-bit) = 0x29

UART debug:
- USART2 @ 115200
- PA2 = TX, PA3 = RX
- Use PuTTY with ST-LINK VCP COM port

PWM audio output:
- TIM3 CH1 = PA6  (PWM Generation CH1)

Button:
- Blue user button on Nucleo (PC13) with EXTI interrupt for waveform switch

.ioc configuration
- I2C1 enabled on PB8/PB9, speed 100 kHz
- USART2 enabled 115200
- TIM3: PWM Generation CH1
  - PSC = 0
  - ARR = 1343
  - Pulse around ARR/2 at start
- TIM2: Base timer interrupt for sample rate 20000
  - NVIC interrupt enabled for TIM2

How to Build and Run
1) Open the project in STM32CubeIDE.
2) Connect the Nucleo board by USB.
3) Build: Project -> Build Project.
4) Run: Run or Debug.
5) Move your hand in front of the VL6180X sensor to change the pitch.
6) Press the blue user button to cycle waveform.

Hardware
- The PWM output is filtered using a two-stage RC low-pass filter on a breadboard.
- The filtered output is connected to a powered speaker through the AUX tip.
- In the lab there was no jack breakout, so the connection was made directly using the AUX cable tip.

