/**
 ******************************************************************************
 * @file    tim_driver.h
 * @brief   Bare-metal TIM2 driver header for STM32F401RE
 * @author  Your Name
 * @date    2026-01-10
 ******************************************************************************
 */

#ifndef TIM_DRIVER_H
#define TIM_DRIVER_H

#include "stm32f401xe.h"
#include <stdint.h>

/**
 * @brief Timer configuration structure
 */
typedef struct {
    uint16_t prescaler;    // Timer prescaler (0-65535)
    uint32_t period;       // Auto-reload value (0-65535 for TIM2-5 in 16-bit mode)
} TIM_Config_t;

/**
 * @brief Initialize TIM2 with given configuration
 * @param config: Pointer to timer configuration structure
 * @retval None
 */
void TIM2_Init(TIM_Config_t *config);

/**
 * @brief Start TIM2 counter
 * @retval None
 */
void TIM2_Start(void);

/**
 * @brief Stop TIM2 counter
 * @retval None
 */
void TIM2_Stop(void);

/**
 * @brief Get current counter value
 * @retval Current counter value
 */
uint32_t TIM2_GetCounter(void);

/**
 * @brief TIM2 interrupt handler (called from vector table)
 * @retval None
 */
void TIM2_IRQHandler(void);

/**
 * @brief User callback function (weak, override in main.c)
 * @retval None
 */
void TIM2_Callback(void);

#endif /* TIM_DRIVER_H */
