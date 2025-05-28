String command;

// Regular LED pins
int redLED = 13;
int yellowLED = 12;
int greenLED = 11;

// RGB LED pins
int rgbRedPin = 6;
int rgbGreenPin = 5;
int rgbBluePin = 4;

// Piezo speaker pin
int piezoPin = 8;

// Regular LED control variables
unsigned long previousMillis[3] = {0, 0, 0};
int ledState[3] = {LOW, LOW, LOW};
int blinkInterval[3] = {0, 0, 0}; // Red, Yellow, Green

// RGB LED control variables
int rgbValues[3] = {0, 0, 0}; // R, G, B values (0-255)
unsigned long rgbPreviousMillis = 0;
int rgbBlinkInterval = 0;
bool rgbState = false;
int rgbBlinkValues[3] = {0, 0, 0}; // Values to use when blinking
bool rgbFading = false;
int fadeDirection = 1;
int fadeSpeed = 5;
unsigned long lastFadeTime = 0;

// Sound control variables
unsigned long soundStartTime = 0;
int soundDuration = 0;
int soundFrequency = 0;
bool soundPlaying = false;
int soundPattern = 0; // 0=off, 1=beep, 2=alarm, 3=melody, 4=tone
unsigned long lastSoundTime = 0;
int melodyStep = 0;

// Melody notes (simple tune)
int melody[] = {262, 294, 330, 349, 392, 440, 494, 523}; // C4 to C5
int melodyLength = 8;

void setup() {
  Serial.begin(9600);
  
  // Regular LEDs
  pinMode(redLED, OUTPUT);
  pinMode(yellowLED, OUTPUT);
  pinMode(greenLED, OUTPUT);
  
  // RGB LED pins
  pinMode(rgbRedPin, OUTPUT);
  pinMode(rgbGreenPin, OUTPUT);
  pinMode(rgbBluePin, OUTPUT);
  
  // Piezo
  pinMode(piezoPin, OUTPUT);
  
  Serial.println("Ready for LED, RGB, and sound commands");
}

void loop() {
  // Check for incoming serial command
  if (Serial.available()) {
    command = Serial.readStringUntil('\n');
    command.trim();
    processCommand(command);
  }
  
  unsigned long currentMillis = millis();
  
  // Handle regular LED blinking
  handleBlink(redLED, 0, currentMillis);
  handleBlink(yellowLED, 1, currentMillis);
  handleBlink(greenLED, 2, currentMillis);
  
  // Handle RGB LED effects
  handleRGBEffects(currentMillis);
  
  // Handle sound patterns
  handleSound(currentMillis);
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

void handleRGBEffects(unsigned long currentMillis) {
  // Handle RGB blinking
  if (rgbBlinkInterval > 0) {
    if (currentMillis - rgbPreviousMillis >= rgbBlinkInterval) {
      rgbPreviousMillis = currentMillis;
      rgbState = !rgbState;
      if (rgbState) {
        setRGBColor(rgbBlinkValues[0], rgbBlinkValues[1], rgbBlinkValues[2]);
      } else {
        setRGBColor(0, 0, 0);
      }
    }
  }
  
  // Handle RGB fading
  if (rgbFading && (currentMillis - lastFadeTime >= 50)) {
    lastFadeTime = currentMillis;
    
    // Simple breathing effect on the current color
    static int brightness = 0;
    brightness += fadeDirection * fadeSpeed;
    
    if (brightness >= 255) {
      brightness = 255;
      fadeDirection = -1;
    } else if (brightness <= 0) {
      brightness = 0;
      fadeDirection = 1;
    }
    
    int r = map(rgbValues[0] * brightness, 0, 255 * 255, 0, 255);
    int g = map(rgbValues[1] * brightness, 0, 255 * 255, 0, 255);
    int b = map(rgbValues[2] * brightness, 0, 255 * 255, 0, 255);
    
    setRGBColor(r, g, b);
  }
}

void setRGBColor(int r, int g, int b) {
  analogWrite(rgbRedPin, r);
  analogWrite(rgbGreenPin, g);
  analogWrite(rgbBluePin, b);
}

void handleSound(unsigned long currentMillis) {
  switch (soundPattern) {
    case 1: // Single beep
      if (soundPlaying && (currentMillis - soundStartTime >= soundDuration)) {
        noTone(piezoPin);
        soundPlaying = false;
        soundPattern = 0;
      }
      break;
      
    case 2: // Alarm pattern (alternating tones)
      if (currentMillis - lastSoundTime >= 500) {
        lastSoundTime = currentMillis;
        if (soundPlaying) {
          noTone(piezoPin);
          soundPlaying = false;
        } else {
          tone(piezoPin, (melodyStep % 2 == 0) ? 800 : 1000);
          soundPlaying = true;
          melodyStep++;
        }
      }
      break;
      
    case 3: // Melody
      if (currentMillis - lastSoundTime >= 300) {
        lastSoundTime = currentMillis;
        noTone(piezoPin);
        if (melodyStep < melodyLength) {
          tone(piezoPin, melody[melodyStep]);
          melodyStep++;
        } else {
          soundPattern = 0; // Stop after one play
          melodyStep = 0;
        }
      }
      break;
      
    case 4: // Continuous tone
      // Tone continues until stopped
      break;
  }
}

void processCommand(String cmd) {
  cmd.toLowerCase();
  
  // Process regular LED commands
  if (cmd.indexOf("red") != -1 && cmd.indexOf("rgb") == -1) {
    updateLED(cmd, redLED, 0);
  }
  if (cmd.indexOf("yellow") != -1) {
    updateLED(cmd, yellowLED, 1);
  }
  if (cmd.indexOf("green") != -1 && cmd.indexOf("rgb") == -1) {
    updateLED(cmd, greenLED, 2);
  }
  
  // Process RGB LED commands
  if (cmd.indexOf("rgb") != -1) {
    updateRGB(cmd);
  }
  
  // Process sound commands
  if (cmd.indexOf("sound off") != -1 || cmd.indexOf("stop sound") != -1 || cmd.indexOf("quiet") != -1) {
    stopSound();
  } else if (cmd.indexOf("beep") != -1) {
    playBeep(1000, 300); // 1kHz for 300ms
  } else if (cmd.indexOf("alarm") != -1) {
    playAlarm();
  } else if (cmd.indexOf("melody") != -1) {
    playMelody();
  } else if (cmd.indexOf("tone high") != -1 || cmd.indexOf("high tone") != -1) {
    playTone(1500);
  } else if (cmd.indexOf("tone low") != -1 || cmd.indexOf("low tone") != -1) {
    playTone(300);
  } else if (cmd.indexOf("buzz") != -1) {
    playTone(100);
  } else if (cmd.indexOf("tone") != -1) {
    playTone(800); // Default tone
  }
  
  Serial.println("Command processed: " + cmd);
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
    blinkInterval[index] = 200;
  } else if (cmd.indexOf("slow") != -1) {
    blinkInterval[index] = 2000;
  } else if (cmd.indexOf("blink") != -1) {
    blinkInterval[index] = 1000;
  }
}

void updateRGB(String cmd) {
  // Stop any current RGB effects
  rgbBlinkInterval = 0;
  rgbFading = false;
  
  if (cmd.indexOf("off") != -1) {
    setRGBColor(0, 0, 0);
    rgbValues[0] = 0; rgbValues[1] = 0; rgbValues[2] = 0;
  } else if (cmd.indexOf("white") != -1) {
    setRGBColor(255, 255, 255);
    rgbValues[0] = 255; rgbValues[1] = 255; rgbValues[2] = 255;
  } else if (cmd.indexOf("red") != -1) {
    setRGBColor(255, 0, 0);
    rgbValues[0] = 255; rgbValues[1] = 0; rgbValues[2] = 0;
  } else if (cmd.indexOf("green") != -1) {
    setRGBColor(0, 255, 0);
    rgbValues[0] = 0; rgbValues[1] = 255; rgbValues[2] = 0;
  } else if (cmd.indexOf("blue") != -1) {
    setRGBColor(0, 0, 255);
    rgbValues[0] = 0; rgbValues[1] = 0; rgbValues[2] = 255;
  } else if (cmd.indexOf("yellow") != -1) {
    setRGBColor(255, 255, 0);
    rgbValues[0] = 255; rgbValues[1] = 255; rgbValues[2] = 0;
  } else if (cmd.indexOf("purple") != -1 || cmd.indexOf("magenta") != -1) {
    setRGBColor(255, 0, 255);
    rgbValues[0] = 255; rgbValues[1] = 0; rgbValues[2] = 255;
  } else if (cmd.indexOf("cyan") != -1) {
    setRGBColor(0, 255, 255);
    rgbValues[0] = 0; rgbValues[1] = 255; rgbValues[2] = 255;
  } else if (cmd.indexOf("orange") != -1) {
    setRGBColor(255, 165, 0);
    rgbValues[0] = 255; rgbValues[1] = 165; rgbValues[2] = 0;
  } else if (cmd.indexOf("pink") != -1) {
    setRGBColor(255, 192, 203);
    rgbValues[0] = 255; rgbValues[1] = 192; rgbValues[2] = 203;
  }
  
  // Handle RGB effects
  if (cmd.indexOf("blink") != -1) {
    rgbBlinkValues[0] = rgbValues[0];
    rgbBlinkValues[1] = rgbValues[1];
    rgbBlinkValues[2] = rgbValues[2];
    
    if (cmd.indexOf("fast") != -1) {
      rgbBlinkInterval = 200;
    } else if (cmd.indexOf("slow") != -1) {
      rgbBlinkInterval = 2000;
    } else {
      rgbBlinkInterval = 1000;
    }
    rgbState = true;
  } else if (cmd.indexOf("fade") != -1 || cmd.indexOf("breathe") != -1) {
    rgbFading = true;
    fadeDirection = 1;
    lastFadeTime = millis();
    
    if (cmd.indexOf("fast") != -1) {
      fadeSpeed = 10;
    } else if (cmd.indexOf("slow") != -1) {
      fadeSpeed = 2;
    } else {
      fadeSpeed = 5;
    }
  }
}

void playBeep(int frequency, int duration) {
  tone(piezoPin, frequency);
  soundStartTime = millis();
  soundDuration = duration;
  soundPlaying = true;
  soundPattern = 1;
}

void playAlarm() {
  soundPattern = 2;
  melodyStep = 0;
  lastSoundTime = millis();
  tone(piezoPin, 800);
  soundPlaying = true;
}

void playMelody() {
  soundPattern = 3;
  melodyStep = 0;
  lastSoundTime = millis();
  tone(piezoPin, melody[0]);
}

void playTone(int frequency) {
  tone(piezoPin, frequency);
  soundPattern = 4;
  soundPlaying = true;
}

void stopSound() {
  noTone(piezoPin);
  soundPattern = 0;
  soundPlaying = false;
  melodyStep = 0;
}