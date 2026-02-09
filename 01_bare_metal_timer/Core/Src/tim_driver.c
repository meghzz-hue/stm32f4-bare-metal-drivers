/**
 ******************************************************************************
 * @file    tim_driver.c
 * @brief   Timer driver - strict Device Driver Philosophy implementation
 * @date    2026-02-06
 *
 * DEVICE DRIVER PHILOSOPHY - 5 COMPONENTS:
 * 1. Data structure overlaying memory-mapped registers
 * 2. Variables tracking hardware/driver state
 * 3. Initialization routine
 * 4. API routines
 * 5. Interrupt service routines
 ******************************************************************************
 */

#include "tim_driver.h"
#include "stm32f401xe.h"
#include <stddef.h>

/* ===========================================================================
 * POINT 1: Data structure overlaying memory-mapped control/status registers
 * ===========================================================================
 * This struct matches the exact hardware register layout of STM32 timers.
 * Using volatile ensures compiler doesn't optimize away hardware accesses.
 */
typedef struct {
    volatile uint32_t CR1;     /* Control register 1,                  offset 0x00 */
    volatile uint32_t DIER;     /* DMA/Interrupt Enable Register,      offset 0x0C */
    volatile uint32_t SR;       /* Status register,                    offset 0x10 */
    volatile uint32_t EGR;      /* Event Generation Register           offset 0x14 */
    volatile uint32_t CNT;      /* Counter,                            offset 0x24 */
    volatile uint32_t PSC;      /* Prescaler,                          offset 0x28 */
    volatile uint32_t ARR;      /* Auto-reload register,               offset 0x2C */
    uint32_t RESERVED1;         /* Reserved,                           offset 0x30 */
    
} TimerRegs;

/* Bit definitions for registers */
#define TIM_CR1_CEN     (1U << 0)   /* Counter enable */
#define TIM_DIER_UIE    (1U << 0)   /* Update interrupt enable */
#define TIM_SR_UIF      (1U << 0)   /* Update interrupt flag */
#define TIM_EGR_UG      (1U << 0)   /* Update generation */

/* ===========================================================================
 * POINT 2: Variables tracking hardware and driver state
 * ===========================================================================
 * Each timer instance needs:
 * - Pointer to its hardware registers
 * - Current state (idle/running/expired)
 * - Event tracking (has event occurred, how many events)
 * - Configuration (prescaler, period)
 */
typedef struct {
    TimerRegs *regs;                /* Hardware register base address */
    TIM_State_t state;              /* Current driver state */
    volatile uint8_t event_flag;    /* Set by ISR when timer expires */
    uint32_t event_count;           /* Total events since initialization */
    uint8_t initialized;            /* Is this timer initialized? */
    TIM_Config_t config;            /* Current configuration */
    IRQn_Type irq;                  /* IRQ number for NVIC */
} TimerDevice;

/* Static array holding state for all 4 timers */
static TimerDevice timers[4];

/* Track which handles are in use */
static uint8_t handle_used[4] = {0};

/* Internal handle structure (opaque to user) */
struct TIM_Handle {
    uint8_t timer_index;        /* Index into timers array */
};

static struct TIM_Handle handle_pool[4];

/* ===========================================================================
 * Helper functions - map timer ID to hardware and track instances
 * ===========================================================================*/

static TimerRegs* get_timer_base(TIM_ID_t id)
{
    switch (id) {
        case TIM_ID_2: return (TimerRegs*)TIM2;
        case TIM_ID_3: return (TimerRegs*)TIM3;
        case TIM_ID_4: return (TimerRegs*)TIM4;
        case TIM_ID_5: return (TimerRegs*)TIM5;
        default: return NULL;
    }
}

static IRQn_Type get_timer_irq(TIM_ID_t id)
{
    switch (id) {
        case TIM_ID_2: return TIM2_IRQn;
        case TIM_ID_3: return TIM3_IRQn;
        case TIM_ID_4: return TIM4_IRQn;
        case TIM_ID_5: return TIM5_IRQn;
        default: return 0;
    }
}

static void enable_timer_clock(TIM_ID_t id)
{
    switch (id) {
        case TIM_ID_2: RCC->APB1ENR |= RCC_APB1ENR_TIM2EN; break;
        case TIM_ID_3: RCC->APB1ENR |= RCC_APB1ENR_TIM3EN; break;
        case TIM_ID_4: RCC->APB1ENR |= RCC_APB1ENR_TIM4EN; break;
        case TIM_ID_5: RCC->APB1ENR |= RCC_APB1ENR_TIM5EN; break;
    }
}

static void disable_timer_clock(TIM_ID_t id)
{
    switch (id) {
        case TIM_ID_2: RCC->APB1ENR &= ~RCC_APB1ENR_TIM2EN; break;
        case TIM_ID_3: RCC->APB1ENR &= ~RCC_APB1ENR_TIM3EN; break;
        case TIM_ID_4: RCC->APB1ENR &= ~RCC_APB1ENR_TIM4EN; break;
        case TIM_ID_5: RCC->APB1ENR &= ~RCC_APB1ENR_TIM5EN; break;
    }
}

static TimerDevice* find_device_by_regs(TimerRegs *regs)
{
    for (int i = 0; i < 4; i++) {
        if (timers[i].initialized && timers[i].regs == regs) {
            return &timers[i];
        }
    }
    return NULL;
}

/* ===========================================================================
 * POINT 3: Initialization routine - sets hardware to known state
 * ===========================================================================*/

TIM_Handle_t *TIM_Open(TIM_ID_t timer_id)
{
    uint8_t idx = (uint8_t)timer_id;
    
    if (idx >= 4) return NULL;
    if (handle_used[idx]) return NULL;
    
    TimerDevice *dev = &timers[idx];
    TimerRegs *regs = get_timer_base(timer_id);
    
    if (!regs) return NULL;
    
    /* Enable peripheral clock */
    enable_timer_clock(timer_id);
    
    /* Initialize device state */
    dev->regs = regs;
    dev->state = TIM_IDLE;
    dev->event_flag = 0;
    dev->event_count = 0;
    dev->initialized = 1;
    dev->irq = get_timer_irq(timer_id);
    
    /* Mark handle as used */
    handle_used[idx] = 1;
    handle_pool[idx].timer_index = idx;
    
    return &handle_pool[idx];
}

/* ===========================================================================
 * POINT 4: API routines providing interface to device
 * ===========================================================================*/

TIM_Status_t TIM_Write(TIM_Handle_t *handle, const TIM_Config_t *config)
{
    if (!handle || !config) return TIM_ERR_BAD_CFG;
    
    uint8_t idx = handle->timer_index;
    if (idx >= 4) return TIM_ERR_BAD_CFG;
    
    TimerDevice *dev = &timers[idx];
    if (!dev->initialized) return TIM_ERR_NOT_INIT;
    
    /* Only allow reconfiguration when stopped */
    if (dev->state != TIM_IDLE && dev->state != TIM_STOPPED) {
        return TIM_ERR_IN_USE;
    }
    
    /* Store configuration */
    dev->config = *config;
    
    /* Configure hardware registers */
    dev->regs->CR1 = 0;                 /* Disable counter */
    dev->regs->PSC = config->prescaler;
    dev->regs->ARR = config->period;
    dev->regs->CNT = 0;
    
    /* Enable update interrupt */
    dev->regs->DIER = TIM_DIER_UIE;
    
    /* Load prescaler and period values */
    dev->regs->EGR = TIM_EGR_UG;
    dev->regs->SR = 0;                  /* Clear update flag */
    
    /* Configure NVIC */
    NVIC_SetPriority(dev->irq, config->priority);
    NVIC_EnableIRQ(dev->irq);
    
    /* Update state */
    dev->state = TIM_READY;
    dev->event_count = 0;
    dev->event_flag = 0;
    
    return TIM_OK;
}

TIM_Status_t TIM_Read(TIM_Handle_t *handle, TIM_Status_Data_t *out)
{
    if (!handle || !out) return TIM_ERR_BAD_CFG;
    
    uint8_t idx = handle->timer_index;
    if (idx >= 4) return TIM_ERR_BAD_CFG;
    
    TimerDevice *dev = &timers[idx];
    if (!dev->initialized) return TIM_ERR_NOT_INIT;
    
    /* Read current state from hardware and driver */
    out->count = dev->regs->CNT;
    out->events = dev->event_count;
    out->state = dev->state;
    out->event_pending = dev->event_flag;
    
    return TIM_OK;
}

TIM_Status_t TIM_Control(TIM_Handle_t *handle, TIM_Cmd_t cmd)
{
    if (!handle) return TIM_ERR_BAD_CFG;
    
    uint8_t idx = handle->timer_index;
    if (idx >= 4) return TIM_ERR_BAD_CFG;
    
    TimerDevice *dev = &timers[idx];
    if (!dev->initialized) return TIM_ERR_NOT_INIT;
    
    switch (cmd) {
        case TIM_CMD_START:
            if (dev->state == TIM_IDLE) {
                return TIM_ERR_NOT_INIT;
            }
            dev->regs->CR1 |= TIM_CR1_CEN;
            dev->state = TIM_RUNNING;
            break;
            
        case TIM_CMD_STOP:
            dev->regs->CR1 &= ~TIM_CR1_CEN;
            dev->state = TIM_STOPPED;
            break;
            
        case TIM_CMD_RESET:
            dev->regs->CNT = 0;
            dev->event_count = 0;
            dev->event_flag = 0;
            break;
            
        case TIM_CMD_ACK_EVENT:
            dev->event_flag = 0;
            break;
            
        default:
            return TIM_ERR_BAD_CFG;
    }
    
    return TIM_OK;
}

TIM_Status_t TIM_Close(TIM_Handle_t *handle)
{
    if (!handle) return TIM_ERR_BAD_CFG;
    
    uint8_t idx = handle->timer_index;
    if (idx >= 4) return TIM_ERR_BAD_CFG;
    
    TimerDevice *dev = &timers[idx];
    
    /* Stop timer */
    dev->regs->CR1 = 0;
    
    /* Disable interrupt */
    NVIC_DisableIRQ(dev->irq);
    
    /* Disable clock */
    disable_timer_clock((TIM_ID_t)idx);
    
    /* Reset state */
    dev->initialized = 0;
    dev->state = TIM_IDLE;
    handle_used[idx] = 0;
    
    return TIM_OK;
}

/* ===========================================================================
 * POINT 5: Interrupt service routines
 * ===========================================================================
 * ISR does minimal work:
 * - Clear hardware interrupt flag
 * - Update driver state
 * - Set event flag for application
 */

static void handle_timer_interrupt(TimerRegs *regs)
{
    if (regs->SR & TIM_SR_UIF) {
        regs->SR &= ~TIM_SR_UIF;        /* Clear interrupt flag */
        
        TimerDevice *dev = find_device_by_regs(regs);
        if (dev) {
            dev->event_count++;
            dev->event_flag = 1;
        }
    }
}

void TIM2_IRQHandler(void)
{
    handle_timer_interrupt((TimerRegs*)TIM2);
}

void TIM3_IRQHandler(void)
{
    handle_timer_interrupt((TimerRegs*)TIM3);
}

void TIM4_IRQHandler(void)
{
    handle_timer_interrupt((TimerRegs*)TIM4);
}

void TIM5_IRQHandler(void)
{
    handle_timer_interrupt((TimerRegs*)TIM5);
}
