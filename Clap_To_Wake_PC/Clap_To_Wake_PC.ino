#include <enc28j60.h>
#include <EtherCard.h>
#include <net.h>
#include <SoftwareSerial.h>

//Define Arduino pin allocation
#define sensorMotion 2
#define sensorMic 3

//Ethernet related parameters
static uint8_t shieldMAC[] = { 0x74, 0x69, 0x69, 0x2D, 0x30, 0x31 }; //Ethernet shield MAC address
static uint8_t computerMAC[] = { 0x34, 0x97, 0xF6, 0x37, 0x5E, 0x71 }; //MAC Address of destination computer
static uint8_t computerMACRev[] = { 0x71, 0x5E, 0x37, 0xF6, 0x97, 0x34 }; //MAC Address of destination computer in reverse order for sleep on LAN
uint8_t Ethernet::buffer[700]; //TCP/IP send and receive buffer
static int controlPin = 10;

//Variables for clap detection
int lastSoundValue;
int soundValue;
long lastNoiseTime = 0;
long currentNoiseTime = 0;
long lastStateChange = 0;

void setup()
{
	//Set pins
	pinMode(sensorMotion, INPUT);
	pinMode(sensorMic, INPUT);
	Serial.begin(9600);
	if (ether.begin(sizeof Ethernet::buffer, shieldMAC, controlPin) == 0) {
		Serial.println("Failed to access Ethernet controller");
	}
	if (!ether.dhcpSetup()) {
		Serial.println("DHCP failed");
	}
	ether.parseIp(ether.hisip, "10.11.12.1");
}

void loop()
{
	soundValue = digitalRead(sensorMic);
	currentNoiseTime = millis();

	if (soundValue == 1)
	{ // if there is currently a noise
		if (
			(currentNoiseTime > lastNoiseTime + 100) && //To debounce a sound occurring in more than a loop cycle as a single noise
			(lastSoundValue == 0) &&  //If it was silent before
			(currentNoiseTime < lastNoiseTime + 800) && //If current clap is less than 0.8 seconds after the first clap
			(currentNoiseTime > lastStateChange + 1000) //To avoid taking a third clap as part of a pattern
			)
		{
			if (!HostAlive())
			{
				Serial.println("Waking");
				ether.sendWol(computerMAC);
			}
			else
			{
				Serial.println("Sleeping");
				ether.sendWol(computerMACRev);
			}
			lastStateChange = currentNoiseTime;
		}
		lastNoiseTime = currentNoiseTime;
	}
	lastSoundValue = soundValue;
}

bool HostAlive()
{
	uint32_t timeout = millis() + 4000;
	ether.clientIcmpRequest(ether.hisip);
	while (millis() <= timeout)
	{
		word len = ether.packetReceive(); // go receive new packets
		word pos = ether.packetLoop(len); // respond to incoming pings

		if (len > 0 && ether.packetLoopIcmpCheckReply(ether.hisip))
		{
			return true;
		}
	}
	return false;
}
