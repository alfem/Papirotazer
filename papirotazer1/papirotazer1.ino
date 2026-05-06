#include <FastLED.h>

#define LED_PIN     6          // Cable verde al Pin 6
#define NUM_LEDS    60         // Tu tira LED
//#define NUM_LEDS    144         // Tu tira LED
#define PULSOS_POR_LED  4            // Pulsos del sensor necesarios para avanzar 1 LED (calibración de fuerza)
#define BRIGHTNESS  50         
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB

#define IR_PIN 8
#define BUZZER_PIN 13          // Buzzer activo en Pin 13

// --- COLORES Y TIEMPOS CONFIGURABLES ---
#define COLOR_INICIO   CRGB::Green   // Color del LED mientras espera (posición 0)
#define COLOR_AVANCE   CRGB::Green    // Color del LED mientras avanza por la tira
#define COLOR_FINAL    CRGB::Red   // Color del LED que se queda encendido al llegar
#define COLOR_RETORNO  CRGB::DarkGray     // Color del LED durante el retroceso
#define INACTIVIDAD_MS 1000          // ms sin cambio en el sensor para iniciar retroceso
#define COLOR_RECORD   CRGB::Yellow  // Color del LED que marca el récord máximo

CRGB leds[NUM_LEDS];
int record = -1;                     // Posición más alta alcanzada (-1 = sin récord)

// --- TONADILLA DE RÉCORD ---
void tocarRecord() {
  // Melodía graciosa tipo "¡has ganado!"
  int notas[]    = {523, 659, 784, 1047, 784, 1047}; // Do Mi Sol Do Sol Do (C5 E5 G5 C6 G5 C6)
  int duracion[] = {100, 100, 100, 200,  100, 300};   // duración de cada nota en ms
  int pausas[]   = { 30,  30,  30,  50,   30,   0};   // silencio entre notas

  int numNotas = sizeof(notas) / sizeof(notas[0]);
  for (int n = 0; n < numNotas; n++) {
    tone(BUZZER_PIN, notas[n]);
    delay(duracion[n]);
    noTone(BUZZER_PIN);
    if (pausas[n] > 0) delay(pausas[n]);
  }
}

// --- MELODÍA DE RISA (trombón triste) ---
void tocarRisa() {
  // Glissando descendente tipo "trombón triste"
  for (int f = 622; f >= 220; f -= 4) {
    tone(BUZZER_PIN, f);
    delay(5);
  }
  noTone(BUZZER_PIN);
  delay(80);
  // Tres golpecitos finales cómicos
  int golpes[] = {262, 247, 220};
  for (int n = 0; n < 3; n++) {
    tone(BUZZER_PIN, golpes[n]);
    delay(100);
    noTone(BUZZER_PIN);
    delay(40);
  }
}

// --- SEÑAL DE "LISTO" ---
// Chirp ascendente que indica al jugador que puede golpear el aspa.
// El sensor se empieza a leer JUSTO después de este sonido.
void tocarListo() {
  for (int f = 400; f <= 1600; f += 40) {
    tone(BUZZER_PIN, f);
    delay(3);
  }
  noTone(BUZZER_PIN);
}

void setup() {
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear();
  FastLED.show();

  pinMode(IR_PIN, INPUT);
  // Buzzer pasivo: no necesita pinMode explícito con tone()
  noTone(BUZZER_PIN);             // Asegura que empieza en silencio
  
}

void loop() {
  // --- FASE 1: AVANZAR CON EL DETECTOR DE MOVIMIENTO ---
  // Empieza en el LED 0 y avanza mientras el sensor IR detecta cambios.
  // Cuando el sensor deja de cambiar, se detiene y deja ese LED encendido.

  int pos = 0;                          // Posición actual del LED
  int recordMostrado = record;          // Posición visual del récord (puede desplazarse)
  int contadorPulsos = 0;              // Cuenta pulsos del sensor; avanza LED cada PULSOS_POR_LED

  // Enciende el primer LED y toca la señal de inicio
  leds[pos] = COLOR_INICIO;
  if (record >= 0) leds[recordMostrado] = COLOR_RECORD;
  FastLED.show();

  //tocarListo();   // <- señal audible: a partir de aquí se lee el sensor

  // Lee el sensor JUSTO después de la señal de listo
  int lastSensor = digitalRead(IR_PIN);
  unsigned long ultimoCambio = millis();


  while (pos < NUM_LEDS - 1) {
    int sensorNow = digitalRead(IR_PIN);

    if (sensorNow != lastSensor) {
      // Se detectó un cambio en el sensor
      lastSensor = sensorNow;
      ultimoCambio = millis();          // Reinicia el contador de inactividad
      contadorPulsos++;

      // Solo avanza el LED cada PULSOS_POR_LED pulsos (factor de calibración)
      if (contadorPulsos >= PULSOS_POR_LED) {
        contadorPulsos = 0;

        // Frecuencia ascendente según posición (200 Hz en LED 0 → 2000 Hz en LED final)
        int freqSubida = map(pos, 0, NUM_LEDS - 1, 200, 2000);
        tone(BUZZER_PIN, freqSubida);

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
        delay(20);                          // Duración del pitido (ms)
        noTone(BUZZER_PIN);               // Apaga el tono
        delay(10);                          // Pausa de silencio entre pitidos
      }
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
  bool nuevoRecord = (pos > record);
  if (nuevoRecord) {
    record = pos;
  }
  // El récord vuelve a su posición real (fin del empuje)
  recordMostrado = record;

  // El LED en 'pos' cambia al color final; el récord se muestra si está en otra posición
  leds[pos] = COLOR_FINAL;
  if (record >= 0 && record != pos) leds[record] = COLOR_RECORD;
  FastLED.show();

  // Si hay nuevo récord, celebrar con la tonadilla
  if (nuevoRecord) {
    tocarRecord();
  } else if (pos >0 and pos <= 10) {
    tocarRisa();  // ¡Qué poco has llegado!
  }

  delay(500); // Pausa breve con el LED final encendido

  // --- FASE 2: VOLVER HACIA ATRÁS ---
  // El sensor se ignora completamente durante el retroceso y el cooldown.
  // El jugador debe esperar a la señal de "listo" antes de golpear.
  for (int i = pos - 1; i >= 0; i--) {
    leds[i]   = COLOR_RETORNO;
    leds[i+1] = CRGB::Black;

    if (i == pos - 1) leds[pos] = COLOR_FINAL;
    if (record >= 0 && i != record && (i + 1) != record) leds[record] = COLOR_RECORD;

    FastLED.show();

    int freqBajada = map(i, 0, NUM_LEDS - 1, 200, 2000);
    tone(BUZZER_PIN, freqBajada);
    delay(20);
    noTone(BUZZER_PIN);
    delay(10);
  }
  noTone(BUZZER_PIN);

  // Cooldown: muestra el resultado final antes de la próxima ronda
  delay(1000);

  FastLED.clear();
  if (record >= 0) leds[record] = COLOR_RECORD;
  FastLED.show();
  // La señal de "listo" suena al inicio del siguiente loop(), justo antes de leer el sensor
}