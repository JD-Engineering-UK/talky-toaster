#include <esp_wifi.h>
#include <esp_log.h>
#include <cstring>
#include "networking.h"
#include "nvs_flash.h"

const static char *TAG = "WiFi Credentials";

ap_cred_t* Wifi::get_credentials(char *ssid) {
	// Search for the credentials for the network with the ssid provided
	char temp_str[50] = {0};

	for(int idx=0; idx < ap_cred_count; ++idx) {
		ap_cred_t *cred = ap_creds[idx];
		memset(temp_str, 0, 50);
		strncpy(temp_str, cred->ssid, sizeof(cred->ssid));

		if(strncmp(ssid, cred->ssid, sizeof(cred->ssid)) == 0) {
			ESP_LOGD(TAG, "Found network!");
			return cred;
		}
	}
	return nullptr;
}

void Wifi::add_ap(const char *ssid, const char *passwd) {
	// Implementation for adding an access point
	nvs_iterator_t it = NULL;
	nvs_handle_t nvs_handle;
	esp_err_t err = nvs_open("wifi", NVS_READWRITE, &nvs_handle);
	if(err != ESP_OK) {
		ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(err));
		return;
	}

	// Update existing AP credentials in NVS
	esp_err_t res = nvs_entry_find_in_handle(nvs_handle, NVS_TYPE_ANY, &it);
	ap_cred_t *ap_cred = (ap_cred_t *)malloc(sizeof(ap_cred_t));
	char key[64];
	bool credentials_found = false;
	uint8_t spare_index = 0;
    while(res == ESP_OK) {
        nvs_entry_info_t info;
        nvs_entry_info(it, &info);
		uint8_t index = 0;
		// Extract the wifi index from the ssid
		int n = 0;
		if (sscanf(info.key, "wifi_%hhu_ssid%n", &index, &n) == 1 && info.key[n] == '\0') {
			if(index == spare_index) {
				spare_index++;
			}
			size_t length = sizeof(ap_cred->ssid);
			nvs_get_str(nvs_handle, info.key, ap_cred->ssid, &length);
			length = sizeof(ap_cred->passwd);
			nvs_get_str(nvs_handle, info.key, ap_cred->passwd, &length);
			if(strcmp(ap_cred->ssid, ssid) == 0){
				credentials_found = true;
				ESP_LOGI(TAG, "AP already exists. Updating credentials.");
				nvs_set_str(nvs_handle, info.key, ssid);
				snprintf(key, sizeof(key), "wifi_%hhu_passwd", index);
				if(passwd != nullptr) {
					nvs_set_str(nvs_handle, key, passwd);
				}else if(strlen(ap_cred->passwd) > 0) {
					nvs_erase_key(nvs_handle, key);
				}
				break;
			}
		}
        res = nvs_entry_next(&it);
    }
	if(!credentials_found && spare_index < MAX_WIFI_CREDENTIALS) {
		// Add new AP credentials
		snprintf(key, sizeof(key), "wifi_%hhu_ssid", spare_index);
		nvs_set_str(nvs_handle, key, ssid);
		snprintf(key, sizeof(key), "wifi_%hhu_passwd", spare_index);
		if(passwd != nullptr) {
			nvs_set_str(nvs_handle, key, passwd);
		}
		else {
			nvs_erase_key(nvs_handle, key);
		}
		ESP_LOGI(TAG, "Added new AP credentials for %s", ssid);
	}
    nvs_release_iterator(it);
	nvs_commit(nvs_handle);
	nvs_close(nvs_handle);
	free(ap_cred);

	// Easier to re-enumerate credentials than to keep track of them
	enumerate_credentials();
}

void Wifi::delete_ap(const char *ssid) {
	// Implementation for deleting an access point
	nvs_iterator_t it = NULL;
	nvs_handle_t nvs_handle;
	esp_err_t err = nvs_open("wifi", NVS_READWRITE, &nvs_handle);
	if(err != ESP_OK) {
		ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(err));
		return;
	}

	// Update existing AP credentials in NVS
	esp_err_t res = nvs_entry_find_in_handle(nvs_handle, NVS_TYPE_ANY, &it);
	size_t i = 0;
	ap_cred_t *ap_cred = (ap_cred_t *)malloc(sizeof(ap_cred_t));
	char key[64];
    while(res == ESP_OK && i < MAX_WIFI_CREDENTIALS) {
        nvs_entry_info_t info;
        nvs_entry_info(it, &info);
		uint8_t index = 0;
		// Extract the wifi index from the ssid
		if(sscanf(info.key, "wifi_%hhu_ssid", &index) == 1) {
			size_t length = sizeof(ap_cred->ssid);
			nvs_get_str(nvs_handle, info.key, ap_cred->ssid, &length);
			length = sizeof(ap_cred->passwd);
			nvs_get_str(nvs_handle, info.key, ap_cred->passwd, &length);
			if(strcmp(ap_cred->ssid, ssid) == 0){
				ESP_LOGI(TAG, "AP found. Deleting credentials.");
				nvs_erase_key(nvs_handle, info.key);
				snprintf(key, sizeof(key), "wifi_%hhu_passwd", index);
				nvs_erase_key(nvs_handle, key);
				break;
			}
			i++;
		}
        res = nvs_entry_next(&it);
    }
	
    nvs_release_iterator(it);
	nvs_close(nvs_handle);
	free(ap_cred);
	// Easier to re-enumerate credentials than to keep track of them
	enumerate_credentials();
}

void Wifi::enumerate_credentials() {
	// Implementation for enumerating stored credentials
	nvs_iterator_t it = NULL;
	nvs_handle_t nvs_handle;
	esp_err_t err = nvs_open("wifi", NVS_READONLY, &nvs_handle);
	if(err != ESP_OK) {
		ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(err));
		return;
	}
	for(size_t i = 0; i < MAX_WIFI_CREDENTIALS; ++i) {
		if(ap_creds[i] != nullptr) {
			free(ap_creds[i]);
			ap_creds[i] = nullptr;
		}
	}
	ap_cred_count = 0;

	// Populate the ap_creds array with stored credentials
    esp_err_t res = nvs_entry_find_in_handle(nvs_handle, NVS_TYPE_STR, &it);
    while(res == ESP_OK && ap_cred_count < MAX_WIFI_CREDENTIALS) {
        nvs_entry_info_t info;
        nvs_entry_info(it, &info);
		uint8_t index = 0;
		char key[64];
		// Extract the wifi index from the ssid
		int n = 0;
		if (sscanf(info.key, "wifi_%hhu_ssid%n", &index, &n) == 1 && info.key[n] == '\0') {
			ap_creds[ap_cred_count] = (ap_cred_t *)malloc(sizeof(ap_cred_t));
			size_t length = sizeof(ap_creds[ap_cred_count]->ssid);
			nvs_get_str(nvs_handle, info.key, ap_creds[ap_cred_count]->ssid, &length);
			length = sizeof(ap_creds[ap_cred_count]->passwd);
			snprintf(key, sizeof(key), "wifi_%hhu_passwd", index);
			nvs_get_str(nvs_handle, key, ap_creds[ap_cred_count]->passwd, &length);
			ap_cred_count++;
		}
        res = nvs_entry_next(&it);
    }
    nvs_release_iterator(it);
	nvs_close(nvs_handle);
	ESP_LOGI(TAG, "Enumerated %u AP credentials", ap_cred_count);
}