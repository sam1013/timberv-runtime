// See LICENSE for license details.

/* Kernel includes. */
#include "FreeRTOS.h"
#include "portmacro.h"
#include "task.h"
#include "ttcb.h"
#include "enclave.h"
#include "enclave_api.h"

#define ENCLAVE_TLS_INDEX  0
#define ENCLS_RETRY_LOCKED_CTR 50

#define ENCLS_RETRY_LOCKED_YIELD(func) ({ \
  tm_errcode_t ERLret; \
  size_t retrycount = ENCLS_RETRY_LOCKED_CTR; \
  while (TM_LOCKED == (ERLret = (func)) && --retrycount) { \
    printf("-----------------------------------------yielding -------------------\n"); \
    ASSERT_STACK_DOES_NOT_GROW(); \
    taskYIELD(); \
  } \
  ERLret; })

/* API for FreeRTOS */

BaseType_t xEnclaveCreate( TaskHandle_t xTask )
{
  void* xNewTTCB = pvPortMalloc( sizeof ( ttcb_t ) );
  if ( NULL == xNewTTCB ) {
    printf("xEnclaveCreate: out of memory\n");
    return pdFAIL;
  }

  BaseType_t xRunningPrivileged = xPortRaisePrivilege();
  tm_errcode_t xReturnValue = ENCLS_RETRY_LOCKED_YIELD(tm_encls_create_enclave( xNewTTCB ));
  vPortResetPrivilege( xRunningPrivileged );
  
  if ( TM_SUCCESS != xReturnValue ) {
    printf("xEnclaveCreate: TM create enclave failed with %d\n", xReturnValue);
    return pdFAIL;
  }

  vTaskSetThreadLocalStoragePointer( xTask, ENCLAVE_TLS_INDEX, xNewTTCB );
  return pdPASS;
}

BaseType_t xEnclaveAddRegion( TaskHandle_t xTask, const MemoryRegion_t * const xRegions )
{
  BaseType_t ret = pdPASS;

  void* xTTCB = pvTaskGetThreadLocalStoragePointer( xTask, ENCLAVE_TLS_INDEX );
  if ( NULL == xTTCB ) {
    printf("xEnclaveAddRegion: Not created\n");
    return pdFAIL;
  }

  BaseType_t xRunningPrivileged = xPortRaisePrivilege();
  region_id_t bRegionID = 0;
  for ( uword_t i = 0; i < MAX_TTCB_REGIONS && i < portNUM_CONFIGURABLE_REGIONS; i++ ) {
    /* Add all MPU regions to the TTCB */
    printf("Adding region %lu\n", i);
    tm_errcode_t xReturnValue = ENCLS_RETRY_LOCKED_YIELD(tm_encls_add_enclave_region(
      xTTCB, 
      xRegions[i].pvBaseAddress,
      (void*)((unsigned long)xRegions[i].pvBaseAddress + xRegions[i].ulLengthInBytes),
      xRegions[i].ulParameters,
      bRegionID++));
    if ( TM_SUCCESS != xReturnValue ) {
      printf("xEnclaveAddRegion: tm_encls_add_enclave_region failed with %d\n", xReturnValue);
      ret = pdFAIL;
      break;
    }
  }
  vPortResetPrivilege( xRunningPrivileged );

  return ret;
}

BaseType_t xEnclaveTagRegion( TaskHandle_t xTask, const EnclaveRegion_t * const xRegions )
{
  BaseType_t ret = pdPASS;

  void* xTTCB = pvTaskGetThreadLocalStoragePointer( xTask, ENCLAVE_TLS_INDEX );
  if ( NULL == xTTCB ) {
    printf("xEnclaveAddRegion: Not created\n");
    return pdFAIL;
  }

  BaseType_t xRunningPrivileged = xPortRaisePrivilege();
  for ( uword_t i = 0; i < MAX_ENCLAVE_REGIONS; i++ ) {
    if (xRegions[i].pvBaseAddress == 0 && xRegions[i].pvBoundAddress == 0) {
      continue;
    }
    /* Tag all enclave regions */
    printf("Tagging enclave region %lu\n", i);
    tm_errcode_t xReturnValue = ENCLS_RETRY_LOCKED_YIELD(tm_encls_add_data(xTTCB, 
      xRegions[i].pvBaseAddress, xRegions[i].pvBoundAddress));
    if ( TM_SUCCESS != xReturnValue ) {
      printf("xEnclaveAddRegion: tm_encls_add_data failed with %d\n", xReturnValue);
      ret = pdFAIL;
      break;
    }
  }
  vPortResetPrivilege( xRunningPrivileged );

  return ret;
}

BaseType_t xEnclaveAddEntryPoints( TaskHandle_t xTask, void* xEntryPoints[] )
{
  void* xTTCB = pvTaskGetThreadLocalStoragePointer( xTask, ENCLAVE_TLS_INDEX );
  if ( NULL == xTTCB ) {
    printf("xEnclaveAddEntryPoints: Not created\n");
    return pdFAIL;
  }

  BaseType_t xRunningPrivileged = xPortRaisePrivilege();
  tm_errcode_t xReturnValue = ENCLS_RETRY_LOCKED_YIELD(tm_encls_add_entries( xTTCB, xEntryPoints ));
  vPortResetPrivilege( xRunningPrivileged );
  
  if ( TM_SUCCESS != xReturnValue ) {
    printf("xEnclaveAddEntryPoints: tm_encls_add_entries failed with %d\n", xReturnValue);
    return pdFAIL;
  }

  return pdPASS;
}

BaseType_t xEnclaveInit( TaskHandle_t xTask )
{
  void* xTTCB = pvTaskGetThreadLocalStoragePointer( xTask, ENCLAVE_TLS_INDEX );
  if ( NULL == xTTCB ) {
    printf("xEnclaveInit: Not created\n");
    return pdFAIL;
  }

  BaseType_t xRunningPrivileged = xPortRaisePrivilege();
  tm_errcode_t xReturnValue = ENCLS_RETRY_LOCKED_YIELD(tm_encls_init_enclave( xTTCB ));
  vPortResetPrivilege( xRunningPrivileged );
  
  if ( TM_SUCCESS != xReturnValue ) {
    printf("xEnclaveInit: tm_encls_init_enclave failed with %d\n", xReturnValue);
    return pdFAIL;
  }

  return pdPASS;
}

BaseType_t xEnclaveLoad( TaskHandle_t xTask )
{
  void* xTTCB = pvTaskGetThreadLocalStoragePointer( xTask, ENCLAVE_TLS_INDEX );
  if ( NULL == xTTCB ) {
    printf("xEnclaveLoad: Not created\n");
    return pdFAIL;
  }

  BaseType_t xRunningPrivileged = xPortRaisePrivilege();
  portDISABLE_INTERRUPTS();
  tm_errcode_t xReturnValue = tm_encls_load_enclave( xTTCB );
  portENABLE_INTERRUPTS();
  vPortResetPrivilege( xRunningPrivileged );
  
  if ( TM_SUCCESS != xReturnValue ) {
    printf("xEnclaveLoad: tm_encls_load_enclave failed with %d\n", xReturnValue);
    return pdFAIL;
  }

  return pdPASS;
}

BaseType_t xEnclaveDestroy( TaskHandle_t xTask )
{
  void* xTTCB = pvTaskGetThreadLocalStoragePointer( xTask, ENCLAVE_TLS_INDEX );
  if ( NULL == xTTCB ) {
    printf("xEnclaveDestroy: Not created\n");
    return pdFAIL;
  }

  /* Prevent enclave from being loaded again */
  vTaskSetThreadLocalStoragePointer( xTask, ENCLAVE_TLS_INDEX, NULL );

  BaseType_t xRunningPrivileged = xPortRaisePrivilege();
  tm_errcode_t xReturnValue = ENCLS_RETRY_LOCKED_YIELD(tm_encls_destroy_enclave( xTTCB ));
  vPortResetPrivilege( xRunningPrivileged );

  if ( TM_SUCCESS != xReturnValue ) {
    printf("xEnclaveDestroy: TM destroy enclave failed with %d\n", xReturnValue);
    return pdFAIL;
  }

  vTaskSetThreadLocalStoragePointer( xTask, ENCLAVE_TLS_INDEX, NULL );

  vPortFree( xTTCB );

  return pdPASS;
}

BaseType_t xEnclaveCreateFull( TaskHandle_t xTask, EnclaveParameters_t* pxEnclaveDefinition )
{
  if ( !ENCLS_RETRY_LOCKED_YIELD(xEnclaveCreate( xTask )) ||
       !ENCLS_RETRY_LOCKED_YIELD(xEnclaveAddRegion( xTask, pxEnclaveDefinition->xTaskParameters.xRegions )) ||
       !ENCLS_RETRY_LOCKED_YIELD(xEnclaveTagRegion( xTask, pxEnclaveDefinition->xEnclaveRegions )) ||
       !ENCLS_RETRY_LOCKED_YIELD(xEnclaveAddEntryPoints( xTask, *(pxEnclaveDefinition->xEnclaveEntryPoints) )) ||
       !ENCLS_RETRY_LOCKED_YIELD(xEnclaveInit( xTask ))) {
    return pdFAIL;
  }
  if ( NULL != xTask && pdPASS != xEnclaveLoad( xTask ) ) {
    return pdFAIL;
  }
  return pdPASS;
}
