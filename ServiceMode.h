#pragma once
#include "LedThread.h"
#include "ServiceMenu.h"

// Modo servicio
class ServiceMode {
public:
  static const byte Led2Pin = 3;

  explicit ServiceMode(ProductCatalog &catalog, LedThread &led)
      : menu(catalog), led(led) {}

  void Enter() {
    state = State::WaitingForCustomer;
    led.Stop();
    log_ms = millis() - 500UL;
    WriteLine(0, "ESPERANDO");
    WriteLine(1, "CLIENTE");
    // Serial.println(F("SERVICIO: ESPERANDO CLIENTE"));
  }

  void ReceiveDistance(unsigned int dist_cm) {
    if (state != State::WaitingForCustomer) return;
    if (dist_cm > 0 && dist_cm < 100) {
      // Conservamos la pantalla de espera hasta que llegue la primera medida DHT.
      state = State::Monitoring;
      monitor_start = millis();
      // Serial.print(F("CLIENTE DETECTADO A "));
      // Serial.print(dist_cm);
      // Serial.println(F(" cm"));
      return;
    }
    const unsigned long now = millis();
    if ((unsigned long)(now - log_ms) >= 500UL) {
      log_ms = now;
      if (dist_cm == 0) {
        // Serial.println(F("ESPERANDO: SIN ECO"));
      } else {
        // Serial.print(F("ESPERANDO: "));
        // Serial.print(dist_cm);
        // Serial.println(F(" cm"));
      }
    }
  }

  void Update(unsigned long now) {
    // Cada fase comprueba su tiempo con millis() y cambia de estado sin delay().
    if (state == State::Monitoring) {
      if ((unsigned long)(now - monitor_start) >=
          MonitoringDurationMs) {
        state = State::Catalog;
        menu.Reset();
        menu.Draw();
        // Serial.println(F("SERVICIO: CATALOGO"));
      }
    } else if (state == State::Preparing) {
      const unsigned long elapsed = now - prep_start;
      if (elapsed >= prep_ms) {
        led.Stop();
        state = State::Pickup;
        pickup_start = now;
        WriteLine(0, "RETIRE BEBIDA");
        WriteLine(1, "");
        // Serial.println(F("SERVICIO: RETIRE BEBIDA"));
      }
    } else if (state == State::Pickup &&
               (unsigned long)(now - pickup_start) >= PickupDurationMs) {
      Enter();
    }
  }

  void ReceiveEnvironment(float temp, float hum) {
    if (state != State::Monitoring) return;
    if (isnan(hum) || isnan(temp)) {
      WriteLine(0, "DHT ERROR");
      WriteLine(1, "Revisar DATA D4");
      // Serial.println(F("DHT11: error de lectura"));
    } else {
      WriteLine(0, "Temp:");
      lcd.setCursor(6, 0);
      lcd.print(temp, 1);
      lcd.print(F(" C"));
      WriteLine(1, "Hum:");
      lcd.setCursor(5, 1);
      lcd.print(hum, 1);
      lcd.print(F(" %"));
      // Serial.print(F("DHT11 Temp: "));
      // Serial.print(temp, 1);
      // Serial.print(F(" C Hum: "));
      // Serial.print(hum, 1);
      // Serial.println(F(" %"));
    }
  }

  bool WaitingForCustomer() const { return state == State::WaitingForCustomer; }
  bool Monitoring() const { return state == State::Monitoring; }
  bool InCatalog() const { return state == State::Catalog; }
  void Handle(Event event) {
    if (!InCatalog() || !menu.Handle(event)) return;
    // El precio ya viene del catálogo compartido con AdminMode.
    state = State::Preparing;
    prep_start = millis();
    prep_ms = random(PreparationMinMs,
                                 PreparationMaxMs + 1UL);
    if (prep_ms == prev_prep) {
      prep_ms =
          prep_ms == PreparationMaxMs
              ? PreparationMinMs
              : prep_ms + 1UL;
    }
    prev_prep = prep_ms;
    led.StartProportional(prep_ms);
    WriteLine(0, "Preparando Cafe");
    WriteLine(1, "...");
    // Serial.print(F("SELECCIONADO: "));
    // Serial.print(menu.SelectedName());
    // Serial.print(F(" "));
    // Serial.print(menu.SelectedPrice());
    // Serial.print(F(" centimos; preparacion "));
    // Serial.print(prep_ms);
    // Serial.println(F(" ms"));
  }
  void Draw() {
    if (InCatalog()) menu.Draw();
  }

private:
  static const unsigned int CustomerDistanceCm = 100;
  static const unsigned long MonitoringDurationMs = 5000UL;
  static const unsigned long PreparationMinMs = 4000UL;
  static const unsigned long PreparationMaxMs = 8000UL;
  static const unsigned long PickupDurationMs = 3000UL;

  enum class State : byte { WaitingForCustomer, Monitoring, Catalog,
                            Preparing, Pickup };

  State state = State::WaitingForCustomer;
  ServiceMenu menu;
  LedThread &led;
  unsigned long log_ms = 0;
  unsigned long monitor_start = 0;
  unsigned long prep_start = 0;
  unsigned long prep_ms = 0;
  unsigned long prev_prep = 0;
  unsigned long pickup_start = 0;
};
