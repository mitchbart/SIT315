// Define constant read-only variables for button and led pins
const uint8_t RED_LED_PIN = 13;
const uint8_t BLUE_LED_PIN = 12;

// Init volatile states - will change
volatile bool tiltState = 0;
volatile bool lightState = 0;

// Run on startup - assign outputs, interrupt and serial connection
void setup() {
  DDRB |= 0b00110000;  // assign led pins 12 and 13 to output;
  PCICR |= 0b00000100;  // enable pin change interrupt for Port D
  PCMSK2 |= 0b00010000;  // enable interrupt for pin 4
  PCICR |= 0b00000010; // enable port change interrupt for Port C - analogue pins
  PCMSK1 |= 0b00000001;  // enable interupt on pin A0
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
  if (lightState == 0) {
    tone(8, 42);
  }
  else {
    noTone(8);
  }
}

