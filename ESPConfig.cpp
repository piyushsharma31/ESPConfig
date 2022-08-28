#include "Arduino.h"
#include <WiFiUdp.h>
#include <ESP8266WiFi.h>
#include <ESP8266httpUpdate.h>
#include <EepromUtil.h>
#include <EEPROM.h>
#include "ESPConfig.h"

/***
*
*	1. ESP configuration stored in EEPROM starting at address (0)
*	|--------------------|-----------------------|---------------------------|------------------------|----------------------------|
*	| is config (1 byte) | routerSSID (24 bytes) | routerSSID key (24 bytes) | device name (16 bytes) | device location (16 bytes) |
*	|--------------------|-----------------------|---------------------------|------------------------|----------------------------|
*
*	is config (0/1): if at all the ESP configuration exists on EEPROM. New device will have this byte = 0
*
*	2. UDP packet header & payload
*	|----------------------|------------------|---------|
*	| packet size (2 byte) | command (1 byte) | payload |
*	|----------------------|------------------|---------|
*
*	2. ESP configuration UDP <payload> received from client (Android)
*	|------------------------|----------------------|-----------------------|---------------------------|------------------------|----------------------------|
*	| packet header (3 byte) | config type (1 byte) | routerSSID (24 bytes) | routerSSID key (24 bytes) | device name (16 bytes) | device location (16 bytes) |
*	|------------------------|----------------------|-----------------------|---------------------------|------------------------|----------------------------|
*
*	config type (>0): payload contains only one of the configurations routerSSID/routerSSIDKey/device name/device location
*	config type (=0): payload contains all the configurations
*
*	3. ESP configuration UDP <payload> sent to client (Android)
*	|-----------------|---------------|-----------------------|---------------------------|------------------------|----------------------------|-----------------------------|
*	| config (1 byte) | mac (6 bytes) | routerSSID (24 bytes) | routerSSID key (24 bytes) | device name (16 bytes) | device location (16 bytes) | firmware version (16 bytes) |
*	|-----------------|---------------|-----------------------|---------------------------|------------------------|----------------------------|-----------------------------|
*
***/

void ESPConfig::init(int indicatorPin) {
	DEBUG_PRINTLN("ESPConfig::init");
	//resetEEPROM();

	EEPROM.begin(EEPROM_MAX_ADDR);
	byte rb;
	EepromUtil::eeprom_read_bytes(IS_CONFIGURED_BYTE_ADDRESS, &rb, 1);
	isConf = rb==1?true:false;
	EEPROM.end();

	// mac id (6 bytes)
	memset(mac, 0, sizeof(mac));
	WiFi.macAddress(mac);

	//memset(routerSSID, 0, sizeof(routerSSID));
	//memset(routerSSIDKey, 0, sizeof(routerSSIDKey));
	//strcpy(routerSSID, "onion");//for test, remove afterwards
	//strcpy(routerSSIDKey, "242374666");//for test, remove afterwards
	//save();//for test, remove afterwards

	//strcpy(controllerName, defaultName);
	//strcpy(controllerLocation, defaultLocation);
	//strcpy(firmwareVersion, defaultFirmware);

	// set indicator to LOW to show not connected
	// commented 28-JAN-19, may conflict with PIN state (like in ACDimmer, Switch)
	// uncommented 17122019, use pin# -1 for where GPIO pins cannot be used like in Switch
	analogWrite(indicatorPin, 25);

	if(isConfigured()) {

		DEBUG_PRINTLN("Device is configured!");

		// load from EEPROM
		load();

	} else {

		// New device detected!!!
		DEBUG_PRINTLN("New device detected!");

		// reset EEPROM
		//clearEEPROM();
		// save the factory defaults to EEPROM (from variables)
		save();

		// setup as AP so that user can setup WiFi in AP or STATION mode
		//setupWiFiAP();
	}

	// connect to configured AP with credentials
	if (connectToAP(indicatorPin) == false) {

		DEBUG_PRINTLN("Device connectToAP failed");

		// if AP connect failed, setup itself as WiFi AP
		setupWiFiAP();

	} else {

		// connected to WiFi, indicate with full bright indicator
		// commented 28-JAN-19, may conflict with PIN state (like in ACDimmer, Switch)
		// uncommented 17122019, use pin# -1 for where GPIO pins cannot be used like in Switch
		analogWrite(indicatorPin, 5);

	}

	DEBUG_PRINTLN("ESPConfig::init end");
}
/*
short ESPConfig::discover(byte* replyBuffer) {
	DEBUG_PRINTLN("ESPConfig::discover");

	int _siz = sizeOfUDPPayload();

	// copy configuration to replyBuffer
	//memcpy(replyBuffer, toByteArray(), _siz);
	_siz = toByteArray(replyBuffer);

	DEBUG_PRINTLN("ESPConfig::discover end");

	return _siz;
}
*/
short ESPConfig::set(byte* replyBuffer, byte* _payload) {
	DEBUG_PRINTLN("ESPConfig::set");

	uint16_t errordesc_length = 100;
	byte errordesc[errordesc_length];
	memset(errordesc, 0, errordesc_length);

	fromByteArray(_payload, errordesc, &errordesc_length);

	memcpy(replyBuffer, errordesc, errordesc_length);

	DEBUG_PRINTLN("ESPConfig::set end");
	return errordesc_length;
}

boolean ESPConfig::isConfigured() {
	return isConf;
}

uint8_t* ESPConfig::getMAC() {
	return mac;
}

int ESPConfig::sizeOfEEPROM() {
	// TOTAL SIZE = 1 + (24 x 5) = 1 + 120 = 121

	return sizeof(isConf) // IS_CONFIGURED_BYTE_ADDRESS = 1byte
	+ sizeof(routerSSID) // 24 bytes
	+ sizeof(routerSSIDKey) // 24 bytes
	+ sizeof(controllerName) // 24 bytes
	+ sizeof(controllerLocation); // 24 bytes
	//	+ sizeof(firmwareVersion);// 24 bytes, always stored in variable
}

int ESPConfig::sizeOfUDPPayload() {
	// TOTAL SIZE = 1 + 6 + (24 x 2) + (16 x 3) = 1 + 6 + 120 = 127

	return sizeof(isConf) // IS_CONFIGURED_BYTE_ADDRESS = 1 byte
	+ sizeof(mac) // 6 bytes
	+ sizeof(routerSSID) // 24 bytes
	+ sizeof(routerSSIDKey) // 24 bytes
	+ sizeof(controllerName) // 16 bytes
	+ sizeof(controllerLocation) // 16 bytes
	+ sizeof(firmwareVersion);// 16 bytes
}

/* 
	EEPROM store: [is configured byte][routerSSID bytes][routerSSID routerSSIDKey bytes][controller name bytes][controller location name bytes]
*/

void ESPConfig::load() {
	DEBUG_PRINTLN("ESPConfig::load");
	// 30JUN19, commented to alleviate WiFi reset
	//delay(10);
	EEPROM.begin(EEPROM_MAX_ADDR);

	int readAddress = IS_CONFIGURED_BYTE_ADDRESS;

	// 1st byte: is configured
	byte rb;
	EepromUtil::eeprom_read_bytes(readAddress++, &rb, 1);
	isConf = rb==1?true:false;

	DEBUG_PRINT("isConfigured ");DEBUG_PRINTLN(isConf);

	// routerSSID
	EepromUtil::eeprom_read_bytes(readAddress, (byte*)routerSSID, sizeof(routerSSID));
	readAddress += sizeof(routerSSID);
	//if(strlen(routerSSID)==0) {
	//reset to default
	//strcpy(routerSSID, defaultName);
	//}

	// routerSSIDKey
	EepromUtil::eeprom_read_bytes(readAddress, (byte*)routerSSIDKey, sizeof(routerSSIDKey));
	readAddress += sizeof(routerSSIDKey);

	// controller name
	EepromUtil::eeprom_read_bytes(readAddress, (byte*)controllerName, sizeof(controllerName));
	readAddress += sizeof(controllerName);
	if(strlen(controllerName)==0) {
		//reset to default
		//strcpy(controllerName, defaultName);
	}

	// controller location
	EepromUtil::eeprom_read_bytes(readAddress, (byte*)controllerLocation, sizeof(controllerLocation));
	readAddress += sizeof(controllerLocation);
	if(strlen(controllerLocation)==0) {
		//reset to default
		//strcpy(controllerLocation, defaultLocation);
	}

	// firmware version
	// 17MAR2020, commented 2 lines below since firmware version is always hardcoded in firmwareVersion variable through a constructor
	//EepromUtil::eeprom_read_bytes(readAddress, (byte*)firmwareVersion, sizeof(firmwareVersion));
	//readAddress += sizeof(firmwareVersion);
	if(strlen(firmwareVersion)==0) {
		//reset to default
		//strcpy(firmwareVersion, defaultFirmware);
	}

	// 30JUN19, commented to alleviate WiFi reset
	//delay(10);
	EEPROM.end();

	DEBUG_PRINTLN("ESPConfig::load end");
	return;
}

void ESPConfig::toString() {
#ifdef IS_DEBUG
	DEBUG_PRINT("isConf ");DEBUG_PRINT(isConf);DEBUG_PRINT(", ");
	DEBUG_PRINT("mac ");DEBUG_PRINT_ARRAY(mac, sizeof(mac), true);DEBUG_PRINT(", ");
	DEBUG_PRINT("routerSSID ");DEBUG_PRINT_ARRAY((byte*)routerSSID, sizeof(routerSSID), false);DEBUG_PRINT(", ");
	DEBUG_PRINT("routerSSIDKey ");DEBUG_PRINT_ARRAY((byte*)routerSSIDKey, sizeof(routerSSIDKey), false);DEBUG_PRINT(", ");
	DEBUG_PRINT("controllerName ");DEBUG_PRINT_ARRAY((byte*)controllerName, sizeof(controllerName), false);DEBUG_PRINT(", ");
	DEBUG_PRINT("controllerLocation ");DEBUG_PRINT_ARRAY((byte*)controllerLocation, sizeof(controllerLocation), false);DEBUG_PRINT(", ");
	DEBUG_PRINT("firmwareVersion ");DEBUG_PRINT_ARRAY((byte*)firmwareVersion, sizeof(firmwareVersion), false);DEBUG_PRINTLN();
#endif
}

void ESPConfig::fromByteArray(byte command, byte* aray, byte* errordesc, uint16_t* errordesc_length) {
	DEBUG_PRINT("ESPConfig::fromByteArray command ");DEBUG_PRINTLN(command);

	int index = 0;
	uint8_t retvalue = 255;//default value 255 for all commands except DEVICE_COMMAND_FIRMWARE_UPDATE
	uint16_t _error_length = 0;

	if(command==DEVICE_COMMAND_SET_CONFIGURATION_NAME) {
		// controller name (24 bytes)
		memset(controllerName, 0, sizeof(controllerName));
		memcpy(controllerName, aray+index, sizeof(controllerName));
		index += sizeof(controllerName);

	} else if(command==DEVICE_COMMAND_SET_CONFIGURATION_SSID) {
		// routerSSID (24 bytes)
		memset(routerSSID, 0, sizeof(routerSSID));
		memcpy(routerSSID, aray+index, sizeof(routerSSID));
		index += sizeof(routerSSID);

		// routerSSIDKey (24 bytes) starts after routerSSID
		memset(routerSSIDKey, 0, sizeof(routerSSIDKey));
		memcpy(routerSSIDKey, aray+index, sizeof(routerSSIDKey));
		index += sizeof(routerSSIDKey);

	} else if(command==DEVICE_COMMAND_SET_CONFIGURATION_AP) {
		//erase routerSSID, routerSSIDKey so that device boot as AP
		memset(routerSSIDKey, 0, sizeof(routerSSIDKey));
		memset(routerSSID, 0, sizeof(routerSSID));

	} else if(command==DEVICE_COMMAND_SET_CONFIGURATION_LOCATION) {
		// controller location (24 bytes) starts after controller name
		memset(controllerLocation, 0, sizeof(controllerLocation));
		memcpy(controllerLocation, aray+index, sizeof(controllerLocation));
		index += sizeof(controllerLocation);

	} else if(command==DEVICE_COMMAND_FIRMWARE_UPDATE) {

		byte boot_after_update = 0;
		uint16_t url_length = 0;

		memcpy(&boot_after_update, aray+index, sizeof(boot_after_update));
		index += sizeof(boot_after_update);
		memcpy(&url_length, aray+index, sizeof(url_length));
		index += sizeof(url_length);

		char firmwareurl[url_length+1];
		memset(firmwareurl, 0, sizeof(firmwareurl));
		memcpy(firmwareurl, aray+index, url_length);
		index += url_length;

		String url = firmwareurl;

		DEBUG_PRINT("ESPConfig::fromByteArray boot_after_update ");DEBUG_PRINT(boot_after_update);DEBUG_PRINT(", url_length ");DEBUG_PRINT(url_length);DEBUG_PRINT(", url ");DEBUG_PRINTLN(url);
		DEBUG_PRINT("ESPConfig::fromByteArray ESPhttpUpdate.updating ...");

		ESPhttpUpdate.rebootOnUpdate(boot_after_update);
		WiFiClient wifiClient;
		//retvalue = ESPhttpUpdate.update(url);//03-APR-2022; based on old lib ESP8266 2.7.4
		retvalue = ESPhttpUpdate.update(wifiClient, url, firmwareVersion);//03-APR-2022; based on new lib ESP8266 3.0.2

		DEBUG_PRINT(" t_httpUpdate_return ");DEBUG_PRINT(retvalue);DEBUG_PRINTLN(" done");

		String _error = String(ESPhttpUpdate.getLastErrorString());
		_error.concat(", code: ");
		_error.concat(ESPhttpUpdate.getLastError());
		_error.getBytes(errordesc, *errordesc_length);
		_error_length = _error.length();
		//sprintf((char*)errordesc, "Error (%d): %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());

		switch (retvalue) {

		case HTTP_UPDATE_FAILED:

			//			Serial.printf("ESPConfig::fromByteArray HTTP_UPDATE_FAILED Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());DEBUG_PRINTLN();
			DEBUG_PRINT("ESPConfig::fromByteArray HTTP_UPDATE_FAILED _error_length ");DEBUG_PRINT(_error_length);DEBUG_PRINT(", ");DEBUG_PRINTLN(_error);
			break;

		case HTTP_UPDATE_NO_UPDATES:
			DEBUG_PRINTLN("ESPConfig::fromByteArray HTTP_UPDATE_NO_UPDATES");
			break;

		case HTTP_UPDATE_OK:
			DEBUG_PRINTLN("ESPConfig::fromByteArray HTTP_UPDATE_OK");

			url = url.substring(url.lastIndexOf('/')+1);
			DEBUG_PRINT("ESPConfig::fromByteArray firmwareVersion updated");DEBUG_PRINTLN(url);

			// update firmwareVersion variable (this will write to EEPROM when save() called)
			// commented 17MAR2020, firmwareVersion is hardcoded in variable, ESPConfig constructor, it will reflect once ESP reboots after this update
			//memset(firmwareVersion, 0, sizeof(firmwareVersion));
			//url.toCharArray(firmwareVersion, sizeof(firmwareVersion));

			break;

		default:
			DEBUG_PRINTLN("Undefined HTTP_UPDATE Code: ");

		}

	}

	// update error length in the variable pointer errordesc_length
	memcpy(errordesc_length, &_error_length, sizeof(uint16_t));

	save();
	printEEPROM(sizeOfEEPROM());

	DEBUG_PRINT("retvalue ");DEBUG_PRINTLN(retvalue);
	DEBUG_PRINT("errordesc_length ");DEBUG_PRINTLN(*errordesc_length);
	DEBUG_PRINT("errordesc ");DEBUG_PRINTLN((char*)errordesc);

	DEBUG_PRINTLN("ESPConfig::fromByteArray command end");
	//return retvalue;
}

void ESPConfig::fromByteArray(byte* aray, byte* errordesc, uint16_t* errordesc_length) {
	int index = 0;

	byte parameter = aray[index++];

	if(parameter!=0) {
		// byte array contains only one configuration parameters value

		//return fromByteArray(parameter, aray+1, errordesc, errordesc_length);
		fromByteArray(parameter, aray+1, errordesc, errordesc_length);
		return;
	}

	DEBUG_PRINTLN("ESPConfig::fromByteArray parameter==0!!!!!!!");

	// byte array contains all configuration parameters value

	// routerSSID (24 bytes)
	memcpy(routerSSID, aray+index, sizeof(routerSSID));
	index += sizeof(routerSSID);

	// routerSSIDKey (24 bytes) starts after routerSSID
	memcpy(routerSSIDKey, aray+index, sizeof(routerSSIDKey));
	index += sizeof(routerSSIDKey);

	// controller name (24 bytes) starts after routerSSIDKey
	memcpy(controllerName, aray+index, sizeof(controllerName));
	index += sizeof(controllerName);

	// controller location (24 bytes) starts after controller name
	memcpy(controllerLocation, aray+index, sizeof(controllerLocation));
	index += sizeof(controllerLocation);

	save();
	printEEPROM(sizeOfEEPROM());

	DEBUG_PRINTLN("ESPConfig::fromByteArray parameter==0!!!!!!! end");
	//return 255;
}

//byte* ESPConfig::toByteArray() {
/*	DEBUG_PRINTLN("ESPConfig::toByteArray");

	byte aray[sizeOfUDPPayload()];

	memset(aray, 0, sizeof(aray));
	int index = 0;

	// 1st byte: is configured
	memcpy(aray+index, &isConf, 1);
	index++;

	// mac id
	memcpy(aray+index, mac, sizeof(mac));
	index += sizeof(mac);

	// routerSSID
	memcpy(aray+index, routerSSID, sizeof(routerSSID));
	index += sizeof(routerSSID);

	// routerSSIDKey
	memcpy(aray+index, routerSSIDKey, sizeof(routerSSIDKey));
	index += sizeof(routerSSIDKey);

	// controller name
	memcpy(aray+index, controllerName, sizeof(controllerName));
	index += sizeof(controllerName);

	// controller location
	memcpy(aray+index, controllerLocation, sizeof(controllerLocation));
	index += sizeof(controllerLocation);

	// firmwareVersion
	memcpy(aray+index, firmwareVersion, sizeof(firmwareVersion));
	index += sizeof(firmwareVersion);

	//printArray(aray, sizeof(aray), false);DEBUG_PRINTLN();
	DEBUG_PRINTLN("ESPConfig::toByteArray end");

	return aray;
}*/

int ESPConfig::toByteArray(byte aray[]) {
	DEBUG_PRINTLN("ESPConfig::toByteArray");

	//byte aray[sizeOfUDPPayload()];

	//memset(aray, 0, sizeof(aray));
	int index = 0;

	// 1st byte: is configured
	memcpy(aray+index, &isConf, 1);
	index++;

	// mac id
	memcpy(aray+index, mac, sizeof(mac));
	index += sizeof(mac);

	// routerSSID
	memcpy(aray+index, routerSSID, sizeof(routerSSID));
	index += sizeof(routerSSID);

	// routerSSIDKey
	memcpy(aray+index, routerSSIDKey, sizeof(routerSSIDKey));
	index += sizeof(routerSSIDKey);

	// controller name
	memcpy(aray+index, controllerName, sizeof(controllerName));
	index += sizeof(controllerName);

	// controller location
	memcpy(aray+index, controllerLocation, sizeof(controllerLocation));
	index += sizeof(controllerLocation);

	// firmwareVersion
	memcpy(aray+index, firmwareVersion, sizeof(firmwareVersion));
	index += sizeof(firmwareVersion);

	//printArray(aray, sizeof(aray), false);DEBUG_PRINTLN();
	DEBUG_PRINT_ARRAY(aray, index, false);
	DEBUG_PRINTLN("ESPConfig::toByteArray end");

	return (index);
}

/*
	EEPROM is specified to handle 100,000 read/erase cycles. 
	This means you can write and then erase/re-write data 
	100,000 times before the EEPROM will become unstable
	It’s important to note that this limit applies to each 
	memory location
*/
void ESPConfig::save(void) {
	DEBUG_PRINTLN("ESPConfig::save");

	// 30JUN19, commented to alleviate WiFi reset
	//delay(10);
	EEPROM.begin(EEPROM_MAX_ADDR);

	int writeAddress = IS_CONFIGURED_BYTE_ADDRESS;
	byte b = 1;

	EepromUtil::eeprom_update_bytes(writeAddress++, &b, 1);
	isConf = true;

	// routerSSID value
	EepromUtil::eeprom_update_bytes(writeAddress, (byte*)routerSSID, sizeof(routerSSID));
	writeAddress += sizeof(routerSSID);

	// routerSSIDKey value
	EepromUtil::eeprom_update_bytes(writeAddress, (byte*)routerSSIDKey, sizeof(routerSSIDKey));
	writeAddress += sizeof(routerSSIDKey);

	// controller name
	EepromUtil::eeprom_update_bytes(writeAddress, (byte*)controllerName, sizeof(controllerName));
	writeAddress += sizeof(controllerName);

	// controller location
	EepromUtil::eeprom_update_bytes(writeAddress, (byte*)controllerLocation, sizeof(controllerLocation));
	writeAddress += sizeof(controllerLocation);

	// firmwareVersion
	// commented 17MAR2020, firmware version is stored in variable only
	//EepromUtil::eeprom_update_bytes(writeAddress, (byte*)firmwareVersion, sizeof(firmwareVersion));
	//writeAddress += sizeof(firmwareVersion);

	EEPROM.commit();
	EEPROM.end();

	// 30JUN19, commented to alleviate WiFi reset
	//delay(10);
	DEBUG_PRINTLN("ESPConfig::save end");
}

char* ESPConfig::getSSID() {
	return routerSSID;
}

char* ESPConfig::getPassword() {
	return routerSSIDKey;
}

char* ESPConfig::getControllerName() {
	return controllerName;
}

char* ESPConfig::getControllerLocation() {
	return controllerLocation;
}

char* ESPConfig::getFirmwareVersion() {
	return firmwareVersion;
}

void ESPConfig::buildUniqueControllerName(char* uniqueName, int sz) {

	// last three bytes of the MAC (HEX'd) to "controllerName-":
	DEBUG_PRINT("buildUniqueControllerName ");DEBUG_PRINTLN(controllerName);

	memset(uniqueName, 0, sz);
	strcat(uniqueName, CONTROLLER_UNIQUE_SSID);
	strcat(uniqueName, String(getMAC()[WL_MAC_ADDR_LENGTH - 3], HEX).c_str());
	strcat(uniqueName, String(getMAC()[WL_MAC_ADDR_LENGTH - 2], HEX).c_str());
	strcat(uniqueName, String(getMAC()[WL_MAC_ADDR_LENGTH - 1], HEX).c_str());
	DEBUG_PRINT("buildUniqueControllerName uniqueName ");DEBUG_PRINTLN(uniqueName);
}

// write zeros at all addresses
void ESPConfig::clearEEPROM() {
	// 30JUN19, commented to alleviate WiFi reset
	//delay(10);
	EEPROM.begin(EEPROM_MAX_ADDR);
	byte b = 0;

	for (int i = 0; i < EEPROM_MAX_ADDR; i++) {
		EepromUtil::eeprom_update_bytes(i, &b, 1);
		//EEPROM.write(i, -1);
	}

	EEPROM.commit();
	EEPROM.end();
	// 30JUN19, commented to alleviate WiFi reset
	//delay(10);
}

// write 0xff at all addresses
void ESPConfig::resetEEPROM() {
	// 30JUN19, commented to alleviate WiFi reset
	//delay(10);
	EEPROM.begin(EEPROM_MAX_ADDR);
	byte b = -1;

	for (int i = 0; i < EEPROM_MAX_ADDR; i++) {
		EepromUtil::eeprom_update_bytes(i, &b, 1);
		//EEPROM.write(i, -1);
	}

	EEPROM.commit();
	EEPROM.end();
	// 30JUN19, commented to alleviate WiFi reset
	//delay(10);
}

void printArray(byte* aray, int sz, boolean printInHex) {
#ifdef IS_DEBUG
	DEBUG_PRINT("printArray (size ");DEBUG_PRINT(sz);DEBUG_PRINT(") ");
	for(int i=0; i<sz; i++) {
		if(printInHex || !isPrintable(aray[i]))
		DEBUG_PRINTXY(aray[i], HEX);
		else
		DEBUG_PRINT((char)aray[i]);

		DEBUG_PRINT(' ');
	}
	DEBUG_PRINTLN();
#endif
}

void printEEPROM(int sz) {
#ifdef IS_DEBUG
	byte aray;
	DEBUG_PRINT("printEEPROM ");

	// 30JUN19, commented to alleviate WiFi reset
	//delay(10);
	EEPROM.begin(sz);

	for(int i=0; i<sz; i++) {
		aray = EEPROM.read(i);

		if(isPrintable(aray)) {
			DEBUG_PRINT((char)aray);
		} else {
			DEBUG_PRINTXY(aray, HEX);
		}
		DEBUG_PRINT(' ');
	}
	DEBUG_PRINTLN();
	// 30JUN19, commented to alleviate WiFi reset
	//delay(10);
	EEPROM.end();
#endif
}

short toShort(byte aray[]) {
	byte one = aray[0];
	byte two = aray[1];

	return two << 8 | one;
}

void ESPConfig::setupWiFiAP() {

	char ssd[MAX_LENGTH_SSID];
	buildUniqueControllerName(ssd, MAX_LENGTH_SSID);
	WiFi.mode(WIFI_AP);
	WiFi.softAP(ssd, CONTROLLER_UNIQUE_SSID_KEY);

	DEBUG_PRINT("setupWiFiAP ");DEBUG_PRINT(ssd);DEBUG_PRINT(", ");DEBUG_PRINT(CONTROLLER_UNIQUE_SSID_KEY);DEBUG_PRINTLN(" end");
}

boolean ESPConfig::connectToAP(int indicatorPin) {
	if(strlen(getSSID())==0 || strlen(getPassword())==0) {
		return false;
	}

	DEBUG_PRINT("connectToAP ");DEBUG_PRINT(String(getSSID()));DEBUG_PRINT(", ");DEBUG_PRINTLN(String(getPassword()));

	int retry_time = 0;
	int retry_delay = 500;//milliseconds

	// wifi states: WIFI_AP, WIFI_AP_STA, WIFI_OFF
	// commented 20AUG2022 as this clashed with NTPClient lib used in DHTSensor_Adafruit project. It should not have impact on other projects
	//WiFi.mode(WIFI_STA);
	WiFi.begin(getSSID(), getPassword());

	while (WiFi.status() != WL_CONNECTED && retry_time < max_retry_wifi_ap_connect_time) {
		// commented 28-JAN-19, may conflict with PIN state (like in ACDimmer, Switch)
		// uncommented 17122019, use pin# -1 for where GPIO pins cannot be used like in Switch
		analogWrite(indicatorPin, 0);
		delay(retry_delay);
		retry_time = retry_time + retry_delay;

		// commented 28-JAN-19, may conflict with PIN state (like in ACDimmer, Switch)
		// uncommented 17122019, use pin# -1 for where GPIO pins cannot be used like in Switch
		analogWrite(indicatorPin, 25);
		delay(retry_delay);
		retry_time = retry_time + retry_delay;

		DEBUG_PRINT(".");
	}

	DEBUG_PRINTLN();

	if (WiFi.status() != WL_CONNECTED) {
		DEBUG_PRINT("WiFi connect timeout");DEBUG_PRINTLN();
		return false;
	} else {
		DEBUG_PRINT("WiFi connected to ");DEBUG_PRINTLN(WiFi.localIP());
		return true;
	}
}
