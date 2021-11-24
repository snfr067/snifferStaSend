#include <string.h>
#include <unistd.h>
#include <stdlib.h>
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
#include "tcpip_adapter.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "driver/uart.h"
#include <arpa/inet.h>
#include <sys/socket.h>
#include "rom/uart.h"
#include <esp_sleep.h>

#include "apsta_mode.h"
#include "sniffer.h"
#include "eth_main.h"
#include "get_settings.h"
#include "sock_client_init.h"

#define TRUE 1
#define FALSE 0

#define AP_WIFI_SSID      CONFIG_ESP_AP_WIFI_SSID
#define AP_WIFI_PASS      CONFIG_ESP_AP_WIFI_PASSWORD
#define STA_WIFI_SSID      CONFIG_ESP_STA_WIFI_SSID
#define STA_WIFI_PASS      CONFIG_ESP_STA_WIFI_PASSWORD

static const char *TAG = "apsta_mode.c";

static esp_err_t event_handler_apsta(void *ctx, system_event_t *event);
static esp_err_t event_handler_ap(void *ctx, system_event_t *event);
static esp_err_t event_handler_sta(void *ctx, system_event_t *event);
void ping();
esp_err_t pingResults(ping_target_id_t msgType, esp_ping_found * pf);
void sendUDP(char *string);

int isConnectAP = 0;
int gotStaConn = 0;
static int s_retry_num = 0;
in_addr_t ip_Addr;
ip4_addr_t sta_ip;
ip4_addr_t ip;
ip4_addr_t gw;
ip4_addr_t msk;
int recv_count,send_count;
char str[24] = "ABCDEFGHIJKLMNOPQRSTUVW";

void wifi_init_softap()
{
    ESP_LOGI(TAG, "Start ap mode");

    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler_ap, NULL));		//if sniffer mode start first, use loopinit
    //esp_event_loop_set_cb(event_handler_ap, NULL);					//if ap and sta mode start first, and then stop it, and then start ap or sta mode again, use loop set cb
	
	
    tcpip_adapter_ip_info_t ip_info = 
	{        
        .ip.addr = ipaddr_addr("192.168.40.1"),        
        .netmask.addr = ipaddr_addr("255.255.255.0"),        
        .gw.addr      = ipaddr_addr("192.168.40.1"),    
    };    
    ESP_ERROR_CHECK(tcpip_adapter_dhcps_stop(TCPIP_ADAPTER_IF_AP));    
    ESP_ERROR_CHECK(tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_AP, &ip_info));    
    ESP_ERROR_CHECK(tcpip_adapter_dhcps_start(TCPIP_ADAPTER_IF_AP));

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
        wifi_config_t wifi_config = {
        .ap = {
            .ssid = AP_WIFI_SSID,
            .ssid_len = strlen(AP_WIFI_SSID),
            .password = AP_WIFI_PASS,
            .max_connection = 10,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
            .channel=1,
			.beacon_interval=100
        },
    }; 
    if (strlen(AP_WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
	
    ESP_ERROR_CHECK(esp_wifi_set_bandwidth(WIFI_MODE_STA,WIFI_BW_HT20));

    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
    
    ESP_ERROR_CHECK(esp_wifi_start());
 
    int8_t max_power = 82;
    esp_err_t ret;
    ret = esp_wifi_set_max_tx_power(max_power);
    ESP_ERROR_CHECK(ret);
    ret = esp_wifi_get_max_tx_power(&max_power);
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "MAX TxPower = %d", max_power);
   
    ESP_LOGI(TAG, "wifi_init_softap finished.SSID:%s password:%s",
             AP_WIFI_SSID, AP_WIFI_PASS);
}




static esp_err_t event_handler_ap(void *ctx, system_event_t *event)
{
    switch(event->event_id) {
    case SYSTEM_EVENT_AP_STACONNECTED:
        ESP_LOGI(TAG, "station:"MACSTR" join, AID=%d",
                 MAC2STR(event->event_info.sta_connected.mac),
                 event->event_info.sta_connected.aid);
        break;
    case SYSTEM_EVENT_AP_STADISCONNECTED:
        ESP_LOGI(TAG, "station:"MACSTR"leave, AID=%d",
                 MAC2STR(event->event_info.sta_disconnected.mac),
                 event->event_info.sta_disconnected.aid);
        break;
    default:
        break;
    }
    return ESP_OK;
}

void wifi_init_softapsta()
{
    int8_t max_power = 82;
    
    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler_apsta, NULL));	

    tcpip_adapter_ip_info_t ip_info = 
	{        
        .ip.addr = ipaddr_addr("192.168.40.1"),        
        .netmask.addr = ipaddr_addr("255.255.255.0"),        
        .gw.addr      = ipaddr_addr("192.168.40.1"),    
    };    
    ESP_ERROR_CHECK(tcpip_adapter_dhcps_stop(TCPIP_ADAPTER_IF_AP));    
    ESP_ERROR_CHECK(tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_AP, &ip_info));    
    ESP_ERROR_CHECK(tcpip_adapter_dhcps_start(TCPIP_ADAPTER_IF_AP));
   
	/*
    tcpip_adapter_ip_info_t ip_info;
	IP4_ADDR(&ip_info.ip, 192,168,2,1);
	IP4_ADDR(&ip_info.gw, 192,168,2,1);
    IP4_ADDR(&ip_info.netmask, 255, 255, 255, 0);
	int ret = tcpip_adapter_dhcpc_stop(TCPIP_ADAPTER_IF_ETH);
    ESP_LOGI(TAG, "dhcp client stop RESULT: %d", ret);
    tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_ETH, &ip_info);
    ESP_LOGI(TAG, "Sniffer-AP IP:"IPSTR, IP2STR(&ip_info.ip));
    ESP_LOGI(TAG, "Sniffer-AP netmask:"IPSTR, IP2STR(&ip_info.netmask));
    ESP_LOGI(TAG, "Sniffer-AP getway:"IPSTR, IP2STR(&ip_info.gw));*/


    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	
    
    wifi_config_t wifi_config_ap = 
    {
        .ap = 
        {
            .ssid = AP_WIFI_SSID,
            .ssid_len = strlen(AP_WIFI_SSID),
            .password = AP_WIFI_PASS,
            .max_connection = 10,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
            .channel=1,
        }
    }; 
    
    wifi_config_t wifi_config_sta = 
    {
        .sta = 
        {
            .ssid = STA_WIFI_SSID,
            .password = STA_WIFI_PASS,
			.bssid_set = 0,
            .channel = 1,            
            .scan_method = WIFI_ALL_CHANNEL_SCAN,
            .sort_method = WIFI_CONNECT_AP_BY_SIGNAL
        }
    }; 
	
    
        
    if (strlen(AP_WIFI_PASS) == 0) 
    {
        wifi_config_ap.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));      
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config_sta));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config_ap));  
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));    
    ESP_ERROR_CHECK(esp_wifi_start());
	
    ESP_LOGI(TAG, "wifi_init_softap finished.SSID:%s password:%s",
             AP_WIFI_SSID, AP_WIFI_PASS);
    ESP_LOGI(TAG, "connect to ap SSID:%s password:%s",
            STA_WIFI_SSID, STA_WIFI_PASS);
             
             
    
    ESP_ERROR_CHECK(esp_wifi_set_max_tx_power(max_power));
    ESP_ERROR_CHECK(esp_wifi_get_max_tx_power(&max_power));
    ESP_LOGI(TAG, "The max TxPower now is %d", max_power);    
}


static esp_err_t event_handler_apsta(void *ctx, system_event_t *event)
{
    switch(event->event_id) 
    {
        //AP
        case SYSTEM_EVENT_AP_STACONNECTED:
            ESP_LOGI(TAG, "station:"MACSTR" join, AID=%d",
                     MAC2STR(event->event_info.sta_connected.mac),
                     event->event_info.sta_connected.aid);
            
            gotStaConn++;
            ESP_LOGI(TAG, "station count:%d", gotStaConn);
                     
            break;
        case SYSTEM_EVENT_AP_STADISCONNECTED:
            ESP_LOGI(TAG, "station:"MACSTR"leave, AID=%d",
                     MAC2STR(event->event_info.sta_disconnected.mac),
                     event->event_info.sta_disconnected.aid);
                     
            gotStaConn--;
            break;
            
        //STA    
        case SYSTEM_EVENT_STA_START:
            esp_wifi_connect();
            
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            ESP_LOGI(TAG, "SYSTEM_EVENT_STA_GOT_IP");
            ip = event->event_info.got_ip.ip_info.ip;
            gw = event->event_info.got_ip.ip_info.gw;
            msk = event->event_info.got_ip.ip_info.netmask;
            ESP_LOGI(TAG, "got ip:%s", ip4addr_ntoa(&ip));
            ESP_LOGI(TAG, "gw ip:%s", ip4addr_ntoa(&gw));
            isConnectAP = TRUE;
            s_retry_num = 0;
			ping();
            
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            ESP_LOGI(TAG, "SYSTEM_EVENT_STA_DISCONNECTED");
            
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG,"retry to connect to the AP, s_retry_num = %d", s_retry_num);
            
            break;            
        default:
            break;
    }
    return ESP_OK;
}


void wifi_init_sta()
{
    s_wifi_event_group = xEventGroupCreate();

    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL) );

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));


    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS,
            .channel = 1
        },
    };

   


    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
    ESP_ERROR_CHECK(esp_wifi_start());



}


static esp_err_t event_handler_sta(void *ctx, system_event_t *event)
{
    switch(event->event_id) 
	{
		case SYSTEM_EVENT_STA_START:
			esp_wifi_connect();
			break;
        case SYSTEM_EVENT_STA_GOT_IP:
            ESP_LOGI(TAG, "SYSTEM_EVENT_STA_GOT_IP");
            ip = event->event_info.got_ip.ip_info.ip;
            gw = event->event_info.got_ip.ip_info.gw;
            msk = event->event_info.got_ip.ip_info.netmask;
            ESP_LOGI(TAG, "got ip:%s", ip4addr_ntoa(&ip));
            ESP_LOGI(TAG, "gw ip:%s", ip4addr_ntoa(&gw));
			
			if(!isConnectAP)
			{
				get_settings();
				init_socket_client();
			}
			
            isConnectAP = TRUE;
            s_retry_num = 0;
			//ping();
            //xTaskCreate(&sendUDP, "sendUDP", 4096, str, 2, NULL);
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            ESP_LOGI(TAG, "SYSTEM_EVENT_STA_DISCONNECTED");
            
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG,"retry to connect to the AP, s_retry_num = %d", s_retry_num);
		default:
			break;
    }
    return ESP_OK;
}


void ping()
{
    uint32_t ping_count = 100000;  //how many pings per report
    uint32_t ping_timeout = 1000; //mS till we consider it timed out
    uint32_t ping_delay = 10; //mS between pings
	
	printf("Ping Start\n");

    while( 1 ) 
	{
		if( isConnectAP )
		{ 
			esp_ping_set_target(PING_TARGET_IP_ADDRESS_COUNT, &ping_count, sizeof(uint32_t));
			esp_ping_set_target(PING_TARGET_RCV_TIMEO, &ping_timeout, sizeof(uint32_t));
			esp_ping_set_target(PING_TARGET_DELAY_TIME, &ping_delay, sizeof(uint32_t));
			esp_ping_set_target(PING_TARGET_IP_ADDRESS, &gw.addr, sizeof(uint32_t));
			esp_ping_set_target(PING_TARGET_RES_FN, &pingResults, sizeof(pingResults));
			ping_init();
		}
		vTaskDelay(ping_delay / portTICK_PERIOD_MS);
    }
}

esp_err_t pingResults(ping_target_id_t msgType, esp_ping_found * pf)
{

	send_count = pf->send_count;
	recv_count = pf->recv_count;


	//printf("AvgTime:%.1fmS Sent:%d Rec:%d Lost:%d Err:%d min(mS):%d max(mS):%d ", (float)pf->total_time/pf->recv_count, pf->send_count, pf->recv_count, (pf->send_count)-(pf->recv_count), pf->err_count, pf->min_time, pf->max_time );
	//printf("Resp(mS):%d Timeouts:%d Total Time:%d\n",pf->resp_time, pf->timeout_count, pf->total_time);

	return ESP_OK;
}

void sendUDP(char *string)
{
	printf("send %s, ", string);
	while(1)
	{
		sendStr(string);
		//vTaskDelay(5 / portTICK_PERIOD_MS);
	}
}
