#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include "drv8835.h"
#include "common.h"

static void _on_interrupt()
{
	drv8835_server_quit();
	sleep(1);
	exit(0);
}

int main(int argc, char *argv[]) {

	int time = -1;

	if (argc > 2 && strcmp(argv[1], "--time") == 0)
	{
		time = atoi(argv[2]);
	}


	if (drv8835_server_init() != 0)
		exit(EXIT_FAILURE);

	if (drv8835_server_listen(DEFAULT_PORT) != 0)
		exit(EXIT_FAILURE);

    signal(SIGINT, _on_interrupt);

	if (time >= 0)
	{
		log_info("Server will run for %d seconds", time);
		sleep(time);
		if (drv8835_server_quit() != 0)
			exit(EXIT_FAILURE);
	}
	else
	{
		int c = 0;
		do 
		{
			c = fgetc(stdin);
			if (c == 'q' || c == 'Q')
			{
				if (drv8835_server_is_connected())
				{
					if (drv8835_server_quit() != 0)
						exit(EXIT_FAILURE);
				}
				else
				{
					if (drv8835_server_force_quit() != 0)
						exit(EXIT_FAILURE);
				}

				break;
			}
		} while(c != EOF);
	}

	if (drv8835_server_wait_till_quit() != 0) 
		exit(EXIT_FAILURE);

	log_info("Exit test.");
	exit(0);
}
