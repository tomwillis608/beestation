/*
*  WiFi beekeeping monitor station with Arduino,
*  BME 280 temperature-pressure-humidity sensor,
*  DS2401 silicon serial number,
*  & the CC3000 chip WiFi.
*
*  Part of the code is based on the work done by Adafruit on the CC3000 chip & the DHT11 sensor
*  Based in part on dht11/cc3300 weatherstation written by Marco Schwartz for Open Home Automation
*  Lots of ideas taken from the many helpful posts in the community online.
*
*  Purpose:
*   Sends data to LAMP backend with HTTP GETs
*   Be stable over all conditions
*
*  To Do:
*   More sensors.
*   Better watchdogging - still seems to stop sending to the PHP server with MTF 36 hours
*   Better network recovery code - I saw some ideas online
*   Powersaving by sleeping instead of waiting
*
*/

#define USE_DHT 0
#define USE_CC3300 01
#define USE_DS2401 1
#define USE_BMP 0
#define USE_BME 1
#define MY_DEBUG 1

// Include required libraries
#include <Adafruit_CC3000.h> // wifi library
#include <SPI.h> // how to talk to adafruit cc3300 board
#include <OneWire.h> // library to read DS 1-Wire sensors, like DS2401 and DS18B20
#if USE_BME 
// Using I2C protocol
//#include <Wire.h>    // imports the wire library for talking over I2C to the BMP180
//#include <Adafruit_Sensor.h>  // generic sensor library
// ^^ included by below
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

// Create global CC3000 instance
Adafruit_CC3000 gCc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS,
	ADAFRUIT_CC3000_IRQ,
	ADAFRUIT_CC3000_VBAT,
	SPI_CLOCK_DIV2); 

#if USE_BME
Adafruit_BME280 gBme280; // I2C
#endif // USE_BME

// PHP server on backend
const char gWebServer[] = WEB_SERVER_IP_DOTS; // IP Address (or name) of server to dump data to 
#define WEB_PORT 80

//unsigned long gTimeNow;
unsigned long gCycles = 0;

// Variables to be exposed 
int gHumidity = 0;
int gPressure = 0;
int gTemperature = 0;

// Serial Number of this beestation 
char gSerialNumber[20]; // read from Maxim DS2401 on pin 9

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
	Serial.println(F("\n\nInitializing Bee Station"));
	readSerialNumber();

#if USE_BME
	// Initialize BMP180 sensor
	if (!gBme280.begin())
	{
		Serial.print(F("Cannot read BME280 sensor!"));
		while (1);
	}
#endif // USE_BME

#if USE_CC3300
	// Initialise the CC3000 module
	//gCc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS,
	//	ADAFRUIT_CC3000_IRQ,
	//	ADAFRUIT_CC3000_VBAT,
	//	SPI_CLOCK_DIV2);
	setupCc3000();
	displayConnectionDetails();
#endif // USE_CC3300
	//digitalWrite(gRedLED, LOW);
	delay(2000);

#if USE_CC3300
	uint32_t ip = gCc3000.IP2U32(WEB_SERVER_IP_COMMAS);
	Serial.print(F("About to connect to web server at "));
	gCc3000.printIPdotsRev(ip);
	Serial.println();
	postStatusToWeb(ip, "Start Bee Monitoring Station");
	Serial.println(F("Looping"));
	//Serial.print(F("Time: "));
	//gTimeNow = millis();
	//prints time since program started
	//Serial.println(gTimeNow);
#endif
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
	Serial.print(F("Free RAM:"));
	Serial.println(calculateFreeRam());

#if USE_BME
	// Measure from BMP180 
	measureBme280();
	wdt_reset();
#endif

#if USE_CC3300
	uint32_t ip = gCc3000.IP2U32(WEB_SERVER_IP_COMMAS);
	Serial.print("About to connect to web server ");
	gCc3000.printIPdotsRev(ip);
	Serial.println();
	// Optional: Do a ping test on the website
	//Serial.print(F("\n\rPinging ")); cc3000.printIPdotsRev(ip); Serial.print(F(" ... "));
	//uint16_t replies = cc3000.ping(ip, 5);
	//Serial.print(replies); Serial.println(F(" replies"));
	wdt_reset();
	postReadingsToWeb(ip, gTemperature, gHumidity, gPressure);
	wdt_reset();
	// Check WiFi connection, reset if connection is lost
	checkAndResetWifi();
#endif // USE_CC3300
	wdt_reset();
	delayBetweenMeasurements(50); // Wait a few seconds between measurements.
}

bool
displayConnectionDetails(void)
{
	uint32_t ipAddress, netmask, gateway, dhcpserv, dnsserv;

	if (!gCc3000.getIPAddress(&ipAddress, &netmask, &gateway, &dhcpserv, &dnsserv))
	{
		Serial.println(F("Can't get IP addresses"));
		return false;
	}
	else
	{
		Serial.println(F("\nIP Addr: ")); gCc3000.printIPdotsRev(ipAddress);
		Serial.print(F("\nNetmask: ")); gCc3000.printIPdotsRev(netmask);
		Serial.print(F("\nGateway: ")); gCc3000.printIPdotsRev(gateway);
		Serial.print(F("\nDHCPsrv: ")); gCc3000.printIPdotsRev(dhcpserv);
		Serial.print(F("\nDNSserv: ")); gCc3000.printIPdotsRev(dnsserv);
		Serial.println("");
		return true;
	}
}

#define ONETHOUSAND 1000
// take a little nap between measurements
void
delayBetweenMeasurements(int delaySeconds)
{
	for (int i = 0; i < delaySeconds; i++) {
		wdt_reset();
		delay(ONETHOUSAND);
	}
}


#define OUTSTRSIZE 256
// send sensor measurements to the webserver
void
postReadingsToWeb(uint32_t ip, int temperature, int humidity, int pressure)
{
	// Build the URL string
	char outStr[OUTSTRSIZE];
	char itoaBuf[12];

	strlcpy(outStr, ("GET /add_data.php?"), OUTSTRSIZE);
	strlcat(outStr, ("serial="), OUTSTRSIZE);
	strlcat(outStr, (gSerialNumber), OUTSTRSIZE);
	strlcat(outStr, ("&"), OUTSTRSIZE);
	strlcat(outStr, ("temperature="), OUTSTRSIZE);
	itoa(temperature, itoaBuf, 10);
	strlcat(outStr, (itoaBuf), OUTSTRSIZE);
	strlcat(outStr, ("&"), OUTSTRSIZE);
	strlcat(outStr, ("humidity="), OUTSTRSIZE);
	itoa(humidity, itoaBuf, 10);
	strlcat(outStr, (itoaBuf), OUTSTRSIZE);
	strlcat(outStr, ("&"), OUTSTRSIZE);
	strlcat(outStr, ("pressure="), OUTSTRSIZE);
	itoa(pressure, itoaBuf, 10);
	strlcat(outStr, (itoaBuf), OUTSTRSIZE);
	//Serial.println(outStr);
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
	//Serial.println(outStr);
	postToWeb(ip, outStr);
}

/// Connect to webserver and send it the urlString 
void
postToWeb(uint32_t ip, const char * urlString)
{
	Adafruit_CC3000_Client client;
	int gConnectTimeout = 2000;
	unsigned long gTimeNow;

	//cycleLed(gLED);
	digitalWrite(gRedLED, HIGH);
	//Serial.println(F("postToweb"));
	gTimeNow = millis();
	do {
		client = gCc3000.connectTCP(ip, WEB_PORT);
		Serial.println(F("Called connectTCP"));
		delay(20);
	} while ((!client.connected()) && ((millis() - gTimeNow) < gConnectTimeout));

	if (client.connected())
	{
		Serial.println(F("Created client interface to web server"));
		Serial.println(urlString);
		Serial.println(F("-> Connected"));
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
		Serial.println(F("<- Disconnected"));
		// note the time that the connection was made:
		//Serial.print(F("Time: "));
		//gTimeNow = millis();
		//prints time since program started
		//Serial.println(gTimeNow);
	}
	else
	{ // you didn't get a connection to the server:
		Serial.println(F("--> connection failed/n"));
	}
	//cycleLed(gLED); 
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
			gSerialNumber[j++] = hex[data / 16];
			gSerialNumber[j++] = hex[data % 16];
		}
		gSerialNumber[j] = 0;
		Serial.println(gSerialNumber);
	}
	else { //Nothing is connected in the bus 
  //Serial.println(F("No 1-Wire Device detected on bus"));
		strlcpy(gSerialNumber, hex, 16);
	}
	//Serial.print(F("Serial Number for this kit: "));
	return present;
}

#if USE_BME
void
measureBme280(void)
{
	/* Get a new sensor event */
	//sensors_event_t event;
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
	bool result = FALSE;
	Serial.println(F("CC3000 setup..."));
	if (!gCc3000.begin())
	{
		Serial.println(F("CC3000 failed to initialize"));
		do { /* do nothing without reset, let the watchdog bite */ } while (1);
	}
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
	unsigned long gTimeNow;

	wdt_reset();
	if (!gCc3000.checkConnected() /* || (gCycles>5)*/ )
	{
		Serial.print(F("Time: "));
		gTimeNow = millis();
		//prints time since program started
		Serial.println(gTimeNow);
		Serial.println(F("Lost WiFi. Rebooting CC3300"));
		gCc3000.disconnect();
		gCc3000.stop();
		/*
		gCc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS,
			ADAFRUIT_CC3000_IRQ,
			ADAFRUIT_CC3000_VBAT,
			SPI_CLOCK_DIV2);
		delay(20);
		*/
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

void
cycleLed(int pin)
{
	uint8_t mode = LOW;

	mode = (mode == HIGH) ? LOW : HIGH;
	digitalWrite(pin, mode);
}

// Feed the dog so it doesn't bite
void
resetHardwareWatchdog(void)
{
	pinMode(DD4, OUTPUT);
	delay(200);
	pinMode(DD4, INPUT);
}