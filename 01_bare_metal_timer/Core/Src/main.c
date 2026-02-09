/**
 ******************************************************************************
 * @file    main.c
 * @brief   LED blink application using Device Driver Philosophy timer
 * @date    2026-02-06
 ******************************************************************************
 */

#include "stm32f401xe.h"
#include "system_stm32f4xx.h"
#include "tim_driver.h"
#include <stddef.h>

#define LED_PORT        GPIOA
#define LED_PIN         5
#define LED_TOGGLE()    (LED_PORT->ODR ^= (1U << LED_PIN))

static TIM_Handle_t *timer_handle = NULL;
volatile uint32_t toggle_count = 0;

static void gpio_init(void);
static void clock_config(void);
static uint32_t get_timer_clock(void);

int main(void)
{
    TIM_Status_t status;
    TIM_Config_t config;
    TIM_Status_Data_t data;

    clock_config();
    gpio_init();

    uint32_t timer_clk = get_timer_clock();
    
    /* Calculate for 3 second period */
    if (timer_clk == 84000000) {
        config.prescaler = 8399;    /* 84 MHz / 8400 = 10 kHz */
        config.period = 29999;      /* 10 kHz / 30000 = 3 sec */
    } else {
        config.prescaler = 1599;
        config.period = 29999;
    }
    config.priority = 0;

    timer_handle = TIM_Open(TIM_ID_2);
    if (!timer_handle) {
        while(1);
    }

    status = TIM_Write(timer_handle, &config);
    if (status != TIM_OK) {
        while(1);
    }

    status = TIM_Control(timer_handle, TIM_CMD_START);
    if (status != TIM_OK) {
        while(1);
    }

    while (1) {
        TIM_Read(timer_handle, &data);
        
        if (data.event_pending) {
            LED_TOGGLE();
            toggle_count++;
            TIM_Control(timer_handle, TIM_CMD_ACK_EVENT);
        }
    }
}

static void gpio_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    
    LED_PORT->MODER &= ~(3U << (LED_PIN * 2));
    LED_PORT->MODER |= (1U << (LED_PIN * 2));
    LED_PORT->OTYPER &= ~(1U << LED_PIN);
    LED_PORT->OSPEEDR &= ~(3U << (LED_PIN * 2));
    LED_PORT->PUPDR &= ~(3U << (LED_PIN * 2));
    LED_PORT->ODR &= ~(1U << LED_PIN);
}

static uint32_t get_timer_clock(void)
{
    uint32_t pclk1;
    uint32_t apb1_prescaler;
    
    SystemCoreClockUpdate();
    
    apb1_prescaler = (RCC->CFGR >> 10) & 0x7;
    
    if (apb1_prescaler < 4) {
        pclk1 = SystemCoreClock;
    } else {
        pclk1 = SystemCoreClock >> (apb1_prescaler - 3);
    }
    
    if (apb1_prescaler >= 4) {
        return pclk1 * 2;
    } else {
        return pclk1;
    }
}

static void clock_config(void)
{
    RCC->CR |= RCC_CR_HSION;
    while (!(RCC->CR & RCC_CR_HSIRDY));
    
    RCC->PLLCFGR = 0;
    RCC->PLLCFGR |= (16 << RCC_PLLCFGR_PLLM_Pos);
    RCC->PLLCFGR |= (168 << RCC_PLLCFGR_PLLN_Pos);
    RCC->PLLCFGR |= (0 << RCC_PLLCFGR_PLLP_Pos);
    RCC->PLLCFGR &= ~RCC_PLLCFGR_PLLSRC;
    
    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY));
    
    FLASH->ACR = FLASH_ACR_LATENCY_2WS
               | FLASH_ACR_ICEN
               | FLASH_ACR_DCEN
               | FLASH_ACR_PRFTEN;
    
    RCC->CFGR |= RCC_CFGR_HPRE_DIV1;
    RCC->CFGR |= RCC_CFGR_PPRE1_DIV2;
    RCC->CFGR |= RCC_CFGR_PPRE2_DIV1;
    
    RCC->CFGR |= RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);
    
    SystemCoreClockUpdate();
}

void Error_Handler(void)
{
    __disable_irq();
    while (1);
}
