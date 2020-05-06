#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <pthread.h>
#include "socket_server.h"
#include "motor_server.h"
#include "common.h"

static bool server_initialised = false;
static bool server_running = false;
static int sockfd;
static pthread_t server_thread;
static pthread_mutex_t server_init_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t server_start_lock = PTHREAD_MUTEX_INITIALIZER;

result_t socket_server_init(int port)
{
	result_t result = 0;
	pthread_mutex_lock(&server_init_lock);

    struct sockaddr_in srv_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
	{
		result = -1;
		goto END;
	}

    memset(&srv_addr, 0, sizeof(struct sockaddr_in));
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_addr.s_addr = INADDR_ANY;
    srv_addr.sin_port = htons(port);
    if (bind(sockfd, (struct sockaddr*)&srv_addr, sizeof(struct sockaddr_in)) < 0)
    {
		result = -1;
		goto END;
    }

	server_initialised = true;
END:

	pthread_mutex_unlock(&server_init_lock);
	return result;
}

bool socket_server_is_initialised()
{
	bool result;
	pthread_mutex_lock(&server_init_lock);
	result = server_initialised;
	pthread_mutex_unlock(&server_init_lock);
	return result;
}

// TODO: move heartbeat checker to another thread created here
void* _server_loop(void* ptr)
{
    if (listen(sockfd, 5) < 0)
    {
		log_error("Error on listening");
		return NULL;
    }

    struct sockaddr_in clt_addr;
    socklen_t cltlen = sizeof(struct sockaddr_in);
ACCEPT_CONNECTION:
    log_info("Now accepting connections...");
    int accept_sockfd = accept(sockfd, (struct sockaddr*)&clt_addr, &cltlen);
    if (accept_sockfd < 0)
    {
		log_error("Error accepting new socket");
		return NULL;
    }
    log_info("Now initiating handshake... ");

    uint8_t buf[2];
    memset(buf, 0, sizeof(buf));
    int n = read(accept_sockfd, buf, sizeof(buf));
    if (n < 0)
    {
		log_error("Error reading handshake from socket");
		return NULL;
    }
	if (memcmp(buf, HANDSHAKE_CLIENT, sizeof(HANDSHAKE_CLIENT)) != 0)
	{
		log_error("Handshake failed");
		return NULL;
	}
    log_info("Received correct handshake from client");
    log_info("Now sending handshake back to client... ");
    n = write(accept_sockfd, HANDSHAKE_SERVER, sizeof(HANDSHAKE_SERVER));
    if (n < 0)
    {
		log_error("Error writing handshake to socket");
		return NULL;
    }

	log_info("Handshake done.");

	motor_param_packet_header tryout_header;
	uint8_t *tryout_buf = (uint8_t *)&tryout_header;
	int tryout_buf_idx = 0;
	int packet_counter = 0;
	int heartbeat_counter = 0;
	int missed_heartbeat = 0;
	time_t last_heartbeat_time = time(NULL);
	while(1)
	{
		bool should_exit = false;
		pthread_mutex_lock(&server_start_lock);
		should_exit = !server_running;
		pthread_mutex_unlock(&server_start_lock);

		if (should_exit)
		{
			log_info("Signaled exit.");
			break;
		}

		if (tryout_buf_idx < sizeof(motor_param_packet_header))
		{
			n = read(accept_sockfd, tryout_buf + tryout_buf_idx, 1);
			if (n > 0)
			{
				if (check_packet_header_by_byte(&tryout_header, tryout_buf_idx) == 0)
				{
					tryout_buf_idx++;
					continue;
				}
				else
				{
					// wrong heade,
					// memmove tryout_buf one byte forward
					memmove(tryout_buf, tryout_buf+1, sizeof(motor_param_packet_header)-1);
					//fprintf(stderr, "Wrong header at %d\n", tryout_buf_idx);
					// reset tryout index
					tryout_buf_idx = 0;
					continue;
				}
			}
			else
			if (n < 0)
			{
				log_error("Error reading packet");
				return NULL;
			}
			else
			{
				// read nothing, check for heartbeat missing
				if (time(NULL) - last_heartbeat_time >= 1)
				{
					missed_heartbeat = time(NULL) - last_heartbeat_time;
				}
			}

		}
		else
		{
			tryout_buf_idx = 0;
			// proceed the rest and check the tail
			motor_param_packet_header *packet;
			motor_param_packet_content *content;
			motor_param_packet_footer *footer;
			int count = (tryout_header.len_bytes - sizeof(motor_param_packet_header) - sizeof(motor_param_packet_footer)) / sizeof(motor_param_packet_content);
			bool is_heartbeat = count == 0;
			if (!is_heartbeat)
				printf("Allocate %d content\n", count);
			if (tryout_header.len_bytes != sizeof(motor_param_packet_header) + count * sizeof(motor_param_packet_content) + sizeof(motor_param_packet_footer))
			{
				fprintf(stderr, "Inconsistent packet length (%d), give it up\n", tryout_header.len_bytes);
				continue;
			}
			packet = allocate_packet(count);
			if (!packet)
			{
				fprintf(stderr, "Cannot allocate %d content, either memory is out or the number isn't right!\n", count);
				continue;
			}
			memcpy(packet, &tryout_header, sizeof(tryout_header));
			content = (motor_param_packet_content*)((uint8_t*)packet + sizeof(motor_param_packet_header));
			footer = (motor_param_packet_footer*)((uint8_t*)packet + packet->len_bytes - sizeof(motor_param_packet_footer));

			n = read(accept_sockfd, content, packet->len_bytes - sizeof(motor_param_packet_header));
			if (n > 0)
			{
				debug_packet(stdout, packet); 
				

				if (n == packet->len_bytes - sizeof(motor_param_packet_header) && 
					check_packet_header(packet) == packet->len_bytes && 
					check_packet_footer(footer) == 0)
				{
					if (is_heartbeat)
					{
						printf("Heartbeat #%d\n", heartbeat_counter);
						heartbeat_counter++;
						last_heartbeat_time = time(NULL);
						missed_heartbeat = 0;
					}
					else
					{
						printf("#%d packet\n", packet_counter);
						packet_counter++;
						motor_param_packet_content content;
						for(int i = 0; i < count; ++i)
						{
							if (extract_packet_param(packet, i, &content) == 0)
							{
								printf("Recv param %d (%d, %d)\n", i, content.motor, content.value);
								motor_server_set_speed(content.motor, content.value);
							}
						}
					}
				}
				else
				{
					log_error("Wrong packet content");
				}
			}
			else
			if (n < 0)
			{
				log_error("Error reading packet");
				return NULL;
			}
			else 
			{
				// read nothing, check missing heartbeat
				if (time(NULL) - last_heartbeat_time >= 1)
				{
					missed_heartbeat = time(NULL) - last_heartbeat_time;
				}
			}
			free_packet(packet);

			
		}
		
		if (missed_heartbeat >= MAX_MISSING_HEARTBEAT)
		{
			log_error("Too many missing heartbeat. Stop.");
			close(accept_sockfd);
			tryout_buf_idx = 0;
			packet_counter = 0;
			heartbeat_counter = 0;
			missed_heartbeat = 0;
			goto ACCEPT_CONNECTION;
		}
	}
    log_info("Finished server loop.");
    close(accept_sockfd);

	return NULL;
}

result_t socket_server_start()
{
	result_t result = 0;

	pthread_mutex_lock(&server_start_lock);
	server_running = true;
	pthread_mutex_unlock(&server_start_lock);
	if (pthread_create(&server_thread, NULL, _server_loop, NULL) != 0)
	{
		pthread_mutex_lock(&server_start_lock);
		server_running = false;
		pthread_mutex_unlock(&server_start_lock);
		result = -1;
	}	

	return result;
}

result_t socket_server_wait_till_stop()
{
	result_t result = 0;
	pthread_mutex_lock(&server_start_lock);
	bool is_running = server_running;
	pthread_mutex_unlock(&server_start_lock);
	if (is_running)
	{
		if (pthread_join(server_thread, NULL) != 0)
		{
			log_error("Cannot join server thread.");
			result = -1;
		}
	}

	return result;
}

result_t socket_server_stop()
{
	result_t result = 0;
	pthread_mutex_lock(&server_start_lock);
	bool is_running = server_running;
	pthread_mutex_unlock(&server_start_lock);
	if (is_running)
	{
		pthread_mutex_lock(&server_start_lock);
		server_running = false;
		pthread_mutex_unlock(&server_start_lock);
		pthread_join(server_thread, NULL);
	}

	pthread_mutex_lock(&server_init_lock);
    close(sockfd);
	pthread_mutex_unlock(&server_init_lock);

	return result;
}

