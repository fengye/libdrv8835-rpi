#pragma once
#include "types.h"
#include <stdbool.h>

extern result_t socket_server_init(int port);
extern bool socket_server_is_initialised();
extern result_t socket_server_start();
extern result_t socket_server_wait_till_stop();
extern result_t socket_server_stop();

