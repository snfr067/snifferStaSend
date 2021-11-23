#define SCAN_SECS 5

typedef struct {
        unsigned frame_ctrl:16;
        unsigned duration_id:16;
        uint8_t addr1[6]; /* receiver address */
        uint8_t addr2[6]; /* sender address */
        uint8_t addr3[6]; /* filtering address */
        unsigned sequence_ctrl:16;
        uint8_t addr4[6]; /* optional */
} wifi_ieee80211_mac_hdr_t;

const char *wifi_sniffer_packet_type2str(wifi_promiscuous_pkt_type_t type)
{
        switch(type) {
        case WIFI_PKT_MGMT: return "MGMT";
        case WIFI_PKT_DATA: return "DATA";
        default:
        case WIFI_PKT_MISC: return "MISC";
        }
}


typedef struct {
        wifi_ieee80211_mac_hdr_t hdr;
        uint8_t payload[0]; /* network data ended with 4 bytes csum (CRC32) */
} wifi_ieee80211_packet_t;

typedef struct {

        unsigned timestamp:32;
        unsigned channel:8;
        signed rssi:8;
        uint8_t addr1[6]; /* receiver address */
        uint8_t addr2[6]; /* sender address */
        uint8_t mac_des[6];

} esp_snif_packet_t;

unsigned char *mac_hdr = NULL;
        
void wcyu_scan_channel(void );
