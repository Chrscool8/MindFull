// https://i.imgur.com/W7Aqzo5.png


#define RX_PIN 32
#define TX_PIN 26

#include <WiFi.h>
#include <M5Atom.h>
#include <PubSubClient.h>

#include "NetworkInfo.h"
// const char* ssid = "SSID";
// const char* password = "PASSWORD";
// const char* mqtt_server = "1.1.1.1";

WiFiClient espClient;
PubSubClient client(espClient);
WiFiServer server(80);

#define LED 2  //On board LED

void reconnect()
{
	// Loop until we're reconnected
	while (!client.connected()) {
		Serial.print("Attempting MQTT connection...");
		// Attempt to connect
		if (client.connect("ESP8266Client")) {
			Serial.println("connected");
			// Subscribe
			client.subscribe("esp32/output");
		}
		else {
			Serial.print("failed, rc=");
			Serial.print(client.state());
			Serial.println(" try again in 5 seconds");
			// Wait 5 seconds before retrying
			delay(5000);
		}
	}
}

int led_count = 0;

void setup()
{
	M5.begin(true, false, true);
	delay(10);

	for (int i = 0; i < 25; i++)
		M5.dis.drawpix(i, CRGB(30, 30, 0));

	Serial.print("Connecting to ");
	Serial.println(ssid);
	WiFi.begin(ssid, password);

	while (WiFi.status() != WL_CONNECTED)
	{
		//M5.dis.drawpix(led_count++ % 25, CRGB(rand() % 255, rand() % 255, rand() % 255));
		//for (int i = 0; i < 25; i++)
		//	M5.dis.drawpix(i, CRGB(30, 0, 30));
		//M5.dis.drawpix(WiFi.status(), CRGB(rand() % 255, rand() % 255, rand() % 255));

		//Serial.println(WiFi.status());
		delay(500);
		//Serial.print(".");
	}

	Serial.println("");
	Serial.println("WiFi connected.");
	Serial.println("IP address: ");
	Serial.println(WiFi.localIP());

	for (int i = 0; i < 25; i++)
		M5.dis.drawpix(i, CRGB(0, 30, 30));

	server.begin();
	delay(1000);
	client.setServer(mqtt_server, 1883);

	Serial.println("Start");

	Serial2.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);  //Serial port 2 initialization
}

int data_count = 0;
int data[1024];

int previous_serial = 0;

bool synced = false;
int packet_size = 0;

unsigned char POOR_SIGNAL;
unsigned char HEART_RATE;
unsigned char ATTENTION;
unsigned char MEDITATION;
unsigned char EIGHTBIT_RAW;
unsigned char RAW_MARKER;

long waves_delta;
long waves_theta;
long waves_low_alpha;
long waves_high_alpha;
long waves_low_beta;
long waves_high_beta;
long waves_low_gamma;
long waves_mid_gamma;

long raw_eeg = 0;

long three_bytes(int starting_at)
{
	long triple = 0;

	triple |= data[starting_at + 0] << 16;
	triple |= data[starting_at + 1] << 8;
	triple |= data[starting_at + 2];

	return triple;
}

long two_bytes(int starting_at)
{
	long dbl = 0;

	dbl |= data[starting_at + 0] << 8;
	dbl |= data[starting_at + 1];

	return dbl;
}

void parse_data()
{
	int cursor = 0;

	while (cursor < packet_size)
	{
		unsigned char current = data[cursor];
		cursor += 1;

		if (current < 0x80)
		{
			switch (current)
			{
				// Single Byte: https://i.imgur.com/cJVMyok.png
			case 0x02:
				POOR_SIGNAL = data[cursor];
				client.publish("mqtt/MindFlex/data/POOR_SIGNAL", (String(POOR_SIGNAL)).c_str());
				cursor += 1;
				break;
			case 0x03:
				HEART_RATE = data[cursor];
				client.publish("mqtt/MindFlex/data/HEART_RATE", String(HEART_RATE).c_str());
				cursor += 1;
				break;
			case 0x04:
				ATTENTION = data[cursor];
				client.publish("mqtt/MindFlex/data/ATTENTION", String(ATTENTION).c_str());
				cursor += 1;
				break;
			case 0x05:
				MEDITATION = data[cursor];
				client.publish("mqtt/MindFlex/data/MEDITATION", String(MEDITATION).c_str());
				cursor += 1;
				break;
			case 0x06:
				EIGHTBIT_RAW = data[cursor];
				cursor += 1;
				break;
			case 0x07:
				RAW_MARKER = data[cursor];
				cursor += 1;
				break;
			default:
				Serial.print("Weird: 0x");
				Serial.println(current, HEX);
				cursor += 1;
			}
		}
		else
		{
			switch (current)
			{
				// Multi-Byte: https://i.imgur.com/I1PxzvS.png
			case 0x80:
				Serial.println("Raw EEG?");
				raw_eeg = two_bytes(cursor);
				cursor += 2;
				break;
			case 0x81:
				cursor += 32;
				break;
			case 0x82:
				Serial.println("Crash message!?");
				cursor += 1;
				break;
			case 0x83:
			{
				int length = data[cursor];
				cursor += 1;

				// read 3 byte

				// Delta
				waves_delta = three_bytes(cursor);
				client.publish("mqtt/MindFlex/data/waves_delta", String(waves_delta).c_str());
				cursor += 3;
				// Theta
				waves_theta = three_bytes(cursor);
				client.publish("mqtt/MindFlex/data/waves_theta", String(waves_theta).c_str());
				cursor += 3;
				// Low-Alpha
				waves_low_alpha = three_bytes(cursor);
				client.publish("mqtt/MindFlex/data/waves_low_alpha", String(waves_low_alpha).c_str());
				cursor += 3;
				// High-alpha
				waves_high_alpha = three_bytes(cursor);
				client.publish("mqtt/MindFlex/data/waves_high_alpha", String(waves_high_alpha).c_str());
				cursor += 3;
				// Low-beta
				waves_low_beta = three_bytes(cursor);
				client.publish("mqtt/MindFlex/data/waves_low_beta", String(waves_low_beta).c_str());
				cursor += 3;
				// High beta
				waves_high_beta = three_bytes(cursor);
				client.publish("mqtt/MindFlex/data/waves_high_beta", String(waves_high_beta).c_str());
				cursor += 3;
				// low gamma
				waves_low_gamma = three_bytes(cursor);
				client.publish("mqtt/MindFlex/data/waves_low_gamma", String(waves_low_gamma).c_str());
				cursor += 3;
				// mid gamma
				waves_mid_gamma = three_bytes(cursor);
				client.publish("mqtt/MindFlex/data/waves_mid_gamma", String(waves_mid_gamma).c_str());
				cursor += 3;

				break;
			}
			case 0x86:
				cursor += 2;
				break;

			default:
				int length = data[cursor];
				cursor += length;

				Serial.print("Weird: 0x");
				Serial.println(current, HEX);
				//cursor++;
			}
		}
	}

	// {@Plot.MyPlotWindow.waves_delta waves_delta}{@Plot.MyPlotWindow.waves_theta waves_theta}{@Plot.MyPlotWindow.waves_low_alpha waves_low_alpha}{@Plot.MyPlotWindow.waves_high_alpha waves_high_alpha}{@Plot.MyPlotWindow.waves_low_beta waves_low_beta}{@Plot.MyPlotWindow.waves_high_beta waves_high_beta}{@Plot.MyPlotWindow.waves_low_gamma waves_low_gamma}{@Plot.MyPlotWindow.waves_mid_gamma waves_mid_gamma}

	Serial.println("---------");

	Serial.print("POOR_SIGNAL: ");
	Serial.print(POOR_SIGNAL);
	Serial.println("/255");

	Serial.print("HEART_RATE: ");
	Serial.print(HEART_RATE);
	Serial.println("/255");

	Serial.print("ATTENTION: ");
	Serial.print(ATTENTION);
	Serial.println("/100");

	Serial.print("MEDITATION: ");
	Serial.print(MEDITATION);
	Serial.println("/100");

	Serial.print("EIGHTBIT_RAW: ");
	Serial.print(EIGHTBIT_RAW);
	Serial.println("/255");

	Serial.print("RAW_MARKER: ");
	Serial.println(RAW_MARKER);

	Serial.println("---------");
}

bool use_mode_2 = false;
bool activated_mode_2 = false;

void loop()
{
	if (!client.connected()) {
		reconnect();
	}
	client.loop();

	if (!synced)
	{
		if (Serial2.available())
		{
			//If the serial port has data to read and print
			int c = Serial2.read();

			if (c == 170 && previous_serial == 170) // found the sync
			{
				Serial.println("Sync!");
				synced = true;
				data_count = -1;
			}
			else
			{
				Serial.print("No Sync: ");
				Serial.println(c);
			}

			previous_serial = c;
		}
	}
	else
	{
		//previous_serial = -1;

		if (Serial2.available())
		{
			//If the serial port has data to read and print
			int c = Serial2.read();
			previous_serial = c;

			if (data_count == -1)
			{
				Serial.print("Size: 0x");
				Serial.println(c, HEX);

				packet_size = c;
				data_count += 1;
			}
			else
			{
				if (data_count <= packet_size)
				{
					data[data_count] = c;
					data_count++;
				}
				else
				{
					int checksum = 0;

					Serial.println("Got all data");
					for (int i = 0; i <= packet_size; i++)
					{
						Serial.print("0x");
						Serial.print(data[i], HEX);
						//Serial.print(data[i]);
						Serial.print("\t");

						if (i != packet_size)
							checksum += data[i];
					}
					Serial.println("");

					checksum &= 0xFF;
					checksum = ~checksum & 0xFF;

					Serial.print("Checksum: 0x");
					Serial.println(checksum, HEX);

					if (checksum == data[packet_size])
					{
						Serial.println("Checksum OK!");
						parse_data();
					}
					else
					{
						Serial.println("Checksum NOT OK!!");
					}

					synced = false;

					if (use_mode_2 && !activated_mode_2)
					{                  //If button A is pressed
						Serial2.print(2); //Send data through serial port 2
						sleep(1);
						Serial.println("New Start");
						//Serial2.flush(); // wait for last transmitted data to be sent
						Serial2.begin(57600, SERIAL_8N1, RX_PIN, TX_PIN);
						//while (Serial2.available()) Serial.println(Serial2.read());

						activated_mode_2 = true;
						previous_serial = -1;
					}
				}
			}
		}
	}

	if (M5.Btn.wasPressed())
	{
		use_mode_2 = true;
	}

	M5.update();
}
