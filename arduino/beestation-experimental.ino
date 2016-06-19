/* 
*  WiFi beekeeping monitor station with Arduino, the DHT22 temperature and humidity sensor & the CC3000 chip WiFi.
*  Part of the code is based on the work done by Adafruit on the CC3000 chip & the DHT11 sensor
*  Based in part on dht11/cc3300 weatherstation written by Marco Schwartz for Open Home Automation
*  Lots of ideas taken from the many helpful posts in the community online.
* 
*  Purpose: 
*  Sends data to LAMP backend with HTTP GETs to reduce load from AREST which seemed to make the app unstable on Uno
*
*  To Do:
*   More sensors.
*   Better watchdogging - still seems to stop sending to the PHP server with MTF 36 hours
*   Better network recovery code - I saw some ideas online
*   Power consumption improvements
*/

// Include required libraries
#include <Adafruit_CC3000.h> // wifi library
#include <SPI.h> // how to talk to adafruit cc3300 board
#include <DHT.h> // library to read DHT22 sensor
#include <avr/wdt.h>

#include <mycommon/mynetwork.h> // defines network secrets and addresses
// WLAN_SSID
// WLAN_PASS
// WEB_SERVER_IP_COMMAS 
// WEB_SERVER_IP_DOTS 

// WiFi network 
#define WLAN_SECURITY   WLAN_SEC_WPA2

// Define CC3000 chip pins (WiFi board)
#define ADAFRUIT_CC3000_IRQ   3 // MUST be an interrupt pin!
#define ADAFRUIT_CC3000_VBAT  5
#define ADAFRUIT_CC3000_CS    10

// DHT22 sensor pins (temperature/humidy sensor)
#define DHTPIN 7 
#define DHTTYPE DHT22

// Create global CC3000 instance
Adafruit_CC3000 gCc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, 
											ADAFRUIT_CC3000_IRQ, 
											ADAFRUIT_CC3000_VBAT, 
											SPI_CLOCK_DIV2);

// create global DHT instance
DHT gDht(DHTPIN, DHTTYPE);

// PHP server on backend
const char gWebServer[] = WEB_SERVER_IP_DOTS; // IP Address (or name) of server to dump data to 
#define WEB_PORT 80

unsigned long gTimeNow;

// Variables to be exposed 
int gTemperature;
int gHumidity;
int gConnectTimeout;
// Serial Number of this beestation - eventually from DS2401
const char gSerialNumber[] = "007";

//register unsigned char counter asm("r2");

void setup(void)
{ 
 // uint8_t initialMCUSR = counter;
  boolean bResetHappened = (MCUSR & _BV(WDRF));
  wdt_disable();
  // Start Serial
  Serial.begin(9600); // 9600
  /*
  Serial.print("MCUSR: ");
  Serial.println(MCUSR);
  Serial.print("initialMCUSR: ");
  Serial.println(initialMCUSR);
  if (MCUSR & _BV(EXTRF)) {
	  // Reset button or otherwise some software reset
	  Serial.println("Reset button was pressed.");
  }
  if (MCUSR & (_BV(BORF) | _BV(PORF))) {
	  // Brownout or Power On
	  Serial.println("Power loss occured!");
  }
  if (MCUSR & _BV(WDRF)) {
	  //Watchdog Reset
	  Serial.println("Watchdog Reset");
  }
  // Clear all MCUSR registers immediately for 'next use'
  MCUSR = 0;
  */
  Serial.println(F("\n\nInitializing Bee Station"));
  
  // Initialize DHT22 sensor
  gDht.begin();
    
  /*
  // Initialise the CC3000 module
  if (!gCc3000.begin())
  {
    while(1);
  }
  // cleanup profiles the CC3000 module
  if (!gCc3000.deleteProfiles())
  {
    while(1);
  }
  // Connect to  WiFi network
  Serial.println(F("Connecting to WiFi"));
  gCc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY);
  // Check DHCP
  while (!gCc3000.checkDHCP())
  {
    delay(100);
    Serial.println(F("Trying WiFi again..."));   
  }  
  displayConnectionDetails(); 
  */
  ConnectToWlan();
  displayFirmwareVersion(); //
  uint32_t ip = gCc3000.IP2U32(WEB_SERVER_IP_COMMAS);
  Serial.print(F("About to connect to web server at "));
  gCc3000.printIPdotsRev(ip);
  Serial.println();
  if (bResetHappened) {
	  // We were reset by watchdog timer
	  postStatusToWeb(ip, "Restart Bee Monitoring Station (Watchdog Timer Reset)");
	  //MCUCR = 0;        // Clear the flag for next time
	  // Reset the watchdog reset flag
	  bitClear(MCUSR, WDRF);
  }
  else { 
	  // Normal startup
	  postStatusToWeb(ip, "Start Bee Monitoring Station");
  }
  Serial.println(F("Looping"));
  Serial.print(F("Time: "));
  gTimeNow = millis();
  //prints time since program started
  Serial.println(gTimeNow);
  gConnectTimeout=1000;
  wdt_enable(WDTO_8S); // options: WDTO_1S, WDTO_2S, WDTO_4S, WDTO_8S
}

void loop(void)
{
  wdt_reset();
  
  delay(2);
  // Measure from DHT
  gTemperature = gDht.readTemperature();
  gHumidity = gDht.readHumidity();
  // Check if any reads failed and exit early (to try again).
  if (isnan(gTemperature) || isnan(gHumidity) ) {
   Serial.print(F("Time: "));
   gTimeNow = millis();
   //prints time since program started
   Serial.println(gTimeNow);
   Serial.println(F("Cannot read DHT sensor!"));
    return;
  }  
  //wdt_reset();  

  Serial.print(F("H: "));
  Serial.print(gHumidity);
  Serial.print(F(" %\t"));
  Serial.print(F("T: "));
  Serial.print(gTemperature);
  Serial.println(F(" *C "));

  wdt_reset();
  //gCc3000.stop(); // just for testing the following
  // Check WiFi connection, reset if connection is lost
  checkAndResetWifi();

  uint32_t ip = gCc3000.IP2U32(WEB_SERVER_IP_COMMAS);
  Serial.print(F("About to connect to web server "));
  gCc3000.printIPdotsRev(ip);
  Serial.println();
  // Optional: Do a ping test on the website
  //Serial.print(F("\n\rPinging ")); cc3000.printIPdotsRev(ip); Serial.print(F(" ... "));
  //uint16_t replies = cc3000.ping(ip, 5);
  //Serial.print(replies); Serial.println(F(" replies"));

  postReadingsToWeb(ip, gTemperature, gHumidity);



  wdt_reset();  
  delayBetweenMeasurements(20); // Wait a few seconds between measurements.
}

bool 
displayConnectionDetails(void)
{
  uint32_t ipAddress, netmask, gateway, dhcpserv, dnsserv;
  
  if(!gCc3000.getIPAddress(&ipAddress, &netmask, &gateway, &dhcpserv, &dnsserv))
  {
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
void
delayBetweenMeasurements(int delaySeconds)
{
	for (int i = 0; i < delaySeconds; i++) {
		wdt_reset();
		delay(ONETHOUSAND);
	}
}


#define OUTSTRSIZE 256
void
postReadingsToWeb(uint32_t ip, int temperature, int humidity)
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
	//Serial.println(outStr);
	postToWeb(ip, outStr);
}


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

void
postToWeb(uint32_t ip, const char * urlString)
{
	Adafruit_CC3000_Client client;
	gTimeNow = millis();
	do {
		client = gCc3000.connectTCP(ip, WEB_PORT);
	} while ((!client.connected()) && ((millis() - gTimeNow) < gConnectTimeout));

	Serial.println(F("Created client interface to web server"));
	Serial.println(urlString);
	if (client.connected())
	{
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
		Serial.println(F("<- Disonnected"));
		// note the time that the connection was made:
		Serial.print(F("Time: "));
		gTimeNow = millis();
		//prints time since program started
		Serial.println(gTimeNow);
	}
	else
	{ // you didn't get a connection to the server:
		Serial.println(F("--> connection failed/n"));
	}
}


void
checkAndResetWifi(void) {
	/*
	if (!gCc3000.checkConnected())
	{
		Serial.print(F("Time: "));
		gTimeNow = millis();
		//prints time since program started
		Serial.println(gTimeNow);
		Serial.println(F("Lost WiFi"));
		while (1) {}
	}
	*/
	wdt_disable();  
	if ((!gCc3000.checkConnected()) || (!gCc3000.checkDHCP())) {
		//  we don't have something the code needs, so go get it
		Serial.print(F("Lost WiFi at time: "));
		gTimeNow = millis();
		Serial.println(gTimeNow);
		Serial.println(F("Lost WiFi: stopping CC3300"));
		gCc3000.stop();
		Serial.println(F("Lost WiFi: rebooting CC3300"));
		gCc3000.reboot();
		ConnectToWlan();
		uint32_t ip = gCc3000.IP2U32(WEB_SERVER_IP_COMMAS);
		//Serial.print(F("About to connect to web server at "));
		gCc3000.printIPdotsRev(ip);
		Serial.println();
		postStatusToWeb(ip, "Restart Bee Monitoring Station (Lost WLAN)");
	}
	wdt_enable(WDTO_8S);
}

void
ConnectToWlan(void)
{
	if (!gCc3000.begin()) {
		while (1);
	}
	// cleanup profiles the CC3000 module
	if (!gCc3000.deleteProfiles())
	{
		while (1);
	}
	// Connect to  WiFi network
	gCc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY);
	// Check DHCP
	while (!gCc3000.checkDHCP())
	{
		delay(100);
		Serial.println(F("Trying WiFi for DHCP again..."));
	}
	displayConnectionDetails();
}

void displayFirmwareVersion(void)
{
#ifndef CC3000_TINY_DRIVER
	uint8_t major, minor;

	if (!gCc3000.getFirmwareVersion(&major, &minor))
	{
		Serial.println(F("Unable to retrieve the firmware version!\r\n"));
	}
	else
	{
		Serial.print(F("Firmware V. : "));
		Serial.print(major); Serial.print(F(".")); Serial.println(minor);
	}
#endif
}
