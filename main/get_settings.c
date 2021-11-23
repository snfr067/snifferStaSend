#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <sys/time.h>

#include "ping/ping.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/apps/sntp.h"
#include "driver/uart.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include <arpa/inet.h>
#include <sys/socket.h>

#include "lwip/err.h"
#include "lwip/sys.h"
#include "driver/uart.h"
#include <arpa/inet.h>
#include <sys/socket.h>
#include "rom/uart.h"
#include <esp_sleep.h>

#include "ota.h"
#include "eth_main.h"
#include "sock_client_init.h"
#include "sock_server_init.h"
#include "sniffer.h"
#include "get_settings.h"
#include "nvs_file_IO.h"

#define TRUE 1
#define FALSE 0
#define DB_PATTERN_COUNT 7
#define MAX_GET_FAILED CONFIG_SNIFFER_SETTING_GET_MAX_FAILED_LIMIT

#define DB_INDEX_CHANNEL 0
#define DB_INDEX_DEST_IP 2
#define DB_INDEX_CK_PORT 3
#define DB_INDEX_RSSI_PORT 4
#define DB_INDEX_FREQ 6

static const char *TAG = "get settings";

uint8_t mac_des[6];
int getCounter = 0, update_status = FALSE;

// global var
int g_channel = CONFIG_SNIFFER_CHANNEL;
char g_dest_ip[30] = CONFIG_SOCKET_DEST_IP;
int g_ck_port = CONFIG_SOCKET_DEST_PORT_CK;
int g_rssi_port = CONFIG_SOCKET_DEST_PORT_RSSI;
int g_report_frequency = CONFIG_REPORT_FREQUENCY;

void timer_get_settings(void *pvParameter);
int check_rom_settings();
void use_rom_settings();
void use_sdk_settings();

void get_settings()
{
    char msg[30] = "";
    esp_read_mac(mac_des, 1);
    sprintf(msg, "%02x:%02x:%02x:%02x:%02x:%02x", mac_des[0], mac_des[1], mac_des[2], mac_des[3], mac_des[4], mac_des[5]);
    ESP_LOGI(TAG, "msg = %s", msg);
    
	ESP_LOGI(TAG, "Use Sdk Settings");
	use_sdk_settings();
    
    /*while(1)
    {
        init_socket_client_get();    
        send_get_message(msg, sizeof(msg));
        ESP_LOGI(TAG, "Message Send : %s", msg);
        close_get_client_socket();    
        
        vTaskDelay(3 * 1000 / portTICK_PERIOD_MS);
        
        getCounter++;
        
        if(update_status == TRUE)
        {
            stop_socket_server();
            ESP_LOGI(TAG, "Use DB Settings");
            break;
        }
        else if(getCounter >= MAX_GET_FAILED)
        {
            stop_socket_server();
            if(check_rom_settings() == TRUE)
            {
                ESP_LOGI(TAG, "Use Rom Settings");
                use_rom_settings();
            }
            else
            {
                ESP_LOGI(TAG, "Use Sdk Settings");
                use_sdk_settings();
            }
            break;
        }
    }*/
    
    //wifi_init_sniffer();
    //ckSniffAlive();
}

void set_update_status(int status)
{
    update_status = status;
}

void stop_socket_server()
{
    ESP_LOGI(TAG, "Remove config updater");
    setDBSettingRecv(FALSE);
}

int check_db_settings(char *db_settings_string)
{
    char db_strtok[DB_PATTERN_COUNT][30];
    
    for(int i = 0; i < DB_PATTERN_COUNT; i++)
    {
        getStrtokString(db_settings_string, i, db_strtok[i]);
    }
    
    if(!(atoi(db_strtok[0]) > 0 && atoi(db_strtok[0]) < 15))
    {
        ESP_LOGI(TAG, "Wrong Channel = %d", atoi(db_strtok[0]));
        return FALSE;
    }
    else if(!(atoi(db_strtok[3]) > 0 && atoi(db_strtok[3]) < 65535))
    {
        ESP_LOGI(TAG, "Wrong Ck Port = %d", atoi(db_strtok[3]));
        return FALSE;
    }
    else if(!(atoi(db_strtok[4]) > 0 && atoi(db_strtok[4]) < 65535))
    {
        ESP_LOGI(TAG, "Wrong RSSI Port = %d", atoi(db_strtok[4]));
        return FALSE;
    }
    else if(!(atoi(db_strtok[5]) > 0 && atoi(db_strtok[5]) < 65535))
    {
        ESP_LOGI(TAG, "Wrong CSI Port = %d", atoi(db_strtok[5]));
        return FALSE;
    }
    else
        return TRUE;
}

void use_db_settings(char *db_settings_string)
{
    char db_strtok[DB_PATTERN_COUNT][30];
    
    for(int i = 0; i < DB_PATTERN_COUNT; i++)
    {
        getStrtokString(db_settings_string, i, db_strtok[i]);
    }
    
    g_channel = atoi(db_strtok[DB_INDEX_CHANNEL]);
    sprintf(g_dest_ip, "%s", db_strtok[DB_INDEX_DEST_IP]);
    g_ck_port = atoi(db_strtok[DB_INDEX_CK_PORT]);
    g_rssi_port = atoi(db_strtok[DB_INDEX_RSSI_PORT]);
    g_report_frequency = atoi(db_strtok[DB_INDEX_FREQ]);
    
    
    ESP_LOGI(TAG, "Channel = %d", g_channel);
    ESP_LOGI(TAG, "Dest IP = %s", g_dest_ip);
    ESP_LOGI(TAG, "Check message port = %d", g_ck_port);
    ESP_LOGI(TAG, "RSSI port = %d", g_rssi_port);
    ESP_LOGI(TAG, "Report Frequency = %d", g_report_frequency);
    
    nvs_write_string_to_flash("CONFIG_FILE", "WRITE", "Y");
    nvs_write_int32_to_flash("CONFIG_FILE", "CHANNEL", g_channel);
    nvs_write_string_to_flash("CONFIG_FILE", "DEST_IP", g_dest_ip);
    nvs_write_int32_to_flash("CONFIG_FILE", "CK_PORT", g_ck_port);
    nvs_write_int32_to_flash("CONFIG_FILE", "RSSI_PORT", g_rssi_port);
    nvs_write_int32_to_flash("CONFIG_FILE", "REPORT_FREQ", g_report_frequency);
        
}

int check_rom_settings()
{
    char isWrite[2] = "N";
    nvs_read_string_from_flash("CONFIG_FILE", "WRITE", isWrite, sizeof(isWrite));
    
    
    if(strcmp(isWrite, "Y") == 0)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
    
}

void use_rom_settings()
{
    if(check_rom_settings() == TRUE)
    {
        g_channel = nvs_read_int32_from_flash("CONFIG_FILE", "CHANNEL");
        nvs_read_string_from_flash("CONFIG_FILE", "DEST_IP", g_dest_ip, sizeof(g_dest_ip));
        g_ck_port = nvs_read_int32_from_flash("CONFIG_FILE", "CK_PORT");
        g_rssi_port = nvs_read_int32_from_flash("CONFIG_FILE", "RSSI_PORT");
        g_report_frequency = nvs_read_int32_from_flash("CONFIG_FILE", "REPORT_FREQ");
        
            
        ESP_LOGI(TAG, "Channel = %d", g_channel);
        ESP_LOGI(TAG, "Dest IP = %s", g_dest_ip);
        ESP_LOGI(TAG, "Check message port = %d", g_ck_port);
        ESP_LOGI(TAG, "RSSI port = %d", g_rssi_port);
        ESP_LOGI(TAG, "Report Frequency = %d", g_report_frequency);
    }
}

void use_sdk_settings()
{
    ESP_LOGI(TAG, "Channel = %d", g_channel);
    ESP_LOGI(TAG, "Dest IP = %s", g_dest_ip);
    ESP_LOGI(TAG, "Check message port = %d", g_ck_port);
    ESP_LOGI(TAG, "RSSI port = %d", g_rssi_port);
    ESP_LOGI(TAG, "Report Frequency = %d", g_report_frequency);
    
    nvs_write_string_to_flash("CONFIG_FILE", "WRITE", "Y");
    nvs_write_int32_to_flash("CONFIG_FILE", "CHANNEL", g_channel);
    nvs_write_string_to_flash("CONFIG_FILE", "DEST_IP", g_dest_ip);
    nvs_write_int32_to_flash("CONFIG_FILE", "CK_PORT", g_ck_port);
    nvs_write_int32_to_flash("CONFIG_FILE", "RSSI_PORT", g_rssi_port);
    nvs_write_int32_to_flash("CONFIG_FILE", "REPORT_FREQ", g_report_frequency);
}

void getStrtokString(char src_str[], int getStrNum, char ret_str[])
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
    if(temp != NULL)
    {
        sprintf(ret_str, "%s", temp);
            
        memset(delim, 0x00, sizeof(delim));
        temp = NULL;
    }
    //else
    //    printf("Error getStrNum %d\n", getStrNum);
    
}

