#include <arpa/inet.h>
#include <sys/types.h>          
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <unistd.h>
#include "esp_wifi.h"
#include "esp_log.h"

#include "get_settings.h"
#include "ota.h"
#include "sniffer.h"
#include "sock_client_init.h"
#include "eth_main.h"

#define TRUE 1
#define FALSE 0
#define SOCKET_SERVER_PORT CONFIG_SNIFFER_SETTING_GET_LOCAL_PORT

static const char *TAG = "sock_server_init";

extern int ser_sock;
extern struct sockaddr_in client;
extern struct sockaddr_in server;

int dBSettingRecv = TRUE;

void getStrtokString(char src_str[], int getStrNum, char ret_str[]);
int isSetChannelCmd(char string[]);
int isHttpsCmd(char string[]);
int isGetInfoCmd(char string[]);
int isRebootCmd(char string[]);
void setDBSettingRecv(int set);

void init_socket_server(void *pvParameter)
{
    int sin_len;
    char mac[18];
    char message[512];
    int socket_des;
    struct sockaddr_in sin;
    printf("Waiting for data from sender\n");

    // Initialize socket address structure for Internet Protocols
    bzero(&sin, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sin.sin_port = htons(SOCKET_SERVER_PORT);
    sin_len = sizeof(sin);

    //Create a UDP socket and bind it to the port
    socket_des = socket(AF_INET, SOCK_DGRAM, 0);
    bind(socket_des, (struct sockaddr *)&sin, sizeof(sin));
   
    // Received data through the socket and process it.The processing in this program is really simple --printing
    while(1)
    {
        recvfrom(socket_des, message, 512, 0, (struct sockaddr *)&sin, (socklen_t *)&sin_len);
        printf("Message from PC : %s\n", message);
        
        
        /*if(TRUE == check_db_settings(message) && TRUE == dBSettingRecv)
        {
            set_update_status(TRUE);
            use_db_settings(message);
        }
        else
        {
            init_socket_client_info(inet_ntoa(sin.sin_addr));
            if(isHttpsCmd(message) == 1)
            {
                sleep(1);        
                    
                ota_main(message);
            }
            else if(isSetChannelCmd(message) == 1)
            {
                char ch_str[10] = "";
                int channel = 0;
                
                getStrtokString(message, 1, ch_str);
                channel = atoi(ch_str);
                
                if(channel <= 0 || channel > 15)
                {
                    printf("set channel error: %d\n", channel);
                    channel = 1;
                }
                
                ESP_ERROR_CHECK(esp_wifi_set_channel(channel,0));
            }
            else if(isGetInfoCmd(message) != 0)
            {                    
            }
            else if(isRebootCmd(message) != 0)
            {                    
                esp_restart();
            }
            else
                ESP_LOGW(TAG, "Received an unknown type message : [%s]\n", message);
            close_socket_client_info();
        }*/
    }
    close(socket_des);    
}

/*void getStrtokString(char src_str[], int getStrNum, char ret_str[])
{
    int index = 0;
    char cp_src_str[50];
    char delim[2] = ",";
    char *temp = NULL;
    
    sprintf(cp_src_str, "%s", src_str);
    temp = strtok(cp_src_str, delim);
    while(index != getStrNum)
    {
        temp = strtok(NULL, delim);        
        index++;
    }
    sprintf(ret_str, "%s", temp);
    printf("getStrtokString = %s\n", ret_str);
    
    memset(delim, 0x00, sizeof(delim));
    *temp = NULL;
}*/

int isSetChannelCmd(char string[])
{
    char ret_str[30];
    getStrtokString(string, 0, ret_str);
    
    if(strcmp(ret_str, "Channel") == 0)
    {
        memset(ret_str, 0x00, sizeof(ret_str));
        return 1;
    }
    else
    {
        memset(ret_str, 0x00, sizeof(ret_str));
        return 0;
    }
    
}


int isHttpsCmd(char string[])
{
    if(string[0] == 'h' && string[1] == 't' && string[2] == 't' && string[3] == 'p' && string[4] == 's')
        return 1;
    else
        return 0;
}

int isGetInfoCmd(char string[])
{
    if(string[0] == 'G' && string[1] == 'E' && string[2] == 'T')
        return 1;
    else
        return 0;
}

int isRebootCmd(char string[])
{
    if(string[0] == 'R' && string[1] == 'E' && string[2] == 'B' && string[3] == 'O' && string[4] == 'O' && string[5] == 'T')
        return 1;
    else
        return 0;
}

void setDBSettingRecv(int set)
{
    dBSettingRecv = set;
}
