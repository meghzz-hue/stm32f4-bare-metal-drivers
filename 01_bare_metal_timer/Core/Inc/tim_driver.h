/**
 ******************************************************************************
 * @file    tim_driver.h
 * @brief   Bare-metal Timer driver header
 *          Implements device driver philosophy: open/write/read/control/close
 *          Hardware-agnostic public API; hardware details hidden in .c
 * @date    2026-01-31
 ******************************************************************************
 */

#ifndef TIM_DRIVER_H
#define TIM_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

/* ---------------------------------------------------------------------------
 * TIMER DEVICE ENUMERATION (Hardware-independent ID)
 * ---------------------------------------------------------------------------*/
/**
 * @brief Timer device identifier (opaque to application)
 * Maps to hardware peripheral but application need not know implementation
 */
typedef enum {
    TIM_ID_2 = 0,  /**< Timer 2 */
    TIM_ID_3 = 1,  /**< Timer 3 */
    TIM_ID_4 = 2,  /**< Timer 4 */
    TIM_ID_5 = 3,  /**< Timer 5 */
} TIM_ID_t;

/* ---------------------------------------------------------------------------
 * STATUS & STATE
 * ---------------------------------------------------------------------------*/
typedef enum {
    TIM_STATUS_OK           = 0x00,
    TIM_STATUS_ERROR        = 0x01,
    TIM_STATUS_INVALID_CFG  = 0x02,
    TIM_STATUS_NOT_INIT     = 0x03,
    TIM_STATUS_ALREADY_INIT = 0x04
} TIM_Status_t;

typedef enum {
    TIM_STATE_UNINITIALIZED = 0x00,
    TIM_STATE_INITIALIZED   = 0x01,
    TIM_STATE_RUNNING       = 0x02,
    TIM_STATE_STOPPED       = 0x03
} TIM_State_t;

/* ---------------------------------------------------------------------------
 * CONFIG & DATA STRUCTURES
 * ---------------------------------------------------------------------------*/
typedef struct {
    uint16_t prescaler;    /**< PSC register value */
    uint32_t period;       /**< ARR register value */
    uint8_t priority;      /**< NVIC priority (0 = highest) */
} TIM_Config_t;

/**
 * @brief Timer control commands for TIM_Control() ioctl
 * TIM_CMD_START    - Enable counter and enter RUNNING state
 * TIM_CMD_STOP     - Disable counter and enter STOPPED state
 * TIM_CMD_RESET    - Clear counter and event flags
 * TIM_CMD_CLEAR_EVENT - Clear event flag (app must call after handling event)
 */
typedef enum {
    TIM_CMD_START = 0,      /**< Start timer */
    TIM_CMD_STOP,           /**< Stop timer */
    TIM_CMD_RESET,          /**< Reset counter */
    TIM_CMD_CLEAR_EVENT     /**< Clear event flag */
} TIM_Command_t;

typedef struct {
    uint32_t counter_value;/**< Snapshot of CNT register */
    uint32_t event_count;  /**< Number of update events seen */
    TIM_State_t state;     /**< Current driver state */
    uint8_t event_occurred;/**< Non-zero if new event pending */
} TIM_Data_t;

/* ---------------------------------------------------------------------------
 * OPAQUE HANDLE (internals hidden from application)
 * ---------------------------------------------------------------------------*/
/**
 * @brief Opaque timer device handle
 * Application receives pointer but never accesses internals.
 * Defined in tim_driver.c; only pointer used by application.
 */
typedef struct TIM_Handle TIM_Handle_t;

/* ---------------------------------------------------------------------------
 * PUBLIC DRIVER API (File-like interface)
 * ---------------------------------------------------------------------------*/

/**
 * @brief Open timer device and allocate a handle
 * @param timer_id: Timer device ID (TIM_ID_2, TIM_ID_3, etc.)
 * @return Pointer to allocated handle or NULL on failure
 *
 * Behavior:
 *  - Allocates handle from static pool (no malloc)
 *  - Initializes hardware (clock, registers)
 *  - Returns handle for later write/read/control/close calls
 */
TIM_Handle_t *TIM_Open(TIM_ID_t timer_id);

/**
 * @brief Configure timer device (write operation)
 * @param handle: Device handle from TIM_Open
 * @param config: Prescaler, period, and priority configuration
 * @return Status code
 *
 * Configures the timer with prescaler, auto-reload period, and NVIC priority.
 * Must be called after TIM_Open and before TIM_Control(START).
 */
TIM_Status_t TIM_Write(TIM_Handle_t *handle, const TIM_Config_t *config);

/**
 * @brief Read device state and any pending events (read operation)
 * @param handle: Device handle from TIM_Open
 * @param data_out: Snapshot of counter, event count, state, and event flag
 * @return Status code
 *
 * Returns current device state without side effects.
 * Application polls this to detect timer events.
 */
TIM_Status_t TIM_Read(TIM_Handle_t *handle, TIM_Data_t *data_out);

/**
 * @brief Control device operations (ioctl)
 * @param handle: Device handle from TIM_Open
 * @param cmd: Control command (TIM_CMD_START, TIM_CMD_STOP, etc.)
 * @param arg: Optional command argument (NULL for most commands)
 * @return Status code
 *
 * Issues control commands to the device:
 *  - TIM_CMD_START     : Enable counter, enter RUNNING state
 *  - TIM_CMD_STOP      : Disable counter, enter STOPPED state
 *  - TIM_CMD_RESET     : Clear counter and event flags
 *  - TIM_CMD_CLEAR_EVENT: Clear event flag after application processes event
 */
TIM_Status_t TIM_Control(TIM_Handle_t *handle, TIM_Command_t cmd, void *arg);

/**
 * @brief Close device and release handle (close operation)
 * @param handle: Device handle from TIM_Open
 * @return Status code
 *
 * Stops timer, disables clock and interrupts, returns handle to pool.
 */
TIM_Status_t TIM_Close(TIM_Handle_t *handle);

#endif /* TIM_DRIVER_H */
