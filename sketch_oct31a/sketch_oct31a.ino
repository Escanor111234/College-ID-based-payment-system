#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN 10
#define RST_PIN 9
MFRC522 mfrc522(SS_PIN, RST_PIN);

String cardUIDs[10]; // Array to store card UIDs (up to 10 cards)
float cardBalances[10]; // Array to store card balances
int totalCards = 0;

void setup() {
  Serial.begin(9600);
  SPI.begin();
  mfrc522.PCD_Init();
  Serial.println("JAVA Food Court System");
  printUserMenu();
}

void loop() {
  int action = 0;
  while (action == 0) {
    action = getSerialInput();
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
  }
}

void printUserMenu() {
  Serial.println("User Menu:");
  Serial.println("1. Register New Card");
  Serial.println("2. View Balance");
  Serial.println("3. Add Balance");
  Serial.println("4. Deduct Balance");
  Serial.println("Enter your choice (1-4):");
}

int getSerialInput() {
  while (Serial.available()) {
    int choice = Serial.parseInt();
    if (choice > 0 && choice <= 4) {
      return choice;
    } else {
      Serial.println("Please enter 1, 2, 3, or 4.");
    }
  }
  return 0;
}

int findCardIndex(String uid) {
  for (int i = 0; i < totalCards; i++) {
    if (uid == cardUIDs[i]) {
      return i;
    }
  }
  return -1;
}

void executeRegisterNewCard() {
  if (totalCards < 10) { // Limit the number of cards to 10
    String newUID = "";
    float initialBalance = 0.0;

    Serial.println("Scan the new card to register...");
    while (!mfrc522.PICC_IsNewCardPresent()) {
      // Wait for a card to be present
    }

    if (mfrc522.PICC_ReadCardSerial()) {
      String uid = "";
      for (byte i = 0; i < mfrc522.uid.size; i++) {
        uid += String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
        uid += String(mfrc522.uid.uidByte[i], HEX);
      }
      uid.toUpperCase();

      if (findCardIndex(uid) == -1) {
        cardUIDs[totalCards] = uid;
        cardBalances[totalCards] = initialBalance;
        totalCards++;
        Serial.println("New card registered.");
      } else {
        Serial.println("Card already registered.");
      }
    }
  } else {
    Serial.println("Card storage is full. Cannot register a new card.");
  }
}


void executeViewBalance() {
  Serial.println("Scan your card to view balance...");
  while (!mfrc522.PICC_IsNewCardPresent()) {
    // Wait for a card to be present
  }

  if (mfrc522.PICC_ReadCardSerial()) {
    String uid = "";
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      uid += String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
      uid += String(mfrc522.uid.uidByte[i], HEX);
    }
    uid.toUpperCase();

    int cardIndex = findCardIndex(uid);
    if (cardIndex != -1) {
      Serial.print("Balance for card ");
      Serial.print(uid);
      Serial.print(": ₹");
      Serial.println(cardBalances[cardIndex], 2);
    } else {
      Serial.println("Card not found. Please register the card first.");
    }

    // Add a delay to display the balance for a few seconds
    delay(500);
  }
}


void executeAddBalance() {
  Serial.println("Scan your card to add balance...");
  while (!mfrc522.PICC_IsNewCardPresent()) {
    // Wait for a card to be present
  }

  if (mfrc522.PICC_ReadCardSerial()) {
    String uid = "";
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      uid += String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
      uid += String(mfrc522.uid.uidByte[i], HEX);
    }
    uid.toUpperCase();

    int cardIndex = findCardIndex(uid);
    if (cardIndex != -1) {
      Serial.print("Enter the amount to add: $");

      // Add a 5-second delay to allow for user input
      delay(5000);

      while (Serial.available() <= 0) {
        // Wait for user input
      }

      float amountToAdd = Serial.parseFloat();
      cardBalances[cardIndex] += amountToAdd;
      Serial.print("Balance added. New balance: $");
      Serial.println(cardBalances[cardIndex], 2);
    } else {
      Serial.println("Card not found. Please register the card first.");
    }
  }
}



void executeDeductBalance() {
  Serial.println("Scan your card to deduct balance...");
  while (!mfrc522.PICC_IsNewCardPresent()) {
    // Wait for a card to be present
  }

  if (mfrc522.PICC_ReadCardSerial()) {
    String uid = "";
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      uid += String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
      uid += String(mfrc522.uid.uidByte[i], HEX);
    }
    uid.toUpperCase();

    int cardIndex = findCardIndex(uid);
    if (cardIndex != -1) {
      Serial.print("Enter the amount to deduct: $");
      float amountToDeduct = 0.0;

      while (amountToDeduct <= 0.0) {
        if (Serial.available()) {
          amountToDeduct = Serial.parseFloat();
        }
      }

      if (cardBalances[cardIndex] >= amountToDeduct) {
        cardBalances[cardIndex] -= amountToDeduct;
        Serial.print("Balance deducted. New balance: $");
        Serial.println(cardBalances[cardIndex], 2);
      } else {
        Serial.println("Insufficient balance.");
      }
    } else {
      Serial.println("Card not found. Please register the card first.");
    }
  }
}