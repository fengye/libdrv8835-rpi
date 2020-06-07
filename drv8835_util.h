#pragma once

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

extern bool drv8835_is_root();

extern void drv8835_log_init(FILE* debug, FILE* info, FILE* error);
extern void drv8835_log_setlevel(int loglevel);

extern void drv8835_log_debug(const char* msg, ...);
extern void drv8835_log_debug_nocr(const char* msg, ...);

extern void drv8835_log_info(const char* msg, ...);
extern void drv8835_log_info_nocr(const char* msg, ...);
	
extern void drv8835_log_error(const char* msg, ...);
extern void drv8835_log_error_nocr(const char* msg, ...);

