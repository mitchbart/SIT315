// Define constant read-only variables for button and led pins
const uint8_t RED_LED_PIN = 13;
const uint8_t BLUE_LED_PIN = 12;

// photo tran values
//int value = 0;
//int photoTran = A0;
int photoTran = 5;

// Init tiltState - will change
volatile bool tiltState = false;
volatile bool lightState = false;

// Run on startup - assign outputs, interrupt and serial connection
void setup() {
  DDRB |= 0b00110000;  // assign led pins 12 and 13 to output;
  PCICR |= 0b00000100;  // enable pin change interrupt for Port D
  //PCMSK2 |= 0b00010000;  // enable interrupt for pin 4
  PCMSK2 |= 0b00110000;  // enable interrupt for pins 4 and 5
  Serial.begin(9600);  //establish serial connection with standard baud rate
}

// Loop left in so we can see it be interrupted
void loop() {
  //Serial.print("Loop begin");
  // value = digitalRead(photoTran);
  // //value = analogRead(photoTran);
  // Serial.println(value);
  // if (value > 0) {
  //   tone(8, 262);
  // }
  // else {
  //   noTone(8);
  // }
  // delay(1000);
  // Serial.print(".");
  // delay(1000);
  // Serial.print(".");
  // delay(1000);
  // Serial.print(". ");
  //delay(2000);
  //Serial.println("Loop end.");
}

// Interrupt service routine watching port D
ISR(PCINT2_vect) {
  Serial.println("");
  Serial.println("INTERRUPT!");
  tiltState = PIND & 0b00010000;  // byte code for digitalRead(4)
  Serial.print("tiltState: ");
  Serial.println(tiltState);  

  // Set red and blue leds based on tiltState
  digitalWrite(RED_LED_PIN, !tiltState);
  digitalWrite(BLUE_LED_PIN, tiltState);


  lightState = PIND & 0b00100000;  // byte code for digitalRead(5)
  Serial.print("lightState: ");
  Serial.println(lightState);
  if (lightState) {
    tone(8, 262, 100);
  }
  // if (lightState == 1) {
  //   tone(8, 262);
  // }
  // else {
  //   noTone(8);
  // }

}