/**
 ******************************************************************************
 * @file    tim_driver.c
 * @brief   Bare-metal TIM2 driver implementation for STM32F401RE
 * @date    2026-01-10
 ******************************************************************************
 */

#include "tim_driver.h"

/**
 * @brief Initialize TIM2 with prescaler and period
 * @param config: Timer configuration
 */
void TIM2_Init(TIM_Config_t *config)
{
    /* 1. Enable TIM2 clock in RCC */
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

    /* 2. Disable counter during configuration */
    TIM2->CR1 &= ~TIM_CR1_CEN;

    /* 3. Set prescaler (PSC register) */
    TIM2->PSC = config->prescaler;

    /* 4. Set auto-reload value (ARR register) */
    TIM2->ARR = config->period;

    /* 5. Reset counter to 0 */
    TIM2->CNT = 0;

    /* 6. Enable update interrupt */
    TIM2->DIER |= TIM_DIER_UIE;

    /* 7. Generate update event to load PSC and ARR values */
    TIM2->EGR |= TIM_EGR_UG;

    /* 8. Clear update interrupt flag (set by update event) */
    TIM2->SR &= ~TIM_SR_UIF;

    /* 9. Configure NVIC for TIM2 interrupt */
    NVIC_SetPriority(TIM2_IRQn, 0);  // Highest priority
    NVIC_EnableIRQ(TIM2_IRQn);       // Enable interrupt in NVIC

    /* 10. Set timer to count upward */
    TIM2->CR1 &= ~TIM_CR1_DIR;  // Upcounting mode
}

/**
 * @brief Start TIM2 counter
 */
void TIM2_Start(void)
{
    TIM2->CR1 |= TIM_CR1_CEN;  // Counter enable bit
}

/**
 * @brief Stop TIM2 counter
 */
void TIM2_Stop(void)
{
    TIM2->CR1 &= ~TIM_CR1_CEN;  // Clear counter enable bit
}

/**
 * @brief Get current counter value
 * @retval Current CNT register value
 */
uint32_t TIM2_GetCounter(void)
{
    return TIM2->CNT;
}

/**
 * @brief TIM2 interrupt handler
 * This function is called automatically when TIM2 interrupt occurs
 */
void TIM2_IRQHandler(void)
{
    /* Check if update interrupt flag is set */
    if (TIM2->SR & TIM_SR_UIF)
    {
        /* Clear the interrupt flag (write 0 to clear) */
        TIM2->SR &= ~TIM_SR_UIF;

        /* Call user-defined callback function */
        TIM2_Callback();
    }
}

/**
 * @brief Weak callback function - user overrides this in main.c
 * This function is called every time timer overflows
 */
__attribute__((weak)) void TIM2_Callback(void)
{
    /* Default implementation: do nothing */
    /* User will override this function in main.c */
}

