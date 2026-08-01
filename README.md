# infineonArduinoLike – Aurix TC275 Arduino‑Style HAL

A lightweight, modular, Arduino‑style Hardware Abstraction Layer (HAL) for **Infineon Aurix TriCore TC275** microcontrollers.  
This project is tested on the **TC275 Lite Kit** and provides a unified set of C‑based drivers, utilities, and high‑level modules that simplify development on the Aurix platform.

The goal is to make TC275 development **approachable**, **modular**, and **Arduino‑like**, while still exposing the full power of Infineon’s peripherals such as **ATOM**, **TOM**, **GTM**, **DMA**, **DSADC**, **Ethernet**, **CAN**, and more.

---

## Project Overview

This repository contains:

- Arduino‑style APIs rewritten in pure C for Aurix  
- High‑level modules (MPU6050, NRF24, ASK RF, A7670 CAT‑1 modem, ultrasonic sensor, MCP2515 CAN controller)  
- Low‑level drivers for GTM/ATOM/TOM PWM, SPI, I2C, UART, DMA, DSADC  
- Ethernet stack components (MAC handler, IP layer, packet builder)  
- Utility functions for pins, interrupts, timers, and ADC/UART combos  
- Phase‑shift PWM engines for multi‑channel synchronized PWM generation  

This is not an Arduino core — it is a **developer‑friendly HAL** inspired by Arduino semantics.

---

## Repository Structure

### AtomAndAru/
High‑performance GTM modules:
- ATOM phase‑shift PWM  
- ATOM serial mode SPI emulator  

### I2C (updated iLLD driver)
Includes:
- Updated Infineon iLLD I2C driver  
- I2C slave implementation  
- High‑level I2C wrapper  

### Utils/
Custom utilities:
- `std_int.c/h` — custom integer and type utilities  

---

## Core Modules (C / H pairs)

### Communication & Protocols
- **A7670CAT1.c/h** — Application layer for A7670 CAT‑1 modem  
- **A7670Cat1Read.c/h** — Low‑level modem communication  
- **Can.c/h** — Internal CAN driver  
- **Mcp2515.c/h** — External CAN controller via SPI  
- **SPI_NRF24.c/h** — NRF24L01 radio driver (Arduino C++ → C)  
- **RH_ASK.c/h** — ASK/OOK RF driver (Arduino RadioHead port)  
- **Spi.c/h** — SPI low‑level driver  
- **SerialInit.c/h** — UART initialization and communication  

### Sensors
- **MPU6050.c/h** — IMU driver  
- **HySrf05.c/h** — Ultrasonic distance sensor  

### PWM / GTM / Motor Control
- **PWM_GENERATOR.c/h** — ATOM + TOM PWM generator  
- **ThreeTimerPhase.c/h** — TOM 3‑phase PWM  
- **phaseShiftAtomPwm.c/h** — Multi‑ATOM synchronized PWM  
- **phaseShiftTomPwm.c/h** — Multi‑TOM synchronized PWM  

### Timing / Interrupts / ADC
- **Tim.c/h** — Timer utilities  
- **INTERRUPTS.c/h** — Interrupt configuration  
- **DsAdc.c/h** — DSADC driver  
- **adc_uart.c/h** — Combined ADC + UART helper  

### Ethernet Stack
- **McEthernetHandler.c/h** — MAC‑level handler  
- **McEthernetIp.c/h** — IP‑level logic  
- **McEthernetPacket.c/h** — Packet builder  

### DMA
- **Dma.c/h** — DMA driver  

### GPIO
- **pinsReadWrite.c/h** — Arduino‑style digitalRead/digitalWrite  

---

## Getting Started

### Requirements
- Infineon Aurix TC275 Lite Kit  
- AURIX Development Studio or HighTec toolchain  
- iLLD (Infineon Low‑Level Drivers)  
- TriCore compiler  

### Build
Copy paste the whole repo on a tc275 aurix development studio project and you are free to use the files.
