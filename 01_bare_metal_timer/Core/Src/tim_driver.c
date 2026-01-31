/**
 ******************************************************************************
 * @file    tim_driver.c
 * @brief   Bare-metal Timer driver implementation
 *          Implements device driver philosophy: open/write/read/control/close
 *          Hardware details hidden from application
 * @date    2026-01-31
 ******************************************************************************
 */

#include "tim_driver.h"
#include "stm32f401xe.h"
#include <stddef.h>
#include <string.h>

/* ---------------------------------------------------------------------------
 * INTERNAL DATA STRUCTURES (Not in public header)
 * ---------------------------------------------------------------------------*/
typedef struct TIM_Handle {
    TIM_TypeDef *instance;        /**< Hardware peripheral (TIM2/TIM3/...) */
    TIM_Config_t config;
    TIM_State_t state;
    volatile uint8_t event_flag;  /**< Set by ISR when period elapses */
    uint32_t event_counter;       /**< Count of update events */
    IRQn_Type irq_number;         /**< IRQ number cached for NVIC ops */
    TIM_ID_t timer_id;            /**< Device ID */
} TIM_Handle_t_Internal;

/* ---------------------------------------------------------------------------
 * Static pool allocation (no dynamic malloc)
 * ---------------------------------------------------------------------------*/
#define MAX_TIMER_INSTANCES 4
static TIM_Handle_t_Internal g_timer_pool[MAX_TIMER_INSTANCES];
static bool g_timer_pool_used[MAX_TIMER_INSTANCES] = {0};
static TIM_Handle_t_Internal *g_timer_instances[MAX_TIMER_INSTANCES] = {0};

/* ---------------------------------------------------------------------------
 * Timer ID to hardware mapping
 * ---------------------------------------------------------------------------*/
static TIM_TypeDef *tim_id_to_instance(TIM_ID_t id)
{
    switch (id) {
        case TIM_ID_2: return TIM2;
        case TIM_ID_3: return TIM3;
        case TIM_ID_4: return TIM4;
        case TIM_ID_5: return TIM5;
        default: return NULL;
    }
}

static IRQn_Type tim_id_to_irq(TIM_ID_t id)
{
    switch (id) {
        case TIM_ID_2: return TIM2_IRQn;
        case TIM_ID_3: return TIM3_IRQn;
        case TIM_ID_4: return TIM4_IRQn;
        case TIM_ID_5: return TIM5_IRQn;
        default: return -1;
    }
}

/* ---------------------------------------------------------------------------
 * Pool management
 * ---------------------------------------------------------------------------*/
static TIM_Handle_t_Internal *TIM_AllocHandle(TIM_ID_t id)
{
    for (int i = 0; i < MAX_TIMER_INSTANCES; ++i) {
        if (!g_timer_pool_used[i]) {
            g_timer_pool_used[i] = true;
            memset(&g_timer_pool[i], 0, sizeof(TIM_Handle_t_Internal));
            g_timer_pool[i].timer_id = id;
            return &g_timer_pool[i];
        }
    }
    return NULL;
}

static void TIM_FreeHandle(TIM_Handle_t_Internal *h)
{
    if (h >= g_timer_pool && h < g_timer_pool + MAX_TIMER_INSTANCES) {
        int idx = (int)(h - g_timer_pool);
        g_timer_pool_used[idx] = false;
    }
}

static TIM_Handle_t_Internal *TIM_FindInstance(TIM_TypeDef *instance)
{
    for (int i = 0; i < MAX_TIMER_INSTANCES; ++i) {
        if (g_timer_instances[i] != NULL && g_timer_instances[i]->instance == instance) {
            return g_timer_instances[i];
        }
    }
    return NULL;
}

static bool TIM_RegisterInstance(TIM_Handle_t_Internal *handle)
{
    for (int i = 0; i < MAX_TIMER_INSTANCES; ++i) {
        if (g_timer_instances[i] == NULL) {
            g_timer_instances[i] = handle;
            return true;
        }
    }
    return false;
}

static void TIM_UnregisterInstance(TIM_Handle_t_Internal *handle)
{
    for (int i = 0; i < MAX_TIMER_INSTANCES; ++i) {
        if (g_timer_instances[i] == handle) {
            g_timer_instances[i] = NULL;
            return;
        }
    }
}

/* ---------------------------------------------------------------------------
 * Clock helpers
 * ---------------------------------------------------------------------------*/
static TIM_Status_t TIM_EnableClock(TIM_Handle_t_Internal *handle)
{
    if (handle->instance == TIM2) {
        RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
        return TIM_STATUS_OK;
    } else if (handle->instance == TIM3) {
        RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
        return TIM_STATUS_OK;
    } else if (handle->instance == TIM4) {
        RCC->APB1ENR |= RCC_APB1ENR_TIM4EN;
        return TIM_STATUS_OK;
    } else if (handle->instance == TIM5) {
        RCC->APB1ENR |= RCC_APB1ENR_TIM5EN;
        return TIM_STATUS_OK;
    }
    return TIM_STATUS_ERROR;
}

static TIM_Status_t TIM_DisableClock(TIM_Handle_t_Internal *handle)
{
    if (handle->instance == TIM2) {
        RCC->APB1ENR &= ~RCC_APB1ENR_TIM2EN;
        return TIM_STATUS_OK;
    } else if (handle->instance == TIM3) {
        RCC->APB1ENR &= ~RCC_APB1ENR_TIM3EN;
        return TIM_STATUS_OK;
    } else if (handle->instance == TIM4) {
        RCC->APB1ENR &= ~RCC_APB1ENR_TIM4EN;
        return TIM_STATUS_OK;
    } else if (handle->instance == TIM5) {
        RCC->APB1ENR &= ~RCC_APB1ENR_TIM5EN;
        return TIM_STATUS_OK;
    }
    return TIM_STATUS_ERROR;
}

/* ---------------------------------------------------------------------------
 * Public API - Open/Write/Read/Control/Close
 * ---------------------------------------------------------------------------*/

TIM_Handle_t *TIM_Open(TIM_ID_t timer_id)
{
    TIM_TypeDef *instance = tim_id_to_instance(timer_id);
    if (instance == NULL) return NULL;

    TIM_Handle_t_Internal *h = TIM_AllocHandle(timer_id);
    if (h == NULL) return NULL;

    h->instance = instance;
    h->state = TIM_STATE_UNINITIALIZED;
    h->event_flag = 0;
    h->event_counter = 0;
    h->irq_number = tim_id_to_irq(timer_id);

    if (!TIM_RegisterInstance(h)) {
        TIM_FreeHandle(h);
        return NULL;
    }

    return (TIM_Handle_t *)h;
}

TIM_Status_t TIM_Write(TIM_Handle_t *handle, const TIM_Config_t *config)
{
    TIM_Handle_t_Internal *h = (TIM_Handle_t_Internal *)handle;
    if (h == NULL || config == NULL) return TIM_STATUS_INVALID_CFG;
    if (h->state != TIM_STATE_UNINITIALIZED && h->state != TIM_STATE_STOPPED) {
        /* allow reconfiguration only when stopped or uninitialized */
        return TIM_STATUS_ALREADY_INIT;
    }

    h->config = *config;

    if (TIM_EnableClock(h) != TIM_STATUS_OK) return TIM_STATUS_ERROR;


    /* Disable counter during configuration */
    h->instance->CR1 &= ~TIM_CR1_CEN;

    /* Set PSC and ARR */
    h->instance->PSC = config->prescaler;
    h->instance->ARR = config->period;
    h->instance->CNT = 0;

    /* Enable update interrupt */
    h->instance->DIER |= TIM_DIER_UIE;

    /* Generate update event to load PSC and ARR */
    h->instance->EGR |= TIM_EGR_UG;
    h->instance->SR &= ~TIM_SR_UIF;

    NVIC_SetPriority(h->irq_number, config->priority);
    NVIC_EnableIRQ(h->irq_number);

    /* Count up */
    h->instance->CR1 &= ~TIM_CR1_DIR;

    h->state = TIM_STATE_INITIALIZED;
    h->event_counter = 0;
    h->event_flag = 0;

    return TIM_STATUS_OK;
}

TIM_Status_t TIM_Read(TIM_Handle_t *handle, TIM_Data_t *data_out)
{
    TIM_Handle_t_Internal *h = (TIM_Handle_t_Internal *)handle;
    if (h == NULL || data_out == NULL) return TIM_STATUS_INVALID_CFG;
    data_out->counter_value = h->instance->CNT;
    data_out->event_count = h->event_counter;
    data_out->state = h->state;
    data_out->event_occurred = h->event_flag ? 1 : 0;
    return TIM_STATUS_OK;
}

TIM_Status_t TIM_Control(TIM_Handle_t *handle, TIM_Command_t cmd, void *arg)
{
    TIM_Handle_t_Internal *h = (TIM_Handle_t_Internal *)handle;
    if (h == NULL) return TIM_STATUS_INVALID_CFG;
    switch (cmd) {
        case TIM_CMD_START:
            if (h->state == TIM_STATE_UNINITIALIZED) return TIM_STATUS_NOT_INIT;
            h->instance->CR1 |= TIM_CR1_CEN;
            h->state = TIM_STATE_RUNNING;
            return TIM_STATUS_OK;
        case TIM_CMD_STOP:
            h->instance->CR1 &= ~TIM_CR1_CEN;
            h->state = TIM_STATE_STOPPED;
            return TIM_STATUS_OK;
        case TIM_CMD_RESET:
            h->instance->CNT = 0;
            h->event_counter = 0;
            h->event_flag = 0;
            return TIM_STATUS_OK;
        case TIM_CMD_CLEAR_EVENT:
            h->event_flag = 0;
            return TIM_STATUS_OK;
        default:
            return TIM_STATUS_INVALID_CFG;
    }
}

TIM_Status_t TIM_Close(TIM_Handle_t *handle)
{
    TIM_Handle_t_Internal *h = (TIM_Handle_t_Internal *)handle;
    if (h == NULL) return TIM_STATUS_INVALID_CFG;
    /* Stop timer */
    h->instance->CR1 &= ~TIM_CR1_CEN;
    h->state = TIM_STATE_UNINITIALIZED;

    if (h->irq_number != -1) {
        NVIC_DisableIRQ(h->irq_number);
    }

    TIM_DisableClock(h);
    TIM_UnregisterInstance(h);
    TIM_FreeHandle(h);
    return TIM_STATUS_OK;
}

/* ---------------------------------------------------------------------------
 * ISR implementations: minimal work - update counters and set event flag
 * ---------------------------------------------------------------------------*/
void TIM2_IRQHandler(void)
{
    TIM_Handle_t *h = TIM_FindInstance(TIM2);
    if (h == NULL) return;
    if (h->instance->SR & TIM_SR_UIF) {
        h->instance->SR &= ~TIM_SR_UIF;
        h->event_counter++;
        h->event_flag = 1;
    }
}

void TIM3_IRQHandler(void)
{
    TIM_Handle_t *h = TIM_FindInstance(TIM3);
    if (h == NULL) return;
    if (h->instance->SR & TIM_SR_UIF) {
        h->instance->SR &= ~TIM_SR_UIF;
        h->event_counter++;
        h->event_flag = 1;
    }
}

void TIM4_IRQHandler(void)
{
    TIM_Handle_t *h = TIM_FindInstance(TIM4);
    if (h == NULL) return;
    if (h->instance->SR & TIM_SR_UIF) {
        h->instance->SR &= ~TIM_SR_UIF;
        h->event_counter++;
        h->event_flag = 1;
    }
}

void TIM5_IRQHandler(void)
{
    TIM_Handle_t *h = TIM_FindInstance(TIM5);
    if (h == NULL) return;
    if (h->instance->SR & TIM_SR_UIF) {
        h->instance->SR &= ~TIM_SR_UIF;
        h->event_counter++;
        h->event_flag = 1;
    }
}
