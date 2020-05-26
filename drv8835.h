#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <memory.h>
#include "types.h"

#define EXPORT __attribute__ ((visibility("default")))

#ifdef RASPI
extern EXPORT result_t	drv8835_server_init();
extern EXPORT result_t	drv8835_server_listen(int port);
extern EXPORT result_t	drv8835_server_quit();
extern EXPORT result_t	drv8835_server_wait_till_quit();
extern EXPORT bool		drv8835_server_is_connected();
extern EXPORT result_t	drv8835_server_force_quit();
extern EXPORT result_t	drv8835_server_set_motor_param(uint8_t motor, int16_t param);
extern EXPORT result_t	drv8835_server_set_motor_params(uint8_t motor1, int16_t param1, uint8_t motor2, int16_t param2);
#endif

extern EXPORT result_t	drv8835_client_connect(const char* host, int port);
extern EXPORT result_t	drv8835_client_disconnect();
extern EXPORT bool		drv8835_client_is_connected();
extern EXPORT result_t	drv8835_client_send_motor_param(uint8_t motor, int16_t param);
extern EXPORT result_t	drv8835_client_send_motor_params(uint8_t motor1, int16_t param1, uint8_t motor2, int16_t param2);

