/*
  AnalogReadSerial
  Reads an analog input on pin 0, prints the result to the serial monitor.
  Attach the center pin of a potentiometer to pin A0, and the outside pins to +5V and ground.
 
 This example code is in the public domain.
 */


int HIGH_WATER = 318;
int LOW_WATER = 297;

#define ADC A1
#define RELAY 1
#define BUTTON 3


// uno: LED = 13
// tiny: LED = 0
#define ATTINY

#ifdef ATTINY
#define LED 0
#else
#define LED 13
#endif 

class Flasher {
  public: 
    Flasher(int pin);
    void start(int flashes, int durationMS);
    void run();
    void stop();
    boolean flashing();
    void reset();
  private:
    int _pin;
    int _flashes;
    int _durationMS;
    int _remainingMS;
}; // Flasher


Flasher::Flasher(int pin) {
  _pin = pin;
  pinMode(_pin, OUTPUT);
} // Flasher::Flasher(pin)


void Flasher::start(int flashes, int durationMS) {
  _flashes = flashes;
  _durationMS = durationMS;
  run();
} // Flasher::start(flashes, durationMS)


void Flasher::run() {
}


boolean Flasher::flashing() {
  return _remainingMS != 0;
} // boolean Flasher::flashing()


void Flasher::stop() {
  _remainingMS = 0;
  digitalWrite(_pin, LOW);
} // Flasher::reset()


Flasher flasher = Flasher(LED);



// the setup routine runs once when you press reset:
void setup() {
  #ifndef ATTINY
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
  #endif 
  pinMode(LED, OUTPUT);
  pinMode(RELAY, OUTPUT);
  digitalWrite(RELAY, LOW);
} // setup()


void flash (int output, int flashes) {
  int i;
  for (i = 0; i < flashes; i++) {
    digitalWrite(output, HIGH);
    delay(100);
    digitalWrite(output, LOW);
    delay(100);
  }
} // flash(output, flashes)


/*
 *  will "report" an int value by flashing the decimal representation, LSB first
 * e.g. "320" -> <blank> 1s pause <2 flashes> 1s pause <3 flashes>
 * leads and ends with a 1s "on"
 */
void report (int output, int value) {
  int flashes;
  digitalWrite(output, HIGH);
  delay(1000);
  digitalWrite(output, LOW);
  delay(1000);
  while (value > 0) {
    flash(output, value % 10);
    value = value / 10;
    delay(1000);
  }

  digitalWrite(output, HIGH);
  delay(1000);
  digitalWrite(output, LOW);
  delay(1000);
} // report(value)


// the loop routine runs over and over again forever:
void loop() {
  // read the input on analog pin 0:
  int sensorValue = analogRead(ADC);
  #ifndef ATTINY
  // print out the value you read:
  Serial.println(sensorValue);
  #endif
  if (sensorValue > HIGH_WATER) {
    digitalWrite(RELAY, HIGH);
  } else if (sensorValue < LOW_WATER) {
    digitalWrite(RELAY, LOW);
  }
  report(LED, sensorValue);
  delay(5000);        // delay in between reads for stability
}
