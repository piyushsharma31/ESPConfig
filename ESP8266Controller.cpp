#include <EepromUtil.h>
#include <ESPConfig.h>
#include "ESP8266Controller.h"

// set capability value
boolean ESP8266Controller::setCapability(char* cname, uint16_t value) {

	for (int i = 0; i < capabilityCount; i++) {
		if (strcmp(cname, capabilities[i]._name)==0) {
			if(value <= capabilities[i]._value_max && value >= capabilities[i]._value_min) {
				capabilities[i]._value = value;
				DEBUG_PRINT("setCapability ");DEBUG_PRINT(cname);DEBUG_PRINT("=");DEBUG_PRINTLN(value);
				return true;
			} else {
				DEBUG_PRINT("***MISMATCH*** setCapability ");DEBUG_PRINT(capabilities[i]._name);DEBUG_PRINT(", val ");DEBUG_PRINTLN(value);
				return false;
			}
		}
	}

	return false;
}

void ESP8266Controller::toString() {
#ifdef IS_DEBUG

	DEBUG_PRINT("pin ");DEBUG_PRINT(pin);DEBUG_PRINT(", controllerName ");DEBUG_PRINT(controllerName);
	for (int i = 0; i < capabilityCount; i++) {
		DEBUG_PRINT(", ");DEBUG_PRINT(capabilities[i]._name);DEBUG_PRINT("=");DEBUG_PRINT(capabilities[i]._value);
	}
	DEBUG_PRINT(", pinState=");DEBUG_PRINT(pinState);
	DEBUG_PRINTLN();
#endif
}

// size required for storing this controller capabilities on EEPROM
int ESP8266Controller::sizeOfEEPROM() {

	int size = 0;
	size += sizeof (pin);
	size += sizeof (capabilityCount);
	size += capabilityCount * (sizeof(capabilities[0]._value) + sizeof(capabilities[0]._name));

	return size;
}

// size of the UDP payload for this controller capabilities
// example 1 : 2 + 16 + (6*30) = 18 + 180 = 198 bytes for LEDController
// example 2 : 2 + 16 + (4*30) = 18 + 120 = 138 bytes for ACDimmer
int ESP8266Controller::sizeOfUDPPayload() {

	int size = 0;
	size += sizeof (pin);
	size += sizeof (capabilityCount);
	size += sizeof (controllerName);
	size += capabilityCount * sizeof(_unit16_capability);
	return size;
}

// output this controller capabilities to byte array
//byte* ESP8266Controller::toByteArray() {
/*
	DEBUG_PRINTLN("LEDController::toByteArray");

	// [pin][no_of_capabilities][capability name][min value][max value][value]
	byte aray[sizeOfUDPPayload()];
	memset(aray, 0, sizeof(aray));
	int index = 0;

	// pin
	memcpy(aray + index, &pin, sizeof(pin));
	index += sizeof(pin);

	// number of capabilities
	memcpy(aray + index, &capabilityCount, 1);
	index += sizeof(capabilityCount);

	// controller name
	memcpy(aray + index, controllerName, sizeof(controllerName));
	index += sizeof(controllerName);

	for (int i = 0; i < capabilityCount; i++) {

		// capability name
		memcpy(aray + index, capabilities[i]._name, sizeof(capabilities[i]._name));
		index += sizeof(capabilities[i]._name);

		// capability min value
		aray[index++] = lowByte(capabilities[i]._value_min);
		aray[index++] = highByte(capabilities[i]._value_min);

		// capability max value
		aray[index++] = lowByte(capabilities[i]._value_max);
		aray[index++] = highByte(capabilities[i]._value_max);

		// capability value
		aray[index++] = lowByte(capabilities[i]._value);
		aray[index++] = highByte(capabilities[i]._value);
	}

	printArray(aray, sizeof(aray), false);
	DEBUG_PRINTLN("LEDController::toByteArray end");

	return aray;
}*/

// output this controller capabilities to byte array
int ESP8266Controller::toByteArray(byte aray[]) {

	DEBUG_PRINTLN("LEDController::toByteArray");

	// [pin][no_of_capabilities][capability name][min value][max value][value]
	//byte aray[sizeOfUDPPayload()];
	//memset(aray, 0, sizeof(aray));
	int index = 0;

	// pin
	memcpy(aray + index, &pin, sizeof(pin));
	index += sizeof(pin);

	// number of capabilities
	memcpy(aray + index, &capabilityCount, 1);
	index += sizeof(capabilityCount);

	// controller name
	memcpy(aray + index, controllerName, sizeof(controllerName));
	index += sizeof(controllerName);

	for (int i = 0; i < capabilityCount; i++) {

		// capability name
		memcpy(aray + index, capabilities[i]._name, sizeof(capabilities[i]._name));
		index += sizeof(capabilities[i]._name);

		// capability min value
		aray[index++] = lowByte(capabilities[i]._value_min);
		aray[index++] = highByte(capabilities[i]._value_min);

		// capability max value
		aray[index++] = lowByte(capabilities[i]._value_max);
		aray[index++] = highByte(capabilities[i]._value_max);

		// capability value
		aray[index++] = lowByte(capabilities[i]._value);
		aray[index++] = highByte(capabilities[i]._value);
	}

	//printArray(aray, sizeof(aray), false);
	DEBUG_PRINT_ARRAY(aray, index, false);
	DEBUG_PRINTLN("LEDController::toByteArray end");

	return (index);
}

// set capabilities from a given byte array
// Android client will generally send one capability at a time to update @device
boolean ESP8266Controller::fromByteArray(byte aray[])  {

	if(aray[0]!=pin) {
		DEBUG_PRINT("LEDController::fromByteArray end, wrong pin!");DEBUG_PRINT(", thispin ");DEBUG_PRINT(aray[0]);DEBUG_PRINT(", pin ");DEBUG_PRINT(pin);DEBUG_PRINTLN();
		return false;
	}

	int index = 0;
	byte thispin = aray[index++];
	int no_of_capabilities = aray[index++];

	DEBUG_PRINT("LEDController::fromByteArray ");DEBUG_PRINT("pin ");DEBUG_PRINTLN(thispin);

	// skip copying controller name from Android client
	// to change controller name Android client can create a local mapping (_name==new_name)

	for (int i = 0; i < no_of_capabilities; i++) {

		// copy capability name
		char nme[sizeof(capabilities[i]._name)];
		memcpy(nme, aray+index, sizeof(capabilities[i]._name));
		index += sizeof(capabilities[i]._name);

		// copy capability minimum value
		// skip copying min, max values from Android client (they never change)
		// capabilities[i]._value_min = toShort(aray + index);
		//index += sizeof(capabilities[i]._value_min);

		// copy capability maximum value
		// skip copying min, max values from Android client (they never change)
		// capabilities[i]._value_max = toShort(aray + index);
		//index += sizeof(capabilities[i]._value_max);

		// copy capability value
		short val = toShort(aray + index);
		index += sizeof(capabilities[i]._value);

		setCapability(nme, val);

	}

	eepromUpdatePending = true;

	DEBUG_PRINTLN("LEDController::fromByteArray end");
	return true;
}

// set controller capabilities from EEPROM
//void ESP8266Controller::loadCapabilities(int start_address) {
void ESP8266Controller::loadCapabilities() {

	DEBUG_PRINT("LEDController::loadCapabilities at ");DEBUG_PRINTLN(eeprom_address);
	if (eeprom_address == 0)
	return;

	DEBUG_PRINT(", pin ");DEBUG_PRINT(pin);DEBUG_PRINT(", size ");DEBUG_PRINTLN(sizeOfEEPROM());

	byte aray[sizeOfEEPROM()];
	memset(aray, 0, sizeof(aray));

	// 30JUN19, commented to alleviate WiFi reset
	//delay(10);
	EEPROM.begin(1024);
	EepromUtil::eeprom_read_bytes(eeprom_address, aray, sizeof (aray));
	// 30JUN19, commented to alleviate WiFi reset
	//delay(10);
	EEPROM.end();

	//fromByteArray(aray);
	//readEEPROM(aray);
	int index = 0;
	byte thispin = aray[index++];

	if(thispin!=pin) {
		return;
	}

	int no_of_capabilities = aray[index++];
	DEBUG_PRINT("loadCapabilities no_of_capabilities ");DEBUG_PRINTLN(no_of_capabilities);

	for (int i = 0; i < no_of_capabilities; i++) {

		// copy capability name
		char nme[sizeof(capabilities[i]._name)];
		memcpy(nme, aray+index, sizeof(capabilities[i]._name));
		index += sizeof(capabilities[i]._name);

		// copy capability value
		short val = toShort(aray + index);
		index += sizeof(capabilities[i]._value);

		setCapability(nme, val);
	}

	toString();
	DEBUG_PRINTLN("ESP8266Controller::loadCapabilities end");
}

// save controller capabilities into EEPROM
void ESP8266Controller::saveCapabilities() {
	/* commented 17MAR2020, delay in save is implemented in loop()
	if (eeprom_address == 0 || (millis() - capabilitiesLastSaved < CAPABILITY_SAVE_INTERVAL && pinLastSaved==pin)) {
		DEBUG_PRINT("ESP8266Controller::saveCapabilities ***too early after last EEPROM write*** ");DEBUG_PRINT(eeprom_address);DEBUG_PRINT(", pin ");DEBUG_PRINTLN(pin);
		return;
	}
*/
	pinLastSaved = pin;

	capabilitiesLastSaved = millis();

	DEBUG_PRINT("ESP8266Controller::saveCapabilities at ");DEBUG_PRINT(eeprom_address);DEBUG_PRINT(", pin ");DEBUG_PRINT(pin);

	byte aray[sizeOfEEPROM()];
	memset(aray, 0, sizeof(aray));
	int index = 0;

	DEBUG_PRINT(", size ");DEBUG_PRINTLN(sizeof(aray));
	// pin
	memcpy(aray + index, &pin, sizeof(pin));
	index += sizeof(pin);

	// number of capabilities
	memcpy(aray + index, &capabilityCount, 1);
	index += sizeof(capabilityCount);

	for (int i = 0; i < capabilityCount; i++) {

		// capability name
		memcpy(aray + index, capabilities[i]._name, sizeof(capabilities[i]._name));
		index += sizeof(capabilities[i]._name);

		// value
		aray[index++] = lowByte(capabilities[i]._value);
		aray[index++] = highByte(capabilities[i]._value);
	}

	// 30JUN19, commented to alleviate WiFi reset
	//delay(10);
	EEPROM.begin(1024);

	// mark as configured
	byte b = 1;
	EepromUtil::eeprom_update_bytes(IS_CONFIGURED_BYTE_ADDRESS, &b, 1);

	EepromUtil::eeprom_update_bytes(eeprom_address, aray, sizeof(aray));

	EEPROM.commit();
	EEPROM.end();
	// 30JUN19, commented to alleviate WiFi reset
	//delay(10);

	eepromUpdatePending = false;

	//printArray(aray, sizeof(aray), false);
	DEBUG_PRINTLN("LEDController::saveCapabilities end");
}
