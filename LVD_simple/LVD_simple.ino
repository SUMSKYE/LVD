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
 * YMMV :)
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
 
#define HIGH_WATER 571
#define LOW_WATER 547


/*
 * ATTiny45 don't have hardware serial (or, I don't use it) and have
 * different pin layouts from a typical Uno dev board
 */

//WTF #ifdef __AVR_ATtiny45__
#define ATTINY
//WTF #endif


// ATMEL ATTINY45 / ARDUINO
//
//                  +-\/-+
// Ain0 (D 5) PB5  1|    |8  Vcc
// Ain3 (D 3) PB3  2|    |7  PB2 (D 2)  Ain1
// Ain2 (D 4) PB4  3|    |6  PB1 (D 1) pwm1
//            GND  4|    |5  PB0 (D 0) pwm0
//                  +----+

#define ADC A1
#define RED 3
#define GREEN 4

#ifdef ATTINY      // ATTiny45 chip layout
#define RELAY 0
#define JUMPER 1
#else
#define RELAY 13   // Arduino UNO or other board
#define JUMPER 1
#define GREEN 13
#endif 


// the setup routine runs once when you press reset:
void setup() {
  #ifndef ATTINY
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
  #endif 
  pinMode(RELAY, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(RED, OUTPUT);
  
  digitalWrite(RELAY, LOW);
  digitalWrite(RED, LOW);
  digitalWrite(GREEN, LOW);
} // setup()


// the loop routine runs over and over again forever:
void loop() {
  int sensorValue = analogRead(ADC);
#ifndef ATTINY
  // print out the value you read:
  Serial.println(sensorValue);
#endif

  if (sensorValue > HIGH_WATER) {
    digitalWrite(RELAY, HIGH);
    digitalWrite(GREEN, HIGH);
    digitalWrite(RED, LOW);
    delay(2000);
  } else if (sensorValue < LOW_WATER) {
    digitalWrite(RELAY, LOW);
    digitalWrite(GREEN, LOW);
    digitalWrite(RED, HIGH);
    delay(2000);
  }
  delay(10); 
} // loop()
