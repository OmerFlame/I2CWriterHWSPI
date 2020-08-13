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
    EndHighPassFilterDeclaration
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

byte commandByte = 0x00;
byte dataByte = 0x00;

int setCounter = 0;
int setState = 0;
int lastSetState = 0;

bool shouldUseMemory = false;
bool shouldShowMenu = false;

int lowPickCounter = 0;
int lowPickState = 0;
int lastLowPickState = 0;

int mid2Counter = 0;
int mid2State = 0;
int lastMid2PickState = 0;

//2068

int highPickCounter = 0;
int highPickState = 0;
int lastHighPickState = 0;

int filterOnCounter = 0;
int filterOnState = 0;
int lastFilterOnState = 0;

int bypassCounter = 0;
int bypassState = 0;
int lastBypassState = 0;

Filter lowBand;
Filter mid1Band;
Filter mid2Band;
Filter highBand;

Filter filters[] = { lowBand, mid1Band, mid2Band, highBand };

int lastFreqReadingHigh;
int lastDBReadingHigh;
int lastQReadingHigh;

int lastFreqReadingMid2;
int lastDBReadingMid2;
int lastQReadingMid2;

int lastLowPassReading;
int memLowPassReading;

int lastFreqReadingMid1;
int lastDBReadingMid1;
int lastQReadingMid1;

int lastHighPassReading;
int memHighPassReading;

int lastFreqReadingLow;
int lastDBReadingLow;
int lastQReadingLow;

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
        
        fileToRW.print("ENDFIN ");
        fileToRW.close();

        Serial.println("SUCCESS WRITING TO FILE!");
    } else {
        Serial.println("Unable to open the file");
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
            lastKeyword = extractedKeywords[i];
            lastKeywordDescription = FilterIndex;
            continue;
        }

        if (lastKeywordDescription == FilterIndex) {
            frequencyData = strtol(extractedKeywords[i].c_str(), NULL, 10);
            Serial.println(frequencyData);
            lastKeyword = extractedKeywords[i];
            lastKeywordDescription = FrequencyValue;
            continue;
        }

        if ((extractedKeywords[i].indexOf(".") > 0) && (lastKeywordDescription == FrequencyValue)) {
            dbData = extractedKeywords[i].toInt();
            Serial.println(dbData);
            lastKeyword = extractedKeywords[i];
            lastKeywordDescription = DBValue;
            continue;
        }

        if ((lastKeyword.indexOf(".") > 0) && (lastKeywordDescription == DBValue)) {
            qData = strtol(extractedKeywords[i].c_str(), NULL, 10);
            Serial.println(extractedKeywords[i].c_str());
            //Serial.println(int(test));
            lastKeyword = extractedKeywords[i];
            lastKeywordDescription = QValue;
            continue;
        }

        if ((lastKeywordDescription == QValue) && (extractedKeywords[i] == "END")) {
            lastKeyword = extractedKeywords[i];
            lastKeywordDescription = EndFilterDeclaration;

            filters[currentFilter].memFreqReading = frequencyData;
            filters[currentFilter].memDBReading = dbData;
            filters[currentFilter].memQReading = qData;
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

    //*_parentMemoryBool = true;
}

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
    pinMode(4, OUTPUT);
    pinMode(23, OUTPUT);
    SPI.begin();
    //SPI.setClockDivider(SPI_CLOCK_DIV2);
    //SPI.setBitOrder(MSBFIRST);
    //SPI.setDataMode(SPI_MODE1);
    digitalWrite(MENU_OUT, HIGH);
    digitalWrite(SET_OUT, HIGH);
    digitalWrite(UP_OUT, HIGH);
    digitalWrite(DOWN_OUT, HIGH);
    
    Serial.begin(9600);
    
    delay(3000);
    
    digitalWrite(53, LOW);
    SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE1));
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
    SPI.transfer16(0x1802);
    SPI.transfer16(0x1802);
    SPI.transfer16(0x1802);
    SPI.transfer16(0x1802);
    SPI.transfer16(0x1802);
    SPI.transfer16(0x1802);
    SPI.transfer16(0x1802);
    SPI.transfer16(0x1802);
    SPI.endTransaction();
    digitalWrite(53, HIGH);
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
    lowPickState = digitalRead(SW_LO_PK);
    
    if (lowPickState != lastLowPickState) {
        if (lowPickState == HIGH) {
            lowPickCounter++;
        }
        
        delay(50);
    }
    
    lastLowPickState = lowPickState;
    
    if (lowPickCounter % 2 == 0) {
        digitalWrite(LED_LO_PK, HIGH);
    } else {
        digitalWrite(LED_LO_PK, LOW);
    }
    
    // MARK: - MID2 LATCH
    mid2State = digitalRead(SW_MID2_PK);
    
    if (mid2State != lastMid2PickState) {
        if (mid2State == HIGH) {
            mid2Counter++;
        }
        
        delay(50);
    }
    
    lastMid2PickState = mid2State;
    
    if (mid2Counter % 2 == 0) {
        digitalWrite(LED_MID2_PK, HIGH);
    } else {
        digitalWrite(LED_MID2_PK, LOW);
    }
    
    // MARK: - HIGH LATCH
    highPickState = digitalRead(SW_HI_PK);
    
    if (highPickState != lastHighPickState) {
        if (highPickState == HIGH) {
            highPickCounter++;
        }
        
        delay(50);
    }
    
    lastHighPickState = highPickState;
    
    if (highPickCounter % 2 == 0) {
        digitalWrite(LED_HI_PK, HIGH);
    } else {
        digitalWrite(LED_HI_PK, LOW);
    }
    
    // MARK: - FILTER ON LATCH
    filterOnState = digitalRead(SW_FILTER_ON);
    
    if (filterOnState != lastFilterOnState) {
        if (filterOnState == HIGH) {
            filterOnCounter++;
        }
        
        delay(50);
    }
    
    lastFilterOnState = filterOnState;
    
    if (filterOnCounter % 2 == 0) {
        digitalWrite(LED_FILTER_ON, LOW);
    } else {
        digitalWrite(LED_FILTER_ON, HIGH);
    }
    
    // MARK: - BYPASS LATCH
    bypassState = digitalRead(SW_BYPASS);
    
    if (bypassState != lastBypassState) {
        if (bypassState == HIGH) {
            bypassCounter++;
        }
        
        delay(50);
    }
    
    lastBypassState = bypassState;
    
    if (bypassCounter % 2 == 0) {
        digitalWrite(LED_BYPASS, LOW);
    } else {
        digitalWrite(LED_BYPASS, HIGH);
    }
    
    if (shouldUseMemory) {
        char buffer[40];
        
        for (int i = 0; i < sizeof(buffer); i++) {
            buffer[i] == 0x00;
        }
        
        // MARK: - Memory Frequency High
        makeTextFrequency(String(map(filters[3].memFreqReading, 0, 1023, 800, 17000))).toCharArray(buffer, 40);
        Wire.beginTransmission(4);
        Wire.write(0x30);
        Wire.write(buffer);
        Wire.endTransmission();
        
        // MARK: - Memory DB High
        makeTextDB(String(mapFloat(filters[3].memDBReading, 0.0, 1023.0, -10.0, 10.0))).toCharArray(buffer, 40);
        
        Serial.print("BUFFER CONTENTS: ");
        Serial.println(buffer);
        
        /*String modifiedReplacementText2 = String(mapFloat(filters[3].memDBReading, 0.0, 1023.0, -10.0, 10.0));
        
        Serial.print("MEMORY DB: ");
        Serial.println(modifiedReplacementText2);
        //_displayHandler.setPrintPos(dbCursorX, dbCursorY);
        //_displayHandler.setColor(0, _textColorR, _textColorG, _textColorB);
        
        modifiedReplacementText2.remove(modifiedReplacementText2.length() - 1);
        
        Serial.print("TRIMMED DB: ");
        Serial.println(modifiedReplacementText2);
        
        if (atof(modifiedReplacementText2.c_str()) > 0) {
            modifiedReplacementText2 = "+" + modifiedReplacementText2;
        }
        
        Serial.print("DB AFTER +: ");
        Serial.println(modifiedReplacementText2);
        
        if (modifiedReplacementText2 == "+10.0") {
            modifiedReplacementText2 = "+10";
        } else if (modifiedReplacementText2 == "-10.0") {
            modifiedReplacementText2 = "-10";
        } else if (modifiedReplacementText2 == "+0.0" || modifiedReplacementText2 == "-0.0") {
            modifiedReplacementText2 = "+0.0";
        }
        
        if (modifiedReplacementText2.length() == 1) {
            //_displayHandler.print("   ");
            modifiedReplacementText2 += "   ";
        } else if (modifiedReplacementText2.length() == 2) {
            //_displayHandler.print("  ");
            modifiedReplacementText2 += "  ";
        } else if (modifiedReplacementText2.length() == 3) {
            //_displayHandler.print(" ");
            modifiedReplacementText2 += " ";
        }
        
        Serial.print("FINAL DB: ");
        Serial.println(modifiedReplacementText2);
        
        modifiedReplacementText2.toCharArray(buffer, 40);*/
        Wire.beginTransmission(4);
        Wire.write(0x31);
        Wire.write(buffer);
        Wire.endTransmission();
        
        // MARK: - Memory Q High
        makeTextQ(String(map(filters[3].memQReading, 0, 1023, 0, 100))).toCharArray(buffer, 40);
        
        Wire.beginTransmission(4);
        Wire.write(0x31);
        Wire.write(buffer);
        Wire.endTransmission();
        
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
        makeTextFrequency(String(map(filters[2].memFreqReading, 0, 1023, 400, 8000))).toCharArray(buffer, 40);
        Wire.beginTransmission(4);
        Wire.write(0x20);
        Wire.write(buffer);
        Wire.endTransmission();
        
        // MARK: - Memory DB Mid-2
        makeTextDB(String(mapFloat(filters[2].memDBReading, 0.0, 1023.0, -10.0, 10.0))).toCharArray(buffer, 40);
        Wire.beginTransmission(4);
        Wire.write(0x21);
        Wire.write(buffer);
        Wire.endTransmission();
        
        // MARK: - Memory Q Mid-2
        makeTextQ(String(map(filters[2].memQReading, 0, 1023, 0, 100))).toCharArray(buffer, 40);
        Wire.beginTransmission(0x22);
        Wire.write(buffer);
        Wire.endTransmission();
        
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
        makeTextLowPass(String(map(memLowPassReading, 0, 1023, long(96.46), 14615))).toCharArray(buffer, 40);
        Wire.beginTransmission(4);
        Wire.write(0x40);
        Wire.write(buffer);
        Wire.endTransmission();
        
        // MARK: - Math Memory Low-Pass
        long lowPassFreq = map(memLowPassReading, 0, 1023, long(96.46), long(14615));
        float lowPassResistance = (1 / (2 * PI * lowPassFreq * 0.000000033));
        lowPassResistance = mapFloat(lowPassResistance, 330.0, 50238.30, 330.0, 50000.0);
        int lowPassWiper = 1023 - resistanceToWiper(50000, lowPassResistance);
        word lowPassInstruction = wiperToInstruction(lowPassWiper);
        
        // MARK: - Memory Frequency Mid-1
        makeTextFrequency(String(map(filters[1].memFreqReading, 0, 1023, 400, 8000))).toCharArray(buffer, 40);
        Wire.beginTransmission(4);
        Wire.write(0x10);
        Wire.write(buffer);
        Wire.endTransmission();
        
        // MARK: - Memory DB Mid-1
        makeTextDB(String(mapFloat(filters[1].memDBReading, 0.0, 1023.0, -10.0, 10.0))).toCharArray(buffer, 40);
        Wire.beginTransmission(4);
        Wire.write(0x11);
        Wire.write(buffer);
        Wire.endTransmission();
        
        // MARK: - Memory Q Mid-1
        makeTextQ(String(map(filters[1].memQReading, 0, 1023, 0, 100))).toCharArray(buffer, 40);
        Wire.beginTransmission(4);
        Wire.write(0x12);
        Wire.write(buffer);
        Wire.endTransmission();
        
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
        makeTextHighPass(String(map(memHighPassReading, 0, 1023, long(26.526), long(2368.4)))).toCharArray(buffer, 40);
        Wire.beginTransmission(4);
        Wire.write(0x50);
        Wire.write(buffer);
        Wire.endTransmission();
        
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
        
        modifiedReplacementText.toCharArray(buffer, 40);
        
        Wire.beginTransmission(4);
        Wire.write(0x00);
        Wire.write(buffer);
        Wire.endTransmission();
        
        // MARK: - Memory DB Low
        makeTextDB(String(mapFloat(filters[0].memDBReading, 0.0, 1023.0, -10.0, 10.0))).toCharArray(buffer, 40);
        
        Wire.beginTransmission(4);
        Wire.write(0x01);
        Wire.write(buffer);
        Wire.endTransmission();
        
        // MARK: - Memory Q Low
        makeTextQ(String(map(filters[0].memQReading, 0, 1023, 0, 100))).toCharArray(buffer, 40);
        
        Wire.beginTransmission(4);
        Wire.write(0x02);
        Wire.write(buffer);
        Wire.endTransmission();
        
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
        
        // MARK: - Memory SPI
        digitalWrite(53, LOW);
        SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE1));
        SPI.transfer16(qInstructionLow);
        delay(1);
        SPI.transfer16(frequencyInstructionLow);
        delay(1);
        SPI.transfer16(frequencyInstructionLow);
        delay(1);
        SPI.transfer16(dbInstructionLow);
        delay(1);
        SPI.transfer16(highPassInstruction);
        delay(1);
        SPI.transfer16(highPassInstruction);
        delay(1);
        SPI.transfer16(qInstructionMid1);
        delay(1);
        SPI.transfer16(frequencyInstructionMid1);
        delay(1);
        SPI.transfer16(frequencyInstructionMid1);
        delay(1);
        SPI.transfer16(dbInstructionMid1);
        delay(1);
        SPI.transfer16(lowPassInstruction);
        delay(1);
        SPI.transfer16(lowPassInstruction);
        delay(1);
        SPI.transfer16(qInstruction2);
        delay(1);
        SPI.transfer16(frequencyInstruction2);
        delay(1);
        SPI.transfer16(frequencyInstruction2);
        delay(1);
        SPI.transfer16(dbInstruction2);
        delay(1);
        SPI.transfer16(qInstruction);
        delay(1);
        SPI.transfer16(frequencyInstruction);
        delay(1);
        SPI.transfer16(frequencyInstruction);
        delay(1);
        SPI.transfer16(dbInstruction);
        delay(1);
        SPI.endTransaction();
        digitalWrite(53, HIGH);
        
        while (shouldUseMemory == true) {
            Wire.requestFrom(4, 2);
            
            commandByte = Wire.read();
            dataByte = Wire.read();
                
            if (commandByte == 0x10) {
                save((int) dataByte);
            } else if (commandByte == 0x20) {
                load((int) dataByte);
            }
            
            if (commandByte == 0x02) {
                if (dataByte == 0x01) {
                    shouldUseMemory = true;
                    
                    Wire.beginTransmission(4);
                    Wire.write(0x90);
                    Wire.endTransmission(true);
                } else {
                    shouldUseMemory = false;
                    
                    Wire.beginTransmission(4);
                    Wire.write(0x80);
                    Wire.endTransmission(true);
                    
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
                
                Wire.beginTransmission(4);
                Wire.write(0x90);
                Wire.endTransmission(true);
                
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
        // MARK: - SPI
        digitalWrite(53, LOW);
        SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE1));
        SPI.transfer16(qInstructionLow);
        delay(1);
        SPI.transfer16(frequencyInstructionLow);
        delay(1);
        SPI.transfer16(frequencyInstructionLow);
        delay(1);
        SPI.transfer16(dbInstructionLow);
        delay(1);
        SPI.transfer16(highPassInstruction);
        delay(1);
        SPI.transfer16(highPassInstruction);
        delay(1);
        SPI.transfer16(qInstructionMid1);
        delay(1);
        SPI.transfer16(frequencyInstructionMid1);
        delay(1);
        SPI.transfer16(frequencyInstructionMid1);
        delay(1);
        SPI.transfer16(dbInstructionMid1);
        delay(1);
        SPI.transfer16(lowPassInstruction);
        delay(1);
        SPI.transfer16(lowPassInstruction);
        delay(1);
        SPI.transfer16(qInstruction2);
        delay(1);
        SPI.transfer16(frequencyInstruction2);
        delay(1);
        SPI.transfer16(frequencyInstruction2);
        delay(1);
        SPI.transfer16(dbInstruction2);
        delay(1);
        SPI.transfer16(qInstruction);
        delay(1);
        SPI.transfer16(frequencyInstruction);
        delay(1);
        SPI.transfer16(frequencyInstruction);
        delay(1);
        SPI.transfer16(dbInstruction);
        delay(1);
        SPI.endTransaction();
        digitalWrite(53, HIGH);
    }
}
