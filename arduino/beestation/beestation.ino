/*
*  WiFi beekeeping monitor station with Arduino,
*  BME 280 temperature-pressure-humidity sensor,
*  Maxim DS2401 silicon serial number,
*  Maxim DS18B20 temperature sensors,
*  & the CC3000 chip WiFi.
*
*  Part of the code is based on the work done by Adafruit on the CC3000 chip & the DHT11 sensor.
*  Based in part on dht11/cc3300 weatherstation written by Marco Schwartz for Open Home Automation.
*  Based in part on OneWire/DS18B20 sample code from the OneWire library.
*  Lots of ideas taken from the many helpful posts in the community online.
*
*  Purpose:
*   Sends data to LAMP backend with HTTP GETs
*   Be stable over all conditions (Hardware reset cicruit currently required)
*
*  To Do:
*   More sensors, besides temperature. <<In Progress>>
*   Better network recovery code - I saw some ideas online
*   Powersaving by sleeping instead of waiting
*   Reduce code size
*
*/

#define USE_DHT 0
#define USE_CC3300 01
#define USE_DS2401 1
#define USE_DS18B20 1
#define USE_BME 1
#define MY_DEBUG 0

// Include required libraries
#include <Adafruit_CC3000.h> // wifi library
#include <SPI.h> // how to talk to adafruit cc3300 board
#include <OneWire.h> // PJRC OneWire library to read DS 1-Wire sensors, like DS2401 and DS18B20
					 // http://www.pjrc.com/teensy/td_libs_OneWire.html
#if USE_BME 
// Using I2C protocol
#include <Adafruit_BME280.h>
#endif // USE_BME

#include <avr/wdt.h> // Watchdog timer

#include <mynetwork.h> // defines network secrets and addresses
// WLAN_SSID
// WLAN_PASS
// WEB_SERVER_IP_COMMAS 
// WEB_SERVER_IP_DOTS 

// WiFi network 
#define WLAN_SECURITY   WLAN_SEC_WPA2

// Define CC3000 chip pins (WiFi board)
#define ADAFRUIT_CC3000_IRQ   3
#define ADAFRUIT_CC3000_VBAT  5
#define ADAFRUIT_CC3000_CS    10

// DS2401 bus pin
#define DS2401PIN 9
#define DS18B20PIN 6

// Create global CC3000 instance
Adafruit_CC3000 gCc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS,
	ADAFRUIT_CC3000_IRQ,
	ADAFRUIT_CC3000_VBAT,
	SPI_CLOCK_DIVIDER);

#if USE_BME
Adafruit_BME280 gBme280; // I2C
// Variables to be exposed 
int gHumidity = 0;
int gPressure = 0;
int gTemperature = 0; 
#endif // USE_BME

#if USE_DS18B20
OneWire gDs(DS18B20PIN);
int gDevices;
#define DS_COUNT 10
byte gDsAddrs[DS_COUNT][8];
// Use array for DS18B20 temperatures and simplify HTTP printout code
int gDsTemps[DS_COUNT];
#endif // USE_DS18B20

// PHP server on backend
const char gWebServer[] = WEB_SERVER_IP_DOTS; // IP Address (or name) of server to dump data to 
#define WEB_PORT 80
uint32_t gIp;

//unsigned long gTimeNow;
unsigned long gCycles = 0;

// Serial Number of this beestation 
char gSerialNumber[3]; // read from Maxim DS2401 on pin 9, use rightmost word

const int gRedLED = 8; // LED on pin
const int gYellowLED = 7; // LED on pin 7

void setup(void)
{
	wdt_disable();
	// Blink LED
	pinMode(gRedLED, OUTPUT);
	pinMode(gYellowLED, OUTPUT);
	digitalWrite(gYellowLED, HIGH);
	digitalWrite(gRedLED, HIGH);
	delay(500);
	digitalWrite(gRedLED, LOW);

	// Start Serial
	Serial.begin(115200); // 9600
	Serial.print(F("\n\nInitializing BeeStation #"));

	// DS2401
	readSerialNumber(); // Populate gSerialNumber
	printFreeRam();
#if USE_DS18B20 
	setupDs18B20();
#endif // USE_DS18B20 

#if USE_BME
	// Initialize BMP180 sensor
	if (!gBme280.begin())
	{
		Serial.print(F("Cannot read BME280 sensor!"));
		while (1);
	}
#endif // USE_BME

	printFreeRam();

#if USE_CC3300
	setupCc3000();
	displayConnectionDetails();
#endif // USE_CC3300
	delay(2000);
#if USE_CC3300
	gIp = gCc3000.IP2U32(WEB_SERVER_IP_COMMAS);
	Serial.print(F("Connecting to web server "));
	gCc3000.printIPdotsRev(gIp);
	Serial.println();
	postStatusToWeb(gIp, "Start");
#endif
	Serial.println(F("Looping"));
	digitalWrite(gYellowLED, LOW);
	wdt_enable(WDTO_8S); // options: WDTO_1S, WDTO_2S, WDTO_4S, WDTO_8S
}

void loop(void)
{
	wdt_reset();
	resetHardwareWatchdog();
	gCycles++;
	Serial.print(F("Cycles:"));
	Serial.println(gCycles);
	printFreeRam();

#if USE_BME
	// Measure from BMP180 
	measureBme280();
	// As I suspected, the temperature reading on the particular BME280 sensor is high about 1 degree C
	// compared to other temperature measurements - 29 Aug 2016 TCW
	wdt_reset();
#endif

#if USE_DS18B20
	measureDs18B20();
	wdt_reset();
#endif // USE_DS18B20

#if USE_CC3300
	Serial.println(F("Connecting to web server"));
	wdt_reset();
	postReadingsToWeb(gIp); 
	wdt_reset();
	if (0 == (gCycles % 100)) {
		char message[64];
		char iBuf[12];
		strlcpy(message, itoa(gCycles, iBuf, 10), 64);
		strlcat(message, ("%20cycles"), 64);
		postStatusToWeb(gIp, message);
	}
	wdt_reset();
	// Check WiFi connection, reset if connection is lost
	checkAndResetWifi();
#endif // USE_CC3300
	wdt_reset();
	delayBetweenMeasurements(70); // Wait about a minute between measurements.
}

bool
displayConnectionDetails(void)
{
	uint32_t ipAddress, netmask, gateway, dhcpserv, dnsserv;

	if (!gCc3000.getIPAddress(&ipAddress, &netmask, &gateway, &dhcpserv, &dnsserv))
	{
		Serial.println(F("Can't get IP"));
		return false;
	}
	else
	{
		Serial.print(F("IP: ")); gCc3000.printIPdotsRev(ipAddress);
		Serial.println(F("\nGateway: ")); gCc3000.printIPdotsRev(gateway);
		return true;
	}
}

#define MILLISEC_IN_A_SECOND 1000
// take a little nap between measurements
void
delayBetweenMeasurements(int delaySeconds)
{
	for (int i = 0; i < delaySeconds; i++) {
		wdt_reset();
		delay(MILLISEC_IN_A_SECOND);
	}
}


#define OUTSTRSIZE 256
// send sensor measurements to the webserver
void
postReadingsToWeb(uint32_t ip)
{
	// Build the URL string
	char outStr[OUTSTRSIZE];
	char itoaBuf[12];

	strlcpy(outStr, ("GET /add_data.php?"), OUTSTRSIZE);
	strlcat(outStr, ("sn="), OUTSTRSIZE);
	strlcat(outStr, (gSerialNumber), OUTSTRSIZE);
	strlcat(outStr, ("&"), OUTSTRSIZE);
#if USE_BME
	strlcat(outStr, ("t1="), OUTSTRSIZE);
	itoa(gTemperature, itoaBuf, 10);
	strlcat(outStr, (itoaBuf), OUTSTRSIZE);
	strlcat(outStr, ("&"), OUTSTRSIZE);
	strlcat(outStr, ("hu="), OUTSTRSIZE);
	itoa(gHumidity, itoaBuf, 10);
	strlcat(outStr, (itoaBuf), OUTSTRSIZE);
	strlcat(outStr, ("&"), OUTSTRSIZE);
	strlcat(outStr, ("pr="), OUTSTRSIZE);
	itoa(gPressure, itoaBuf, 10);
	strlcat(outStr, (itoaBuf), OUTSTRSIZE);
#endif // USE_BME
	/* Write DS18B20 results */
#if USE_DS18B20
	for (int j = 0; j < DS_COUNT; j++) {
		strlcat(outStr, ("&t"), OUTSTRSIZE);
		itoa((j+2), itoaBuf, 10);
		strlcat(outStr, (itoaBuf), OUTSTRSIZE);
		strlcat(outStr, ("="), OUTSTRSIZE);
		itoa(gDsTemps[j], itoaBuf, 10);
		strlcat(outStr, (itoaBuf), OUTSTRSIZE);
	}
#endif
	Serial.println(outStr);
	postToWeb(ip, outStr);
}

// Send startup or other message about the station to the webserver
void
postStatusToWeb(uint32_t ip, const char * message)
{
	// Build the URL string
	char outStr[OUTSTRSIZE];
	strlcpy(outStr, ("GET /add_info.php?"), OUTSTRSIZE);
	strlcat(outStr, ("serial="), OUTSTRSIZE);
	strlcat(outStr, (gSerialNumber), OUTSTRSIZE);
	strlcat(outStr, ("&"), OUTSTRSIZE);
	strlcat(outStr, ("message="), OUTSTRSIZE);
	strlcat(outStr, (message), OUTSTRSIZE);
	postToWeb(ip, outStr);
}

/// Connect to webserver and send it the urlString 
void
postToWeb(uint32_t ip, const char * urlString)
{
	Adafruit_CC3000_Client client;
	unsigned int gConnectTimeout = 2000;
	unsigned long gTimeNow;

	digitalWrite(gRedLED, HIGH);
	gTimeNow = millis();
	do {
		Serial.println(F("->connectTCP"));
		client = gCc3000.connectTCP(ip, WEB_PORT);
		Serial.println(F("<-connectTCP"));
		delay(20);
	} while ((!client.connected()) && ((millis() - gTimeNow) < gConnectTimeout));

	if (client.connected())
	{
		//Serial.println(F("Created client interface"));
		Serial.println(urlString);
		Serial.println(F("->")); //  Connected
		client.fastrprint(urlString);
		client.fastrprint(" HTTP/1.1\r\n");
		client.fastrprint("Host: ");
		client.fastrprint(gWebServer);
		client.fastrprint("\r\n");
		//client.fastrprint("Connection: close\r\n");
		client.fastrprint("User-Agent: Arduino Beestation/0.0\r\n");
		client.fastrprintln("\r\n");
		//client.fastrprintln("");
		delay(100);
		while (client.available()) {
			char c = client.read();
			Serial.print(c);
		}
		client.close();
		Serial.println(F("<-")); // Disconnected
	}
	else
	{ // you didn't get a connection to the server:
		Serial.println(F("--> connection failed/n"));
	} 
	digitalWrite(gRedLED, LOW);
}

bool
readSerialNumber(void)
{
	const char hex[] = "0123456789ABCDEF";
	byte i = 0, j = 0;
	boolean present;
	byte data;
	OneWire ds2401(DS2401PIN); // create local 1-Wire instance on DS2401PIN for silicon serial number ds2401

	present = ds2401.reset();
	if (present == TRUE) {
		//Serial.println(F("1-Wire Device present bus"));
		ds2401.write(0x33);  //Send READ data command to 1-Wire bus
		for (i = 0; i <= 7; i++) {
			data = ds2401.read();
			if (i == 7) {
				gSerialNumber[j++] = hex[data / 16];
				gSerialNumber[j++] = hex[data % 16];
			}
		}
		gSerialNumber[j] = 0;
		Serial.println(gSerialNumber);
	}
	else { //Nothing is connected in the bus 
  //Serial.println(F("No 1-Wire Device detected on bus"));
		strlcpy(gSerialNumber, "00", 3);
	}
	//Serial.print(F("Serial Number for this kit: "));
	return present;
}

#if USE_BME
void
measureBme280(void)
{
	float temperature;
	float pressure;
	float humidity;

	temperature = gBme280.readTemperature();
	wdt_reset();
	pressure = gBme280.readPressure() / 100.0F;
	wdt_reset();
	humidity = gBme280.readHumidity();

	gPressure = (int)(pressure + 0.5);
#if MY_DEBUG
	Serial.print(F("Pressure:    "));
	Serial.print(pressure);
	Serial.print(F(" hPa => "));
	Serial.println(gPressure);
#endif // MY_DEBUG

	gTemperature = (int)(temperature + 0.5);
#if MY_DEBUG
	Serial.print(F("Temperature: "));
	Serial.print(temperature);
	Serial.print(F(" C =>"));
	Serial.println(gTemperature);
#endif

	gHumidity = (int)(humidity + 0.5);
#if MY_DEBUG
	Serial.print(F("Humidity: "));
	Serial.print(humidity);
	Serial.print(F(" RH =>"));
	Serial.println(gHumidity);
#endif 
	wdt_reset();
}
#endif // USE_BME

void
setupCc3000(void)
{
	Serial.println(F("CC3000 setup..."));
	/* 5/4/17 hack - seems stable 6/30/17 TCW */
	gCc3000.reboot();
		Serial.println(F("CC3000 reboot initialize"));
		delay(500);
#if 01
if (!gCc3000.begin())
	{
		Serial.println(F("CC3000 failed initialize"));
		do { /* do nothing without reset, let the watchdog bite */ } while (1);
	}
	#endif
// cleanup profiles the CC3000 module
	if (!gCc3000.deleteProfiles())
	{
		Serial.println(F("CC3000 failed to delete old profiles"));
		do { /* do nothing without reset, let the watchdog bite */ } while (1);
	}

	// Connect to  WiFi network
	Serial.println(F("Connecting to WiFi"));
	gCc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY);

	// Check DHCP
	int dhcpCount = 0;
	while (!gCc3000.checkDHCP())
	{
		delay(100);
		Serial.println(F("Trying WiFi again..."));
		if (++dhcpCount > 100) {
			do { /* do nothing without reset, let the watchdog bite */ } while (1);
		}
	}
}

void
checkAndResetWifi(void)
{
	wdt_reset();
	if (!gCc3000.checkConnected() )
	{
		Serial.print(F("Time: "));
#if 0
		gTimeNow = millis();
		//prints time since program started
		Serial.println(gTimeNow);
#endif
		Serial.println(F("Lost WiFi. Rebooting CC3000"));
		gCc3000.disconnect();
		gCc3000.stop();
		wdt_reset();
		gCc3000.reboot(0);
		// setupCc3000() was rarely finishing before the watchdog bites, so let it
		do { /* do nothing without reset, let the watchdog bite */ } while (1);
	}
}

int
calculateFreeRam(void) {
	extern int __heap_start, *__brkval;
	int v;
	return (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
}

// Feed the dog so it doesn't bite
void
resetHardwareWatchdog(void)
{
	pinMode(DD4, OUTPUT);
	delay(200);
	pinMode(DD4, INPUT);
}

// DS 18B20 Temperature sensor array

void
setupDs18B20(void)
{

	gDevices = countDsDevices(gDs);
	Serial.print(F("Found "));
	Serial.print(gDevices);
	Serial.println(F(" devices on bus"));
	for (int i = 0; i < gDevices; i++) {
		getDsAddrByIndex(gDs, gDsAddrs[i], i);
	}
}

int
countDsDevices(OneWire myDs)
{
	byte addr[8];
	int deviceCount = 0;

	myDs.reset();
	myDs.reset_search();
	while (myDs.search(addr)) {
		if (checkDsCrc(myDs, addr)) {
			deviceCount++;
		}
	}
	return deviceCount;
}

bool
checkDsCrc(OneWire myDs, byte addr[8])
{
	return (OneWire::crc8(addr, 7) == addr[7]);
}

bool
getDsAddrByIndex(OneWire myDs, byte firstadd[], int index)
{
	byte i;
	//	byte present = 0;
	byte addr[8];
	int count = 0;

	myDs.reset();
	myDs.reset_search();
	//Serial.print(F("OneWire @ "));
	//Serial.println(index);

	while (myDs.search(addr) && (count <= index)) {
		if ((count == index) && checkDsCrc(myDs, addr))
		{
			//Serial.print(F("Found addr: "));
			//Serial.print(F("0x"));
			for (i = 0; i < 8; i++) {
				firstadd[i] = addr[i];
				//if (addr[i] < 16) {
				//	//Serial.print(F("0"));
				//}
				//Serial.print(addr[i], HEX);
				//if (i < 7) {
				//	Serial.print(F(", "));
				//}
			}
			//Serial.println();
			return true;
		}
		count++;
	}
	return false;
}

// There are 10 DS12B80 sensors connected
// Three are in a linear probe that will go in the hive, above the first brood box, at 2, 6 and 10 inches spacing in a 12 inch tube
// Three more are in another linear probe that will go in the hive, above the second brood box, at 2, 6 and 10 inches spacing in a 12 inch tube
// Three more are in yet another linear probe that will go in the hive, aboce the first honey super, at 2, 6 and 10 inches spacing in a 12 inch tube
// There is one more mounted in the station near the BME280 sensor
void
measureDs18B20(void) {
	for (int i = 0; i < gDevices; i++) {
		int intTemp = getDsTempeature(gDs, gDsAddrs[i]);
#if MY_DEBUG 
		Serial.println(intTemp);
#endif	// MY_DEBUG
 // save some bytes over switch with cascading if's - ha ha
		if (0x67 == gDsAddrs[i][7]) {
			gDsTemps[1] = intTemp;
#if MY_DEBUG
			Serial.print(F("2in temp: "));
#endif
		}
		else if (0x36 == gDsAddrs[i][7]) {
			gDsTemps[2] = intTemp;
#if MY_DEBUG
			Serial.print(F("6in temp: "));
#endif
		}
		else if (0xC0 == gDsAddrs[i][7]) {
			gDsTemps[0] = intTemp;
#if MY_DEBUG
			Serial.print(F("box temp: "));
#endif
		}
		else if (0xB1 == gDsAddrs[i][7]) {
			gDsTemps[3] = intTemp;
#if MY_DEBUG
			Serial.print(F("10in temp: "));
#endif
		}
		else if (0xEC == gDsAddrs[i][7]) {
			gDsTemps[4] = intTemp;
#if MY_DEBUG
			Serial.print(F("2in temp2: "));
#endif
		}
		else if (0xDD == gDsAddrs[i][7]) {
			gDsTemps[5] = intTemp;
#if MY_DEBUG
			Serial.print(F("6in temp2: "));
#endif
		}
		else if (0x4D == gDsAddrs[i][7]) {
			gDsTemps[6] = intTemp;
#if MY_DEBUG
			Serial.print(F("10in temp2: "));
#endif
		}
		else if (0x6F == gDsAddrs[i][7]) {
			gDsTemps[7] = intTemp;
#if MY_DEBUG
			Serial.print(F("2in temp2: "));
#endif
		}
		else if (0x52 == gDsAddrs[i][7]) {
			gDsTemps[8] = intTemp;
#if MY_DEBUG
			Serial.print(F("6in temp2: "));
#endif
		}
		else if (0xAB == gDsAddrs[i][7]) {
			gDsTemps[9] = intTemp;
#if MY_DEBUG
			Serial.print(F("10in temp2: "));
#endif
		}
#if MY_DEBUG
		Serial.println(intTemp);
#endif
	}
}

// Testing shows 10-bit resolution give more than adequate precision and accuracy
int
getDsTempeature(OneWire myDs, byte addr[8])
{
	byte data[12];
	wdt_reset();

	// Get byte for desired resolution
	byte resbyte = 0x3F; // for 10-bit resolution

	// Set configuration
	myDs.reset();
	myDs.select(addr);
	myDs.write(0x4E);         // Write scratchpad
	myDs.write(0);            // TL
	myDs.write(0);            // TH
	myDs.write(resbyte);      // Configuration Register
	myDs.write(0x48);         // Copy Scratchpad
	myDs.reset();
	myDs.select(addr);

	//long starttime = millis();
	myDs.write(0x44, 1);         // start conversion
	while (!myDs.read()) {
		// do nothing
	}
#if MY_DEBUG1
	Serial.print(F("Conversion took: "));
	Serial.print(millis() - starttime);
	Serial.println(F(" ms"));
#endif // MY_DEBUG

	//delay(1000);     // maybe 750ms is enough, maybe not
	// we might do a ds.depower() here, but the reset will take care of it.
	wdt_reset();
	myDs.reset();
	myDs.select(addr);
	myDs.write(0xBE);         // Read Scratchpad


#if MY_DEBUG
	Serial.print(F("Raw Data: "));
#endif // MY_DEBUG
	for (int i = 0; i < 9; i++) {           // we need 9 bytes
		data[i] = myDs.read();
#if MY_DEBUG
		Serial.print(data[i], HEX);
		Serial.print(F(" "));
#endif // MY_DEBUG
	}
#if MY_DEBUG
	Serial.println();
#endif // MY_DEBUG

	// convert the data to actual temperature
	int raw = (data[1] << 8) + data[0];
	// What about negative temps? 
	// look for sign bit per http://stackoverflow.com/questions/30646783/arduino-temperature-sensor-negative-temperatures
	int signBit = raw & 0x8000;  // test most sig bit
	if (signBit) { // negative
		raw = ((raw ^ 0xffff) + 1) * (-1); // 2's comp
	}
	int celsius_100 = (6 * raw) + raw / 4;    // multiply by (100 * 0.0625) or 6.25

	int whole = celsius_100 / 100;  // separate off the whole and fractional portions
	//int fraction = celsius_100 % 100;
#if MY_DEBUG
	Serial.print("Temp (C): ");
#endif // MY_DEBUG
	return whole;
	}

void
printFreeRam(void)
{
	Serial.print(F("Free:"));
	Serial.println(calculateFreeRam());
}

