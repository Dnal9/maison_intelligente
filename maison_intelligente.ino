#include <LiquidCrystal_I2C.h>
#include <AccelStepper.h>


const char* NUM_ETUDIANT = "2413335";

// LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);
unsigned long lastLcdUpdate = 0;
const unsigned long lcdUpdateInterval = 100;

// Capteur HC-SR04 
const int trigPin = 6;
const int echoPin = 7;
long distance = 0;
unsigned long lastDistanceRead = 0;
const unsigned long distanceInterval = 50;

//  Moteur 
#define MOTOR_INTERFACE_TYPE 4
#define IN_1 31
#define IN_2 33
#define IN_3 35
#define IN_4 37
AccelStepper myStepper(MOTOR_INTERFACE_TYPE, IN_1, IN_3, IN_2, IN_4);

const int STEPS_PAR_180 = 2048;
int angle = 90;
int lastAngle = 90;
bool moteurAttache = false;


unsigned long lastSerialTime = 0;
const unsigned long serialInterval = 100;

// === États système ===
enum Etat { TROP_PRES, TROP_LOIN, VISER };
Etat etatActuel = TROP_LOIN;

// === Alarme ===
const int buzzerPin = 2;
const int redPin = 10;
const int bluePin = 9;

bool alarmeActive = false;
unsigned long lastAlarmeTrigger = 0;
unsigned long lastGyroBlink = 0;
bool gyroState = false;

// === Initialisation ===
void initialiserLCD() {
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print(NUM_ETUDIANT);
  lcd.setCursor(0, 1);
  lcd.print("Labo 4B");
  delay(2000);
  lcd.clear();
}

void initialiserMoteur() {
  myStepper.setMaxSpeed(500);
  myStepper.setAcceleration(100);
  myStepper.setSpeed(200);
  myStepper.setCurrentPosition(0);
  myStepper.moveTo(0);
  detacherMoteur();
}

void initialiserCapteur() {
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
}

void initialiserAlarme() {
  pinMode(buzzerPin, OUTPUT);
  pinMode(redPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
  digitalWrite(buzzerPin, LOW);
  digitalWrite(redPin, LOW);
  digitalWrite(bluePin, LOW);
}

// === Moteur ===
void rattacherMoteur() {
  digitalWrite(IN_1, HIGH);
  digitalWrite(IN_2, HIGH);
  digitalWrite(IN_3, HIGH);
  digitalWrite(IN_4, HIGH);
  moteurAttache = true;
}

void detacherMoteur() {
  digitalWrite(IN_1, LOW);
  digitalWrite(IN_2, LOW);
  digitalWrite(IN_3, LOW);
  digitalWrite(IN_4, LOW);
  moteurAttache = false;
}


long lireDistanceCM() {
  digitalWrite(trigPin, LOW); delayMicroseconds(2);
  digitalWrite(trigPin, HIGH); delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duree = pulseIn(echoPin, HIGH, 30000);
  long d = duree * 0.034 / 2;
  if (d < 2 || d > 400) return distance;
  return d;
}

int distanceToAngle(long d) {
  return constrain(map(d, 30, 60, 10, 170), 10, 170);
}

int angleToStep(int a) {
  return map(a, 0, 180, -STEPS_PAR_180 / 2, STEPS_PAR_180 / 2);
}


void tacheLireDistance() {
  if (millis() - lastDistanceRead >= distanceInterval) {
    distance = lireDistanceCM();
    lastDistanceRead = millis();
  }
}

void tacheMettreAJourEtat() {
  if (distance < 30) {
    etatActuel = TROP_PRES;
  } else if (distance > 60) {
    etatActuel = TROP_LOIN;
  } else {
    etatActuel = VISER;
    angle = distanceToAngle(distance);
  }
}

void tacheControleMoteur() {
  if (etatActuel == VISER && angle != lastAngle) {
    rattacherMoteur();
    int cible = angleToStep(angle);
    myStepper.moveTo(cible);
    lastAngle = angle;
  }

  if (myStepper.distanceToGo() != 0) {
    myStepper.run();
  } else if (moteurAttache) {
    detacherMoteur();
  }
}

void tacheAffichageLCD() {
  if (millis() - lastLcdUpdate >= lcdUpdateInterval) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Dist: ");
    lcd.print(distance);
    lcd.print("cm");

    lcd.setCursor(0, 1);
    switch (etatActuel) {
      case TROP_PRES: lcd.print("obj : Trop pres"); break;
      case TROP_LOIN: lcd.print("obj : Trop loin"); break;
      case VISER:
        lcd.print("Angle: ");
        lcd.print(angle);
        break;
    }

    lastLcdUpdate = millis();
  }
}

void tacheSerie() {
  if (millis() - lastSerialTime >= serialInterval) {
    Serial.print("etd:");
    Serial.print(NUM_ETUDIANT);
    Serial.print(",dist:");
    Serial.print(distance);
    Serial.print(",deg:");
    Serial.println(angle);
    lastSerialTime = millis();
  }
}

void tacheAlarme() {
  unsigned long maintenant = millis();

  if (distance <= 15) {
    alarmeActive = true;
    lastAlarmeTrigger = maintenant;
    digitalWrite(buzzerPin, HIGH);

    if (maintenant - lastGyroBlink >= 250) {
      gyroState = !gyroState;
      digitalWrite(redPin, gyroState ? HIGH : LOW);
      digitalWrite(bluePin, gyroState ? LOW : HIGH);
      lastGyroBlink = maintenant;
    }
  } else if (alarmeActive && maintenant - lastAlarmeTrigger >= 3000) {
    alarmeActive = false;
    digitalWrite(buzzerPin, LOW);
    digitalWrite(redPin, LOW);
    digitalWrite(bluePin, LOW);
  }
}


void setup() {
  Serial.begin(9600);
  initialiserLCD();
  initialiserMoteur();
  initialiserCapteur();
  initialiserAlarme();
  


}
// main
void loop() {
  tacheLireDistance();
  tacheMettreAJourEtat();
  tacheControleMoteur();
  tacheAffichageLCD();
  tacheSerie();
  tacheAlarme();
}
