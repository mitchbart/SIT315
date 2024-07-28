// Init read-only pins for LEDs
const uint8_t RED_LED_PIN = 13;
const uint8_t BLUE_LED_PIN = 2;
const uint8_t GREEN_LED_PIN = 6;

// Init changing states
volatile bool tiltState = 0;
volatile bool lightState = 0;
volatile bool prevLightState = 0;
volatile bool lightState2 = 0;
volatile bool prevLightState2 = 0;

// Run on startup
void setup() {
  // Assign output LED pins
  DDRB |= 0b00100000; // Pin 13;
  DDRD |= 0b01000100; // Pin 2 and 6;
  
  // Enable pin change interrupt on all ports
  PCICR |= 0b00000111;
  
  // Watch interrupts on pins 4, A0, and 11
  PCMSK2 |= 0b00010000; // Pin 4
  PCMSK1 |= 0b00000001; // Pin A0
  PCMSK0 |= 0b00001000; // Pin 11
  
  // Begin timer function and open serial port
  startTimer();
  Serial.begin(9600); 
}

void loop() {
  // Empty
}

// Tilt interrupt - watching port D
ISR(PCINT2_vect) {
  Serial.println("Tilt interrupt!");

  // Get tiltState and display
  tiltState = PIND & 0b00010000; // Digital read of tilt sensor pin
  Serial.print("tiltState: ");
  Serial.println(tiltState);

  // Update leds
  digitalWrite(RED_LED_PIN, !tiltState);
  digitalWrite(BLUE_LED_PIN, tiltState);
}

// Phototransistor 1 interrupt - watching port C
ISR(PCINT1_vect) {
  Serial.println("Light interrupt 1!");

  // Get lightState and display - digital read used on analog pin
  lightState = PINC & 0b00000001; // Digital read of phototransistor pin
  Serial.print("lightState: ");
  Serial.println(lightState);

  // Update lightstate and play tones
  if (lightState != prevLightState) { // If lightState has changed
  	prevLightState = lightState; // Update previous lightState
    if (lightState == 1) { // If lightState == 1 play a tone
    	tone(8, 494);
  	}
  	else { // If lightState == 0, no tone
      noTone(8);
  	}
  }
}

// Phototransistor 2 interrupt - watching port B
ISR(PCINT0_vect) {
  Serial.println("Light interrupt 2!");

  // Get lightState2 and display
  lightState2 = PINB & 0b00001000; // Digital read of phototransistor 2 pin
  Serial.print("lightState2: ");
  Serial.println(lightState2);

// Update lightstate2 and play tones
  if (lightState2 != prevLightState2) { // If lightState has changed
  	prevLightState2 = lightState2; // Update previous lightState2
    if (lightState2 == 1) { // If lightState2 == 1 play a tone
    	tone(8, 330);
  	}
  	else { // If lightState2 == 0, no tone
      noTone(8);
  	}
  }
}

// Start timer function
void startTimer(){
  // Interrupts paused while function is running
  noInterrupts();
  
  // Clear timer controllers
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0; // Timer counter
  
  // Set prescaler and reset CTC
  TCCR1B |= 0b00001101;  
  
  // Frequency to call timer compare interrupt
  OCR1A = 15624;  // Every 1 second
  
  // Enable timer compare interrupt
  TIMSK1 |= 0b00000010;
  
  interrupts();
}

// Timer interrupt
ISR(TIMER1_COMPA_vect){
  // Flip the state of the green LED
  digitalWrite(GREEN_LED_PIN, digitalRead(GREEN_LED_PIN) ^ 1);
}
