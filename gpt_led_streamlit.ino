String command;
int redLED = 13;
int yellowLED = 12;
int greenLED = 11;

unsigned long previousMillis[3] = {0, 0, 0};
int ledState[3] = {LOW, LOW, LOW};
int blinkInterval[3] = {0, 0, 0};  // Red, Yellow, Green

void setup() {
  Serial.begin(9600);
  pinMode(redLED, OUTPUT);
  pinMode(yellowLED, OUTPUT);
  pinMode(greenLED, OUTPUT);
  Serial.println("Ready for commands");
}

void loop() {
  // Check for incoming serial command
  if (Serial.available()) {
    command = Serial.readStringUntil('\n');
    command.trim();
    processCommand(command);
  }

  unsigned long currentMillis = millis();
  handleBlink(redLED, 0, currentMillis);
  handleBlink(yellowLED, 1, currentMillis);
  handleBlink(greenLED, 2, currentMillis);
}

void handleBlink(int pin, int index, unsigned long currentMillis) {
  if (blinkInterval[index] > 0) {
    if (currentMillis - previousMillis[index] >= blinkInterval[index]) {
      previousMillis[index] = currentMillis;
      ledState[index] = !ledState[index];
      digitalWrite(pin, ledState[index]);
    }
  }
}

void processCommand(String cmd) {
  cmd.toLowerCase();

  if (cmd.indexOf("red") != -1) {
    updateLED(cmd, redLED, 0);
  }
  if (cmd.indexOf("yellow") != -1) {
    updateLED(cmd, yellowLED, 1);
  }
  if (cmd.indexOf("green") != -1) {
    updateLED(cmd, greenLED, 2);
  }
}

void updateLED(String cmd, int pin, int index) {
  if (cmd.indexOf("off") != -1) {
    digitalWrite(pin, LOW);
    blinkInterval[index] = 0;
    ledState[index] = LOW;
  } else if (cmd.indexOf("on") != -1) {
    digitalWrite(pin, HIGH);
    blinkInterval[index] = 0;
    ledState[index] = HIGH;
  } else if (cmd.indexOf("fast") != -1) {
    blinkInterval[index] = 300;
  } else if (cmd.indexOf("slow") != -1) {
    blinkInterval[index] = 2000;
  } else if (cmd.indexOf("blink") != -1) {
    blinkInterval[index] = 1000;
  }
}
