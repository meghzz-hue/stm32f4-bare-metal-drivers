/**
 ******************************************************************************
 * @file    main.c
 * @brief   Bare-metal timer application - LED blinks every 3 seconds
 *          Uses device driver pattern with callback registration
 * @date    2026-01-31
 ******************************************************************************
 */

#include "stm32f401xe.h"
#include "system_stm32f4xx.h"
#include "tim_driver.h"
#include <stddef.h>

/* ============================================================================
 * GPIO CONFIGURATION FOR LED
 * ============================================================================*/

#define LED_PORT        GPIOA
#define LED_PIN         5

/* GPIO Helper Macros */
#define LED_ON()        (LED_PORT->ODR |= (1U << LED_PIN))
#define LED_OFF()       (LED_PORT->ODR &= ~(1U << LED_PIN))
#define LED_TOGGLE()    (LED_PORT->ODR ^= (1U << LED_PIN))

/* ============================================================================
 * GLOBAL VARIABLES & STATISTICS
 * ============================================================================*/

static TIM_Handle_t *g_timer2_handle = NULL;
volatile uint32_t g_led_toggle_count = 0;

/* ============================================================================
 * FUNCTION PROTOTYPES
 * ============================================================================*/

static void GPIO_LED_Init(void);
static void SystemClock_Config(void);
static uint32_t Get_APB1_Timer_Clock(void);
void Error_Handler(void);

/* ============================================================================
 * MAIN APPLICATION ENTRY POINT
 * ============================================================================*/

/**
 * @brief Main application entry point
 * Initializes hardware and starts timer with callback
 */
int main(void)
{
    TIM_Status_t status;
    TIM_Config_t timer_config;

    /* Step 1: Configure system clock to 84 MHz */
    SystemClock_Config();

    /* Step 2: Initialize GPIO for LED on PA5 */
    GPIO_LED_Init();

    /* Step 3: Get actual APB1 timer clock frequency */
    uint32_t apb1_timer_clk = Get_APB1_Timer_Clock();

    /* Step 4: Prepare timer configuration */
    if (apb1_timer_clk == 84000000) {
        timer_config.prescaler = 8399;   /* 84 MHz / 8400 = 10 kHz */
        timer_config.period = 29999;     /* 10 kHz / 30,000 = 3 seconds */
    } else {
        timer_config.prescaler = 1599;   /* 16 MHz / 1600 = 10 kHz */
        timer_config.period = 29999;
    }
    timer_config.priority = 0;

    /* Step 5: Open and configure timer */
    g_timer2_handle = TIM_Open(TIM_ID_2);
    if (g_timer2_handle == NULL) {
        Error_Handler();
    }

    status = TIM_Write(g_timer2_handle, &timer_config);
    if (status != TIM_STATUS_OK) {
        Error_Handler();
    }

    /* Step 6: Start timer */
    status = TIM_Control(g_timer2_handle, TIM_CMD_START, NULL);
    if (status != TIM_STATUS_OK) {
        Error_Handler();
    }

    /* Step 7: Main loop - actively poll for events */
    TIM_Data_t timer_data;
    while (1) {
        TIM_Read(g_timer2_handle, &timer_data);
        if (timer_data.event_occurred) {
            LED_TOGGLE();
            g_led_toggle_count++;
            TIM_Control(g_timer2_handle, TIM_CMD_CLEAR_EVENT, NULL);
        }

        if (timer_data.state != TIM_STATE_RUNNING) {
            Error_Handler();
        }
    }
}

/* No callback used in refactored design; main polls the driver */

/* ============================================================================
 * HARDWARE INITIALIZATION FUNCTIONS
 * ============================================================================*/

/**
 * @brief Initialize GPIO PA5 as output for LED
 */
static void GPIO_LED_Init(void)
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
    LED_OFF();
}

/**
 * @brief Get APB1 timer clock frequency
 * APB1 prescaler affects both APB1 clock and timer clock
 *
 * @retval APB1 timer clock in Hz
 */
static uint32_t Get_APB1_Timer_Clock(void)
{
    uint32_t pclk1;
    uint32_t apb1_prescaler;

    /* Update system clock variable */
    SystemCoreClockUpdate();

    /* Get APB1 prescaler from RCC_CFGR register (PPRE1[2:0] bits 12-10) */
    apb1_prescaler = (RCC->CFGR >> 10) & 0x7;

    /* Calculate PCLK1 (APB1 clock) based on prescaler */
    if (apb1_prescaler < 4) {
        /* No division - prescaler 0,1,2,3 = divide by 1 */
        pclk1 = SystemCoreClock;
    } else {
        /* Division by 2, 4, 8, or 16 */
        pclk1 = SystemCoreClock >> (apb1_prescaler - 3);
    }

    /* Timer clock is 2x APB1 clock if APB1 has prescaler != 1 */
    if (apb1_prescaler >= 4) {
        return pclk1 * 2;  /* Timer clock = 2 × PCLK1 */
    } else {
        return pclk1;      /* Timer clock = PCLK1 */
    }
}

/**
 * @brief Configure system clock to 84 MHz using HSI and PLL
 * Initializes clock tree and enables peripheral clocks
 */
static void SystemClock_Config(void)
{
    /* Enable HSI (High-Speed Internal 16 MHz oscillator) */
    RCC->CR |= RCC_CR_HSION;
    while (!(RCC->CR & RCC_CR_HSIRDY));  /* Wait until HSI is stable */

    /* Configure PLL: HSI / 16 × 168 / 2 = 84 MHz
     * PLLM = 16: Input frequency = 16 MHz / 16 = 1 MHz
     * PLLN = 168: Output frequency = 1 MHz × 168 = 168 MHz
     * PLLP = 2: Final frequency = 168 MHz / 2 = 84 MHz
     */
    RCC->PLLCFGR = 0;
    RCC->PLLCFGR |= (16 << RCC_PLLCFGR_PLLM_Pos);   /* PLLM = 16 */
    RCC->PLLCFGR |= (168 << RCC_PLLCFGR_PLLN_Pos);  /* PLLN = 168 */
    RCC->PLLCFGR |= (0 << RCC_PLLCFGR_PLLP_Pos);    /* PLLP = 2 */
    RCC->PLLCFGR &= ~RCC_PLLCFGR_PLLSRC;            /* PLL source = HSI */

    /* Enable PLL */
    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY));  /* Wait until PLL is locked */

    /* Configure Flash latency for 84 MHz operation */
    FLASH->ACR = FLASH_ACR_LATENCY_2WS  /* 2 wait states for 84 MHz */
               | FLASH_ACR_ICEN          /* Instruction cache enable */
               | FLASH_ACR_DCEN          /* Data cache enable */
               | FLASH_ACR_PRFTEN;       /* Prefetch buffer enable */

    /* Configure prescalers for AHB, APB1, and APB2 */
    RCC->CFGR |= RCC_CFGR_HPRE_DIV1;    /* AHB prescaler = 1 (84 MHz) */
    RCC->CFGR |= RCC_CFGR_PPRE1_DIV2;   /* APB1 prescaler = 2 (42 MHz, Timer clk = 84 MHz) */
    RCC->CFGR |= RCC_CFGR_PPRE2_DIV1;   /* APB2 prescaler = 1 (84 MHz) */

    /* Select PLL as system clock source */
    RCC->CFGR |= RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);

    /* Update SystemCoreClock global variable */
    SystemCoreClockUpdate();
}

/* ============================================================================
 * ERROR HANDLING
 * ============================================================================*/

/**
 * @brief Error handler - enters infinite loop with interrupts disabled
 * Called when fatal error is detected
 */
void Error_Handler(void)
{
    /* Disable all interrupts */
    __disable_irq();

    /* Infinite loop - system halted */
    while (1) {
        /* Remain here indefinitely */
    }
}
