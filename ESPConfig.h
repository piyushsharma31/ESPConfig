#ifndef ESPConfig_h
#define ESPConfig_h

#include "Arduino.h"
#include <ESP8266WiFi.h>

#define IS_DEBUG
#ifdef IS_DEBUG
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#define DEBUG_PRINTXY(x,y) Serial.print(x, y)
#define DEBUG_PRINT_ARRAY(x,y,z) printArray(x, y, z)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#define DEBUG_PRINTXY(x,y)
#define DEBUG_PRINT_ARRAY(x,y,z)
#endif

#define eeprom_update_interval 4000

#define IS_CONFIGURED_BYTE_ADDRESS  0

static const uint8_t MAX_LENGTH_SSID = 24;
static const uint8_t MAX_LENGTH_NAME = 16;

static const uint8_t DEVICE_COMMAND_NONE = 0;// empty
static const uint8_t DEVICE_COMMAND_DISCOVER = 1;// discover devices in LAN
static const uint8_t DEVICE_COMMAND_SET_CONFIGURATION = 5;// set all configuration items of device (2-4 is reserved)
static const uint8_t DEVICE_COMMAND_SET_CONFIGURATION_NAME = 6;// set Name, password to connect device to WiFi router
static const uint8_t DEVICE_COMMAND_SET_CONFIGURATION_SSID = 7;// set SSID
static const uint8_t DEVICE_COMMAND_SET_CONFIGURATION_AP = 8;// set SSID
static const uint8_t DEVICE_COMMAND_SET_CONFIGURATION_LOCATION = 9;// set SSID, password to connect device to WiFi router
static const uint8_t DEVICE_COMMAND_GET_CONTROLLER = 15;// get 1 capability settings (10-14 is reserved)
static const uint8_t DEVICE_COMMAND_SET_CONTROLLER = 16;// set 1 capability
static const uint8_t DEVICE_COMMAND_GETALL_CONTROLLER = 17;// get all capabilities of a controller
static const uint8_t DEVICE_COMMAND_SETALL_CONTROLLER = 18;// set all capabilities of a controller
static const uint8_t DEVICE_COMMAND_FIRMWARE_UPDATE = 19;// update ESP8266 firmware version

// maximum retry duration in milliseconds
static const unsigned int max_retry_wifi_ap_connect_time = 10000;

// UDP port for listening to App requests
static const unsigned int port = 2390;
static unsigned long capabilitiesLastSaved = 0;//last saved time in milliseconds
static uint8_t pinLastSaved = 10;//last saved pin, same pin capabilities cannot be saved in succession
static const uint16_t CAPABILITY_SAVE_INTERVAL = 1000;//interval between 2 successive save in milliseconds
static const char CONTROLLER_UNIQUE_SSID[] = "RCSLEDS";//Controller SSID prefix is "RCSLEDS"
static const char CONTROLLER_UNIQUE_SSID_KEY[] = "";//Controller SSID key is always "administrator". no password (updated 16MAR20)

// maximum intensity of PIN=PWMRANGE
//static char defaultName[] = "Controller";
//static char defaultLocation[] = "Unknown";
//static char defaultFirmware[] = "rgbc.200217.bin";

short toShort(byte aray[]);
void printArray(byte* aray, int sz, boolean printInHex);
void printEEPROM(int sz);

class ESPConfig {
public:
	ESPConfig(char* nam/*controller name*/, char* loc/*location*/, char* fver/*firmware version*/, char* ssid/*router SSID*/, char* ssidpass/*router SSID key*/) {
		DEBUG_PRINTLN("ESPConfig::ESPConfig");

		strcpy(controllerName, nam);
		strcpy(controllerLocation, loc);
		strcpy(firmwareVersion, fver);
		strcpy(routerSSID, ssid);
		strcpy(routerSSIDKey, ssidpass);
	}

public:
	void setupWiFiAP();
	boolean connectToAP(int indicatorPin);
	uint8_t* getMAC();
	void init(int indicatorPin);
	void load();
	void save();
	void fromByteArray(byte command, byte* ar, byte* errordesc, uint16_t* errordesc_length);
	void fromByteArray(byte* ar, byte* errordesc, uint16_t* errordesc_length);
	//byte* toByteArray();
	int toByteArray(byte aray[]);
	char* getSSID();
	char* getPassword();
	char* getControllerName();
	char* getControllerLocation();
	char* getFirmwareVersion();
	int sizeOfEEPROM();
	int sizeOfUDPPayload();
	boolean isConfigured();
	void buildUniqueControllerName(char* uniqueName, int sz);
	void resetEEPROM();
	void clearEEPROM();
	void toString();
	//short discover(byte* replyBuffer);
	short set(byte* replyBuffer, byte* _payload);

private:
	// If controller is starting for the first time, IS_CONFIGURED_BYTE_ADDRESS = 0xFF, else IS_CONFIGURED_BYTE_ADDRESS = 1
	boolean isConf;

	// e.g. "A0,F0,19,23,31,33"
	uint8_t mac[WL_MAC_ADDR_LENGTH];

	// SSID to which (router) this controller connects for LAN/Internet
	char routerSSID[MAX_LENGTH_SSID];

	// Password for SSID to which (router) this controller connects for LAN/Internet
	char routerSSIDKey[MAX_LENGTH_SSID];

	// e.g. "Controller"
	char controllerName[MAX_LENGTH_NAME];

	// e.g. "HR-FBD-21C-510"
	char controllerLocation[MAX_LENGTH_NAME];

	// e.g. "rgbc.191222.bin"
	// format: <4-char device code>.<yymmdd>.bin.
	// Use this 4-char device code to identify the type of controller: "rgbc" for RGB LED Controller, "acds" for AC Dimmer+Switch, "ac3s" for AC Switch
	char firmwareVersion[MAX_LENGTH_NAME];
};
#endif
