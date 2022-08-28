#ifndef ESP8266Controller_h
#define ESP8266Controller_h

typedef struct {
	// packet size
	uint16_t _size;

	// commands:discover(1), set WiFi credentials(2), get capability (5), set device name(4), set capability(6)
	uint8_t _command;

	//	command response:
	//	WiFi format     	: [size_of_ssid(1 byte)][SSID string][size_of_pwd][PWD string]
	//	Discovery format  	: [size_of_ssid][SSID string][size_of_device_name][device name string][size_of_device_location][device location string]
	//	Device info format  : [size_of_device_name][device name string][size_of_device_location][device location string]
	//	Device capabilities : {size_of_pin][pin][capability_name][min value][max value][value]

	//	capability name is always 24 bytes when sent over network. the string ending char '\0' must be present within 24 bytes

	char* _payload;

} _udp_packet;

typedef struct {

public:

	char _name[16];
	uint16_t _value_min;
	uint16_t _value_max;
	uint16_t _value;

} _unit16_capability;

class ESP8266Controller {

public:

	ESP8266Controller(char* nam, uint8_t _pin, uint8_t capCount, int start_address) {
		DEBUG_PRINTLN("ESP8266Controller::ESP8266Controller");

		pin = _pin;
		capabilityCount = capCount;
		eeprom_address = start_address;
		strcpy(controllerName, nam);
		capabilities = (_unit16_capability*)malloc (sizeof(_unit16_capability) *capabilityCount);
	}

public:
	// controller name
	char controllerName[16];

	// PIN# to which device/sensor/bulb/LED is connected
	uint8_t pin;

	// EEPROM address where this controller capabilities are stored
	int eeprom_address = 0;

	// last time EEPROM was updated with this object
	unsigned long lastEepromUpdate = 0;

	// if EEPROM and this object are in sync or not
	boolean eepromUpdatePending = false;

	// PIN current state
	short pinState = LOW;

	// list of capabilities
	_unit16_capability *capabilities;

	// "capabilityCount" MUST BE CHANGED FOR EACH ESP IMPLEMENTATION
	// e.g. RGB LED CONTROLLER HAS SIX(6) CAPABILITIES
	// e.g. AC DIMMER HAS FOUR(4) CAPABILITIES
	uint8_t capabilityCount = 0;

	// capabilities of the device which can be controlled by this class
	virtual boolean setCapability(char* cname, uint16_t value);

	// load capability data into variables from EEPROM
	virtual void loadCapabilities();

	// save capabilities to EEPROM from variables
	virtual void saveCapabilities();

	// get EEPROM saved capabilities into byte array
	//virtual byte* toByteArray();
	virtual int toByteArray(byte aray[]);

	// initialize the capabilities with provided array (capabilities) received over network
	virtual boolean fromByteArray(byte aray[]);

	// size occupied by this controller capabilities saved in EEPROM
	int sizeOfEEPROM();

	// size required by this controller capabilities as UDP payload
	int sizeOfUDPPayload();

	// loop is called in loop() to reflect the current state of pin (time based changes - blink LED every second)
	virtual void loop() = 0;

	void toString();

};

#endif
