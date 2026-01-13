/**
  ******************************************************************************
  * @file    stm32f4xx.h
  * @brief   CMSIS STM32F4xx Device Peripheral Access Layer Header File
  * @author  Bare-metal wrapper
  * @date    2026-01-12
  ******************************************************************************
  */

#ifndef __STM32F4XX_H
#define __STM32F4XX_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Device selection - Uncomment the line for your device */
#if !defined (STM32F401xE)
  #define STM32F401xE  /* STM32F401RE device */
#endif

/* Include CMSIS device header */
#if defined(STM32F401xE)
  #include "stm32f401xe.h"
#else
  #error "Please select device in stm32f4xx.h file"
#endif

/* Exported types */
typedef enum
{
  RESET = 0,
  SET = !RESET
} FlagStatus, ITStatus;

typedef enum
{
  DISABLE = 0,
  ENABLE = !DISABLE
} FunctionalState;

typedef enum
{
  ERROR = 0,
  SUCCESS = !ERROR
} ErrorStatus;

#ifdef __cplusplus
}
#endif

#endif /* __STM32F4XX_H */
