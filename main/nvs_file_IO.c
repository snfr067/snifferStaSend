#include "nvs_flash.h"
#include "nvs.h"


void nvs_write_int32_to_flash(const char* nvs_fileName, const char* param, int32_t value)
{
    nvs_handle handle;

    ESP_ERROR_CHECK( nvs_open( nvs_fileName, NVS_READWRITE, &handle) );
    ESP_ERROR_CHECK( nvs_set_i32( handle, param, value) );

    ESP_ERROR_CHECK( nvs_commit(handle) );
    nvs_close(handle);
}

void nvs_write_string_to_flash(const char* nvs_fileName, const char* param, const char* string)
{
    nvs_handle handle;

    ESP_ERROR_CHECK( nvs_open( nvs_fileName, NVS_READWRITE, &handle) );
    ESP_ERROR_CHECK( nvs_set_str( handle, param, string) );

    ESP_ERROR_CHECK( nvs_commit(handle) );
    nvs_close(handle);
}

int32_t nvs_read_int32_from_flash(const char* nvs_fileName, const char* param)
{
    nvs_handle handle;
    int32_t value = NULL;

    ESP_ERROR_CHECK( nvs_open(nvs_fileName, NVS_READWRITE, &handle) );

    nvs_get_i32(handle, param, &value);
	
    nvs_close(handle);
	
	return value;
}

void nvs_read_string_from_flash(const char* nvs_fileName, const char* param, char* str_data, uint32_t str_length)
{
    nvs_handle handle;

    ESP_ERROR_CHECK( nvs_open( nvs_fileName, NVS_READWRITE, &handle) );
	
    nvs_get_str(handle, param, str_data, &str_length);

    nvs_close(handle);
}

void nvs_write_data_to_flash(void)
{
    nvs_handle handle;
    static const char *NVS_CUSTOMER = "customer data";
    static const char *DATA1 = "param 1";
    static const char *DATA2 = "param 2";
    //static const char *DATA3 = "param 3";

    int32_t value_for_store = 666;

    /*wifi_config_t wifi_config_to_store = {
        .sta = {
            .ssid = "store_ssid:hello_kitty",
            .password = "store_password:1234567890",
        },
    };*/

    //printf("set size:%u\r\n", sizeof(wifi_config_to_store));
    ESP_ERROR_CHECK( nvs_open( NVS_CUSTOMER, NVS_READWRITE, &handle) );
    ESP_ERROR_CHECK( nvs_set_str( handle, DATA1, "i am a string.") );
    ESP_ERROR_CHECK( nvs_set_i32( handle, DATA2, value_for_store) );
    //ESP_ERROR_CHECK( nvs_set_blob( handle, DATA3, &wifi_config_to_store, sizeof(wifi_config_to_store)) );

    ESP_ERROR_CHECK( nvs_commit(handle) );
    nvs_close(handle);

}

void nvs_read_data_from_flash(void)
{
    nvs_handle handle;
    static const char *NVS_CUSTOMER = "customer data";
    static const char *DATA1 = "param 1";
    static const char *DATA2 = "param 2";
    //static const char *DATA3 = "param 3";

    uint32_t str_length = 32;
    char str_data[32] = {0};
    int32_t value = 0;
    /*wifi_config_t wifi_config_stored;
    memset(&wifi_config_stored, 0x0, sizeof(wifi_config_stored));
    uint32_t len = sizeof(wifi_config_stored);*/

    ESP_ERROR_CHECK( nvs_open(NVS_CUSTOMER, NVS_READWRITE, &handle) );

    ESP_ERROR_CHECK ( nvs_get_str(handle, DATA1, str_data, &str_length) );
    ESP_ERROR_CHECK ( nvs_get_i32(handle, DATA2, &value) );
    //ESP_ERROR_CHECK ( nvs_get_blob(handle, DATA3, &wifi_config_stored, &len) );

    printf("[data1]: %s len:%u\r\n", str_data, str_length);
    printf("[data2]: %d\r\n", value);
    //printf("[data3]: ssid:%s passwd:%s\r\n", wifi_config_stored.sta.ssid, wifi_config_stored.sta.password);

    nvs_close(handle);
}