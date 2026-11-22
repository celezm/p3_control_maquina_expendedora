#pragma once
#include <Arduino.h>
#include <Thread.h>

class LedThread : public Thread {
public:
  // La tarea se revisa cada 20 ms
  static const unsigned long TaskIntervalMs = 20UL;

  explicit LedThread(byte led_pin)
      : Thread(NULL, TaskIntervalMs), pin(led_pin) {
    enabled = false;
  }

  void Begin() {
    pinMode(pin, OUTPUT);
    Stop();
  }

  void StartBlink(byte flash_count, unsigned long interval_ms) {
    if (flash_count == 0 || interval_ms == 0) {
      Stop();
      return;
    }

    // Cada destello tiene dos cambios: encender y apagar.
    operation = Operation::Blink;
    edges_left = (unsigned int)flash_count * 2;
    blink_interval_ms = interval_ms;
    last_step_ms = millis();
    finished = false;
    level = 255;
    analogWrite(pin, level);
    enabled = true;
  }

// Ilumina el led de forma proporcional
  void StartProportional(unsigned long duration_ms) {
    if (duration_ms == 0) {
      SetLevel(255);
      return;
    }

    // El brillo se calcula como porcentaje del tiempo 
    operation = Operation::Proportional;
    duration_ms_total = duration_ms;
    started_ms = millis();
    level = 0;
    analogWrite(pin, level);
    finished = false;
    enabled = true;
  }

  // Setea un led al "nivel" pwm que se le mande
  void SetLevel(byte new_level) {
    // Un nivel fijo cancela cualquier parpadeo o rampa que estuviera activa.
    operation = Operation::Idle;
    level = new_level;
    analogWrite(pin, level);
    finished = true;
    enabled = false;
  }

  void Stop() { SetLevel(0); }

  bool Finished() const { return finished; }

  void run() override {
    const unsigned long now = millis();

    if (operation == Operation::Blink &&
        (unsigned long)(now - last_step_ms) >= blink_interval_ms) {
      last_step_ms = now;
      --edges_left;

      if (edges_left == 0) {
        SetLevel(0);
      } else {
        level = level == 0 ? 255 : 0;
        analogWrite(pin, level);
      }
    } else if (operation == Operation::Proportional) {
      const unsigned long elapsed = now - started_ms;
      if (elapsed >= duration_ms_total) {
        SetLevel(255);
      } else {
        const byte new_level =
            (unsigned long)(elapsed * 255UL) / duration_ms_total;
        if (new_level != level) {
          level = new_level;
          analogWrite(pin, level);
        }
      }
    }

    runned();
  }

private:
  enum class Operation : byte { Idle, Blink, Proportional };

  byte pin;
  byte level = 0;
  unsigned int edges_left = 0;
  unsigned long blink_interval_ms = 0;
  unsigned long duration_ms_total = 0;
  unsigned long started_ms = 0;
  unsigned long last_step_ms = 0;
  Operation operation = Operation::Idle;
  bool finished = true;
};
