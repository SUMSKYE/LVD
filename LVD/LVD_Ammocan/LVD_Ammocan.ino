/*
 * A "flashy" LVD circuit: http://austindavid/lvd
 * Austin David, austin@austindavid.com 6 Sept 2013
 * (cc)
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

#define VOLTAGE_SCALE        47.62
#define VOLTAGE_CALIBRATION  (13.1 * VOLTAGE_SCALE)
#define VOLTAGE_HIGH         (12.0 * VOLTAGE_SCALE)
#define VOLTAGE_LOW          (11.5 * VOLTAGE_SCALE)

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
}


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
  
  sum = 0;
  for (byte i = 0; i < 10; i++) {
    sum += analogRead(JUMPER);
    delay(5);
  }
  
  jumperValue = sum / 10;
  
  if (jumperValue < 750) {
    flash(RED, 5); 
    int calibrated_voltage = sampleVoltage(V_SENSE);
    
   }
  
} // try_to_calibrate()


void read_calibration(void) {
  delay(500);
  flash(RED, 2);
  delay(500);
  flash(RED, 2);
} // read_calibration()


void setLVD(byte level) {
  state = level;
  digitalWrite(RED, !level);
  digitalWrite(RELAY, level);
}


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
  
  // try to set it "on" *once*
  int voltage = sampleVoltage(V_SENSE);
  if (voltage > HIGH_WATER) {
    setLVD(HIGH);
  } else {
    setLVD(LOW);
  }
} // setup()


// the loop routine runs over and over again forever:
void loop() {
  int voltage = sampleVoltage(V_SENSE);
  
#ifndef ATTINY
  // print out the value you read:
  Serial.println(voltage);
#endif

  if (voltage < LOW_WATER) {
    setLVD(LOW);
    delay(999999); // sleep a long time
  }
   
  delay(100); 
} // loop()
