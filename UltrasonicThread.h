#pragma once
#include <Arduino.h>
#include <Thread.h>

class UltrasonicThread : public Thread {
public:
  static const byte TrigPin = A4;
  static const byte EchoPin = A5;

  UltrasonicThread() : Thread(NULL, 0) {}

  void run() override {
    Update();
    runned();
  }

  void start() {
    // Al empezar una nueva espera borramos también el filtro de lecturas anterior.
    state = State::WaitNext;
    measure_us = micros() - MeasurementIntervalUs;
    new_measure = false;
    sample_count = 0;
    sample_index = 0;
    digitalWrite(TrigPin, LOW);
  }

  void Update() {
    const unsigned long now = micros();
    switch (state) {
      case State::WaitNext:
        // Se espera entre "triggers" para evitar ecos del pulso anterior.
        if ((unsigned long)(now - measure_us) >= MeasurementIntervalUs) {
          digitalWrite(TrigPin, HIGH);
          mark_us = micros();
          state = State::TriggerPulse;
        }
        break;
      case State::TriggerPulse:
        // TRIG debe estar alto al menos 10 microsegundos.
        if ((unsigned long)(now - mark_us) >= 10UL) {
          digitalWrite(TrigPin, LOW);
          mark_us = micros();
          state = State::WaitEchoHigh;
        }
        break;
      case State::WaitEchoHigh:
        // Primero esperamos el inicio del eco si no llega, guardamos una lectura fallida.
        if (digitalRead(EchoPin) == HIGH) {
          echo_start = micros();
          state = State::WaitEchoLow;
        } else if ((unsigned long)(now - mark_us) >= EchoTimeoutUs) {
          Finish(0);
        }
        break;
      case State::WaitEchoLow:
        // La duración del pulso ECHO indica el tiempo que ha viajado el sonido.
        if (digitalRead(EchoPin) == LOW) {
          Finish((unsigned long)(micros() - echo_start));
        } else if ((unsigned long)(now - echo_start) >= EchoTimeoutUs) {
          Finish(0);
        }
        break;
    }
  }

  bool TakeMeasurement(unsigned int &dist) {
    // La medición queda disponible hasta que el programa principal la recoge
    if (!new_measure) return false;
    new_measure = false;
    dist = dist_cm;
    return true;
  }

private:
  enum class State : byte {WaitNext, TriggerPulse, WaitEchoHigh, WaitEchoLow};
  static const unsigned long MeasurementIntervalUs = 100000UL;
  static const unsigned long EchoTimeoutUs = 30000UL;
  static const byte FilterSampleCount = 3;

  State state = State::WaitNext;

  unsigned long measure_us = 0;
  unsigned long mark_us = 0;
  unsigned long echo_start = 0;
  unsigned int dist_cm = 0;
  unsigned int samples[FilterSampleCount] = {0, 0, 0};

  byte sample_count = 0;
  byte sample_index = 0;
  bool new_measure = false;

  void Finish(unsigned long echo_us) {
    const unsigned int sample_cm =
        echo_us == 0 ? 0 : echo_us / 58UL;
    samples[sample_index] = sample_cm;
    sample_index = (sample_index + 1) % FilterSampleCount;

    if (sample_count < FilterSampleCount) ++sample_count;
    if (sample_count == FilterSampleCount) {
      dist_cm = Median3(samples[0], samples[1], samples[2]);
      new_measure = true;
    }

    measure_us = micros();
    state = State::WaitNext;
  }

  unsigned int Median3(unsigned int a, unsigned int b,
                       unsigned int c) const {
    if (a > b) { const unsigned int temp = a; a = b; b = temp; }
    if (b > c) { const unsigned int temp = b; b = c; c = temp; }
    if (a > b) { const unsigned int temp = a; a = b; b = temp; }
    return b;
  }
};
