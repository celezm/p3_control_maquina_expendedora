#pragma once
#include <Thread.h>

enum class Event : byte { None, Up, Down, Left, Right, Press };

// Tarea para la lectura del Joystick
class JoystickThread : public Thread {
public:
  static const byte SwPin = 12;
  static const byte YPin = A1;
  static const byte XPin = A2;

  JoystickThread() : Thread(NULL, SampleIntervalMs) {}

  Event TakeEvent() {
    // Se entrega el evento una sola vez y se deja el "buzón" vacío.
    Event result = pending;
    pending = Event::None;
    return result;
  }

  void run() override {
    // Joystick girado: pin A1 controla izquierda/derecha y A2 arriba/abajo.
    const int y = analogRead(YPin);
    const int x = analogRead(XPin);

    // Un desplazamiento solo cuenta una vez, hasta volver al centro.
    if (y > CenterMin && y < CenterMax) {
      y_centered = true;
    } else if (y_centered && (y < MoveLow || y > MoveHigh)) {
      const bool y_high = y > MoveHigh;
      Emit(y_high ? Event::Left : Event::Right);
      y_centered = false;
    }

    if (x > CenterMin && x < CenterMax) {
      x_centered = true;
    } else if (x_centered && (x < MoveLow || x > MoveHigh)) {
      const bool x_high = x > MoveHigh;
      Emit(x_high ? Event::Up : Event::Down);
      x_centered = false;
    }

    const unsigned long now = millis();
    const bool pressed = digitalRead(SwPin) == LOW;
    if (pressed != raw_btn) {
      raw_btn = pressed;
      last_change = now;
    }
    if ((unsigned long)(now - last_change) >= ButtonDebounceMs &&
        pressed != stable_btn) {
      stable_btn = pressed;
      if (stable_btn) Emit(Event::Press);
    }
    // runned() marca el final de esta muestra periódica.
    runned();
  }

private:
  static const unsigned long SampleIntervalMs = 40UL; // tiempo entre ejecuciones de run
  static const unsigned long ButtonDebounceMs = 30UL; // antirrebote
  static const int CenterMin = 400;
  static const int CenterMax = 624;
  static const int MoveLow = 350;
  static const int MoveHigh = 674;

  Event pending = Event::None;
  bool y_centered = true;
  bool x_centered = true;
  bool raw_btn = false;
  bool stable_btn = false;
  unsigned long last_change = 0;

  void Emit(Event event) {

    // Guarda el evento si no había ya alguno pendiente
    if (pending != Event::None) return;
    pending = event;
    Serial.print(F("JOY "));
    switch (event) {
      case Event::Up: Serial.println(F("ARRIBA")); break;
      case Event::Down: Serial.println(F("ABAJO")); break;
      case Event::Left: Serial.println(F("IZQUIERDA")); break;
      case Event::Right: Serial.println(F("DERECHA")); break;
      case Event::Press: Serial.println(F("PULSAR")); break;
      case Event::None: break;
    } 
  }
};
