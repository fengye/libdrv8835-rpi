#include "drv8835.h"
#include "drv8835_util.h"
#include <stdbool.h>
#include <stdint.h>
#include <SDL2/SDL.h>

int main(int argc, const char** argv)
{
	const char* hostname = "127.0.0.1";

	if (argc > 2 && (strcmp(argv[1], "--host") == 0 || strcmp(argv[1], "-h") == 0))
	{
		hostname = argv[2];
	}

	drv8835_log_setlevel(LOG_INFO);

	if (drv8835_client_connect(hostname, DEFAULT_PORT) < 0)
	{
		drv8835_log_error("Cannot connect to %s", hostname);
		return -1;
	}

	if (SDL_Init(SDL_INIT_VIDEO) != 0)
	{
		drv8835_log_error("SDL initialisation failed: %s", SDL_GetError());
		return -1;
	}

	if (SDL_InitSubSystem(SDL_INIT_EVENTS) != 0)
	{
		drv8835_log_error("SDL event subsystem initialisation failed.");
	}
	else
	{
		SDL_Window *window;
		SDL_Renderer *renderer;

		window = SDL_CreateWindow("SDL client test", 
			SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
			320, 240, SDL_WINDOW_OPENGL);	
		if (!window)
		{
			drv8835_log_error("Cannot create SDL window");
			goto DESTROY_WINDOW;
		}

		renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
		if (!renderer)
		{
			drv8835_log_error("Cannot create SDL renderer");
			goto DESTROY_RENDERER;
		}
		
		SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);

		int16_t up_state[2] = {0, 0};
		int16_t down_state[2] = {0, 0};

		bool running = true;
		while(running)
		{
			SDL_Event event;

			while(SDL_PollEvent(&event))
			{
				switch(event.type)
				{
					case SDL_KEYDOWN:
						if (event.key.repeat == 0)
						{
							bool valid_key = false;
							switch (event.key.keysym.sym)
							{
								case SDLK_w:
									up_state[0] = MAX_SPEED;
									valid_key = true;
									break;
								case SDLK_s:
									down_state[0] = -MAX_SPEED;
									valid_key = true;
									break;
								case SDLK_i:
									up_state[1] = MAX_SPEED;
									valid_key = true;
									break;
								case SDLK_k:
									down_state[1] = -MAX_SPEED;
									valid_key = true;
									break;
							}
							if (valid_key)
							{
								drv8835_log_debug("Key down(%d): 0x%02X", event.key.repeat, event.key.keysym.scancode );
								drv8835_client_send_motor_params(MOTOR0, up_state[0] + down_state[0], MOTOR1, up_state[1] + down_state[1]);
							}
						}
						break;

					case SDL_KEYUP:
						if (event.key.repeat == 0)
						{
							bool valid_key = false;
							switch (event.key.keysym.sym)
							{
								case SDLK_w:
									up_state[0] = 0;
									valid_key = true;
									break;
								case SDLK_s:
									down_state[0] = 0;
									valid_key = true;
									break;
								case SDLK_i:
									up_state[1] = 0;
									valid_key = true;
									break;
								case SDLK_k:
									down_state[1] = 0;
									valid_key = true;
									break;
							}
							if (valid_key)
							{
								drv8835_log_debug("Key up(%d): 0x%02X", event.key.repeat, event.key.keysym.scancode );
								drv8835_client_send_motor_params(MOTOR0, up_state[0] + down_state[0], MOTOR1, up_state[1] + down_state[1]);
							}
						}
						break;

					case SDL_QUIT:
						running = false;
						break;

					default:
						break;
				}

				SDL_RenderClear(renderer);
				SDL_RenderPresent(renderer);
				
			};
		};
DESTROY_RENDERER:
		if (renderer)
			SDL_DestroyRenderer(renderer);
DESTROY_WINDOW:
		if (window)
			SDL_DestroyWindow(window);
		
	}

	SDL_Quit();
	drv8835_client_disconnect();

	return 0;
}
