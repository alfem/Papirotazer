#include <FastLED.h>

#define LED_PIN     6          // Cable verde al Pin 6
#define NUM_LEDS    60         // Tu tira de 30 LEDs
#define BRIGHTNESS  50         
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB

#define IR_PIN 8

// --- COLORES Y TIEMPOS CONFIGURABLES ---
#define COLOR_INICIO   CRGB::Green   // Color del LED mientras espera (posición 0)
#define COLOR_AVANCE   CRGB::Green    // Color del LED mientras avanza por la tira
#define COLOR_FINAL    CRGB::Red   // Color del LED que se queda encendido al llegar
#define COLOR_RETORNO  CRGB::DarkGray     // Color del LED durante el retroceso
#define INACTIVIDAD_MS 1000          // ms sin cambio en el sensor para iniciar retroceso
#define COLOR_RECORD   CRGB::Yellow  // Color del LED que marca el récord máximo

CRGB leds[NUM_LEDS];
int record = -1;                     // Posición más alta alcanzada (-1 = sin récord)

void setup() {
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear();
  FastLED.show();

  pinMode(IR_PIN, INPUT); 
  
}

void loop() {
  // --- FASE 1: AVANZAR CON EL DETECTOR DE MOVIMIENTO ---
  // Empieza en el LED 0 y avanza mientras el sensor IR detecta cambios.
  // Cuando el sensor deja de cambiar, se detiene y deja ese LED encendido.

  int pos = 0;                          // Posición actual del LED
  int lastSensor = digitalRead(IR_PIN); // Estado inicial del sensor
  int recordMostrado = record;          // Posición visual del récord (puede desplazarse)

  // Enciende el primer LED (color de inicio) y espera a que haya movimiento
  leds[pos] = COLOR_INICIO;
  if (record >= 0) leds[recordMostrado] = COLOR_RECORD;
  FastLED.show();

  // Bucle de avance: sigue mientras no hayamos llegado al final
  unsigned long ultimoCambio = millis(); // Marca de tiempo del último cambio detectado

  while (pos < NUM_LEDS - 1) {
    int sensorNow = digitalRead(IR_PIN);

    if (sensorNow != lastSensor) {
      // Se detectó un cambio en el sensor → avanzar un LED
      lastSensor = sensorNow;
      ultimoCambio = millis();          // Reinicia el contador de inactividad

      leds[pos] = CRGB::Black;           // Apaga el LED actual
      pos++;

      // --- Lógica del empuje del récord ---
      if (record >= 0) {
        // Cuando el LED avanzante alcanza al récord, lo empuja una posición adelante
        if (pos >= recordMostrado && recordMostrado < NUM_LEDS - 1) {
          leds[recordMostrado] = CRGB::Black; // Borra la posición anterior del récord
          recordMostrado = pos + 1;            // Empuja el récord una posición por delante
          leds[recordMostrado] = COLOR_RECORD;
        } else if (pos != recordMostrado) {
          leds[recordMostrado] = COLOR_RECORD; // Mantiene el récord visible si no hay solapamiento
        }
      }

      leds[pos] = COLOR_AVANCE;          // LED avanzante encima del récord si coinciden
      FastLED.show();
      delay(30);                          // Pausa corta para estabilizar
    }

    // Si no hay cambio durante INACTIVIDAD_MS, inicia el retroceso
    if (millis() - ultimoCambio >= INACTIVIDAD_MS) {
      break;
    }
  }

  // Borra la posición "empujada" del récord antes de actualizar el estado final
  // (evita que queden dos LEDs amarillos simultáneamente)
  if (record >= 0 && recordMostrado != pos) {
    leds[recordMostrado] = CRGB::Black;
  }

  // Actualiza el récord real si se ha superado
  if (pos > record) {
    record = pos;
  }
  // El récord vuelve a su posición real (fin del empuje)
  recordMostrado = record;

  // El LED en 'pos' cambia al color final; el récord se muestra si está en otra posición
  leds[pos] = COLOR_FINAL;
  if (record >= 0 && record != pos) leds[record] = COLOR_RECORD;
  FastLED.show();
  delay(500); // Pausa breve con el LED final encendido

  // --- FASE 2: VOLVER HACIA ATRÁS ---
  for (int i = pos - 1; i >= 0; i--) {
    leds[i]   = COLOR_RETORNO;         // Enciende el LED actual (color de retorno)
    leds[i+1] = CRGB::Black;          // Apaga el anterior

    // Mantiene el LED fijo en 'pos' con el color final durante toda la vuelta
    if (i == pos - 1) {
      leds[pos] = COLOR_FINAL;
    }
    // Mantiene el récord visible durante el retroceso
    if (record >= 0 && i != record && (i + 1) != record) leds[record] = COLOR_RECORD;

    FastLED.show();
    delay(10);
  }

  delay(1000); // Pausa antes de reiniciar
  FastLED.clear();                     // Apaga todos los LEDs...
  if (record >= 0) {
    leds[record] = COLOR_RECORD;       // ...excepto el del récord
  }
  FastLED.show();
}