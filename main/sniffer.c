#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include "pthread.h"
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

#include "esp_pthread.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "driver/uart.h"
#include <arpa/inet.h>
#include <sys/socket.h>
#include "rom/uart.h"
#include <esp_sleep.h>

#include "def_vars.h"
#include "eth_main.h"
#include "sock_client_init.h"
#include "sock_server_init.h"
#include "sniffer.h"
#include "apsta_mode.h"

#define TRUE 1
#define FALSE 0

#define SNIFFER_CHANNEL CONFIG_SNIFFER_CHANNEL


void wifi_sniffer_packet_handler(void* buff, wifi_promiscuous_pkt_type_t type);
inline static void socket_data_send(char *Buf);
inline static void socket_data_send_ck(char *Buf);
static void console();
snifPktQueue *createSnifPktNode(esp_snif_packet_t pkt);
void addSnifPktInQueue(snifPktQueue *node);
void delHeadSnifPktInQueue();
//void printDataAfterNode(snifPktQueue *start);
//void printAll();
void sendSnifPkt(void *pvParameter);

static void periodic_timer_callback(void* arg);

static const char *TAG = "sniffer.c";
static EventGroupHandle_t s_wifi_event_group;
static uint8_t mac_des[6];

extern int ser_sock;
extern struct sockaddr_in client;
extern struct sockaddr_in server;

extern int ser_sock_ck;
extern struct sockaddr_in client_ck;
extern struct sockaddr_in server_ck;

extern int ser_sock_info;
extern struct sockaddr_in client_info;
extern struct sockaddr_in server_info;

extern char got_ip;
extern int isConnectAP;

extern int g_rssi_port, g_ck_port, g_channel, g_report_frequency;

int timeOut = 0;
int fstPktTime = 0;
int successTimes = 0, failedTimes = 0;
int pktCount = 0;

pthread_t snif_pth_pkt;
esp_snif_packet_t send_pkt;
int isSend = FALSE;
uint8_t scanMac_phone[6] = {0xe4, 0x33, 0xae, 0x0c, 0x0d, 0x39};

snifPktQueue *snifQueHead = NULL, *snifQueCurrent = NULL;

void app_main()
{
    console();
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    wifi_init_sniffer();
    xTaskCreate(&init_socket_server, "init_socket_server", 4096, NULL, 2, NULL);
    xTaskCreate(&sendSnifPkt, "sendSnifPkt", 4096, NULL, 2, NULL);
}


inline static void socket_data_send(char *Buf)
{
    if( sendto(ser_sock, Buf, sizeof(esp_snif_packet_t), 0, (struct sockaddr*)&server, sizeof(server)) < 0)
    {
        //puts("Send failed");
        //exit(0);
    //    printf(", Send Failed\n");
        if(isConnectAP)
            failedTimes++;
    }
    else
    {
      //  printf(", Send Success\n");
        if(isConnectAP)
            successTimes++;
    }
}



inline static void socket_data_send_ck(char *Buf)
{
    if( sendto(ser_sock_ck, Buf, 6, 0, (struct sockaddr*)&server_ck, sizeof(server_ck)) < 0)
    {
        puts("Send failed");
        //exit(0);
    }
}


static void console()
{
    const uart_config_t uart_config = {
    .baud_rate = 115200,
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_EVEN,
    .stop_bits = UART_STOP_BITS_1,
    //.use_ref_tick = true,
    //.flow_ctrl = UART_HW_FLOWCTRL_CTS_RTS,
    //.rx_flow_ctrl_thresh = 122,            

    };

    ESP_ERROR_CHECK( uart_driver_delete(UART_NUM_0) );

    ESP_ERROR_CHECK( uart_param_config(UART_NUM_0, &uart_config) );
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM_0, 1, 3, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
   // Setup UART buffered IO with event queue
    const int uart_buffer_size = 168;
    QueueHandle_t uart_queue;
    // Install UART driver using an event queue here
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_0, uart_buffer_size,uart_buffer_size, 168, &uart_queue, 0));



}


void wifi_init_sniffer()
{
    bool snif_mode;
    wifi_ps_type_t wc_ps;
    esp_read_mac(mac_des, 1);

    printf("init_sniffer\n");
    s_wifi_event_group = xEventGroupCreate();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    //ESP_ERROR_CHECK(esp_event_loop_init(wifi_sniffer_packet_handler, NULL));
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_NULL) );
    ESP_ERROR_CHECK(esp_wifi_set_bandwidth(WIFI_MODE_NULL, WIFI_BW_HT40));
    ESP_ERROR_CHECK(esp_wifi_start());
    //ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(1));
    ESP_ERROR_CHECK(esp_wifi_get_ps(&wc_ps));
    ESP_ERROR_CHECK(esp_wifi_get_promiscuous(&snif_mode));
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(&wifi_sniffer_packet_handler));
    ESP_ERROR_CHECK(esp_wifi_set_channel(g_channel,0));

    wifi_init_sta();

  

}


void wifi_sniffer_packet_handler(void* buff, wifi_promiscuous_pkt_type_t type)
{
    uint8_t scanMac_snifapsta[6] = {0x4c, 0x11, 0xae, 0x6e, 0x38, 0x18};
    uint8_t scanMac_snifstaap[6] = {0x4c, 0x11, 0xae, 0x6e, 0x38, 0x19};
    uint8_t scanMac_ap[6] = {0xbc, 0xdd, 0xc2, 0xcf, 0x48, 0xb1};
    uint8_t scanMac_sta[6] = {0x98, 0xcd, 0xac, 0x85, 0x14, 0x94};
    

    const wifi_promiscuous_pkt_t *ppkt = (wifi_promiscuous_pkt_t *)buff;
    const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)ppkt->payload;
    const wifi_ieee80211_mac_hdr_t *hdr = &ipkt->hdr;
        
    //send_pkt.timestamp = ppkt->rx_ctrl.timestamp;
    send_pkt.timestamp = pktCount;
    send_pkt.channel = ppkt->rx_ctrl.channel;
    send_pkt.rssi = ppkt->rx_ctrl.rssi;
    memcpy(send_pkt.addr1, hdr->addr1,6);
    memcpy(send_pkt.addr2, hdr->addr2,6);
    memcpy(send_pkt.mac_des, mac_des,6);
    
    int boolSnifsta = (memcmp(scanMac_snifapsta, hdr->addr2, sizeof(mac_des)) == 0);
    int boolSnifap = (memcmp(scanMac_snifstaap, hdr->addr1, sizeof(mac_des)) == 0);
    int boolAp = (memcmp(scanMac_ap, hdr->addr1, sizeof(mac_des)) == 0 || memcmp(scanMac_ap, hdr->addr2, sizeof(mac_des)) == 0);
    int boolSta = (memcmp(scanMac_sta, hdr->addr1, sizeof(mac_des)) == 0 || memcmp(scanMac_sta, hdr->addr2, sizeof(mac_des)) == 0);
    int boolPhone = (memcmp(scanMac_phone, hdr->addr2, sizeof(mac_des)) == 0);
    
    
    /*if(fstPktTime == 0)
        fstPktTime = send_pkt.timestamp;
    
    if((send_pkt.timestamp - fstPktTime) < (120*1000000))
    {
        printf("%d, ", successTimes);
        printf("%d, ", failedTimes);
        printf("%d, ", ppkt->rx_ctrl.timestamp);
        printf("%d, ", ppkt->rx_ctrl.channel);
        printf("%d, ", ppkt->rx_ctrl.rssi);'
        printf("%02X:%02X, ", hdr->addr1[4], hdr->addr1[5]);
        printf("%02X:%02X, ", hdr->addr2[4], hdr->addr2[5]);
        printf("%02X:%02X\n", mac_des[4], mac_des[5]);

    }
    else if(timeOut == 0 && (send_pkt.timestamp - fstPktTime) >= (120*1000000))
    {
        timeOut++;
        printf("Times up\n");  
    }*/
	

	
	/*if(pktCount <= 10)
	{
			
		printf("%d, ", send_pkt.timestamp);
		printf("%d, ", send_pkt.channel);
		printf("%d, ", send_pkt.rssi);
		printf("%02X:%02X, ", send_pkt.addr1[4], send_pkt.addr1[5]);
		printf("%02X:%02X, ", send_pkt.addr2[4], send_pkt.addr2[5]);
		printf("%02X:%02X\n", send_pkt.mac_des[4], send_pkt.mac_des[5]);
	
		addSnifPktInQueue(createSnifPktNode(send_pkt));
		pktCount++;
	}
	else if(pktCount != 1000)
	{
		printAll();
		
		delHeadSnifPktInQueue();
		printAll();
		
		pktCount = 1000;
	}
    */
	
    pktCount++;
	printf("%d, ", successTimes);
	printf("%d, ", failedTimes);
	printf("%d, ", send_pkt.timestamp);
	printf("%d, ", send_pkt.channel);
	printf("%d, ", send_pkt.rssi);
	printf("%02X:%02X, ", send_pkt.addr1[4], send_pkt.addr1[5]);
	printf("%02X:%02X, ", send_pkt.addr2[4], send_pkt.addr2[5]);
	printf("%02X:%02X\n", send_pkt.mac_des[4], send_pkt.mac_des[5]);

	addSnifPktInQueue(createSnifPktNode(send_pkt));
		
    //socket_data_send((char *)&send_pkt);
    
    

    isSend = FALSE;
    
    
}
void ckSniffAlive( void )
{
    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &periodic_timer_callback,
        /* name is optional, but may help identify the timer when debugging */
        .name = "periodic"
    };

    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));

    /* Start the timers */
    ESP_LOGI(TAG, "freq:%dms", g_report_frequency);

    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, g_report_frequency*1000));

}

static void periodic_timer_callback(void* arg)
{    
    int64_t time_since_boot = esp_timer_get_time();
    ESP_LOGI(TAG, "Periodic timer called, time since boot: %lld us", time_since_boot);
    /*init_socket_client_ck();
    socket_data_send_ck((char *)mac_des);
    close(ser_sock_ck);*/
}

snifPktQueue *createSnifPktNode(esp_snif_packet_t pkt)
{
	snifPktQueue *node = (snifPktQueue *)malloc(sizeof(esp_snif_packet_t)+1);
	memcpy(&(node->pkt), &pkt, sizeof(esp_snif_packet_t));
	node->next = NULL;
    
    return node;
}

void addSnifPktInQueue(snifPktQueue *node)
{
	if(NULL == snifQueHead)	//first pkt in queue
	{
		snifQueHead = node;
		snifQueCurrent = snifQueHead;
	}
	else
	{
		snifQueCurrent->next = node;
		snifQueCurrent = snifQueCurrent->next;
		snifQueCurrent->pkt = node->pkt;
		snifQueCurrent->next = NULL;
	}
}

/*void printDataAfterNode(snifPktQueue *start) 
{
	int num = 1;
	if(NULL != start)
	{
		while(start->next != NULL) 
		{
			printf("(%d)", num);
			printf("%d, ", start->pkt.timestamp);
			printf("%d, ", start->pkt.channel);
			printf("%d, ", start->pkt.rssi);
			printf("%02X:%02X, ", start->pkt.addr1[4], start->pkt.addr1[5]);
			printf("%02X:%02X, ", start->pkt.addr2[4], start->pkt.addr2[5]);
			printf("%02X:%02X\n", start->pkt.mac_des[4], start->pkt.mac_des[5]);
			start = start->next;
			num++;
    } 
	}
}

void printAll() 
{
    if(snifQueHead == NULL) 
	{
        printf("No sniffer data!\n");
    } 
	else 
	{
        printDataAfterNode(snifQueHead);
    }
}*/

void delHeadSnifPktInQueue()
{
	if(NULL != snifQueHead)
	    snifQueHead = snifQueHead->next;
}

void sendSnifPkt(void *pvParameter)
{
	printf("sendSnifPkt\n");
	while(1)
	{
		if(NULL != snifQueHead)
		{
			socket_data_send((char *)&(snifQueHead->pkt));
			delHeadSnifPktInQueue();
		}
        vTaskDelay(1 / portTICK_PERIOD_MS);
	}
}