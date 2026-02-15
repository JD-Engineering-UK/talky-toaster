#pragma once

#include <esp_err.h>
#include <esp_event.h>


#define MAX_WIFI_CREDENTIALS 64


typedef struct {
	char ssid[32];
	char passwd[64];
} ap_cred_t;

void initialise_ntp();
void wait_for_updated_time();

/*
 * Singleton class for managing WiFi connectivity
 * Provides methods to start and stop WiFi, add credentials, and handle events.
 */
class Wifi{
public:
	enum state_e {
		NOT_INITIALISED,
		STOPPED,
		DISCONNECTED,
		ACTIVE_SCANNING,
		CONNECTING,
		CONNECTED,
		ERROR
	};
private:
	static state_e state;
	ap_cred_t *ap_creds[MAX_WIFI_CREDENTIALS] = {0};
	size_t ap_cred_count = 0;
public:
    Wifi(const Wifi&) = delete; // Disable copy constructor
    Wifi& operator=(const Wifi&) = delete; // Disable assignment operator
    static Wifi* getInstance() {
        static Wifi *instance = new Wifi(); // Guaranteed to be destroyed
        return instance; // Instantiated on first use
    }

	esp_err_t init();
	esp_err_t start();
	esp_err_t stop();
	void add_ap(const char *ssid, const char *passwd);
	void delete_ap(const char *ssid);
	bool isConnected(void);
private:
	Wifi(){}; // Private constructor to prevent instantiation
	void enumerate_credentials();
	ap_cred_t* get_credentials(char *ssid);
	static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
	static void ip_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
};
