#include <SPI.h>
#include <MFRC522.h>
#include <Keypad.h>        // Include the Keypad library
#include <LiquidCrystal.h> // Include the LiquidCrystal library

// =================== RFID Configuration ===================
#define SS_PIN 10
#define RST_PIN 9
MFRC522 mfrc522(SS_PIN, RST_PIN);

// =================== Keypad Configuration ===================
const byte ROWS = 4; // Four rows
const byte COLS = 4; // Four columns
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {2, 3, 4, 5};      // Connect to the row pinouts of the keypad
byte colPins[COLS] = {6, 7, 8, A0};     // Connect to the column pinouts of the keypad

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// =================== LCD Configuration ===================
// Adjust the pin numbers according to your wiring
const int rs = 12, en = 11, d4 = 10, d5 = A1, d6 = A2, d7 = A3;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// =================== Data Storage ===================
String cardUIDs[10];      // Array to store card UIDs (up to 10 cards)
float cardBalances[10];  // Array to store card balances
int totalCards = 0;

// =================== Setup Function ===================
void setup() {
  // Initialize Serial Communication
  Serial.begin(9600);
  
  // Initialize LCD
  lcd.begin(16, 2); // 16 columns and 2 rows
  lcd.print("JAVA Food Court");
  lcd.setCursor(0,1);
  lcd.print("System Init...");
  delay(2000); // Wait for 2 seconds
  
  // Initialize SPI and RFID
  SPI.begin();
  mfrc522.PCD_Init();
  
  // Display the main menu
  printUserMenu();
}

// =================== Loop Function ===================
void loop() {
  int action = 0;
  
  // Wait for user to press a key corresponding to menu options
  while (action == 0) {
    action = getKeypadInput();
  }

  switch (action) {
    case 1:  // Register New Card
      executeRegisterNewCard();
      break;
    case 2:  // View Balance
      executeViewBalance();
      break;
    case 3:  // Add Balance
      executeAddBalance();
      break;
    case 4:  // Deduct Balance
      executeDeductBalance();
      break;
    default:
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Invalid Option");
      delay(2000);
      break;
  }

  // After action is complete, show the menu again
  printUserMenu();
}

// =================== Display Functions ===================

// Function to display the user menu on LCD
void printUserMenu() {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("User Menu:");
  lcd.setCursor(0,1);
  lcd.print("1.Reg 2.View");
  // Optionally, scroll or display more options in a timed manner
  delay(1000); // Wait for 1 second before updating the menu
  lcd.setCursor(0,1);
  lcd.print("3.Add 4.Deduct");
}

// =================== Input Functions ===================

// Function to get keypad input for menu selection
int getKeypadInput() {
  char key = keypad.getKey();
  if (key) {
    Serial.print("Key Pressed: ");
    Serial.println(key);
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Key Pressed: ");
    lcd.print(key);
    delay(1000); // Display for 1 second
    if (key >= '1' && key <= '4') {
      return key - '0';  // Convert char digit to integer
    } else {
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Press 1-4");
      delay(1000);
    }
  }
  return 0; // No valid input yet
}

// =================== Helper Functions ===================

// Function to find the index of a card based on UID
int findCardIndex(String uid) {
  for (int i = 0; i < totalCards; i++) {
    if (uid == cardUIDs[i]) {
      return i;
    }
  }
  return -1;
}

// =================== Action Functions ===================

// Function to register a new card
void executeRegisterNewCard() {
  if (totalCards < 10) { // Limit the number of cards to 10
    String newUID = "";
    float initialBalance = 0.0;

    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Scan New Card");
    lcd.setCursor(0,1);
    lcd.print("To Register");

    // Wait for a new card
    while (!mfrc522.PICC_IsNewCardPresent()) {
      delay(100);
    }

    if (mfrc522.PICC_ReadCardSerial()) {
      String uid = "";
      for (byte i = 0; i < mfrc522.uid.size; i++) {
        if(mfrc522.uid.uidByte[i] < 0x10) {
          uid += "0";
        }
        uid += String(mfrc522.uid.uidByte[i], HEX);
      }
      uid.toUpperCase();

      if (findCardIndex(uid) == -1) {
        cardUIDs[totalCards] = uid;
        cardBalances[totalCards] = initialBalance;
        totalCards++;
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Card Registered");
        lcd.setCursor(0,1);
        lcd.print("UID: " + uid);
        delay(2000);
      } else {
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Card Already");
        lcd.setCursor(0,1);
        lcd.print("Registered");
        delay(2000);
      }

      // Halt PICC
      mfrc522.PICC_HaltA();
    }
  } else {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Storage Full");
    lcd.setCursor(0,1);
    lcd.print("Cannot Register");
    delay(2000);
  }
}

// Function to view the balance of a card
void executeViewBalance() {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Scan Card to");
  lcd.setCursor(0,1);
  lcd.print("View Balance");

  // Wait for card
  while (!mfrc522.PICC_IsNewCardPresent()) {
    delay(100);
  }

  if (mfrc522.PICC_ReadCardSerial()) {
    String uid = "";
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      if(mfrc522.uid.uidByte[i] < 0x10) {
        uid += "0";
      }
      uid += String(mfrc522.uid.uidByte[i], HEX);
    }
    uid.toUpperCase();

    int cardIndex = findCardIndex(uid);
    if (cardIndex != -1) {
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Balance:");
      lcd.setCursor(0,1);
      lcd.print("₹" + String(cardBalances[cardIndex], 2));
      delay(2000);
    } else {
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Card Not Found");
      lcd.setCursor(0,1);
      lcd.print("Register First");
      delay(2000);
    }

    // Halt PICC
    mfrc522.PICC_HaltA();
  }
}

// Function to add balance to a card
void executeAddBalance() {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Scan Card to");
  lcd.setCursor(0,1);
  lcd.print("Add Balance");

  // Wait for card
  while (!mfrc522.PICC_IsNewCardPresent()) {
    delay(100);
  }

  if (mfrc522.PICC_ReadCardSerial()) {
    String uid = "";
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      if(mfrc522.uid.uidByte[i] < 0x10) {
        uid += "0";
      }
      uid += String(mfrc522.uid.uidByte[i], HEX);
    }
    uid.toUpperCase();

    int cardIndex = findCardIndex(uid);
    if (cardIndex != -1) {
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Enter Amount:");
      lcd.setCursor(0,1);
      lcd.print("Press # to Confirm");

      float amountToAdd = getAmountFromKeypad();

      cardBalances[cardIndex] += amountToAdd;

      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Added: ₹" + String(amountToAdd, 2));
      lcd.setCursor(0,1);
      lcd.print("New Bal: ₹" + String(cardBalances[cardIndex], 2));
      delay(2000);
    } else {
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Card Not Found");
      lcd.setCursor(0,1);
      lcd.print("Register First");
      delay(2000);
    }

    // Halt PICC
    mfrc522.PICC_HaltA();
  }
}

// Function to deduct balance from a card
void executeDeductBalance() {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Scan Card to");
  lcd.setCursor(0,1);
  lcd.print("Deduct Balance");

  // Wait for card
  while (!mfrc522.PICC_IsNewCardPresent()) {
    delay(100);
  }

  if (mfrc522.PICC_ReadCardSerial()) {
    String uid = "";
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      if(mfrc522.uid.uidByte[i] < 0x10) {
        uid += "0";
      }
      uid += String(mfrc522.uid.uidByte[i], HEX);
    }
    uid.toUpperCase();

    int cardIndex = findCardIndex(uid);
    if (cardIndex != -1) {
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Enter Amount:");
      lcd.setCursor(0,1);
      lcd.print("Press # to Confirm");

      float amountToDeduct = getAmountFromKeypad();

      if (cardBalances[cardIndex] >= amountToDeduct) {
        cardBalances[cardIndex] -= amountToDeduct;
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Deducted: ₹" + String(amountToDeduct, 2));
        lcd.setCursor(0,1);
        lcd.print("New Bal: ₹" + String(cardBalances[cardIndex], 2));
      } else {
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Insufficient");
        lcd.setCursor(0,1);
        lcd.print("Balance");
      }
      delay(2000);
    } else {
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Card Not Found");
      lcd.setCursor(0,1);
      lcd.print("Register First");
      delay(2000);
    }

    // Halt PICC
    mfrc522.PICC_HaltA();
  }
}

// =================== Input Handling ===================

// Helper function to get numerical input from the keypad
float getAmountFromKeypad() {
  String input = "";
  char key;
  while (true) {
    key = keypad.getKey();
    if (key) {
      Serial.print("Key Pressed: ");
      Serial.println(key);
      if (key == '#') {
        break; // End input on '#'
      } else if (key == '*') {
        // Handle backspace
        if (input.length() > 0) {
          input.remove(input.length() - 1);
          // Move cursor back, overwrite with space, move cursor back again
          int pos = input.length();
          lcd.setCursor(pos,1);
          lcd.print(" ");
          lcd.setCursor(pos,1);
        }
      } else if (isDigit(key) || key == '.') {
        input += key;
        lcd.setCursor(input.length()-1,1);
        lcd.print(key);
      }
    }
    delay(50); // Small delay to debounce keypad
  }

  float amount = input.toFloat();
  return amount;
}
