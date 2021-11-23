#include "nvs_flash.h"
#include "nvs.h"

void nvs_write_int32_to_flash(const char* nvs_fileName, const char* param, int32_t value);
void nvs_write_string_to_flash(const char* nvs_fileName, const char* param, const char* string);
int32_t nvs_read_int32_from_flash(const char* nvs_fileName, const char* param);
void nvs_read_string_from_flash(const char* nvs_fileName, const char* param, char* str_data, uint32_t str_length);
void nvs_write_data_to_flash(void);
void nvs_read_data_from_flash(void);