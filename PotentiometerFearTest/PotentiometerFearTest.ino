//
// PotentiometerFearTest
//
// Description of the project
// Developed with [embedXcode](https://embedXcode.weebly.com)
//
// Author 		Omer Shamai
// 				___ORGANIZATIONNAME___
//
// Date			14/07/2020 11:20
// Version		<#version#>
//
// Copyright	© Omer Shamai, 2020
// Licence		<#licence#>
//
// See         ReadMe.txt for references
//


// Core library for code-sense - IDE-based
// !!! Help: http://bit.ly/2AdU7cu
#if defined(ENERGIA) // LaunchPad specific
#include "Energia.h"
#elif defined(TEENSYDUINO) // Teensy specific
#include "Arduino.h"
#elif defined(ESP8266) // ESP8266 specific
#include "Arduino.h"
#elif defined(ARDUINO) // Arduino 1.8 specific
#include "Arduino.h"
#else // error
#error Platform not defined
#endif // end IDE

// Set parameters


// Include application, user and local libraries
// !!! Help http://bit.ly/2CL22Qp
#include <SPI.h>
#include <Wire.h>
#include <SD.h>

// Define structures and classes


// Define variables and constants
#define SW_LO_PK                        3
#define SW_MID2_PK                     22
#define SW_HI_PK                       25
#define SW_FILTER_ON                   24
#define SW_BYPASS                      27

#define LED_FILTER_ON                  28
#define LED_HI_PK                      29
#define LED_MID2_PK                     6
#define LED_BYPASS                     47
#define LED_LO_PK                      44
#define MEM_INDICATOR                  40

#define MENU_BTN  9
#define SET_BTN   8

#define MENU_OUT 42
#define SET_OUT  43
#define UP_OUT   40
#define DOWN_OUT 41

enum parserSituations {
    FilterDeclaration,
    FilterIndex,
    FrequencyValue,
    DBValue,
    QValue,
    EndFilterDeclaration,
    LowPassFilterDeclaration,
    LowPassFilterData,
    EndLowPassFilterDeclaration,
    HighPassFilterDeclaration,
    HighPassFilterData,
    EndHighPassFilterDeclaration,
    LatchDeclaration,
    LatchIndex,
    LatchCounterValue,
    LatchStateValue,
    LatchLastStateValue,
    EndLatchDeclaration
};

class Filter {
public:
    int lastFreqReading;
    int lastDBReading;
    int lastQReading;
    
    int memFreqReading;
    int memDBReading;
    int memQReading;
};

class Latch {
public:
    int counter = 0;
    int state = 0;
    int lastState = 0;
    
    int memCounter;
    int memState;
    int memLastState;
    
    int tempCounter;
    int tempState;
    int tempLastState;
};

byte commandByte = 0x00;
byte dataByte = 0x00;

//int setCounter = 0;
//int setState = 0;
//int lastSetState = 0;

bool shouldUseMemory = false;
bool shouldShowMenu = false;

//int latches[0].counter = 0;
//int latches[0].state = 0;
//int latches[0].lastState = 0;

//int mid2Counter = 0;
//int mid2State = 0;
//int lastMid2PickState = 0;

//2068

//int latches[2].counter = 0;
//int latches[2].state = 0;
//int latches[2].lastState = 0;

//int latches[3].counter = 0;
//int latches[3].state = 0;
//int latches[3].lastState = 0;

//int latches[4].counter = 0;
//int latches[4].state = 0;
//int latches[4].lastState = 0;

Filter lowBand;
Filter mid1Band;
Filter mid2Band;
Filter highBand;

Latch lowPick;
Latch mid2;
Latch highPick;
Latch filterOn;
Latch bypass;

Filter filters[] = { lowBand, mid1Band, mid2Band, highBand };
Latch latches[] = { lowPick, mid2, highPick, filterOn, bypass };

/*int lastFreqReadingHigh;
int lastDBReadingHigh;
int lastQReadingHigh;

int lastFreqReadingMid2;
int lastDBReadingMid2;
int lastQReadingMid2;*/

int lastLowPassReading;
int memLowPassReading;

/*int lastFreqReadingMid1;
int lastDBReadingMid1;
int lastQReadingMid1;*/

int lastHighPassReading;
int memHighPassReading;

/*
int lastFreqReadingLow;
int lastDBReadingLow;
int lastQReadingLow;*/

File fileToRW;

// Prototypes
// !!! Help: http://bit.ly/2TAbgoI


// Utilities


// Functions
void receiveEvent(int howMany) {
    byte buffer[howMany];
    
    for (int i = 0; i < howMany; i++) {
        buffer[i] = 0x00;
    }
    
    Wire.readBytes(buffer, howMany);
    
    if (buffer[0] == 0x01) {
        Serial.println("RECEIVED 0x01");
    }
}

void load(int selection) {
    int keywordCount = 0;
    int currentCell = 0;
    String fileContents = "";
    String extractedKeyword = "";
    long frequencyData;
    int dbData;
    int qData;
    long lpfData;
    long hpfData;

    if (SD.exists(String(selection) + ".txt")) {
        fileToRW = SD.open(String(selection) + ".txt");
    } else {
        return;
    }

    while (fileToRW.available()) {
        char currentCharacter = fileToRW.read();
        fileContents += currentCharacter;

        if (currentCharacter == ' ') {
            keywordCount++;
        }
    }

    keywordCount++;

    Serial.print("Keyword count: ");
    Serial.println(String(keywordCount));

    String extractedKeywords[keywordCount];

    Serial.println(fileContents);

    for (int i = 0; i <= fileContents.length() + 7; i++) {
        if (fileContents[i] != ' ') {
            extractedKeyword += fileContents[i];
        }

        if (fileContents[i] == ' ') {
            extractedKeywords[currentCell] = extractedKeyword;
            currentCell++;

            Serial.print("The extracted keyword is: ");
            Serial.println(extractedKeyword);
            extractedKeyword = "";
        }
    }

    fileToRW.close();

    int currentFilter;
    int currentLatch;
    String lastKeyword;
    parserSituations lastKeywordDescription;
    for (int i = 0; i < keywordCount; i++) {
        if (extractedKeywords[i] == "FILTER") {
            lastKeyword = extractedKeywords[i];
            lastKeywordDescription = FilterDeclaration;
            continue;
        }

        if ((extractedKeywords[i] == "0" || "1" || "2" || "3") && (lastKeywordDescription == FilterDeclaration) && lastKeyword == "FILTER") {
            currentFilter = extractedKeywords[i].toInt();
            Serial.print("CURRENT FILTER: ");
            Serial.println(currentFilter);
            lastKeyword = extractedKeywords[i];
            lastKeywordDescription = FilterIndex;
            continue;
        }

        if (lastKeywordDescription == FilterIndex) {
            //frequencyData = strtol(extractedKeywords[i].c_str(), NULL, 10);
            frequencyData = extractedKeywords[i].toInt();
            filters[currentFilter].memFreqReading = frequencyData;
            Serial.println(frequencyData);
            lastKeyword = extractedKeywords[i];
            lastKeywordDescription = FrequencyValue;
            continue;
        }

        if (lastKeywordDescription == FrequencyValue) {
            dbData = extractedKeywords[i].toInt();
            filters[currentFilter].memDBReading = dbData;
            Serial.println(dbData);
            lastKeyword = extractedKeywords[i];
            lastKeywordDescription = DBValue;
            continue;
        }

        if (lastKeywordDescription == DBValue) {
            qData = strtol(extractedKeywords[i].c_str(), NULL, 10);
            //qData = extractedKeywords[currentFilter].toInt();
            filters[currentFilter].memQReading = qData;
            Serial.println(extractedKeywords[i].c_str());
            //Serial.println(int(test));
            lastKeyword = extractedKeywords[i];
            lastKeywordDescription = QValue;
            continue;
        }

        if ((lastKeywordDescription == QValue) && (extractedKeywords[i] == "END")) {
            lastKeyword = extractedKeywords[i];
            lastKeywordDescription = EndFilterDeclaration;

            //filters[currentFilter].memFreqReading = frequencyData;
            //filters[currentFilter].memDBReading = dbData;
            //filters[currentFilter].memQReading = qData;
            continue;
        }
        
        if ((lastKeywordDescription == EndFilterDeclaration) && (extractedKeywords[i] == "LPF")) {
            lastKeyword = extractedKeywords[i];
            lastKeywordDescription = LowPassFilterDeclaration;
            continue;
        }
        
        if ((lastKeywordDescription == LowPassFilterDeclaration)) {
            lpfData = strtol(extractedKeywords[i].c_str(), NULL, 10);
            lastKeyword = extractedKeywords[i];
            lastKeywordDescription = LowPassFilterData;
            continue;
        }
        
        if ((lastKeywordDescription == LowPassFilterData) && (extractedKeywords[i] == "END")) {
            memLowPassReading = lpfData;
            lastKeyword = extractedKeywords[i];
            lastKeywordDescription = EndLowPassFilterDeclaration;
            continue;
        }
        
        if ((lastKeywordDescription == EndLowPassFilterDeclaration) && (extractedKeywords[i] == "HPF")) {
            lastKeyword = extractedKeywords[i];
            lastKeywordDescription = HighPassFilterDeclaration;
            continue;
        }
        
        if ((lastKeywordDescription == HighPassFilterDeclaration)) {
            hpfData = strtol(extractedKeywords[i].c_str(), NULL, 10);
            lastKeyword = extractedKeywords[i];
            lastKeywordDescription = HighPassFilterData;
            continue;
        }
        
        if ((lastKeywordDescription == HighPassFilterData) && (extractedKeywords[i] == "END")) {
            memHighPassReading = hpfData;
            lastKeyword = extractedKeywords[i];
            lastKeywordDescription = EndHighPassFilterDeclaration;
            continue;
        }
        
        if (extractedKeywords[i] == "LATCH") {
            lastKeyword = extractedKeywords[i];
            lastKeywordDescription = LatchDeclaration;
            continue;
        }
        
        if ((lastKeywordDescription == LatchDeclaration) && (extractedKeywords[i] == "0" || "1" || "2" || "3" || "4")) {
            currentLatch = extractedKeywords[i].toInt();
            Serial.println("CURRENTLY IN LATCH " + String(currentLatch));
            lastKeywordDescription = LatchIndex;
            lastKeyword = extractedKeywords[i];
            continue;
        }
        
        if (lastKeywordDescription == LatchIndex) {
            Serial.println("EXTRACTED LATCH COUNTER: " + String(extractedKeywords[i].toInt()));
            latches[currentLatch].memCounter = extractedKeywords[i].toInt();
            lastKeyword = extractedKeywords[i];
            lastKeywordDescription = LatchCounterValue;
            continue;
        }
        
        if (lastKeywordDescription == LatchCounterValue) {
            latches[currentLatch].memState = extractedKeywords[i].toInt();
            lastKeyword = extractedKeywords[i];
            lastKeywordDescription = LatchStateValue;
            continue;
        }
        
        if (lastKeywordDescription == LatchStateValue) {
            latches[currentLatch].memLastState = extractedKeywords[i].toInt();
            lastKeyword = extractedKeywords[i];
            lastKeywordDescription = LatchLastStateValue;
            continue;
        }
        
        if (lastKeywordDescription == LatchLastStateValue && extractedKeywords[i] == "END") {
            lastKeyword = extractedKeywords[i];
            lastKeywordDescription = EndLatchDeclaration;
            continue;
        }
        
        // COUNTER, STATE AND THEN LASTSTATE

        if (extractedKeywords[i] == "ENDFIN") {
            break;
        }
    }
    
    for (int i = 0; i < 5; i++) {
        Serial.println("latches[" + String(i) + "].memCounter = " + String(latches[i].memCounter));
        Serial.println("latches[" + String(i) + "].memState = " + String(latches[i].memState));
        Serial.println("latches[" + String(i) + "].memLastState = " + String(latches[i].memLastState));
    }
    
    Serial.println("VALUES:");
    
    for (int i = 0; i < 4; i++) {
        Serial.print("filters[" + String(i) + "].memFreqReading: ");
        Serial.println(filters[i].memFreqReading);
        
        Serial.print("filters[" + String(i) + "].memDBReading: ");
        Serial.println(filters[i].memDBReading);
        
        Serial.print("filters[" + String(i) + "].memQReading: ");
        Serial.println(filters[i].memQReading);
    }
    
    Serial.print("memLowPassReading: ");
    Serial.println(memLowPassReading);
    
    Serial.print("memHighPassReading: ");
    Serial.println(memHighPassReading);
    //*_parentMemoryBool = true;
    
    delay(1000);
}

void save(int selection) {
    Serial.println(String(selection) + ".txt");
    if (SD.exists(String(selection) + ".txt")) {
        SD.remove(String(selection) + ".txt");

        fileToRW = SD.open(String(selection) + ".txt", FILE_WRITE);
    } else {
        fileToRW = SD.open(String(selection) + ".txt", FILE_WRITE);
        fileToRW.close();

        if(SD.exists(String(selection) + ".txt")) {
            Serial.println("New file created");
            fileToRW = SD.open(String(selection) + ".txt", FILE_WRITE);
        }
    }

    if (fileToRW) {
        //fileToRW.seek(0);

        //fileToRW.println("This is a test");
        //fileToRW.close();
        for (int i = 0; i < 4; i++) {
            long freqToSave = filters[i].lastFreqReading;
            int dbToSave = filters[i].lastDBReading;
            int qToSave = filters[i].lastQReading;

            Serial.println(freqToSave);
            Serial.println(String(dbToSave));
            Serial.println(String(qToSave));

            fileToRW.print("FILTER ");
            fileToRW.print(String(i));
            fileToRW.print(" ");
            
            fileToRW.print(String(freqToSave) + " ");
            fileToRW.print(String(dbToSave) + " ");
            fileToRW.print(String(qToSave) + " ");
            
            fileToRW.print("END ");
        }
        
        long lpfFrequencyToSave = lastLowPassReading;
        Serial.print("LPF: ");
        Serial.println(lpfFrequencyToSave);
        
        fileToRW.print("LPF ");
        fileToRW.print(String(lpfFrequencyToSave) + " ");
        fileToRW.print("END ");
        
        long hpfFrequencyToSave = lastHighPassReading;
        Serial.print("HPF: ");
        Serial.println(String(hpfFrequencyToSave));
        
        fileToRW.print("HPF ");
        fileToRW.print(String(hpfFrequencyToSave) + " ");
        fileToRW.print("END ");
        
        //int setCounterToSave = setCounter;
        //int setStateToSave = setState;
        
        for (int i = 0; i < 5; i++) {
            fileToRW.print("LATCH ");
            fileToRW.print(String(i) + " ");
            
            Serial.println("latches[" + String(i) + "].counter = " + String(latches[i].counter));
            Serial.println("latches[" + String(i) + "].state = " + String(latches[i].state));
            Serial.println("latches[" + String(i) + "].lastState = " + String(latches[i].lastState));
            
            fileToRW.print(String(latches[i].counter) + " ");
            fileToRW.print(String(latches[i].state) + " ");
            fileToRW.print(String(latches[i].lastState) + " ");
            
            fileToRW.print("END ");
        }
        
        fileToRW.print("ENDFIN ");
        fileToRW.close();

        Serial.println("SUCCESS WRITING TO FILE!");
    } else {
        Serial.println("Unable to open the file");
    }
    
    //delay(1000);
    
    load(selection);
}

/*void load(int selection) {
    int keywordCount = 0;
    int currentCell = 0;
    String fileContents = "";
    String extractedKeyword = "";
    long frequencyData;
    int dbData;
    int qData;
    long lpfData;
    long hpfData;

    if (SD.exists(String(selection) + ".txt")) {
        fileToRW = SD.open(String(selection) + ".txt");
    } else {
        return;
    }

    while (fileToRW.available()) {
        char currentCharacter = fileToRW.read();
        fileContents += currentCharacter;

        if (currentCharacter == ' ') {
            keywordCount++;
        }
    }

    keywordCount++;

    Serial.print("Keyword count: ");
    Serial.println(String(keywordCount));

    String extractedKeywords[keywordCount];

    Serial.println(fileContents);

    for (int i = 0; i <= fileContents.length() + 7; i++) {
        if (fileContents[i] != ' ') {
            extractedKeyword += fileContents[i];
        }

        if (fileContents[i] == ' ') {
            extractedKeywords[currentCell] = extractedKeyword;
            currentCell++;

            Serial.print("The extracted keyword is: ");
            Serial.println(extractedKeyword);
            extractedKeyword = "";
        }
    }

    fileToRW.close();

    int currentFilter;
    String lastKeyword;
    parserSituations lastKeywordDescription;
    for (int i = 0; i < keywordCount; i++) {
        if (extractedKeywords[i] == "FILTER") {
            lastKeyword = extractedKeywords[i];
            lastKeywordDescription = FilterDeclaration;
            continue;
        }

        if ((extractedKeywords[i] == "0" || "1" || "2" || "3") && (lastKeywordDescription == FilterDeclaration) && lastKeyword == "FILTER") {
            currentFilter = extractedKeywords[i].toInt();
            Serial.print("CURRENT FILTER: ");
            Serial.println(currentFilter);
            lastKeyword = extractedKeywords[i];
            lastKeywordDescription = FilterIndex;
            continue;
        }

        if (lastKeywordDescription == FilterIndex) {
            //frequencyData = strtol(extractedKeywords[i].c_str(), NULL, 10);
            frequencyData = extractedKeywords[i].toInt();
            filters[currentFilter].memFreqReading = frequencyData;
            Serial.println(frequencyData);
            lastKeyword = extractedKeywords[i];
            lastKeywordDescription = FrequencyValue;
            continue;
        }

        if (lastKeywordDescription == FrequencyValue) {
            dbData = extractedKeywords[i].toInt();
            filters[currentFilter].memDBReading = dbData;
            Serial.println(dbData);
            lastKeyword = extractedKeywords[i];
            lastKeywordDescription = DBValue;
            continue;
        }

        if (lastKeywordDescription == DBValue) {
            qData = strtol(extractedKeywords[i].c_str(), NULL, 10);
            //qData = extractedKeywords[currentFilter].toInt();
            filters[currentFilter].memQReading = qData;
            Serial.println(extractedKeywords[i].c_str());
            //Serial.println(int(test));
            lastKeyword = extractedKeywords[i];
            lastKeywordDescription = QValue;
            continue;
        }

        if ((lastKeywordDescription == QValue) && (extractedKeywords[i] == "END")) {
            lastKeyword = extractedKeywords[i];
            lastKeywordDescription = EndFilterDeclaration;

            //filters[currentFilter].memFreqReading = frequencyData;
            //filters[currentFilter].memDBReading = dbData;
            //filters[currentFilter].memQReading = qData;
            continue;
        }
        
        if ((lastKeywordDescription == EndFilterDeclaration) && (extractedKeywords[i] == "LPF")) {
            lastKeyword = extractedKeywords[i];
            lastKeywordDescription = LowPassFilterDeclaration;
            continue;
        }
        
        if ((lastKeywordDescription == LowPassFilterDeclaration)) {
            lpfData = strtol(extractedKeywords[i].c_str(), NULL, 10);
            lastKeyword = extractedKeywords[i];
            lastKeywordDescription = LowPassFilterData;
            continue;
        }
        
        if ((lastKeywordDescription == LowPassFilterData) && (extractedKeywords[i] == "END")) {
            memLowPassReading = lpfData;
            lastKeyword = extractedKeywords[i];
            lastKeywordDescription = EndLowPassFilterDeclaration;
            continue;
        }
        
        if ((lastKeywordDescription == EndLowPassFilterDeclaration) && (extractedKeywords[i] == "HPF")) {
            lastKeyword = extractedKeywords[i];
            lastKeywordDescription = HighPassFilterDeclaration;
            continue;
        }
        
        if ((lastKeywordDescription == HighPassFilterDeclaration)) {
            hpfData = strtol(extractedKeywords[i].c_str(), NULL, 10);
            lastKeyword = extractedKeywords[i];
            lastKeywordDescription = HighPassFilterData;
            continue;
        }
        
        if ((lastKeywordDescription == HighPassFilterData) && (extractedKeywords[i] == "END")) {
            memHighPassReading = hpfData;
            lastKeyword = extractedKeywords[i];
            lastKeywordDescription = EndHighPassFilterDeclaration;
            continue;
        }

        if (extractedKeywords[i] == "ENDFIN") {
            break;
        }
    }
    
    Serial.println("VALUES:");
    
    for (int i = 0; i < 4; i++) {
        Serial.print("filters[" + String(i) + "].memFreqReading: ");
        Serial.println(filters[i].memFreqReading);
        
        Serial.print("filters[" + String(i) + "].memDBReading: ");
        Serial.println(filters[i].memDBReading);
        
        Serial.print("filters[" + String(i) + "].memQReading: ");
        Serial.println(filters[i].memQReading);
    }
    
    Serial.print("memLowPassReading: ");
    Serial.println(memLowPassReading);
    
    Serial.print("memHighPassReading: ");
    Serial.println(memHighPassReading);
    //*_parentMemoryBool = true;
    
    delay(1000);
}*/

String makeTextFrequency(String replacementText) {
    String modifiedReplacementText = replacementText;
    //_displayHandler.setPrintPos(freqCursorX, freqCursorY);
    //_displayHandler.setColor(0, _textColorR, _textColorG, _textColorB);
    
    switch (modifiedReplacementText.length()) {
        case 1:
            modifiedReplacementText =  "00" + modifiedReplacementText;
            break;
    
        case 2:
            modifiedReplacementText = "0" + modifiedReplacementText;
            break;
    
        default:
            if (String(17000).length() - modifiedReplacementText.length() == 1) {
                //Serial.print("Frequency: ");
                //Serial.println(modifiedReplacementText);
                modifiedReplacementText = "0" + modifiedReplacementText;
            } else if (String(17000).length() - modifiedReplacementText.length() == 2) {
                modifiedReplacementText = "00" + modifiedReplacementText;
            } else {
                modifiedReplacementText = modifiedReplacementText;
            }
            break;
    }
    
    //_displayHandler.print(modifiedReplacementText);
    
    return modifiedReplacementText;
}

String makeTextDB(String replacementText) {
    String modifiedReplacementText = replacementText;
    //_displayHandler.setPrintPos(dbCursorX, dbCursorY);
    //_displayHandler.setColor(0, _textColorR, _textColorG, _textColorB);
    
    modifiedReplacementText.remove(replacementText.length() - 1);
    
    if (atof(modifiedReplacementText.c_str()) > 0) {
        modifiedReplacementText = "+" + modifiedReplacementText;
    }
    
    if (modifiedReplacementText == "+10.0") {
        modifiedReplacementText = "+10";
    }
    if (modifiedReplacementText == "-10.0") {
        modifiedReplacementText = "-10";
    }
    if (modifiedReplacementText == "+0.0" || modifiedReplacementText == "-0.0") {
        modifiedReplacementText = "+0.0";
    }
    //_displayHandler.print(modifiedReplacementText);
    
    if (modifiedReplacementText.length() == 1) {
        //_displayHandler.print("   ");
        modifiedReplacementText += "   ";
        return modifiedReplacementText;
    }
    if (modifiedReplacementText.length() == 2) {
        //_displayHandler.print("  ");
        modifiedReplacementText += "  ";
        return modifiedReplacementText;
    }
    if (modifiedReplacementText.length() == 3) {
        //_displayHandler.print(" ");
        modifiedReplacementText += " ";
        return modifiedReplacementText;
    }
}

String makeTextQ(String replacementText) {
    String modifiedReplacementText = replacementText;
    //_displayHandler.setPrintPos(qCursorX, qCursorY);
    //_displayHandler.setColor(0, _textColorR, _textColorG, _textColorB);
    
    switch (modifiedReplacementText.length()) {
        case 1:
            modifiedReplacementText = "00" + modifiedReplacementText;
            break;
            
        case 2:
            modifiedReplacementText = "0" + modifiedReplacementText;
            break;
            
        default:
            break;
    }
    
    //_displayHandler.print(modifiedReplacementText);
    
    return modifiedReplacementText;
}

String makeTextLowPass(String replacementText) {
    String modifiedReplacementText = replacementText;
    
    switch (modifiedReplacementText.length()) {
        case 1:
            modifiedReplacementText = "0000" + modifiedReplacementText;
            break;
        case 2:
            modifiedReplacementText = "000" + modifiedReplacementText;
            break;
            
        case 3:
            modifiedReplacementText = "00" + modifiedReplacementText;
            break;
            
        case 4:
            modifiedReplacementText = "0" + modifiedReplacementText;
            
        default:
            
            break;
    }
    
    return modifiedReplacementText;
}

String makeTextHighPass(String replacementText) {
    String modifiedReplacementText = replacementText;
    
    switch (modifiedReplacementText.length()) {
        case 2:
            modifiedReplacementText = "00" + modifiedReplacementText;
            break;
            
        case 3:
            modifiedReplacementText = "0" + modifiedReplacementText;
            break;
            
        default:
            break;
    }
    
    return modifiedReplacementText;
}

bool didFrequencyChange(int wiper, int* lastWiper) {
    return wiper != lastWiper;
}

bool didDBChange(int wiper, int* lastWiper) {
    return wiper != lastWiper;
}

bool didQChange(int wiper, int* lastWiper) {
    return wiper != lastWiper;
}


float mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

word wiperToInstruction(unsigned int val) {
    byte highbyte = (val >> 8) + 0x04;
    byte lowbyte = val & 0xFF;
    
    word combined = (highbyte << 8) | (lowbyte);
    return combined;
}

float freqToResistanceAntiLog(long frequency, float _capFarads) {
    float resistance = (1 / (2 * PI * frequency * _capFarads));
    return mapFloat(resistance, 2400.53, 51011.20, 2400, 50000);
}

double travelToResistanceLog(float travel) {
    double b = 1 / 0.15 - 1;
    b = b * b;
    double a = 1 / (b - 1);
    
    double resistance = (a * pow(b, travel)) - a;
    return resistance;
}

unsigned int resistanceToWiper(long maximumResistance, long resistance) {
    double long wiper = (resistance * 1023) / maximumResistance;
    return int(wiper);
}

// Add setup code
void setup()
{
    pinMode(LED_FILTER_ON, OUTPUT);
    pinMode(LED_HI_PK, OUTPUT);
    pinMode(LED_MID2_PK, OUTPUT);
    pinMode(LED_BYPASS, OUTPUT);
    pinMode(LED_LO_PK, OUTPUT);
    pinMode(MENU_BTN, INPUT);
    pinMode(SET_BTN, INPUT);
    pinMode(SW_LO_PK, INPUT);
    pinMode(SW_MID2_PK, INPUT);
    pinMode(SW_HI_PK, INPUT);
    pinMode(SW_FILTER_ON, INPUT);
    pinMode(SW_BYPASS, INPUT);
    pinMode(MENU_OUT, OUTPUT);
    pinMode(SET_OUT, OUTPUT);
    pinMode(MEM_INDICATOR, OUTPUT);
    pinMode(4, OUTPUT);
    //pinMode(53, OUTPUT);
    pinMode(12, OUTPUT);
    
    SPI.begin();
    //SPI.setClockDivider(SPI_CLOCK_DIV2);
    //SPI.setBitOrder(MSBFIRST);
    //SPI.setDataMode(SPI_MODE1);
    digitalWrite(MENU_OUT, HIGH);
    digitalWrite(SET_OUT, HIGH);
    digitalWrite(UP_OUT, HIGH);
    digitalWrite(DOWN_OUT, HIGH);
    digitalWrite(MEM_INDICATOR, LOW);
    
    Serial.begin(9600);
    
    delay(3000);
    
    /*SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE1));
    digitalWrite(53, LOW);
    SPI.transfer16(0x1802);
    SPI.transfer16(0x1802);
    SPI.transfer16(0x1802);
    SPI.transfer16(0x1802);
    SPI.transfer16(0x1802);
    SPI.transfer16(0x1802);
    SPI.transfer16(0x1802);
    SPI.transfer16(0x1802);
    digitalWrite(53, HIGH);
    SPI.endTransaction();
    
    digitalWrite(12, LOW);
    SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE1));
    SPI.transfer16(0x1802);
    SPI.transfer16(0x1802);
    SPI.transfer16(0x1802);
    SPI.transfer16(0x1802);
    SPI.transfer16(0x1802);
    SPI.transfer16(0x1802);
    SPI.transfer16(0x1802);
    SPI.transfer16(0x1802);
    SPI.transfer16(0x1802);
    SPI.transfer16(0x1802);
    SPI.transfer16(0x1802);
    SPI.transfer16(0x1802);
    
    //delay(70);
    digitalWrite(12, HIGH);
    SPI.endTransaction();*/
    
    SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE1));
    digitalWrite(53, LOW);
    delay(30);
    
    SPI.transfer16(0x1802);
    SPI.transfer16(0x1802);
    SPI.transfer16(0x1802);
    SPI.transfer16(0x1802);
    SPI.transfer16(0x1802);
    delay(10);
    SPI.transfer16(0x1802);
    //SPI.transfer16(0x0000);
    SPI.transfer16(0x1802);
    SPI.transfer16(0x1802);
    SPI.transfer16(0x1802);
    SPI.transfer16(0x1802);
    //SPI.transfer16(0x1802);
    //SPI.transfer16(0x1802);
    SPI.transfer16(0x1802);
    SPI.transfer16(0x1802);
    SPI.transfer16(0x1802);
    SPI.transfer16(0x1802);
    SPI.transfer16(0x1802);
    SPI.transfer16(0x1802);
    SPI.transfer16(0x1802);
    SPI.transfer16(0x1802);
    
    delay(30);
    digitalWrite(53, HIGH);
    SPI.endTransaction();
    
    delay(1);
    
    if (!SD.begin(2)) {
        Serial.println("SD FS Initialization Failed!");
    } else {
        Serial.println("SD FS Initialization Complete!");
    }
    
    Wire.begin();
    Wire.onReceive(receiveEvent);
    Wire.setTimeout(1);
}

// Add loop code
void loop()
{
    /*if (shouldShowMenu) {
        Wire.requestFrom(4, 2);
        
        commandByte = Wire.read();
        dataByte = Wire.read();
        
        if (commandByte == 0x10) {
            save((int) dataByte);
        } else if (commandByte == 0x20) {
            load((int) dataByte);
        }
    }
    
    if (commandByte == 0x02) {
        if (dataByte == 0x01) {
            shouldUseMemory = true;
            
            Wire.beginTransmission(4);
            Wire.write(0x90);
            Wire.endTransmission();
        } else {
            shouldUseMemory = false;
            
            Wire.beginTransmission(4);
            Wire.write(0x80);
            Wire.endTransmission();
            
            for (int i = 0; i < 4; i++) {
                filters[i].lastFreqReading = -1;
                filters[i].lastDBReading = -1;
                filters[i].lastQReading = -1;
            }
            
            lastHighPassReading = -1;
            lastLowPassReading = -1;
        }
    }*/
    
    // MARK: - LOW LATCH
    latches[0].state = digitalRead(SW_LO_PK);
    
    if (latches[0].state != latches[0].lastState) {
        if (latches[0].state == HIGH) {
            latches[0].counter++;
        }
        
        delay(50);
    }
    
    //latches[0].lastState = latches[0].state;
    latches[0].lastState = latches[0].state;
    
    if (latches[0].counter % 2 == 0) {
        digitalWrite(LED_LO_PK, HIGH);
    } else {
        digitalWrite(LED_LO_PK, LOW);
    }
    
    // MARK: - MID2 LATCH
    latches[1].state = digitalRead(SW_MID2_PK);
    
    if (latches[1].state != latches[1].lastState) {
        if (latches[1].state == HIGH) {
            latches[1].counter++;
        }
        
        delay(50);
    }
    
    latches[1].lastState = latches[1].state;
    
    if (latches[1].counter % 2 == 0) {
        digitalWrite(LED_MID2_PK, HIGH);
    } else {
        digitalWrite(LED_MID2_PK, LOW);
    }
    
    // MARK: - HIGH LATCH
    latches[2].state = digitalRead(SW_HI_PK);
    
    if (latches[2].state != latches[2].lastState) {
        if (latches[2].state == HIGH) {
            latches[2].counter++;
        }
        
        delay(50);
    }
    
    latches[2].lastState = latches[2].state;
    
    if (latches[2].counter % 2 == 0) {
        digitalWrite(LED_HI_PK, HIGH);
    } else {
        digitalWrite(LED_HI_PK, LOW);
    }
    
    // MARK: - FILTER ON LATCH
    latches[3].state = digitalRead(SW_FILTER_ON);
    
    if (latches[3].state != latches[3].lastState) {
        if (latches[3].state == HIGH) {
            latches[3].counter++;
        }
        
        delay(50);
    }
    
    latches[3].lastState = latches[3].state;
    
    if (latches[3].counter % 2 == 0) {
        digitalWrite(LED_FILTER_ON, LOW);
    } else {
        digitalWrite(LED_FILTER_ON, HIGH);
    }
    
    // MARK: - BYPASS LATCH
    latches[4].state = digitalRead(SW_BYPASS);
    
    if (latches[4].state != latches[4].lastState) {
        if (latches[4].state == HIGH) {
            latches[4].counter++;
        }
        
        delay(50);
    }
    
    latches[4].lastState = latches[4].state;
    
    if (latches[4].counter % 2 == 0) {
        digitalWrite(LED_BYPASS, LOW);
    } else {
        digitalWrite(LED_BYPASS, HIGH);
    }
    
    if (shouldUseMemory) {
        char buffer[20];
        
        for (int i = 0; i < sizeof(buffer); i++) {
            buffer[i] == 0x00;
        }
        
        // MARK: - Memory Frequency High
        makeTextFrequency(String(map(filters[3].memFreqReading, 0, 1023, 800, 17000))).toCharArray(buffer, 20);
        //delay(100);
        Wire.beginTransmission(4);
        Wire.write(0x30);
        Wire.write(buffer);
        //Wire.write("040");
        Wire.endTransmission(true);
        
        // MARK: - Memory DB High
        makeTextDB(String(mapFloat(filters[3].memDBReading, 0.0, 1023.0, -10.0, 10.0))).toCharArray(buffer, 20);
        //delay(100);
        Wire.beginTransmission(4);
        Wire.write(0x31);
        Wire.write(buffer);
        //Wire.write("-10");
        Wire.endTransmission(true);
        
        // MARK: - Memory Q High
        makeTextQ(String(map(filters[3].memQReading, 0, 1023, 0, 100))).toCharArray(buffer, 20);
        //delay(100);
        Wire.beginTransmission(4);
        Wire.write(0x32);
        Wire.write(buffer);
        //Wire.write("000");
        Wire.endTransmission(true);
        
        // MARK: - Math Memory High
        long frequency = map(filters[3].memFreqReading, 0, 1023, 800, 17000);
        float resistanceFromFrequency = freqToResistanceAntiLog(frequency, 0.0000000039);
        int wiperFromResistance = resistanceToWiper(50000, resistanceFromFrequency);
        word frequencyInstruction = wiperToInstruction(wiperFromResistance);
        //int dbWiper = ;
        float db = mapFloat(filters[3].memDBReading, 0.0, 1023.0, -10.0, 10.0);
        word dbInstruction = wiperToInstruction(filters[3].memDBReading);
        //int qReading = analogRead(A4);
        int q = map(filters[3].memQReading, 0, 1023, 0, 100);
        float travel = mapFloat(q, 0.0, 100.0, 0.0, 1.0);
        float resistanceFromTravel = 20000 * travelToResistanceLog(travel);
        int wiperFromTravel = resistanceToWiper(20000, resistanceFromTravel);
        word qInstruction = wiperToInstruction(wiperFromTravel);
        
        // MARK: - Memory Frequency Mid-2
        makeTextFrequency(String(map(filters[2].memFreqReading, 0, 1023, 400, 8000))).toCharArray(buffer, 20);
        //delay(100);
        Wire.beginTransmission(4);
        Wire.write(0x20);
        Wire.write(buffer);
        Wire.endTransmission();
        
        // MARK: - Memory DB Mid-2
        makeTextDB(String(mapFloat(filters[2].memDBReading, 0.0, 1023.0, -10.0, 10.0))).toCharArray(buffer, 20);
        //delay(100);
        Wire.beginTransmission(4);
        Wire.write(0x21);
        Wire.write(buffer);
        Wire.endTransmission(true);
        
        // MARK: - Memory Q Mid-2
        makeTextQ(String(map(filters[2].memQReading, 0, 1023, 0, 100))).toCharArray(buffer, 20);
        //delay(100);
        Wire.beginTransmission(4);
        Wire.write(0x22);
        Wire.write(buffer);
        Wire.endTransmission(true);
        
        // MARK: - Math Memory Mid-2
        long frequency2 = map(filters[2].memFreqReading, 0, 1023, 400, 8000);
        float resistanceFromFrequency2 = freqToResistanceAntiLog(frequency2, 0.0000000082);
        int wiperFromResistance2 = resistanceToWiper(50000, resistanceFromFrequency2);
        word frequencyInstruction2 = wiperToInstruction(wiperFromResistance2);
        
        word dbInstruction2 = wiperToInstruction(filters[2].memDBReading);
        
        int q2 = map(filters[2].memQReading, 0, 1023, 0, 100);
        float travel2 = mapFloat(q2, 0.0, 100.0, 0.0, 1.0);
        float resistanceFromTravel2 = 5000 * travelToResistanceLog(travel2);
        int wiperFromTravel2 = resistanceToWiper(20000, resistanceFromTravel2);
        word qInstruction2 = wiperToInstruction(wiperFromTravel2);
        
        // MARK: - Memory Low-Pass
        makeTextLowPass(String(map(memLowPassReading, 0, 1023, long(96.46), 14615))).toCharArray(buffer, 20);
        //delay(100);
        Wire.beginTransmission(4);
        Wire.write(0x40);
        Wire.write(buffer);
        Wire.endTransmission(true);
        
        // MARK: - Math Memory Low-Pass
        long lowPassFreq = map(memLowPassReading, 0, 1023, long(96.46), long(14615));
        float lowPassResistance = (1 / (2 * PI * lowPassFreq * 0.000000033));
        lowPassResistance = mapFloat(lowPassResistance, 330.0, 50238.30, 330.0, 50000.0);
        int lowPassWiper = 1023 - resistanceToWiper(50000, lowPassResistance);
        word lowPassInstruction = wiperToInstruction(lowPassWiper);
         
        // MARK: - Memory Frequency Mid-1
        makeTextFrequency(String(map(filters[1].memFreqReading, 0, 1023, 400, 8000))).toCharArray(buffer, 20);
        //delay(100);
        Wire.beginTransmission(4);
        Wire.write(0x10);
        Wire.write(buffer);
        Wire.endTransmission(true);
        
        // MARK: - Memory DB Mid-1
        makeTextDB(String(mapFloat(filters[1].memDBReading, 0.0, 1023.0, -10.0, 10.0))).toCharArray(buffer, 20);
        //delay(100);
        Wire.beginTransmission(4);
        Wire.write(0x11);
        Wire.write(buffer);
        Wire.endTransmission(true);
        
        // MARK: - Memory Q Mid-1
        makeTextQ(String(map(filters[1].memQReading, 0, 1023, 0, 100))).toCharArray(buffer, 20);
        //delay(100);
        Wire.beginTransmission(4);
        Wire.write(0x12);
        Wire.write(buffer);
        Wire.endTransmission(true);
        
        // MARK: - Math Memory Mid-1
        long frequencyMid1 = map(filters[1].memFreqReading, 0, 1023, 400, 8000);
        float resistanceFromFrequencyMid1 = freqToResistanceAntiLog(frequencyMid1, 0.0000000082);
        int wiperFromResistanceMid1 = resistanceToWiper(50000, resistanceFromFrequencyMid1);
        word frequencyInstructionMid1 = wiperToInstruction(wiperFromResistanceMid1);
        
        //int dbWiper2 = dbReading;
        word dbInstructionMid1 = wiperToInstruction(filters[1].memDBReading);
        
        //int qReading2 = qRead;
        int qMid1 = map(filters[1].memQReading, 0, 1023, 0, 100);
        float travelMid1 = mapFloat(qMid1, 0.0, 100.0, 0.0, 1.0);
        float resistanceFromTravelMid1 = 5000 * travelToResistanceLog(travelMid1);
        int wiperFromTravelMid1 = resistanceToWiper(20000, resistanceFromTravelMid1);
        word qInstructionMid1 = wiperToInstruction(wiperFromTravelMid1);
        
        // MARK: - Memory High-Pass
        makeTextHighPass(String(map(memHighPassReading, 0, 1023, long(26.526), long(2368.4)))).toCharArray(buffer, 20);
        //delay(100);
        Wire.beginTransmission(4);
        Wire.write(0x50);
        Wire.write(buffer);
        Wire.endTransmission(true);
        
        // MARK: - Math Memory High-Pass
        long highPassFreq = map(memHighPassReading, 0, 1023, long(26.526), long(2368.4));
        float highPassResistance = (1 / (2 * PI * highPassFreq * 0.00000012));
        highPassResistance = mapFloat(highPassResistance, 560.09, 51011.2, 560.0, 50000.0);
        int highPassWiper = resistanceToWiper(50000, highPassResistance);
        word highPassInstruction = wiperToInstruction(highPassWiper);
        
        // MARK: - Memory Frequency Low
        String modifiedReplacementText = String(map(filters[0].memFreqReading, 0, 1023, 40, 800));
        switch (modifiedReplacementText.length()) {
            case 2:
                modifiedReplacementText = "0" + modifiedReplacementText;
                break;
                
            default:
                break;
        }
        
        modifiedReplacementText.toCharArray(buffer, 20);
        //delay(100);
        Wire.beginTransmission(4);
        Wire.write(0x00);
        Wire.write(buffer);
        Wire.endTransmission(true);
        
        // MARK: - Memory DB Low
        makeTextDB(String(mapFloat(filters[0].memDBReading, 0.0, 1023.0, -10.0, 10.0))).toCharArray(buffer, 20);
        //delay(100);
        Wire.beginTransmission(4);
        Wire.write(0x01);
        Wire.write(buffer);
        Wire.endTransmission(true);
        
        // MARK: - Memory Q Low
        makeTextQ(String(map(filters[0].memQReading, 0, 1023, 0, 100))).toCharArray(buffer, 20);
        //delay(100);
        Wire.beginTransmission(4);
        Wire.write(0x02);
        Wire.write(buffer);
        Wire.endTransmission(true);
        
        // MARK: - Math Memory Low
        long frequencyLow = map(filters[0].memFreqReading, 0, 1023, 40, 800);
        float frequencyResistanceLow = (1 / (2 * PI * frequencyLow * 0.000000082));
        int frequencyWiperLow = resistanceToWiper(50000, frequencyResistanceLow);
        word frequencyInstructionLow = wiperToInstruction(frequencyWiperLow);
        
        word dbInstructionLow = wiperToInstruction(filters[0].memDBReading);
        
        int qLow = map(filters[0].memQReading, 0, 1023, 0, 100);
        float travelLow = mapFloat(qLow, 0.0, 100.0, 0.0, 1.0);
        float resistanceFromTravelLow = 5000 * travelToResistanceLog(travelLow);
        int wiperFromTravelLow = resistanceToWiper(20000, resistanceFromTravelLow);
        word qInstructionLow = wiperToInstruction(wiperFromTravelLow);
        
        /*Wire.beginTransmission(4);
        Wire.write(0x90);
        Wire.endTransmission(true);*/
        
        digitalWrite(MEM_INDICATOR, HIGH);
        
        // MARK: - Memory SPI
        /*digitalWrite(53, LOW);
        SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE1));
        SPI.transfer16(qInstruction2);
        SPI.transfer16(frequencyInstruction2);
        SPI.transfer16(frequencyInstruction2);
        SPI.transfer16(dbInstruction2);
        SPI.transfer16(qInstruction);
        SPI.transfer16(frequencyInstruction);
        SPI.transfer16(frequencyInstruction);
        SPI.transfer16(dbInstruction);
        digitalWrite(53, HIGH);
        
        digitalWrite(12, LOW);
        SPI.transfer16(qInstructionLow);
        SPI.transfer16(frequencyInstructionLow);
        SPI.transfer16(frequencyInstructionLow);
        SPI.transfer16(dbInstructionLow);
        SPI.transfer16(highPassInstruction);
        SPI.transfer16(highPassInstruction);
        SPI.transfer16(qInstructionMid1);
        SPI.transfer16(frequencyInstructionMid1);
        SPI.transfer16(frequencyInstructionMid1);
        SPI.transfer16(dbInstructionMid1);
        SPI.transfer16(lowPassInstruction);
        SPI.transfer16(lowPassInstruction);
        
        //delay(30);
        digitalWrite(12, HIGH);
        SPI.endTransaction();*/
        
        digitalWrite(53, LOW);
        delay(10);
        
        SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE1));
        SPI.transfer16(qInstructionLow);
        SPI.transfer16(frequencyInstructionLow);
        SPI.transfer16(frequencyInstructionLow);
        SPI.transfer16(dbInstructionLow);
        SPI.transfer16(highPassInstruction);
        delay(10);
        SPI.transfer16(highPassInstruction);
        //SPI.transfer16(0x0000);
        SPI.transfer16(qInstructionMid1);
        SPI.transfer16(frequencyInstructionMid1);
        SPI.transfer16(frequencyInstructionMid1);
        SPI.transfer16(dbInstructionMid1);
        //SPI.transfer16(lowPassInstruction);
        //SPI.transfer16(lowPassInstruction);
        SPI.transfer16(qInstruction2);
        SPI.transfer16(frequencyInstruction2);
        SPI.transfer16(frequencyInstruction2);
        SPI.transfer16(dbInstruction2);
        SPI.transfer16(qInstruction);
        SPI.transfer16(frequencyInstruction);
        SPI.transfer16(frequencyInstruction);
        SPI.transfer16(dbInstruction);
        
        delay(10);
        digitalWrite(53, HIGH);
        SPI.endTransaction();
        
        latches[3].tempState = latches[3].memState;
        latches[3].tempCounter = latches[3].memCounter;
        latches[3].tempLastState = latches[3].memLastState;
        
        latches[4].tempState = latches[4].memState;
        latches[4].tempCounter = latches[4].memCounter;
        latches[4].tempLastState = latches[4].memLastState;
        
        // MARK: - FILTER ON LATCH
        /*latches[3].tempState = digitalRead(SW_FILTER_ON);
        
        if (latches[3].tempState != latches[3].tempLastState) {
            if (latches[3].tempState == HIGH) {
                latches[3].tempCounter++;
            }
            
            delay(50);
        }
        
        latches[3].tempLastState = latches[3].tempState;*/
        
        if (latches[3].tempCounter % 2 == 0) {
            digitalWrite(LED_FILTER_ON, LOW);
        } else {
            digitalWrite(LED_FILTER_ON, HIGH);
        }
        
        // MARK: - BYPASS LATCH
        /*latches[4].tempState = digitalRead(SW_BYPASS);
        
        if (latches[4].tempState != latches[4].tempLastState) {
            if (latches[4].tempState == HIGH) {
                latches[4].tempCounter++;
            }
            
            delay(50);
        }
        
        latches[4].tempLastState = latches[4].tempState;*/
        
        if (latches[4].tempCounter % 2 == 0) {
            digitalWrite(LED_BYPASS, LOW);
        } else {
            digitalWrite(LED_BYPASS, HIGH);
        }
        
        
        while (shouldUseMemory == true) {
            // MARK: - LOW LATCH
            /*latches[0].state = digitalRead(SW_LO_PK);
            
            if (latches[0].state != latches[0].lastState) {
                if (latches[0].state == HIGH) {
                    latches[0].counter++;
                }
                
                delay(50);
            }
            
            latches[0].lastState = latches[0].state;*/
            
            if (latches[0].memCounter % 2 == 0) {
                digitalWrite(LED_LO_PK, HIGH);
            } else {
                digitalWrite(LED_LO_PK, LOW);
            }
            
            // MARK: - MID2 LATCH
            /*latches[1].state = digitalRead(SW_MID2_PK);
            
            if (latches[1].state != latches[1].lastState) {
                if (latches[1].state == HIGH) {
                    latches[1].counter++;
                }
                
                delay(50);
            }
            
            latches[1].lastState = latches[1].state;*/
            
            if (latches[1].memCounter % 2 == 0) {
                digitalWrite(LED_MID2_PK, HIGH);
            } else {
                digitalWrite(LED_MID2_PK, LOW);
            }
            
            // MARK: - HIGH LATCH
            /*latches[2].state = digitalRead(SW_HI_PK);
            
            if (latches[2].state != latches[2].lastState) {
                if (latches[2].state == HIGH) {
                    latches[2].counter++;
                }
                
                delay(50);
            }
            
            latches[2].lastState = latches[2].state;*/
            
            if (latches[2].memCounter % 2 == 0) {
                digitalWrite(LED_HI_PK, HIGH);
            } else {
                digitalWrite(LED_HI_PK, LOW);
            }
            
            // MARK: - FILTER ON LATCH
            latches[3].tempState = digitalRead(SW_FILTER_ON);
            
            if (latches[3].tempState != latches[3].tempLastState) {
                if (latches[3].tempState == HIGH) {
                    latches[3].tempCounter++;
                }
                
                delay(50);
            }
            
            latches[3].tempLastState = latches[3].tempState;
            
            if (latches[3].tempCounter % 2 == 0) {
                digitalWrite(LED_FILTER_ON, LOW);
            } else {
                digitalWrite(LED_FILTER_ON, HIGH);
            }
            
            // MARK: - BYPASS LATCH
            latches[4].tempState = digitalRead(SW_BYPASS);
            
            if (latches[4].tempState != latches[4].tempLastState) {
                if (latches[4].tempState == HIGH) {
                    latches[4].tempCounter++;
                }
                
                delay(50);
            }
            
            latches[4].tempLastState = latches[4].tempState;
            
            if (latches[4].tempCounter % 2 == 0) {
                digitalWrite(LED_BYPASS, LOW);
            } else {
                digitalWrite(LED_BYPASS, HIGH);
            }
            
            Wire.requestFrom(4, 2);
            
            commandByte = Wire.read();
            dataByte = Wire.read();
                
            if (commandByte == 0x10) {
                save((int) dataByte);
            } else if (commandByte == 0x20) {
                load((int) dataByte);
            }
            
            if (commandByte == 0x02) {
                if (dataByte == 0x00) {
                    shouldUseMemory = false;
                    
                    digitalWrite(MEM_INDICATOR, LOW);
                    
                    for (int i = 0; i < 4; i++) {
                        filters[i].lastFreqReading = -1;
                        filters[i].lastDBReading = -1;
                        filters[i].lastQReading = -1;
                    }
                    
                    lastHighPassReading = -1;
                    lastLowPassReading = -1;
                    return;
                }
            }
            
            if (commandByte == 0x95) {
                if (dataByte == 0x00) {
                    //shouldUseMemory = true;
                    return;
                } else {
                    shouldUseMemory = false;
                    for (int i = 0; i < 4; i++) {
                        filters[i].lastFreqReading = -1;
                        filters[i].lastDBReading = -1;
                        filters[i].lastQReading = -1;
                    }
                    
                    lastHighPassReading = -1;
                    lastLowPassReading = -1;
                    
                    return;
                }
            }
        }
        
        //return;
    } else {
        Wire.requestFrom(4, 2, true);
        
        commandByte = Wire.read();
        dataByte = Wire.read();
            
        if (commandByte == 0x10) {
            save((int) dataByte);
            delay(500);
        } else if (commandByte == 0x20) {
            load((int) dataByte);
            delay(500);
        }
        
        if (commandByte == 0x02) {
            if (dataByte == 0x01) {
                shouldUseMemory = true;
                
                /*Wire.beginTransmission(4);
                Wire.write(0x90);
                Wire.endTransmission();*/
                delay(1000);
                return;
            }
        }
        
        if (commandByte == 0x95) {
            if (dataByte == 0x01) {
                for (int i = 0; i < 4; i++) {
                    filters[i].lastFreqReading = -1;
                    filters[i].lastDBReading = -1;
                    filters[i].lastQReading = -1;
                }
                
                lastHighPassReading = -1;
                lastLowPassReading = -1;
            } else {
                shouldUseMemory = true;
                return;
            }
        }
        
        // MARK: - Transition to High Values
        int freqReading = analogRead(A12);
        int dbReading = analogRead(A6);
        int qRead = analogRead(A4);
        
        char buffer[40];
        
        for (int i = 0; i < sizeof(buffer); i++) {
            buffer[i] = 0x00;
        }
    
        // MARK: - didFrequencyChange High
        if (didFrequencyChange(freqReading, filters[3].lastFreqReading)) {
            makeTextFrequency(String(map(freqReading, 0, 1023, 800, 17000))).toCharArray(buffer, 40);
            filters[3].lastFreqReading = freqReading;
            
            long frequency = map(freqReading, 0, 1023, 800, 17000);
            
            float resistanceFromFrequency = freqToResistanceAntiLog(frequency, 0.0000000039);
            
            int wiperFromResistance = resistanceToWiper(50000, resistanceFromFrequency);
            
            word frequencyInstruction = wiperToInstruction(wiperFromResistance);
            
            //TWCR = 0;
            //Wire.begin(1);
            
            Wire.beginTransmission(4);
            Wire.write(0x30);
            Wire.write(buffer);
            Wire.endTransmission(true);
            
            /*digitalWrite(4, LOW);
             softSPI.transfer16(0x0000);
             softSPI.transfer16(0x0000);
             softSPI.transfer16(0x0000);
             softSPI.transfer16(0x0000);
             
             softSPI.transfer16(0x0000);
             softSPI.transfer16(frequencyInstruction);
             softSPI.transfer16(frequencyInstruction);
             softSPI.transfer16(0x0000);
             digitalWrite(4, HIGH);*/
        }
    
        // MARK: - didDBChange High
        if (didDBChange(dbReading, filters[3].lastDBReading)) {
            makeTextDB(String(mapFloat(dbReading, 0.0, 1023.0, -10.0, 10.0))).toCharArray(buffer, 40);
            filters[3].lastDBReading = dbReading;
            
            float db = mapFloat(dbReading, 0.0, 1023.0, -10.0, 10.0);
            word dbInstruction = wiperToInstruction(db);
            
            //TWCR = 0;
            //Wire.begin(1);
            
            Wire.beginTransmission(4);
            Wire.write(0x31);
            Wire.write(buffer);
            Wire.endTransmission(true);
            
            /*digitalWrite(4, LOW);
             softSPI.transfer16(0x0000);
             softSPI.transfer16(0x0000);
             softSPI.transfer16(0x0000);
             softSPI.transfer16(0x0000);
             
             softSPI.transfer16(0x0000);
             softSPI.transfer16(0x0000);
             softSPI.transfer16(0x0000);
             softSPI.transfer16(dbInstruction);
             digitalWrite(4, HIGH);*/
        }
    
        // MARK: - didQChange High
        if (didQChange(qRead, filters[3].lastQReading)) {
            makeTextQ(String(map(qRead, 0, 1023, 0, 100))).toCharArray(buffer, 40);
            filters[3].lastQReading = qRead;
            
            float travel = mapFloat(qRead, 0.0, 100.0, 0.0, 1.0);
            float resistanceFromTravel = 5000 * travelToResistanceLog(travel);
            int wiperFromTravel = resistanceToWiper(20000, resistanceFromTravel);
            word qInstruction = wiperToInstruction(wiperFromTravel);
            
            //TWCR = 0;
            //Wire.begin(1);
            
            Wire.beginTransmission(4);
            Wire.write(0x32);
            Wire.write(buffer);
            Wire.endTransmission(true);
            
            /*digitalWrite(4, LOW);
             softSPI.transfer16(0x0000);
             softSPI.transfer16(0x0000);
             softSPI.transfer16(0x0000);
             softSPI.transfer16(0x0000);
             
             softSPI.transfer16(qInstruction);
             softSPI.transfer16(0x0000);
             softSPI.transfer16(0x0000);
             softSPI.transfer16(0x0000);
             digitalWrite(4, HIGH);*/
        }
    
        // MARK: - Math High
        long frequency = map(freqReading, 0, 1023, 800, 17000);
        float resistanceFromFrequency = freqToResistanceAntiLog(frequency, 0.0000000039);
        int wiperFromResistance = resistanceToWiper(50000, resistanceFromFrequency);
        word frequencyInstruction = wiperToInstruction(wiperFromResistance);
        //int dbWiper = ;
        float db = mapFloat(dbReading, 0.0, 1023.0, -10.0, 10.0);
        word dbInstruction = wiperToInstruction(dbReading);
        //int qReading = analogRead(A4);
        int q = map(qRead, 0, 1023, 0, 100);
        float travel = mapFloat(q, 0.0, 100.0, 0.0, 1.0);
        float resistanceFromTravel = 20000 * travelToResistanceLog(travel);
        int wiperFromTravel = resistanceToWiper(20000, resistanceFromTravel);
        word qInstruction = wiperToInstruction(wiperFromTravel);
        
        // MARK: - Transition to Mid-2 Values
        freqReading = analogRead(A11);
        dbReading = analogRead(A7);
        qRead = analogRead(A3);
    
        // MARK: - didFrequencyChange Mid-2
        if (didFrequencyChange(freqReading, filters[2].lastFreqReading)) {
            makeTextFrequency(String(map(freqReading, 0, 1023, 400, 8000))).toCharArray(buffer, 40);
            filters[2].lastFreqReading = freqReading;
            
            long frequency = map(freqReading, 0, 1023, 400, 8000);
            float resistanceFromFrequency = freqToResistanceAntiLog(frequency, 0.0000000082);
            int wiperFromResistance = resistanceToWiper(50000, resistanceFromFrequency);
            
            word frequencyInstruction = wiperToInstruction(wiperFromResistance);
            
            //TWCR = 0;
            //Wire.begin(1);
            
            Wire.beginTransmission(4);
            Wire.write(0x20);
            Wire.write(buffer);
            Wire.endTransmission(true);
            
            /*digitalWrite(4, LOW);
             softSPI.transfer16(0x0000);
             softSPI.transfer16(frequencyInstruction);
             softSPI.transfer16(frequencyInstruction);
             softSPI.transfer16(0x0000);
             
             softSPI.transfer16(0x0000);
             softSPI.transfer16(0x0000);
             softSPI.transfer16(0x0000);
             softSPI.transfer16(0x0000);
             digitalWrite(4, HIGH);*/
        }
    
        // MARK: - didDBChange Mid-2
        if (didDBChange(dbReading, filters[2].lastDBReading)) {
            makeTextDB(String(mapFloat(dbReading, 0.0, 1023.0, -10.0, 10.0))).toCharArray(buffer, 40);
            filters[2].lastDBReading = dbReading;
            
            float db = mapFloat(dbReading, 0.0, 1023.0, -10.0, 10.0);
            word dbInstruction = wiperToInstruction(db);
            
            Wire.beginTransmission(4);
            Wire.write(0x21);
            Wire.write(buffer);
            Wire.endTransmission(true);
            
            /*digitalWrite(4, LOW);
             softSPI.transfer16(0x0000);
             softSPI.transfer16(0x0000);
             softSPI.transfer16(0x0000);
             softSPI.transfer16(dbInstruction);
             
             softSPI.transfer16(0x0000);
             softSPI.transfer16(0x0000);
             softSPI.transfer16(0x0000);
             softSPI.transfer16(0x0000);
             digitalWrite(4, HIGH);*/
        }
        
        //int dbWiper2 = analogRead(A7);
        //float db2 = mapFloat(dbWiper2, 0.0, 1023.0, -10.0, 10.0);
        //word dbInstruction2 = wiperToInstruction(dbWiper2);
    
        // MARK: - didQChange Mid-2
        if (didQChange(qRead, filters[2].lastQReading)) {
            makeTextQ(String(map(qRead, 0, 1023, 0, 100))).toCharArray(buffer, 40);
            filters[2].lastQReading = qRead;
            
            float travel = mapFloat(qRead, 0.0, 100.0, 0.0, 1.0);
            float resistanceFromTravel = 5000 * travelToResistanceLog(travel);
            int wiperFromTravel = resistanceToWiper(20000, resistanceFromTravel);
            word qInstruction = wiperToInstruction(wiperFromTravel);
            
            //TWCR = 0;
            //Wire.begin(1);
            
            Wire.beginTransmission(4);
            Wire.write(0x22);
            Wire.write(buffer);
            Wire.endTransmission(true);
            
            /*digitalWrite(4, LOW);
             softSPI.transfer16(qInstruction);
             softSPI.transfer16(0x0000);
             softSPI.transfer16(0x0000);
             softSPI.transfer16(0x0000);
             
             softSPI.transfer16(0x0000);
             softSPI.transfer16(0x0000);
             softSPI.transfer16(0x0000);
             softSPI.transfer16(0x0000);
             digitalWrite(4, HIGH);*/
        }
    
        // MARK: - Math Mid-2
        long frequency2 = map(freqReading, 0, 1023, 400, 8000);
        float resistanceFromFrequency2 = freqToResistanceAntiLog(frequency2, 0.0000000082);
        int wiperFromResistance2 = resistanceToWiper(50000, resistanceFromFrequency2);
        word frequencyInstruction2 = wiperToInstruction(wiperFromResistance2);
        
        //int dbWiper2 = dbReading;
        float db2 = mapFloat(dbReading, 0.0, 1023.0, -10.0, 10.0);
        word dbInstruction2 = wiperToInstruction(dbReading);
        
        //int qReading2 = qRead;
        int q2 = map(qRead, 0, 1023, 0, 100);
        float travel2 = mapFloat(q2, 0.0, 100.0, 0.0, 1.0);
        float resistanceFromTravel2 = 5000 * travelToResistanceLog(travel2);
        int wiperFromTravel2 = resistanceToWiper(20000, resistanceFromTravel2);
        word qInstruction2 = wiperToInstruction(wiperFromTravel2);
        
        
        // MARK: - Transition to Low-Pass Vals
        freqReading = analogRead(A10);
        
        // MARK: - didFrequencyChange Low-Pass
        if (didFrequencyChange(freqReading, lastLowPassReading)) {
            makeTextLowPass(String(map(freqReading, 0, 1023, long(96.46), 14615))).toCharArray(buffer, 40);
            lastLowPassReading = freqReading;
            
            //TWCR = 0;
            //Wire.begin(1);
            
            Wire.beginTransmission(4);
            Wire.write(0x40);
            Wire.write(buffer);
            Wire.endTransmission(true);
        }
        
        // MARK: - Math Low-Pass
        //float lowPassResistance = freqToResistanceAntiLog(freqReading, 0.00000000033);
        long lowPassFreq = map(freqReading, 0, 1023, long(96.46), long(14615));
        float lowPassResistance = (1 / (2 * PI * lowPassFreq * 0.000000033));
        lowPassResistance = mapFloat(lowPassResistance, 330.0, 50238.30, 330.0, 50000.0);
        int lowPassWiper = 1023 - resistanceToWiper(50000, lowPassResistance);
        word lowPassInstruction = wiperToInstruction(lowPassWiper);
        //Serial.print("LOW PASS WIPER: ");
        //Serial.println(lowPassWiper);
        
        // MARK: - Transition to Mid-1 Values
        freqReading = analogRead(A14);
        dbReading = analogRead(A8);
        qRead = analogRead(A2);
    
        // MARK: - didFrequencyChange Mid-1
        if (didFrequencyChange(freqReading, filters[1].lastFreqReading)) {
            makeTextFrequency(String(map(freqReading, 0, 1023, 400, 8000))).toCharArray(buffer, 40);
            filters[1].lastFreqReading = freqReading;
            
            long frequency = map(freqReading, 0, 1023, 400, 8000);
            float resistanceFromFrequency = freqToResistanceAntiLog(frequency, 0.0000000082);
            int wiperFromResistance = resistanceToWiper(50000, resistanceFromFrequency);
            
            word frequencyInstruction = wiperToInstruction(wiperFromResistance);
            
            //TWCR = 0;
            //Wire.begin(1);
            
            Wire.beginTransmission(4);
            Wire.write(0x10);
            Wire.write(buffer);
            Wire.endTransmission(true);
        }
    
        // MARK: - didDBChange Mid-1
        if (didDBChange(dbReading, filters[1].lastDBReading)) {
            makeTextDB(String(mapFloat(dbReading, 0.0, 1023.0, -10.0, 10.0))).toCharArray(buffer, 40);
            filters[1].lastDBReading = dbReading;
            
            Wire.beginTransmission(4);
            Wire.write(0x11);
            Wire.write(buffer);
            Wire.endTransmission(true);
        }
    
        // MARK: - didQChange Mid-1
        if (didQChange(qRead, filters[1].lastQReading)) {
            makeTextQ(String(map(qRead, 0, 1023, 0, 100))).toCharArray(buffer, 40);
            filters[1].lastQReading = qRead;
            
            //TWCR = 0;
            //Wire.begin(1);
            
            Wire.beginTransmission(4);
            Wire.write(0x12);
            Wire.write(buffer);
            Wire.endTransmission(true);
        }
    
        // MARK: - Math Mid-1
        long frequencyMid1 = map(freqReading, 0, 1023, 400, 8000);
        float resistanceFromFrequencyMid1 = freqToResistanceAntiLog(frequencyMid1, 0.0000000082);
        int wiperFromResistanceMid1 = resistanceToWiper(50000, resistanceFromFrequencyMid1);
        word frequencyInstructionMid1 = wiperToInstruction(wiperFromResistanceMid1);
        
        //int dbWiper2 = dbReading;
        float dbMid1 = mapFloat(dbReading, 0.0, 1023.0, -10.0, 10.0);
        word dbInstructionMid1 = wiperToInstruction(dbReading);
        
        //int qReading2 = qRead;
        int qMid1 = map(qRead, 0, 1023, 0, 100);
        float travelMid1 = mapFloat(qMid1, 0.0, 100.0, 0.0, 1.0);
        float resistanceFromTravelMid1 = 5000 * travelToResistanceLog(travelMid1);
        int wiperFromTravelMid1 = resistanceToWiper(20000, resistanceFromTravelMid1);
        word qInstructionMid1 = wiperToInstruction(wiperFromTravelMid1);
    
        // MARK: - Transition to High-Pass Vals
        freqReading = analogRead(A15);
    
        // MARK: - didFrequencyChange High-Pass
        if (didFrequencyChange(freqReading, lastHighPassReading)) {
            makeTextHighPass(String(map(freqReading, 0, 1023, long(26.526), long(2368.4)))).toCharArray(buffer, 40);
            
            lastHighPassReading = freqReading;
            
            //makeTextHighPass(String(map(freqReading, 0, 1023, 50, 2000))).toCharArray(buffer, 40);
            
            //TWCR = 0;
            //Wire.begin(1);
            
            Wire.beginTransmission(4);
            Wire.write(0x50);
            Wire.write(buffer);
            Wire.endTransmission(true);
        }
    
        // MARK: - Math High-Pass
        long highPassFreq = map(freqReading, 0, 1023, long(26.526), long(2368.4));
        float highPassResistance = (1 / (2 * PI * highPassFreq * 0.00000012));
        highPassResistance = mapFloat(highPassResistance, 560.09, 51011.2, 560.0, 50000.0);
        int highPassWiper = resistanceToWiper(50000, highPassResistance);
        word highPassInstruction = wiperToInstruction(highPassWiper);
    
        // MARK: - Transition to Low Values
        freqReading = analogRead(A13);
        dbReading = analogRead(A9);
        qRead = analogRead(A5);
    
        // MARK: - didFrequencyChange Low
        if (didFrequencyChange(freqReading, filters[0].lastFreqReading)) {
            //makeTextFrequency(String(map(freqReading, 0, 1023, 40, 800))).toCharArray(buffer, 40);
            
            String modifiedReplacementText = String(map(freqReading, 0, 1023, 40, 800));
            switch (modifiedReplacementText.length()) {
                case 2:
                    modifiedReplacementText = "0" + modifiedReplacementText;
                    break;
                    
                default:
                    break;
            }
            
            modifiedReplacementText.toCharArray(buffer, 40);
            
            filters[0].lastFreqReading = freqReading;
            
            //TWCR = 0;
            //Wire.begin(1);
            
            Wire.beginTransmission(4);
            Wire.write(0x00);
            Wire.write(buffer);
            Wire.endTransmission(true);
        }
    
        // MARK: - didDBChange Low
        if (didDBChange(dbReading, filters[0].lastDBReading)) {
            makeTextDB(String(mapFloat(dbReading, 0.0, 1023.0, -10.0, 10.0))).toCharArray(buffer, 40);
            filters[0].lastDBReading = dbReading;
            
            //TWCR = 0;
            //Wire.begin(1);
            
            Wire.beginTransmission(4);
            Wire.write(0x01);
            Wire.write(buffer);
            Wire.endTransmission(true);
        }
    
        // MARK: - didQChange Low
        if (didQChange(qRead, filters[0].lastQReading)) {
            makeTextQ(String(map(qRead, 0, 1023, 0, 100))).toCharArray(buffer, 40);
            filters[0].lastQReading = qRead;
        
            Wire.beginTransmission(4);
            Wire.write(0x02);
            Wire.write(buffer);
            Wire.endTransmission(true);
        }
    
        // MARK: - Math Low
        long frequencyLow = map(freqReading, 0, 1023, 40, 800);
        float frequencyResistanceLow = (1 / (2 * PI * frequencyLow * 0.000000082));
        int frequencyWiperLow = resistanceToWiper(50000, frequencyResistanceLow);
        word frequencyInstructionLow = wiperToInstruction(frequencyWiperLow);
    
        word dbInstructionLow = wiperToInstruction(dbReading);
    
        int qLow = map(qRead, 0, 1023, 0, 100);
        float travelLow = mapFloat(qLow, 0.0, 100.0, 0.0, 1.0);
        float resistanceFromTravelLow = 5000 * travelToResistanceLog(travelLow);
        int wiperFromTravelLow = resistanceToWiper(20000, resistanceFromTravelLow);
        word qInstructionLow = wiperToInstruction(wiperFromTravelLow);
        
        word instructionBank[] = {
            qInstructionLow,
            frequencyInstructionLow,
            frequencyInstructionLow,
            dbInstructionLow,
            
            highPassInstruction,
            highPassInstruction,
            
            qInstructionMid1,
            frequencyInstructionMid1,
            frequencyInstructionMid1,
            dbInstructionMid1,
            
            lowPassInstruction,
            lowPassInstruction,
            
            qInstruction2,
            frequencyInstruction2,
            frequencyInstruction2,
            dbInstruction2,
            
            qInstruction,
            frequencyInstruction,
            frequencyInstruction,
            dbInstruction
        };
        // MARK: - SPI
        /*digitalWrite(53, LOW);
        SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE1));
        SPI.transfer16(qInstruction2);
        SPI.transfer16(frequencyInstruction2);
        SPI.transfer16(frequencyInstruction2);
        SPI.transfer16(dbInstruction2);
        SPI.transfer16(qInstruction);
        SPI.transfer16(frequencyInstruction);
        SPI.transfer16(frequencyInstruction);
        SPI.transfer16(dbInstruction);
        digitalWrite(53, HIGH);
        SPI.endTransaction();
        
        digitalWrite(12, LOW);
        SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE1));
        SPI.transfer16(qInstructionLow);
        SPI.transfer16(frequencyInstructionLow);
        SPI.transfer16(frequencyInstructionLow);
        SPI.transfer16(dbInstructionLow);
        SPI.transfer16(highPassInstruction);
        SPI.transfer16(highPassInstruction);
        SPI.transfer16(qInstructionMid1);
        SPI.transfer16(frequencyInstructionMid1);
        SPI.transfer16(frequencyInstructionMid1);
        SPI.transfer16(dbInstructionMid1);
        SPI.transfer16(lowPassInstruction);    
        SPI.transfer16(lowPassInstruction);
        SPI.endTransaction();
        digitalWrite(12, HIGH);*/
        
        digitalWrite(53, LOW);
        delay(10);
        
        SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE1));
        SPI.transfer16(qInstructionLow);
        SPI.transfer16(frequencyInstructionLow);
        SPI.transfer16(frequencyInstructionLow);
        SPI.transfer16(dbInstructionLow);
        SPI.transfer16(highPassInstruction);
        delay(10);
        SPI.transfer16(highPassInstruction);
        //SPI.transfer16(0x0000);
        SPI.transfer16(qInstructionMid1);
        SPI.transfer16(frequencyInstructionMid1);
        SPI.transfer16(frequencyInstructionMid1);
        SPI.transfer16(dbInstructionMid1);
        //SPI.transfer16(lowPassInstruction);
        //SPI.transfer16(lowPassInstruction);
        SPI.transfer16(qInstruction2);
        SPI.transfer16(frequencyInstruction2);
        SPI.transfer16(frequencyInstruction2);
        SPI.transfer16(dbInstruction2);
        SPI.transfer16(qInstruction);
        SPI.transfer16(frequencyInstruction);
        SPI.transfer16(frequencyInstruction);
        SPI.transfer16(dbInstruction);
        
        delay(10);
        digitalWrite(53, HIGH);
        SPI.endTransaction();
    }
}
