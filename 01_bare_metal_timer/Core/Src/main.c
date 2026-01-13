/**
 ******************************************************************************
 * @file    main.c
 * @brief   Bare-metal timer application - LED blinks every 3 seconds
 * @date    2026-01-12
 ******************************************************************************
 */

#include "stm32f401xe.h"
#include "system_stm32f4xx.h"
#include "tim_driver.h"

/* GPIO Configuration */
#define LED_PORT        GPIOA
#define LED_PIN         5

/* GPIO Helper Macros */
#define LED_ON          (LED_PORT->ODR |= (1U << LED_PIN))
#define LED_OFF         (LED_PORT->ODR &= ~(1U << LED_PIN))
#define LED_TOGGLE      (LED_PORT->ODR ^= (1U << LED_PIN))

/* Function Prototypes */
void GPIO_Init(void);
void SystemClock_Config(void);
uint32_t Get_APB1_Timer_Clock(void);

/* Global variable for debugging */
volatile uint32_t toggle_count = 0;

/**
 * @brief Main application entry point
 */
int main(void)
{
    /* 1. Configure system clock */
    SystemClock_Config();

    /* 2. Initialize GPIO for LED (PA5) */
    GPIO_Init();

    /* 3. Get actual APB1 timer clock frequency */
    uint32_t apb1_timer_clk = Get_APB1_Timer_Clock();

    /* 4. Configure timer for 3-second interrupt
     * For 16 MHz APB1 timer clock:
     * Prescaler = 1599 -> 16 MHz / 1600 = 10,000 Hz
     * Period = 29999 -> 10,000 Hz / 30,000 = 0.333 Hz = 3 seconds
     */
    TIM_Config_t timer_config;

    if (apb1_timer_clk == 84000000) {
        // If APB1 timer clock is 84 MHz (PLL configured)
        timer_config.prescaler = 8399;   // 84 MHz / 8400 = 10,000 Hz
        timer_config.period = 29999;     // 10 kHz / 30,000 = 3 seconds
    } else {
        // If APB1 timer clock is 16 MHz (HSI default)
        timer_config.prescaler = 1599;   // 16 MHz / 1600 = 10,000 Hz
        timer_config.period = 29999;     // 10 kHz / 30,000 = 3 seconds
    }

    TIM2_Init(&timer_config);

    /* 5. Start the timer */
    TIM2_Start();

    /* 6. Main loop - CPU is free to do other tasks */
    while (1)
    {
        /* Infinite loop - interrupts handle LED toggling */
        /* You can add other tasks here */

        // Optional: Low-power mode
        // __WFI();  // Wait for interrupt
    }
}

/**
 * @brief Initialize GPIO PA5 as output for LED
 */
void GPIO_Init(void)
{
    /* Enable GPIOA clock */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    /* Configure PA5 as output */
    LED_PORT->MODER &= ~(3U << (LED_PIN * 2));  // Clear mode bits
    LED_PORT->MODER |= (1U << (LED_PIN * 2));   // Set as output (01)

    /* Configure as push-pull output */
    LED_PORT->OTYPER &= ~(1U << LED_PIN);       // Push-pull (0)

    /* Set speed to low */
    LED_PORT->OSPEEDR &= ~(3U << (LED_PIN * 2)); // Low speed (00)

    /* No pull-up/pull-down */
    LED_PORT->PUPDR &= ~(3U << (LED_PIN * 2));   // No pull (00)

    /* Initial state: LED OFF */
    LED_OFF;
}

/**
 * @brief Get APB1 timer clock frequency
 * @retval APB1 timer clock in Hz
 */
uint32_t Get_APB1_Timer_Clock(void)
{
    uint32_t pclk1;
    uint32_t apb1_prescaler;

    /* Get system clock (default is 16 MHz HSI) */
    SystemCoreClockUpdate();

    /* Get APB1 prescaler from RCC_CFGR register */
    apb1_prescaler = (RCC->CFGR >> 10) & 0x7;  // PPRE1[2:0] bits

    /* Calculate PCLK1 (APB1 clock) */
    if (apb1_prescaler < 4) {
        pclk1 = SystemCoreClock;  // No division
    } else {
        pclk1 = SystemCoreClock >> (apb1_prescaler - 3);  // Divide by 2, 4, 8, or 16
    }

    /* Timer clock is 2x APB1 clock if APB1 prescaler != 1 */
    if (apb1_prescaler >= 4) {
        return pclk1 * 2;  // Timer clock = 2 × PCLK1
    } else {
        return pclk1;      // Timer clock = PCLK1
    }
}

/**
 * @brief Configure system clock to 84 MHz using HSI and PLL
 */
void SystemClock_Config(void)
{
    /* Enable HSI (16 MHz internal oscillator) */
    RCC->CR |= RCC_CR_HSION;
    while (!(RCC->CR & RCC_CR_HSIRDY));  // Wait until HSI is ready

    /* Configure PLL: HSI / 16 × 168 / 2 = 84 MHz */
    RCC->PLLCFGR = 0;  // Reset value
    RCC->PLLCFGR |= (16 << RCC_PLLCFGR_PLLM_Pos);   // PLLM = 16 (16 MHz / 16 = 1 MHz)
    RCC->PLLCFGR |= (168 << RCC_PLLCFGR_PLLN_Pos);  // PLLN = 168 (1 MHz × 168 = 168 MHz)
    RCC->PLLCFGR |= (0 << RCC_PLLCFGR_PLLP_Pos);    // PLLP = 2 (168 MHz / 2 = 84 MHz)
    RCC->PLLCFGR &= ~RCC_PLLCFGR_PLLSRC;            // PLL source = HSI

    /* Enable PLL */
    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY));  // Wait until PLL is ready

    /* Configure Flash latency for 84 MHz */
    FLASH->ACR = FLASH_ACR_LATENCY_2WS | FLASH_ACR_ICEN | FLASH_ACR_DCEN | FLASH_ACR_PRFTEN;

    /* Configure AHB, APB1, APB2 prescalers */
    RCC->CFGR |= RCC_CFGR_HPRE_DIV1;   // AHB = 84 MHz
    RCC->CFGR |= RCC_CFGR_PPRE1_DIV2;  // APB1 = 42 MHz (TIM2 clock = 84 MHz)
    RCC->CFGR |= RCC_CFGR_PPRE2_DIV1;  // APB2 = 84 MHz

    /* Select PLL as system clock */
    RCC->CFGR |= RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);  // Wait until PLL is system clock

    /* Update SystemCoreClock variable */
    SystemCoreClockUpdate();
}

/**
 * @brief Timer callback - called every 3 seconds
 * This function overrides the weak function in tim_driver.c
 */
void TIM2_Callback(void)
{
    /* Toggle LED state */
    LED_TOGGLE;

    /* Increment debug counter */
    toggle_count++;
}

/**
 * @brief Error handler - infinite loop
 */
void Error_Handler(void)
{
    /* Disable interrupts */
    __disable_irq();

    /* Infinite loop */
    while (1)
    {
        /* Stay here if error occurs */
    }
}

