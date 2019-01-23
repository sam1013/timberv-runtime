// See LICENSE for license details.

#ifndef DEMO_H
#define DEMO_H

#include "queue.h"

/* Task behaviour. */
#define bktQUEUE_LENGTH				( 5 )
#define bktSHORT_WAIT				( ( ( TickType_t ) 20 ) / portTICK_PERIOD_MS )
#define bktPRIMARY_BLOCK_TIME		( 10 )
#define bktALLOWABLE_MARGIN			( 15 )
#define bktTIME_TO_BLOCK			( 175 )
#define bktDONT_BLOCK				( ( TickType_t ) 0 )
#define bktRUN_INDICATOR			( ( UBaseType_t ) 0x55 )

/* Task priorities.  Allow these to be overridden. */
#if ( portUSING_MPU_WRAPPERS == 1 )
  #define bktPRIMARY_PRIORITY		(( configMAX_PRIORITIES - 1 ) | portPRIVILEGE_BIT )
  #define bktSECONDARY_PRIORITY		(( configMAX_PRIORITIES - 2 ) | portPRIVILEGE_BIT )
#else
  #define bktPRIMARY_PRIORITY		( configMAX_PRIORITIES - 1 )
  #define bktSECONDARY_PRIORITY		( configMAX_PRIORITIES - 2 )
#endif

extern QueueHandle_t xTestQueue;

void assertPriv(BaseType_t expectedPriv);

/* Used to ensure that tasks are still executing without error. */
static volatile BaseType_t xPrimaryCycles = 0, xSecondaryCycles = 0;

#endif /* DEMO_H */
