# Project 01: Timer Basic - 3 Second LED Blink

Simple bare-metal timer application demonstrating hardware timer interrupt on STM32F401RE using a professional device driver architecture.

---

## 🎯 Objective

Learn how to:
- Design **hardware-independent device drivers** (not HAL-style callbacks)
- Follow **POSIX file-like API** semantics: `open/write/read/ioctl/close`
- Implement **clean separation of concerns**: driver ↔ application
- Use **static memory allocation** for embedded systems (no malloc)
- Create **opaque handles** for encapsulation
- Build drivers incrementally: struct → state → init → API → ISR
- Configure hardware timer (TIM2) from scratch
- Generate periodic interrupts
- Toggle GPIO without HAL libraries
- Set up bare-metal build system

---

## 🎓 What This Project Teaches

This is NOT just a timer blink example. It demonstrates **professional device driver architecture**:

### ✅ Core Principles Implemented
1. **Hide Hardware Completely** — Driver is only code that touches TIM2 registers
2. **Hardware-Independent API** — Change from TIM2 to TIM3 without modifying application
3. **Opaque Handle Design** — Application cannot corrupt driver state
4. **State Machine** — Clear lifecycle: UNINITIALIZED → INITIALIZED → RUNNING → STOPPED
5. **Minimal ISR** — Interrupt handler only sets event flags (< 10 CPU cycles)
6. **Event Polling** — Application controls event handling, not driver callbacks
7. **Deterministic Allocation** — Static pool (no unpredictable malloc/free)
8. **Reusable & Scalable** — Supports 4 timers (TIM2-TIM5) from same code

---

## ✨ Features

- **Driver**: Hardware-agnostic timer driver with clean API
- **Timer Support**: TIM2, TIM3, TIM4, TIM5 via logical IDs
- **Interrupt**: Minimal ISR (flag-setting only), main polling loop
- **GPIO**: PA5 (LED) toggles every 3 seconds via application control
- **Allocation**: Static pool (4 instances), no malloc/free
- **Clock**: System clock configured to 84 MHz using PLL
- **Interrupts**: NVIC configured for TIM2 global interrupt

---

## 📋 Architecture Overview

### Layering (Strict Separation)

```
┌─────────────────────────────────────┐
│  Application (main.c)               │
│  - Opens/reads/writes driver        │
│  - Polls for events                 │
│  - Handles application logic        │
│  - ZERO hardware knowledge          │
└────────────────────┬────────────────┘
                     │ (generic API)
┌─────────────────────────────────────┐
│  Timer Driver (tim_driver.c)        │
│  - Manages hardware state           │
│  - Provides clean interface         │
│  - Hides all STM32 types            │
│  - Static pool allocation           │
└────────────────────┬────────────────┘
                     │ (hardware access)
┌─────────────────────────────────────┐
│  Hardware (TIM2, TIM3, TIM4, TIM5)  │
└─────────────────────────────────────┘
```

### Refactoring Comparison

| **Aspect**             | **OLD (Non-Compliant)**     | **NEW (Compliant)**             | **Why**                    |
| ---------------------- | --------------------------- | ------------------------------- | -------------------------- |
| **Event Notification** | Callback (driver calls app) | Polling (app reads from driver) | ✅ App remains in control   |
| **Hardware Types**     | Exposed in header           | Hidden inside `.c` file         | ✅ Hardware-agnostic design |
| **Memory Management**  | `malloc()` / `free()`       | Static memory pool              | ✅ Deterministic behavior   |
| **Handle Design**      | Fully visible structure     | Opaque pointer                  | ✅ Strong encapsulation     |
| **State Management**   | No explicit state tracking  | `TIM_State_t` enum              | ✅ Clear lifecycle control  |
| **Scalability**        | Single timer support        | Timer pool (4 instances)        | ✅ Reusable & scalable      |
| **Portability**        | Embedded-only               | Linux / RTOS compatible         | ✅ Industry-grade design    |

---

## 🏗️ Hardware Configuration

### Supported Timers

The driver abstracts hardware timer selection via `TIM_ID_t` enum:

| Timer ID | Hardware | Status |
|----------|----------|--------|
| TIM_ID_2 | TIM2 (APB1) | ✅ Supported |
| TIM_ID_3 | TIM3 (APB1) | ✅ Supported |
| TIM_ID_4 | TIM4 (APB1) | ✅ Supported |
| TIM_ID_5 | TIM5 (APB1) | ✅ Supported |

**To use TIM3 instead of TIM2:**
```c
// Just change this line in main.c:
TIM_Handle_t *h = TIM_Open(TIM_ID_3);  // ← Change ID only!
// Rest of code works unchanged
```

### Default Configuration

```c
Timer Frequency:  1 MHz (84 MHz clock / 84 prescaler)
Period:           3000 ms (3 seconds)
Interrupt:        Update event (counter overflow)
Priority:         5 (mid-level, configurable)
```

### Physical Hardware

| Component | Pin | Description |
|-----------|-----|-------------|
| LED       | PA5 | Onboard green LED (LD2) |
| ST-LINK   | USB | Programming and debugging |

### Memory Layout
- **Flash**: 512 KB (0x08000000 - 0x0807FFFF)
- **RAM**:   96 KB  (0x20000000 - 0x20017FFF)
- **Stack**: 1 KB
- **Heap**:  512 bytes (not used - static allocation)

---

## 📖 Driver API Reference

### Public Functions

#### `TIM_Handle_t *TIM_Open(TIM_ID_t timer_id)`
Opens a timer device for use.

```c
TIM_Handle_t *h = TIM_Open(TIM_ID_2);
if (!h) { /* Error handling */ }
```

**Parameters:** `timer_id` - Logical timer ID (TIM_ID_2/3/4/5)  
**Returns:** Opaque handle or NULL on error

---

#### `TIM_Status_t TIM_Write(TIM_Handle_t *handle, const TIM_Config_t *config)`
Configures timer parameters (prescaler, period, priority).

```c
TIM_Config_t config = {
    .prescaler = 83,      // Clock divisor (84MHz → 1MHz)
    .period_ms = 3000,    // Period in milliseconds
    .priority = 5,        // Interrupt priority (0-15)
};
TIM_Write(h, &config);
```

**Returns:** `TIM_STATUS_OK` or error code

---

#### `TIM_Status_t TIM_Read(TIM_Handle_t *handle, TIM_Data_t *data_out)`
Reads timer status (polling for events).

```c
TIM_Data_t data;
TIM_Read(h, &data);

if (data.event_occurred) {
    // Timer expired, handle event
    LED_Toggle();
    TIM_Control(h, TIM_CMD_CLEAR_EVENT, NULL);
}
```

**Output Structure:**
```c
typedef struct {
    TIM_State_t state;           // Current state
    uint32_t event_counter;      // Number of expirations
    bool event_occurred;         // True if timer expired
} TIM_Data_t;
```

**Returns:** `TIM_STATUS_OK` or error code

---

#### `TIM_Status_t TIM_Control(TIM_Handle_t *handle, TIM_Command_t cmd, void *arg)`
Control timer operations (start, stop, reset, clear event).

```c
TIM_Control(h, TIM_CMD_START, NULL);         // Start timer
TIM_Control(h, TIM_CMD_STOP, NULL);          // Stop timer
TIM_Control(h, TIM_CMD_RESET, NULL);         // Reset counter
TIM_Control(h, TIM_CMD_CLEAR_EVENT, NULL);   // Clear event flag
```

**Commands:**
- `TIM_CMD_START` — Start counting
- `TIM_CMD_STOP` — Stop counting
- `TIM_CMD_RESET` — Reset counter to 0
- `TIM_CMD_CLEAR_EVENT` — Clear event flag

**Returns:** `TIM_STATUS_OK` or error code

---

#### `TIM_Status_t TIM_Close(TIM_Handle_t *handle)`
Closes timer device and releases resources.

```c
TIM_Close(h);  // Handle invalid after this
```

**Returns:** `TIM_STATUS_OK` or error code

---

## 📁 Project Structure

```
01_timer_basic/
├── Core/
│   ├── Inc/                    # Header files
│   │   ├── tim_driver.h        # Timer driver API (PUBLIC)
│   │   └── [CMSIS headers]
│   │
│   └── Src/                    # Source files
│       ├── main.c              # Application entry point
│       ├── tim_driver.c        # Timer driver implementation
│       ├── syscalls.c          # System call stubs
│       └── [startup files]
│
├── Linker/
│   └── STM32F401RETX_FLASH.ld  # Memory layout
│
├── Makefile                    # Build system
└── README.md            
```

### Key Files

#### Core/Inc/tim_driver.h
- **Purpose:** Public driver API (hardware-agnostic)
- **Contains:** Function declarations, enums, opaque types
- **Includes:** Only `<stdint.h>`, `<stdbool.h>` (no hardware headers)
- **Does NOT Include:** Any STM32-specific headers or types

#### Core/Src/tim_driver.c
- **Purpose:** Private driver implementation
- **Contains:** Hardware details, state management, ISRs
- **Includes:** `stm32f401xe.h` (PRIVATE, not in public header)
- **Key Internals:**
  - `TIM_Handle_t_Internal` — Full structure with hardware types
  - `g_timer_pool[]` — Static allocation pool (4 timers)
  - `TIM2_IRQHandler()`, etc. — Minimal ISRs
- **Hardware Isolation:** All hardware details hidden here

#### Core/Src/main.c
- **Purpose:** Application using the driver
- **Contains:** LED control, main polling loop
- **Includes:** Only `tim_driver.h` (no STM32 types)
- **Does NOT Include:** `stm32f401xe.h`, `stm32f4xx.h`
- **Key Features:** Pure driver API usage, event polling loop, no callbacks

---

## 🚀 Build & Flash Instructions

### Using Makefile

```bash
cd 01_timer_basic

# Clean and build
make clean
make all

# Build artifacts in build/ directory:
# - 01_bare_metal_timer.elf    (Debuggable ELF with symbols)
# - 01_bare_metal_timer.hex    (Intel HEX format)
# - 01_bare_metal_timer.bin    (Raw binary for st-flash)
```

### Using STM32CubeIDE

1. Import project: File → Import → Existing Projects → Select `01_timer_basic`
2. Build: Project → Build All (Ctrl+B)
3. Run: Right-click project → Run As → STM32 C/C++ Application

### Flashing to Board

#### Method 1: st-flash (Command Line)
```bash
st-flash write build/01_bare_metal_timer.bin 0x8000000
```

#### Method 2: STM32CubeIDE
Automatically handled via Debug/Run configurations

#### Method 3: OpenOCD
```bash
openocd -f board/stm32f4discovery.cfg \
  -c "program build/01_bare_metal_timer.elf verify reset exit"
```

---

## ✅ Expected Behavior

### LED Behavior
- **LED starts OFF** after flashing
- **After 3 seconds:** LED turns ON
- **After 6 seconds:** LED turns OFF
- **After 9 seconds:** LED turns ON
- Pattern repeats indefinitely

### Timing Verification
Use a stopwatch to verify:
- Count blinks in 60 seconds → Should be **20 toggles** (10 ON→OFF cycles)
- Each state should last exactly **3.00 seconds**

---

## 🔧 Troubleshooting

### LED not blinking?

1. **Check physical hardware:**
   - Is LED connected to PA5?
   - Try toggling LED in a delay loop (verify GPIO works)

2. **Check clock configuration:**
   - System clock should be 84 MHz
   - Verify `SystemClock_Config()` in system_stm32f4xx.c

3. **Check timer configuration:**
   - Prescaler = 83 (for 1MHz from 84MHz)
   - Period = 3000 ms

4. **Check interrupts:**
   - Verify NVIC interrupt is enabled
   - Check interrupt priority is set

### Wrong timing (not 3 seconds)?

1. **Verify system clock:**
   ```c
   uint32_t sys_clock = SystemCoreClock;  // Should be 84000000
   ```

2. **Check prescaler math:**
   - 84 MHz / (prescaler+1) = 1 MHz tick
   - For 1 MHz: prescaler = 83

3. **Measure with oscilloscope:**
   - Connect logic analyzer to PA5
   - Verify exactly 3.000 second period

### Build errors?

```bash
# Clean rebuild
make clean
make all
```

- Ensure CMSIS headers are in `Core/Inc/`
- Check `Makefile` paths for `-I` flags
- Verify `STM32F401RETX_FLASH.ld` path

---

## 🧪 Testing

### Functional Verification
```c
// Manual verification in main.c
TIM_Handle_t *h = TIM_Open(TIM_ID_2);
assert(h != NULL);

TIM_Config_t config = {.prescaler = 83, .period_ms = 1000, .priority = 5};
assert(TIM_Write(h, &config) == TIM_STATUS_OK);
assert(TIM_Control(h, TIM_CMD_START, NULL) == TIM_STATUS_OK);

// Poll for events
TIM_Data_t data;
while(1) {
    TIM_Read(h, &data);
    if (data.event_occurred) {
        LED_Toggle();
        TIM_Control(h, TIM_CMD_CLEAR_EVENT, NULL);
    }
}
```

### Multiple Timer Test
```c
// Can open multiple timers from same pool
TIM_Handle_t *h1 = TIM_Open(TIM_ID_2);  // ✅ First timer
TIM_Handle_t *h2 = TIM_Open(TIM_ID_3);  // ✅ Second timer
TIM_Handle_t *h3 = TIM_Open(TIM_ID_4);  // ✅ Third timer
TIM_Handle_t *h4 = TIM_Open(TIM_ID_5);  // ✅ Fourth timer
TIM_Handle_t *h5 = TIM_Open(TIM_ID_2);  // ❌ Fails (already allocated)
```

---

## 📊 Compliance & Metrics

### Device Driver Philosophy Compliance
- ✅ **Component 1:** Register overlay struct (TIM_Handle_t_Internal)
- ✅ **Component 2:** State tracking (TIM_State_t, event_flag, event_counter)
- ✅ **Component 3:** Hardware init routine (TIM_Open)
- ✅ **Component 4:** API routines (TIM_Open/Write/Read/Control/Close)
- ✅ **Component 5:** ISR routines (TIM2_IRQHandler, minimal design)

---

## 🔄 Next Steps

1. Flash to STM32F401RE board
2. Verify LED toggles every 3 seconds
3. Experiment with different timers (TIM3, TIM4, TIM5)
4. Try different periods (modify `config.period_ms`)
5. Proceed to **Project 02: Software Timers** to learn:
   - Multiple concurrent timers
   - Software timer management
   - Task scheduling without RTOS
   - Callback-based events

---

## 📚 Learning Resources

### Documentation
- [STM32F401 Reference Manual](https://www.st.com/resource/en/reference_manual/rm0368-stm32f401xbc-and-stm32f401xde-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
- [STM32F401 Datasheet](https://www.st.com/resource/en/datasheet/stm32f401re.pdf)
- [Cortex-M4 Programming Manual](https://www.st.com/resource/en/programming_manual/pm0214-stm32-cortexm4-mcus-and-mpus-programming-manual-stmicroelectronics.pdf)

### Device Driver Philosophy
- **Book:** "Programming Embedded Systems in C and C++" by Michael Barr
  - Chapter 7: Peripherals (Device Driver Philosophy)
  - Chapter 7.2: The Device Driver Philosophy (5-point pattern)
  - Chapter 7.3: A Simple Timer Driver
