/**
 ******************************************************************************
 * @file    tim_driver.c
 * @brief   Bare-metal Timer driver implementation for STM32F401RE
 *          Implements device driver pattern with callback management
 * @date    2026-01-24
 ******************************************************************************
 */

#include "tim_driver.h"
#include <stddef.h>

/* ============================================================================
 * DRIVER INTERNAL STATE MANAGEMENT
 * ============================================================================*/

/**
 * @brief Static storage for timer handles
 * Allows IRQ handlers to access device context
 */
static TIM_Handle_t *g_tim2_handle = NULL;
static TIM_Handle_t *g_tim3_handle = NULL;

/* ============================================================================
 * HELPER FUNCTIONS
 * ============================================================================*/

/**
 * @brief Validate timer handle
 * @param handle: Pointer to timer handle
 * @retval true if handle is valid, false otherwise
 */
static bool TIM_IsHandleValid(const TIM_Handle_t *handle)
{
    if (handle == NULL) {
        return false;
    }
    if (handle->instance == NULL) {
        return false;
    }
    return true;
}

/**
 * @brief Enable timer peripheral clock
 * @param handle: Pointer to timer handle
 * @retval TIM_Status_t: Status code
 */
static TIM_Status_t TIM_EnableClock(TIM_Handle_t *handle)
{
    if (handle->instance == TIM2) {
        RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
        return TIM_STATUS_OK;
    }
    else if (handle->instance == TIM3) {
        RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
        return TIM_STATUS_OK;
    }
    else if (handle->instance == TIM4) {
        RCC->APB1ENR |= RCC_APB1ENR_TIM4EN;
        return TIM_STATUS_OK;
    }
    else if (handle->instance == TIM5) {
        RCC->APB1ENR |= RCC_APB1ENR_TIM5EN;
        return TIM_STATUS_OK;
    }

    return TIM_STATUS_ERROR;
}

/**
 * @brief Disable timer peripheral clock
 * @param handle: Pointer to timer handle
 * @retval TIM_Status_t: Status code
 */
static TIM_Status_t TIM_DisableClock(TIM_Handle_t *handle)
{
    if (handle->instance == TIM2) {
        RCC->APB1ENR &= ~RCC_APB1ENR_TIM2EN;
        return TIM_STATUS_OK;
    }
    else if (handle->instance == TIM3) {
        RCC->APB1ENR &= ~RCC_APB1ENR_TIM3EN;
        return TIM_STATUS_OK;
    }
    else if (handle->instance == TIM4) {
        RCC->APB1ENR &= ~RCC_APB1ENR_TIM4EN;
        return TIM_STATUS_OK;
    }
    else if (handle->instance == TIM5) {
        RCC->APB1ENR &= ~RCC_APB1ENR_TIM5EN;
        return TIM_STATUS_OK;
    }

    return TIM_STATUS_ERROR;
}

/**
 * @brief Get NVIC IRQ number for timer
 * @param handle: Pointer to timer handle
 * @retval IRQn_Type: IRQ number for the timer
 */
static IRQn_Type TIM_GetIRQNumber(TIM_Handle_t *handle)
{
    if (handle->instance == TIM2) {
        return TIM2_IRQn;
    }
    else if (handle->instance == TIM3) {
        return TIM3_IRQn;
    }
    else if (handle->instance == TIM4) {
        return TIM4_IRQn;
    }
    else if (handle->instance == TIM5) {
        return TIM5_IRQn;
    }

    return -1;
}

/**
 * @brief Store timer handle for IRQ handler access
 * @param handle: Pointer to timer handle
 */
static void TIM_RegisterHandle(TIM_Handle_t *handle)
{
    if (handle->instance == TIM2) {
        g_tim2_handle = handle;
    }
    else if (handle->instance == TIM3) {
        g_tim3_handle = handle;
    }
}

/**
 * @brief Unregister timer handle
 * @param handle: Pointer to timer handle
 */
static void TIM_UnregisterHandle(TIM_Handle_t *handle)
{
    if (handle->instance == TIM2) {
        g_tim2_handle = NULL;
    }
    else if (handle->instance == TIM3) {
        g_tim3_handle = NULL;
    }
}

/* ============================================================================
 * PUBLIC DRIVER INTERFACE IMPLEMENTATION
 * ============================================================================*/

/**
 * @brief Initialize timer device with given configuration
 */
TIM_Status_t TIM_Init(TIM_Handle_t *handle, const TIM_Config_t *config)
{
    /* Validate input parameters */
    if (!TIM_IsHandleValid(handle) || config == NULL) {
        return TIM_STATUS_INVALID_CFG;
    }

    /* Check if already initialized */
    if (handle->state != TIM_STATE_UNINITIALIZED) {
        return TIM_STATUS_ALREADY_INIT;
    }

    /* Store configuration */
    handle->config = *config;

    /* Enable timer peripheral clock */
    if (TIM_EnableClock(handle) != TIM_STATUS_OK) {
        return TIM_STATUS_ERROR;
    }

    /* Get IRQ number for this timer */
    handle->irq_number = TIM_GetIRQNumber(handle);
    if (handle->irq_number == -1) {
        return TIM_STATUS_ERROR;
    }

    /* Disable counter during configuration */
    handle->instance->CR1 &= ~TIM_CR1_CEN;

    /* Set prescaler (PSC register) */
    handle->instance->PSC = config->prescaler;

    /* Set auto-reload value (ARR register) */
    handle->instance->ARR = config->period;

    /* Reset counter to 0 */
    handle->instance->CNT = 0;

    /* Enable update interrupt */
    handle->instance->DIER |= TIM_DIER_UIE;

    /* Generate update event to load PSC and ARR values */
    handle->instance->EGR |= TIM_EGR_UG;

    /* Clear update interrupt flag (set by update event) */
    handle->instance->SR &= ~TIM_SR_UIF;

    /* Configure NVIC for timer interrupt */
    NVIC_SetPriority(handle->irq_number, config->priority);
    NVIC_EnableIRQ(handle->irq_number);

    /* Set timer to count upward */
    handle->instance->CR1 &= ~TIM_CR1_DIR;

    /* Initialize state variables */
    handle->state = TIM_STATE_INITIALIZED;
    handle->event_counter = 0;
    handle->period_elapsed_cb = NULL;

    /* Register handle for IRQ access */
    TIM_RegisterHandle(handle);

    return TIM_STATUS_OK;
}

/**
 * @brief Start timer counter
 */
TIM_Status_t TIM_Start(TIM_Handle_t *handle)
{
    if (!TIM_IsHandleValid(handle)) {
        return TIM_STATUS_INVALID_CFG;
    }

    if (handle->state == TIM_STATE_UNINITIALIZED) {
        return TIM_STATUS_NOT_INIT;
    }

    /* Enable counter */
    handle->instance->CR1 |= TIM_CR1_CEN;
    handle->state = TIM_STATE_RUNNING;

    return TIM_STATUS_OK;
}

/**
 * @brief Stop timer counter
 */
TIM_Status_t TIM_Stop(TIM_Handle_t *handle)
{
    if (!TIM_IsHandleValid(handle)) {
        return TIM_STATUS_INVALID_CFG;
    }

    if (handle->state == TIM_STATE_UNINITIALIZED) {
        return TIM_STATUS_NOT_INIT;
    }

    /* Disable counter */
    handle->instance->CR1 &= ~TIM_CR1_CEN;
    handle->state = TIM_STATE_STOPPED;

    return TIM_STATUS_OK;
}

/**
 * @brief Get current counter value
 */
TIM_Status_t TIM_GetCounter(TIM_Handle_t *handle, uint32_t *counter)
{
    if (!TIM_IsHandleValid(handle) || counter == NULL) {
        return TIM_STATUS_INVALID_CFG;
    }

    if (handle->state == TIM_STATE_UNINITIALIZED) {
        return TIM_STATUS_NOT_INIT;
    }

    *counter = handle->instance->CNT;
    return TIM_STATUS_OK;
}

/**
 * @brief Register user callback for period elapsed event
 */
TIM_Status_t TIM_RegisterCallback(TIM_Handle_t *handle, TIM_Callback_t callback)
{
    if (!TIM_IsHandleValid(handle)) {
        return TIM_STATUS_INVALID_CFG;
    }

    if (handle->state == TIM_STATE_UNINITIALIZED) {
        return TIM_STATUS_NOT_INIT;
    }

    handle->period_elapsed_cb = callback;
    return TIM_STATUS_OK;
}

/**
 * @brief Get timer device state
 */
TIM_State_t TIM_GetState(TIM_Handle_t *handle)
{
    if (!TIM_IsHandleValid(handle)) {
        return TIM_STATE_UNINITIALIZED;
    }
    return handle->state;
}

/**
 * @brief Get timer event counter
 */
uint32_t TIM_GetEventCount(TIM_Handle_t *handle)
{
    if (!TIM_IsHandleValid(handle)) {
        return 0;
    }
    return handle->event_counter;
}

/**
 * @brief De-initialize timer device
 */
TIM_Status_t TIM_DeInit(TIM_Handle_t *handle)
{
    if (!TIM_IsHandleValid(handle)) {
        return TIM_STATUS_INVALID_CFG;
    }

    /* Stop timer if running */
    if (handle->state == TIM_STATE_RUNNING) {
        TIM_Stop(handle);
    }

    /* Disable interrupts */
    NVIC_DisableIRQ(handle->irq_number);

    /* Disable timer peripheral clock */
    TIM_DisableClock(handle);

    /* Clear handle and callback */
    TIM_UnregisterHandle(handle);
    handle->period_elapsed_cb = NULL;
    handle->state = TIM_STATE_UNINITIALIZED;

    return TIM_STATUS_OK;
}

/* ============================================================================
 * INTERRUPT HANDLERS (Called by ARM Cortex-M4 vector table)
 * ============================================================================*/

/**
 * @brief TIM2 interrupt handler
 * Manages update interrupt for TIM2 device
 */
void TIM2_IRQHandler(void)
{
    if (g_tim2_handle == NULL) {
        return;
    }

    /* Check if update interrupt flag is set */
    if (g_tim2_handle->instance->SR & TIM_SR_UIF) {
        /* Clear the interrupt flag (write 0 to clear) */
        g_tim2_handle->instance->SR &= ~TIM_SR_UIF;

        /* Increment event counter */
        g_tim2_handle->event_counter++;

        /* Call user callback if registered */
        if (g_tim2_handle->period_elapsed_cb != NULL) {
            g_tim2_handle->period_elapsed_cb();
        }
    }
}

/**
 * @brief TIM3 interrupt handler
 * Manages update interrupt for TIM3 device
 */
void TIM3_IRQHandler(void)
{
    if (g_tim3_handle == NULL) {
        return;
    }

    /* Check if update interrupt flag is set */
    if (g_tim3_handle->instance->SR & TIM_SR_UIF) {
        /* Clear the interrupt flag (write 0 to clear) */
        g_tim3_handle->instance->SR &= ~TIM_SR_UIF;

        /* Increment event counter */
        g_tim3_handle->event_counter++;

        /* Call user callback if registered */
        if (g_tim3_handle->period_elapsed_cb != NULL) {
            g_tim3_handle->period_elapsed_cb();
        }
    }
}
