void get_settings();
void set_update_status(int status);
void stop_socket_server();
void timer_get_settings(void *pvParameter);
int check_db_settings(char *db_settings_string);
void use_db_settings(char *db_settings_string);
void getStrtokString(char src_str[], int getStrNum, char ret_str[]);