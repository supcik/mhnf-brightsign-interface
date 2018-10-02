// Copyright 2018 Jacques Supcik
// Haute école d'ingénierie et d'architecture de Fribourg
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <Arduino.h>
#include <EEPROM.h>

#define MAX_COMMAND_LEN 256
#define NUMBER_OF_PINS 54
#define SERIAL_SPEED 115200
#define SCREEN_WIDTH 78
#define PORT_PER_LINE (SCREEN_WIDTH / 7)
#define DEBOUNCE 50

const char *version = "1.1";
int8_t pin_mode[NUMBER_OF_PINS];
int8_t saved_state[NUMBER_OF_PINS];
uint32_t last_change[NUMBER_OF_PINS];

void initSavedState() {
    for (int i = 0; i < NUMBER_OF_PINS; i++) {
        if ((pin_mode[i] == INPUT) || (pin_mode[i] == INPUT_PULLUP)) {
            saved_state[i] = digitalRead(i);
        } else {
            saved_state[i] = -1;
        }
        last_change[i] = 0;
    }
}

void printPortInfo(int pinNo) {
    Serial.print(pinNo / 10);
    Serial.print(pinNo % 10);
    Serial.print(':');
    switch (pin_mode[pinNo]) {
    case OUTPUT:
        Serial.print('>');
        Serial.print(digitalRead(pinNo) == HIGH ? '1' : '0');
        break;
    case INPUT:
        Serial.print('<');
        Serial.print(digitalRead(pinNo) == HIGH ? '1' : '0');
        break;
    case INPUT_PULLUP:
        Serial.print('^');
        Serial.print(digitalRead(pinNo) == HIGH ? '1' : '0');
        break;
    case -1:
        Serial.print("XX");
        break;
    default:
        Serial.print("??");
    }
}

void applyConfig() {
    for (int i = 2; i < NUMBER_OF_PINS; i++) {
        switch (pin_mode[i]) {
        case OUTPUT:
            digitalWrite(i, LOW);
            pinMode(i, OUTPUT);
            break;
        case INPUT:
            pinMode(i, INPUT);
            break;
        case INPUT_PULLUP:
            pinMode(i, INPUT_PULLUP);
            break;
        }
    }
}

void readConfig() {
    for (int i = 0; i < (int)EEPROM.length() && (i < NUMBER_OF_PINS); i++) {
        uint8_t mode = EEPROM.read(i);
        // Fix invalid values
        if (mode != OUTPUT && mode != INPUT && mode != INPUT_PULLUP) {
            mode = -1;
        }
        pin_mode[i] = mode;
    }
}

void writeConfig() {
    for (int i = 0; i < (int)EEPROM.length() && (i < NUMBER_OF_PINS); i++) {
        EEPROM.update(i, pin_mode[i]);
    }
}

void factoryReset() {
    Serial.println("FACTORY RESET...");
    // Reserve all pins
    for (int i = 0; i < NUMBER_OF_PINS; i++) {
        pin_mode[i] = -1;
    }
    // Set all pins (besides 0,1 and 13) as INPUT_PULLUPS
    for (int i = 2; i < NUMBER_OF_PINS; i++) {
        if (i == LED_BUILTIN) continue; 
        pin_mode[i] = INPUT_PULLUP;
    }
    // Set builtin LED as output
    pin_mode[LED_BUILTIN] = OUTPUT;

    writeConfig();
    applyConfig();
    Serial.println("Done.");
}

void configurePin(char* command) {
    char* i = command+1;
    while (strlen(i) > 0) {
        if (strlen(i) < 3) {
            Serial.println("Invalid command");
            Serial.print("Command : ");
            Serial.println(command);
            return;
        }
        int pinNo = (i[0] - '0') * 10 + (i[1] - '0');
        if (pinNo < 2 || pinNo > NUMBER_OF_PINS) {
            Serial.print("Invalid pin number (");
            Serial.print(pinNo);
            Serial.println(")");
            Serial.print("Command : ");
            Serial.println(command);
            return;
        }
        switch (i[2]) {
        case 'x':
        case 'X':
            pin_mode[pinNo] = -1;
            break;
        case 'i':
        case '<':
            pin_mode[pinNo] = INPUT;
            break;
        case 'o':
        case '>':
            pin_mode[pinNo] = OUTPUT;
            break;
        case 'p':
        case 'u':
        case '^':
            pin_mode[pinNo] = INPUT_PULLUP;
            break;
        default:
            Serial.print("Invalid mode (");
            Serial.print(i[2]);
            Serial.println(")");
            Serial.print("Command : ");
            Serial.println(command);
            return;
        }
        i += 3;
        applyConfig();
        printPortInfo(pinNo);
        Serial.println();
    }
    writeConfig();
}

void writePin(char* command) {
    char* i = command+1;
    while (strlen(i) > 0) {
        if (strlen(i) < 3) {
            Serial.println("Invalid command");
            Serial.print("Command : ");
            Serial.println(command);
            return;
        }
        int pinNo = (i[0] - '0') * 10 + (i[1] - '0');
        if (pinNo < 0 || pinNo > NUMBER_OF_PINS) {
            Serial.print("Invalid pin number (");
            Serial.print(pinNo);
            Serial.println(")");
            Serial.print("Command : ");
            Serial.println(command);
            return;
        }
        if (pin_mode[pinNo] != OUTPUT) {
            Serial.print("Pin is not configured as OUTPUT : ");
            printPortInfo(pinNo);
            Serial.println();
            Serial.print("Command : ");
            Serial.println(command);
            return;
        }
        switch (i[2]) {
        case '0':
            digitalWrite(pinNo, LOW);
            break;
        case '1':
            digitalWrite(pinNo, HIGH);
            break;
        default:
            Serial.print("Invalid value (");
            Serial.print(i[2]);
            Serial.println(")");
            Serial.print("Command : ");
            Serial.println(command);
            return;
        }
        i += 3;
    }
}

void printInfo() {
    for (int i = 0; i < SCREEN_WIDTH; i++) {
        Serial.print('-');
    }
    Serial.println();
    Serial.print("MHNF BrightSign Interface / Version ");
    Serial.println(version);
    Serial.println("Copyright (c) 2018 Jacques Supcik <jacques.supcik@hefr.ch>");
    Serial.println("Haute ecole d'ingenierie et d'architecture de Fribourg");
    for (int i = 0; i < SCREEN_WIDTH; i++) {
        Serial.print('-');
    }
    Serial.println();
    for (int i = 0; i < NUMBER_OF_PINS; i++) {
        if (i > 0 && i % PORT_PER_LINE == 0)
            Serial.println();
        if (i % PORT_PER_LINE != 0)
            Serial.print(", ");
        printPortInfo(i);
    }
    Serial.println();
    for (int i = 0; i < SCREEN_WIDTH; i++) {
        Serial.print('-');
    }
    Serial.println();
}

void processCommand(char *command) {
    if (strcmp(command, "i") == 0 || strcmp(command, "p") == 0 ) {
        printInfo();
    } else if (strcmp(command, "r!") == 0) {
        factoryReset();
    } else if (strncmp(command, "x", 1) == 0) {
        configurePin(command);
    } else if (strncmp(command, "d", 1) == 0) {
        writePin(command);
    }

}

void serialEvent() {
    static char inputString[MAX_COMMAND_LEN];
    static int pos = 0;

    while (Serial.available()) {
        char inChar = (char)Serial.read();
        if (inChar == '\n' || inChar == '\r') {
            if (pos > 0) {
                inputString[pos] = '\0';
                processCommand(inputString);
                pos = 0;
            }
        } else if (pos < MAX_COMMAND_LEN - 1) {
            inputString[pos++] = inChar;
        }
    }
}

void checkPort(int pin) {
    unsigned long now = millis();
    if ((pin_mode[pin] == INPUT) || (pin_mode[pin] == INPUT_PULLUP)) {
        int s = digitalRead(pin);
        if ((s == LOW) && saved_state[pin] == HIGH) { // HIGH -> LOW
            if ((now - last_change[pin]) > DEBOUNCE) {
                Serial.print('[');
                Serial.print(pin / 10);
                Serial.print(pin % 10);
                Serial.println(']');
            }
            last_change[pin] = now;
        } else if ((s == HIGH) && saved_state[pin] == LOW) { // LOW -> HIGH
            last_change[pin] = now;
        }
        saved_state[pin] = s;
    } else {
        saved_state[pin] = -1;
        last_change[pin] = 0;
    }
}

void setup() {
    Serial.begin(SERIAL_SPEED);
    readConfig();
    applyConfig();
    initSavedState();
}

void loop() {
    static int portNo = 0;
    checkPort(portNo);
    portNo = (portNo + 1) % NUMBER_OF_PINS;
}