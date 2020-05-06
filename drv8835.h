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

extern result_t drv8835_server_init();
extern result_t drv8835_server_listen(int port);
extern result_t drv8835_server_quit();
extern result_t drv8835_server_wait_till_quit();
extern result_t drv8835_server_set_motor_param(uint8_t motor, int16_t param);
extern result_t drv8835_server_set_motor_params(uint8_t motor1, int16_t param1, uint8_t motor2, int16_t param2);

extern result_t drv8835_client_init();
extern result_t drv8835_client_connect(const char* host, int port);
extern result_t drv8835_client_disconnect();
//extern result_t drv8835_client_send_heartbeat();
extern result_t drv8835_client_send_motor_param(uint8_t motor, int16_t param);
extern result_t drv8835_client_send_motor_params(uint8_t motor1, int16_t param1, uint8_t motor2, int16_t param2);

