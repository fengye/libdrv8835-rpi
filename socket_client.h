#pragma once
#include "types.h"
#include <stdbool.h>
#include <stdint.h>

extern result_t	socket_client_connect(const char *host, int port);
extern result_t	socket_client_disconnect();
extern bool		socket_client_is_connected();
extern result_t	socket_client_send_motor_param(uint8_t motor, int16_t param);
extern result_t socket_client_send_motor_params(uint8_t motor0, int16_t param0, uint8_t motor1, int16_t param1);
