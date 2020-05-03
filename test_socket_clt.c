#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>
#include "common.h"

#define INSERT_RANDOM_BYTES

int main(int argc, char **argv)
{
	int sockfd;
	struct sockaddr_in srv_addr;
	struct hostent *server;
	const char* hostname = argc > 1 ? argv[1] : "127.0.0.1";

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
	{
		error_exit("Error creating socket", EXIT_FAILURE);
	}

	server = gethostbyname(hostname);
	if (!server)
	{
		char error_msg[256] = {0};
		sprintf(error_msg, "Error finding host: %s", hostname);
		error_exit(error_msg, EXIT_FAILURE);
	}
	memset(&srv_addr, 0, sizeof(struct sockaddr_in));
	srv_addr.sin_family = AF_INET;
	memcpy(&srv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
	srv_addr.sin_port = htons(PORT);
	if (connect(sockfd, (struct sockaddr *)&srv_addr, sizeof(struct sockaddr_in)) < 0)
	{
		error_exit("Error connecting to server", EXIT_FAILURE);
	}

	int n = write(sockfd, HANDSHAKE_CLIENT, sizeof(HANDSHAKE_CLIENT));
	if (n < 0)
		error_exit("Error writing handshake to socket", EXIT_FAILURE);

	uint8_t reply_buf[2];
	memset(reply_buf, 0, sizeof(reply_buf));
	n = read(sockfd, reply_buf, sizeof(reply_buf));
	if (n < 0)
		error_exit("Error reading handshake from socket", EXIT_FAILURE);
	if (memcmp(reply_buf, HANDSHAKE_SERVER, sizeof(HANDSHAKE_SERVER)) != 0)
	{
		error_exit("Handshake failed", EXIT_FAILURE);
	}

	printf("Successfully handshaked\n");
	int packet_counter = 0;
	int heartbeat_counter = 0;
	time_t last_heartbeat_time = time(NULL);
	while(1)
	{
		// if we need to send heartbeat?
		if (time(NULL) - last_heartbeat_time >= 1)
		{
			motor_param_packet_header* heartbeat = allocate_packet(0);
			n = write(sockfd, heartbeat, heartbeat->len_bytes);
			if (n > 0)
			{
				printf("Sending heartbeat %d...\n", heartbeat_counter);
				heartbeat_counter++;
			}
			else
			if ( n < 0 )
			{
				error_exit("Error writing heartbeat", EXIT_FAILURE);
			}
			free_packet(heartbeat);
			last_heartbeat_time = time(NULL);
		}
		else
		{
			int number = rand() % 70;
			if (number == 0)
				number = 1;
			motor_param_packet_header* packet = allocate_packet(number);
			printf("Total content: %d\n", number);
			for(int i = 0; i < number; ++i)
			{
				int motor = rand() % 2;
				int param = rand() % 960 - 480;
				if (set_packet_param(packet, i, motor, param) < 0)
					error_exit("Error setting packet param", EXIT_FAILURE);
			}
			motor_param_packet_content content;
			for(int i = 0; i < number; ++i)
			{
				if (extract_packet_param(packet, i, &content) == 0)
					printf("Sending %d packet(%d, %d)\n", i, content.motor, content.value);
			}
#ifdef INSERT_RANDOM_BYTES
			// insert random bytes
			if (rand() % 100 > 66)
			{
				uint8_t rand_buf[16];
				size_t rand_len = rand() % 16;
				if (rand_len == 0) rand_len = 1;

				for(int i = 0; i < rand_len; ++i)
				{
					rand_buf[i] = rand() % 0xFF;
				}
				write(sockfd, rand_buf, rand_len); 	
				/*
				fprintf(stderr, "Written rubbish %d bytes\n", rand_len);
				for(int i = 0; i < rand_len; ++i)
				{
					fprintf(stderr, "%.2X ", rand_buf[i]);
				}
				fprintf(stderr, "\n");
				*/
			}
#endif
			n = write(sockfd, packet, packet->len_bytes);
			if (n > 0)
			{
				//printf("Written %d/%d bytes\n", n, packet->len_bytes);
				printf("#%d packet\n", packet_counter);
				packet_counter++;
			}
			else
			if ( n < 0 )
			{
				error_exit("Error writing packet", EXIT_FAILURE);
			}
			free_packet(packet);
			//usleep(1000*20);
		}
	}
	close(sockfd);	
	return 0;
}
