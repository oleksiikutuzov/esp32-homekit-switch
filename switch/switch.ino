/*********************************************************************************
 *  MIT License
 *
 *  Copyright (c) 2022 Gregg E. Berman
 *
 *  https://github.com/HomeSpan/HomeSpan
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 *
 ********************************************************************************/

/*
 *                ESP-WROOM-32 Utilized pins
 *              ╔═════════════════════════════╗
 *              ║┌─┬─┐  ┌──┐  ┌─┐             ║
 *              ║│ | └──┘  └──┘ |             ║
 *              ║│ |            |             ║
 *              ╠═════════════════════════════╣
 *          +++ ║GND                       GND║ +++
 *          +++ ║3.3V                     IO23║ USED_FOR_NOTHING
 *              ║                         IO22║
 *              ║IO36                      IO1║ TX
 *              ║IO39                      IO3║ RX
 *              ║IO34                     IO21║
 *              ║IO35                         ║ NC
 *      RED_LED ║IO32                     IO19║
 *              ║IO33                     IO18║ SWITCH
 *              ║IO25                      IO5║
 *              ║IO26                     IO17║
 *              ║IO27                     IO16║
 *              ║IO14                      IO4║
 *              ║IO12                      IO0║ +++, BUTTON
 *              ╚═════════════════════════════╝
 */

#define REQUIRED VERSION(1, 5, 0)

#include "HomeSpan.h"
#include "extras/Pixel.h" // include the HomeSpan Pixel class
#include <WiFiClient.h>
#include <WebServer.h>
#include <ElegantOTA.h>

char sNumber[18] = "11:11:11:11:11:11";

void addSwitch();

///////////////////////////////
WebServer server(80);

struct DEV_Switch : Service::Switch {

	int					ledPin; // relay pin
	SpanCharacteristic *power;

	// Constructor
	DEV_Switch(int ledPin) : Service::Switch() {
		power		 = new Characteristic::On(0, true);
		this->ledPin = ledPin;
		pinMode(ledPin, OUTPUT);

		digitalWrite(ledPin, power->getVal());
	}

	// Override update method
	boolean update() {
		digitalWrite(ledPin, power->getNewVal());

		return (true);
	}
};

///////////////////////////////

void setup() {

	Serial.begin(115200);

	for (int i = 0; i < 17; ++i) // we will iterate through each character in WiFi.macAddress() and copy it to the global char sNumber[]
	{
		sNumber[i] = WiFi.macAddress()[i];
	}
	sNumber[17] = '\0'; // the last charater needs to be a null

	homeSpan.setLogLevel(0);										// set log level to 0 (no logs)
	homeSpan.setStatusPin(32);										// set the status pin to GPIO32
	homeSpan.setStatusAutoOff(10);									// disable led after 10 seconds
	homeSpan.setWifiCallback(setupWeb);								// Set the callback function for wifi events
	homeSpan.reserveSocketConnections(5);							// reserve 5 socket connections for Web Server
	homeSpan.setControlPin(0);										// set the control pin to GPIO0
	homeSpan.setPortNum(88);										// set the port number to 81
	homeSpan.enableAutoStartAP();									// enable auto start of AP
	homeSpan.enableWebLog(10, "pool.ntp.org", "UTC-3:00", "myLog"); // enable Web Log

	homeSpan.begin(Category::ProgrammableSwitches, "Switch");

	new SpanAccessory();
	new Service::AccessoryInformation();
	new Characteristic::Name("Switch");
	new Characteristic::Manufacturer("HomeSpan");
	new Characteristic::SerialNumber(sNumber);
	new Characteristic::Model("Programmable Switch");
	new Characteristic::FirmwareRevision("1.0");
	new Characteristic::Identify();

	new Service::HAPProtocolInformation();
	new Characteristic::Version("1.1.0");

	new DEV_Switch(18);
}

///////////////////////////////

void loop() {
	homeSpan.poll();
	server.handleClient();
}

///////////////////////////////

void setupWeb() {

	server.on("/metrics", HTTP_GET, []() {
		double uptime		= esp_timer_get_time() / (6 * 10e6);
		double heap			= esp_get_free_heap_size();
		String uptimeMetric = "# HELP uptime LED Strip uptime\nhomekit_uptime{device=\"led_strip\",location=\"home\"} " + String(int(uptime));
		String heapMetric	= "# HELP heap Available heap memory\nhomekit_heap{device=\"led_strip\",location=\"home\"} " + String(int(heap));

		Serial.println(uptimeMetric);
		Serial.println(heapMetric);
		server.send(200, "text/plain", uptimeMetric + "\n" + heapMetric);
	});

	server.on("/reboot", HTTP_GET, []() {
		String content = "<html><body>Rebooting!  Will return to configuration page in 10 seconds.<br><br>";
		content += "<meta http-equiv = \"refresh\" content = \"10; url = /\" />";
		server.send(200, "text/html", content);

		ESP.restart();
	});

	ElegantOTA.begin(&server); // Start ElegantOTA
	server.begin();
	LOG1("HTTP server started");
} // setupWeb