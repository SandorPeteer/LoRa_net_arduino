// include the library
#include <RadioLib.h>
#include "U8glib.h"
U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_DEV_0 | U8G_I2C_OPT_NO_ACK | U8G_I2C_OPT_FAST); // Fast I2C / TWI

#define CLOCK 7 //D-
#define DATA 6  //D+

const char keymap[] = {
  0, 0,  0,  0,  0,  0,  0,  0,
  0, 0,  0,  0,  0,  0, '`', 0,
  0, 0 , 0 , 0,  0, 'q', '1', 0,
  0, 0, 'y', 's', 'a', 'w', '2', 0,
  0, 'c', 'x', 'd', 'e', '4', '3', 0,
  0, ' ', 'v', 'f', 't', 'r', '5', 0,
  0, 'n', 'b', 'h', 'g', 'z', '6', 0,
  0, 0, 'm', 'j', 'u', '7', '8', 0,
  0, '?', 'k', 'i', 'o', '0', '9', 0,
  0, '.', '/', 'l', ';', 'p', '-', 0,
  0, 0, '\'', 0, '[', '=', 0, 0,
  0, 0, 13, ']', 0, '\\', 0, 0,
  0, 0, 0, 0, 0, 0, 127, 0,
  0, '1', 0, '4', '7', 0, 0, 0,
  '0', '.', '2', '5', '6', '8', 0, 0,
  0, '+', '3', '-', '*', '9', 0, 0,
  0, 0, 0, 0
};

uint8_t lastscan;
String str;
bool kuld = false;


ISR(PCINT2_vect)
{
  uint16_t scanval = 0;
  for (int i = 0; i < 11; i++)
  {
    while (digitalRead(CLOCK));
    scanval |= digitalRead(DATA) << i;
    while (!digitalRead(CLOCK));
  }
  scanval >>= 1;
  scanval &= 0xFF;
  //Serial.println(scanval, DEC);
  if (lastscan != 0xF0 && scanval != 0xF0)
    switch (scanval)
    {
      case 0x5A: //Enter
        //Serial.println(">");
        u8g.print(":");
        kuld = true;
        break;
      case 102: //del
        str = "";
        break;
      default:
        str += keymap[scanval];
        //Serial.print(keymap[scanval]);
        u8g.print(keymap[scanval]);
    }
  lastscan = scanval;
  bitSet(PCIFR, PCIF2);
}

// SX1278 has the following connections:
// NSS pin:   10
// DIO0 pin:  2
// RESET pin: 9
// DIO1 pin:  3
SX1278 radio = new Module(10, 2, 9, 3);

// save transmission state between loops
int transmissionState = RADIOLIB_ERR_NONE;

void setup() {
  //Serial.begin(115200);

  pinMode(CLOCK, INPUT_PULLUP); //For most keyboards the builtin pullups are sufficient, so the 10k pullups can be omitted
  pinMode(DATA, INPUT_PULLUP);
  bitSet(PCICR, PCIE2); // Enable pin change interrupts on pin D0-D7
  bitSet(PCMSK2, CLOCK); // Pin change interrupt on Clock pin


  // initialize SX1278 with default settings
  // Serial.print("Initializing ... ");
  int state = radio.begin();
  if (state == RADIOLIB_ERR_NONE) {
    //Serial.println("jo");
  } else {
    //Serial.print("hiba");
    //Serial.println(state);
    while (true);
  }

  // set the function that will be called
  // when packet transmission is finished
  radio.setPacketSentAction(setFlag);

  // start transmitting the first packet
  //Serial.print("Sending first packet ... ");

  transmissionState = radio.startTransmit("LoRa");

}

// flag to indicate that a packet was sent
volatile bool transmittedFlag = false;
volatile bool receivedFlag = false;

#if defined(ESP8266) || defined(ESP32)
ICACHE_RAM_ATTR
#endif

void setFlag(void) {
  // we sent a packet, set the flag
  transmittedFlag = true;
}
void setFlag2(void) {
  receivedFlag = true;
}

// counter to keep track of transmitted packets
int count = 0;

void loop() {


  // check if the previous transmission finished
  if (transmittedFlag) {
    // reset flag
    transmittedFlag = false;

    if (transmissionState == RADIOLIB_ERR_NONE) {
      // packet was successfully sent
      u8g.print("ok");

    } else {
      u8g.print("failed");
      //Serial.println(transmissionState);
    }
    radio.finishTransmit();
    radio.startReceive();
    radio.setPacketReceivedAction(setFlag2);
  }

  // check if the flag is set
  if (receivedFlag) {
    // reset flag
    receivedFlag = false;

    // you can read received data as an Arduino String
    int state = radio.readData(str);

    if (state == RADIOLIB_ERR_NONE) {
      // packet was successfully received
      //Serial.println("Received packet!");

      // print data of the packet
      //Serial.print("Data:\t\t");
      u8g.print(str);

      // print RSSI (Received Signal Strength Indicator)
      //Serial.print("RSSI:\t\t");
      //Serial.print(radio.getRSSI());
      //Serial.println(F(" dBm"));

      // print SNR (Signal-to-Noise Ratio)
      //Serial.print("SNR:\t\t");
      //Serial.print(radio.getSNR());
      //Serial.println(" dB");

      // print frequency error
      //Serial.print(F("[SX1278] Frequency error:\t"));
      //Serial.print(radio.getFrequencyError());
      //Serial.println(" Hz");

    } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
      // packet was received, but is malformed
      //Serial.println("CRC error!");

    } else {
      // some other error occurred
      //Serial.print("Failed, code ");
      //Serial.println(state);

    }
  }

  if (kuld == true) {
    radio.begin();
    radio.setPacketSentAction(setFlag);
    transmissionState = radio.startTransmit(str);
    str = "";
    kuld = false;
  }

  // picture loop
  u8g.firstPage();
  do {
    u8g.setFont(u8g_font_unifont);
    u8g.setPrintPos(0, 20);
    u8g.print(str);
  } while ( u8g.nextPage() );
}
