#include "common.h"
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

const int DEFAULT_PORT = 9939;
const uint8_t HANDSHAKE_CLIENT[] = {0xCA, 0xFE};
const uint8_t HANDSHAKE_SERVER[] = {0xBE, 0xEF};
const int MAX_MISSING_HEARTBEAT = 5;
const uint8_t MOTOR_PARAM_PACKET_MAGIC1[] = {0xCD, 0xCD};
const uint8_t MOTOR_PARAM_PACKET_MAGIC2[] = {0xDC, 0xDC};

const int LOG_DEBUG = 1;
const int LOG_INFO = 3;
const int LOG_ERROR = 5;

static FILE* debug_out = NULL;
static FILE* info_out = NULL;
static FILE* error_out = NULL;
static int log_level = LOG_DEBUG;

void check_root()
{
    if(geteuid() != 0)
    {
        fprintf(stderr, "This program requires root.\n");
        exit(EXIT_FAILURE);
    }

}

bool is_root()
{
	return geteuid() == 0;
}

void log_init(FILE* debug, FILE* info, FILE* error)
{
	debug_out = debug;
	info_out = info;
	error_out = error;
}

void log_setlevel(int level)
{
	log_level = level;
}

void log_debug(const char* format, ...)
{
	if (log_level >  LOG_DEBUG)
		return;
	va_list args;
	va_start (args, format);
	vfprintf (debug_out?debug_out:stdout, format, args);
	va_end (args);
	fputc('\n', debug_out?debug_out:stdout);
}

void log_debug_nocr(const char* format, ...)
{
	if (log_level > LOG_DEBUG)
		return;
	va_list args;
	va_start (args, format);
	vfprintf (debug_out?debug_out:stdout, format, args);
	va_end (args);
}

void log_info(const char* format, ...)
{
	if (log_level >  LOG_INFO)
		return;
	va_list args;
	va_start (args, format);
	vfprintf (info_out?info_out:stdout, format, args);
	va_end (args);
	fputc('\n', info_out?info_out:stdout);
}

void log_info_nocr(const char* format, ...)
{
	if (log_level > LOG_INFO)
		return;
	va_list args;
	va_start (args, format);
	vfprintf (info_out?info_out:stdout, format, args);
	va_end (args);
}

void log_error(const char* format, ...)
{
	if (log_level > LOG_ERROR)
		return;
	va_list args;
	va_start (args, format);
	vfprintf (error_out?error_out:stderr, format, args);
	va_end (args);
	fputc('\n', error_out?error_out:stderr);
}

void log_error_nocr(const char* format, ...)
{
	if (log_level > LOG_ERROR)
		return;

	va_list args;
	va_start (args, format);
	vfprintf (error_out?error_out:stderr, format, args);
	va_end (args);
}

void error_exit(const char *msg, int exit_code)
{
    perror(msg);
    exit(exit_code);
}

int check_packet_header(motor_param_packet_header* header)
{
	if (!header)
		return -1;
	if (memcmp(header->magic1, MOTOR_PARAM_PACKET_MAGIC1, sizeof(MOTOR_PARAM_PACKET_MAGIC1)) != 0)
		return -1;
	return (int)header->len_bytes;	
}

int check_packet_header_by_byte(motor_param_packet_header* header, int byte_index)
{
	if (!header)
		return -1;
	switch(byte_index)
	{
		case 0:
			if (header->magic1[0] == MOTOR_PARAM_PACKET_MAGIC1[0])
			{
				return 0;
			}
			break;
		case 1:
			if (header->magic1[1] == MOTOR_PARAM_PACKET_MAGIC1[1])
			{
				return 0;
			}
			break;

		case 2:
			if (header->len_bytes >= sizeof(motor_param_packet_header) + sizeof(motor_param_packet_footer))
			{
				return 0;
			}
		default:
			return -1;
	}

	return -1;
}



int check_packet_footer(motor_param_packet_footer* footer)
{
	if (!footer)
		return -1;
	if (memcmp(footer->magic2, MOTOR_PARAM_PACKET_MAGIC2, sizeof(MOTOR_PARAM_PACKET_MAGIC2)) != 0)
		return -1;
	return 0;
}

motor_param_packet_header* allocate_packet(int param_count)
{
	if (param_count < 0)
		return NULL;

	// too many params will overflow len_bytes
	if (param_count > 70)
		return NULL;

	size_t length = sizeof(motor_param_packet_header) + sizeof(motor_param_packet_footer) + param_count * sizeof(motor_param_packet_content);
	motor_param_packet_header *header = (motor_param_packet_header*)malloc(length);
	memcpy(header->magic1, MOTOR_PARAM_PACKET_MAGIC1, sizeof(MOTOR_PARAM_PACKET_MAGIC1));
	header->len_bytes = (uint8_t)length;

	motor_param_packet_content* content_start = (motor_param_packet_content*)((uint8_t*)header + sizeof(motor_param_packet_header));
	/* Initialise content block */
	for(int i = 0; i < param_count; ++i)
	{
		content_start[i].motor = 0xFF;
		content_start[i].value = 0;
	}

	motor_param_packet_footer* footer = (motor_param_packet_footer*)((uint8_t*)header + sizeof(motor_param_packet_header) + param_count * sizeof(motor_param_packet_content));
	memcpy(footer->magic2, MOTOR_PARAM_PACKET_MAGIC2, sizeof(MOTOR_PARAM_PACKET_MAGIC2)); 
	return header; 
} 

int set_packet_param(motor_param_packet_header* header, int idx, int motor, int param) 
{ 
	if (!header)
		return -1;
	size_t count = ((size_t)header->len_bytes - sizeof(motor_param_packet_header) - sizeof(motor_param_packet_footer)) / sizeof(motor_param_packet_content);
	if (idx >= count)
		return -1;

	motor_param_packet_content* content = (motor_param_packet_content*)((uint8_t*)header + sizeof(motor_param_packet_header));
	content[idx].motor = (uint8_t)motor;
	content[idx].value = (int16_t)param;

	return 0;
}

int extract_packet_param(motor_param_packet_header* header, int idx, motor_param_packet_content* out)
{
	if (!header)
		return -1;
	size_t count = ((size_t)header->len_bytes - sizeof(motor_param_packet_header) - sizeof(motor_param_packet_footer)) / sizeof(motor_param_packet_content);
	if (idx >= count)
		return -1;

	motor_param_packet_content* content_start = (motor_param_packet_content*)((uint8_t*)header + sizeof(motor_param_packet_header));
	motor_param_packet_content* content = content_start + idx;
	memcpy(out, content, sizeof(motor_param_packet_content));
	return 0;
}

void free_packet(motor_param_packet_header* header)
{
	free(header);
}

void debug_packet(FILE* stream, motor_param_packet_header* header)
{
	uint8_t *ptr = (uint8_t *)header;
	size_t bytes = header->len_bytes;

	
	log_debug("Packet(%d): ", bytes);
	for(size_t i = 0; i < bytes; ++i)
	{
		log_debug_nocr("%.2X ", ptr[i]);
	}
	log_debug("");
}
