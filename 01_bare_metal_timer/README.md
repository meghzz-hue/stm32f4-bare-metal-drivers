# Project 01: Timer Basic - 3 Second LED Blink

Simple bare-metal timer application demonstrating hardware timer interrupt on STM32F401RE.

## 🎯 Objective

Learn how to:
- Configure hardware timer (TIM2) from scratch
- Generate periodic interrupts
- Toggle GPIO without HAL libraries
- Set up bare-metal build system

## 🔧 Features

- **Timer**: TIM2 generates interrupt every 3 seconds
- **GPIO**: PA5 (onboard green LED) toggles on interrupt
- **Clock**: System clock configured to 84 MHz using PLL
- **Interrupts**: NVIC configured for TIM2 global interrupt

## 📐 Hardware Connections

| Component | Pin | Description |
|-----------|-----|-------------|
| LED       | PA5 | Onboard green LED (LD2) |
| ST-LINK   | USB | Programming and debugging |



## ⚙️ Configuration

### Timer Configuration
```c
TIM2 Clock:    84 MHz (APB1 timer clock)
Prescaler:     8399  → 84 MHz / 8400 = 10 kHz
Period:        29999 → 10 kHz / 30000 = 3 seconds
Interrupt:     Update event (counter overflow)
```

### Memory Layout
- **Flash**: 512 KB (0x08000000 - 0x0807FFFF)
- **RAM**:   96 KB  (0x20000000 - 0x20017FFF)
- **Stack**: 1 KB
- **Heap**:  512 bytes

## 🚀 Build Instructions

### Using Makefile
```bash
cd 01_timer_basic
make clean
make all
```

### Using STM32CubeIDE
1. Import project: File → Import → Existing Projects
2. Select `01_timer_basic` folder
3. Build: Project → Build All (Ctrl+B)

## 📤 Flashing

### Method 1: st-flash
```bash
st-flash write build/bare_metal_timer_project.bin 0x8000000
```

### Method 2: STM32CubeIDE
1. Right-click project → Run As → STM32 C/C++ Application
2. Select Debug configuration
3. Click Run



## ✅ Expected Behavior

- **LED blinks ON/OFF every 3 seconds**
- Initial state: LED OFF
- After 3s: LED ON
- After 6s: LED OFF
- Repeats indefinitely

## 📊 Timing Verification

Use a stopwatch to verify:
- Count blinks in 60 seconds → Should be **20 toggles** (10 ON→OFF cycles)
- Each state should last exactly 3.00 seconds

## 🐛 Troubleshooting

### LED not blinking?
1. Check clock configuration in `SystemClock_Config()`
2. Verify APB1 timer clock = 84 MHz
3. Check NVIC interrupt is enabled
4. Confirm GPIO PA5 is configured as output

### Wrong timing?
1. Verify system clock: Should be 84 MHz
2. Check prescaler and period calculations
3. Measure with oscilloscope/logic analyzer



## 📁 Project Structure

```
01_timer_basic/
├── Core/
│   ├── Inc/               # Header files
│   │   ├── tim_driver.h   # Timer driver API
│   │   └── [CMSIS headers]
│   │
│   └── Src/               # Source files
│       ├── main.c         # Application entry point
│       ├── tim_driver.c   # Timer driver implementation
│       ├── syscalls.c     # System call stubs
│       └── [startup files]
│
├── Linker/
│   └── STM32F401RETX_FLASH.ld  # Memory layout
│
├── Makefile               # Build system
└── README.md            
```

## 🔍 Key Files

### main.c
- Application entry point
- GPIO initialization
- System clock configuration
- Timer configuration and start

### tim_driver.c/h
- TIM2 initialization
- Interrupt handler
- Timer control functions (start/stop)

### startup_stm32f401xe.s
- Vector table
- Reset handler
- Default interrupt handlers

## 📚 Learning Resources
- https://www.st.com/resource/en/reference_manual/rm0368-stm32f401xbc-and-stm32f401xde-advanced-armbased-32bit-mcus-stmicroelectronics.pdf
- https://www.st.com/resource/en/datasheet/stm32f401re.pdf
- [STM32F401 Reference Manual](https://www.st.com/resource/en/reference_manual/rm0368-stm32f401xbc-and-stm32f401xde-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
- [Cortex-M4 Programming Manual](https://www.st.com/resource/en/programming_manual/pm0214-stm32-cortexm4-mcus-and-mpus-programming-manual-stmicroelectronics.pdf)

## 🔄 Next Steps

See **Project 02: Software Timers** to learn:
- Multiple concurrent timers
- Software timer management
- Task scheduling without RTOS
- Callback-based events

