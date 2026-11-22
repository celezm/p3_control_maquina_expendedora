#pragma once
#include <DHT.h>
#include <Thread.h>

extern DHT dht;

// Tarea para la lectura del sensor de humedad
class DhtThread : public Thread {
public:
  DhtThread() : Thread(NULL, ReadIntervalMs) { enabled = false; }

  void run() override {
    // se guardan para leerlas desde loop()
    hum = dht.readHumidity();
    temp = dht.readTemperature();
    new_read = true;
    runned();
  }

  // Devuelve los resultados de la lectura del sensor:
  bool TakeReading(float &t_out, float &h_out) {
    // new_read evita procesar el mismo dato varias veces
    if (!new_read) return false;
    new_read = false;
    t_out = temp;
    h_out = hum;
    return true;
  }

private:
  static const unsigned long ReadIntervalMs = 2000UL; // tiempo entre ejecuciones de run

  float temp = 0;
  float hum = 0;
  bool new_read = false;
};
