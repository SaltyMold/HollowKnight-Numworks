#ifndef EADK_VARS_H
#define EADK_VARS_H

#include <stdint.h>

extern const char eadk_app_name[] __attribute__((section(".rodata.eadk_app_name")));
extern const uint32_t eadk_api_level __attribute__((section(".rodata.eadk_api_level")));

#endif