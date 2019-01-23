// See LICENSE for license details.

#ifndef APPS_H
#define APPS_H

#include "trustmon.h"
#include "enclave_api.h"

#ifdef APPNAME

#define MODE_NORMAL normal
#define MODE_SECURE secure
#define TYPE_CODE   text
#define TYPE_DATA   data

/* Defines ordering of symbols. 
 * ATTR_MIDDLE must be within ATTR_START and ATTR_END 
 * The linker script will alphabetically sort symbols using SORT_BY_NAME
 */
#define ATTR_START  a  /* Used for auto-generating start symbol */
#define ATTR_MIDDLE b  /* Used for actual code */
#define ATTR_PRE_END c /* End of code section but before ATTR_END */
#define ATTR_END    z  /* Used for auto-generating end symbol */

/* Function attributes to be used by programmer */
#define NORMAL_FUNCTION _APPS_ATTRIBUTE(APPNAME, MODE_NORMAL, TYPE_CODE, ATTR_MIDDLE)
#define SECURE_FUNCTION _APPS_ATTRIBUTE(APPNAME, MODE_SECURE, TYPE_CODE, ATTR_MIDDLE)

/* Data attributes to be used by programmer */
#define NORMAL_DATA _APPS_ATTRIBUTE(APPNAME, MODE_NORMAL, TYPE_DATA, ATTR_MIDDLE)
#define SECURE_DATA _APPS_ATTRIBUTE(APPNAME, MODE_SECURE, TYPE_DATA, ATTR_MIDDLE)


/* one more level of indirection to make macro expansion work */
#define _APPS_ATTRIBUTE(app, mode, type, pos) __APPS_ATTRIBUTE(app, mode, type, pos)

#define __APPS_ATTRIBUTE(app, mode, type, pos) __attribute__((section(".app."#app"."#mode"."#type"."#pos),aligned(8)))

/* Definition of start/end symbols.
 * By instanciating APP_START it will create the following symbols
 * to determine boundaries for functions.
 * E.g. If APPNAME = demo, the following symbols are created:
 * 
 * app_demo_normal_start
 * app_demo_normal_end
 * app_demo_secure_start
 * app_demo_secure_end
 */
#define DEFINE_ALL_APP_BOUNDARY_SYMBOLS \
  _DEFINE_APP_BOUNDARY_SYMBOLS(APPNAME, MODE_NORMAL, TYPE_CODE) \
  _DEFINE_APP_BOUNDARY_SYMBOLS(APPNAME, MODE_SECURE, TYPE_CODE) \
  _DEFINE_APP_BOUNDARY_SYMBOLS(APPNAME, MODE_NORMAL, TYPE_DATA) \
  _DEFINE_APP_BOUNDARY_SYMBOLS(APPNAME, MODE_SECURE, TYPE_DATA)

/* one more level of indirection to make macro expansion work */
#define _DEFINE_APP_BOUNDARY_SYMBOLS(app, mode, type) \
  __DEFINE_APP_BOUNDARY_SYMBOLS(app, mode, type)

#define __DEFINE_APP_BOUNDARY_SYMBOLS(app, mode, type) \
  _APPS_ATTRIBUTE(app, mode, type, ATTR_START) \
  uword_t app_##app##_##mode##_##type##_start[1]; \
  _APPS_ATTRIBUTE(app, mode, type, ATTR_END) \
  uword_t app_##app##_##mode##_##type##_end[1];

#define vSET_REGION(xRegions, idx, pvBase, pvBound, ulParams) do { \
  xRegions[ idx ].pvBaseAddress = (pvBase); \
  xRegions[ idx ].ulLengthInBytes = (unsigned long)(pvBound) - (unsigned long)xRegions[ idx ].pvBaseAddress; \
  xRegions[ idx ].ulParameters = (ulParams); } while(0)

#define vSET_ENCLAVE_REGION(xRegions, idx, pvBase, pvBound) do { \
  xRegions[ idx ].pvBaseAddress = (pvBase); \
  xRegions[ idx ].pvBoundAddress = (pvBound); } while(0)

#define GEN_INIT _GEN_INIT(APPNAME)
#define _GEN_INIT(app) __GEN_INIT(app)

#define __GEN_INIT(app) \
void vInitTask##app##Ranges(TaskParameters_t* pxTaskDefinition); \
void vInitTask##app##Ranges(TaskParameters_t* pxTaskDefinition) { \
  /* Do at load-time because linker cannot know symbols at link time */ \
  vSET_REGION(pxTaskDefinition->xRegions, 0, app_##app##_normal_text_start, app_##app##_secure_text_end, portMPU_REGION_EXECUTE    | portMPU_REGION_UTRUSTED); \
  vSET_REGION(pxTaskDefinition->xRegions, 1, app_##app##_normal_data_start, app_##app##_secure_data_end, portMPU_REGION_READ_WRITE | portMPU_REGION_UTRUSTED); \
} \
void vInitTask##app##Tags(EnclaveRegion_t pxTagRegion[ MAX_ENCLAVE_REGIONS ]); \
void vInitTask##app##Tags(EnclaveRegion_t pxTagRegion[ MAX_ENCLAVE_REGIONS ]) { \
  /* Do at load-time because linker cannot know symbols at link time */ \
  vSET_ENCLAVE_REGION(pxTagRegion, 0, app_##app##_secure_text_start, app_##app##_secure_text_end); \
  vSET_ENCLAVE_REGION(pxTagRegion, 1, app_##app##_secure_data_start, app_##app##_secure_data_end); \
}

DEFINE_ALL_APP_BOUNDARY_SYMBOLS

GEN_INIT

#else /* APPNAME */
#error "Define APPNAME first before including apps.h!"
#endif /* APPNAME */

#endif /* APPS_H */
