#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include "drv8835.h"
#include "common.h"

int main(int argc, char *argv[]) {

	if (drv8835_server_init() != 0)
		exit(EXIT_FAILURE);

	if (drv8835_server_listen(DEFAULT_PORT) != 0)
		exit(EXIT_FAILURE);

	if (drv8835_server_wait_till_quit() != 0)
		exit(EXIT_FAILURE);

	log_info("Exit");
	exit(0);
}
