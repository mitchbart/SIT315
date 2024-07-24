// Define constant read-only variables for button and led pins
const uint8_t RED_LED_PIN = 13;
const uint8_t BLUE_LED_PIN = 12;

// Init tiltState - will change
volatile bool tiltState = false;

// Run on startup - assign outputs, interrupt and serial connection
void setup() {
  DDRB |= 0b00110000;  // assign leds 12 and 13 to output;
  PCICR |= 0b00000100;  // enable pin change interrupt for Port D
  PCMSK2 |= 0b00001000;  // enable interrupt for pin 4
  Serial.begin(9600);  //establish serial connection with standard baud rate
}

// Loop left in so we can see it be interrupted
void loop() {
  Serial.print("Loop begin");
  delay(1000);
  Serial.print(".");
  delay(1000);
  Serial.print(".");
  delay(1000);
  Serial.print(". ");
  delay(1000);
  Serial.println("Loop end.");
}

// Interrupt service routine watching port D
ISR(PCINT2_vect) {
  Serial.println("");
  Serial.println("INTERUPTED!");
  tiltState = PIND & 0b00001000;  // alternative byte code for digitalRead()
  Serial.print("Tiltstate: ");
  Serial.println(tiltState);

  // Set red and blue leds based on tiltState
  digitalWrite(RED_LED_PIN, !tiltState);
  digitalWrite(BLUE_LED_PIN, tiltState);
}