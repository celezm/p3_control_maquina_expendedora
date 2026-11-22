#pragma once
#include <Arduino.h>

enum class ButtonAction : byte { None, Reset, ToggleAdmin };

class Button {
public:
  static const byte Pin = 2;

  ButtonAction Update(volatile bool &pending) {
    const unsigned long now = millis();
    if (pending) {
      // Se limpia la bandera sin que la interrupción pueda cambiarla a mitad.
      noInterrupts();
      pending = false;
      interrupts();
      const bool reading = digitalRead(Pin) == LOW;
      if (reading != raw_btn) {
        raw_btn = reading;
        change_ms = now;
      }
    }

    // Se acepta el cambio solo si la señal lleva 30 ms estable (antirrebote).
    if (raw_btn != stable_btn &&
        (unsigned long)(now - change_ms) >= DebounceMs) {
      stable_btn = raw_btn;

      if (stable_btn) {
        press_ms = now;
        long_done = false;
        Serial.println(F("BOTON EXTERNO: pulsado"));

      } else {
        const unsigned long duration = now - press_ms;
        Serial.print(F("BOTON EXTERNO: liberado tras "));
        Serial.print(duration);
        Serial.println(F(" ms"));

        if (!long_done && duration >= AdminHoldMs) {
          long_done = true;
          return ButtonAction::ToggleAdmin;
        }

        if (!long_done &&
            duration >= ResetMinMs &&
            duration <= ResetMaxMs) {
          return ButtonAction::Reset;
        }

        if (!long_done) Serial.println(F("BOTON EXTERNO: sin accion"));
        
      }
    }

    if (stable_btn && !long_done &&
        (unsigned long)(now - press_ms) >= AdminHoldMs) {
      // Admin se activa al llegar a 5 s, aunque el botón siga pulsado.
      long_done = true;
      return ButtonAction::ToggleAdmin;
    }
    return ButtonAction::None;
  }

private:
  static const unsigned long DebounceMs = 30UL;
  static const unsigned long ResetMinMs = 2000UL;
  static const unsigned long ResetMaxMs = 3000UL;
  static const unsigned long AdminHoldMs = 5000UL;

  bool raw_btn = false;
  bool stable_btn = false;
  bool long_done = false;
  unsigned long change_ms = 0;
  unsigned long press_ms = 0;
};
