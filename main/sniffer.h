#define NORMAL_INFO 0x00

#define TO_DO_OTA 0x10
#define OTA_OVER 0x11
#define OTA_SAME_VER 0x12
#define OTA_FINISH 0x13

void wifi_init_sniffer();
void ckSniffAlive( void );
void setSnifferOnOff(int set);