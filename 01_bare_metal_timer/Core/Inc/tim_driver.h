/**
 ******************************************************************************
 * @file    tim_driver.h
 * @brief   Bare-metal Timer driver header for STM32F401RE
 *          Implements device driver pattern with callback registration
 * @date    2026-01-24
 ******************************************************************************
 */

#ifndef TIM_DRIVER_H
#define TIM_DRIVER_H

#include "stm32f401xe.h"
#include <stdint.h>
#include <stdbool.h>

/* ============================================================================
 * TYPE DEFINITIONS - Device Driver Pattern
 * ============================================================================*/

/**
 * @brief Timer status codes
 */
typedef enum {
    TIM_STATUS_OK           = 0x00,    /**< Operation successful */
    TIM_STATUS_ERROR        = 0x01,    /**< Operation failed */
    TIM_STATUS_INVALID_CFG  = 0x02,    /**< Invalid configuration */
    TIM_STATUS_NOT_INIT     = 0x03,    /**< Device not initialized */
    TIM_STATUS_ALREADY_INIT = 0x04     /**< Device already initialized */
} TIM_Status_t;

/**
 * @brief Timer state enumeration
 */
typedef enum {
    TIM_STATE_UNINITIALIZED = 0x00,
    TIM_STATE_INITIALIZED   = 0x01,
    TIM_STATE_RUNNING       = 0x02,
    TIM_STATE_STOPPED       = 0x03
} TIM_State_t;

/**
 * @brief Timer configuration structure
 */
typedef struct {
    uint16_t prescaler;              /**< Timer prescaler (0-65535) */
    uint32_t period;                 /**< Auto-reload value (0-65535 for 16-bit mode) */
    uint8_t priority;                /**< NVIC interrupt priority (0-15) */
} TIM_Config_t;

/**
 * @brief Timer callback function pointer type
 * Called when timer interrupt occurs
 */
typedef void (*TIM_Callback_t)(void);

/**
 * @brief Timer device handle - encapsulates timer state and configuration
 * Implements device driver pattern for state management
 */
typedef struct {
    TIM_TypeDef *instance;           /**< Pointer to timer peripheral (TIM2, TIM3, etc.) */
    TIM_Config_t config;             /**< Current timer configuration */
    TIM_State_t state;               /**< Current device state */
    TIM_Callback_t period_elapsed_cb;/**< User callback for period elapsed event */
    uint32_t event_counter;          /**< Number of timer events occurred */
    IRQn_Type irq_number;            /**< Timer IRQ number */
} TIM_Handle_t;

/* ============================================================================
 * DRIVER INTERFACE FUNCTIONS
 * ============================================================================*/

/**
 * @brief Initialize timer device with given configuration
 * Implements device driver initialization pattern
 *
 * @param handle: Pointer to timer device handle to be initialized
 * @param config: Pointer to timer configuration structure
 * @retval TIM_Status_t: Status code (TIM_STATUS_OK or error code)
 */
TIM_Status_t TIM_Init(TIM_Handle_t *handle, const TIM_Config_t *config);

/**
 * @brief Start timer counter
 *
 * @param handle: Pointer to initialized timer device handle
 * @retval TIM_Status_t: Status code
 */
TIM_Status_t TIM_Start(TIM_Handle_t *handle);

/**
 * @brief Stop timer counter
 *
 * @param handle: Pointer to initialized timer device handle
 * @retval TIM_Status_t: Status code
 */
TIM_Status_t TIM_Stop(TIM_Handle_t *handle);

/**
 * @brief Get current counter value
 *
 * @param handle: Pointer to initialized timer device handle
 * @param counter: Pointer to store counter value
 * @retval TIM_Status_t: Status code
 */
TIM_Status_t TIM_GetCounter(TIM_Handle_t *handle, uint32_t *counter);

/**
 * @brief Register user callback for period elapsed event
 * Implements callback registration pattern
 *
 * @param handle: Pointer to initialized timer device handle
 * @param callback: Function pointer to user callback
 * @retval TIM_Status_t: Status code
 */
TIM_Status_t TIM_RegisterCallback(TIM_Handle_t *handle, TIM_Callback_t callback);

/**
 * @brief Get timer device state
 *
 * @param handle: Pointer to timer device handle
 * @retval TIM_State_t: Current device state
 */
TIM_State_t TIM_GetState(TIM_Handle_t *handle);

/**
 * @brief Get timer event counter (number of interrupts occurred)
 *
 * @param handle: Pointer to timer device handle
 * @retval Number of timer events
 */
uint32_t TIM_GetEventCount(TIM_Handle_t *handle);

/**
 * @brief De-initialize timer device
 * Releases resources and resets state
 *
 * @param handle: Pointer to timer device handle
 * @retval TIM_Status_t: Status code
 */
TIM_Status_t TIM_DeInit(TIM_Handle_t *handle);

/* ============================================================================
 * INTERNAL/IRQ HANDLER FUNCTIONS (Not typically called by user)
 * ============================================================================*/

/**
 * @brief Internal interrupt handler for TIM2
 * Called by vector table - manages all TIM2 registered handles
 */
void TIM2_IRQHandler(void);

/**
 * @brief Internal interrupt handler for TIM3
 * Called by vector table - manages all TIM3 registered handles
 */
void TIM3_IRQHandler(void);

#endif /* TIM_DRIVER_H */
