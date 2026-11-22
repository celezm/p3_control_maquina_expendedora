#pragma once
#include "JoystickThread.h"
#include "ProductCatalog.h"

class AdminMode {
public:
  explicit AdminMode(ProductCatalog &cat_ref)
      : opts{"Ver temperatura", "Ver distancia", "Ver contador",
                "Editar precios"},
        catalog(cat_ref) {}

  void Enter() {
    // Al entrar en Admin empezamos siempre en la primera opción.
    view = View::Menu;
    opt_idx = 0;
    prod_idx = 0;
    env_ok = false;
    dist_ok = false;
    dirty = true;
  }

  // El .ino usa estas consultas para activar solo el sensor de la vista actual.
  bool ReadsTemperature() const { return view == View::Temperature; }
  bool ReadsDistance() const { return view == View::Distance; }

  void Handle(Event event) {
    if (event == Event::None) return;

    // El mismo movimiento tiene un efecto distinto según la pantalla admin abierta.
    if (view == View::Menu) {
      if (event == Event::Up) {
        opt_idx = (opt_idx + OptionCount - 1) % OptionCount;
        dirty = true;

      } else if (event == Event::Down) {
        opt_idx = (opt_idx + 1) % OptionCount;
        dirty = true;

      } else if (event == Event::Press) {
        Serial.print(F("ADMIN: "));
        Serial.println(opts[opt_idx]);

        if (opt_idx == 0) {
          view = View::Temperature;
          env_ok = false;
        } else if (opt_idx == 1) {
          view = View::Distance;
          dist_ok = false;
        } else if (opt_idx == 2) {
          view = View::Counter;
        } else {
          view = View::PriceList;
          prod_idx = 0;
        }

        dirty = true;

      }
      return;
    }

    if (view == View::PriceList) {
      if (event == Event::Left) {
        view = View::Menu;
        dirty = true;
        return;
      }

      if (event == Event::Up) {
        // El módulo hace que al subir desde el primero aparezca el último producto.
        prod_idx = (prod_idx + catalog.Count() - 1) % catalog.Count();
        dirty = true;
      } else if (event == Event::Down) {
        prod_idx = (prod_idx + 1) % catalog.Count();
        dirty = true;
      } else if (event == Event::Press) {
        // Editamos una copia temporal, el catálogo no cambia hasta confirmar.
        edit_cts = catalog.Get(prod_idx).price_cents;
        view = View::PriceEdit;
        dirty = true;
      }

      return;
    }

    if (view == View::PriceEdit) {
      if (event == Event::Left) {
        // Salir sin confirmar descarta edit_cts y conserva el precio original.
        view = View::PriceList;
        dirty = true;
        return;
      }

      if (event == Event::Up && edit_cts <= 9990) {
        edit_cts += 5;
        dirty = true;
      } else if (event == Event::Down && edit_cts >= 5) {
        edit_cts -= 5;
        dirty = true;
      } else if (event == Event::Press) {
        // El cambio se copia al catálogo solo al confirmar con SW.
        catalog.SetPriceCents(prod_idx, edit_cts);
        Serial.print(F("ADMIN PRECIO: "));
        Serial.print(catalog.Get(prod_idx).name);
        Serial.print(F(" = "));
        Serial.print(edit_cts / 100);
        Serial.write('.');
        if (edit_cts % 100 < 10) Serial.write('0');
        Serial.println(edit_cts % 100);
        view = View::PriceList;
        dirty = true;
      }

      return;
    }

    if (event == Event::Left) {
      view = View::Menu;
      dirty = true;
    }
  }

  void ReceiveEnvironment(float temp_val, float hum_val) {
    temp = temp_val;
    hum = hum_val;
    env_ok = !isnan(temp) && !isnan(hum);
    dirty = true;
  }

  void ReceiveDistance(unsigned int dist_val) {
    if (!dist_ok || dist_cm != dist_val) dirty = true;
    dist_cm = dist_val;
    dist_ok = true;
  }

  void Update(unsigned long now) {
    if (view == View::Counter && now / 1000UL != shown_s) {
      shown_s = now / 1000UL;
      dirty = true;
    }
  }

  void Draw(unsigned long now) {
    // Solo repintamos cuando cambia algo para no escribir la LCD en cada loop().
    if (!dirty) return;
    dirty = false;

    if (view == View::Menu) {
      WriteLine(0, opts[opt_idx]);
      WriteLine(1, opts[(opt_idx + 1) % OptionCount]);

    } else if (view == View::Temperature) {
      if (!env_ok) {
        WriteLine(0, "Temp: -- C");
        WriteLine(1, "Hum: -- %");
      } else {
        WriteLine(0, "Temp:");
        lcd.setCursor(6, 0);
        lcd.print(temp, 1);
        lcd.print(F(" C"));
        WriteLine(1, "Hum:");
        lcd.setCursor(5, 1);
        lcd.print(hum, 1);
        lcd.print(F(" %"));
      }

    } else if (view == View::Distance) {
      WriteLine(0, "Distancia:");
      if (!dist_ok || dist_cm == 0) {
        WriteLine(1, "Sin eco");
      } else {
        WriteLine(1, "");
        lcd.setCursor(0, 1);
        lcd.print(dist_cm);
        lcd.print(F(" cm"));
      }

    } else if (view == View::Counter) {
      WriteLine(0, "Segundos:");
      WriteLine(1, "");
      lcd.setCursor(0, 1);
      lcd.print(now / 1000UL);

    } else if (view == View::PriceList) {
      WriteLine(0, catalog.Get(prod_idx).name);
      const int price = catalog.Get(prod_idx).price_cents;
      WriteLine(1, "");
      lcd.setCursor(0, 1);
      lcd.print(price / 100);
      lcd.print('.');
      if (price % 100 < 10) lcd.print('0');
      lcd.print(price % 100);
      lcd.print(F(" EUR SW=OK"));

    } else {
      WriteLine(0, catalog.Get(prod_idx).name);
      WriteLine(1, "");
      lcd.setCursor(0, 1);
      lcd.print(edit_cts / 100);
      lcd.print('.');
      if (edit_cts % 100 < 10) lcd.print('0');
      lcd.print(edit_cts % 100);
      lcd.print(F(" +/-5 SW=OK"));

    }
  }

private:
  enum class View : byte { Menu, Temperature, Distance, Counter, PriceList, PriceEdit };
  static const byte OptionCount = 4;
  const char *const opts[OptionCount];
  ProductCatalog &catalog;
  View view = View::Menu;
  byte opt_idx = 0;
  byte prod_idx = 0;
  int edit_cts = 0;
  float temp = 0;
  float hum = 0;
  unsigned int dist_cm = 0;
  unsigned long shown_s = 0;
  bool env_ok = false;
  bool dist_ok = false;
  bool dirty = true;
};
