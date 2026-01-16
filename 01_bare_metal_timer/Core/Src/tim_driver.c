/**
 ******************************************************************************
 * @file    tim_driver.c
<<<<<<< HEAD
 * @brief   Bare-metal TIM2 driver implementation for STM32F401RE
 * @date    2026-01-10
 ******************************************************************************
=======
 * @brief   STM32F4 TIM2 Timer Driver Implementation
 * @date    2026-01-16
 *******************************************************************************
>>>>>>> 751281c (Modified driver framework)
 */

#include "tim_driver.h"
#include <stddef.h>

/* Private variable to store registered user callback */
static TIM_Callback_t user_callback = NULL;

/**
 * @brief Initialize TIM2 peripheral with specified configuration
 * @param config Pointer to timer configuration structure
 * @note This function enables TIM2 clock, configures registers, and sets up interrupts
 */
void TIM2_Init(const TIM_Config_t *config)
{
    /* Reset callback to NULL on initialization */
    user_callback = NULL;

    /* 1. Enable TIM2 clock in RCC APB1 peripheral clock register */
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

    /* 2. Disable counter during configuration */
    TIM2->CR1 &= ~TIM_CR1_CEN;

    /* 3. Set prescaler register (PSC) */
    TIM2->PSC = config->prescaler;

    /* 4. Set auto-reload register (ARR) */
    TIM2->ARR = config->period;

    /* 5. Reset counter register to 0 */
    TIM2->CNT = 0;

    /* 6. Enable update interrupt in DIER register */
    TIM2->DIER |= TIM_DIER_UIE;

    /* 7. Generate update event to load PSC and ARR immediately */
    TIM2->EGR |= TIM_EGR_UG;

    /* 8. Clear update interrupt flag set by update event */
    TIM2->SR &= ~TIM_SR_UIF;

    /* 9. Configure NVIC for TIM2 interrupt */
    NVIC_SetPriority(TIM2_IRQn, 0);  /* Highest priority */
    NVIC_EnableIRQ(TIM2_IRQn);       /* Enable interrupt in NVIC */

    /* 10. Configure timer for upcounting mode */
    TIM2->CR1 &= ~TIM_CR1_DIR;
}

/**
 * @brief Register callback function for TIM2 update interrupts
 * @param callback Function pointer to user callback (NULL to disable callback)
 * @note Callback is invoked from TIM2_IRQHandler when timer period elapses
 */
void TIM2_RegisterCallback(TIM_Callback_t callback)
{
    user_callback = callback;
}

/**
 * @brief Start TIM2 counter
 * @note Timer begins counting and generating interrupts based on configuration
 */
void TIM2_Start(void)
{
    TIM2->CR1 |= TIM_CR1_CEN;
}

/**
 * @brief Stop TIM2 counter
 * @note Timer stops counting but retains configuration
 */
void TIM2_Stop(void)
{
    TIM2->CR1 &= ~TIM_CR1_CEN;
}

/**
 * @brief Get current TIM2 counter value
 * @return Current counter value (0 to period-1)
 */
uint32_t TIM2_GetCounter(void)
{
    return TIM2->CNT;
}

/**
 * @brief TIM2 interrupt service routine
 * @note Called automatically by NVIC when TIM2 interrupt occurs
 * @note Clears interrupt flags and invokes registered user callback
 */
void TIM2_IRQHandler(void)
{
    /* Check if update interrupt flag is set */
    if (TIM2->SR & TIM_SR_UIF)
    {
        /* Clear the update interrupt flag */
        TIM2->SR &= ~TIM_SR_UIF;

        /* Invoke registered user callback if available */
        if (user_callback != NULL)
        {
            user_callback();
        }
    }
}
<<<<<<< HEAD

/**
 * @brief Weak callback function - user overrides this in main.c
 * This function is called every time timer overflows
 */
__attribute__((weak)) void TIM2_Callback(void)
{
    /* Default implementation: do nothing */
    /* User will override this function in main.c */
}

=======
>>>>>>> 751281c (Modified driver framework)
