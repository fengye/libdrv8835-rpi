#pragma once
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

extern const int DEFAULT_PORT;
extern const uint8_t HANDSHAKE_CLIENT[2];
extern const uint8_t HANDSHAKE_SERVER[2];
extern const int MAX_MISSING_HEARTBEAT;
extern const uint8_t MOTOR_PARAM_PACKET_MAGIC1[2];
extern const uint8_t MOTOR_PARAM_PACKET_MAGIC2[2];
/* header
 * content_1 
 * content_2
 * ...
 * content_N
 * footer
 */
typedef struct __attribute__((packed)) _motor_param_packet_header
{
	uint8_t magic1[2];
	uint8_t len_bytes;
} motor_param_packet_header;

typedef struct __attribute__((packed)) _motor_param_packet_content
{
	uint8_t motor;
	int16_t value;
} motor_param_packet_content;

typedef struct __attribute__((packed)) __motor_param_packet_footer
{
	uint8_t magic2[2];
} motor_param_packet_footer;

extern void check_root();

extern bool is_root();

extern void log_info(const char* msg);
	
extern void log_error(const char* msg);

extern void error_exit(const char *msg, int exit_code);

extern int check_packet_header(motor_param_packet_header* header);

extern int check_packet_header_by_byte(motor_param_packet_header* header, int byte_index);

extern int check_packet_footer(motor_param_packet_footer* footer);

extern motor_param_packet_header* allocate_packet(int param_count);
	
extern int set_packet_param(motor_param_packet_header* header, int idx, int motor, int param);

extern int extract_packet_param(motor_param_packet_header* header, int idx, motor_param_packet_content* out);

extern void free_packet(motor_param_packet_header* header);

extern void debug_packet(FILE* stream, motor_param_packet_header* header);

