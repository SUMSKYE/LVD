/*
 * derived from a "flashy" LVD circuit: http://austindavid/lvd
 * Austin David, austin@austindavid.com 6 Sept 2013
 *               updated 16 Apr 2016
 * (cc)
 * 
 * Calibration code borrows from 
 * http://www.re-innovation.co.uk/web12/index.php/en/information/electronics-information/accurate-voltage-measurement
 *
 *
 * This is specifically for the LVD hardware at https://oshpark.com/shared_projects/1gKnngEO
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
 * scale = 72.16
 * 12.0 -> 866
 * 11.5 -> 829
 * 11.0 -> 794
 *
 *
 * Usage / tuning:
 *   Connect the input voltage
 *   set the input to the "turn me on" level -- fully charged battery or whatever
 *   turn the LVD "off" (press the button briefly to get the LED to illuminate)
 *   while LVD is off (LED is illuminated), press & hold the button
 *     LED turns off, LVD switches on
 *     after 2s, LED flashes twice
 *     this new level is saved permanently
 *   set the input voltage to the "turn me off" level -- for SLA this is ~ 12v,
 *     but use whatever is appropriate for the application
 *   turn the LVD "on" (press the button briefly to extinguish the LED, if needed)
 *   while the LVD is on (LED is extinguised) press & hold the button
 *     LED turns on, LVD turns off
 *     after 2s (still holding) LED flashes twice
 *     this new level is saved permanently.
 *     
 *   After this procedure the LVD will turn on & off at these levels.
 */
 
#define VREF_ADDY 10
#define LOW_ADDY  0
#define HIGH_ADDY (LOW_ADDY + sizeof(int))
#include <EEPROM.h>
#include "elapsedMillis.h"

#define VOLTAGE_SCALE        72.16
#define VOLTAGE_HIGH         12.6
#define VOLTAGE_LOW          11.5

#define HYSTERESIS           2000  // ms (2s)
#define LOW_TIMEOUT          30000 // ms (30s)


float v_scale = VOLTAGE_SCALE;
float v_high = VOLTAGE_HIGH * v_scale;
float v_low = VOLTAGE_LOW * v_scale;


/*
 * ATtiny45 don't have hardware serial (or, I don't use it) and have
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


#define BUTTON   3 // DIP 2
#define V_SENSE A1 // DIP 7
#define LED      4 // DIP 2
#define RELAY    0 // DIP 5

#ifndef ATTINY     // Arduino UNO or other board
#define LED   13   // the built-in LED on an Uno
#endif 

// #define TESTING


/*
 * int sampleVoltage(byte pin)
 * 
 * returns the average ADC reading from 
 * a number of samples over time;
 * used to smooth out noisy readings
 */
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


/*
 * flashes the pin some number of times
 */
void flash(byte pin, byte nr_flashes) {
  bool prevState = digitalRead(pin);
  
  for (byte i = 0; i < nr_flashes; i++) {
    digitalWrite(pin, !prevState);
    delay(100);
    digitalWrite(pin, prevState);
    delay(100);
  }  
} // flash(pin, nr_flashes)


/*
 * read a previously saved calibration level
 */
void read_calibration(void) {
  int low, high;
  EEPROM.get(LOW_ADDY, low);
  EEPROM.get(HIGH_ADDY, high);

  // valid range is 300-1024, which is about 4-14v
  if (low > 300 && low < 1024) {
    v_low = low;
  }

  if (high > 300 && high < 1024) {
    v_high = high;
  }
} // read_calibration()


void saveVoltage(bool state, int newVoltage) {
  if (state == LOW) {
    EEPROM.put(LOW_ADDY, newVoltage);
    v_low = newVoltage;
  } else {
    EEPROM.put(HIGH_ADDY, newVoltage);
    v_high = newVoltage;
  }

  flash(LED, 2);
} // saveVoltage(state, level)


byte state = LOW;
void setLVD(bool newState) {
  state = newState;
  digitalWrite(LED, !newState);
  digitalWrite(RELAY, newState);
} // void setLVD(byte newState)


void tweakLVD(bool newState, int voltage) {
  setLVD(newState); // invert state
  // save this voltage temporarily
  if (newState == HIGH) {
    v_high = voltage;
    // low will be at least 0.1v under 
    v_low = min(v_low, voltage - 0.1*VOLTAGE_SCALE);
  } else {
    v_low = voltage;
    // high will be at least 0.1v over
    v_high = max(v_high, voltage + 0.1*VOLTAGE_SCALE);
  }
} // tweakLVD(state, voltage)


bool isPressed(byte pin, int timeout) {
  elapsedMillis timer = 0;

  while (timer < timeout) {
    if (digitalRead(pin) == LOW) {
      return true;
    }
  }

  return false;
} // isPressed(pin, timeout)


int getPressedDuration(byte pin, int maxTime) {
  elapsedMillis buttonTimer = 0;

  while (digitalRead(pin) == LOW && buttonTimer < maxTime) {
    // nop
  }

  return buttonTimer;
} // int getPressedDuration(pin, maxTime)





// the setup routine runs once when you press reset:
void setup() {
  #ifndef ATTINY
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
  #endif 
  pinMode(V_SENSE, INPUT);
  pinMode(RELAY, OUTPUT);
  pinMode(LED, OUTPUT);
  pinMode(BUTTON, INPUT_PULLUP);
  
  // powerup: state off + quiet
  state = LOW;
  digitalWrite(RELAY, LOW);
  digitalWrite(LED, LOW);

  // read a calibrated value for the ADC
  read_calibration();

  // try to set it "on" *once*
  int voltage = sampleVoltage(V_SENSE);
  if (voltage > v_high) {
    setLVD(HIGH);
  } else {
    setLVD(LOW);
  }  
} // setup()


elapsedMillis low_timer = 0;

// the loop routine runs over and over again forever:
void loop() {
  int voltage = sampleVoltage(V_SENSE);
  
#ifndef ATTINY
  // print out the value you read:
  Serial.println(voltage);
#endif

  if (voltage <= v_low && state == HIGH) {    
    if (low_timer >= LOW_TIMEOUT) {
      setLVD(LOW);
      delay(HYSTERESIS); // sleep a little while
    } else {
      flash(LED, 1);
    }
  } else {
    low_timer = 0;
    if (voltage > v_high) {
      setLVD(HIGH);
      delay(HYSTERESIS); // sleep a little while
    }
  }


  // button pressed: toggle
  if (isPressed(BUTTON, 100)) {
    tweakLVD(!state, voltage); // invert state

    // press+hold for ~ 2s to save this level permanently
    if (getPressedDuration(BUTTON, 2000) >= 2000) {
      saveVoltage(state, voltage); // save this new state's level
    }
  }
} // loop()
