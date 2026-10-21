#include <IRremote.hpp>
#include <LiquidCrystal.h>
#include <Servo.h>
#include <EEPROM.h>

////////TEMPORARY BUTTONS FOR THE JIMMY THE IR RECEVIER, RIP JIMMY
int command = -1;
const int IR_PIN = 12;
const int BUZZ = 2;

// ======================================================
// I/O SETUP
// ======================================================

// TARGET_PINS index = Mimmo target number
// 0 = SLIP, 1-6 = punches

const int TARGET_PINS[] = {
    13, // 0 = SLIP
    A0, // 1
    A1, // 2
    A2, // 3
    A3, // 4
    3,  // 5
    4   // 6
};

const int TARGET_COUNT = 7;

const int SPEED_POT = A4;
int lastPotSpeed = -1;
const int RAND_POT = A5;
int lastPotRand = -1;

//                RS, E, D4, D5, D6, D7
LiquidCrystal lcd(6, 7, 8, 9, 10, 11);

Servo mimmoServo;

const int SERVO_PIN = 5;
bool servoForward = true;

////// EDIT/CREATE COMBOS TO EEPROM VARS
byte comboEditBuffer[16] = {};
byte comboEditLength = 0;
bool comboEditing = false;

bool editingExistingCombo = false;
byte comboEditCursor = 0;

unsigned long lastCursorBlink = 0;
bool cursorVisible = true;

// ======================================================
// GAME VARIABLES
// ======================================================

// Human-readable speed:
// 0 = slowest
// 9 = fastest
int speed = 5;
int roundsInput = 0;
int rounds = 3;
int currentRound = 0;
int randomnessInput = 0;
int randomness = 0;
int i = 1;
bool southpaw = true;

// Actual timing derived from speed
int delayTime = 2000 - (speed * 200);

bool running = false;

int combo = 1;

int activeTarget = -1;

bool answeredCurrentHit = false;
int score = 0;
int totalScore = 0;

int badHits = 0;
int tooSlow = 0;

int successfulPunches = 0;

unsigned long totalResponseTime = 0;

// ======================================================
// HIT / COMBO STATE
// ======================================================

int hitPhase = 0;

// 0 = hit not started
// 1 = LED ON
// 2 = LED OFF / waiting for end of beat

unsigned long hitStartMillis = 0;

bool buzzerOn = false;
unsigned long buzzerStartMillis = 0;

int currentHit = 0;

/// STATE AND MENU
bool menuOpen = false;
bool speedMenuOpen = false;
bool RNDMenuOpen = false;
bool roundsMenuOpen = false;
bool comboMenuOpen = false;
bool selectComboOpen = false;
bool createComboOpen = false;
bool deleteComboOpen = false;
bool editComboOpen = false;
bool southpawMenuOpen = false;
bool southpawSelection = false;

bool debugScreenOpen = false;
unsigned long lastDebugSignal = 0;

bool speedShow = false;
unsigned long speedShowStart = 0;
const unsigned long SPEED_SHOW_DURATION = 3000;

bool randShow = false;
unsigned long randShowStart = 0;
const unsigned long RAND_SHOW_DURATION = 3000;

int menuItem = 0;
int subMenuItem = 0;
int comboSelectItem = 0;

// Combos

const int COMBO1[] = {1, 2, 3, 4, 5, 6};
const int COMBO1_LENGTH = 6;

const int COMBO2[] = {1, 4, 3, 2, 5, 6, 0};
const int COMBO2_LENGTH = 7;

const int COMBO3[] = {1, 1, 2, 0};
const int COMBO3_LENGTH = 4;

////////COMBO DB
struct ComboData
{
    bool used;
    String name;
    int length;
};

ComboData comboSlots[10] = {
    {true, "COMBO 1", COMBO1_LENGTH},
    {true, "COMBO 2", COMBO2_LENGTH},
    {true, "COMBO 3", COMBO3_LENGTH},
    {true, "COMBO 4", 8},
    {false, "COMBO 5", 0},
    {false, "COMBO 6", 0},
    {false, "COMBO 7", 0},
    {false, "COMBO 8", 0},
    {false, "COMBO 9", 0},
    {false, "COMBO 10", 0}};

// ======================================================
// CUSTOM COMBO STORAGE - RAM
// ======================================================

const int CUSTOM_COMBO_COUNT = 7;
const int MAX_COMBO_LENGTH = 16;

byte customCombos[CUSTOM_COMBO_COUNT][MAX_COMBO_LENGTH] = {};

// ======================================================
// EEPROM MEMORY MAP
// ======================================================

const int EEPROM_MAGIC_ADDR = 0;
const int EEPROM_VERSION_ADDR = 1;

const byte EEPROM_MAGIC = 0x4D;
const byte EEPROM_VERSION = 1;

const int EEPROM_COMBOS_START = 2;
const int EEPROM_COMBO_SIZE = 17;

// Calculate the starting address of a custom combo
int comboEEPROMAddress(int customIndex)
{
    return EEPROM_COMBOS_START + customIndex * EEPROM_COMBO_SIZE;
}

// ======================================================
// SAVE CUSTOM COMBO TO EEPROM
// ======================================================

void saveCustomCombo(int customIndex)
{

    // Safety: only custom combos 0-6
    if (customIndex < 0 || customIndex >= CUSTOM_COMBO_COUNT)
    {
        return;
    }

    // Translate custom index into comboSlots index
    int slotIndex = customIndex + 3;

    // Find this combo's EEPROM starting address
    int address = comboEEPROMAddress(customIndex);

    // Get its actual length (0 means EMPTY)
    byte length = comboSlots[slotIndex].used
                      ? comboSlots[slotIndex].length
                      : 0;

    // Safety: never exceed 16 actions
    if (length > MAX_COMBO_LENGTH)
    {
        return;
    }

    // Write the sequence
    for (int i = 0; i < MAX_COMBO_LENGTH; i++)
    {
        EEPROM.update(address + 1 + i, customCombos[customIndex][i]);
    }

    // Write the length LAST
    EEPROM.update(address, length);

    Serial.print(F("EEPROM SAVED COMBO "));
    Serial.println(slotIndex + 1);
}

// ======================================================
// LOAD CUSTOM COMBO FROM EEPROM
// ======================================================

void loadCustomCombo(int customIndex)
{

    if (customIndex < 0 || customIndex >= CUSTOM_COMBO_COUNT)
    {
        return;
    }

    int slotIndex = customIndex + 3;
    int address = comboEEPROMAddress(customIndex);

    byte length = EEPROM.read(address);

    // Reject invalid lengths
    if (length > MAX_COMBO_LENGTH)
    {
        length = 0;
    }

    comboSlots[slotIndex].length = length;
    comboSlots[slotIndex].used = (length > 0);

    for (int i = 0; i < MAX_COMBO_LENGTH; i++)
    {

        byte action = EEPROM.read(address + 1 + i);

        if (i < length && action > 6)
        {
            length = 0;
        }

        customCombos[customIndex][i] = action;
    }

    comboSlots[slotIndex].length = length;
    comboSlots[slotIndex].used = (length > 0);

    Serial.print(F("EEPROM LOADED COMBO "));
    Serial.print(slotIndex + 1);
    Serial.print(F(" LENGTH: "));
    Serial.println(length);
}

void loadAllCustomCombos()
{

    if (EEPROM.read(EEPROM_MAGIC_ADDR) != EEPROM_MAGIC ||
        EEPROM.read(EEPROM_VERSION_ADDR) != EEPROM_VERSION)
    {

        Serial.println(F("NO VALID EEPROM DATA"));
        return;
    }

    for (int i = 0; i < CUSTOM_COMBO_COUNT; i++)
    {
        loadCustomCombo(i);
    }
}

// ======================================================
// IDENTIFY MIIIMMO EEPROM DATA
// ======================================================

void saveEEPROMHeader()
{
    EEPROM.update(EEPROM_MAGIC_ADDR, EEPROM_MAGIC);
    EEPROM.update(EEPROM_VERSION_ADDR, EEPROM_VERSION);
}

////////
// RANDOMISER VARS
////////

const int MAX_SEQUENCE = 20;

int currentSequence[MAX_SEQUENCE];
int currentSequenceLength = 0;

void deleteCustomCombo(int slotIndex)
{

    // Factory combos are protected
    if (slotIndex < 3 || slotIndex >= 10)
    {
        return;
    }

    // Nothing to delete
    if (!comboSlots[slotIndex].used)
    {
        return;
    }

    int customIndex = slotIndex - 3;

    // Clear RAM metadata
    comboSlots[slotIndex].used = false;
    comboSlots[slotIndex].length = 0;

    // Clear the stored sequence in RAM
    for (int i = 0; i < MAX_COMBO_LENGTH; i++)
    {
        customCombos[customIndex][i] = 0;
    }

    // Make deletion permanent
    saveCustomCombo(customIndex);

    // Always return to factory Combo 1
    combo = 1;
    currentHit = 0;
    currentSequenceLength = 0;
    // resetCurrentHit();

    Serial.print(F("PERMANENTLY DELETED COMBO "));
    Serial.println(slotIndex + 1);
}

// ======================================================
// LED CONTROL
// ======================================================

void LED1_ON()
{
    digitalWrite(TARGET_PINS[1], HIGH);
}

void LED1_OFF()
{
    digitalWrite(TARGET_PINS[1], LOW);
}

void LED2_ON()
{
    digitalWrite(TARGET_PINS[2], HIGH);
}

void LED2_OFF()
{
    digitalWrite(TARGET_PINS[2], LOW);
}

void LED3_ON()
{
    digitalWrite(TARGET_PINS[3], HIGH);
}

void LED3_OFF()
{
    digitalWrite(TARGET_PINS[3], LOW);
}

void LED4_ON()
{
    digitalWrite(TARGET_PINS[4], HIGH);
}

void LED4_OFF()
{
    digitalWrite(TARGET_PINS[4], LOW);
}

void LED5_ON()
{
    digitalWrite(TARGET_PINS[5], HIGH);
}

void LED5_OFF()
{
    digitalWrite(TARGET_PINS[5], LOW);
}

void LED6_ON()
{
    digitalWrite(TARGET_PINS[6], HIGH);
}

void LED6_OFF()
{
    digitalWrite(TARGET_PINS[6], LOW);
}

void SLIP_LED_ON()
{
    digitalWrite(TARGET_PINS[0], HIGH);
}

void SLIP_LED_OFF()
{
    digitalWrite(TARGET_PINS[0], LOW);
}

// Emergency / startup cleanup

void ALL_LEDS_OFF()
{

    for (int i = 0; i < TARGET_COUNT; i++)
    {
        digitalWrite(TARGET_PINS[i], LOW);
    }
}

void resetCurrentHit()
{

    ALL_LEDS_OFF();

    hitPhase = 0;
}

void shutTheSpeedScreen()
{

    if (speedShow &&
        millis() - speedShowStart >= SPEED_SHOW_DURATION)
    {

        speedShow = false;
        refreshLCD();
    }
}

void shutTheRandScreen()
{

    if (randShow &&
        millis() - randShowStart >= RAND_SHOW_DURATION)
    {

        randShow = false;
        refreshLCD();
    }
}

void refreshLCD()
{

    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print(F("COMBO: "));
    lcd.print(combo);

    lcd.setCursor(0, 1);
    lcd.print(F("SPEED: "));
    lcd.print(speed);
}

void resetGame()
{

    running = false;

    score = 0;
    badHits = 0;
    tooSlow = 0;
    totalResponseTime = 0;
    totalScore = 0;
    successfulPunches = 0;

    currentSequenceLength = 0;

    ALL_LEDS_OFF();

    currentHit = 0;
    currentRound = 0;
    hitPhase = 0;
}

bool hit(int target)
{

    unsigned long currentMillis = millis();

    // START HIT
    if (hitPhase == 0)
    {

        answeredCurrentHit = false;
        Serial.print(F("HIT THIS: "));
        Serial.println(activeTarget);

        digitalWrite(TARGET_PINS[target], HIGH);
        if (target == 0)
        {
            servoSlip();
        }

        hitStartMillis = currentMillis;
        hitPhase = 1;

        return false;
    }

    // 75% OF BEAT -> LIGHT OFF
    if (hitPhase == 1 &&
        currentMillis - hitStartMillis >= delayTime * 3 / 4)
    {

        digitalWrite(TARGET_PINS[target], LOW);

        hitPhase = 2;

        return false;
    }

    // 100% OF BEAT -> HIT COMPLETE
    if (hitPhase == 2 &&
        currentMillis - hitStartMillis >= delayTime)
    {

        if (!answeredCurrentHit)
        {

            if (activeTarget == 0)
            {

                score++;

                Serial.println(F("GOOD SLIP"));

                digitalWrite(BUZZ, HIGH);
                buzzerOn = true;
                buzzerStartMillis = millis();
            }

            else
            {

                tooSlow++;
                Serial.println(F("TOO SLOW MAN"));
            }
        }

        hitPhase = 0;
        return true;
    }

    return false;
}

int southpawTarget(int activeTarget)
{
    if (activeTarget == 0)
    {
        return 0;
    }
    if (activeTarget % 2 == 0)
    {
        activeTarget = activeTarget - 1;
    }
    else
    {
        activeTarget = activeTarget + 1;
    }
    return activeTarget;
}

void servoSlip()
{

    if (servoForward)
    {
        mimmoServo.write(165);
    }
    else
    {
        mimmoServo.write(15);
    }

    servoForward = !servoForward;
}

// ======================================================
// COMBOS
// ======================================================

// For now, Combo 1 is your complete LED test sequence.

template <typename T>
void playCombo(const T sequence[], int length)
{
    if (hitPhase == 0)
    {

        int target = sequence[currentHit];

        int previousOriginalTarget = -1;

        if (currentHit > 0)
        {
            previousOriginalTarget = sequence[currentHit - 1];
        }

        activeTarget = randomiseHit(target, previousOriginalTarget);

        if (southpaw == true)
        {
            activeTarget = southpawTarget(activeTarget);
        }

        if (currentSequenceLength < MAX_SEQUENCE)
        {
            currentSequence[currentSequenceLength] = activeTarget;
            currentSequenceLength++;
        }
    }

    bool hitFinished = hit(activeTarget);

    if (hitFinished)
    {

        currentHit++;

        // Reached end of combo?
        if (currentHit >= length)
        {

            currentHit = 0;

            currentRound++;

            currentSequenceLength = 0;

            if (currentRound >= rounds)
            {
                totalScore = length * rounds;
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print(F("GAME FINISHED"));
                lcd.setCursor(0, 1);
                lcd.print(F("SCORE:"));
                lcd.print(score);
                lcd.print(F("/"));
                lcd.print(totalScore);
                Serial.println(F("----- FINAL RESULTS -----"));
                Serial.print(F("FINAL SCORE: "));
                Serial.print(score);
                Serial.print(F("/"));
                Serial.println(totalScore);

                Serial.print(F("BAD HITS: "));
                Serial.println(badHits);

                Serial.print(F("TOO SLOW: "));
                Serial.println(tooSlow);

                if (successfulPunches > 0)
                {

                    unsigned long averageResponseTime = totalResponseTime / successfulPunches;

                    Serial.print(F("AVG RESPONSE: "));
                    Serial.print(averageResponseTime);
                    Serial.println(F(" ms"));
                }

                Serial.println(F("-------------------------"));

                resetGame();
            }
        }
    }
}

void registerHit(int input)
{

    if (!running)
    {
        return;
    }

    if (!answeredCurrentHit)
    {

        answeredCurrentHit = true;

        // SLIP: pressing ANYTHING is wrong
        if (activeTarget == 0)
        {

            badHits++;

            Serial.println(F("BAD - YOU PUNCHED DURING SLIP"));

            return;
        }

        // NORMAL PUNCH
        if (input == activeTarget)
        {

            unsigned long responseTime = millis() - hitStartMillis;

            score++;
            successfulPunches++;
            totalResponseTime += responseTime;

            digitalWrite(BUZZ, HIGH);
            buzzerOn = true;
            buzzerStartMillis = millis();

            Serial.print(F("GOOD HIT - "));
            Serial.print(responseTime);
            Serial.println(F(" ms"));
        }
        else
        {

            badHits++;
            Serial.println(F("BAD HIT"));
        }
    }
}

/////////////////////////
// MENU LOGIC
///////////////////////
void showDebugScreen()
{

    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print(F("SPE:"));
    lcd.print(speed);

    lcd.setCursor(7, 0);
    lcd.print(F("ROU:"));
    lcd.print(rounds);

    lcd.setCursor(0, 1);
    lcd.print(F("COM:"));
    lcd.print(combo);

    lcd.setCursor(7, 1);
    lcd.print(F("RAND:"));
    lcd.print(randomness);
}

void refreshMenu()
{

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("MENU:"));

    lcd.setCursor(0, 1);

    if (menuItem == 0)
    {
        lcd.print(F(">SPEED"));
    }

    if (menuItem == 1)
    {
        lcd.print(F(">COMBOS"));
    }

    if (menuItem == 2)
    {
        lcd.print(F(">ROUNDS"));
    }
    if (menuItem == 3)
    {
        lcd.print(F(">RANDOMNESS"));
    }
    if (menuItem == 4)
    {
        lcd.print(F(">SOUTHPAW"));
    }
    if (menuItem == 5)
    {
        lcd.print(F(">EXIT"));
    }
}

////////////////
// REMOTE TRANSLATOR
/////////////////

int remoteNumber(int command)
{

    if (command == 0x0C)
        return 1;
    if (command == 0x18)
        return 2;
    if (command == 0x5E)
        return 3;
    if (command == 0x08)
        return 4;
    if (command == 0x1C)
        return 5;
    if (command == 0x5A)
        return 6;
    if (command == 0x42)
        return 7;
    if (command == 0x52)
        return 8;
    if (command == 0x4A)
        return 9;
    if (command == 0x16)
        return 0;

    return -1;
}

void refreshSpeedSub()
{
    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print(F("SPEED"));

    lcd.setCursor(0, 1);
    lcd.print(speed);
}

void speedSubMenu(int command)
{
    if (speedMenuOpen)
    {

        int number = remoteNumber(command);

        if (number != -1)
        {

            speed = number;

            refreshSpeedSub();

            Serial.print(F("New speed: "));
            Serial.println(speed);

            delay(300);
        }
    }

    if (speedMenuOpen && command == 0x0D)
    {

        delayTime = 2000 - (speed * 200);

        speedMenuOpen = false;

        refreshMenu();

        Serial.print(F("Speed confirmed: "));
        Serial.println(speed);

        delay(300);
    }
}

void refreshSouthpawMenu()
{

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("SOUTHPAW"));

    lcd.setCursor(0, 1);

    if (southpawSelection)
    {
        lcd.print(F(">ON"));
    }
    else
    {
        lcd.print(F(">OFF"));
    }
}

void refreshRNDSub()
{
    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print(F("RANDOMNESS"));

    lcd.setCursor(0, 1);
    lcd.print(randomnessInput);
    lcd.setCursor(2, 1);
    lcd.print(F("%"));
}

void refreshComboSub()
{

    lcd.noBlink();
    lcd.noCursor();

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("COMBO:"));

    lcd.setCursor(0, 1);

    if (subMenuItem == 0)
    {
        lcd.print(F(">SELECT COMBO"));
    }

    if (subMenuItem == 1)
    {
        lcd.print(F(">CREATE COMBO"));
    }

    if (subMenuItem == 2)
    {
        lcd.print(F(">EDIT COMBO"));
    }
    if (subMenuItem == 3)
    {
        lcd.print(F(">DELETE COMBO"));
    }
}

void refreshSelectComboSub()
{

    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print(F("SELECT COMBO"));

    lcd.setCursor(0, 1);

    lcd.print(comboSelectItem + 1);
    lcd.print(F(" > "));

    if (comboSlots[comboSelectItem].used)
    {
        lcd.print(comboSlots[comboSelectItem].name);
    }
    else
    {
        lcd.print(F("EMPTY"));
    }
}

void refreshCreateComboSub()
{

    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print(F("CREATE COMBO"));

    lcd.setCursor(0, 1);

    lcd.print(comboSelectItem + 1);
    lcd.print(F(" > "));

    if (comboSlots[comboSelectItem].used)
    {
        lcd.print(F("USED"));
    }
    else
    {
        lcd.print(comboSlots[comboSelectItem].name);
    }
}

void refreshEditComboSub()
{

    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print(F("EDIT COMBO "));
    lcd.print(comboSelectItem + 1);

    lcd.setCursor(0, 1);

    if (!comboSlots[comboSelectItem].used)
    {
        lcd.print(F("EMPTY"));
        return;
    }

    // Factory combos are protected
    if (comboSelectItem < 3)
    {
        lcd.print(F("FACTORY COMBO"));
        return;
    }

    int customIndex = comboSelectItem - 3;
    int length = comboSlots[comboSelectItem].length;

    for (int i = 0; i < length; i++)
    {
        lcd.print(customCombos[customIndex][i]);
    }
}

void refreshComboEditor()
{

    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print(F("CREATE COMBO "));
    lcd.print(comboSelectItem + 1);

    lcd.setCursor(0, 1);

    // Display every action already entered
    for (int i = 0; i < comboEditLength; i++)
    {

        lcd.print(comboEditBuffer[i]);
    }

    // Show where the next action will go
    if (comboEditLength < MAX_COMBO_LENGTH)
    {
        lcd.print(F("_"));
    }
}

void refreshExistingComboEditor()
{

    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print(F("EDIT COMBO "));
    lcd.print(comboSelectItem + 1);

    lcd.setCursor(0, 1);

    for (int i = 0; i < comboEditLength; i++)
    {
        lcd.print(comboEditBuffer[i]);
    }

    if (comboEditLength < MAX_COMBO_LENGTH)
    {
        lcd.print(F("_"));
    }

    // Highlight the current cursor position
    lcd.setCursor(comboEditCursor, 1);
    lcd.cursor();
    lcd.blink();
}

void startEditingExistingCombo()
{

    Serial.println(F("ENTERED startEditingExistingCombo"));

    // Only existing custom combos 4-10
    if (comboSelectItem < 3 ||
        comboSelectItem >= 10 ||
        !comboSlots[comboSelectItem].used)
    {
        return;
    }

    int customIndex = comboSelectItem - 3;

    comboEditLength = comboSlots[comboSelectItem].length;

    for (int i = 0; i < MAX_COMBO_LENGTH; i++)
    {
        comboEditBuffer[i] = customCombos[customIndex][i];
    }

    comboEditCursor = 0;
    editingExistingCombo = true;
    Serial.println(F("EDIT MODE ACTIVE"));

    refreshExistingComboEditor();

    Serial.print(F("EDITING COMBO "));
    Serial.println(comboSelectItem + 1);
}

void moveComboEditCursor(int direction)
{

    if (!editingExistingCombo)
        return;

    int maxPosition = comboEditLength;

    if (maxPosition >= MAX_COMBO_LENGTH)
    {
        maxPosition = MAX_COMBO_LENGTH - 1;
    }

    int newPosition = comboEditCursor + direction;

    if (newPosition < 0)
        newPosition = 0;
    if (newPosition > maxPosition)
        newPosition = maxPosition;

    comboEditCursor = newPosition;

    lcd.setCursor(comboEditCursor, 1);
}

void replaceComboEditAction(int action)
{

    if (!editingExistingCombo)
        return;
    if (action < 0 || action > 6)
        return;

    // Replace an existing action
    if (comboEditCursor < comboEditLength)
    {
        comboEditBuffer[comboEditCursor] = action;
    }

    // Append at the underscore
    else if (comboEditLength < MAX_COMBO_LENGTH)
    {
        comboEditBuffer[comboEditLength] = action;
        comboEditLength++;
    }

    // Automatically advance
    if (comboEditCursor < comboEditLength)
    {
        comboEditCursor++;
    }

    if (comboEditCursor >= MAX_COMBO_LENGTH)
    {
        comboEditCursor = MAX_COMBO_LENGTH - 1;
    }

    refreshExistingComboEditor();
}

void backComboAction()
{

    // Already at the beginning? Nothing to undo.
    if (comboEditLength == 0)
    {
        return;
    }

    // Move back one position
    comboEditLength--;

    // Clear the action we just removed
    comboEditBuffer[comboEditLength] = 0;

    // Redraw the sequence
    refreshComboEditor();

    Serial.println(F("COMBO EDITOR: BACK"));
}
void addComboAction(int action)
{

    // Only accept punches 0-6
    if (action < 0 || action > 6)
    {
        return;
    }

    // Maximum 16 actions
    if (comboEditLength >= MAX_COMBO_LENGTH)
    {
        return;
    }

    // Store the action
    comboEditBuffer[comboEditLength] = action;

    // Move to the next position
    comboEditLength++;

    // Update LCD
    refreshComboEditor();

    Serial.print(F("ADDED ACTION: "));
    Serial.println(action);
}

void validateComboEditor()
{

    // Cannot save an empty combo
    if (comboEditLength == 0)
    {
        return;
    }

    // Safety: custom combos only (4-10)
    if (comboSelectItem < 3 || comboSelectItem >= 10)
    {
        return;
    }

    int customIndex = comboSelectItem - 3;

    // Copy our editor buffer into the actual combo
    for (int i = 0; i < MAX_COMBO_LENGTH; i++)
    {
        customCombos[customIndex][i] = comboEditBuffer[i];
    }

    // Activate the combo
    comboSlots[comboSelectItem].length = comboEditLength;
    comboSlots[comboSelectItem].used = true;

    // Save permanently
    saveCustomCombo(customIndex);
    saveEEPROMHeader();

    // Select the newly created combo
    combo = comboSelectItem + 1;

    // Reset game position
    currentHit = 0;
    currentSequenceLength = 0;

    // Exit the editor and menus
    comboEditing = false;
    createComboOpen = false;
    comboMenuOpen = false;
    menuOpen = false;

    // Return to the normal LCD
    refreshLCD();

    Serial.print(F("CREATED COMBO "));
    Serial.print(combo);
    Serial.print(F(" LENGTH: "));
    Serial.println(comboEditLength);
}

void saveEditedCombo()
{

    if (!editingExistingCombo)
        return;

    // Protect factory combos and reject empty sequences
    if (comboSelectItem < 3 ||
        comboSelectItem >= 10 ||
        comboEditLength == 0)
    {
        return;
    }

    int customIndex = comboSelectItem - 3;

    // Copy edited sequence into the real RAM storage
    for (int i = 0; i < MAX_COMBO_LENGTH; i++)
    {
        customCombos[customIndex][i] = comboEditBuffer[i];
    }

    // Update combo metadata
    comboSlots[comboSelectItem].length = comboEditLength;
    comboSlots[comboSelectItem].used = true;

    // Write permanently to EEPROM
    saveCustomCombo(customIndex);
    saveEEPROMHeader();

    Serial.print(F("EDITED COMBO SAVED: "));
    Serial.println(comboSelectItem + 1);

    // Leave editing mode
    editingExistingCombo = false;
    editComboOpen = false;

    lcd.noBlink();
    lcd.noCursor();

    // Return to COMBOS submenu
    subMenuItem = 2;
    refreshComboSub();
}

void refreshDeleteComboSub()
{

    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print(F("DELETE COMBO"));

    lcd.setCursor(0, 1);

    lcd.print(comboSelectItem + 1);
    lcd.print(F(" > "));

    if (comboSlots[comboSelectItem].used)
    {
        lcd.print(comboSlots[comboSelectItem].name);
    }
    else
    {
        lcd.print(F("EMPTY"));
    }
}

void RNDSubMenu(int command)
{
    if (RNDMenuOpen)
    {

        int number = remoteNumber(command);

        if (number != -1)
        {

            if (randomnessInput < 10)
            {
                randomnessInput = randomnessInput * 10 + number;
            }
            else
            {
                randomnessInput = (randomnessInput % 10) * 10 + number;
            }

            refreshRNDSub();

            Serial.print(F("New rand: "));
            Serial.println(randomnessInput);

            delay(300);
        }
    }

    if (RNDMenuOpen && command == 0x0D)
    {

        randomness = randomnessInput;
        randomnessInput = 0;

        RNDMenuOpen = false;

        refreshMenu();

        Serial.print(F("Rand confirmed: "));
        Serial.println(randomness);

        delay(300);
    }
}

void refreshRoundsSub()
{
    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print(F("ROUNDS"));

    lcd.setCursor(0, 1);
    lcd.print(roundsInput);
}

void roundsSubMenu(int command)
{
    if (roundsMenuOpen)
    {

        int number = remoteNumber(command);

        if (number != -1)
        {

            if (roundsInput < 10)
            {
                roundsInput = roundsInput * 10 + number;
            }
            else
            {
                roundsInput = (roundsInput % 10) * 10 + number;
            }
            refreshRoundsSub();

            Serial.print(F("New rounds: "));
            Serial.println(roundsInput);

            delay(300);
        }
    }

    if (roundsMenuOpen && command == 0x0D)
    {

        roundsMenuOpen = false;

        refreshMenu();
        rounds = roundsInput;

        Serial.print(F("Rounds confirmed: "));
        Serial.println(rounds);

        delay(300);
    }
}

void countdown()
{
    int l = 0;
    lcd.clear();

    for (i = 3; i > 0; i--)
    {

        lcd.setCursor(l, 0);
        lcd.print(i);
        l++;
        SLIP_LED_ON();
        delay(750);
        SLIP_LED_OFF();
        delay(250);
    }
    digitalWrite(BUZZ, HIGH);
    buzzerOn = true;
    buzzerStartMillis = millis();
}
// ======================================================
// SETUP
// ======================================================
void setup()
{

    Serial.begin(115200);
    // PINS FOR BUTTONS
    // pinMode(2, INPUT_PULLUP);  // MENU
    // pinMode(3, INPUT_PULLUP);  // VALIDATE
    // pinMode(4, INPUT_PULLUP);  // DOWN
    // pinMode(5, INPUT_PULLUP);  // UP

    // pinMode(MENU_BUTTON, INPUT_PULLUP);
    pinMode(BUZZ, OUTPUT);

    for (int i = 0; i < TARGET_COUNT; i++)
    {
        pinMode(TARGET_PINS[i], OUTPUT);
        digitalWrite(TARGET_PINS[i], LOW);
    }

    mimmoServo.attach(SERVO_PIN);
    mimmoServo.write(15);
    delay(500);

    //// SET EEPROM
    // EEPROM.update(0, 5);       // Store value 5 at address 0
    // byte value = EEPROM.read(0);  // Read it back

    /////INITIALIZE SPEED POT
    int rawValue = analogRead(SPEED_POT);
    speed = map(rawValue, 0, 1023, 1, 9);
    delayTime = 2000 - (speed * 200);
    lastPotSpeed = speed;

    /////INITIALIZE RAND POT
    rawValue = analogRead(RAND_POT);
    randomness = map(rawValue, 0, 1023, 0, 99);
    lastPotRand = randomness;

    IrReceiver.begin(IR_PIN, DISABLE_LED_FEEDBACK);
    delay(1000);

    lcd.begin(16, 2);
    lcd.setCursor(0, 0);
    lcd.print(F("I AM mIIIMMO"));

    lcd.setCursor(0, 1);
    lcd.print(F("PUNCH ME"));

    Serial.println(F("MIMMO III READY, NOW WITH GRUDGE"));

    // Restore saved custom combos from EEPROM
    loadAllCustomCombos();

    // Check that Combo 4 was restored into RAM
    Serial.print(F("COMBO 4 IN RAM - LENGTH: "));
    Serial.println(comboSlots[3].length);

    Serial.print(F("COMBO 4 IN RAM - ACTIONS: "));

    for (int i = 0; i < comboSlots[3].length; i++)
    {
        Serial.print(customCombos[0][i]);
        Serial.print(F(" "));
    }

    Serial.println();

    // ======================================================
    // TEMPORARY EEPROM TEST - COMBO 4
    // ======================================================

    /*
        // Define five actions in RAM
        customCombos[0][0] = 1;
        customCombos[0][1] = 2;
        customCombos[0][2] = 0;  // SLIP
        customCombos[0][3] = 3;
        customCombos[0][4] = 4;

        // Tell MIIIMMO this combo contains five actions
        comboSlots[3].used = true;
        comboSlots[3].length = 5;

        // Save our test combo and EEPROM header
        saveCustomCombo(0);
        saveEEPROMHeader();
    */

    // Read EEPROM directly to verify the result
    Serial.println(F("--- EEPROM READBACK ---"));

    Serial.print(F("MAGIC: "));
    Serial.println(EEPROM.read(0), HEX);

    Serial.print(F("VERSION: "));
    Serial.println(EEPROM.read(1));

    Serial.print(F("COMBO 4 LENGTH: "));
    Serial.println(EEPROM.read(2));

    for (int i = 0; i < 5; i++)
    {
        Serial.print(F("ACTION "));
        Serial.print(i + 1);
        Serial.print(F(": "));
        Serial.println(EEPROM.read(3 + i));
    }

    Serial.println(F("-----------------------"));
}

// ======================================================
// MAIN LOOP
// ======================================================

void loop()
{

    command = IrReceiver.decodedIRData.command;

    /*
       if (digitalRead(2) == LOW) {
            command = 0x19;  // MENU
            delay(300);
        }

        if (digitalRead(3) == LOW) {
            command = 0x0D;  // VALIDATE
            delay(300);
        }

        if (digitalRead(4) == LOW) {
            command = 0x09;  // DOWN
            delay(300);
        }

        if (digitalRead(5) == LOW) {
            command = 0x07;  // UP
            delay(300);
        }



        /// END TEMP

    */

    // --------------------------------------------------
    // POTENTIOMETER FOR RAND
    // --------------------------------------------------
    int rawValue = analogRead(SPEED_POT);
    int potSpeed = map(rawValue, 0, 1023, 1, 9);

    if (potSpeed != lastPotSpeed)
    {
        speed = potSpeed;
        delayTime = 2000 - (speed * 200);
        lastPotSpeed = potSpeed;
        Serial.print(F("POT SPEED: "));
        Serial.println(speed);
        lcd.clear();
        lcd.print(F("SPEED:"));
        lcd.print(speed);

        speedShow = true;
        speedShowStart = millis();
    }

    // --------------------------------------------------
    // POTENTIOMEYER FOR RAND
    // --------------------------------------------------
    rawValue = analogRead(RAND_POT);
    int potRand = map(rawValue, 0, 1023, 0, 99);

    if (potRand < 5)
    {
        potRand = 0;
    }

    if (abs(potRand - lastPotRand) >= 5)
    {
        randomness = potRand;
        lastPotRand = potRand;
        Serial.print(F("POT RAND: "));
        Serial.println(randomness);
        lcd.clear();
        lcd.print(F("RAND:"));
        lcd.print(randomness);

        randShow = true;
        randShowStart = millis();
    }

    // --------------------------------------------------
    // REMOTE CONTROL
    // --------------------------------------------------

    if (IrReceiver.decode())
    {

        int command = IrReceiver.decodedIRData.command;

        if (command == 0x47)
        {

            lastDebugSignal = millis();

            if (!debugScreenOpen)
            {
                debugScreenOpen = true;
                showDebugScreen();
            }
        }
        ///////CHECK IF 0X47 IS STILL ON

        // ----------------------------------------------
        // PLAY / PAUSE
        // 0x40
        // ----------------------------------------------

        if (command == 0x40)
        {

            running = !running;
            if (running)
            {
                countdown();
            }

            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print(F("PLAYING"));

            Serial.println(F("PLAYING"));
            Serial.print(F("speed: "));
            Serial.println(speed);

            delay(300);
        }

        /////EDIT COMBO ARROWS
        if (editingExistingCombo && command == 0x44)
        {
            moveComboEditCursor(-1);
            delay(150);
        }

        if (editingExistingCombo && command == 0x43)
        {
            moveComboEditCursor(1);
            delay(150);
        }

        // ----------------------------------------------
        // PREVIOUS COMBO
        // 0x44
        // ----------------------------------------------

        if (command == 0x44 && !editingExistingCombo)
        {

            resetCurrentHit();

            combo--;

            if (combo < 1)
            {
                combo = 9;
            }

            Serial.print(F("Combo: "));
            Serial.println(combo);

            refreshLCD();

            delay(300);
        }

        // ----------------------------------------------
        // NEXT COMBO
        // 0x43
        // ----------------------------------------------

        if (command == 0x43 && !editingExistingCombo)
        {

            resetCurrentHit();

            combo++;

            if (combo > 10)
            {
                combo = 1;
            }

            Serial.print(F("Combo: "));
            Serial.println(combo);

            refreshLCD();

            delay(300);
        }

        // ----------------------------------------------
        // RED / STOP
        // 0x45
        // ----------------------------------------------

        if (command == 0x45)
        {

            resetGame();

            Serial.println(F("STOP - GAME RESET"));

            delay(300);

            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print(F("STOP"));

            delay(300);
        }

        // ----------------------------------------------
        // VOL+ = FASTER
        // 0x46
        // ----------------------------------------------

        if (command == 0x46)
        {

            speed++;

            if (speed > 9)
            {
                speed = 9;
            }

            delayTime = 2000 - (speed * 200);

            Serial.print(F("Speed: "));
            Serial.print(speed);

            Serial.print(F("   Delay: "));
            Serial.println(delayTime);

            refreshLCD();

            delay(300);
        }

        // ----------------------------------------------
        // VOL- = SLOWER
        // 0x15
        // ----------------------------------------------

        if (command == 0x15)
        {

            speed--;

            if (speed < 0)
            {
                speed = 0;
            }

            delayTime = 2000 - (speed * 200);

            Serial.print(F("Speed: "));
            Serial.print(speed);

            Serial.print(F("   Delay: "));
            Serial.println(delayTime);

            refreshLCD();

            delay(300);
        }

        // OPEN MENU
        if (command == 0x19 && createComboOpen && comboEditing)
        {

            backComboAction();
            delay(300);
        }
        else if (command == 0x19)
        {
            if (southpawMenuOpen)
            {

                southpawMenuOpen = false;

                refreshMenu();
            }
            else if (editingExistingCombo)
            {

                editingExistingCombo = false;

                lcd.noBlink();
                lcd.noCursor();

                refreshEditComboSub();

                Serial.println(F("EDIT CANCELLED - RAM ONLY"));
            }

            else if (selectComboOpen)
            {

                // SELECT COMBO -> COMBO MENU
                selectComboOpen = false;
                subMenuItem = 0;
                refreshComboSub();
            }
            else if (createComboOpen)
            {

                // CREATE COMBO -> COMBO MENU
                createComboOpen = false;
                subMenuItem = 1;
                refreshComboSub();
            }
            else if (editComboOpen)
            {
                editComboOpen = false;
                subMenuItem = 2;
                refreshComboSub();
            }
            else if (deleteComboOpen)
            {

                deleteComboOpen = false;
                subMenuItem = 3;
                refreshComboSub();
            }
            else if (comboMenuOpen)
            {

                // COMBO MENU -> MAIN MENU
                comboMenuOpen = false;
                menuItem = 1;
                refreshMenu();
            }

            else
            {

                // NORMAL SCREEN -> MAIN MENU
                menuOpen = true;
                menuItem = 0;
                refreshMenu();
            }

            delay(300);
        }

        // SOUTHPAW ON/OFF SELECTION
        if (southpawMenuOpen &&
            (command == 0x09 || command == 0x07))
        {

            southpawSelection = !southpawSelection;

            refreshSouthpawMenu();
            delay(300);
        }

        // SCROLL MENU DOWN
        if (menuOpen && !comboMenuOpen && !southpawMenuOpen && command == 0x09)
        {

            menuItem--;

            if (menuItem < 0)
            {
                menuItem = 5;
            }

            refreshMenu();

            delay(300);
        }

        // SCROLL MENU UP
        if (menuOpen && !comboMenuOpen && !southpawMenuOpen && command == 0x07)
        {

            menuItem++;

            if (menuItem > 5)
            {
                menuItem = 0;
            }

            refreshMenu();

            delay(300);
        }

        // SCROLL COMBO SUBMENU DOWN
        if (comboMenuOpen && !selectComboOpen && !createComboOpen && !deleteComboOpen && !editComboOpen && command == 0x09)
        {

            subMenuItem--;

            if (subMenuItem < 0)
            {
                subMenuItem = 3;
            }

            refreshComboSub();

            delay(300);
        }

        // SCROLL COMBO SUBMENU UP
        if (comboMenuOpen && !selectComboOpen && !createComboOpen && !deleteComboOpen && !editComboOpen && command == 0x07)
        {

            subMenuItem++;

            if (subMenuItem > 3)
            {
                subMenuItem = 0;
            }

            refreshComboSub();

            delay(300);
        }
        // SCROLL COMBO SUBS DOWN
        if ((selectComboOpen || createComboOpen || editComboOpen || deleteComboOpen) && command == 0x09)
        {

            comboSelectItem--;

            if (comboSelectItem < (editComboOpen ? 3 : 0))
            {
                comboSelectItem = 9;
            }

            if (selectComboOpen)
            {
                refreshSelectComboSub();
            }
            if (deleteComboOpen)
            {
                refreshDeleteComboSub();
            }
            if (editComboOpen)
            {

                if (comboSelectItem >= 3 &&
                    comboSlots[comboSelectItem].used)
                {

                    startEditingExistingCombo();
                }
                else
                {

                    editingExistingCombo = false;

                    lcd.noBlink();
                    lcd.noCursor();

                    refreshEditComboSub();
                }
            }

            if (createComboOpen)
            {
                refreshCreateComboSub();
            }

            delay(300);
        }

        // SCROLL COMBO SUBS UP
        if ((selectComboOpen || createComboOpen || editComboOpen || deleteComboOpen) && command == 0x07)
        {

            comboSelectItem++;

            if (comboSelectItem > 9)
            {
                comboSelectItem = editComboOpen ? 3 : 0;
            }

            if (selectComboOpen)
            {
                refreshSelectComboSub();
            }

            if (createComboOpen)
            {
                refreshCreateComboSub();
            }

            if (editComboOpen)
            {

                if (comboSelectItem >= 3 &&
                    comboSlots[comboSelectItem].used)
                {

                    startEditingExistingCombo();
                }
                else
                {

                    editingExistingCombo = false;

                    lcd.noBlink();
                    lcd.noCursor();

                    refreshEditComboSub();
                }
            }
            if (deleteComboOpen)
            {
                refreshDeleteComboSub();
            }

            delay(300);
        }

        // MENU VALIDATE CHOICE
        // VALIDATE SEQUENCE EDITING
        if (editingExistingCombo)
        {

            int action = remoteNumber(command);

            if (action >= 0 && action <= 6)
            {
                replaceComboEditAction(action);
                delay(150);
            }
        }
        // ENTER ACTIONS WHILE CREATING A COMBO
        if (createComboOpen && comboEditing)
        {

            int action = remoteNumber(command);

            if (action >= 0 && action <= 6)
            {

                addComboAction(action);

                delay(300);
            }
        }

        // SAVE EDITED COMBO
        if (editingExistingCombo && command == 0x0D)
        {

            saveEditedCombo();
            delay(300);
        }

        // ENTER EDIT MODE FROM AN EMPTY-CURSOR BROWSER
        else if (editComboOpen &&
                 !editingExistingCombo &&
                 command == 0x0D)
        {

            startEditingExistingCombo();
            delay(300);
        }
        else if (southpawMenuOpen && command == 0x0D)
        {

            southpaw = southpawSelection;

            southpawMenuOpen = false;

            refreshMenu();

            Serial.print(F("SOUTHPAW: "));
            Serial.println(southpaw ? F("ON") : F("OFF"));

            delay(300);
        }
        else if (speedMenuOpen)
        {
            speedSubMenu(command);
        }
        else if (RNDMenuOpen)
        {
            RNDSubMenu(command);
        }
        else if (roundsMenuOpen)
        {
            roundsSubMenu(command);
        }

        else if (comboMenuOpen && !selectComboOpen && !createComboOpen && !editComboOpen && !deleteComboOpen && command == 0x0D)
        {

            if (subMenuItem == 0)
            {

                selectComboOpen = true;
                comboSelectItem = 0;
                refreshSelectComboSub();
                delay(300);
            }
            if (subMenuItem == 1)
            {

                createComboOpen = true;
                comboSelectItem = 0;
                refreshCreateComboSub();
                delay(300);
            }
            if (subMenuItem == 2)
            {
                editComboOpen = true;
                comboSelectItem = 3;

                if (comboSlots[comboSelectItem].used)
                {
                    startEditingExistingCombo();
                }
                else
                {
                    refreshEditComboSub();
                }

                delay(300);
            }
            if (subMenuItem == 3)
            {

                deleteComboOpen = true;
                comboSelectItem = 0;
                refreshDeleteComboSub();
                delay(300);
            }
        }
        else if (createComboOpen && comboEditing && command == 0x0D)
        {

            validateComboEditor();

            delay(300);
        }

        else if (createComboOpen && !comboEditing && command == 0x0D)
        {

            // Only custom slots 4-10, and only if EMPTY
            if (comboSelectItem >= 3 && !comboSlots[comboSelectItem].used)
            {

                // Start a fresh sequence
                comboEditLength = 0;
                comboEditing = true;

                refreshComboEditor();

                Serial.print(F("CREATING COMBO "));
                Serial.println(comboSelectItem + 1);
            }

            delay(300);
        }

        else if (selectComboOpen && command == 0x0D)
        {

            if (comboSlots[comboSelectItem].used)
            {

                combo = comboSelectItem + 1;

                selectComboOpen = false;
                comboMenuOpen = false;
                menuOpen = false;

                refreshLCD();

                Serial.print(F("Combo selected: "));
                Serial.println(combo);

                delay(300);
            }
        }

        else if (deleteComboOpen && command == 0x0D)
        {

            // Protect the three factory combos
            if (comboSelectItem >= 3 && comboSlots[comboSelectItem].used)
            {
                deleteCustomCombo(comboSelectItem);

                refreshDeleteComboSub();

                delay(300);
            }

            delay(300);
        }
        else if (menuOpen && !editingExistingCombo && !southpawMenuOpen && command == 0x0D)
        {

            // We were in the MAIN menu

            if (menuItem == 0)
            {

                speedMenuOpen = true;
                refreshSpeedSub();
            }
            if (menuItem == 1)
            {
                subMenuItem = 0;
                comboMenuOpen = true;
                refreshComboSub();
            }
            if (menuItem == 3)
            {
                roundsInput = 0;
                RNDMenuOpen = true;
                refreshRNDSub();
            }

            if (menuItem == 2)
            {
                randomnessInput = 0;
                roundsMenuOpen = true;
                refreshRoundsSub();
            }
            if (menuItem == 4)
            {

                southpawMenuOpen = true;

                // Start by highlighting the actual current setting
                southpawSelection = southpaw;

                refreshSouthpawMenu();
            }

            if (menuItem == 5)
            {

                menuOpen = false;
                refreshLCD();
            }
            delay(300);
        }

        ///////CHECK FOR NUMPAD INOUTS WHEN GAME PLAYS
        if (running)
        {

            int number = remoteNumber(command);

            // Serial.print("REMOTE NUMBER: ");
            // Serial.println(number);

            if (number != -1)
            {

                registerHit(number);
            }
        }

        IrReceiver.resume();

        // Serial.print("IR command: 0x");
        // Serial.println(command, HEX);
    }
    if (debugScreenOpen && millis() - lastDebugSignal >= 500)
    {

        debugScreenOpen = false;
        refreshLCD();
    }
    ////SHUT THE BUZZER
    if (buzzerOn && millis() - buzzerStartMillis >= 300)
    {

        /// Serial.println("SHUT THE HELL UP");

        digitalWrite(BUZZ, LOW);
        buzzerOn = false;
    }

    shutTheSpeedScreen();
    shutTheRandScreen();

    // --------------------------------------------------
    // GAME
    // --------------------------------------------------

    if (running)
    {

        if (combo == 1)
        {
            playCombo(COMBO1, COMBO1_LENGTH);
        }

        else if (combo == 2)
        {
            playCombo(COMBO2, COMBO2_LENGTH);
        }

        else if (combo == 3)
        {
            playCombo(COMBO3, COMBO3_LENGTH);
        }

        else if (combo >= 4 && combo <= 10)
        {

            int customIndex = combo - 4;
            int slotIndex = combo - 1;

            if (comboSlots[slotIndex].used &&
                comboSlots[slotIndex].length > 0)
            {

                playCombo(
                    customCombos[customIndex],
                    comboSlots[slotIndex].length);
            }
        }
    }
}
int randomiseHit(int target, int previousOriginalTarget)
{

    // ---------------------------------
    // RANDOMNESS SCALE
    //
    // 0-50   -> effective randomness 0-50
    // 51-100 -> effective randomness 1-50
    //           AND SLIP enters the pool
    // ---------------------------------

    int effectiveRandomness = randomness;

    if (randomness > 50)
    {
        effectiveRandomness = randomness - 50;
    }

    if (currentSequenceLength >= 2)
    {

        int lastHit = currentSequence[currentSequenceLength - 1];
        int previousHit = currentSequence[currentSequenceLength - 2];

        if (randomness > 0 && lastHit == 1 && previousHit == 1)
        {
            return 0;
        }
    }

    // ---------------------------------
    // DECIDE WHETHER TO RANDOMISE
    // ---------------------------------

    int roll = random(100); // 0-99

    if (roll >= effectiveRandomness)
    {
        return target;
    }

    // ---------------------------------
    // TARGET HAS BEEN SELECTED
    // FOR RANDOMISATION
    // ---------------------------------

    int possibleHits[5];
    int possibleCount = 0;

    // ---------------------------------
    // ODD TARGET
    // ---------------------------------

    if (target % 2 != 0)
    {

        if (target != 1)
            possibleHits[possibleCount++] = 1;
        if (target != 3)
            possibleHits[possibleCount++] = 3;
        if (target != 5)
            possibleHits[possibleCount++] = 5;
    }

    // ---------------------------------
    // EVEN TARGET
    // ---------------------------------

    else
    {

        if (target != 2)
            possibleHits[possibleCount++] = 2;
        if (target != 4)
            possibleHits[possibleCount++] = 4;
        if (target != 6)
            possibleHits[possibleCount++] = 6;
        possibleHits[possibleCount++] = 1;
    }

    // ---------------------------------
    // ABOVE 50:
    // SLIP JOINS THE POSSIBLE CHOICES
    // ---------------------------------

    if (randomness > 50)
    {
        possibleHits[possibleCount++] = 0;
    }

    // ---------------------------------
    // CHOOSE THE NEW TARGET
    // ---------------------------------

    int choice = random(possibleCount);

    return possibleHits[choice];
}
