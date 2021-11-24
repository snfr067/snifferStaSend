#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include "ping/ping.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_ping.h"
#include "esp_image_format.h"
#include "esp_ota_ops.h"
#include "esp_heap_caps.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "driver/uart.h"
#include <arpa/inet.h>
#include <sys/socket.h>
#include "rom/uart.h"
#include <esp_sleep.h>

#include "sock_client_init.h"
#include "sniffer.h"
#include "eth_main.h"

#define EXAMPLE_GET_HOST_IP        CONFIG_SNIFFER_SETTING_GET_HOST_IP
#define EXAMPLE_GET_HOST_PORT        CONFIG_SNIFFER_SETTING_GET_HOST_PORT
#define SOCK_PORT_INFO        8890


static const char *TAG = "wifi sniffer socket client";

extern char g_dest_ip[30];
extern int g_ck_port;
extern int g_rssi_port;

int ser_sock_get;
struct sockaddr_in client_get;
struct sockaddr_in server_get;

int ser_sock_info;
struct sockaddr_in client_info;
struct sockaddr_in server_info;

int ser_sock;
struct sockaddr_in client;
struct sockaddr_in server;

int ser_sock_ck;
struct sockaddr_in client_ck;
struct sockaddr_in server_ck;


void init_socket_client_get( void )
{
    char *ser_ip = EXAMPLE_GET_HOST_IP;
    ser_sock_get = socket(AF_INET , SOCK_DGRAM , 0);
    if (ser_sock_get == -1)
    {
        printf("Could not create socket");
    }
    
	client_get.sin_family = AF_INET;
    client_get.sin_addr.s_addr = htonl(INADDR_ANY);
    client_get.sin_port = htons(0);


    
	server_get.sin_family = AF_INET;
    server_get.sin_port = htons( EXAMPLE_GET_HOST_PORT );
   
   
    if(inet_pton(AF_INET, ser_ip , &server_get.sin_addr)<=0)
    {
        printf("\n inet_pton error occured\n");
        exit(1);
        return;
    }

    if ( bind(ser_sock_get, (struct sockaddr*)&client_get, sizeof(client_get) ) < 0 ) 
	{
        perror("bind failed!");
        exit(1);
    }
 
}

void send_get_message(char *msg, int size)
{
	if( sendto(ser_sock_get, msg, size, 0, (struct sockaddr*)&server_get, sizeof(server_get)) < 0)
    {
        puts("ser_sock_get Send failed");
    }
}

void close_get_client_socket()
{
	close(ser_sock_get);
}

void init_socket_client( void )
{
	int flags = 0;
	char ser_ip[30] = "";
	sprintf(ser_ip, "%s", g_dest_ip);
	
    ser_sock = socket(AF_INET , SOCK_DGRAM , 0);
    if (ser_sock == -1)
    {
        printf("Could not create socket");
    }
	
	flags = fcntl(ser_sock, F_GETFL);
	if(fcntl(ser_sock, F_SETFL, flags & (~O_NONBLOCK)) < 0 )
	{
		perror("fcntl(ser_sock, F_SETFL, flags & ~O_NONBLOCK) fail!");
	}

    client.sin_family = AF_INET;
    client.sin_addr.s_addr = htonl(INADDR_ANY);
    client.sin_port = htons(0);

    server.sin_family = AF_INET;
    server.sin_port = htons( g_rssi_port );
//	ESP_LOGI(TAG, "init socket rssi client server: [%s:%d]", ser_ip, g_rssi_port);

	if(inet_pton(AF_INET, ser_ip , &server.sin_addr)<=0)
	{
		printf("\n inet_pton error occured\n");
		exit(1);
		return;
	}

	if ( bind(ser_sock, (struct sockaddr*)&client, sizeof(client) ) <0 ) {
		perror("bind failed!");
		exit(1);
	}
 
}



void init_socket_client_ck( void )
{
	char ser_ip[30] = "";
	sprintf(ser_ip, "%s", g_dest_ip);
	
    ser_sock_ck = socket(AF_INET , SOCK_DGRAM , 0);
    if (ser_sock_ck == -1)
    {
        ESP_LOGE(TAG, "Could not create socket");
    }

    client_ck.sin_family = AF_INET;
    client_ck.sin_addr.s_addr = htonl(INADDR_ANY);
    client_ck.sin_port = htons(0);

    server_ck.sin_family = AF_INET;
    server_ck.sin_port = htons( g_ck_port );
	
	ESP_LOGI(TAG, "init socket ck msg client server: [%s:%d]", ser_ip, g_ck_port);


	
	if(inet_pton(AF_INET, ser_ip , &server_ck.sin_addr)<=0)
	{
		ESP_LOGE(TAG, "inet_pton error occured");
		exit(1);
		return;
	}

	if ( bind(ser_sock_ck, (struct sockaddr*)&client_ck, sizeof(client_ck) ) <0 ) 
	{
		perror("bind failed!");
		exit(1);
	}
 
}

void init_socket_client_info( char *info_ip )
{
    char *ser_ip = info_ip;
    ser_sock_info = socket(AF_INET , SOCK_DGRAM , 0);
    if (ser_sock_info == -1)
    {
        printf("Could not create socket");
    }

    client_info.sin_family = AF_INET;
    client_info.sin_addr.s_addr = htonl(INADDR_ANY);
    client_info.sin_port = htons(0);

    server_info.sin_family = AF_INET;
    server_info.sin_port = htons( SOCK_PORT_INFO );
        
    if(inet_pton(AF_INET, ser_ip , &server_info.sin_addr)<=0)
    {
        printf("\n inet_pton error occured\n");
        exit(1);
        return;
    }

    if ( bind(ser_sock_info, (struct sockaddr*)&client_info, sizeof(client_info) ) <0 ) 
	{
        perror("bind failed!");
        exit(1);
    }
 
}

void close_socket_client_info( void )
{
	close(ser_sock_info);
}


