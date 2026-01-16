/**
 ******************************************************************************
 * @file    tim_driver.h
<<<<<<< HEAD
 * @brief   Bare-metal TIM2 driver header for STM32F401RE
 * @date    2026-01-10
=======
 * @brief   STM32F4 TIM2 Timer Driver Header

 * @date    2026-01-16
>>>>>>> 751281c (Modified driver framework)
 ******************************************************************************
 */

#ifndef TIM_DRIVER_H
#define TIM_DRIVER_H

#include "stm32f401xe.h"
#include <stdint.h>

/**
 * @brief TIM2 configuration structure
 */
typedef struct {
    uint16_t prescaler;    /**< Timer prescaler value (0-65535) */
    uint32_t period;       /**< Auto-reload period value (0-4294967295) */
} TIM_Config_t;

/**
 * @brief TIM2 callback function pointer type
 * Function signature for user callbacks invoked from TIM2 ISR
 */
typedef void (*TIM_Callback_t)(void);

/**
 * @brief Initialize TIM2 peripheral with specified configuration
 * @param config Pointer to timer configuration structure
 * @note This function enables TIM2 clock, configures registers, and sets up interrupts
 */
void TIM2_Init(const TIM_Config_t *config);

/**
 * @brief Register callback function for TIM2 update interrupts
 * @param callback Function pointer to user callback (NULL to disable callback)
 * @note Callback is invoked from TIM2_IRQHandler when timer period elapses
 */
void TIM2_RegisterCallback(TIM_Callback_t callback);

/**
 * @brief Start TIM2 counter
 * @note Timer begins counting and generating interrupts based on configuration
 */
void TIM2_Start(void);

/**
 * @brief Stop TIM2 counter
 * @note Timer stops counting but retains configuration
 */
void TIM2_Stop(void);

/**
 * @brief Get current TIM2 counter value
 * @return Current counter value (0 to period-1)
 */
uint32_t TIM2_GetCounter(void);

/**
 * @brief TIM2 interrupt service routine
 * @note Called automatically by NVIC when TIM2 interrupt occurs
 * @note Clears interrupt flags and invokes registered user callback
 */
void TIM2_IRQHandler(void);

#endif /* TIM_DRIVER_H */

