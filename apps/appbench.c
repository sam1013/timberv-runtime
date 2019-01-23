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

#define APPNAME Bench
#include "apps.h"
#include "enclave_api.h"

#if TM_BENCH == 1

NORMAL_DATA portSTACK_TYPE xTaskBenchStack[ configMINIMAL_STACK_SIZE ];

NORMAL_FUNCTION void vTaskBench( void *pvParameters );

SECURE_FUNCTION void vSecureEntry1() {
  return;
}
SECURE_FUNCTION void vSecureEntry2() {
  return;
}
SECURE_FUNCTION void vSecureEntry3() {
  return;
}
SECURE_FUNCTION void vSecureEntry4() {
  return;
}
SECURE_FUNCTION void vSecureEntry5() {
  return;
}

SECURE_FUNCTION void vSecureEntry6();
SECURE_FUNCTION void vTaskBenchEcall();
SECURE_FUNCTION void vTaskBenchEtoOcall();
extern void ocall_vTaskBenchOcall();
extern void oret_vTaskBenchOcall();
SECURE_FUNCTION void zzz_secure_func();

NORMAL_DATA static void* xEntry0[] = {
  NULL,
};

NORMAL_DATA static void* xEntry1[] = {
  vSecureEntry1,
  NULL,
};

NORMAL_DATA static void* xEntry2[] = {
  vSecureEntry2,
  vSecureEntry3,
  NULL,
};

NORMAL_DATA static void* xEntry3[] = {
  vSecureEntry4,
  vSecureEntry5,
  vSecureEntry6,
  NULL,
};

NORMAL_DATA static void* xOtherEntries[] = {
  vTaskBenchEcall,
  vTaskBenchEtoOcall,
  oret_vTaskBenchOcall,
  NULL
};

/* Fill in a TaskParameters_t structure per reg test task to define the tasks. */
EnclaveParameters_t pxEnclaveBenchDefinition =
{
  {
    vTaskBench,
    "TaskBench",
    configMINIMAL_STACK_SIZE,
    NULL,
    bktPRIMARY_PRIORITY | portPRIVILEGE_BIT,
    xTaskBenchStack,
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
  NULL,
};

TaskParameters_t* vInitTaskBench() {
  /* Initialize pxTaskDefinition.xRegions during runtime
   * because the linker symbols are not available during compile time */
  /* This function is instanciated by including apps.h */
  vInitTaskBenchRanges(&(pxEnclaveBenchDefinition.xTaskParameters));
  vInitTaskBenchTags(pxEnclaveBenchDefinition.xEnclaveRegions);
  return &pxEnclaveBenchDefinition.xTaskParameters;
}

NORMAL_DATA static int xGlobalN;
SECURE_DATA static int xGlobalS;

NORMAL_FUNCTION void vTaskBench( void *pvParameters )
{
  BaseType_t xItem = 0;
  ( void ) pvParameters;
  tm_errcode_t xReturnValue;
  
  void* xTTCB = pvPortMalloc( sizeof ( ttcb_t ) );
  configASSERT ( xTTCB != NULL );

  /* Benchmark nothing */
  BENCH_START(B_NOP);
  BENCH_STOP(B_NOP);

  /* Benchmark syscall latency (due to M-mode software delegation) */
  // Do a syscall(SYS_write, 1, (long) buf, buflen);
  register long a7 asm("a7") = SYS_write;
  register long a0 asm("a0") = 1;
  register long a1 asm("a1") = NULL;
  register long a2 asm("a2") = 0;
  BENCH_START(B_SYSCALL);
  asm volatile ("scall":"+r"(a0) : "r"(a1), "r"(a2), "r"(a7));

  /* Benchmark ENCLS call latency */
  BENCH_START(B_ENCLS);
  tm_encls_bench( NULL );

  /* Benchmark ENCLS return latency */
  tm_encls_bench( NULL );
  BENCH_STOP(B_ENCLSR);

  /* Benchmark enclave create */
  BENCH_START(B_ECREATE);
  xReturnValue = tm_encls_create_enclave( xTTCB );
  while( xReturnValue != TM_SUCCESS );
  BENCH_STOP(B_ECREATE);

  MemoryRegion_t* region = &pxEnclaveBenchDefinition.xTaskParameters.xRegions[0];
  void* base = region->pvBaseAddress;
  void* bound = (void*)((unsigned long)region->pvBaseAddress + region->ulLengthInBytes);
  unsigned long parameters = region->ulParameters;

  /* Benchmark add region */
  BENCH_START(B_EADD0);
  xReturnValue = tm_encls_add_enclave_region(xTTCB, base, bound, parameters, 0);
  while( xReturnValue != TM_SUCCESS );
  BENCH_STOP(B_EADD0);

  region = &pxEnclaveBenchDefinition.xTaskParameters.xRegions[1];
  base = region->pvBaseAddress;
  bound = (void*)((unsigned long)region->pvBaseAddress + region->ulLengthInBytes);
  parameters = region->ulParameters;

  BENCH_START(B_EADD1);
  xReturnValue = tm_encls_add_enclave_region(xTTCB, base, bound, parameters, 1);
  while( xReturnValue != TM_SUCCESS );
  BENCH_STOP(B_EADD1);

  region = &pxEnclaveBenchDefinition.xTaskParameters.xRegions[2];
  base = region->pvBaseAddress;
  bound = (void*)((unsigned long)region->pvBaseAddress + region->ulLengthInBytes);
  parameters = region->ulParameters;

  BENCH_START(B_EADD2);
  xReturnValue = tm_encls_add_enclave_region(xTTCB, base, bound, parameters, 2);
  while( xReturnValue != TM_SUCCESS );
  BENCH_STOP(B_EADD2);

  //~ base = &private_buf[0];
  //~ bound = &private_buf[4];
  //~ parameters = portMPU_REGION_READ_WRITE | portMPU_REGION_UTRUSTED;

  region = &pxEnclaveBenchDefinition.xTaskParameters.xRegions[3];
  base = region->pvBaseAddress;
  bound = (void*)((unsigned long)region->pvBaseAddress + region->ulLengthInBytes);
  parameters = region->ulParameters;

  BENCH_START(B_EADD3);
  xReturnValue = tm_encls_add_enclave_region(xTTCB, base, bound, parameters, 3);
  while( xReturnValue != TM_SUCCESS );
  BENCH_STOP(B_EADD3);

  /* Benchmark add data from region[0], length 0 */
  void* dbase = pxEnclaveBenchDefinition.xEnclaveRegions[0].pvBaseAddress;
  void* dbound = pxEnclaveBenchDefinition.xEnclaveRegions[0].pvBaseAddress;
  BENCH_START(B_EDATA0);
  xReturnValue = tm_encls_add_data(xTTCB, dbase, dbound);
  while( xReturnValue != TM_SUCCESS );
  BENCH_STOP(B_EDATA0);

  /* Benchmark add data from region[0], length 0 */
  uword_t** private_buf = (void*)&zzz_secure_func;
  dbase = &private_buf[0];
  dbound = &private_buf[0];
  BENCH_START(B_EDATA1);
  xReturnValue = tm_encls_add_data(xTTCB, dbase, dbound);
  while( xReturnValue != TM_SUCCESS );
  BENCH_STOP(B_EDATA1);

  dbase = &private_buf[0]; /* length 1 word */
  dbound = &private_buf[1];
  BENCH_START(B_EDATA2);
  xReturnValue = tm_encls_add_data(xTTCB, dbase, dbound);
  while( xReturnValue != TM_SUCCESS );
  BENCH_STOP(B_EDATA2);

  dbase = &private_buf[1]; /* length 2 words */
  dbound = &private_buf[2];
  BENCH_START(B_EDATA3);
  xReturnValue = tm_encls_add_data(xTTCB, dbase, dbound);
  while( xReturnValue != TM_SUCCESS );
  BENCH_STOP(B_EDATA3);

  /* Prepare correct enclave */
  configASSERT( TM_SUCCESS == tm_encls_add_data(xTTCB, 
    pxEnclaveBenchDefinition.xEnclaveRegions[0].pvBaseAddress,
    &zzz_secure_func) );
    //pxEnclaveBenchDefinition.xEnclaveRegions[0].pvBoundAddress) );
  configASSERT( TM_SUCCESS == tm_encls_add_data(xTTCB, 
    pxEnclaveBenchDefinition.xEnclaveRegions[1].pvBaseAddress,
    pxEnclaveBenchDefinition.xEnclaveRegions[1].pvBoundAddress) );

  /* Benchmark add entries from region[0] */
  BENCH_START(B_EENTRY0);
  xReturnValue = tm_encls_add_entries( xTTCB, xEntry0 );
  while( xReturnValue != TM_SUCCESS );
  BENCH_STOP(B_EENTRY0);

  BENCH_START(B_EENTRY1);
  xReturnValue = tm_encls_add_entries( xTTCB, xEntry1 );
  while( xReturnValue != TM_SUCCESS );
  BENCH_STOP(B_EENTRY1);

  BENCH_START(B_EENTRY2);
  xReturnValue = tm_encls_add_entries( xTTCB, xEntry2 );
  while( xReturnValue != TM_SUCCESS );
  BENCH_STOP(B_EENTRY2);

  BENCH_START(B_EENTRY3);
  xReturnValue = tm_encls_add_entries( xTTCB, xEntry3 );
  while( xReturnValue != TM_SUCCESS );
  BENCH_STOP(B_EENTRY3);

  /* Prepare for later */
  xReturnValue = tm_encls_add_entries( xTTCB, xOtherEntries );
  while( xReturnValue != TM_SUCCESS );

  BENCH_START(B_EINIT);
  xReturnValue = tm_encls_init_enclave( xTTCB );
  while( xReturnValue != TM_SUCCESS );
  BENCH_STOP(B_EINIT);

  /* Prepare for running the enclave */
  portDISABLE_INTERRUPTS();
  BENCH_START(B_ELOAD);
  xReturnValue = tm_encls_load_enclave( xTTCB );
  while (xReturnValue != TM_SUCCESS );
  BENCH_STOP(B_ELOAD);
  portENABLE_INTERRUPTS();

  portSWITCH_TO_USER_MODE();

  vSecureEntry6();

  /* Benchmark Ecall/Ocall latency */
  BENCH_START(B_ECALL);
  vTaskBenchEcall();
  BENCH_STOP(B_ECALLR);
  vTaskBenchEtoOcall();

  /* Destroy enclave */
  xPortRaisePrivilege();
  configASSERT( TM_SUCCESS == tm_encls_destroy_enclave ( xTTCB ) );

  /* Benchmark destroy enclave */
  configASSERT( TM_SUCCESS == tm_encls_create_enclave( xTTCB ) );
  configASSERT( TM_SUCCESS == tm_encls_init_enclave( xTTCB ) );

  BENCH_START(B_EDESTROY);
  xReturnValue = tm_encls_destroy_enclave( xTTCB );
  while( xReturnValue != TM_SUCCESS );
  BENCH_STOP(B_EDESTROY);

  printf("done\n");
#if TM_BENCH == 1
  /* Benchmark should have aborted before reaching here */
  exit(-1);
#else
  exit(0);
#endif
}

/* Update manually */
SECURE_DATA unsigned char fake_eid[EID_SIZE] = {
   0xCE,0xB1,0x78,0x05,0x4E,0x4A,0x7B,0xE4,0x3C,0xD1,0x22,0x37,0xA8,0x88,0x16,0x1D,0x3E,0xD8,0xE4,0x6F,0x83,0x39,0x01,0xC4,0x30,0x86,0xC7,0xB7,0x66,0x17,0xC8,0xBF,
};

extern tm_errcode_t enclu_bench();

SECURE_DATA unsigned char* bench_shm_start;
SECURE_DATA unsigned char* bench_shm_start;

/*-----------------------------------------------------------*/
SECURE_FUNCTION void vSecureEntry6() {
  tm_errcode_t xReturnValue;

  /* Benchmark B_EAEX */
  /* Benchmark B_ERESUME */
  /* These must be the first ENCLU call because each ENCLU call will act like an AEX without functionality */
  BENCH_START(B_EAEX);
  register long reg7 asm("a7") = 9999; // SYS_interrupt of FreeRTOS
  asm volatile("ecall\n" : : "r"(reg7));
  BENCH_STOP(B_ERESUME);

  /* Benchmark ENCLU call latency */
  BENCH_START(B_ENCLU);
  enclu(ENCLU_BENCH, NULL, 0, NULL);

  /* Benchmark ENCLU return latency */
  enclu(ENCLU_BENCH, NULL, 0, NULL);
  BENCH_STOP(B_ENCLUR);

  /* Benchmark ENCLU_GETKEY */
  unsigned char securekey[TM_KEY_SIZE];
  int secureid;
  BENCH_START(B_EGETKEY);
  xReturnValue = enclu(ENCLU_GETKEY, &secureid, sizeof(secureid), &securekey);
  while( xReturnValue != TM_SUCCESS );
  BENCH_STOP(B_EGETKEY);

  /* Benchmark ENCLU_SHM_OFFER */
  unsigned char shmbuf[TM_KEY_SIZE];
  region_t region;
  region.range.start = shmbuf;
  region.range.end = shmbuf + sizeof(shmbuf);
  region.flags = SHM_ACCESS_UT | SHM_ACCESS_R | SHM_ACCESS_W;
  BENCH_START(B_ESHMOFFER);
  xReturnValue = enclu(ENCLU_SHMOFFER, (long)&fake_eid, (long)&region, 0);
  while( xReturnValue != TM_SUCCESS );
  BENCH_STOP(B_ESHMOFFER);

  /* Benchmark ENCLU_SHM_ACCEPT */
  BENCH_START(B_ESHMACCEPT);
  /* if TM_BENCH==1, ENCLU_SHMACCEPT does not check EID's and considers the first enclave as owner and it's offer towards us. In fact, we open a shm towards ourself */
  xReturnValue = enclu(ENCLU_SHMACCEPT, (long)&fake_eid, &bench_shm_start, &bench_shm_start);
  while( xReturnValue != TM_SUCCESS );
  BENCH_STOP(B_ESHMACCEPT);

  /* Benchmark B_ESHMCLOSE */
  BENCH_START(B_ESHMCLOSE);
  xReturnValue = enclu(ENCLU_SHMCLOSE, bench_shm_start, bench_shm_start, 0);
  while( xReturnValue != TM_SUCCESS );
  BENCH_STOP(B_ESHMCLOSE);
}

NORMAL_FUNCTION void vTaskBenchOcall() {
  BENCH_STOP(B_OCALL);
  printf("vTaskBenchOcall'd\n");
  BENCH_START(B_OCALLR);
}

__attribute__((naked)) // avoid setting up a stack frame. We cannot do any call from a naked function since it does not save return address 'ra' properly
SECURE_FUNCTION void vTaskBenchEcall() {
  BENCH_STOP(B_ECALL);
  BENCH_START(B_ECALLR);
  asm volatile("ret");
}

SECURE_FUNCTION void vTaskBenchEtoOcall() {
  BENCH_START(B_OCALL);
  ocall_vTaskBenchOcall();
  BENCH_STOP(B_OCALLR);
}

// Cause function to be at end of secure functions
_APPS_ATTRIBUTE(APPNAME, MODE_SECURE, TYPE_CODE, ATTR_PRE_END)
void zzz_secure_func() {
  asm volatile("nop\n nop\n nop\n nop\n nop\n nop\n nop\n nop\n nop\n nop\n nop\n nop\n nop\n nop\n nop\n nop\n ret\n");
}

#endif
