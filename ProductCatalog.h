#pragma once
#include <Arduino.h>

// Clase para los productos del menu
struct Product {
  const char *name;
  int price_cents;
};

class ProductCatalog {
public:
  static const byte ProductCount = 5;

  // Diccionario que almacena los productos y precios: para hacer visible la modificacion
  ProductCatalog() : products{
    {"Cafe Solo",    100},
    {"Cafe Cortado", 110},
    {"Cafe Doble",   125},
    {"Cafe Premium", 150},
    {"Chocolate",    200}
  } {}

  byte Count() const { return ProductCount; }

  const Product &Get(byte index) const {
    return products[index < ProductCount ? index : 0];
  }

  bool SetPriceCents(byte index, int price_cents) {
    // Se comprueba el índice y el rango antes de modificar el precio compartido.
    if (index >= ProductCount || price_cents < 0 || price_cents > 9999) {
      return false;
    }
    products[index].price_cents = price_cents;
    return true;
  }

private:
  Product products[ProductCount];
};
