#include <avr/wdt.h>
#include <Arduino.h>
#include <LiquidCrystal.h>

extern LiquidCrystal lcd;
void WriteLine(byte row, const char *text);

#include <ThreadController.h>
#include "JoystickThread.h"
#include "DhtThread.h"
#include "Button.h"
#include "LedThread.h"
#include "ServiceMenu.h"
#include "ProductCatalog.h"
#include "StartupMode.h"
#include "UltrasonicThread.h"
#include "ServiceMode.h"
#include "AdminMode.h"

enum class Mode : byte { Startup, Service, Admin };

// pines LCD
const byte LcdRs = 5;
const byte LcdE = 6;
const byte LcdD4 = 7;
const byte LcdD5 = 8;
const byte LcdD6 = 9;
const byte LcdD7 = 10;

// pines sensor humedad y temperatura
const byte DhtPin = 4;
LiquidCrystal lcd(LcdRs, LcdE, LcdD4, LcdD5, LcdD6, LcdD7);
DHT dht(DhtPin, DHT11);

void WriteLine(byte row, const char *text) {
  lcd.setCursor(0, row);
  byte column = 0;
  while (column < 16 && text[column] != '\0') {
    lcd.write(text[column]);
    ++column;
  }
  while (column++ < 16) lcd.write(' ');
}

ThreadController tasks;
JoystickThread joystick;
DhtThread dht_sensor;
LedThread led1(StartupMode::Led1Pin);
LedThread led2(ServiceMode::Led2Pin);
StartupMode startup(led1);
ProductCatalog catalog;
ServiceMode service(catalog, led2);
AdminMode admin(catalog);
UltrasonicThread ultrasonic_sensor;
Mode mode = Mode::Startup;
Button button;

// Cambian su valor en interrupciones, volatile hace
// que se revise su valor cada vez que se utiliza
volatile bool btn_changed = false;
const unsigned long StartupSettleMs = 500UL;
unsigned long startup_wait_ms = 0;
bool startup_started = false;

void ExternalButtonIsr() {
  btn_changed = true;
}

void EnterService() {
  mode = Mode::Service;
  led1.Stop();
  joystick.TakeEvent(); // Descarta eventos anteriores al reinicio.
  service.Enter();
  ultrasonic_sensor.start();
}

void EnterAdmin() {
  mode = Mode::Admin;
  led1.SetLevel(255);
  led2.SetLevel(255);
  joystick.TakeEvent();
  dht_sensor.enabled = false;
  admin.Enter();
  admin.Draw(millis());
  Serial.println(F("BOTON EXTERNO: ENTRAR ADMIN"));
}

void setup() {
  MCUSR = 0;
  wdt_disable();
  Serial.begin(115200);

  led1.Begin();
  led2.Begin();
  pinMode(Button::Pin, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(Button::Pin),
                  ExternalButtonIsr, CHANGE);

  pinMode(JoystickThread::SwPin, INPUT_PULLUP);
  pinMode(UltrasonicThread::TrigPin, OUTPUT);
  pinMode(UltrasonicThread::EchoPin, INPUT);

  digitalWrite(UltrasonicThread::TrigPin, LOW);

  lcd.begin(16, 2);
  dht.begin();
  WriteLine(0, "CARGANDO...");
  WriteLine(1, "");

  randomSeed(analogRead(A0));

  joystick.enabled = false;
  dht_sensor.enabled = false;
  ultrasonic_sensor.enabled = false;

  tasks.add(&joystick);
  tasks.add(&dht_sensor);
  tasks.add(&ultrasonic_sensor);
  tasks.add(&led1);
  tasks.add(&led2);

  startup_wait_ms = millis();

  wdt_enable(WDTO_2S);
}

void loop() {

  wdt_reset();

  if (mode == Mode::Startup && !startup_started) {
    if ((unsigned long)(millis() - startup_wait_ms) >= StartupSettleMs) {
      startup.Enter();
      startup_started = true;
    }
    return;
  }

  // Se revisa si se ha accionado el botón: 2-3s (rst) o 5s (admin)
  const ButtonAction action = button.Update(btn_changed);

  if (action == ButtonAction::ToggleAdmin) {
    if (mode == Mode::Admin) {
      Serial.println(F("BOTON EXTERNO: SALIR ADMIN"));
      EnterService();
    } else {
      EnterAdmin();
    }

  } else if (action == ButtonAction::Reset) {
    if (mode == Mode::Service) {
      Serial.println(F("BOTON EXTERNO: RESET SERVICIO"));
      EnterService();
    }
  }

  if (mode == Mode::Admin) {
    joystick.enabled = true;
    dht_sensor.enabled = admin.ReadsTemperature();
    ultrasonic_sensor.enabled = admin.ReadsDistance();
    tasks.run();

    const bool was_dist = admin.ReadsDistance();
    admin.Handle(joystick.TakeEvent());
    if (!was_dist && admin.ReadsDistance()) {
      ultrasonic_sensor.start();
    }

    float temp, hum;
    if (dht_sensor.TakeReading(temp, hum)) {
      admin.ReceiveEnvironment(temp, hum);
    }

    if (admin.ReadsDistance()) {
      unsigned int dist_cm;
      if (ultrasonic_sensor.TakeMeasurement(dist_cm)) {
        admin.ReceiveDistance(dist_cm);
      }

    }

    const unsigned long now = millis();
    admin.Update(now);
    admin.Draw(now);
    return;
  }

  if (mode == Mode::Startup) {
    tasks.run();
    if (startup.Update()) {
      EnterService();
    }
    return;
  }

  // Modo servicio:

  // Esperando al usuario:
  const bool was_waiting = service.WaitingForCustomer();
  service.Update(millis());
  if (!was_waiting && service.WaitingForCustomer()) {
    ultrasonic_sensor.start();
  }
  joystick.enabled = service.InCatalog();
  dht_sensor.enabled = service.Monitoring();
  ultrasonic_sensor.enabled = service.WaitingForCustomer();
  tasks.run();

  if (service.WaitingForCustomer()) {
    unsigned int dist_cm;

    if (ultrasonic_sensor.TakeMeasurement(dist_cm)) {
      service.ReceiveDistance(dist_cm);
    }

  }
  
  // Temperatura y humedad 5s
  float temp, hum;
  if (dht_sensor.TakeReading(temp, hum)) {
    service.ReceiveEnvironment(temp, hum);
  }

  // Navegación por el catalogo
  if (service.InCatalog()) {
    service.Handle(joystick.TakeEvent());
    service.Draw();
  }

}
