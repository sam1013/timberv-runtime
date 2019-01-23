#ifndef PORTCONFIG_H
#define PORTCONFIG_H

#define portMACHINE_KERNEL    0
#define portSUPERVISOR_KERNEL 1

#define portUSING_MPU_WRAPPERS		1
#define portPRIVILEGE_BIT			( 0x80000000UL ) /* is multiplexed onto task priority */

#define portUSING_TM		1

#endif // PORTCONFIG_H
