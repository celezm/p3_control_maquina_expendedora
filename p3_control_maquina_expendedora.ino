#include <Arduino.h>
#include <Thread.h>
#include <ThreadController.h>

#define JOYSTICK_X A0
#define JOYSTICK_Y A1
#define JOYSTICK_SW 2

int joystickX = 0;
int joystickY = 0;
bool joystickPressed = false;

ThreadController controller;

Thread joystickThread;
Thread serialThread;

void readJoystick() {
    joystickX = analogRead(JOYSTICK_X);
    joystickY = analogRead(JOYSTICK_Y);

    joystickPressed = (digitalRead(JOYSTICK_SW) == LOW);
}

void printJoystick() {
    Serial.print("X: ");
    Serial.print(joystickX);

    Serial.print(" | Y: ");
    Serial.print(joystickY);

    Serial.print(" | SW: ");
    Serial.println(joystickPressed);
}

void setup() {
    Serial.begin(9600);

    pinMode(JOYSTICK_SW, INPUT_PULLUP);

    joystickThread.onRun(readJoystick);
    joystickThread.setInterval(50);

    serialThread.onRun(printJoystick);
    serialThread.setInterval(200);

    controller.add(&joystickThread);
    controller.add(&serialThread);
}

void loop() {
    controller.run();
}