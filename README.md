# STM32F4 Bare-Metal Drivers

A collection of bare-metal (no HAL/LL) peripheral drivers for STM32F401RE microcontroller, designed for learning embedded systems programming from scratch.

## 🎯 Project Overview

This repository contains educational bare-metal drivers built from the ground up using only:
- CMSIS Core headers
- STM32F401 device headers
- Direct register manipulation

**No HAL. No Libraries. Just pure embedded C.**

## 🔧 Hardware

- **Board**: STM32F401RE Nucleo
- **MCU**: STM32F401RETx (Cortex-M4, 512KB Flash, 96KB RAM)
- **Clock**: 84 MHz (PLL configured from 16 MHz HSI)

## 📚 Projects

### ✅ 01 - Timer Driver (TIM2)
**Status**: Complete  
**Features**:
- Bare-metal TIM2 driver with interrupt support
- LED blinks every 3 seconds on PA5
- Demonstrates:
  - Timer configuration (prescaler, period)
  - NVIC interrupt handling
  - GPIO output control
  - System clock configuration (84 MHz)

**Files**:
- `tim_driver.h/c` - Timer driver implementation
- `main.c` - Application code

## 🛠️ Build Instructions

### Prerequisites
```bash
# Install ARM GCC toolchain
sudo apt-get install gcc-arm-none-eabi

# Install STM32 flash tool
sudo apt-get install stlink-tools

# Or use STM32CubeIDE (includes toolchain)
```

### Building with Makefile
```bash
cd 01_timer_driver
make clean
make all
```

### Building with STM32CubeIDE
1. File → Open Projects from File System
2. Select project directory
3. Project → Build All (Ctrl+B)

### Flashing to Board
```bash
# Using st-flash
make flash

# Or manually
st-flash write build/project.bin 0x8000000

# Or use STM32CubeIDE
Right-click → Run As → STM32 C/C++ Application
```

## 📖 Learning Path

**Recommended order**:
1. ✅ **Timer Driver** - Interrupts & timing

## 📂 Project Structure

```
project_name/
├── Core/
│   ├── Inc/          # Header files
│   └── Src/          # Source files
├── Linker/           # Linker script
├── Makefile          # Build configuration
└── README.md         # Project documentation
```

## 🎓 Key Concepts Demonstrated

- Direct register manipulation
- Interrupt handling (NVIC)
- Clock tree configuration
- Memory-mapped peripheral access
- Linker scripts
- Startup code (assembly)
- Vector table configuration

## 📝 Documentation

Each project includes:
- Detailed code comments
- Register-level explanations
- Timing calculations
- Hardware configuration notes

## 🤝 Contributing

This is an educational project. Feel free to:
- Report issues
- Suggest improvements
- Add new driver examples
- Improve documentation

## 📄 License

MIT License - Feel free to use for learning and teaching

## 🔗 Resources

- [STM32F401RE Reference Manual](https://www.st.com/resource/en/reference_manual/dm00096844.pdf)
- [STM32F401RE Datasheet](https://www.st.com/resource/en/datasheet/stm32f401re.pdf)
- [ARM Cortex-M4 Technical Reference](https://developer.arm.com/documentation/100166/0001)
- [CMSIS Documentation](https://arm-software.github.io/CMSIS_5/Core/html/index.html)

## 🌟 Acknowledgments

- STMicroelectronics for CMSIS headers
- ARM for Cortex-M4 architecture
- Open-source embedded community

---

**Happy Coding! 🚀**
