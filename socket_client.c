#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include "socket_client.h"
#include "common.h"

static int sockfd;
static pthread_mutex_t client_running_lock = PTHREAD_MUTEX_INITIALIZER;
static bool client_running = false;
static pthread_mutex_t sock_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_t client_thread;
static pthread_t heartbeat_thread;
// cond var
static bool sending_param = false;
static int16_t motor_params[2] = {0, 0};
static pthread_mutex_t send_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t send_cond = PTHREAD_COND_INITIALIZER;

static void	*_heartbeat_loop(void*);
static void	*_client_loop(void*);

result_t socket_client_connect(const char *hostname, int port)
{
	struct sockaddr_in srv_addr;
	struct hostent *server;

	LOCK(&sock_lock);
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
	{
		log_error("Cannot create socket.");
		return -1;
	}

	server = gethostbyname(hostname);
	if (!server)
	{
		log_error("Error resolving host: %s", hostname);
		return -1;
	}
	memset(&srv_addr, 0, sizeof(struct sockaddr_in));
	srv_addr.sin_family = AF_INET;
	memcpy(&srv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
	srv_addr.sin_port = htons(port);
	if (connect(sockfd, (struct sockaddr *)&srv_addr, sizeof(struct sockaddr_in)) < 0)
	{
		log_error("Error connecting to server.");
		return -1;
	}

	int n = write(sockfd, HANDSHAKE_CLIENT, sizeof(HANDSHAKE_CLIENT));
	if (n < 0)
	{
		log_error("Error writing handshake to socket");
		return -1;
	}

	uint8_t reply_buf[2];
	memset(reply_buf, 0, sizeof(reply_buf));
	n = read(sockfd, reply_buf, sizeof(reply_buf));
	if (n < 0)
	{
		log_error("Error reading handshake from socket");
		return -1;
	}
	if (memcmp(reply_buf, HANDSHAKE_SERVER, sizeof(HANDSHAKE_SERVER)) != 0)
	{
		log_error("Handshake failed");
		return -1;
	}

	log_info("Successfully handshaked");
	UNLOCK(&sock_lock);

	LOCK(&client_running_lock);
	client_running = true;
	UNLOCK(&client_running_lock);

	if (pthread_create(&heartbeat_thread, NULL, _heartbeat_loop, NULL) != 0)
	{
		LOCK(&client_running_lock);
		client_running = false;
		UNLOCK(&client_running_lock);
		log_error("Cannot create heartbeat thread.");
		return -1;
	}
	else {
		if (pthread_create(&client_thread, NULL, _client_loop, NULL) != 0)
		{
			LOCK(&client_running_lock);
			client_running = false;
			UNLOCK(&client_running_lock);
			log_error("Cannot create client thread.");
			return -1;
		}
	}
	return 0;
}

result_t socket_client_disconnect()
{
	bool is_connected;
	LOCK(&client_running_lock);
	is_connected = client_running;
	if (client_running)
	{
		client_running = false;

		// make sure client thread blocked on cond var will also get a chance toresponde to client_running
		LOCK(&send_lock);
		sending_param = true;
		motor_params[MOTOR0] = 0;
		motor_params[MOTOR1] = 0;
		UNLOCK(&send_lock);
		pthread_cond_signal(&send_cond);
	}
	UNLOCK(&client_running_lock);
	if (is_connected)
	{
		if (pthread_join(client_thread, NULL) == 0)
		{
			if (pthread_join(heartbeat_thread, NULL) == 0)
			{
				close(sockfd);
				log_info("Successfully disconnected.");
			}
			else
			{
				log_error("Error joining heartbeat thread.");
				return -1;
			}
		}
		else
		{
			log_error("Error joining client thread.");
			return -1;
		}
	}
	else
	{
		log_info("Client not connected. Ignore.");
	}
	return 0;
}

bool socket_client_is_connected()
{
	bool is_connected;
	LOCK(&client_running_lock);
	is_connected = client_running;
	UNLOCK(&client_running_lock);
	return is_connected;
}

result_t socket_client_send_motor_param(uint8_t motor, int16_t param)
{
	if (motor > 1 || param > MAX_SPEED || param < -MAX_SPEED)
	{
		log_error("Incorrect motor parameter.");
		return -1;
	}

	LOCK(&send_lock);
	sending_param = true;
	motor_params[motor] = param;
	UNLOCK(&send_lock);
	pthread_cond_signal(&send_cond);
	return 0;
}

result_t socket_client_send_motor_params(uint8_t motor0, int16_t param0, uint8_t motor1, int16_t param1)
{
	if (motor0 > 1 || param0 > MAX_SPEED || param0 < -MAX_SPEED ||
		motor1 > 1 || param1 > MAX_SPEED || param1 < -MAX_SPEED )
	{
		log_error("Incorrect motor parameter.");
		return -1;
	}
	LOCK(&send_lock);
	sending_param = true;
	motor_params[motor0] = param0;
	motor_params[motor1] = param1;
	UNLOCK(&send_lock);
	pthread_cond_signal(&send_cond);
	return 0;
}

void	*_heartbeat_loop(void* param)
{
	int heartbeat_counter = 0;
	int n;

	while(1)
	{
		bool is_running;
		LOCK(&client_running_lock);
		is_running = client_running;
		UNLOCK(&client_running_lock);
		if (!is_running)
			break;
		
		motor_param_packet_header* heartbeat = allocate_packet(0);
		n = write(sockfd, heartbeat, heartbeat->len_bytes);
		if (n > 0)
		{
			log_debug("Sending heartbeat %d...\n", heartbeat_counter);
			heartbeat_counter++;
		}
		else
		if ( n < 0 )
		{
			log_error("Error writing heartbeat");
		}
		free_packet(heartbeat);

		sleep(1);
	}
	return NULL;
}

void	*_client_loop(void* param)
{
	int n;
	int packet_counter = 0;
	while (1)
	{
		bool is_running;
		LOCK(&client_running_lock);
		is_running = client_running;
		UNLOCK(&client_running_lock);
		if (!is_running)
			break;

		LOCK(&send_lock);
		while(!sending_param)
		{
			pthread_cond_wait(&send_cond, &send_lock);
		}

		// detected sending signal
		motor_param_packet_header* packet = allocate_packet(2);

		if (set_packet_param(packet, 0, MOTOR0, motor_params[MOTOR0]) < 0)
			log_error("Error setting packet param 0");
		if (set_packet_param(packet, 1, MOTOR1, motor_params[MOTOR1]) < 0)
			log_error("Error setting packet param 1");

		n = write(sockfd, packet, packet->len_bytes);
		if (n > 0)
		{
			log_debug("#%d packet", packet_counter);
			packet_counter++;
		}
		else
		if ( n < 0 )
		{
			log_error("Error writing packet");
		}
		free_packet(packet);
		// reset sending_param
		sending_param = false;
		UNLOCK(&send_lock);

	}

	return NULL;
}
;
