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

#define APPNAME T
#include "apps.h"
#include "enclave_api.h"

NORMAL_DATA static portSTACK_TYPE xTaskTStack[ configMINIMAL_STACK_SIZE ];

NORMAL_FUNCTION void vTaskT( void *pvParameters );

SECURE_FUNCTION int vTaskTGetkey();
SECURE_FUNCTION unsigned char* vTaskTShmAccept();
SECURE_FUNCTION void vTaskTInitEID();
SECURE_FUNCTION unsigned char vTaskTShmRecv();
NORMAL_DATA static void* xTaskTEntryPoints[] = {
  vTaskTGetkey,
  vTaskTShmAccept,
  vTaskTInitEID,
  vTaskTShmRecv,
  NULL,
};

/* Fill in a TaskParameters_t structure per reg test task to define the tasks. */
EnclaveParameters_t pxEnclaveTDefinition =
{
  {
    vTaskT,
    "TaskT",
    configMINIMAL_STACK_SIZE,
    NULL,
    bktPRIMARY_PRIORITY | portPRIVILEGE_BIT,
    xTaskTStack,
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
  &xTaskTEntryPoints,
};

TaskParameters_t* vInitTaskT() {
  /* Initialize pxTaskDefinition.xRegions during runtime
   * because the linker symbols are not available during compile time */
  /* This function is instanciated by including apps.h */
  vInitTaskTRanges(&(pxEnclaveTDefinition.xTaskParameters));
  vInitTaskTTags(pxEnclaveTDefinition.xEnclaveRegions);
  return &pxEnclaveTDefinition.xTaskParameters;
}

NORMAL_DATA unsigned char normalbuf[TM_KEY_SIZE];

/* Update this EID manually! */
NORMAL_DATA unsigned char shm_eid[EID_SIZE] = {
  0x25,0x66,0x17,0x31,0x24,0x67,0x78,0x8B,0xB6,0xDC,0xC6,0x0E,0xDD,0x44,0xB2,0xC9,0x4E,0x99,0x81,0xE7,0xC7,0x9A,0xCB,0xB2,0xC1,0xF4,0x84,0xDC,0xD4,0x68,0x0C,0x96,
};

SECURE_DATA unsigned char Tshm_eid[EID_SIZE] = {
  0
};

NORMAL_FUNCTION void dump_buf(unsigned char* buf, size_t len) {
  for (size_t i = 0; i < len; i++) {
    printf("%02x", buf[i]);
  }
  printf("\n");
}


NORMAL_FUNCTION void vTaskT( void *pvParameters )
{
  puts("TaskT start\n");
  //vSET_REGION(pxEnclaveTDefinition.xTaskParameters.xRegions, 0, 1, 2, portMPU_REGION_READ_WRITE | portMPU_REGION_UTRUSTED);
  /* Enabling this line crashes by accessing an unmapped MMIO region */
  vTaskAllocateMPURegions( NULL, pxEnclaveTDefinition.xTaskParameters.xRegions );
  taskYIELD();
  
  portSWITCH_TO_USER_MODE();
  assertPriv(pdFALSE);

  configASSERT(pdPASS == xEnclaveCreateFull( NULL, &pxEnclaveTDefinition ));
  taskYIELD();

  printf("enclave loaded\n");
  dump_buf(normalbuf, TM_KEY_SIZE);
  vTaskTInitEID();
  configASSERT( vTaskTGetkey() );
  printf("key: ");
  dump_buf(normalbuf, TM_KEY_SIZE);

  unsigned char* ptr;
  while ( !(ptr = vTaskTShmAccept()) ) {
    taskYIELD();
  }
  printf("shm accepted at %lx\n", (uintptr_t)ptr);

  /* update task's MPU regions to capture shm */
  vSET_REGION(pxEnclaveTDefinition.xTaskParameters.xRegions, 2, ptr, ptr + TM_KEY_SIZE, portMPU_REGION_READ_WRITE | portMPU_REGION_UTRUSTED);
  vTaskAllocateMPURegions( NULL, pxEnclaveTDefinition.xTaskParameters.xRegions );
  taskYIELD();
  
  while (1) {
    printf("received %02x\n", vTaskTShmRecv());
  }
}

SECURE_FUNCTION void tagged_copy_from_n(void* dst, void* src, size_t len) {
  unsigned char* pdst = dst;
  unsigned char* psrc = src;
  while (len--) {
    *pdst = DEREF_BYTE_PTR_READ(psrc, T_NORMAL);
    psrc++;
    pdst++;
  }
}

SECURE_FUNCTION void tagged_copy_to_n(void* dst, void* src, size_t len) {
  unsigned char* pdst = dst;
  unsigned char* psrc = src;
  while (len--) {
    DEREF_BYTE_PTR_WRITE(pdst, T_NORMAL, *psrc);
    psrc++;
    pdst++;
  }
}
/*-----------------------------------------------------------*/

SECURE_FUNCTION int vTaskTGetkey()
{
  unsigned char securekey[TM_KEY_SIZE];
  long secureid = 5;
  if (TM_SUCCESS != enclu(ENCLU_GETKEY, (long)&secureid, sizeof(secureid), (long)securekey)) {
    return pdFALSE;
  }
  tagged_copy_to_n(normalbuf, securekey, TM_KEY_SIZE);
  return pdTRUE;
}

SECURE_FUNCTION void vTaskTInitEID()
{
  tagged_copy_from_n(Tshm_eid, shm_eid, sizeof(shm_eid));
}

SECURE_DATA unsigned char* shm_start;
SECURE_DATA unsigned char* shm_end;

SECURE_FUNCTION unsigned char* vTaskTShmAccept()
{
  if (TM_SUCCESS != enclu(ENCLU_SHMACCEPT, (long)&Tshm_eid, &shm_start, &shm_end)) {
    return 0;
  }
  return shm_start;
}

SECURE_FUNCTION unsigned char vTaskTShmRecv()
{
  /* Wait for data to be available */
  while (!shm_start[0])
    ;
  unsigned char val = shm_start[1];
  shm_start[0] = 0;
  return val;
}
