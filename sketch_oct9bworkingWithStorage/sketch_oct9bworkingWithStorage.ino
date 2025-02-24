#include <SPI.h>
#include <MFRC522.h>
#include <Keypad.h>             // Include the Keypad library
#include <Wire.h>               // Include Wire library for I2C communication
#include <LiquidCrystal_I2C.h>  // Include the I2C LCD library
#include <EEPROM.h>             // Include EEPROM library

// =================== RFID Configuration ===================
#define SS_PIN 10
#define RST_PIN 9
MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance

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

// =================== LCD Configuration (I2C) ===================
LiquidCrystal_I2C lcd(0x27, 16, 2); // Set the LCD I2C address (0x27) and size (16x2)
// Note: Some LCD modules might use 0x3F instead of 0x27. Use an I2C scanner to verify.

// =================== EEPROM Configuration ===================
const int EEPROM_START = 0;
const int UID_SIZE = 8;         // Fixed UID size (8 characters)
const int BALANCE_SIZE = 4;     // Size of float
const int CARD_SIZE = UID_SIZE + BALANCE_SIZE; // Total size per card
const int MAX_CARDS = 10;       // Maximum number of cards
const int EEPROM_MAX = EEPROM_START + (CARD_SIZE * MAX_CARDS);

// =================== Data Storage ===================
String cardUIDs[MAX_CARDS];      // Array to store card UIDs (up to 10 cards)
float cardBalances[MAX_CARDS];  // Array to store card balances
int totalCards = 0;

// =================== Function Prototypes ===================
void printUserMenu();
int getKeypadInput();
int findCardIndex(String uid);
void executeRegisterNewCard();
void executeViewBalance();
void executeAddBalance();
void executeDeductBalance();
float getAmountFromKeypad();
void saveCardToEEPROM(int index);
void loadCardsFromEEPROM();
void writeFloatToEEPROM(int address, float value);
float readFloatFromEEPROM(int address);
bool isHexDigit(char c); // Prototype for custom isHexDigit function

// =================== Custom isHexDigit Function ===================
bool isHexDigit(char c) {
  return (isDigit(c) || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'));
}

// =================== Setup Function ===================
void setup() {
  // Initialize Serial Communication
  Serial.begin(9600);
  
  // Initialize LCD over I2C
  lcd.init();            // Initialize the LCD
  lcd.backlight();       // Turn on the LCD backlight
  lcd.setCursor(0,0);
  lcd.print("JAVA Food Court");
  lcd.setCursor(0,1);
  lcd.print("System Init...");
  delay(2000); // Wait for 2 seconds
  
  // Initialize SPI and RFID
  SPI.begin();
  mfrc522.PCD_Init();
  
  // Load existing cards from EEPROM
  loadCardsFromEEPROM();
  
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
  if (totalCards < MAX_CARDS) { // Limit the number of cards
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

      // Validate UID characters
      bool validUID = true;
      for (int i = 0; i < uid.length(); i++) {
        if (!isHexDigit(uid[i])) {
          validUID = false;
          break;
        }
      }

      if (validUID) {
        if (findCardIndex(uid) == -1) {
          // Add UID to array
          cardUIDs[totalCards] = uid;
          // Initialize balance
          cardBalances[totalCards] = initialBalance;
          // Save to EEPROM
          saveCardToEEPROM(totalCards);
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
      } else {
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Invalid UID");
        lcd.setCursor(0,1);
        lcd.print("Register Failed");
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

    // Validate UID characters
    bool validUID = true;
    for (int i = 0; i < uid.length(); i++) {
      if (!isHexDigit(uid[i])) {
        validUID = false;
        break;
      }
    }

    if (validUID) {
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
    } else {
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Invalid UID");
      lcd.setCursor(0,1);
      lcd.print("Cannot View");
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

    // Validate UID characters
    bool validUID = true;
    for (int i = 0; i < uid.length(); i++) {
      if (!isHexDigit(uid[i])) {
        validUID = false;
        break;
      }
    }

    if (validUID) {
      int cardIndex = findCardIndex(uid);
      if (cardIndex != -1) {
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Enter Amount:");
        lcd.setCursor(0,1);
        lcd.print("Press # to Confirm");

        float amountToAdd = getAmountFromKeypad();

        if (amountToAdd > 0) {
          cardBalances[cardIndex] += amountToAdd;
          // Save updated balance to EEPROM
          saveCardToEEPROM(cardIndex);

          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("Added: ₹" + String(amountToAdd, 2));
          lcd.setCursor(0,1);
          lcd.print("New Bal: ₹" + String(cardBalances[cardIndex], 2));
          delay(2000);
        } else {
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("Invalid Amount");
          lcd.setCursor(0,1);
          lcd.print("Add Failed");
          delay(2000);
        }
      } else {
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Card Not Found");
        lcd.setCursor(0,1);
        lcd.print("Register First");
        delay(2000);
      }
    } else {
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Invalid UID");
      lcd.setCursor(0,1);
      lcd.print("Cannot Add");
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

    // Validate UID characters
    bool validUID = true;
    for (int i = 0; i < uid.length(); i++) {
      if (!isHexDigit(uid[i])) {
        validUID = false;
        break;
      }
    }

    if (validUID) {
      int cardIndex = findCardIndex(uid);
      if (cardIndex != -1) {
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Enter Amount:");
        lcd.setCursor(0,1);
        lcd.print("Press # to Confirm");

        float amountToDeduct = getAmountFromKeypad();

        if (amountToDeduct > 0) {
          if (cardBalances[cardIndex] >= amountToDeduct) {
            cardBalances[cardIndex] -= amountToDeduct;
            // Save updated balance to EEPROM
            saveCardToEEPROM(cardIndex);

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
          lcd.print("Invalid Amount");
          lcd.setCursor(0,1);
          lcd.print("Deduct Failed");
          delay(2000);
        }
      } else {
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Card Not Found");
        lcd.setCursor(0,1);
        lcd.print("Register First");
        delay(2000);
      }
    } else {
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Invalid UID");
      lcd.setCursor(0,1);
      lcd.print("Cannot Deduct");
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

// =================== EEPROM Handling ===================

// Function to write a float to EEPROM
void writeFloatToEEPROM(int address, float value) {
  byte* p = (byte*)(void*)&value;
  for (int i = 0; i < sizeof(float); i++) {
    EEPROM.write(address + i, *p++);
  }
}

// Function to read a float from EEPROM
float readFloatFromEEPROM(int address) {
  float value = 0.0;
  byte* p = (byte*)(void*)&value;
  for (int i = 0; i < sizeof(float); i++) {
    *p++ = EEPROM.read(address + i);
  }
  return value;
}

// Function to save a card's data to EEPROM
void saveCardToEEPROM(int index) {
  if (index >= 0 && index < MAX_CARDS) {
    int addr = EEPROM_START + (CARD_SIZE * index);
    
    // Save UID as fixed length
    String uid = cardUIDs[index];
    uid = uid.substring(0, UID_SIZE); // Ensure UID is not longer than UID_SIZE
    while (uid.length() < UID_SIZE) {
      uid += ' '; // Pad with spaces if shorter
    }
    for (int i = 0; i < UID_SIZE; i++) {
      EEPROM.write(addr + i, uid[i]);
    }
    
    // Save balance
    float balance = cardBalances[index];
    writeFloatToEEPROM(addr + UID_SIZE, balance);
  }
}

// Function to load all cards from EEPROM
void loadCardsFromEEPROM() {
  totalCards = 0;
  for (int i = 0; i < MAX_CARDS; i++) {
    int addr = EEPROM_START + (CARD_SIZE * i);
    
    // Read UID
    char uidChars[UID_SIZE + 1] = {0};
    for (int j = 0; j < UID_SIZE; j++) {
      uidChars[j] = EEPROM.read(addr + j);
    }
    String uid = String(uidChars);
    uid.trim(); // Remove any trailing spaces
    
    // Read balance
    float balance = readFloatFromEEPROM(addr + UID_SIZE);
    
    // Check if UID is valid (not empty and valid hex digits)
    if (uid.length() == UID_SIZE) {
      bool validUID = true;
      for (int k = 0; k < uid.length(); k++) {
        if (!isHexDigit(uid[k])) {
          validUID = false;
          break;
        }
      }
      if (validUID) {
        cardUIDs[totalCards] = uid;
        cardBalances[totalCards] = balance;
        totalCards++;
        if (totalCards >= MAX_CARDS) {
          break;
        }
      }
    }
  }
}
