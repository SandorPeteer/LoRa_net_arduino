// include the library
#include <RadioLib.h>

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
        Serial.println(" ->");
        kuld = true;
        break;
      default:
        str += keymap[scanval];
        Serial.print(keymap[scanval]);
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
  Serial.begin(115200);

  pinMode(CLOCK, INPUT_PULLUP); //For most keyboards the builtin pullups are sufficient, so the 10k pullups can be omitted
  pinMode(DATA, INPUT_PULLUP);
  bitSet(PCICR, PCIE2); // Enable pin change interrupts on pin D0-D7
  bitSet(PCMSK2, CLOCK); // Pin change interrupt on Clock pin


  // initialize SX1278 with default settings
  Serial.print(F("[SX1278] Initializing ... "));
  int state = radio.begin();
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true);
  }

  // set the function that will be called
  // when packet transmission is finished
  radio.setPacketSentAction(setFlag);

  // start transmitting the first packet
  Serial.print(F("[SX1278] Sending first packet ... "));

  transmissionState = radio.startTransmit("Hello SP RADIO!");

}

// flag to indicate that a packet was sent
volatile bool transmittedFlag = false;

// this function is called when a complete packet
// is transmitted by the module
// IMPORTANT: this function MUST be 'void' type
//            and MUST NOT have any arguments!
#if defined(ESP8266) || defined(ESP32)
ICACHE_RAM_ATTR
#endif
void setFlag(void) {
  // we sent a packet, set the flag
  transmittedFlag = true;
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
      Serial.println(F("ok"));

      // NOTE: when using interrupt-driven transmit method,
      //       it is not possible to automatically measure
      //       transmission data rate using getDataRate()

    } else {
      Serial.print(F("failed"));
      Serial.println(transmissionState);

    }
    radio.finishTransmit();

  }
  if (kuld == true) {
    transmissionState = radio.startTransmit(str);
    str = "";
    kuld = false;
  }
}
