#include <ESP8266WiFi.h> 
#include <Servo.h>

// Pin Definitions for NodeMCU
#define THERMISTOR_PIN A0
#define LDR_PIN        D1
#define IR_PIN         D2
#define FAN_PIN        D3   // D3 for Analog Speed Control (PWM)
#define BUTTON_PIN     D4
#define LED_PIN        D5
#define BUZZER_PIN     D6   // Your single buzzer
#define SERVO_PIN      D7   

Servo doorServo;

// Musical Note Frequencies (Hz)
#define NOTE_E4  330
#define NOTE_G4  392
#define NOTE_A4  440
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_D5  587
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_G5  784
#define NOTE_A5  880

#define Q    480       
#define E    (Q / 2)   
#define HOLD 1056      

// System State & Calibration Variables
int baselineTemp = 0;
const int TEMP_SENSITIVITY = 80; 
bool securityMode = false;   

// Tracking Variables
bool isDoorOpen = false;     
unsigned long doorOpenMillis = 0; 
bool lastFanState = false;   
bool fanActive = false;      // Hysteresis lock for the fan

unsigned long pressStartTime = 0;
unsigned long lastReleaseTime = 0;
int pressCount = 0;
bool lastButtonState = HIGH;
bool longPressTriggered = false;
int lastNoteFreq = 0;

// Standard Tone Generator
void playTone(int frequency, int durationMs) {
  if (frequency > 0) {
    long halfPeriod = 1000000L / (frequency * 2);
    unsigned long startTime = millis();
    while (millis() - startTime < (unsigned long)durationMs) {
      digitalWrite(BUZZER_PIN, HIGH);
      delayMicroseconds(halfPeriod);
      digitalWrite(BUZZER_PIN, LOW);
      delayMicroseconds(halfPeriod);
    }
  } else {
    delay(durationMs);
  }
  digitalWrite(BUZZER_PIN, LOW);
  delay(25);
  yield();
}

// Smooth Gliding Tone Generator
void playGlideNote(int targetFreq, int durationMs, int gapAfterMs = 5) {
  if (targetFreq > 0) {
    unsigned long startTime = millis();
    unsigned long glideTime = (lastNoteFreq > 0 && lastNoteFreq != targetFreq) ? 45 : 0;
    int startFreq = (lastNoteFreq > 0) ? lastNoteFreq : targetFreq;

    while (millis() - startTime < (unsigned long)durationMs) {
      unsigned long elapsed = millis() - startTime;
      int currentFreq = targetFreq;

      if (elapsed < glideTime) {
        currentFreq = startFreq + ((long)(targetFreq - startFreq) * elapsed) / glideTime;
      }

      long halfPeriod = 1000000L / (currentFreq * 2);
      digitalWrite(BUZZER_PIN, HIGH);
      delayMicroseconds(halfPeriod);
      digitalWrite(BUZZER_PIN, LOW);
      delayMicroseconds(halfPeriod);
    }
    lastNoteFreq = targetFreq;
  } else {
    delay(durationMs);
    lastNoteFreq = 0; 
  }
  digitalWrite(BUZZER_PIN, LOW);
  if (gapAfterMs > 0) delay(gapAfterMs); 
  yield();
}

// Classic Airtel Ringtone 
void playAirtelTone() {
  lastNoteFreq = 0; 

  playGlideNote(NOTE_A4, Q, 12);
  playGlideNote(NOTE_A4, E, 12);
  playGlideNote(NOTE_A4, E, 12);
  playGlideNote(NOTE_A4, Q, 2);
  playGlideNote(NOTE_E5, E, 2);
  playGlideNote(0, E, 0);        
  playGlideNote(NOTE_B4, E, 2);
  playGlideNote(NOTE_C5, E, 2);
  playGlideNote(NOTE_B4, E, 2);
  playGlideNote(NOTE_A4, E, 2);
  playGlideNote(NOTE_G4, Q, 2);
  playGlideNote(NOTE_B4, Q, 10);

  playGlideNote(NOTE_A4, E, 2);
  playGlideNote(NOTE_B4, E, 2);
  playGlideNote(NOTE_A4, E, 2);
  playGlideNote(NOTE_G4, E, 2);
  playGlideNote(NOTE_A4, E, 2);
  playGlideNote(NOTE_B4, E, 2);
  playGlideNote(NOTE_C5, E, 2);
  playGlideNote(0, E, 0);        
  playGlideNote(NOTE_B4, HOLD, 2);
  playGlideNote(NOTE_A4, E, 2);
  playGlideNote(NOTE_G4, E, 2);
  playGlideNote(NOTE_E4, HOLD, 15);

  playGlideNote(NOTE_A5, Q, 2);
  playGlideNote(NOTE_E5, Q, 2);
  playGlideNote(NOTE_G5, Q, 2);
  playGlideNote(NOTE_E5, E, 2);
  playGlideNote(0, E, 0);        
  playGlideNote(NOTE_E5, Q, 2);
  playGlideNote(NOTE_D5, Q, 2);
  playGlideNote(NOTE_C5, Q, 2);
  playGlideNote(NOTE_A4, Q, 10);
  
  playGlideNote(NOTE_C5, Q, 2);
  playGlideNote(NOTE_D5, Q, 2);
  playGlideNote(NOTE_E5, Q, 2);
  playGlideNote(NOTE_F5, E, 2);
}

// Servo Movement
void moveDoor(int angle, int holdTimeMs) {
  doorServo.attach(SERVO_PIN, 500, 2400);
  doorServo.write(angle);
  delay(holdTimeMs);
  doorServo.detach();
}

void setup() {
  Serial.begin(115200);
  
  // FORCE WI-FI RADIO OFF FOR POWER STABILITY
  WiFi.mode(WIFI_OFF);
  WiFi.forceSleepBegin();
  delay(10);
  
  pinMode(LDR_PIN, INPUT);
  pinMode(IR_PIN, INPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(FAN_PIN, OUTPUT);
  
  // Sets PWM to 25kHz so the physical motor coils do not whine or buzz!
  analogWriteFreq(25000); 
  
  digitalWrite(LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);
  analogWrite(FAN_PIN, 0); // Ensure fan is fully off at boot

  // Startup Chime & Door Initialize
  playTone(NOTE_C5, 120);
  playTone(NOTE_E5, 120);
  playTone(NOTE_G5, 200);
  moveDoor(20, 500); 
  isDoorOpen = false;

  baselineTemp = analogRead(THERMISTOR_PIN);
}

void loop() {
  handleButton();

  // ----- AUTO-CLOSE DOOR 5 SECOND TIMER -----
  if (isDoorOpen && (millis() - doorOpenMillis >= 5000)) {
    digitalWrite(LED_PIN, HIGH);
    playTone(NOTE_C5, 200); 
    digitalWrite(LED_PIN, LOW);
    playTone(NOTE_E5, 300); 
    moveDoor(20, 600);      
    isDoorOpen = false;     
  }

  // Read sensors globally
  int tempDiff = abs(analogRead(THERMISTOR_PIN) - baselineTemp);
  bool isDark = (digitalRead(LDR_PIN) == HIGH); 
  bool motionDetected = (digitalRead(IR_PIN) == LOW);

  // ----- INTRUDER ALARM (RAPID BURST) -----
  if (securityMode && motionDetected) {
    for (int i = 0; i < 6; i++) {
      digitalWrite(LED_PIN, HIGH);
      playTone(1500, 80);  // "tu"
      digitalWrite(LED_PIN, LOW);
      playTone(0, 50);     // tiny pause
    }
    delay(400); 
  } 
  // ----- NORMAL LIGHT CONTROL -----
  else {
    if (isDark || (tempDiff > TEMP_SENSITIVITY)) {
      digitalWrite(LED_PIN, HIGH);
    } else {
      digitalWrite(LED_PIN, LOW);
    }
  }

  // ----- FAN ANALOG CONTROL (JITTER-FREE HYSTERESIS) -----
  // Turns ON when hot, but won't turn OFF until it cools down by 15 points
  if (tempDiff > TEMP_SENSITIVITY) {
    fanActive = true; 
  } else if (tempDiff < (TEMP_SENSITIVITY - 15)) {
    fanActive = false;
  }

  int fanSpeed = 0;
  if (fanActive) {
    // Fan spins smoothly based on heat
    fanSpeed = map(tempDiff, TEMP_SENSITIVITY - 15, TEMP_SENSITIVITY + 100, 400, 1023);
    fanSpeed = constrain(fanSpeed, 400, 1023);
  }
  
  analogWrite(FAN_PIN, fanSpeed);

  // ----- FAN QUICK-BEEP TRACKER -----
  if (fanActive && !lastFanState) {
    // Beeps EXACTLY ONCE when the fan crosses the threshold!
    playGlideNote(600, 80, 20);  
    playGlideNote(1400, 150, 0); 
  }
  lastFanState = fanActive;
}

// ----- PHYSICAL BUTTON CONTROLS -----
void handleButton() {
  bool currentButtonState = digitalRead(BUTTON_PIN);

  if (currentButtonState == LOW && lastButtonState == HIGH) {
    delay(30); 
    pressStartTime = millis();
    longPressTriggered = false;
  }

  // LONG PRESS (Hold 1s) -> Toggle System Arm/Disarm
  if (currentButtonState == LOW && !longPressTriggered) {
    if (millis() - pressStartTime > 800) {
      longPressTriggered = true;
      pressCount = 0; 
      
      if (!securityMode) {
        securityMode = true; 
        playAirtelTone();    
      } else {
        securityMode = false; 
        playTone(1000, 120);
        playTone(600, 180);
      }
    }
  }

  if (currentButtonState == HIGH && lastButtonState == LOW) {
    delay(30); 
    if (!longPressTriggered) {
      pressCount++;
      lastReleaseTime = millis();
    }
  }

  lastButtonState = currentButtonState;

  if (pressCount > 0 && (millis() - lastReleaseTime > 400)) {
    if (pressCount == 1) {
      if (!isDoorOpen) {
        digitalWrite(LED_PIN, HIGH);
        playTone(NOTE_E5, 250); 
        digitalWrite(LED_PIN, LOW);
        playTone(NOTE_C5, 400); 
        moveDoor(110, 600); 
        isDoorOpen = true;  
        doorOpenMillis = millis(); 
      } else {
        digitalWrite(LED_PIN, HIGH);
        playTone(NOTE_C5, 200); 
        digitalWrite(LED_PIN, LOW);
        playTone(NOTE_E5, 300); 
        moveDoor(20, 600);  
        isDoorOpen = false; 
      }
    } 
    else if (pressCount >= 2) {
      securityMode = !securityMode;
      if (securityMode) {
        playTone(600, 120);
        playTone(1000, 180);
      } else {
        playTone(1000, 120);
        playTone(600, 180);
      }
    }
    pressCount = 0; 
  }
}