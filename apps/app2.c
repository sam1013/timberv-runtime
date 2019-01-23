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
#include "demo.h"

#define APPNAME 2
#include "apps.h"
#include "enclave_api.h"

NORMAL_DATA static portSTACK_TYPE xTaskBStack[ configMINIMAL_STACK_SIZE ];

NORMAL_FUNCTION void vTaskB( void *pvParameters );

SECURE_FUNCTION int privTaskBSecure( int xItem );

NORMAL_DATA static void* xTaskBEntryPoints[] = {
  privTaskBSecure,
  NULL,
};

/* Fill in a TaskParameters_t structure per reg test task to define the tasks. */
EnclaveParameters_t pxEnclave2Definition =
{
  {
    vTaskB,
    "TaskB",
    configMINIMAL_STACK_SIZE,
    NULL,
    bktPRIMARY_PRIORITY | portPRIVILEGE_BIT,
    xTaskBStack,
    {
      /* Base address		Length		Parameters */
      {0,0,0},
      {0,0,0},
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
  &xTaskBEntryPoints,
};

TaskParameters_t* vInitTask2() {
  vInitTask2Ranges(&(pxEnclave2Definition.xTaskParameters));
  vInitTask2Tags(pxEnclave2Definition.xEnclaveRegions);
  //Artificially cause a region overlap which shall crash
  //~ extern TaskParameters_t pxTask1Definition;
  //~ pxTask2Definition.xRegions[3].pvBaseAddress = pxTask1Definition.xRegions[3].pvBaseAddress;
  //~ pxTask2Definition.xRegions[3].ulLengthInBytes = 100000;
  //~ pxTask2Definition.xRegions[3].ulParameters = pxTask1Definition.xRegions[3].ulParameters;
  return &pxEnclave2Definition.xTaskParameters;
}

NORMAL_DATA int xGlobal;

NORMAL_FUNCTION void vTaskB( void *pvParameters )
{	taskYIELD();
	BaseType_t xItem = 0;
	( void ) pvParameters;
	printf("Task2 start\n");
	assertPriv(pdTRUE);
	portSWITCH_TO_USER_MODE();
	configASSERT(pdPASS == xEnclaveCreateFull( NULL, &pxEnclave2Definition ));
	printf("Task2 enclave full\n");
	for( ;; )
	{
		ASSERT_STACK_DOES_NOT_GROW();
		assertPriv(pdFALSE);
		if( xQueueReceive( xTestQueue, &xItem, bktDONT_BLOCK ) != pdPASS )
		{
			printf("Task2 receive error\n");
			//privTaskBSecure(0);
		} else {
		  printf("Task2 received %ld\n", xItem);
		  xGlobal = privTaskBSecure(xItem);
		  printf("Task2secure %d\n", xGlobal);
		  if (xGlobal > 200) {
		    xEnclaveDestroy( NULL );
		    printf("Task2 enclave destroyed\n");
		    while (1) taskYIELD();
		  }
		}
		taskYIELD();
	}
}

SECURE_DATA int xGlobalSecure = 0;
SECURE_FUNCTION int privTaskBSecure( int xItem )
{
  xGlobalSecure++;
  //size_t i = 100000000;
  //while(i--)
  //  ;
  return xItem + xGlobalSecure;
}
