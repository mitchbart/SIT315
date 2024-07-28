// Define constant read-only variables for button and led pins
const uint8_t RED_LED_PIN = 13;
//const uint8_t BLUE_LED_PIN = 12;
const uint8_t BLUE_LED_PIN = 2;
const uint8_t GREEN_LED_PIN = 6;

// Init volatile states - will change
volatile bool tiltState = 0;
volatile bool lightState = 0;
volatile bool lightState2 = 0;
volatile bool ledState = 0;

volatile bool prevLightState = 0;
volatile bool prevLightState2 = 0;


// Run on startup - assign outputs, interrupt and serial connection
void setup() {
  // Assign output led pins
  DDRB |= 0b00100000; // Pin 13;
  DDRD |= 0b01000100; // Pin 2 and 6;
  
  // Enable pin change interrupt on all ports
  PCICR |= 0b00000111;
  
  // Enable interrupt on pins 4, A0, and 11
  PCMSK2 |= 0b00010000; // Pin 4
  PCMSK1 |= 0b00000001; // Pin A0
  PCMSK0 |= 0b00001000; // Pin 11
  
  startTimer();
  
  Serial.begin(9600);  //establish serial connection with standard baud rate
}

void loop() {
  // Empty
}

// Interrupt service routine watching port D
ISR(PCINT2_vect) {
  Serial.println("TILT INTERRUPT!");
  // Get and print tiltState
  tiltState = PIND & 0b00010000;  // byte code for digitalRead(4)
  Serial.print("tiltState: ");
  Serial.println(tiltState);  
  // Set red and blue leds based on tiltState
  digitalWrite(RED_LED_PIN, !tiltState);
  digitalWrite(BLUE_LED_PIN, tiltState);
}

ISR(PCINT1_vect) {
  Serial.println("LIGHT INTERRUPT!");
  // Get and print lightState
  lightState = PINC & 0b00000001;  // digital read of lightState - faster?
  Serial.print("lightState: ");
  Serial.println(lightState);
  // Play a tone when NO light is detected
  
  if (lightState != prevLightState) {
  	prevLightState = lightState;
    if (lightState != 0) {
    	tone(8, 42);
  	}
  	else {
    	//noTone(8);
    	tone(8, 340);
  	}
  }
  
  //if (lightState == 0) {
  //  tone(8, 42);
  //}
  //else {
    //noTone(8);
  //  tone(8, 340);
  //}
}

ISR(PCINT0_vect) {
  Serial.println("LIGHT INTERRUPT2!");
  // Get and print lightState
  lightState2 = PINB & 0b00001000;  // digital read of lightState - faster?
  Serial.print("lightState2: ");
  Serial.println(lightState2);
  // Play a tone when NO light is detected
  if (lightState2 == 0) {
    tone(8, 420);
  }
  else {
    //noTone(8);
    tone(8, 670);
  }
}

void startTimer(){
  noInterrupts();
  
  // Clear timer controllers
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;  // timer counter
  
  // Set prescaler
  //TCCR1B |= (1 << CS10);
  //TCCR1B |= (1 << CS12);
  //TCCR1B |= (1 << WGM12);  // CTC time compare reset
  //TCCR1B |= 0b00001101;  // check bytes are correct
  TCCR1B |= 0b00001101;  // check bytes are correct
  
  
  OCR1A = 31249;  // Every 2 seconds
  //OCR1A = 16000000/(1024 * freq) - 1;
  
  //TIMSK1 |= (1 << OCIE1A);
  TIMSK1 |= 0b00000010;
  
  interrupts();
}

// ISR for timer
ISR(TIMER1_COMPA_vect){
  ledState = !ledState;
  digitalWrite(GREEN_LED_PIN, ledState);
}
