// See LICENSE for license details.

#ifndef ENCLAVE_API_H
#define ENCLAVE_API_H

/* Kernel includes. */
#include "FreeRTOS.h"
#include "portmacro.h"
#include "task.h"
#include "enclave.h"

#define MAX_ENCLAVE_REGIONS 4

typedef struct {
  void *pvBaseAddress;
  void *pvBoundAddress;
} EnclaveRegion_t;

typedef struct {
  TaskParameters_t xTaskParameters;
  EnclaveRegion_t  xEnclaveRegions[ MAX_ENCLAVE_REGIONS ];
  void* (*xEnclaveEntryPoints)[];
} EnclaveParameters_t;

#define ASSERT_STACK_DOES_NOT_GROW() { \
  register long currentsp asm("sp"); \
  static long sp_##__LINE__ = 0; \
  if (!sp_##__LINE__) { \
    sp_##__LINE__ = currentsp; \
  } else { \
    configASSERT(sp_##__LINE__ == currentsp); \
  } \
}

BaseType_t xEnclaveCreate( TaskHandle_t xTask );
BaseType_t xEnclaveAddRegion( TaskHandle_t xTask, const MemoryRegion_t * const xRegions );
BaseType_t xEnclaveTagRegion( TaskHandle_t xTask, const EnclaveRegion_t * const xRegions );
BaseType_t xEnclaveAddEntryPoints( TaskHandle_t xTask, void* xEntryPoints[] );
BaseType_t xEnclaveInit( TaskHandle_t xTask );
BaseType_t xEnclaveLoad( TaskHandle_t xTask );
BaseType_t xEnclaveDestroy( TaskHandle_t xTask );
BaseType_t xEnclaveCreateFull( TaskHandle_t xTask, EnclaveParameters_t* pxEnclaveDefinition );

#endif /* ENCLAVE_API_H */
