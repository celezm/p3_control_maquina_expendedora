#pragma once
#include "JoystickThread.h"
#include "ProductCatalog.h"

// Navegacion y seleccion del menu servicio
class ServiceMenu {
public:
  explicit ServiceMenu(ProductCatalog &cat_ref) : catalog(cat_ref) {}

  void Reset() {
    first_vis = 0;
    dirty = true;
  }

  bool Handle(Event event) {
    // Este menú solo desplaza la lista
    // el producto de la primera fila es el que se selecciona
    switch (event) {
      case Event::Up:
        // El módulo permite pasar del primer producto al último.
        first_vis = (first_vis + catalog.Count() - 1) % catalog.Count();
        dirty = true;
        return false;
      case Event::Down:
        first_vis = (first_vis + 1) % catalog.Count();
        dirty = true;
        return false;
      case Event::Press:
        return true; // La primera fila es el producto seleccionado.
      case Event::Left:
      case Event::Right:
      case Event::None:
        return false;
    }
    return false;
  }

  const char *SelectedName() const {
    return catalog.Get(first_vis).name;
  }

  int SelectedPrice() const {
    return catalog.Get(first_vis).price_cents;
  }

  void Draw() {
    // dirty indica que hay que repintar la pantalla
    // los movimientos sin cambios no escriben la LCD

    if (!dirty) return;
    dirty = false;
    // Reservamos 12 columas de la lcd al nombre y 4 al precio

    for (byte row = 0; row < 2; ++row) {
      const byte index = (first_vis + row) % catalog.Count();
      const Product &product = catalog.Get(index);

      lcd.setCursor(0, row);
      byte column = 0;

      while (column < 12 && product.name[column] != '\0') {
        lcd.write(product.name[column]);
        ++column;
      }

      // rellena lo que falta de la lcd con espacios
      while (column++ < 12) lcd.write(' ');

      // escritura del precio
      lcd.print(product.price_cents / 100);
      lcd.write('.');
      const int cents = product.price_cents % 100;
      if (cents < 10) lcd.write('0');
      lcd.print(cents);
    }
  }

private:
  ProductCatalog &catalog;
  byte first_vis = 0;
  bool dirty = true;
};
