// See LICENSE for license details.

/* Kernel includes. */
#include "FreeRTOS.h"
#include "portmacro.h"
#include "task.h"
#include "queue.h"
#include "timers.h"

/* RISCV includes */
#include "arch/syscalls.h"
#include "arch/clib.h"

#include "trustmon.h"
#include "tag.h"
#include "demo.h"

#define APPNAME 1
#include "apps.h"
#include "enclave_api.h"

NORMAL_DATA static portSTACK_TYPE xTaskAStack[ configMINIMAL_STACK_SIZE ];

NORMAL_FUNCTION void vTaskA( void *pvParameters );

NORMAL_FUNCTION void vTaskASecurewrapper();
SECURE_FUNCTION int vTaskASecure();
SECURE_FUNCTION int vTaskAGetkey();
extern int vTaskARecurseEcall(int val);
extern int vTaskARecurseOcall(int val);
extern int ocall_vTaskARecurseOcall(int val);
extern int oret_vTaskARecurseOcall(int val);
  
extern void ocall_vTaskAOcall();
extern void oret_vTaskAOcall();

NORMAL_DATA static void* xTaskAEntryPoints[] = {
  vTaskASecure,
  vTaskARecurseEcall,
  vTaskAGetkey,
  oret_vTaskAOcall,
  oret_vTaskARecurseOcall,
  NULL,
};

/* Fill in a TaskParameters_t structure per reg test task to define the tasks. */
EnclaveParameters_t pxEnclave1Definition =
{
  {
    vTaskA,
    "TaskA",
    configMINIMAL_STACK_SIZE,
    NULL,
    bktPRIMARY_PRIORITY | portPRIVILEGE_BIT,
    xTaskAStack,
    {
      /* Base address           Length          Parameters */
      {0,0,0}, /* normal and secure text */
      {0,0,0}, /* normal and secure data */
      {0,0,0},
      {0,0,0},
    }
  },
  {
    {NULL, NULL}, /* secure text */
    {NULL, NULL}, /* secure data */
    {NULL, NULL}, /* secure data */
    {NULL, NULL}, /* secure data */
  },
  &xTaskAEntryPoints,
};

TaskParameters_t* vInitTask1() {
  /* Initialize pxTaskDefinition.xRegions during runtime
   * because the linker symbols are not available during compile time */
  /* This function is instanciated by including apps.h */
  vInitTask1Ranges(&(pxEnclave1Definition.xTaskParameters));
  vInitTask1Tags(pxEnclave1Definition.xEnclaveRegions);
  return &pxEnclave1Definition.xTaskParameters;
}

extern QueueHandle_t xTestQueue;

NORMAL_DATA static int xGlobalN;
SECURE_DATA static int xGlobalS;

NORMAL_FUNCTION void vTaskA( void *pvParameters )
{
  BaseType_t xItem = 0;
  ( void ) pvParameters;
  puts("Task1 start\n");

  assertPriv(pdTRUE);
  portSWITCH_TO_USER_MODE();
  puts("Task1 in usermode?\n");
  assertPriv(pdFALSE);
  puts("Task1 in usermode\n");
  configASSERT( xEnclaveCreate( NULL ) );
  puts("Task1 enclave created\n");
  configASSERT( xEnclaveAddRegion( NULL, pxEnclave1Definition.xTaskParameters.xRegions ) );
  puts("Task1 regions added\n");
  configASSERT( xEnclaveTagRegion( NULL, pxEnclave1Definition.xEnclaveRegions ) );
  puts("Task1 enclave regions tagged\n");
  configASSERT( xEnclaveAddEntryPoints( NULL, *(pxEnclave1Definition.xEnclaveEntryPoints) ) );
  puts("Task1 entry points added\n");
  configASSERT( xEnclaveInit( NULL ) );
  puts("Task1 init\n");
  //configASSERT( xEnclaveLoad( NULL ) );
  taskYIELD();
  puts("Task1 loaded\n");
  vTaskASecurewrapper();
  vTaskASecurewrapper();
  //taskYIELD();
  configASSERT( xEnclaveLoad( NULL ) );
  //xEnclaveLoad( NULL );
  vTaskASecurewrapper();
  puts("Task1 secure called\n");

  vTaskARecurseOcall(30);

  vTaskAGetkey();

  for( ;; xItem++)
  {
    ASSERT_STACK_DOES_NOT_GROW();
    assertPriv(pdFALSE);
    printf("Task1\n");
    //taskYIELD();
    if( xQueueSend( xTestQueue, &xItem, bktDONT_BLOCK ) != pdPASS )
    {
      puts("Task1 queue full\n");
      configASSERT( xEnclaveDestroy( NULL ) );
      puts("Task1 enclave destroyed\n");
      for (;;);
    }
    printf("Task1 sent %ld\n", xItem);
    taskYIELD();
  }
}
/*-----------------------------------------------------------*/

NORMAL_FUNCTION void vTaskASecurewrapper() {
  printf("vTaskASecure returned %d\n", vTaskASecure());
}

NORMAL_FUNCTION void vTaskAOcall(int val) {
  printf("vTaskAOcall'd with %d\n", val);
}

SECURE_FUNCTION int vTaskARecurseEcall(int val) {
  if (val <= 0) {
    return 0;
  }
  return ocall_vTaskARecurseOcall(val-1);
}

NORMAL_FUNCTION int vTaskARecurseOcall(int val) {
  printf("vTaskARecurseOcall: %d\n", val);
  if (val <= 0) {
    return 0;
  }
  return vTaskARecurseEcall(val-1);
}


SECURE_FUNCTION int vTaskASecure()
{
  DEREF_PTR_WRITE(&xGlobalN, T_NORMAL, xGlobalS++);
  if (TM_SUCCESS != enclu(ENCLU_GETKEY, 2, 3, 4)) {
    return 1;
  }

  ocall_vTaskAOcall(42);

  //return vTaskASecure(); // recursion to test out-of-stack behavior
  return 0;
}

SECURE_DATA unsigned char securekey[TM_KEY_SIZE];
SECURE_DATA int secureid;

SECURE_FUNCTION int vTaskAGetkey()
{
        secureid = 5;
        if (TM_SUCCESS != enclu(ENCLU_GETKEY, &secureid, sizeof(secureid), &securekey)) {
          while(1);
        }
        return 0;
}
