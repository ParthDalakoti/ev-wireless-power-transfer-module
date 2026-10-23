#include <SPI.h> 
#include <MFRC522.h> 
#include <Servo.h> 
#include <Wire.h> 
#include <LiquidCrystal_I2C.h> 
 
// Pins 
#define SS_PIN 10 
#define RST_PIN 9 
#define IR1_PIN 2      // Entry sensor 
#define IR2_PIN 5      // Exit sensor 
#define RELAY_PIN 4 
#define SERVO_PIN 3 
 
#define CHARGING_TIME 120000   // 2 minutes 
#define RFID_WAIT 10000        // 10 seconds 
 
MFRC522 rfid(SS_PIN, RST_PIN); 
Servo gateServo; 
LiquidCrystal_I2C lcd(0x27, 16, 2); 
 
// Flags 
bool gateIsOpen = false; 
bool ir2Last = HIGH; 
 
bool isCharging = false; 
unsigned long chargeStart = 0; 
 
bool cardSeen = false; 
unsigned long cardTime = 0; 
 
void setup() { 
  Serial.begin(9600); 
  SPI.begin(); 
  rfid.PCD_Init(); 
 
  pinMode(IR1_PIN, INPUT); 
  pinMode(IR2_PIN, INPUT); 
  pinMode(RELAY_PIN, OUTPUT); 
  digitalWrite(RELAY_PIN, LOW); 
 
  gateServo.attach(SERVO_PIN); 
  gateServo.write(90);   // gate closed position 
 
  lcd.init(); 

  lcd.backlight(); 
 
  lcd.setCursor(0,0); 
  lcd.print("EV WIRELESS"); 
  lcd.setCursor(0,1); 
  lcd.print("CHARGING SYSTEM"); 
  delay(2500); 
 
  lcd.clear(); 
  lcd.setCursor(0,0); 
  lcd.print("EV CHARGING"); 
  lcd.setCursor(0,1); 
  lcd.print(">>>>>>>>>>>>>>>"); 
} 
 
void slowServo(int from, int to) { 
  if (from > to) { 
    for (int i = from; i >= to; i--) { 
      gateServo.write(i); 
      delay(15); 
    } 
  } else { 
    for (int i = from; i <= to; i++) { 
      gateServo.write(i); 
      delay(15); 
    } 
  } 
} 
 
void loop() { 
 
  // Gate open on car entry 
  if (digitalRead(IR1_PIN) == LOW && !gateIsOpen) { 
    slowServo(90, 0); 
    gateIsOpen = true; 
  } 
 
  // RFID Card check 
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) { 
 
    if (isCharging) { 
      // Busy message 
      lcd.clear(); 
      lcd.setCursor(0,0); 
      lcd.print("CHARGING"); 
      lcd.setCursor(0,1); 
      lcd.print("STOP BUZY"); 
      delay(1000); 

 
      // back to charging display 
      lcd.clear(); 
      lcd.setCursor(0,0); 
      lcd.print("CHARGING"); 
      lcd.setCursor(0,1); 
      int perc = map(millis() - chargeStart, 0, CHARGING_TIME, 0, 100); 
      if (perc > 100) perc = 100; 
      lcd.print("CHARGE: "); 
      lcd.print(perc); 
      lcd.print("%"); 
 
      rfid.PICC_HaltA(); 
      rfid.PCD_StopCrypto1(); 
      ir2Last = digitalRead(IR2_PIN); 
      goto endLoop;   // simple way to skip rest this time 
    } 
 
    // Normal card detection 
    lcd.clear(); 
    lcd.setCursor(0,0); 
    lcd.print("CARD DETECTED"); 
    lcd.setCursor(0,1); 
    lcd.print("PLEASE WAIT..."); 
 
    cardSeen = true; 
    cardTime = millis(); 
 
    rfid.PICC_HaltA(); 
    rfid.PCD_StopCrypto1(); 
  } 
 
  // Start charging after 10 sec wait 
  if (cardSeen && !isCharging && millis() - cardTime >= RFID_WAIT) { 
    digitalWrite(RELAY_PIN, HIGH); 
    chargeStart = millis(); 
    isCharging = true; 
 
    lcd.clear(); 
    lcd.setCursor(0,0); 
    lcd.print("CHARGING"); 
    lcd.setCursor(0,1); 
    lcd.print("CHARGE: 0%"); 
  } 
 
  // Update charging percentage 
  if (isCharging) { 
    unsigned long passed = millis() - chargeStart; 

    int percent = map(passed, 0, CHARGING_TIME, 0, 100); 
    if (percent > 100) percent = 100; 
 
    lcd.setCursor(0,1); 
    lcd.print("CHARGE: "); 
    lcd.print(percent); 
    lcd.print("%   "); 
  } 
 
  // Charging finished 
  if (isCharging && millis() - chargeStart >= CHARGING_TIME) { 
    digitalWrite(RELAY_PIN, LOW); 
    isCharging = false; 
    cardSeen = false; 
 
    lcd.clear(); 
    lcd.setCursor(0,0); 
    lcd.print("CHARGING DONE"); 
    lcd.setCursor(0,1); 
    lcd.print("100%"); 
 
    delay(3000); 
 
    lcd.clear(); 
    lcd.setCursor(0,0); 
    lcd.print("EV CHARGING"); 
    lcd.setCursor(0,1); 
    lcd.print(">>>>>>>>>>>>>>>"); 
  } 
 
  // Gate close on exit 
  bool ir2Now = digitalRead(IR2_PIN); 
  if (ir2Last == LOW && ir2Now == HIGH && gateIsOpen) { 
    slowServo(0, 90); 
    gateIsOpen = false; 
  } 
  ir2Last = ir2Now; 
 
endLoop:; 
}