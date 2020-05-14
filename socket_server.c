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
static bool handshake_accepted = false;
static bool heartbeat_checking = false;
static int heartbeat_counter = 0;
static int missed_heartbeat = 0;
static int sockfd;
static pthread_t server_thread;
static pthread_t heartbeat_thread;
static pthread_mutex_t server_init_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t server_running_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t handshake_accepted_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t heartbeat_checking_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t handshake_accepted_cond = PTHREAD_COND_INITIALIZER;
static pthread_cond_t heartbeat_checking_cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t heartbeat_lock = PTHREAD_MUTEX_INITIALIZER;


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

bool socket_server_is_handshake_accepted()
{
	bool result;
	LOCK(&handshake_accepted_lock);
	result = handshake_accepted;
	UNLOCK(&handshake_accepted_lock);
	return result;
}

void* _heartbeat_loop(void* ptr)
{
	do
	{
		pthread_mutex_lock(&server_running_lock);
		bool is_server_running = server_running;
		pthread_mutex_unlock(&server_running_lock);
		if (!is_server_running)
		{
			log_info("Signaled exit. Quit heartbeat loop.");
			break;
		}

		// wait for handshake cond var
		pthread_mutex_lock(&handshake_accepted_lock);
		log_info("Wait for handshake signal");
		while(!handshake_accepted)
		{
			pthread_cond_wait(&handshake_accepted_cond, &handshake_accepted_lock);
		}
		pthread_mutex_unlock(&handshake_accepted_lock);

		// reset heartbeat and missed heartbeat to 0
		pthread_mutex_lock(&heartbeat_lock);
		heartbeat_counter = 0;
		missed_heartbeat = 0;
		pthread_mutex_unlock(&heartbeat_lock);
		// reset heartbeat_checking to true
		pthread_mutex_lock(&heartbeat_checking_lock);
		heartbeat_checking = true;
		pthread_mutex_unlock(&heartbeat_checking_lock);

		const int check_interval = 1;
		log_info("Now start checking heartbeat every %d sec.", check_interval);

		int last_heartbeat_counter = 0;
		while(1)
		{
			pthread_mutex_lock(&server_running_lock);
			is_server_running = server_running;
			pthread_mutex_unlock(&server_running_lock);
			if (!is_server_running)
			{
				log_info("Heartbeat loop detected exit signal.");
				break;
			}
			sleep(check_interval);

			int curr_heartbeat_counter;
			int curr_missed_heartbeat;
			pthread_mutex_lock(&heartbeat_lock);
			curr_heartbeat_counter = heartbeat_counter;
			if (curr_heartbeat_counter == last_heartbeat_counter)
			{
				missed_heartbeat++; 	
			}
			else
			{
				missed_heartbeat = 0;
			}
			curr_missed_heartbeat = missed_heartbeat;
			pthread_mutex_unlock(&heartbeat_lock);

			last_heartbeat_counter = curr_heartbeat_counter;

			if (curr_missed_heartbeat >= MAX_MISSING_HEARTBEAT)
			{
				log_error("Reached max(%d) heartbeat miss.", MAX_MISSING_HEARTBEAT);
				break;
			}
		}

		pthread_mutex_lock(&heartbeat_checking_lock);
		heartbeat_checking = false;
		pthread_mutex_unlock(&heartbeat_checking_lock);
		pthread_cond_signal(&heartbeat_checking_cond);

		usleep(1500);
	} while(1);

	return NULL;
}

void* _server_loop(void* ptr)
{
    if (listen(sockfd, 5) < 0)
    {
		log_error("Error on listening");
		return NULL;
    }

    struct sockaddr_in clt_addr;
    socklen_t cltlen = sizeof(struct sockaddr_in);
	do 
	{
		pthread_mutex_lock(&server_running_lock);
		bool is_server_running = server_running;
		pthread_mutex_unlock(&server_running_lock);
		if (!is_server_running)
		{
			log_info("Signaled exit. Quit socket server loop.");
			break;
		}
		// reset handshake accepted 
		pthread_mutex_lock(&handshake_accepted_lock);
		handshake_accepted = false;
		pthread_mutex_unlock(&handshake_accepted_lock);

		log_info("Now accepting connections...");
    	memset(&clt_addr, 0, cltlen);
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
			close(accept_sockfd);
			accept_sockfd = -1;
			continue;
		}
		log_info("Received correct handshake from client");
		log_info("Now sending handshake back to client... ");
		n = write(accept_sockfd, HANDSHAKE_SERVER, sizeof(HANDSHAKE_SERVER));
		if (n < 0)
		{
			log_error("Error writing handshake to socket");
			close(accept_sockfd);
			accept_sockfd = -1;
			continue;
		}

		log_info("Handshake done.");

		// signal to allow heartbeat check thread to continue
		pthread_mutex_lock(&handshake_accepted_lock);
		handshake_accepted = true;
		pthread_mutex_unlock(&handshake_accepted_lock);
		pthread_cond_signal(&handshake_accepted_cond);

		motor_param_packet_header tryout_header;
		uint8_t *tryout_buf = (uint8_t *)&tryout_header;
		int tryout_buf_idx = 0;
		int packet_counter = 0;
		while(1)
		{
			pthread_mutex_lock(&server_running_lock);
			is_server_running = server_running;
			pthread_mutex_unlock(&server_running_lock);
			if (!is_server_running)
			{
				log_info("Socket server loop detected exit signal.");
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
					// read nothing, ignore 
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
							pthread_mutex_lock(&heartbeat_lock);
							printf("Heartbeat #%d\n", heartbeat_counter);
							heartbeat_counter++;
							pthread_mutex_unlock(&heartbeat_lock);
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
					// read nothing, ignore 
				}
				free_packet(packet);

				
			}
		

			pthread_mutex_lock(&heartbeat_lock);
			int curr_missed_heartbeat = missed_heartbeat;
			pthread_mutex_unlock(&heartbeat_lock);
			if (curr_missed_heartbeat >= MAX_MISSING_HEARTBEAT)
			{
				log_error("Too many missing heartbeat. Stop.");
				close(accept_sockfd);
				accept_sockfd = -1;
				tryout_buf_idx = 0;
				packet_counter = 0;

				// reset handshake accepted 
				pthread_mutex_lock(&handshake_accepted_lock);
				handshake_accepted = false;
				pthread_mutex_unlock(&handshake_accepted_lock);

				// wait until heartbeat checking ended
				log_info_nocr("Wait heartbeat checking thread to reset...");
				pthread_mutex_lock(&heartbeat_checking_lock);
				while(heartbeat_checking)
				{
					pthread_cond_wait(&heartbeat_checking_cond, &heartbeat_checking_lock);
				}
				pthread_mutex_unlock(&heartbeat_checking_lock);	
				log_info(" Done.");

				break;
			}
		}

		if (accept_sockfd >= 0)
		{
			close(accept_sockfd);
			accept_sockfd = -1;
		}


		usleep(1000); // a little breath
	} while(1);

    log_info("Finished server loop.");

	return NULL;
}

result_t socket_server_start()
{
	result_t result = 0;

	pthread_mutex_lock(&server_running_lock);
	server_running = true;
	pthread_mutex_unlock(&server_running_lock);

	// init heartbeat related counter before starting threads
	pthread_mutex_lock(&heartbeat_lock);
	heartbeat_counter = 0;
	missed_heartbeat = 0;
	pthread_mutex_unlock(&heartbeat_lock);

	if (pthread_create(&heartbeat_thread, NULL, _heartbeat_loop, NULL) != 0)
	{
		pthread_mutex_lock(&server_running_lock);
		server_running = false;
		pthread_mutex_unlock(&server_running_lock);
		result = -1;
	}
	else
	{
		if (pthread_create(&server_thread, NULL, _server_loop, NULL) != 0)
		{
			pthread_mutex_lock(&server_running_lock);
			server_running = false;
			pthread_mutex_unlock(&server_running_lock);
			result = -1;
		}	
	}

	return result;
}

result_t socket_server_wait_till_stop()
{
	result_t result = 0;
	pthread_mutex_lock(&server_running_lock);
	bool is_running = server_running;
	pthread_mutex_unlock(&server_running_lock);
	if (is_running)
	{
		if (pthread_join(heartbeat_thread, NULL) != 0)
		{
			log_error("Cannot join heartbeat thread.");
			result = -1;
		}
		else {
			if (pthread_join(server_thread, NULL) != 0)
			{
				log_error("Cannot join server thread.");
				result = -1;
			}
		}
	}

	pthread_mutex_lock(&server_init_lock);
    close(sockfd);
	pthread_mutex_unlock(&server_init_lock);

	log_info("Socket server successfully stopped.");

	return result;
}

result_t socket_server_stop()
{
	result_t result = 0;
	pthread_mutex_lock(&server_running_lock);
	bool is_running = server_running;
	pthread_mutex_unlock(&server_running_lock);
	if (is_running)
	{
		pthread_mutex_lock(&server_running_lock);
		server_running = false;
		pthread_mutex_unlock(&server_running_lock);
		if (pthread_join(heartbeat_thread, NULL) != 0)
		{
			log_error("Cannot join heartbeat thread.");
			result = -1;
		}
		else {
			if (pthread_join(server_thread, NULL) != 0)
			{
				log_error("Cannot join server thread.");
				result = -1;
			}
		}
	}

	pthread_mutex_lock(&server_init_lock);
    close(sockfd);
	pthread_mutex_unlock(&server_init_lock);

	log_info("Socket server successfully stopped.");
	return result;
}

result_t socket_server_force_stop()
{
	result_t result = 0;
	pthread_mutex_lock(&server_running_lock);
	bool is_running = server_running;
	pthread_mutex_unlock(&server_running_lock);
	if (is_running)
	{
		pthread_mutex_lock(&server_running_lock);
		server_running = false;
		pthread_mutex_unlock(&server_running_lock);
		if (pthread_cancel(heartbeat_thread) != 0)
		{
			log_error("Cannot terminate heartbeat thread.");
			result = -1;
		}
		else {
			if (pthread_cancel(server_thread) != 0)
			{
				log_error("Cannot terminate server thread.");
				result = -1;
			}
		}
	}

	pthread_mutex_lock(&server_init_lock);
    close(sockfd);
	pthread_mutex_unlock(&server_init_lock);

	log_info("Socket server successfully terminated.");
	return result;
}

