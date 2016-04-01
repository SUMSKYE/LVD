/*
 * derived from a "flashy" LVD circuit: http://austindavid/lvd
 * Austin David, austin@austindavid.com 6 Sept 2013
 * (cc)
 * 
 * Calibration code borrows from 
 * http://www.re-innovation.co.uk/web12/index.php/en/information/electronics-information/accurate-voltage-measurement
 *
 *
 * This is specifically for the LVD hardware at https://oshpark.com/shared_projects/81gfc0Mb
 */

/*
 * The ADC is fed wth a voltage divider, 10k + 49.9k
 * Ideal voltage conversion:
 * ADC = Vin * (10 / (10 + 49.9)) / 5 * 1024
 * 
 * 11.3v -> ADC @ 387
 * 10.5v -> ADC @ 359
 * 
 * YMMV :)  These values are pretty wrong on a breadboard (lots of weird
 * resistances), but work great on a tight soldered protoboard.
 *
 * Make one, ground the debug pin, and check the values; reprogram if needed.
 *
 *
 * For the new PCB LVD, 10k + 33k @ 3.3v VCC
 *   ADC = Vin * (10/(10+33)) / 3.3 * 1024
 * 11.5 -> 902
 * 11.0 -> 794
 *
 *  at 5V, 47.62/V
 *  12.0 -> 571.44
 *  11.5 -> 547.63
 *  11.0 -> 523.82
 *
 *
 * bench observed:
 *   12.0 =~ 547
 */
 
/* experimental 
#define HIGH_WATER 550 
#define LOW_WATER 530
*/

#define VREF_ADDY 10
#include <EEPROM.h>

#define VOLTAGE_SCALE        47.62
#define VOLTAGE_CALIBRATION  13.1
#define VOLTAGE_HIGH         12.7
#define VOLTAGE_LOW          12.1

#define LOW_TIMEOUT          30    // seconds


float v_scale = 47.62;
float v_high = VOLTAGE_HIGH * v_scale;
float v_low = VOLTAGE_LOW * v_scale;
int low_timer = 0;

/*
 * ATTiny45 don't have hardware serial (or, I don't use it) and have
 * different pin layouts from a typical Uno dev board
 */

#ifdef __AVR_ATtiny45__
#define ATTINY
#endif

#ifdef __AVR_ATtiny85__
#define ATTINY
#endif

// ATMEL ATTINY45 / ARDUINO
//
//                  +-\/-+
// Ain0 (D 5) PB5  1|    |8  Vcc
// Ain3 (D 3) PB3  2|    |7  PB2 (D 2) Ain1
// Ain2 (D 4) PB4  3|    |6  PB1 (D 1) pwm1
//            GND  4|    |5  PB0 (D 0) pwm0
//                  +----+


#define JUMPER  A0 // DIP 1, also RESET so be careful
#define V_SENSE A1 // DIP 7
#define RED      3 // DIP 2
#define RELAY 0    // DIP 5

#ifndef ATTINY     // Arduino UNO or other board
#define RELAY 13   // the built-in LED on an Uno
#endif 

// #define TESTING
#undef TESTING

byte state;

#define NUMBER_OF_SAMPLES 20
#define DELAY_BETWEEN_SAMPLES 10 // ms
int sampleVoltage(byte pin) {
  int voltage, sum;
  sum = 0;
  for (int i = 0; i < NUMBER_OF_SAMPLES; i++) {
    sum += analogRead(pin);
    delay(DELAY_BETWEEN_SAMPLES);
  }
  voltage = sum / NUMBER_OF_SAMPLES;
  return voltage;
} // int sampleVoltage(byte pin) 


void flash(byte pin, byte nr_flashes) {
  for (byte i = 0; i < nr_flashes; i++) {
    digitalWrite(pin, HIGH);
    delay(100);
    digitalWrite(pin, LOW);
    delay(100);
  }  
} // flash(pin, nr_flashes)


void try_to_calibrate(void) {
  int jumperValue, sum;
  #ifdef TESTING
  flash(RED, 1);
  delay(1000);
  #endif
  sum = 0;
  for (byte i = 0; i < 10; i++) {
    sum += analogRead(JUMPER);
    delay(5);
  }

  #ifdef TESTING
  flash(RED, 2);
  delay(1000);
  #endif
  jumperValue = sum / 10;
  
  if (jumperValue < 750) {
    #ifndef TESTING
    flash(RED, 2);
    #endif 
    int calibrated_voltage = sampleVoltage(V_SENSE);
    write_calibration(calibrated_voltage);
    #ifndef TESTING
    flash(RED, 2);
    #endif
   }
  
} // try_to_calibrate()


void write_calibration(int calibrated_voltage) {
  EEPROM.write(VREF_ADDY, calibrated_voltage >> 8); 
  EEPROM.write(VREF_ADDY + 1, calibrated_voltage & 0xff); 

  flash(RED, 3);
  delay(1000);
} // void write_calibration(int calibrated_voltage)


void read_calibration(void) {
  byte hiByte, loByte;
  int Vref;
  
  #ifdef TESTING
  flash(RED, 4);
  delay(1000);
  #endif

  hiByte = EEPROM.read(VREF_ADDY);
  loByte = EEPROM.read(VREF_ADDY + 1);
  Vref = (hiByte << 8)+loByte; 
  if (hiByte != 255 && loByte != 255 && Vref < 1024) {   
    v_scale = 1.0 * Vref / VOLTAGE_CALIBRATION;
    v_high = VOLTAGE_HIGH * v_scale;
    v_low = VOLTAGE_LOW * v_scale;
  } else {
    flash(RED, 20);
  }
} // read_calibration()


void setLVD(byte level) {
  state = level;
  digitalWrite(RED, !level);
  digitalWrite(RELAY, level);
} // void setLVD(byte level)



// the setup routine runs once when you press reset:
void setup() {
  #ifndef ATTINY
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
  #endif 
  pinMode(V_SENSE, INPUT);
  pinMode(RELAY, OUTPUT);
  pinMode(RED, OUTPUT);
  
  // default state: off + quiet
  state = LOW;
  digitalWrite(RELAY, LOW);
  digitalWrite(RED, LOW);
  
  // check to see if we're supposed to recalibrate
  try_to_calibrate();
  
  // read a calibrated value for the ADC
  read_calibration();
  
  #ifdef TESTING
  flash(RED, 5);
  delay(1000);
  #endif
} // setup()


// the loop routine runs over and over again forever:
void loop() {
  int voltage = sampleVoltage(V_SENSE);
  
#ifndef ATTINY
  // print out the value you read:
  Serial.println(voltage);
#endif

  if (voltage >= v_high) {
    setLVD(HIGH);
    delay(2000);
  } else if (voltage <= v_low) {
    setLVD(LOW);
    delay(2000);
  }
  delay(100);
} // loop()
