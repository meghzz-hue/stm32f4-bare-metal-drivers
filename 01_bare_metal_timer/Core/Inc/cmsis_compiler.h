/**
  ******************************************************************************
  * @file    cmsis_compiler.h
  * @brief   CMSIS compiler generic header file
  ******************************************************************************
  */

#ifndef __CMSIS_COMPILER_H
#define __CMSIS_COMPILER_H

#include <stdint.h>

/* GNU Compiler (GCC) */
#if defined(__GNUC__)
  #include "cmsis_gcc.h"

/* ARM Compiler (ARMCC) */
#elif defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
  #include "cmsis_armclang.h"

/* IAR Compiler */
#elif defined(__ICCARM__)
  #include "cmsis_iccarm.h"

#else
  #error "Unknown compiler."
#endif

#endif /* __CMSIS_COMPILER_H */
