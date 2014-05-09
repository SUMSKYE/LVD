/*
 * A "flashy" LVD circuit: http://austindavid/t/lvd-a
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
 */
 
#define HIGH_WATER 387
#define LOW_WATER 359


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


#define VOLTAGE A1 // DIP 7
#define RED 3      // DIP 2
#define GREEN 4    // DIP 3

#ifdef ATTINY      // ATTiny45 assignments
#define RELAY 0    // DIP 5
#define JUMPER 1   // DIP 6
#else
#define RELAY 13   // Arduino UNO or other board
#define JUMPER 2
#define GREEN 13
#endif 


/*
 * simple array-based stack of ints.  For this project I wrote this to 
 * keep it in one file.  For any ATMega-based project, see:
 *   http://playground.arduino.cc/Code/StackList
 *   http://playground.arduino.cc/Code/StackArray
 */
class Stack {
  public:
    Stack();
    void reset();
    void push(int value);
    int pop();
    void dump();
    boolean isEmpty();
  private:
#define STACK_MAX 50    // 50 ints / state transitions; "999" -> ~35 transitions
    int _head;
    int _stack[STACK_MAX]; 
}; // class Stack


// constructor
Stack::Stack() {
  reset();
} // Stack()


// reset the Stack to "empty"
void Stack::reset() {
  _head = -1;
} // Stack::reset()


/* 
 *push(value) onto the top of the stack
 *
 * silently refuses to act if the stack is full
 */
void Stack::push(const int value) {
  if (_head < STACK_MAX - 1) {
    _head ++;
    _stack[_head] = value;
  }
} // push(value)


/*
 * pop a value off the top of the stack
 * 
 * returns 0 if the stack is empty.
 */
int Stack::pop() {
  int value;
  if (! isEmpty()) {
    value = _stack[_head];
    _head --;
    return value;
  } else {
    return 0;
  }
} // int pop()


// returns true if the stack isEmpty
boolean Stack::isEmpty() {
  return _head < 0;
} // boolean Stack::isEmpty()


// dumps the stack contents to Serial
void Stack::dump() {
#ifndef ATTINY
  for (int i = 0; i <= _head; i++) {
    Serial.print(i); Serial.print(":");
    Serial.println(_stack[i]); 
  }
#endif
} // Stack::dump()

// end class Stack


/*
 * An asynchronous Flasher
 * This flashes a decimal value in a series of on / off (flashes)
 * separated by pauses (off).
 *
 * Usage: 
 * Flasher flash = Flasher(1); // new Flasher on pin 1
 * in loop():
 *    flasher.run();               // check to see if it's time to do something
 *    if (! flasher.flashing()) {
 *      flasher.start(209);        // begin flashing the sequence 2..0..9
 *    }
 *    ...
 *    delay(10); 
 *
 * The shortest delay between events is 50ms; for proper operation,
 * run() should execute at least that often.
 *
 * "0" is a very quick flash (50ms HIGH, 200ms LOW)
 * 1-9 are a series of 1-9 normal flashes (200ms HIGH, 200ms LOW)
 * pauses between digits are 500ms LOW
 * the cycle terminates with a 1s LOW, 500ms HIGH, 1s LOW (very slow flash)
 * 
 * "209" -> 2 flashes, a very fast flash, 9 flashes, 1 very slow flash
 * 
 * Flasher will not automagically repeat; run() will return quickly if
 * there is no more work.  The example above restarts the flasher once it 
 * is done flashing.
 */
class Flasher {
  public: 
    Flasher(int pin);
    void start(int value);
    void run();
    boolean isFlashing();
    void stop();
  private:
    int _pin;
    Stack _stack;
    int _remainingMS;                     // time (ms) till next transition
    unsigned long _previousMillis;        // time of last transition
    void _show(int level);
}; // Flasher


/*
 * constructor; pin == LED
 */
Flasher::Flasher(int pin) {
  _pin = pin;
  _stack = Stack();
  pinMode(_pin, OUTPUT); 
  stop();
} // Flasher::Flasher(pin)


/*
 * stops the Flasher
 */
void Flasher::stop() {
  _previousMillis = 0;
  _remainingMS = 0;
  _stack.reset();
} // Flasher::stop()


/*
 * seeds the Flasher with a new value & kicks it off
 */
void Flasher::start(int value) {
  stop();
  
  // note that the stack is last-in / first-out -- it replays backward.
  
  // always end with a slow flash.  Negative values are LOW, time in ms;
  // -1000 == 1000ms "off"; 500 == 500ms "on"
  _stack.push(-1000);
  _stack.push(1000); 
  _stack.push(-500);

  // push the value's digits onto the stack  
  while (value > 0) {
    _stack.push(-500);                    // 500ms pause at the end
    int digit = value % 10;
    if (digit > 0) {
      for (int i = 0; i < digit; i ++) {
        _stack.push(-200);                // 200ms ON, 200ms OFF
        _stack.push(200);
      }
    } else {
      _stack.push(-200);                  // 0: 50ms ON, 200ms OFF
      _stack.push(50);
    }
    value = value / 10;
  }
  
  _remainingMS = -1;  // force a step
#ifdef FLASHER_DEBUG
#ifndef ATTINY
  _stack.dump();
#endif
#endif
  _show(LOW);
  run();
} // Flasher::display(value)


// show (display) a HIGH or LOW on this->_pin
void Flasher::_show(int level) {
#ifdef FLASHER_DEBUG
#ifndef ATTINY
  Serial.print(F("Showing "));
  Serial.println(level);
#endif
#endif
  digitalWrite(_pin, level);
} // Flasher::_show(level)


/*
 * returns immediately if there is no required work
 *
 * if the current timer has expired, execute the state transition &
 * reset the timer
 */
void Flasher::run() {
  if (isFlashing()) {
    unsigned long currentMillis = millis();
    if (_remainingMS < 0 || 
        currentMillis - _previousMillis >= (unsigned long)(_remainingMS)) {
      // time to do something
      _previousMillis = currentMillis;
      int next = _stack.pop();
      // if stack is empty, next == 0, _show(LOW), remaining = 0 --> stopped
      _show(next <= 0 ? LOW : HIGH);
      _remainingMS = abs(next);
    } // if _remainingMS expired
#ifdef FLASHER_DEBUG
#ifndef ATTINY
    else {
      Serial.print(F("waiting; c="));
      Serial.print(currentMillis); Serial.print(", p=");
      Serial.print(_previousMillis); Serial.print(", r=");
      Serial.print(_remainingMS);
      Serial.println(F(" MS"));
    }
#endif
#endif
  } // isFlashing()
} // Flasher::run()



// are we actively flashing?
boolean Flasher::isFlashing() {
  return ! (_stack.isEmpty() && _remainingMS == 0);
} // boolean Flasher::flashing()


/*
 * end class Flasher
 */


/*
 * GLOBALS
 *
 * green: the Flasher on the GREEN LED
 * state: the current HIGH/LOW state of the LVD
 */
Flasher green = Flasher(GREEN);
int state = HIGH;

// require HOLD_COUNT high in a row (or low in a row) before accepting 
// a state transition
#define HOLD_COUNT 5
int high_counter = 0;
int low_counter = 0;

/*
 * usage: setLVD(HIGH) -> relay ON,  green ON,  red OFF
 *        setLVD(LOW)  -> relay OFF, green OFF, red ON
 */
void setLVD(int level) {
  if (state != level) {
    if (level == HIGH) {
      high_counter ++;
      low_counter = 0;
      if (high_counter > HOLD_COUNT) {
        digitalWrite(RELAY, HIGH);
        digitalWrite(GREEN, HIGH);
        digitalWrite(RED, LOW);
        state = level;
      } // high counter expires
    } else {
      low_counter ++;
      high_counter = 0;
      if (low_counter > HOLD_COUNT) {
        digitalWrite(RELAY, LOW);
        digitalWrite(GREEN, LOW);
        digitalWrite(RED, HIGH);
        state = level;
      } // low counter expires
    } // level == HIGH or LOW
  } // state change
} // setLVD(level)


// the setup routine runs once when you press reset:
void setup() {
  #ifndef ATTINY
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
  #endif 
  pinMode(JUMPER, INPUT_PULLUP);
  pinMode(RELAY, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(RED, OUTPUT);
  
  // default state: off
  state = LOW;
  digitalWrite(RELAY, LOW);
  digitalWrite(GREEN, LOW);
  digitalWrite(RED, HIGH);
} // setup()


// the loop routine runs over and over again forever:
void loop() {
  int sensorValue = analogRead(VOLTAGE);

#ifndef ATTINY
  // print out the value you read:
  Serial.println(sensorValue);
#endif

  if (sensorValue > HIGH_WATER) {
    setLVD(HIGH);
  } else if (sensorValue < LOW_WATER) {
    setLVD(LOW);
  }
 
  // if the JUMPER is in place, flash GREEN to show the sensorValue
  boolean debugging = digitalRead(JUMPER) == LOW;
  if (debugging) {
    green.run();
    if (! green.isFlashing()) {
      green.start(sensorValue);
    }
  } else {
    green.stop();
    digitalWrite(GREEN, state);   // set GREEN to match the current state 
  }
  
  delay(10); 
} // loop()
