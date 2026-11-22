#pragma once
#include "LedThread.h"

class StartupMode {
public:
  static const byte Led1Pin = 11;
  static const unsigned long BlinkIntervalMs = 500UL;

  explicit StartupMode(LedThread &led) : led(led) {}

  void Enter() {
    // El modo arranque pide tres destellos, LedThread se ocupa de temporizarlos.
    led.StartBlink(3, BlinkIntervalMs);
    WriteLine(0, "CARGANDO...");
    WriteLine(1, "");
    Serial.println(F("MODO ARRANQUE"));
  }

  // loop() cambia a Servicio cuando la tarea LED informa que terminó.
  bool Update() const { return led.Finished(); }

private:
  LedThread &led;
};

// El eco se mide en loop() para respetar el pulso de 10 us sin bloquear.
