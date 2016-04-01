#include <EEPROM.h>
#include <Bounce.h>



// ATTiny45: http://www.atmel.com/devices/attiny45.aspx?tab=parameters
// 256b flash
#undef ATTINY

#ifdef ATTINY
#define EEPROM_START 0
#define EEPROM_SIZE 256
#undef SERIAL_DEBUG
#else
#define EEPROM_START 0
#define EEPROM_SIZE 256
#define SERIAL_DEBUG
#endif


#define HIGH_ADDRESS (EEPROM_START)
#define LOW_ADDRESS (EEPROM_START + 2)

#define HIGH_DEFAULT 387
#define LOW_DEFAULT 359
int highWatermark;
int lowWatermark;


int readInt(const int addy) {
  byte low, high;
  high = EEPROM.read(addy);
  low = EEPROM.read(addy + 1);
  int value = 0;
  value = (high << 8) | low;

#ifdef SERIAL_DEBUG
  Serial.print("read: ");
  Serial.print(high, HEX);  Serial.print(", ");  Serial.print(low, HEX);
  Serial.print(" => ");  Serial.println(value, HEX);
#endif

  return value;
} // int readInt(addy)


void writeInt(const int addy, const int value) {
  byte low, high;
  low = byte(value & 0xFF);   // lower bits
  high = byte(value >> 8);
  EEPROM.write(addy, high);
  EEPROM.write(addy + 1, low);
  
#ifdef SERIAL_DEBUG
  Serial.print("write: ");
  Serial.print(value, HEX);  Serial.print(" => ");
  Serial.print(high, HEX);  Serial.print(", ");  Serial.println(low, HEX);
#endif
} // writeInt(addy, value)


/*
 * reads high/low_value from EEPROM.  If unset (either are 0xFFFF),
 * values are set to the _DEFAULT
 */
void readWatermarks() {
  int tmpHigh = readInt(HIGH_ADDRESS);
  int tmpLow = readInt(LOW_ADDRESS);

  if (tmpHigh == 0xFFFF || tmpLow == 0xFFFF) {
#ifdef SERIAL_DEBUG
    Serial.println("EEPROM not initialized; using defaults");
#endif
    highWatermark = HIGH_DEFAULT;
    lowWatermark = LOW_DEFAULT;
  } else {
    highWatermark = tmpHigh;
    lowWatermark = tmpLow;
  }
} // readWatermarks


/*
 * writes the high/lowWatermark to EEPROM
 */
void writeWatermarks() {
  writeInt(HIGH_ADDRESS, highWatermark);
  writeInt(LOW_ADDRESS, lowWatermark);
} // writeWatermarks()


void setup() {
#ifdef SERIAL_DEBUG
  Serial.begin(9600);
#endif

  readWatermarks();

#ifdef SERIAL_DEBUG
  Serial.print("Watermarks: ");
  Serial.print(highWatermark);  Serial.print(", ");
  Serial.println(lowWatermark);  
#endif

}


int loops = 0;
void loop() {
  return;
  if (loops < 3) {
    ++loops;
    delay(2000);
    unsigned int value = 0xabcd;
#ifdef SERIAL_DEBUG
    Serial.print("value: ");
    Serial.print(value, HEX); 
    Serial.print("; ");
#endif
    writeInt(HIGH_ADDRESS, value);
    value = readInt(HIGH_ADDRESS);
#ifdef SERIAL_DEBUG
    Serial.print("readInt: "); Serial.println(value, HEX);
#endif
    int addy = EEPROM_START;
    while (addy < EEPROM_START + 4) {
      byte b = EEPROM.read(addy);
#ifdef SERIAL_DEBUG
      Serial.print(addy);  Serial.print(": "); Serial.println(b);
#endif
      addy ++;
    }

    delay(5000);
  } else {
    delay(10000);
  }
  
  delay(500);
}
