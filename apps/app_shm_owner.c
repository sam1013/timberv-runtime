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

#define APPNAME O
#include "apps.h"
#include "enclave_api.h"

NORMAL_DATA static portSTACK_TYPE xTaskOStack[ configMINIMAL_STACK_SIZE ];

NORMAL_FUNCTION void vTaskO( void *pvParameters );

SECURE_FUNCTION int vTaskOGetkey();
SECURE_FUNCTION int vTaskOShmOpen();
SECURE_FUNCTION void vTaskOShmSend();

NORMAL_DATA static void* xTaskOEntryPoints[] = {
  vTaskOGetkey,
  vTaskOShmOpen,
  vTaskOShmSend,
  NULL,
};

/* Fill in a TaskParameters_t structure per reg test task to define the tasks. */
EnclaveParameters_t pxEnclaveODefinition =
{
  {
    vTaskO,
    "TaskO",
    configMINIMAL_STACK_SIZE,
    NULL,
    bktPRIMARY_PRIORITY | portPRIVILEGE_BIT,
    xTaskOStack,
    {
      /* Base address		Length		Parameters */
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
  &xTaskOEntryPoints,
};

TaskParameters_t* vInitTaskO() {
  /* Initialize pxTaskDefinition.xRegions during runtime
   * because the linker symbols are not available during compile time */
  /* This function is instanciated by including apps.h */
  vInitTaskORanges(&(pxEnclaveODefinition.xTaskParameters));
  vInitTaskOTags(pxEnclaveODefinition.xEnclaveRegions);
  return &pxEnclaveODefinition.xTaskParameters;
}

NORMAL_DATA unsigned char Onormalbuf[TM_KEY_SIZE];

/* Update this manually */
SECURE_DATA unsigned char target_eid[EID_SIZE] = {
   0xCE,0xB1,0x78,0x05,0x4E,0x4A,0x7B,0xE4,0x3C,0xD1,0x22,0x37,0xA8,0x88,0x16,0x1D,0x3E,0xD8,0xE4,0x6F,0x83,0x39,0x01,0xC4,0x30,0x86,0xC7,0xB7,0x66,0x17,0xC8,0xBF,
};

NORMAL_FUNCTION void Odump_buf(unsigned char* buf, size_t len) {
  for (size_t i = 0; i < len; i++) {
    printf("%02x", buf[i]);
  }
  printf("\n");
}

NORMAL_FUNCTION void vTaskO( void *pvParameters )
{
	puts("TaskO start\n");
	portSWITCH_TO_USER_MODE();
	assertPriv(pdFALSE);
	
	configASSERT(pdPASS == xEnclaveCreateFull( NULL, &pxEnclaveODefinition ));
	taskYIELD();
	
	printf("TaskO enclave loaded\n");
	Odump_buf(Onormalbuf, TM_KEY_SIZE);
	if (vTaskOGetkey()) {
	  printf("key: ");
	  Odump_buf(Onormalbuf, TM_KEY_SIZE);
	} else {
	  printf("getkey error\n");
	}

	configASSERT( vTaskOShmOpen() );
	printf("shm offered\n");

	vTaskOShmSend();
}

SECURE_FUNCTION void Otagged_copy_from_n(void* dst, void* src, size_t len) {
  unsigned char* pdst = dst;
  unsigned char* psrc = src;
  while (len--) {
    *pdst = DEREF_BYTE_PTR_READ(psrc, T_NORMAL);
    psrc++;
    pdst++;
  }
}

SECURE_FUNCTION void Otagged_copy_to_n(void* dst, void* src, size_t len) {
  unsigned char* pdst = dst;
  unsigned char* psrc = src;
  while (len--) {
    DEREF_BYTE_PTR_WRITE(pdst, T_NORMAL, *psrc);
    psrc++;
    pdst++;
  }
}
/*-----------------------------------------------------------*/

SECURE_FUNCTION int vTaskOGetkey()
{
	unsigned char securekey[TM_KEY_SIZE];
	long secureid = 5;
	if (TM_SUCCESS != enclu(ENCLU_GETKEY, (long)&secureid, sizeof(secureid), (long)securekey)) {
	  return pdFALSE;
	}
	Otagged_copy_to_n(Onormalbuf, securekey, TM_KEY_SIZE);
	return pdTRUE;
}

SECURE_DATA unsigned char shmbuf[TM_KEY_SIZE];

SECURE_FUNCTION int vTaskOShmOpen()
{
	region_t region;
	region.range.start = shmbuf;
	region.range.end = shmbuf + sizeof(shmbuf);
	region.flags = SHM_ACCESS_UT | SHM_ACCESS_R | SHM_ACCESS_W;
	if (TM_SUCCESS != enclu(ENCLU_SHMOFFER, (long)&target_eid, (long)&region, 0)) {
	  return pdFALSE;
	}
	return pdTRUE;
}

SECURE_FUNCTION void vTaskOShmSend()
{
  while (1) {
    if (shmbuf[0] == 0) {
      shmbuf[1]++;
      shmbuf[0] = 1;
    }
  }
}
