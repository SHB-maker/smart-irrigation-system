#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// =====================================================
// SMART IRRIGATION - IoT EDUCATIONAL TEST BENCH
// Arduino UNO
// =====================================================

// ---------------- LCD ----------------
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ---------------- SOIL MOISTURE ----------------
const int SOIL_1_PIN = A0;
const int SOIL_2_PIN = A1;

// ---------------- WATER LEVEL SENSORS ----------------
// Float switches / digital level sensors
const int CLEAN_WATER_LEVEL_PIN   = A2;
const int RECYCLED_WATER_LEVEL_PIN = A3;

// ---------------- RELAYS ----------------
const int CLEAN_PUMP_RELAY    = 7;
const int RECYCLED_PUMP_RELAY = 8;
const int LIGHT_RELAY         = 9;

// Most relay modules are ACTIVE LOW
const int RELAY_ON  = LOW;
const int RELAY_OFF = HIGH;

// ---------------- ULTRASONIC SENSOR ----------------
const int TRIG_PIN = 2;
const int ECHO_PIN = 3;

// ---------------- SETTINGS ----------------

// Adjust this value after calibrating the soil sensors.
// Lower value = dry soil for many common sensors.
const int SOIL_DRY_THRESHOLD = 500;

// Minimum recycled water level required
// before using the recycled-water pump.
const bool WATER_AVAILABLE = HIGH;

// Presence distance in centimeters
const int PRESENCE_DISTANCE = 100;

// Irrigation duration
const unsigned long IRRIGATION_TIME = 5000;

// Time before checking soil again
const unsigned long IRRIGATION_INTERVAL = 30000;

// Display interval
const unsigned long DISPLAY_INTERVAL = 3000;

// ---------------- VARIABLES ----------------

unsigned long lastIrrigation = 0;
unsigned long lastDisplay = 0;

int soil1 = 0;
int soil2 = 0;

bool cleanWaterAvailable = false;
bool recycledWaterAvailable = false;

bool irrigationRunning = false;


// =====================================================
// SETUP
// =====================================================

void setup() {

  Serial.begin(9600);

  // Soil sensors
  pinMode(SOIL_1_PIN, INPUT);
  pinMode(SOIL_2_PIN, INPUT);

  // Water level sensors
  pinMode(CLEAN_WATER_LEVEL_PIN, INPUT_PULLUP);
  pinMode(RECYCLED_WATER_LEVEL_PIN, INPUT_PULLUP);

  // Relays
  pinMode(CLEAN_PUMP_RELAY, OUTPUT);
  pinMode(RECYCLED_PUMP_RELAY, OUTPUT);
  pinMode(LIGHT_RELAY, OUTPUT);

  // Turn everything OFF
  digitalWrite(CLEAN_PUMP_RELAY, RELAY_OFF);
  digitalWrite(RECYCLED_PUMP_RELAY, RELAY_OFF);
  digitalWrite(LIGHT_RELAY, RELAY_OFF);

  // Ultrasonic
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // LCD
  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("SMART IRRIGATION");

  lcd.setCursor(0, 1);
  lcd.print("IoT TEST BENCH");

  delay(2000);

  lcd.clear();

  Serial.println("================================");
  Serial.println(" SMART IRRIGATION TEST BENCH");
  Serial.println("================================");
}


// =====================================================
// LOOP
// =====================================================

void loop() {

  // Read sensors
  readSensors();

  // Manage lighting
  manageLighting();

  // Display information
  if (millis() - lastDisplay >= DISPLAY_INTERVAL) {

    lastDisplay = millis();

    displayInformation();
  }

  // Check irrigation
  if (!irrigationRunning &&
      millis() - lastIrrigation >= IRRIGATION_INTERVAL) {

    checkIrrigation();
  }

  delay(100);
}


// =====================================================
// READ SENSORS
// =====================================================

void readSensors() {

  soil1 = analogRead(SOIL_1_PIN);
  soil2 = analogRead(SOIL_2_PIN);

  // With INPUT_PULLUP:
  // LOW  = sensor activated
  // HIGH = sensor not activated
  //
  // Change the logic below if your level sensors
  // work differently.

  cleanWaterAvailable =
    digitalRead(CLEAN_WATER_LEVEL_PIN) == LOW;

  recycledWaterAvailable =
    digitalRead(RECYCLED_WATER_LEVEL_PIN) == LOW;
}


// =====================================================
// IRRIGATION MANAGEMENT
// =====================================================

void checkIrrigation() {

  int averageSoil = (soil1 + soil2) / 2;

  Serial.println();
  Serial.println("--- IRRIGATION CHECK ---");

  Serial.print("Soil 1: ");
  Serial.println(soil1);

  Serial.print("Soil 2: ");
  Serial.println(soil2);

  Serial.print("Average: ");
  Serial.println(averageSoil);

  // Soil is sufficiently wet
  if (averageSoil <= SOIL_DRY_THRESHOLD) {

    Serial.println("Soil moisture OK.");
    Serial.println("No irrigation required.");

    return;
  }

  // Soil needs water
  Serial.println("Soil is dry.");
  Serial.println("Irrigation required.");

  // ---------------------------------------------------
  // PRIORITY 1: RECYCLED WATER
  // ---------------------------------------------------

  if (recycledWaterAvailable) {

    Serial.println("Using RECYCLED WATER.");

    runPump(RECYCLED_PUMP_RELAY);

  }

  // ---------------------------------------------------
  // PRIORITY 2: CLEAN WATER
  // ---------------------------------------------------

  else if (cleanWaterAvailable) {

    Serial.println("Recycled water unavailable.");
    Serial.println("Using CLEAN WATER.");

    runPump(CLEAN_PUMP_RELAY);

  }

  // ---------------------------------------------------
  // NO WATER AVAILABLE
  // ---------------------------------------------------

  else {

    Serial.println("ERROR: No water available.");

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("NO WATER");

    lcd.setCursor(0, 1);
    lcd.print("CHECK RESERVOIRS");

    delay(2000);
  }
}


// =====================================================
// PUMP CONTROL
// =====================================================

void runPump(int pumpRelay) {

  irrigationRunning = true;

  Serial.println("Pump ON");

  digitalWrite(pumpRelay, RELAY_ON);

  unsigned long startTime = millis();

  while (millis() - startTime < IRRIGATION_TIME) {

    // Keep checking water level during irrigation
    readSensors();

    // Safety: stop pump if water disappears
    if (pumpRelay == RECYCLED_PUMP_RELAY &&
        !recycledWaterAvailable) {

      Serial.println("Recycled reservoir empty.");
      break;
    }

    if (pumpRelay == CLEAN_PUMP_RELAY &&
        !cleanWaterAvailable) {

      Serial.println("Clean water reservoir empty.");
      break;
    }

    delay(100);
  }

  digitalWrite(pumpRelay, RELAY_OFF);

  Serial.println("Pump OFF");

  irrigationRunning = false;

  lastIrrigation = millis();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("IRRIGATION DONE");

  delay(1500);
}


// =====================================================
// LIGHTING MANAGEMENT
// =====================================================

void manageLighting() {

  long distance = getDistance();

  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  if (distance > 0 && distance <= PRESENCE_DISTANCE) {

    digitalWrite(LIGHT_RELAY, RELAY_ON);

  } else {

    digitalWrite(LIGHT_RELAY, RELAY_OFF);
  }
}


// =====================================================
// ULTRASONIC DISTANCE
// =====================================================

long getDistance() {

  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);

  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);

  if (duration == 0) {
    return -1;
  }

  long distance = duration * 0.034 / 2;

  return distance;
}


// =====================================================
// LCD INFORMATION
// =====================================================

void displayInformation() {

  static int screen = 0;

  lcd.clear();

  // ---------------- SCREEN 1 ----------------

  if (screen == 0) {

    lcd.setCursor(0, 0);
    lcd.print("S1:");
    lcd.print(soil1);

    lcd.print(" S2:");
    lcd.print(soil2);

    lcd.setCursor(0, 1);

    int average = (soil1 + soil2) / 2;

    lcd.print("AVG:");
    lcd.print(average);

    if (average > SOIL_DRY_THRESHOLD) {
      lcd.print(" DRY");
    } else {
      lcd.print(" OK");
    }
  }

  // ---------------- SCREEN 2 ----------------

  else if (screen == 1) {

    lcd.setCursor(0, 0);
    lcd.print("CLEAN:");

    if (cleanWaterAvailable) {
      lcd.print("OK");
    } else {
      lcd.print("EMPTY");
    }

    lcd.setCursor(0, 1);
    lcd.print("RECYC:");

    if (recycledWaterAvailable) {
      lcd.print("OK");
    } else {
      lcd.print("EMPTY");
    }
  }

  // ---------------- SCREEN 3 ----------------

  else if (screen == 2) {

    lcd.setCursor(0, 0);
    lcd.print("SYSTEM STATUS");

    lcd.setCursor(0, 1);

    if (irrigationRunning) {
      lcd.print("IRRIGATION ON");
    } else {
      lcd.print("READY");
    }
  }

  screen++;

  if (screen > 2) {
    screen = 0;
  }
}
