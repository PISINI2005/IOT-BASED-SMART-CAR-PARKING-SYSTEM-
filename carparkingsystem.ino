#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>

// LCD configuration
LiquidCrystal_I2C lcd(0x27, 20, 4); // Set the LCD address to 0x27 for a 20 chars and 4 line display

// Servo configuration
Servo gateServo;

// Pin configuration for sensors
#define ir_enter 3
#define ir_exit  4
#define ir_car1 5
#define ir_car2 6
#define ir_car3 7
#define ir_car4 8

// Variables to store sensor states
int S1 = 0, S2 = 0, S3 = 0, S4 = 0;
int prevS1 = 0, prevS2 = 0, prevS3 = 0, prevS4 = 0;
int slot = 4; // Total slots available

// Debounce variables
unsigned long lastDebounceTimeEnter = 0;
unsigned long lastDebounceTimeExit = 0;
unsigned long debounceDelay = 200; // Debounce delay time

// Car passing state variables
bool gateOpen = false;
bool carDetected = false;

// State machine states
enum State {
  IDLE,
  CAR_ENTERING,
  CAR_EXITING,
  WAITING_TO_CLOSE
};

State currentState = IDLE;

void setup() {
  Serial.begin(9600);
  
  // Initialize pins for sensors
  pinMode(ir_car1, INPUT);
  pinMode(ir_car2, INPUT);
  pinMode(ir_car3, INPUT);
  pinMode(ir_car4, INPUT);
  pinMode(ir_enter, INPUT);
  pinMode(ir_exit, INPUT);

  // Attach servo motor
  gateServo.attach(2);
  gateServo.write(90); // Initially close the gate

  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("    Car Parking   ");
  lcd.setCursor(0, 1);
  lcd.print("       System     ");
  delay(2000);
  lcd.clear();

  // Read initial sensor states and calculate available slots
  Read_Sensor();
  int total = S1 + S2 + S3 + S4;
  slot -= total;
}

void loop() {
  // Continuously read sensor states
  Read_Sensor();

  // Update LCD with parking status
  Update_LCD();

  // Handle state machine
  switch (currentState) {
    case IDLE:
      // Check for car entering
      if (digitalRead(ir_enter) == LOW && (millis() - lastDebounceTimeEnter > debounceDelay)) {
        lastDebounceTimeEnter = millis();
        if (slot > 0) {
          currentState = CAR_ENTERING;
          gateServo.write(180); // Open the gate
          carDetected = true;
        } else {
          lcd.setCursor(0, 0);
          lcd.print(" Sorry Parking Full ");
          delay(1500);
          lcd.clear();
        }
      }

      // Check for car exiting
      if (digitalRead(ir_exit) == LOW && (millis() - lastDebounceTimeExit > debounceDelay)) {
        lastDebounceTimeExit = millis();
        currentState = CAR_EXITING;
        gateServo.write(180); // Open the gate
        carDetected = true;
      }
      break;

    case CAR_ENTERING:
      // Check if car has fully entered (cleared the entry sensor)
      if (digitalRead(ir_enter) == HIGH && carDetected) {
        currentState = WAITING_TO_CLOSE;
        carDetected = false;
      }
      break;

    case CAR_EXITING:
      // Check if car has fully exited (cleared the exit sensor)
      if (digitalRead(ir_exit) == HIGH && carDetected) {
        currentState = WAITING_TO_CLOSE;
        carDetected = false;
      }
      break;

    case WAITING_TO_CLOSE:
      // Wait for both sensors to be cleared before closing the gate
      if (digitalRead(ir_enter) == HIGH && digitalRead(ir_exit) == HIGH) {
        gateServo.write(90); // Close the gate
        if (currentState == CAR_EXITING) {
          slot = constrain(slot + 1, 0, 4); // Increment slot count
        } else if (currentState == CAR_ENTERING) {
          slot = constrain(slot - 1, 0, 4); // Decrement slot count
        }
        currentState = IDLE;
      }
      break;
  }

  delay(100);
}

// Function to read sensor states
void Read_Sensor() {
  S1 = digitalRead(ir_car1) == LOW ? 1 : 0;
  S2 = digitalRead(ir_car2) == LOW ? 1 : 0;
  S3 = digitalRead(ir_car3) == LOW ? 1 : 0;
  S4 = digitalRead(ir_car4) == LOW ? 1 : 0;
}

// Function to update LCD with parking status
void Update_LCD() {
  if (slot == 0) {
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print("  No Space Left  ");
  } else {
    lcd.setCursor(0, 2);
    lcd.print("Available Slots: ");
    lcd.print(slot);
    lcd.print("     ");

    lcd.setCursor(0, 0);
    lcd.print("S1:");
    lcd.print(S1 == 1 ? "Full " : "Empty");
    lcd.print("  S2:");
    lcd.print(S2 == 1 ? "Full " : "Empty");

    lcd.setCursor(0, 1);
    lcd.print("S3:");
    lcd.print(S3 == 1 ? "Full " : "Empty");
    lcd.print("  S4:");
    lcd.print(S4 == 1 ? "Full " : "Empty");
  }
}
