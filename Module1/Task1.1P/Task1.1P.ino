// Read only variables, sensor pin  and base temp
const int sensorPin = A0;
const float baseTempC = 28.0;

void setup() {
  pinMode(2, LOW);  // red pin
  pinMode(3, LOW);  // blue pin
  Serial.begin(9600);  // Begin serial reading
}

void loop() {
  // Get ADC read from sensor pin
  float sensorRead = analogRead(sensorPin);

  // Convert ADC to voltage
  float voltage = (sensorRead/1024) * 5.0;

  // Convert voltage to temp celsius 
  float tempC = (voltage - 0.5) * 100;

  // Output temp value
  Serial.print("Temperature: ");
  Serial.println(tempC);
  
  // If temp is below baseline, turn on blue light
  if (tempC < baseTempC) {
    digitalWrite(2, LOW);
    digitalWrite(3, HIGH);
  }
  else {  // else turn on red light
    digitalWrite(3, LOW);
    digitalWrite(2, HIGH);
  }
    
  delay(500);  // 0.5 second delay
}
