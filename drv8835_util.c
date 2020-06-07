#include "drv8835_util.h"
#include <unistd.h>
#include <sys/types.h>
#include <stdarg.h>

const int LOG_DEBUG = 1;
const int LOG_INFO = 3;
const int LOG_ERROR = 5;


static FILE* debug_out = NULL;
static FILE* info_out = NULL;
static FILE* error_out = NULL;
static int log_level = 1;


bool drv8835_is_root()
{
	return geteuid() == 0;
}

void drv8835_log_init(FILE* debug, FILE* info, FILE* error)
{
	debug_out = debug;
	info_out = info;
	error_out = error;
}

void drv8835_log_setlevel(int level)
{
	log_level = level;
}

void drv8835_log_debug(const char* format, ...)
{
	if (log_level >  LOG_DEBUG)
		return;
	va_list args;
	va_start (args, format);
	vfprintf (debug_out?debug_out:stdout, format, args);
	va_end (args);
	fputc('\n', debug_out?debug_out:stdout);
}

void drv8835_log_debug_nocr(const char* format, ...)
{
	if (log_level > LOG_DEBUG)
		return;
	va_list args;
	va_start (args, format);
	vfprintf (debug_out?debug_out:stdout, format, args);
	va_end (args);
}

void drv8835_log_info(const char* format, ...)
{
	if (log_level >  LOG_INFO)
		return;
	va_list args;
	va_start (args, format);
	vfprintf (info_out?info_out:stdout, format, args);
	va_end (args);
	fputc('\n', info_out?info_out:stdout);
}

void drv8835_log_info_nocr(const char* format, ...)
{
	if (log_level > LOG_INFO)
		return;
	va_list args;
	va_start (args, format);
	vfprintf (info_out?info_out:stdout, format, args);
	va_end (args);
}

void drv8835_log_error(const char* format, ...)
{
	if (log_level > LOG_ERROR)
		return;
	va_list args;
	va_start (args, format);
	vfprintf (error_out?error_out:stderr, format, args);
	va_end (args);
	fputc('\n', error_out?error_out:stderr);
}

void drv8835_log_error_nocr(const char* format, ...)
{
	if (log_level > LOG_ERROR)
		return;

	va_list args;
	va_start (args, format);
	vfprintf (error_out?error_out:stderr, format, args);
	va_end (args);
}


