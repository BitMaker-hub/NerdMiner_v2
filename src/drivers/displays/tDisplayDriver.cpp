/********************************************************************************************
 *
 *   Escrito por: M8AX
 *
 *   Descripción:
 *   ------------
 *
 *   Expansión de las pantallas del NerdMinerV2 con aún más datos y mucho más "nerd"
 *   de lo que ya era xD...
 *
 *   Se añaden más funciones y datos para que puedas ver en tiempo real lo que está
 *   ocurriendo en tu minero, además de poder enviar mensajes a tu canal de telegram.
 *
 *   Se añaden más funciones para que puedas ver datos en pantalla y enviarlos a tu
 *   canal de telegram, como la factorización prima de un número aleatorio, el día de
 *   la semana, la hora, la fecha, la IP pública, el precio de Bitcoin, el promedio por
 *   transacción, la altura de bloque, el hash rate global, el tiempo minando, la mejor
 *   dificultad alcanzada, la dificultad de la red, el cómputo total, el hash rate actual,
 *   la temperatura de la CPU, las plantillas de bloque, los shares enviados a la pool, En
 *   resumen, un montón de datos que podrás ver en pantalla y enviar a tu canal de telegram
 *   si lo has configurado, sino, no te preocupes, no pasa nada, todo seguirá funcionando y
 *   minando como siempre.
 *
 *
 *           Un minero de Bitcoin es un dispositivo o software que realiza cálculos
 *           matemáticos complejos para verificar y validar transacciones en la red.
 *           Los mineros compiten para resolver estos problemas y añadir un bloque
 *           a la cadena. A cambio, reciben bitcoins recién creados como recompensa.
 *
 *
 *                              PARA MÁS INFORMACIÓN LEER PDF
 *
 *                     Tmp. De Programación 14H - 6260 Líneas De Código
 *                     ------------------------------------------------
 *
 ********************************************************************************************/

// Invocando las poderosas librerías que hacen posible esta obra maestra del minado nerd

#include "displayDriver.h"
#ifdef T_DISPLAY
#include <TFT_eSPI.h>
#include "media/images_320_170.h"
#include "media/myFonts.h"
#include "media/Free_Fonts.h"
#include "version.h"
#include <inttypes.h>
#include "moonPhase.h"
#include <FS.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <HTTPClient.h>
#include <SPIFFS.h>
#include <Arduino.h>
#include <stdint.h>
#include <qrcode.h>
#include <urlencode.h>
#include <limits.h>
#include <stdio.h>
#include "monitor.h"
#include <math.h>
#include "time.h"
#include <SPI.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <esp_system.h>
#include <stdlib.h>
#include "OpenFontRender.h"
#include "rotation.h"
#include <string>
#include <ctime>
#include <clientntp.h>
#include "drivers/storage/storage.h"
#include "drivers/devices/device.h"

// Variables externas que cruzan fronteras en el código para hacer magia

extern TSettings Settings;

// Definiciones que configuran el cerebro del sistema

#define WIDTH 340
#define HEIGHT 170
#define MIN_KH 0
#define MAX_KH 360
#define MAX_RESULT_LENGTH 500
#define MAX_NUMBERS 6
#define MAX_DEPTH 4
#define MAX_GEN_COUNT 1000
#define GRIDX 320
#define GRIDY 170
#define CELLXY 2
#define GEN_DELAY 1

// Declaración de variables globales

OpenFontRender render;
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite background = TFT_eSprite(&tft);
uint8_t grid[GRIDX][GRIDY];
uint8_t newgrid[GRIDX][GRIDY];
uint16_t genCount = 0;
uint16_t colors[] = {TFT_WHITE, TFT_RED, TFT_GREEN, TFT_BLUE, TFT_YELLOW, TFT_CYAN, TFT_MAGENTA, TFT_ORANGE, TFT_GREENYELLOW, TFT_PINK, TFT_LIGHTGREY, TFT_SKYBLUE, TFT_OLIVE, TFT_GOLD, TFT_SILVER};
uint16_t coloris[] = {TFT_WHITE, TFT_YELLOW, TFT_CYAN, TFT_GREENYELLOW, TFT_LIGHTGREY, TFT_BLACK, TFT_ORANGE, TFT_GOLD, TFT_SILVER};
int colorrrr = esp_random() % 9;
int colorIndex = 0;
int colorI = 0;
int columna = 0;
int secondCounter = 0;
int random_number = 1;
int limite = 0;
int aciertos = 0;
int fallos = 0;
int totalci = 0;
int mirarTiempo = 0;
int sumatele = 1;
int abortar = 0;
int alertatemp = 0;
int maxtemp = 0;
int mintemp = 1000;
int diadecambios;
float anterBTC = 0.0;
float maxkh = 0.00;
float minkh = 1000.00;
float porcentaje = 0.00;
const char *nombrecillo;
const char *apiUrl = "http://ip-api.com/json/";
const char *serverName = "https://zenquotes.io/api/random";
const char *criptomonedas[] = {
    "BTC-USD",   // Bitcoin
    "ETH-USD",   // Ethereum
    "BNB-USD",   // Binance Coin
    "SOL-USD",   // Solana
    "XRP-USD",   // Ripple
    "ADA-USD",   // Cardano
    "AVAX-USD",  // Avalanche
    "DOGE-USD",  // Dogecoin (Elon Musk)
    "DOT-USD",   // Polkadot
    "LINK-USD",  // Chainlink
    "MATIC-USD", // Polygon (Matic)
    "ATOM-USD",  // Cosmos
    "HBAR-USD",  // Hedera
    "UNI-USD",   // Uniswap
    "ALGO-USD",  // Algorand
    "FET-USD",   // Fetch.AI
    "NEAR-USD",  // Near Protocol (añadido)
    "APT-USD",   // Aptos (añadido)
    "ARB-USD",   // Arbitrum (añadido)
    "TRUMP-USD"  // TrumpCoin
};
const char *ciudades[] = {
    "Nueva York", "Londres", "Paris", "Tokio", "Sidney",
    "Los Angeles", "Beijing", "Moscu", "Delhi", "Buenos Aires",
    "Berlin", "Mexico", "Madrid", "Seul", "Roma",
    "El Cairo", "Amsterdam", "Toronto", "Sao Paulo", "Cape Town"};
const char *urlsm8ax[] = {
    "YT - https://youtube.com/m8ax",
    "OS - https://opensea.io/es/m8ax",
    "OC - https://oncyber.io/m8ax",
    "FW - https://m8ax.github.io/MvIiIaX-NerdMiner_V2-DeV/",
    "GH - https://github.com/m8ax"};
String urls[] = {
    "https://cointelegraph.com/rss",
    "https://es.cointelegraph.com/rss/tag/altcoin",
    "https://es.cointelegraph.com/rss/category/analysis",
    "https://es.cointelegraph.com/rss/tag/regulation",
    "https://es.cointelegraph.com/rss/tag/bitcoin",
    "https://es.cointelegraph.com/rss/tag/blockchain",
    "https://es.cointelegraph.com/rss/tag/ethereum",
    "https://es.cointelegraph.com/rss/category/top-10-cryptocurrencies",
    "https://es.cointelegraph.com/rss/category/market-analysis"};
int zonasHorarias[] = {
    -5, 0, 1, 9, 11, // Nueva York, Londres, Paris, Tokio, Sidney
    -8, 8, 3, 5, -3, // Los Angeles, Beijing, Moscu, Delhi, Buenos Aires
    1, -6, 1, 9, 1,  // Berlin, Mexico, Madrid, Seul, Roma
    2, 1, -5, -3, 2  // El Cairo, Amsterdam, Toronto, Sao Paulo, Cape Town
};
bool esHorarioDeVerano(int mes, int dia);
char result[MAX_RESULT_LENGTH];
String textoFinalm8ax1;
String textoFinalm8ax2;
String textoFinalm8ax3;
String textoFinalm8ax4;
String cadenanoti = "";
String ciudad = "";
String tempciudad = "";
String BOT_TOKEN;
String CHAT_ID;
String subebaja = ". ESPERANDO .";
uint32_t rndnumero = 0;
uint32_t rndnumero2 = 0;
uint32_t actualizarcalen = 0;
uint32_t actuanot = 0;
uint32_t actualizarc = 0;
uint32_t actual = 0;
uint32_t correccion = 0;
uint32_t numfrases = 0;
uint32_t numnotis = 0;
uint32_t ContadorEspecial = 0;
uint32_t uncontadormas = 0;
WiFiUDP udp;
HTTPClient http;
mining_data mineria;
clock_data relojete;
coin_data monedilla;
moonPhase mymoonPhase;

unsigned long lastTelegramEpochTime = 0;       // Guarda el tiempo de la última ejecución (en segundos desde Epoch)
unsigned long startTime = 0;                   // Para guardar Epoch de inicio
const unsigned long interval = 60 * 2 * 60;    // 2 horas en segundos (2 horas * 60 minutos * 60 segundos)
const unsigned long minStartupTime = interval; // Segundos para que no envíe mensaje a telegram si esta configurado, nada más arrancar

typedef struct
{
  int value;
  char operation[500];
} State;

template <typename T, typename T2>

inline T map(T2 val, T2 in_min, T2 in_max, T out_min, T out_max)
{
  return (val - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void tDisplay_Init(void)
{

  // Inicializar el pin 15 para habilitar la alimentación externa de 5V (LilyGo bug)

#ifdef PIN_ENABLE5V
  pinMode(PIN_ENABLE5V, OUTPUT);
  digitalWrite(PIN_ENABLE5V, HIGH);
#endif

  tft.init();
#ifdef LILYGO_S3_T_EMBED
  tft.setRotation(ROTATION_270);
#else
  tft.setRotation(ROTATION_90);
#endif
  tft.setSwapBytes(true);
  background.createSprite(WIDTH, HEIGHT);
  background.setSwapBytes(true);
  render.setDrawer(background);
  render.setLineSpaceRatio(0.9);

  // Cargamos fuente y vemos si se puede leer

  if (render.loadFont(DigitalNumbers, sizeof(DigitalNumbers)))
  {
    Serial.println("M8AX - Error En Carga De Fuente...");
    return;
  }
}

// Cambiamos estado de la pantalla al pulsar el botón 1 y abortar=1, para abortar la actualiazación de la pantalla de precios de criptomonedas

void tDisplay_AlternateScreenState(void)
{
  int screen_state = digitalRead(TFT_BL);
  Serial.println("M8AX - Cambiando Estado De La Pantalla");
  digitalWrite(TFT_BL, !screen_state);
  abortar = 1;
}

// Rotamos la pantalla

void tDisplay_AlternateRotation(void)
{
  tft.setRotation(flipRotation(tft.getRotation()));
}

/**
 * Simula una pantalla de televisión con efectos de barras y diagonales de colores aleatorios.
 *
 * La función genera y muestra en pantalla una serie de barras de colores en diferentes direcciones
 * (horizontales, verticales y diagonales), con un breve retraso entre cada una para crear una animación.
 * Al finalizar, la pantalla se limpia y se muestra la palabra "HOLA" o "HELLO" en un color aleatorio
 * O bien, se muestra "AUPA" o "ALOHA" en función de otro número aleatorio.
 *
 * - El ancho de las barras y la velocidad del efecto se generan aleatoriamente dentro de un rango.
 * - Se utilizan colores aleatorios para cada barra y se eliminan tras un breve retraso.
 * - Finalmente, la pantalla se limpia y se muestra el mensaje "HOLA" o "HELLO" con un tamaño de texto grande.
 * - O bien, se muestra "AUPA" o "ALOHA" en función de otro número aleatorio, (numeroSaludo).
 */

void television()
{
  int barWidth = (esp_random() % (41 - 5)) + 5; // Genera un número entre 5 y 40
  int speed = (esp_random() % (26 - 5)) + 5;    // Genera un número entre 5 y 25

  // Barras horizontales
  for (int y = 0; y < tft.height(); y += barWidth)
  {
    colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
    tft.fillRect(0, y, tft.width(), barWidth, colors[colorI]);
    delay(speed);
    tft.fillRect(0, y, tft.width(), barWidth, TFT_BLACK);
  }

  // Barras verticales
  for (int x = 0; x < tft.width(); x += barWidth)
  {
    colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
    tft.fillRect(x, 0, barWidth, tft.height(), colors[colorI]);
    delay(speed);
    tft.fillRect(x, 0, barWidth, tft.height(), TFT_BLACK);
  }

  // Barras diagonales (de esquina a esquina)
  for (int i = 0; i < tft.width() + tft.height(); i += barWidth)
  {
    colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
    tft.drawLine(i, 0, 0, i, colors[colorI]); // Diagonal de arriba a la izquierda
    delay(speed);
    tft.drawLine(i, 0, 0, i, TFT_BLACK); // Borrar la línea
  }

  for (int i = 0; i < tft.width() + tft.height(); i += barWidth)
  {
    colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
    tft.drawLine(tft.width() - i, tft.height(), tft.width(), tft.height() - i, colors[colorI]); // Diagonal de abajo a la derecha
    delay(speed);
    tft.drawLine(tft.width() - i, tft.height(), tft.width(), tft.height() - i, TFT_BLACK); // Borrar la línea
  }

  // Finalizar con la pantalla en negro
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  int numeroSaludo = (esp_random() % 10) + 1;
  tft.setTextColor(colors[colorI], TFT_BLACK);
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(7);

  if (colorI % 2 == 0)
  {
    tft.setCursor(76, 52);
    (numeroSaludo % 2 == 0) ? tft.print("HOLA") : tft.print("AUPA");
  }
  else
  {
    tft.setCursor(60, 52);
    (numeroSaludo % 2 == 0) ? tft.print("HELLO") : tft.print("ALOHA");
  }

  tft.setTextSize(1);
}

/**
 * Animación de inicio para el ESP32-S3 con pantalla TFT.
 *
 * Esta función genera una animación de bienvenida que dura aproximadamente 5 segundos.
 * Incluye varios efectos visuales antes de mostrar el texto central "M8AX" y "MINADOR DE BTC".
 *
 * Pasos de la animación:
 * 1. Llena la pantalla de negro como fondo inicial.
 * 2. Dibuja círculos concéntricos de colores aleatorios desde el centro, creando un efecto de expansión.
 * 3. Realiza un efecto de destello cambiando la pantalla a diferentes colores rápidamente.
 * 4. Muestra el texto "M8AX" en grande en el centro y debajo "MINADOR DE BTC".
 * 5. Mantiene el mensaje en pantalla por un breve tiempo.
 * 6. Finaliza con la animación television().
 */

void animacionInicio()
{
  tft.fillScreen(TFT_BLACK); // Pantalla en negro
  int centroX = tft.width() / 2;
  int centroY = tft.height() / 2;
  int maxRadio = min(centroX, centroY);

  // Efecto de círculos concéntricos

  for (int r = 5; r < maxRadio; r += 5)
  {
    colorIndex = esp_random() % (sizeof(colors) / sizeof(colors[0]));
    tft.drawCircle(centroX, centroY, r, colors[colorIndex]);
    delay(20);
  }

  // Efecto de destello

  for (int i = 0; i < 3; i++)
  {
    colorIndex = esp_random() % (sizeof(colors) / sizeof(colors[0]));
    tft.fillScreen(colors[colorIndex]);
    delay(50);
    colorIndex = esp_random() % (sizeof(colors) / sizeof(colors[0]));
    tft.fillScreen(colors[colorIndex]);
    delay(50);
  }

  // Mostrar el texto principal

  tft.setTextColor(TFT_BLACK, colors[colorIndex]);
  tft.setTextDatum(MC_DATUM);
  tft.setTextSize(3);
  tft.drawString("M8AX", centroX, centroY - 20);
  tft.setTextSize(2);
  tft.drawString("MINADOR DE BTC", centroX, centroY + 20);
  delay(750); // Mantiene el mensaje en pantalla

  // Finalizar con la animación television

  television();
  delay(1000);
  tft.setTextSize(1);
}

// Funcion para convertir un numero entero a números romanos

String convertirARomanos(int num)
{
  if (num == 0)
  {
    return "CERO";
  }

  String result = "";
  int values[] = {1000, 900, 500, 400, 100, 90, 50, 40, 10, 9, 5, 4, 1};
  String romans[] = {"M", "CM", "D", "CD", "C", "XC", "L", "XL", "X", "IX", "V", "IV", "I"};

  for (int i = 0; i < 13; i++)
  {
    while (num >= values[i])
    {
      result += romans[i];
      num -= values[i];
    }
  }

  return result;
}

// Funcion para obtener el día de la semana, Lun Mar Mie...

std::string obtenerDiaSemana(const std::string &fecha)
{
  const char *diasem[] = {"Dom", "Lun", "Mar", "Mie", "Jue", "Vie", "Sab"};

  int dia = std::stoi(fecha.substr(0, 2));
  int mes = std::stoi(fecha.substr(3, 2)) - 1;
  int anio = std::stoi(fecha.substr(6, 4));

  std::tm timeStruct = {};
  timeStruct.tm_mday = dia;
  timeStruct.tm_mon = mes;
  timeStruct.tm_year = anio - 1900;

  std::mktime(&timeStruct);
  int diaSemana = timeStruct.tm_wday; // 0 = domingo, 1 = lunes, ...

  return diasem[diaSemana];
}

// Funcion para obtener nuestra ip pública

String getPublicIP()
{
  HTTPClient http;
  String publicIP = "";

  http.begin("http://api.ipify.org"); // Servicio que devuelve la IP pública
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK)
  {
    publicIP = http.getString(); // Obtener la respuesta del servidor
  }

  http.end();
  return publicIP;
}

// Función para quitar acentos de una cadena de texto

String quitarAcentos(String str)
{
  str.replace("á", "a");
  str.replace("é", "e");
  str.replace("í", "i");
  str.replace("ó", "o");
  str.replace("ú", "u");
  str.replace("Á", "A");
  str.replace("É", "E");
  str.replace("Í", "I");
  str.replace("Ó", "O");
  str.replace("Ú", "U");
  str.replace("ñ", "n");
  str.replace("Ñ", "N");
  str.replace("¿", ""); // Eliminar signos de interrogación al principio de la frase
  str.replace("¡", ""); // Eliminar signos de exclamación al principio de la frase
  return str;
}

// Funcion para cortar una cadena desde el ultimo guión ( - )

void borrarDesdeUltimoGuion(char *str)
{
  char *pos = strrchr(str, '-'); // Encuentra el último '-'
  if (pos)
  {
    *pos = '\0'; // Corta la cadena en ese punto
  }
}

// Función recursiva para encontrar operaciones matemáticas

void find_operations(int numbers[], int target, State current, State *best, int depth, int used[])
{
  // Si alcanzamos el objetivo, actualizamos el mejor resultado

  if (current.value == target)
  {
    *best = current;
    return;
  }

  // Si hemos usado todos los números o hemos alcanzado la profundidad máxima, verificamos si este resultado es mejor

  if (depth == MAX_DEPTH)
  {
    int diff = abs(current.value - target);
    int mejor_diff = abs(best->value - target);
    if (diff < mejor_diff)
    {
      *best = current;
    }
    return;
  }

  // Iterar sobre todos los números disponibles

  for (int i = 0; i < MAX_NUMBERS; i++)
  {
    if (!used[i])
    {
      used[i] = 1; // Marcar el número como usado

      // Suma
      int new_value = current.value + numbers[i];

      // No se permiten resultados negativos
      if (new_value >= 0)
      {
        State new_state;
        new_state.value = new_value;
        snprintf(new_state.operation, sizeof(new_state.operation), "%s -> (%d + %d) = %d", current.operation, current.value, numbers[i], new_value);
        find_operations(numbers, target, new_state, best, depth + 1, used);
      }

      // Resta (solo si no genera negativos)
      if (current.value - numbers[i] >= 0)
      {
        State new_state;
        new_state.value = current.value - numbers[i];
        snprintf(new_state.operation, sizeof(new_state.operation), "%s -> (%d - %d) = %d", current.operation, current.value, numbers[i], new_state.value);
        find_operations(numbers, target, new_state, best, depth + 1, used);
      }

      // Multiplicación (solo si no es innecesario)
      int mult_value = current.value * numbers[i];

      // Limitar el rango de multiplicación
      if (mult_value >= 0 && mult_value <= target + 100)
      {
        State new_state;
        new_state.value = mult_value;
        snprintf(new_state.operation, sizeof(new_state.operation), "%s -> (%d * %d) = %d", current.operation, current.value, numbers[i], mult_value);
        find_operations(numbers, target, new_state, best, depth + 1, used);
      }

      // División exacta (solo si no genera fracciones)
      if (numbers[i] != 0 && current.value % numbers[i] == 0)
      {
        State new_state;
        new_state.value = current.value / numbers[i];
        snprintf(new_state.operation, sizeof(new_state.operation), "%s -> (%d / %d) = %d", current.operation, current.value, numbers[i], new_state.value);
        find_operations(numbers, target, new_state, best, depth + 1, used);
      }
      used[i] = 0; // Desmarcar el número
    }
  }
}

// Función para calcular las operaciones que transforman un conjunto de números en un valor objetivo.
// Se realiza una búsqueda recursiva para encontrar la mejor combinación de operaciones y números,
// registrando el mejor resultado obtenido. Al final, se devuelve el resultado con un mensaje indicando
// si se alcanzó el valor exacto o no, junto con el porcentaje de aciertos acumulado.

void calculate_operations(int numbers[], int target, char *result)
{
  int used[MAX_NUMBERS] = {0};   // Para rastrear números usados
  State best = {numbers[0], ""}; // Inicializamos con el primer número

  // Empezamos probando con cada número en la lista como punto de inicio
  for (int i = 0; i < MAX_NUMBERS; i++)
  {
    State current;
    current.value = numbers[i]; // Probar cada número como inicio
    snprintf(current.operation, sizeof(current.operation), "Inicio: %d", numbers[i]);

    // Marcar el número actual como usado
    memset(used, 0, sizeof(used));
    used[i] = 1;

    // Buscar las mejores operaciones desde el número actual
    find_operations(numbers, target, current, &best, 1, used);
  }

  // Añadir información sobre si es exacto o no
  snprintf(result, 500, "%s\nResultado: %d", best.operation, best.value);
  // Verificar si se ha alcanzado el objetivo exacto
  totalci = (totalci == 0) ? 1 : totalci;
  porcentaje = (static_cast<float>(aciertos) * 100) / totalci;
  if (best.value == target)
  {
    aciertos++;
    strcat(result, ", ( EXACTO ) -");
    char porcentaje_str[20];
    snprintf(porcentaje_str, sizeof(porcentaje_str), " ( AC %.2f%% )", porcentaje);
    strcat(result, porcentaje_str);
  }
  else
  {
    fallos++;
    int diff = abs(best.value - target);
    char diff_str[50];
    snprintf(diff_str, sizeof(diff_str), ", Dif. - ( %d ) -", diff);
    strcat(result, diff_str);
    char porcentaje_str[20];
    snprintf(porcentaje_str, sizeof(porcentaje_str), " ( AC %.2f%% )", porcentaje);
    strcat(result, porcentaje_str);
  }
  totalci++;
}

// Genera una serie de números aleatorios dentro de un rango especificado
// y los almacena en el arreglo 'numbers[]'.
// 'size' define cuántos números generar, 'min' es el valor mínimo
// y 'max' es el valor máximo del rango.
// Los números generados se distribuyen de forma uniforme en el rango [min, max].

void generate_random_numbers(int numbers[], int size, int min, int max)
{
  for (int i = 0; i < size; i++)
  {
    numbers[i] = min + (esp_random() % (max - min + 1));
  }
}

// Realiza la factorización prima de un número entero positivo 'number'.
// La función descompone el número en factores primos y sus respectivos exponentes.
// Los factores y exponentes se almacenan en dos arreglos: 'factors[]' y 'exponents[]'.
// La cadena de caracteres 'result' contiene la representación de la factorización
// en el formato: "factor1^exponente1 * factor2^exponente2 ..."
// Si el número es primo, la cadena resultante también incluirá "( PRIMO )" al final.

const char *factorize(uint32_t number)
{
  char buffer[20];       // Buffer temporal para formatear factores
  uint32_t factors[16];  // Almacena factores primos
  uint8_t exponents[16]; // Almacena exponentes
  uint8_t count = 0;     // Número de factores encontrados

  // Factorización por 2
  if (number % 2 == 0)
  {
    factors[count] = 2;
    exponents[count] = 0;
    while (number % 2 == 0)
    {
      number >>= 1; // Dividir por 2 usando desplazamiento de bits
      exponents[count]++;
    }
    count++;
  }

  // Factorización por divisores impares
  uint32_t sqrt_num = sqrt(number);
  for (uint32_t divisor = 3; divisor <= sqrt_num; divisor += 2)
  {
    if (number % divisor == 0)
    {
      factors[count] = divisor;
      exponents[count] = 0;
      while (number % divisor == 0)
      {
        number /= divisor;
        exponents[count]++;
      }
      count++;
      sqrt_num = sqrt(number);
    }
  }

  // Si el número restante es primo
  if (number > 1)
  {
    factors[count] = number;
    exponents[count] = 1;
    count++;
  }

  // Construir la cadena de resultado
  result[0] = '\0'; // Inicializar el buffer de resultado
  for (uint8_t i = 0; i < count; i++)
  {
    if (i > 0)
    {
      strcat(result, " * "); // Separador
    }
    if (exponents[i] == 1)
    {
      sprintf(buffer, "%u", factors[i]);
    }
    else
    {
      sprintf(buffer, "%ue%u", factors[i], exponents[i]); // Formato "2e3"
    }
    strcat(result, buffer);
  }

  // Verificar si el número era primo
  if (count == 1 && exponents[0] == 1)
  {
    strcat(result, " ( PRIMO )");
  }
  return result;
}

// Función que envía un texto que contiene estadísticas de minería a tu canal de telegram

void enviarMensajeATelegram(String mensaje)
{
  // Verificar y reconectar WiFi si es necesario
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("M8AX - Reconectando WiFi...");
    WiFi.disconnect();
    WiFi.reconnect();
    delay(5000); // Espera para reconectar

    // Si después del intento sigue sin conexión, salir de la función

    if (WiFi.status() != WL_CONNECTED)
    {
      Serial.println("M8AX - No Se Pudo Reconectar WiFi...");
      return;
    }
  }
  WiFiClientSecure client;
  client.setInsecure();
  String mensajeCodificado = urlEncode(mensaje); // Mirar función
  mensajeCodificado.reserve(mensajeCodificado.length() + 100);
  mensaje = "";
  mensaje.reserve(0);

  // Reemplazar los saltos de línea con %0A

  mensajeCodificado.replace("\n", "%0A");

  Serial.println("M8AX - Enviando Mensaje A Telegram...");

  String url = "https://api.telegram.org/bot" + BOT_TOKEN + "/sendMessage?chat_id=" + CHAT_ID + "&text=" + mensajeCodificado;
  url.reserve(url.length() + 50);
  mensajeCodificado = "";
  mensajeCodificado.reserve(0);

  if (client.connect("api.telegram.org", 443))
  {
    client.print(String("GET ") + url + " HTTP/1.1\r\n" + "Host: api.telegram.org\r\n" + "Connection: close\r\n\r\n");
    delay(1500); // Espera para dar tiempo a la respuesta
    Serial.println("M8AX - Mensaje Enviado A Telegram...");
    client.flush();
    client.stop();
    sumatele += 1;
    url = "";
    url.reserve(0);
  }
  else
  {
    Serial.println("M8AX - Error De Conexión, Mensaje A Telegram Falló...");
    client.flush();
    client.stop();
    url = "";
    url.reserve(0);
  }
}

// Función para recopilar todo lo que vamos a enviar a nuestro canal de telegram
// Se ha añadido el envío del RSSI de la señal WiFi

void recopilaTelegram()
{
  unsigned long epochTime = timeClient.getEpochTime(); // Obtener segundos desde 1970
  time_t epoch = (time_t)epochTime;                    // Convertir a time_t
  struct tm *timeinfo = localtime(&epoch);             // Convertir a una estructura de tiempo local
  int dia = timeinfo->tm_mday;                         // Día del mes (1 a 31)
  int mes = timeinfo->tm_mon + 1;                      // Mes (1 a 12)
  int anio = timeinfo->tm_year + 1900;                 // Año (por defecto es desde 1900)
  int horita = timeinfo->tm_hour;                      // Hora
  int minutitos = timeinfo->tm_min;                    // Minutos
  int segundos = timeinfo->tm_sec;                     // Segundos
  int indice = esp_random() % 5;

  // Formatear la hora en "00:00:00"
  char horaFormateada[9];
  sprintf(horaFormateada, "%02d:%02d:%02d", horita, minutitos, segundos);

  // Formatear la fecha en "dia/mes/año"
  char fechaFormateada[11];
  sprintf(fechaFormateada, "%02d/%02d/%04d", dia, mes, anio);

  // Extraer los últimos 4 dígitos de la mac
  uint8_t mac[6];
  WiFi.macAddress(mac);
  char u4digits[5];
  sprintf(u4digits, "%02X%02X", mac[4], mac[5]);

  std::string quediase = obtenerDiaSemana(std::string(fechaFormateada));
  String telrb = monedilla.remainingBlocks;
  String cadenaEnvio;
  cadenaEnvio.reserve(5000);
  cadenaEnvio = "";
  cadenaEnvio = F("------------------------------------------------------------------------------------------------\n");
  cadenaEnvio += "------------------------ M8AX - NerdMinerV2-" + String(u4digits) + " DATOS DE MINERÍA - M8AX -----------------------\n";
  cadenaEnvio += F("------------------------------------------------------------------------------------------------\n");
  cadenaEnvio += "----------------------------------- " + String(fechaFormateada) + " " + quediase.c_str() + " - " + horaFormateada + " ----------------------------------\n";
  cadenaEnvio += F("------------------------------------------------------------------------------------------------\n");
  quediase.clear();
  quediase.shrink_to_fit();
  if (sumatele <= 3999)
  {
    cadenaEnvio += "Mensaje Número - " + convertirARomanos(sumatele) + "\n";
  }
  else
  {
    cadenaEnvio += "Mensaje Número - " + String(sumatele) + "\n";
  }
  cadenaEnvio += "Tiempo Minando - " + (mineria.timeMining.substring(0, mineria.timeMining.indexOf(" ")).length() == 1 ? "0" + mineria.timeMining.substring(0, mineria.timeMining.indexOf(" ")) : mineria.timeMining.substring(0, mineria.timeMining.indexOf(" "))) + " Días" + mineria.timeMining.substring(mineria.timeMining.indexOf(" ") + 1) + "\n";
  cadenaEnvio += "HR Actual - " + mineria.currentHashRate + " KH/s ( MAX - " + String(maxkh) + " | MIN - " + String(minkh) + " )\n";
  cadenaEnvio += "Temp. De CPU - " + mineria.temp + "° ( MAX - " + String(maxtemp) + "° | MIN - " + String(mintemp) + "° | TMP>70° - " + String(alertatemp) + " )\n";
  cadenaEnvio += "Plantillas De Bloque - " + mineria.templates + "\n";
  cadenaEnvio += "Shares Enviados A La Pool - " + mineria.completedShares + "\n";
  cadenaEnvio += "Mejor Dificultad Alcanzada - " + mineria.bestDiff + "\n";
  cadenaEnvio += "Dificultad De La Red - " + monedilla.netwrokDifficulty + "\n";
  cadenaEnvio += "Cómputo Total - " + mineria.totalKHashes + " KH - ( " + String(atof(mineria.totalKHashes.c_str()) / 1000, 3) + " MH )\n";
  cadenaEnvio += "HR Global - " + monedilla.globalHashRate + " EH/s\n";
  cadenaEnvio += "Precio De BTC - " + monedilla.btcPrice + " | En 24H -> " + subebaja + " |\n";
  cadenaEnvio += "FEE Promedio Por TX - " + monedilla.halfHourFee + "\n";
  cadenaEnvio += "Altura De Bloque - " + relojete.blockHeight + "\n";
  telrb.replace("BLOCKS", "");
  cadenaEnvio += "Bloques Entre Halvings - 210000\n";
  cadenaEnvio += "Bloques Hasta El Halving - " + String(telrb) + "\n";
  long int hechos = 210000 - telrb.toInt();
  cadenaEnvio += "Bloques Minados Post-Halving - " + String(hechos) + "\n";
  char buffer[10];
  dtostrf((hechos * 100.0) / 210000.0, 0, 3, buffer); // Convierte float a string con 3 decimales
  cadenaEnvio += "% Completado Desde El Último Halving - " + String(buffer) + "%\n";
  cadenaEnvio += "Pool De Minería - " + Settings.PoolAddress + "\n";
  cadenaEnvio += "Puerto Del Pool - " + String(Settings.PoolPort) + "\n";
  cadenaEnvio += "Tu Wallet De BTC - " + String(Settings.BtcWallet) + "\n";
  cadenaEnvio += "Tu IP - " + getPublicIP() + " | WiFi RSSI " + String(WiFi.RSSI()) + "\n";
  cadenaEnvio += urlsm8ax[indice];
  cadenaEnvio += F("\n------------------------------------------------------------------------------------------------\n");
  if (mineria.valids.toInt() == 1)
  {
    cadenaEnvio += "||| ¡ BLOQUE MINADO ! ¡ A COBRAR ! :) |||\n";
  }
  else
  {
    cadenaEnvio += "||| ¡ SIN PASTA, SIN GLORIA ! ¡ A SEGUIR CON LA HISTORIA ! |||\n";
  }
  cadenaEnvio += F("------------------------------------------------------------------------------------------------\n");
  cadenaEnvio += F("---------------------------------- M8AX - DATOS NERD - M8AX ------------------------------------\n");
  cadenaEnvio += F("------------------------------------------------------------------------------------------------\n");
  rndnumero = esp_random();
  cadenaEnvio += "Factorización De Número - " + String(rndnumero) + " -> " + factorize(rndnumero) + "\n";
  cadenaEnvio += "------------------------------------------------------------------------------------------------\n                                       By M8AX Corp. " + convertirARomanos(anio);
  cadenaEnvio += F("\n------------------------------------------------------------------------------------------------");
  enviarMensajeATelegram(cadenaEnvio);
  cadenaEnvio = "";
  cadenaEnvio.reserve(0);
}

// Función para imprimir datos en pantalla en formato texto plano, todo seguido con información
// Se añaden todos los datos disponibles para mostrar + RSSI de la red WiFi

void datosPantallaTextoPlano()
{
  unsigned long epochTime = timeClient.getEpochTime(); // Obtener segundos desde 1970
  time_t epoch = (time_t)epochTime;                    // Convertir a time_t
  struct tm *timeinfo = localtime(&epoch);             // Convertir a una estructura de tiempo local
  int dia = timeinfo->tm_mday;                         // Día del mes (1 a 31)
  int mes = timeinfo->tm_mon + 1;                      // Mes (1 a 12)
  int anio = timeinfo->tm_year + 1900;                 // Año (por defecto es desde 1900)
  int horita = timeinfo->tm_hour;                      // Hora
  int minutitos = timeinfo->tm_min;                    // Minutos
  int segundos = timeinfo->tm_sec;                     // Segundos
  String telrb = monedilla.remainingBlocks;
  String cadenaEnvio2;
  cadenaEnvio2.reserve(5000);
  cadenaEnvio2 = "";
  cadenaEnvio2 += relojete.currentDate + " - " + relojete.currentTime;
  cadenaEnvio2 += ". WiFi RSSI " + String(WiFi.RSSI());
  cadenaEnvio2 += ". Tiempo Minando - " + (mineria.timeMining.substring(0, mineria.timeMining.indexOf(" ")).length() == 1 ? "0" + mineria.timeMining.substring(0, mineria.timeMining.indexOf(" ")) : mineria.timeMining.substring(0, mineria.timeMining.indexOf(" "))) + " Días" + mineria.timeMining.substring(mineria.timeMining.indexOf(" ") + 1);
  cadenaEnvio2 += ". HR Actual - " + mineria.currentHashRate + " KH/s ( MAX - " + String(maxkh) + " | MIN - " + String(minkh) + " )";
  cadenaEnvio2 += ". Temp. De CPU - " + mineria.temp + "g ( MAX - " + String(maxtemp) + "g | MIN - " + String(mintemp) + "g | TMP>70° - " + String(alertatemp) + " )";
  cadenaEnvio2 += ". Plantillas De Bloque - " + mineria.templates;
  cadenaEnvio2 += ". Shares Enviados A La Pool - " + mineria.completedShares;
  cadenaEnvio2 += ". Mejor Dificultad Alcanzada - " + mineria.bestDiff;
  cadenaEnvio2 += ". Dificultad De La Red - " + monedilla.netwrokDifficulty;
  cadenaEnvio2 += ". Cómputo Total - " + mineria.totalKHashes + " KH - ( " + String(atof(mineria.totalKHashes.c_str()) / 1000, 3) + " MH )";
  cadenaEnvio2 += ". HR Global - " + monedilla.globalHashRate + " EH/s";
  cadenaEnvio2 += ". Precio De BTC - " + monedilla.btcPrice + " | En 24H -> " + subebaja + " |\n";
  cadenaEnvio2 += ". FEE Promedio Por TX - " + monedilla.halfHourFee;
  cadenaEnvio2 += ". Altura De Bloque - " + relojete.blockHeight;
  telrb.replace("BLOCKS", "");
  cadenaEnvio2 += ". Bloques Entre Halvings - 210000";
  cadenaEnvio2 += ". Bloques Hasta El Halving - " + String(telrb);
  long int hechos = 210000 - telrb.toInt();
  cadenaEnvio2 += ". Bloques Minados Post-Halving - " + String(hechos);
  char buffer[10];
  dtostrf((hechos * 100.0) / 210000.0, 0, 3, buffer); // Convierte float a string con 5 decimales
  cadenaEnvio2 += ". % Completado Desde El Último Halving - " + String(buffer) + "%";
  cadenaEnvio2 += ". % Restante Para Próximo Halving - " + String(100.000 - round(atof(buffer) * 1000) / 1000, 3) + "%";
  if (mineria.valids.toInt() == 1)
  {
    cadenaEnvio2 += ". ||| BLOQUE MINADO, A COBRAR xD |||";
  }
  else
  {
    cadenaEnvio2 += ". ||| SIN PASTA, SIN GLORIA, A SEGUIR CON LA HISTORIA... |||";
  }
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(1, 1);
  tft.setTextSize(2);
  tft.print("   DATOS EN TEXTO PLANO");
  tft.setCursor(1, 22);
  tft.setTextSize(1);
  tft.setTextColor(colors[colorIndex]);
  tft.print(quitarAcentos(cadenaEnvio2));
  cadenaEnvio2 = "";
  cadenaEnvio2.reserve(0);
}

// Función para obterner la ciudad y la temperatura mediante la IP pública

std::pair<String, String> obtenerCiudadYTemperatura(const String &ip)
{
  String ciudad = "";
  String temperatura = "";
  String latitud = "";
  String longitud = "";

  // Obtener la ciudad, latitud y longitud usando la API de geolocalización
  HTTPClient http;
  String urlGeo = "http://ip-api.com/json/" + ip + "?fields=city,lat,lon"; // Obtener ciudad, latitud y longitud
  http.begin(urlGeo);

  int httpCode = http.GET();
  if (httpCode > 0)
  {
    String payload = http.getString();

    // Parsear el JSON para obtener la ciudad y coordenadas
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, payload);
    payload = "";
    payload.reserve(0);
    if (error)
    {
      Serial.println("M8AX - Error Al Parsear JSON De Geolocalización");
      return std::make_pair(ciudad, temperatura); // Devolver vacías en caso de error
    }
    ciudad = doc["city"].as<String>();           // Ciudad obtenida
    latitud = String(doc["lat"].as<float>(), 6); // Convertir a string con 6 decimales
    longitud = String(doc["lon"].as<float>(), 6);
  }
  else
  {
    Serial.println("M8AX - Error Al Obtener Datos De Geolocalización");
    return std::make_pair(ciudad, temperatura); // Devolver vacías si hay error
  }

  http.end();

  // Obtener la temperatura usando wttr.in con latitud y longitud
  if (latitud != "" && longitud != "")
  {
    // Usamos WiFiClientSecure para HTTPS
    WiFiClientSecure client;
    client.setInsecure(); // Permitir conexiones HTTPS sin verificar certificado

    String urlTemp = "https://wttr.in/" + latitud + "," + longitud + "?format=%t"; // Solicitar temperatura con coordenadas
    http.begin(client, urlTemp);                                                   // Iniciar la solicitud HTTPS
    httpCode = http.GET();
    urlTemp.reserve(0);

    if (httpCode > 0)
    {
      temperatura = http.getString(); // Obtener la temperatura
    }
    else
    {
      Serial.println("M8AX - Error Al Obtener Temperatura");
    }
    http.end();
  }
  else
  {
    Serial.println("M8AX - Coordenadas No Encontradas Para La IP");
  }

  return std::make_pair(ciudad, temperatura); // Devolver ciudad y temperatura
}

// Dibuja una cuadrícula en la pantalla TFT, actualizando las celdas cuya
// condición ha cambiado entre el estado anterior (grid) y el nuevo (newgrid).
// Cada celda se dibuja con el color correspondiente (blanco si el valor es 1,
// negro si el valor es 0), utilizando el tamaño de celda definido por 'CELLXY'.
// La función recorre todas las celdas de la cuadrícula y actualiza las que han cambiado.

void drawGrid(void)
{
  uint16_t color = color = TFT_WHITE;
  for (int16_t x = 1; x < GRIDX - 1; x++)
  {
    for (int16_t y = 1; y < GRIDY - 1; y++)
    {
      if ((grid[x][y]) != (newgrid[x][y]))
      {
        if (newgrid[x][y] == 1)
          color = 0xFFFF; // random(0xFFFF);
        else
          color = 0;
        tft.fillRect(CELLXY * x, CELLXY * y, CELLXY, CELLXY, color);
      }
    }
  }
}

// Calcula y devuelve el número de vecinos activos (celdas con valor 1)
// alrededor de una celda ubicada en las coordenadas (x, y) en la cuadrícula.
// La función considera las 8 celdas adyacentes (incluyendo diagonales) y suma
// el valor de cada celda vecina (1 si está activa, 0 si no lo está).

int getNumberOfNeighbors(int x, int y)
{
  return grid[x - 1][y] + grid[x - 1][y - 1] + grid[x][y - 1] + grid[x + 1][y - 1] + grid[x + 1][y] + grid[x + 1][y + 1] + grid[x][y + 1] + grid[x - 1][y + 1];
}

// Inicializa la cuadrícula 'grid' y 'newgrid'. La cuadrícula está compuesta por
// celdas activas (valor 1) e inactivas (valor 0). Las celdas en los bordes
// están inactivas, mientras que las celdas internas se asignan aleatoriamente como
// activas con una probabilidad de 1/3. 'newgrid' se inicializa a 0 en todas sus celdas.

void initGrid(void)
{
  for (int16_t x = 0; x < GRIDX; x++)
  {
    for (int16_t y = 0; y < GRIDY; y++)
    {
      newgrid[x][y] = 0;
      if (x == 0 || x == GRIDX - 1 || y == 0 || y == GRIDY - 1)
      {
        grid[x][y] = 0;
      }
      else
      {
        if (esp_random() % 3 == 1)
          grid[x][y] = 1;
        else
          grid[x][y] = 0;
      }
    }
  }
}

// Calcula el siguiente estado de la cuadrícula en base a las reglas del
// autómata celular. Para cada celda de la cuadrícula, se evalúa el número
// de vecinos activos (1). Si la celda está activa (1) y tiene 2 o 3 vecinos,
// permanece activa en el siguiente paso. Si tiene menos de 2 o más de 3 vecinos,
// se desactiva. Si la celda está inactiva (0) y tiene exactamente 3 vecinos,
// se activa en el siguiente paso. El resultado se almacena en 'newgrid'.

void computeCA()
{
  for (int16_t x = 1; x < GRIDX; x++)
  {
    for (int16_t y = 1; y < GRIDY; y++)
    {
      int neighbors = getNumberOfNeighbors(x, y);
      if (grid[x][y] == 1 && (neighbors == 2 || neighbors == 3))
      {
        newgrid[x][y] = 1;
      }
      else if (grid[x][y] == 1)
        newgrid[x][y] = 0;
      if (grid[x][y] == 0 && (neighbors == 3))
      {
        newgrid[x][y] = 1;
      }
      else if (grid[x][y] == 0)
        newgrid[x][y] = 0;
    }
  }
}

// Función que recibe una palabra y la devuelve con la primera letra en mayúscula

String capitalizar(String palabra)
{
  if (palabra.length() > 0)
  {
    palabra[0] = toupper(palabra[0]);
  }
  return palabra;
}

// Función que recibe un número entero y lo devuelve en formato cadena y escrito en letra

String numeroAEscrito(int num, bool esDecimal = false)
{
  if (num < 0 || num > 9999)
    return "Número Fuera De Rango";

  String unidades[] = {"cero", "uno", "dos", "tres", "cuatro", "cinco", "seis", "siete", "ocho", "nueve"};
  String especiales[] = {"Diez", "Once", "Doce", "Trece", "Catorce", "Quince",
                         "Dieciséis", "Diecisiete", "Dieciocho", "Diecinueve"};
  String decenas[] = {"", "", "Veinte", "Treinta", "Cuarenta", "Cincuenta",
                      "Sesenta", "Setenta", "Ochenta", "Noventa"};
  String centenas[] = {"", "Ciento", "Doscientos", "Trescientos", "Cuatrocientos",
                       "Quinientos", "Seiscientos", "Setecientos", "Ochocientos", "Novecientos"};

  if (num == 100)
    return "Cien"; // Caso especial
  if (num == 0)
    return "Cero";

  String resultado = "";

  // Si es la parte decimal y el número es menor que 10, agregar "Cero"

  if (esDecimal && num < 10)
  {
    resultado += "Cero ";
  }

  // Miles

  if (num >= 1000)
  {
    if (num / 1000 == 1)
    {
      resultado += "Mil";
    }
    else
    {
      resultado += capitalizar(unidades[num / 1000]) + " Mil";
    }
    num %= 1000; // Eliminamos los miles
    if (num > 0)
      resultado += " ";
  }

  // Centenas

  if (num >= 100)
  {
    resultado += capitalizar(centenas[num / 100]);
    num %= 100; // Eliminamos las centenas
    if (num > 0)
      resultado += " ";
  }

  // Decenas y unidades

  if (num >= 10 && num <= 19)
  {
    resultado += capitalizar(especiales[num - 10]); // Números entre 10 y 19
  }
  else
  {
    if (num >= 20)
    {
      // Para números del 20 al 29, usamos la forma correcta "Veinti" + unidades

      if (num < 30)
      {
        resultado += "Veinti";
        if (num % 10 > 0)
          resultado += unidades[num % 10]; // "VeintiUno", "VeintiTres", etc.
      }
      else
      {
        resultado += capitalizar(decenas[num / 10]);
        if (num % 10 > 0)
          resultado += " Y " + capitalizar(unidades[num % 10]); // "Treinta Y Uno", "Cuarenta Y Cinco"
      }
    }
    else if (num > 0)
    {
      resultado += capitalizar(unidades[num]); // Unidades solas
    }
  }

  return resultado;
}

/**
 * Dibuja un dado en la pantalla TFT en la posición especificada.
 *
 * @param numero Número del dado a dibujar (1 a 6).
 * @param x Coordenada X del centro del dado.
 * @param y Coordenada Y del centro del dado.
 *
 * La función dibuja un cuadrado representando el dado y coloca los puntos según el número indicado.
 * - El dado tiene un tamaño de 60x60 píxeles, centrado en (x, y).
 * - Los puntos se dibujan con un radio de 5 píxeles.
 * - Se usa `fillCircle` para dibujar los puntos en la posición correcta según el número del dado.
 * - Se verifica cada número de 1 a 6 para posicionar los puntos correctamente.
 */

void dibujarDado(int numero, int x, int y)
{
  // Dibuja el borde del dado
  tft.drawRect(x - 30, y - 30, 60, 60, TFT_WHITE); // Cuadrado del dado

  int r = 5; // Radio de los puntos

  // Posiciones de los puntos del dado según el número (del 1 al 6)
  // Definimos los puntos en un arreglo para que siempre se muestren correctamente
  switch (numero)
  {
  case 1:
    // Un solo punto en el centro
    tft.fillCircle(x, y, r, TFT_WHITE);
    break;
  case 2:
    // Dos puntos en esquinas opuestas
    tft.fillCircle(x - 15, y - 15, r, TFT_WHITE);
    tft.fillCircle(x + 15, y + 15, r, TFT_WHITE);
    break;
  case 3:
    // Tres puntos: dos en las esquinas y uno en el centro
    tft.fillCircle(x, y, r, TFT_WHITE);
    tft.fillCircle(x - 15, y - 15, r, TFT_WHITE);
    tft.fillCircle(x + 15, y + 15, r, TFT_WHITE);
    break;
  case 4:
    // Cuatro puntos: en las esquinas
    tft.fillCircle(x - 15, y - 15, r, TFT_WHITE);
    tft.fillCircle(x + 15, y - 15, r, TFT_WHITE);
    tft.fillCircle(x - 15, y + 15, r, TFT_WHITE);
    tft.fillCircle(x + 15, y + 15, r, TFT_WHITE);
    break;
  case 5:
    // Cinco puntos: cuatro en las esquinas y uno en el centro
    tft.fillCircle(x, y, r, TFT_WHITE);
    tft.fillCircle(x - 15, y - 15, r, TFT_WHITE);
    tft.fillCircle(x + 15, y - 15, r, TFT_WHITE);
    tft.fillCircle(x - 15, y + 15, r, TFT_WHITE);
    tft.fillCircle(x + 15, y + 15, r, TFT_WHITE);
    break;
  case 6:
    // Seis puntos: en filas de 3
    tft.fillCircle(x - 15, y - 15, r, TFT_WHITE);
    tft.fillCircle(x + 15, y - 15, r, TFT_WHITE);
    tft.fillCircle(x - 15, y, r, TFT_WHITE);
    tft.fillCircle(x + 15, y, r, TFT_WHITE);
    tft.fillCircle(x - 15, y + 15, r, TFT_WHITE);
    tft.fillCircle(x + 15, y + 15, r, TFT_WHITE);
    break;
  }
}

/**
 * Dibuja una moneda en la pantalla TFT en la posición especificada.
 *
 * @param moneda Valor de la moneda: 0 para "Cara", 1 para "Cruz".
 * @param x Coordenada X del centro de la moneda.
 * @param y Coordenada Y del centro de la moneda.
 *
 * La función representa una moneda con:
 * - Un círculo de radio 45 píxeles para la forma de la moneda.
 * - Un texto "C" en color verde si la moneda muestra "Cara".
 * - Un texto "X" en color rojo si la moneda muestra "Cruz".
 * - El texto se posiciona centrado dentro de la moneda.
 */

void dibujarMoneda(int moneda, int x, int y)
{
  int radio = 45; // Un tamaño más grande para la moneda

  tft.drawCircle(x, y, radio, TFT_WHITE); // Dibujar la moneda

  if (moneda == 0)
  {
    tft.setTextColor(TFT_GREEN);
    tft.setTextSize(2);
    tft.drawCentreString("C", x, y - 20, 4); // Dibuja "C" para cara
  }
  else
  {
    tft.setTextColor(TFT_RED);
    tft.setTextSize(2);
    tft.drawCentreString("X", x, y - 20, 4); // Dibuja "X" para cruz
  }
}

/**
 * Dibuja un texto centrado en la pantalla con efecto de escritura letra por letra.
 *
 * La función calcula el ancho del texto y lo centra horizontalmente en la pantalla.
 * Luego, dibuja cada letra una por una con un pequeño retardo para generar un efecto
 * de escritura progresiva. Cada letra se muestra con un color aleatorio.
 *
 * @param text       Cadena de texto a mostrar.
 * @param y          Coordenada Y donde se dibujará el texto.
 * @param delayTime  Tiempo de espera (en milisegundos) entre cada letra.
 */

void drawCenteredText(const char *text, int y, int delayTime)
{
  int screenWidth = tft.width();         // Ancho de la pantalla
  int textWidth = tft.textWidth(text);   // Ancho del texto
  int x = (screenWidth - textWidth) / 5; // Posición X para centrar el texto

  // Efecto de escritura letra por letra
  for (int i = 0; i < strlen(text); i++)
  {
    colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
    tft.setTextColor(colors[colorI], TFT_BLACK);
    tft.drawChar(text[i], x, y, 2);          // Dibuja una letra
    x += tft.textWidth(String(text[i])) + 5; // Ajusta la posición X para la siguiente letra
    delay(delayTime);                        // Retardo para el efecto
  }
}

/**
 * Simula una animación de nieve cayendo en la pantalla.
 *
 * La función dibuja copos de nieve que caen desde posiciones aleatorias en la parte superior
 * de la pantalla y los mueve hacia abajo con velocidades aleatorias. Cuando un copo alcanza
 * la parte inferior, reaparece en la parte superior en una nueva posición horizontal.
 *
 * - Se generan 100 copos de nieve con posiciones iniciales aleatorias.
 * - La animación dura 5 segundos y se actualiza cada 35 ms.
 * - Al finalizar, se muestra el mensaje "FELIZ NAVIDAD" centrado en la pantalla con un color aleatorio.
 */

void nevar()
{
  tft.fillScreen(TFT_BLACK); // Fondo negro (puedes cambiarlo)

  const int NUM_COPOS = 100;      // Número de copos de nieve en pantalla
  int x[NUM_COPOS], y[NUM_COPOS]; // Coordenadas de los copos

  // Inicializa copos en posiciones aleatorias
  for (int i = 0; i < NUM_COPOS; i++)
  {
    x[i] = (esp_random() % 320); // Genera un número entre 0 y 319
    y[i] = (esp_random() % 170); // Genera un número entre 0 y 169
  }

  unsigned long startTime = millis();

  while (millis() - startTime < 5000)
  {                            // 5 segundos
    tft.fillScreen(TFT_BLACK); // Borra pantalla

    for (int i = 0; i < NUM_COPOS; i++)
    {
      tft.drawPixel(x[i], y[i], TFT_WHITE); // Dibuja copo de nieve
      y[i] += (esp_random() % 4) + 1;       // Baja la posición del copo

      if (y[i] > 170)
      { // Si sale de la pantalla, reaparece arriba
        y[i] = 0;
        x[i] = esp_random() % 320;
      }
    }

    delay(35); // Controla la velocidad de la animación
  }

  // Muestra "FELIZ NAVIDAD" al final
  tft.fillScreen(TFT_BLACK);
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  tft.setTextColor(colors[colorI], TFT_BLACK);
  tft.setTextDatum(MC_DATUM);               // Centra el texto
  tft.setFreeFont(FSB18);                   // Fuente grande (cambia si es necesario)
  tft.drawString("FELIZ NAVIDAD", 160, 85); // Texto en el centro
  delay(1500);
  tft.setFreeFont(NULL);
}

/**
 * Simula una animación de nieve cayendo con mayor densidad y velocidad.
 *
 * Esta función es una versión mejorada de "nevar()", aumentando el número de copos a 200
 * y ajustando la velocidad de caída para un efecto más dinámico.
 *
 * - Se generan 200 copos de nieve en posiciones aleatorias dentro de la pantalla.
 * - La animación dura 5 segundos, con actualizaciones cada 25 ms para una caída más rápida.
 * - Al finalizar, muestra el mensaje "HAPPY NEW YEAR" centrado en la pantalla con un color aleatorio.
 */

void nevar2()
{
  tft.fillScreen(TFT_BLACK); // Fondo negro (puedes cambiarlo)

  const int NUM_COPOS = 200;      // Número de copos de nieve en pantalla
  int x[NUM_COPOS], y[NUM_COPOS]; // Coordenadas de los copos

  // Inicializa copos en posiciones aleatorias
  for (int i = 0; i < NUM_COPOS; i++)
  {
    x[i] = esp_random() % 320;
    y[i] = esp_random() % 170;
  }

  unsigned long startTime = millis();

  while (millis() - startTime < 5000)
  {                            // 5 segundos
    tft.fillScreen(TFT_BLACK); // Borra pantalla

    for (int i = 0; i < NUM_COPOS; i++)
    {
      tft.drawPixel(x[i], y[i], TFT_WHITE); // Dibuja copo de nieve
      y[i] += (esp_random() % 5) + 1;       // Baja Posición Del Copo
      if (y[i] > 170)
      { // Si sale de la pantalla, reaparece arriba
        y[i] = 0;
        x[i] = esp_random() % 320;
      }
    }

    delay(25); // Controla la velocidad de la animación
  }

  // Muestra "FELIZ --- AÑO" al final
  tft.fillScreen(TFT_BLACK);
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  tft.setTextColor(colors[colorI], TFT_BLACK);
  tft.setTextDatum(MC_DATUM);                // Centra el texto
  tft.setFreeFont(FSB18);                    // Fuente grande (cambia si es necesario)
  tft.drawString("HAPPY NEW YEAR", 160, 85); // Texto en el centro
  delay(1500);
  tft.setFreeFont(NULL);
}

/**
 * Muestra un mensaje animado y ejecuta un efecto de televisión.
 *
 * - Borra la pantalla y selecciona un color aleatorio para el texto.
 * - Muestra tres líneas de texto centradas con un efecto de escritura.
 * - Espera 500 ms y luego ejecuta la animación "television()".
 * - Tras la animación, espera 1 segundo y restablece el tamaño del texto a 1.
 */

void M8AXTicker()
{
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(colors[colorI], TFT_BLACK);
  tft.setTextSize(2);
  drawCenteredText("M v I i I a X", 30, 100);
  drawCenteredText("NerdMiner V2", 70, 100);
  drawCenteredText("V  10 . 03 . 77", 110, 100);
  delay(500);
  television();
  delay(1000);
  tft.setTextSize(1);
}

/**
 * Simula una nevada animada en la pantalla con colores aleatorios.
 *
 * - Llena la pantalla de negro como fondo inicial.
 * - Genera 500 copos de nieve con posiciones aleatorias.
 * - Durante 2 segundos, los copos caen con velocidades aleatorias y se
 *   regeneran en la parte superior cuando salen de la pantalla.
 * - Cada copo de nieve se dibuja con un color aleatorio.
 * - La velocidad de la animación varía aleatoriamente en cada iteración.
 * - Al finalizar, se ejecuta la función M8AXTicker().
 */

void nevar3()
{
  tft.fillScreen(TFT_BLACK); // Fondo negro (puedes cambiarlo)

  const int NUM_COPOS = 500;      // Número de copos de nieve en pantalla
  int x[NUM_COPOS], y[NUM_COPOS]; // Coordenadas de los copos

  // Inicializa copos en posiciones aleatorias
  for (int i = 0; i < NUM_COPOS; i++)
  {
    x[i] = esp_random() % 320;
    y[i] = esp_random() % 170;
  }

  unsigned long startTime = millis();

  while (millis() - startTime < 2000)
  {                            // 5 segundos
    tft.fillScreen(TFT_BLACK); // Borra pantalla
    for (int i = 0; i < NUM_COPOS; i++)
    {
      colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
      tft.drawPixel(x[i], y[i], colors[colorI]); // Dibuja copo de nieve
      y[i] += (esp_random() % 5) + 1;            // Baja La Posición DeL Copo

      if (y[i] > 170)
      { // Si sale de la pantalla, reaparece arriba
        y[i] = 0;
        x[i] = esp_random() % 320;
      }
    }
    int nnumeroAleatorio = esp_random() % 20 + 1;
    delay(nnumeroAleatorio); // Controla la velocidad de la animación
  }
  M8AXTicker();
}

/**
 * Simula el cierre de unas cortinas en la pantalla.
 *
 * - Dibuja dos líneas verticales negras que se alejan del centro de la pantalla.
 * - La animación comienza desde el centro de la pantalla y las líneas se expanden hacia los bordes.
 * - Utiliza líneas verticales para dar el efecto de cortinas cerrándose.
 * - La velocidad de la animación se controla con un retraso de 10 ms entre cada paso.
 */

void cortinas2()
{
  int centerX = tft.width() / 2;  // Centro de la pantalla en X
  int centerY = tft.height() / 2; // Centro de la pantalla en Y

  for (int i = 0; i <= centerX; i++)
  {
    // Dibuja líneas verticales que se alejan del centro
    tft.drawFastVLine(centerX - i, 0, tft.height(), TFT_BLACK); // Izquierda
    tft.drawFastVLine(centerX + i, 0, tft.height(), TFT_BLACK); // Derecha
    delay(10);                                                  // Ajusta la velocidad de la animación
  }
}

/**
 * Muestra una animación de cortinas cerrándose y luego muestra información de la versión del software.
 *
 * - Llame a la función `cortinas2()` para crear el efecto de cortinas que se cierran en la pantalla.
 * - Después de la animación de las cortinas, se muestran textos centrados en la pantalla con información:
 *   - "M 8 A X"
 *   - "NerdMiner V2"
 *   - "V 10 . 03 . 77"
 * - Luego de mostrar el texto, hay un pequeño retraso de 500 ms y después se ejecuta la función `television()`.
 * - Después de 1000 ms, restablece el tamaño del texto a 1.
 */

void cortinas()
{
  tft.fillScreen(TFT_BLACK); // Fondo negro
  cortinas2();
  drawCenteredText("M 8 A X", 30, 100);
  drawCenteredText("NerdMiner V2", 70, 100);
  drawCenteredText("V  10 . 03 . 77", 110, 100);
  delay(500);
  television();
  delay(1000);
  tft.setTextSize(1);
}

/**
 * Muestra una animación similar al M8AXTicker pero con un texto diferente, específicamente diseñado para el minador BTC.
 *
 * - Comienza llenando la pantalla con color negro y eligiendo un color de texto aleatorio.
 * - Luego muestra tres líneas de texto centradas en la pantalla con la siguiente información:
 *   - "B T C  M I N E R"
 *   - "NerdMiner V2"
 *   - "V  10 . 03 . 77"
 * - Después de mostrar el texto, realiza una pausa de 500 ms antes de ejecutar la función `television()`.
 * - Después de 1000 ms, restablece el tamaño del texto a 1.
 */

void M8AXTicker2()
{
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(colors[colorI], TFT_BLACK);
  tft.setTextSize(2);
  drawCenteredText("B T C  M I N E R", 30, 100);
  drawCenteredText("NerdMiner V2", 70, 100);
  drawCenteredText("V  10 . 03 . 77", 110, 100);
  delay(500);
  television();
  delay(1000);
  tft.setTextSize(1);
}

/**
 * Muestra una animación similar al M8AXTicker, pero con el texto "M I N E  T E C H".
 *
 * - Llenará la pantalla con un fondo negro y luego seleccionará un color de texto aleatorio.
 * - Muestra tres líneas de texto centradas:
 *   - "M I N E  T E C H"
 *   - "NerdMiner V2"
 *   - "V  10 . 03 . 77"
 * - Después de mostrar el texto, realiza una pausa de 500 ms.
 * - Luego ejecuta la función `television()`, que probablemente genera algún tipo de animación visual.
 * - Después de 1000 ms, restablece el tamaño del texto a 1.
 */

void M8AXTicker3()
{
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(colors[colorI], TFT_BLACK);
  tft.setTextSize(2);
  drawCenteredText("M I N E  T E C H", 30, 100);
  drawCenteredText("NerdMiner V2", 70, 100);
  drawCenteredText("V  10 . 03 . 77", 110, 100);
  delay(500);
  television();
  delay(1000);
  tft.setTextSize(1);
}

/**
 * Muestra una animación similar al M8AXTicker, pero con el texto "I M O D   T E C H".
 *
 * - Llena la pantalla con un fondo negro y luego selecciona un color de texto aleatorio.
 * - Muestra tres líneas de texto centradas:
 *   - "I M O D   T E C H"
 *   - "NerdMiner V2"
 *   - "V  10 . 03 . 77"
 * - Después de mostrar el texto, realiza una pausa de 500 ms.
 * - Luego ejecuta la función `television()`, que probablemente genera algún tipo de animación visual.
 * - Después de 1000 ms, restablece el tamaño del texto a 1.
 */

void M8AXTicker4()
{
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(colors[colorI], TFT_BLACK);
  tft.setTextSize(2);
  drawCenteredText("I M O D   T E C H", 30, 100);
  drawCenteredText("NerdMiner V2", 70, 100);
  drawCenteredText("V  10 . 03 . 77", 110, 100);
  delay(500);
  television();
  delay(1000);
  tft.setTextSize(1);
}

/**
 * Muestra una animación similar al M8AXTicker, pero con el texto "I M O D   T E C H".
 *
 * - Llena la pantalla con un fondo negro y luego selecciona un color de texto aleatorio.
 * - Muestra tres líneas de texto centradas:
 *   - "E H D  -  M D D D"
 *   - "NerdMiner V2"
 *   - "V  10 . 03 . 77"
 * - Después de mostrar el texto, realiza una pausa de 500 ms.
 * - Luego ejecuta la función `television()`, que probablemente genera algún tipo de animación visual.
 * - Después de 1000 ms, restablece el tamaño del texto a 1.
 */

void M8AXTicker5()
{
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(colors[colorI], TFT_BLACK);
  tft.setTextSize(2);
  drawCenteredText("E H D  -  M D D D", 30, 100);
  drawCenteredText("NerdMiner V2", 70, 100);
  drawCenteredText("V  10 . 03 . 77", 110, 100);
  delay(500);
  television();
  delay(1000);
  tft.setTextSize(1);
}

/**
 * Dibuja un código QR en la pantalla TFT.
 *
 * Esta función genera un código QR a partir de una cadena de texto (`data`) y lo dibuja en la pantalla TFT
 * en una posición específica con un tamaño y color determinado.
 *
 * @param data Cadena de texto que contiene la información a codificar en el código QR.
 * @param xPos Posición X en la pantalla donde se ubicará el código QR.
 * @param yPos Posición Y en la pantalla donde se ubicará el código QR.
 * @param qrSize Tamaño total del código QR que se desea dibujar (en píxeles).
 * @param color Color del código QR (normalmente blanco o negro para los módulos).
 */

void dibujaQR(String data, int xPos, int yPos, int qrSize, int color)
{
  QRCode qrcode;
  uint8_t qrcodeData[qrcode_getBufferSize(3)]; // Tamaño del QR, nivel 3

  // Inicializa el QR con los datos
  qrcode_initText(&qrcode, qrcodeData, 3, 0, data.c_str());

  // Tamaño real del QR en módulos
  int qrRealSize = qrcode.size;

  // Calcular el tamaño de cada bloque (módulo) según el tamaño total deseado
  int blockSize = qrSize / qrRealSize;

  // Dibujar los módulos (pixeles) del QR en la pantalla
  for (int y = 0; y < qrRealSize; y++)
  {
    for (int x = 0; x < qrRealSize; x++)
    {
      // Si es un módulo negro, dibujarlo
      if (qrcode_getModule(&qrcode, x, y))
      {
        tft.fillRect(xPos + x * blockSize, yPos + y * blockSize, blockSize, blockSize, color);
      }
      // Si es un módulo blanco, NO hacer nada (deja el fondo transparente)
    }
  }
}

/**
 * Verifica si es horario de verano.
 *
 * Esta función determina si una fecha específica (mes y día) cae dentro del horario de verano.
 *
 * @param mes Mes del año (1-12).
 * @param dia Día del mes.
 * @return true si es horario de verano, false en caso contrario.
 */

bool esHorarioDeVerano(int mes, int dia)
{
  if (mes > 3 && mes < 10)
  {
    return true;
  }
  if (mes == 3 && dia >= 25)
  {
    return true;
  }
  if (mes == 10 && dia <= 31)
  {
    return false;
  }

  return false;
}

/**
 * Dibuja un medidor analógico de KH/s.
 *
 * Esta función dibuja un medidor circular en la pantalla con graduaciones y una aguja que se mueve
 * según el valor de KH/s. Además, muestra el valor numérico de KH/s en el centro del medidor.
 *
 * @param khs Valor de KH/s que se usa para mover la aguja.
 */

void dibujaAnalogKH(float khs)
{
  int centerX = 160;
  int centerY = 85;
  int radius = 60;
  tft.setTextSize(1);
  tft.drawCircle(centerX, centerY, radius, TFT_WHITE);
  for (int i = 0; i < 360; i += 15)
  {
    float angle = (i - 90) * DEG_TO_RAD;
    int outerX = centerX + radius * cos(angle);
    int outerY = centerY + radius * sin(angle);
    int innerX = centerX + (radius - 5) * cos(angle);
    int innerY = centerY + (radius - 5) * sin(angle);
    if (i % 90 == 0)
    {
      innerX = centerX + (radius - 10) * cos(angle);
      innerY = centerY + (radius - 10) * sin(angle);
      tft.drawLine(innerX, innerY, outerX, outerY, TFT_WHITE);
    }
    else
    {
      tft.drawLine(innerX, innerY, outerX, outerY, TFT_WHITE);
    }
    if (i % 90 == 0)
    {
      tft.setTextColor(TFT_WHITE);
      int textX = centerX + (radius - 20) * cos(angle); // Coordenadas para el texto
      int textY = centerY + (radius - 20) * sin(angle);
      tft.setCursor(textX - 5, textY - 5); // Ajustar el texto
      if (i == 0)
        tft.print("360"); // 12 horas -> 360 KH/s
      else if (i == 90)
        tft.print("90"); // 15 minutos -> 90 KH/s
      else if (i == 180)
        tft.print("180"); // 30 minutos -> 180 KH/s
      else if (i == 270)
        tft.print("270"); // 45 minutos -> 270 KH/s
    }
  }

  // No dibujar la aguja si khs es 0
  if (khs == 0)
  {
    return;
  }

  // Mapear el valor de KH/s a un ángulo de -90 (0 KH/s) a 90 (360 KH/s)
  float angle = map(khs, 0, 360, -90, 270); // Amplitud del reloj ajustada a 360 grados

  // Convertir el ángulo a radianes
  float radianes = angle * DEG_TO_RAD;

  // Coordenadas de la punta de la aguja (calculada a partir del ángulo)
  int agujaX = centerX + radius * cos(radianes);
  int agujaY = centerY + radius * sin(radianes);

  // Dibujar la aguja
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  tft.drawLine(centerX, centerY, agujaX, agujaY, TFT_GREENYELLOW); // Dibuja la aguja
  tft.setCursor(142, 96);
  tft.setTextSize(1);
  tft.setTextColor(colors[colorI]);
  tft.print(String(khs));
  tft.setCursor(142, 105);
  tft.print(" KH/s");
}

/**
 * Obtiene y procesa las noticias desde un feed RSS.
 *
 * Realiza una solicitud HTTP GET a una de las URL de noticias seleccionada aleatoriamente, extrae los titulares de noticias y los muestra en el monitor serial.
 * También muestra los titulares en la pantalla LCD del dispositivo.
 *
 * Solo muestra hasta 5 noticias válidas, y omite noticias de canales específicos como "Cointelegraph.com News".
 */

void obtenerNoticias()
{
  HTTPClient http;
  int indice = esp_random() % 9;
  String urlnot = urls[indice];
  http.begin(urlnot);        // URL del feed RSS
  int httpCode = http.GET(); // Realiza la solicitud GET
  cadenanoti = "";
  if (httpCode == HTTP_CODE_OK)
  { // Si la solicitud es exitosa
    String payload = http.getString();
    Serial.println("M8AX - Noticias Obtenidas. Procesando...");

    int itemIndex = 0;
    int startIndex = 0;
    int endIndex = 0;

    // Extraer hasta 5 titulares
    while (itemIndex < 5)
    {
      // Buscar la etiqueta <item> que marca una nueva noticia
      startIndex = payload.indexOf("<item>", endIndex); // Encuentra <item>
      if (startIndex == -1)
        break;

      endIndex = payload.indexOf("</item>", startIndex); // Encuentra </item>
      if (endIndex == -1)
        break;

      // Extraer el título dentro de <title> de cada <item>
      int titleStart = payload.indexOf("<title>", startIndex);
      int titleEnd = payload.indexOf("</title>", titleStart);
      String title = payload.substring(titleStart + 7, titleEnd); // Extrae el título

      // Eliminar la parte CDATA si está presente
      int cdataStart = title.indexOf("<![CDATA[");
      int cdataEnd = title.indexOf("]]>");
      if (cdataStart != -1 && cdataEnd != -1)
      {
        title = title.substring(cdataStart + 9, cdataEnd); // Elimina CDATA
      }

      // Si el título es del canal (ej. "Cointelegraph.com News"), busca otra noticia
      if (title.indexOf("Cointelegraph.com News") != -1)
      {
        continue; // Si el título es del canal, salta a la siguiente noticia
      }

      // Si llegamos aquí, es una noticia válida, la mostramos
      Serial.println(String(itemIndex + 1) + ". " + title);
      cadenanoti += String(itemIndex + 1) + ". " + title + "\n\n";
      itemIndex++;
      taskYIELD();
    }
    tft.setCursor(1, 26);
    tft.setTextSize(1);
    colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
    tft.setTextColor(colors[colorI]);
    tft.print(quitarAcentos(cadenanoti));
    cadenanoti = "";

    // Si no se encontraron 5 noticias válidas, avisa al usuario
    if (itemIndex == 0)
    {
      Serial.println("M8AX - No Se Encontraron Noticias Válidas.");
    }
  }
  else
  {
    Serial.println("M8AX - Error Al Obtener El Feed RSS. Código HTTP: " + String(httpCode));
  }

  http.end(); // Finaliza la solicitud HTTP
}

/**
 * Obtiene el precio de un par de divisas de la API de Coinbase.
 *
 * Realiza una solicitud HTTPS a la API de Coinbase para obtener el precio actual de una criptomoneda o par de divisas especificado.
 *
 * @param currency_pair El par de divisas para obtener el precio (ej. "BTC-USD").
 * @return El precio actual de la criptomoneda en formato flotante, o -1 si hay un error de conexión o de procesamiento.
 */

float obtenerPrecio(String currency_pair)
{
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("M8AX - WiFi No Conectado, Intentando Reconectar...");
    WiFi.disconnect();
    WiFi.reconnect();
    delay(500);
  }

  WiFiClientSecure client;
  client.setInsecure(); // Desactiva verificación SSL (Coinbase usa HTTPS)

  const char *host = "api.coinbase.com";
  const int port = 443; // HTTPS usa el puerto 443

  Serial.print("M8AX - Conectando A ");
  Serial.println(host);

  if (!client.connect(host, port))
  {
    Serial.println("M8AX - Error: No Se Pudo Conectar Al Servidor");
    return -1;
  }

  // Construimos la petición HTTP manualmente
  String url = "/v2/prices/" + currency_pair + "/spot";
  client.print(String("GET ") + url + " HTTP/1.1\r\n" + "Host: " + host + "\r\n" + "User-Agent: ESP32\r\n" + "Connection: close\r\n\r\n");

  // Esperamos la respuesta del servidor
  unsigned long timeout = millis();
  while (client.available() == 0)
  {
    if (millis() - timeout > 2000)
    { // Timeout de 5 segundos
      Serial.println("M8AX - Error: Se Pasó El Tiempo De Espera");
      client.stop();
      return -1;
    }
  }

  // Leemos y descartamos los headers HTTP
  while (client.available())
  {
    String linea = client.readStringUntil('\n');
    if (linea == "\r")
    {
      break; // Fin de los headers
    }
  }

  // Leemos solo el JSON de la respuesta
  String payload = "";
  while (client.available())
  {
    payload += client.readString();
  }

  Serial.println("M8AX - Respuesta De La API:");
  Serial.println(payload);

  client.stop(); // Cerramos la conexión

  // Parseamos el JSON con menos consumo de RAM
  DynamicJsonDocument doc(256);
  DeserializationError error = deserializeJson(doc, payload);

  if (error)
  {
    Serial.println("M8AX - Error Al Parsear JSON: ");
    Serial.println(error.c_str());
    return -1;
  }

  // Extraemos el precio si existe
  if (doc.containsKey("data") && doc["data"].containsKey("amount"))
  {
    float precio = doc["data"]["amount"].as<float>();
    return precio;
  }
  else
  {
    Serial.println("M8AX - El JSON No Tiene El Formato Esperado.");
    return -1;
  }
}

// Función que recibe un número entero y devuelve una cadena con el número en morse

String Amorse(int n)
{
  // Diccionario de los números y sus representaciones en código morse
  const String morse_dict[] = {
      "-----", ".----", "..---", "...--", "....-", ".....", "-....", "--...", "---..", "----."};

  // Convertir el número a cadena de caracteres
  String num_str = String(n);

  // Variable para almacenar el resultado en código Morse
  String morse_code = "";

  // Convertir cada dígito del número a morse y concatenarlo a la cadena resultante
  for (unsigned int i = 0; i < num_str.length(); i++)
  {
    char digit = num_str.charAt(i);
    morse_code += morse_dict[digit - '0'] + " "; // Restamos '0' para convertir el carácter a un índice
  }

  return morse_code;
}

// Función que recibe un número entero y devuelve una cadena con el número en binario

String ABinario(int num)
{
  String binary = "";

  if (num == 0)
  {
    return "0"; // Si el número es 0, devuelve "0"
  }

  while (num > 0)
  {
    binary = (num % 2 == 0 ? "0" : "1") + binary; // Agregar el bit al principio
    num /= 2;                                     // Divide el número entre 2
  }

  return binary;
}

/**
 * Obtiene una cita desde un servidor remoto.
 *
 * Realiza múltiples intentos para obtener una cita de una API. Si no se puede obtener la cita después de varios intentos, devuelve un mensaje de error.
 *
 * @return La cita obtenida o un mensaje de error si no se puede recuperar.
 */

String getQuote()
{
  HTTPClient http;
  http.begin(serverName);
  http.addHeader("Content-Type", "application/json");

  String quote = "ERROR AL OBTENER LA CITA... Mi Lema Es - ( ... Por Muchas Vueltas Que Demos, Siempre Tendremos El Culo Atras ... )"; // Valor por defecto en caso de error

  int httpResponseCode = 0;
  int attempts = 0;
  const int maxAttempts = 5; // Número máximo de intentos

  while (httpResponseCode <= 0 && attempts < maxAttempts)
  {
    // Realizar la solicitud GET
    httpResponseCode = http.GET();
    attempts++;
    taskYIELD();
    if (httpResponseCode > 0)
    {
      String payload = http.getString();

      // Parsear el JSON
      DynamicJsonDocument doc(1024);
      deserializeJson(doc, payload);

      // Extraer la cita y el autor
      String quoteText = doc[0]["q"].as<String>();
      String author = doc[0]["a"].as<String>();

      quote = "\"" + quoteText + "\"\n- " + author;
      Serial.println("M8AX - Frase Número - " + String(numfrases) + " - " + quote);
    }
    else
    {
      Serial.println("M8AX - Error Al Recibir La Cita. Reintentando...");
      delay(1000); // Esperar 1 segundos antes de volver a intentar
    }
  }

  if (attempts == maxAttempts)
  {
    Serial.println("M8AX - Se Alcanzó El Número Máximo De Intentos...");
  }
  http.end();
  return quote;
}

/**
 * Muestra una cita en la pantalla TFT.
 *
 * Esta función imprime una cita en la pantalla TFT, añadiendo una línea decorativa encima del texto y aplicando un color aleatorio a la cita.
 *
 * @param quote La cita a mostrar en la pantalla.
 */

void displayQuote(String quote)
{
  tft.setTextColor(TFT_WHITE); // Establecer color de texto
  // Configuración de la fuente
  tft.setTextSize(2);
  tft.setCursor(0, 28);
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  tft.setTextColor(colors[colorI]);
  tft.drawLine(0, 20, 320, 20, colors[colorI]); // Dibujar línea de (0, y) a (320, y)
  tft.print(quitarAcentos(quote));
  taskYIELD();
}

// Función para obtener el nombre del mes

String obtenerNombreMes(int mes)
{
  switch (mes)
  {
  case 1:
    return "Enero";
  case 2:
    return "Febrero";
  case 3:
    return "Marzo";
  case 4:
    return "Abril";
  case 5:
    return "Mayo";
  case 6:
    return "Junio";
  case 7:
    return "Julio";
  case 8:
    return "Agosto";
  case 9:
    return "Septiembre";
  case 10:
    return "Octubre";
  case 11:
    return "Noviembre";
  case 12:
    return "Diciembre";
  default:
    return "Mes inválido";
  }
}

/**
 * Dibuja un porcentaje de un círculo como un arco iluminado.
 * Esta función dibuja un círculo completo y luego ilumina un porcentaje del mismo, según el valor de `porcentaje` proporcionado. El área iluminada es proporcional a dicho porcentaje y se pinta de color.
 *
 * @param centroX El valor de la coordenada X del centro del círculo.
 * @param centroY El valor de la coordenada Y del centro del círculo.
 * @param radio El radio del círculo.
 * @param porcentaje El porcentaje de iluminación del círculo (0 a 100).
 */

void dibujarPorcentajeLunar(int centroX, int centroY, int radio, float porcentaje)
{
  // Aseguramos que el porcentaje esté en un rango válido (0% a 100%)
  if (porcentaje < 0)
    porcentaje = 0;
  if (porcentaje > 100)
    porcentaje = 100;
  // Calculamos la posición horizontal hasta donde se debe rellenar
  int limiteX = centroX + (porcentaje / 100.0) * (2 * radio) - radio; // Punto X del límite
  // Dibujar el círculo completo con el color de fondo
  tft.fillCircle(centroX, centroY, radio, TFT_TRANSPARENT);
  // Dibujar el porcentaje iluminado horizontalmente
  for (int x = -radio; x <= radio; x++)
  { // Recorrer el ancho del círculo
    for (int y = -radio; y <= radio; y++)
    { // Recorrer la altura del círculo
      // Verificar si el punto está dentro del círculo
      if (x * x + y * y <= radio * radio)
      {
        // Verificar si el punto está dentro del porcentaje horizontal
        if (centroX + x <= limiteX)
        {
          tft.drawPixel(centroX + x, centroY + y, colors[colorIndex]); // Color del área iluminada
        }
      }
    }
  }
}

/**
 * Dibuja el reloj con las manecillas de horas, minutos y segundos, junto con la fase lunar y otros datos.
 *
 * Esta función actualiza la pantalla con la hora actual, las manecillas del reloj, la fase lunar, el estado de la riqueza (millonario/no millonario), la ciudad y la temperatura. Los datos se extraen de varias fuentes y se visualizan de manera interactiva en la pantalla TFT.
 *
 * @param horas La hora actual.
 * @param minutos Los minutos actuales.
 * @param segundos Los segundos actuales.
 * @param dia El día actual en formato de cadena.
 * @param mes El mes actual en formato de cadena.
 * @param anio El año actual en formato de cadena.
 * @param quediase El texto adicional que se mostrará en el reloj.
 * @param HRate La tasa de hash actual (en KH/s).
 * @param tempera La temperatura actual.
 * @param millonario Indica si el usuario es millonario (1) o no (0).
 */

void dibujarReloj(int horas, int minutos, int segundos, String dia, String mes, String anio, std::string quediase, String HRate, String tempera, int millonario)
{
  moonData_t moon;
  int diaa = dia.toInt();
  int mess = mes.toInt();
  int anioo = anio.toInt();
  int horaa = horas;
  int minutoo = minutos;
  unsigned long segundo = timeClient.getSeconds();
  String mesecillo = obtenerNombreMes(mess).substring(0, 3);
  // Inicializar la estructura tm
  struct tm timeinfo;
  timeinfo.tm_year = anioo - 1900; // Año desde 1900
  timeinfo.tm_mon = mess - 1;      // Mes (0 = enero)
  timeinfo.tm_mday = diaa;         // Día del mes
  timeinfo.tm_hour = horaa;        // Hora
  timeinfo.tm_min = minutoo;       // Minutos
  timeinfo.tm_sec = segundo;       // Segundos
  timeinfo.tm_isdst = -1;          // Determina si es horario de verano (automático)
  // Convertir a time_t
  time_t cadenaDeTiempo = mktime(&timeinfo);
  moon = mymoonPhase.getPhase(cadenaDeTiempo);
  double porcentajeIluminado = moon.percentLit * 100;
  char porcentajeTexto[10];
  snprintf(porcentajeTexto, sizeof(porcentajeTexto), "%.2f%%", porcentajeIluminado);
  String datoslunarelogrande = String(porcentajeTexto);
  // Limpia la pantalla para redibujar
  background.pushImage(0, 0, M8AXRelojLunarWidth, M8AXRelojLunarHeight, M8AXRelojLunar);
  background.pushSprite(0, 0);
  // Centro del reloj
  int centroX = 160;     // Centro horizontal
  int centroY = 170 / 2; // Centro vertical
  int radio = 80;        // Aumenta el radio para hacer el reloj más grande
  // Dibuja el círculo del reloj
  tft.drawCircle(centroX, centroY, radio, colors[colorIndex]);
  // Calcula los ángulos para las manecillas
  float anguloSegundos = (segundos * 6) - 90;                         // 360° / 60 = 6° por segundo
  float anguloMinutos = (minutos * 6) - 90;                           // 360° / 60 = 6° por minuto
  float anguloHoras = (horas % 12) * 30 - 90 + (minutos / 60.0) * 30; // 360° / 12 = 30° por hora
  // Dibuja la manecilla de los segundos
  int xSegundos = centroX + radio * 0.85 * cos(radians(anguloSegundos)); // Ajusta la longitud de la manecilla
  int ySegundos = centroY + radio * 0.85 * sin(radians(anguloSegundos));
  tft.drawLine(centroX, centroY, xSegundos, ySegundos, TFT_RED);
  // Dibuja la manecilla de los minutos
  int xMinutos = centroX + radio * 0.75 * cos(radians(anguloMinutos)); // Manecilla de minutos más larga
  int yMinutos = centroY + radio * 0.75 * sin(radians(anguloMinutos));
  tft.drawLine(centroX, centroY, xMinutos, yMinutos, TFT_WHITE);
  // Dibuja la manecilla de las horas
  int xHoras = centroX + radio * 0.60 * cos(radians(anguloHoras)); // Manecilla de horas más corta
  int yHoras = centroY + radio * 0.60 * sin(radians(anguloHoras));
  tft.drawLine(centroX, centroY, xHoras, yHoras, TFT_WHITE);
  // Dibuja las marcas de las horas
  for (int i = 0; i < 12; i++)
  {
    float anguloMarca = i * 30 - 90; // 360° / 12 = 30° por hora
    int xExterior = centroX + radio * 0.95 * cos(radians(anguloMarca));
    int yExterior = centroY + radio * 0.95 * sin(radians(anguloMarca));
    int xInterior = centroX + radio * 0.8 * cos(radians(anguloMarca));
    int yInterior = centroY + radio * 0.8 * sin(radians(anguloMarca));
    tft.drawLine(xInterior, yInterior, xExterior, yExterior, colors[colorIndex]);
  }
  tft.setCursor(4, 5);
  tft.setTextColor(colors[colorIndex]);
  tft.setTextSize(3);
  tft.print(dia);
  tft.setCursor(4, 37);
  tft.print(mesecillo);
  tft.setCursor(4, 69);
  tft.print(String(quediase.c_str()));
  tft.setCursor(4, 101);
  tft.print(anio);
  tft.setCursor(4, 133);
  tft.setTextSize(2);
  tft.print(quitarAcentos(ciudad).substring(0, 7));
  tft.setCursor(23, 153);
  tempciudad.replace("°", "");
  tft.print(tempciudad);
  tft.setTextSize(1);
  tft.setCursor(143, 115);
  tft.print(HRate);
  tft.setCursor(143, 125);
  tft.print(" KH/s");
  tft.setCursor(254, 35);
  tft.setTextSize(2);
  tft.print("LUNA");
  tft.setCursor(236, 121);
  tft.setTextSize(2);
  tft.print(datoslunarelogrande);
  tft.setTextSize(1);
  tft.setCursor(143, 42);
  tft.print("  " + tempera);
  tft.setCursor(143, 52);
  tft.print("Grados");
  tft.setTextColor(TFT_DARKGREY);
  tft.setCursor(254, 5);
  tft.setTextSize(2);
  tft.print("RICO");
  tft.setCursor(230, 150);
  tft.setTextSize(2);
  tft.print("NoRICO");
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  if (millonario == 0)
  {
    tft.setTextColor(colors[colorI]);
    tft.setCursor(230, 150);
    tft.setTextSize(2);
    tft.print("NoRICO");
  }
  else
  {
    tft.setTextColor(colors[colorI]);
    tft.setCursor(254, 5);
    tft.setTextSize(2);
    tft.print("RICO");
  }
  dibujarPorcentajeLunar(281, 84, 25, porcentajeIluminado);
  if (mirarTiempo == 0 || ciudad == "ERROR" || (minutos % 10 == 0 && segundos == 0))
  {
    mirarTiempo = 1;
    std::pair<String, String> resultado = obtenerCiudadYTemperatura(getPublicIP());
    // Verificar si los valores fueron obtenidos correctamente
    if (resultado.first != "" && resultado.second != "")
    {
      // Imprimir los resultados en el Monitor Serie
      Serial.println("M8AX - Ciudad: " + resultado.first);
      Serial.println("M8AX - Temperatura: " + resultado.second);
      ciudad = resultado.first;
      tempciudad = resultado.second;
    }
    else
    {
      Serial.println("M8AX - No Se Pudo Obtener La Ciudad O La Temperatura.");
      ciudad = "ERROR";
      tempciudad = "ERROR";
    }
  }
  actualizarc = 0;
}

// Función para generar 6 números únicos y devolverlos como String

String generarNumerosPrimitiva()
{
  const int MIN = 1;
  const int MAX = 49;
  const int NUM_COUNT = 6;
  int numeros[NUM_COUNT];
  int contador = 0;

  while (contador < NUM_COUNT)
  {
    int num = (esp_random() % 49) + 1; // Generar número aleatorio
    bool repetido = false;
    // Comprobar si el número ya existe en el array
    for (int i = 0; i < contador; i++)
    {
      if (numeros[i] == num)
      {
        repetido = true;
        break;
      }
    }
    // Si no está repetido, lo añadimos al array
    if (!repetido)
    {
      numeros[contador++] = num;
    }
  }
  // Convertir los números a un String
  String resultado = "";
  for (int i = 0; i < NUM_COUNT; i++)
  {
    resultado += String(numeros[i]);
    if (i < NUM_COUNT - 1)
    {
      resultado += ", "; // Separador entre números
    }
  }
  return resultado; // Retornar como String
}

// Función para calcular el primer día del mes

int calcularPrimerDia(int dia, int mes, int anio)
{
  // Si el mes es enero o febrero, se ajusta como en el algoritmo de Zeller
  if (mes == 1)
  {
    mes = 13;
    anio--;
  }
  if (mes == 2)
  {
    mes = 14;
    anio--;
  }
  // Cálculo según la fórmula de Zeller
  int q = dia;
  int m = mes;
  int k = anio % 100;
  int j = anio / 100;
  int h = (q + (13 * (m + 1)) / 5 + k + k / 4 + j / 4 + 5 * j) % 7;
  // Ajuste del valor de h para alinear correctamente los días de la semana
  // La fórmula de Zeller devuelve 0 = sábado, 1 = domingo, ..., 6 = viernes
  // Queremos que 0 sea domingo, 1 sea lunes, ..., 6 sea sábado
  h = (h + 5) % 7;
  return h;
}

/*
  Función: mostrarCalendario
  Propósito: Esta función dibuja un calendario en una pantalla TFT con el mes y año especificados,
  mostrando los días de la semana y resaltando el día actual. Además, muestra la hora en formato
  de 24 horas y el mes y año en la parte inferior del calendario.

  Parámetros:
    - dia: El día actual del mes (1 a 31).
    - mes: El mes del calendario (1 a 12).
    - anio: El año del calendario (por ejemplo, 2025).
    - h1, h2: Las dos partes de la hora actual (por ejemplo, 14 para las 2 PM).
    - m1, m2: Las dos partes de los minutos actuales (por ejemplo, 30 para los 30 minutos).

  La función calcula el primer día del mes y muestra los días de la semana en la parte superior.
  Luego, llena el calendario con los días correspondientes, comenzando desde el día correcto.
  Resalta el día actual y marca los fines de semana con un color distinto.
  También muestra la hora actual y el mes/año en la parte inferior del calendario.
*/

void mostrarCalendario(int dia, int mes, int anio, int h1, int h2, int m1, int m2)
{
  // Calcular el primer día del mes usando la función calcularPrimerDia
  int primerDia = calcularPrimerDia(1, mes, anio); // El día 1 del mes
  // Días de la semana
  String diasSemana[7] = {"LUN", "MAR", "MIE", "JUE", "VIE", "SAB", "DOM"};
  // Número de días en el mes (esto debe tenerse en cuenta para cada mes)
  int diasDelMes = 31; // Por defecto, asumir 31 días
  if (mes == 4 || mes == 6 || mes == 9 || mes == 11)
    diasDelMes = 30; // Meses con 30 días
  if (mes == 2)
  { // Febrero, comprobar si es bisiesto
    if ((anio % 4 == 0 && anio % 100 != 0) || (anio % 400 == 0))
    {
      diasDelMes = 29; // Año bisiesto
    }
    else
    {
      diasDelMes = 28; // Año no bisiesto
    }
  }
  // Mostrar los días de la semana
  for (int i = 0; i < 7; i++)
  {
    tft.setCursor(20 + (i * 40), 10);
    tft.setTextColor(colors[colorIndex]);
    tft.setTextSize(1);
    tft.print(diasSemana[i]);
  }
  // Mostrar los días del mes
  int x = primerDia * 40;
  int y = 30;
  for (int i = 1; i <= diasDelMes; i++)
  {
    // Si es el primer día, comenzamos en la posición correcta según el primer día del mes
    int diaDeLaSemana = (primerDia + i - 1) % 7;
    // Imprimir el día
    tft.setCursor(20 + x, y);
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE);
    if (i == dia)
    {
      tft.setTextColor(TFT_YELLOW);
      tft.setTextSize(2);
    }
    else if (diaDeLaSemana == 5 || diaDeLaSemana == 6)
    {
      tft.setTextColor(TFT_ORANGE);
    }
    else
    { // Días regulares
      tft.setTextColor(TFT_WHITE);
    }
    tft.print(i);
    // Mover a la siguiente columna
    x += 40;
    // Si hemos llegado al final de la fila (7 días), pasamos a la siguiente fila
    if (x >= 7 * 40)
    {
      x = 0;
      y += 20;
    }
    String mesecillo = String(obtenerNombreMes(mes));
    tft.setTextColor(colors[colorIndex]);
    tft.setTextSize(3);
    tft.setCursor(5, 145);
    tft.print(mesecillo + " " + String(anio));
    tft.setCursor(290, 14);
    tft.print(String(h1));
    tft.setCursor(290, 44);
    tft.print(String(h2));
    tft.setCursor(290, 74);
    tft.print("-");
    tft.setCursor(290, 104);
    tft.print(String(m1));
    tft.setCursor(290, 134);
    tft.print(String(m2));
  }
}

/*
  Función: esPrimo
  Propósito: Determina si un número entero positivo es primo. Un número es primo si es mayor que 1
  y no tiene divisores positivos distintos de 1 y él mismo.

  Parámetros:
    - n: Número entero positivo (de tipo uint32_t) a verificar si es primo.

  Retorno:
    - Devuelve 'true' si el número n es primo.
    - Devuelve 'false' si el número n no es primo.

  Descripción:
    La función comienza verificando si el número es menor que 2, en cuyo caso devuelve 'false' ya que los números menores a 2 no son primos.
    Luego, maneja los casos especiales para los números 2 y 3, que son primos.
    Si el número es par, lo descarta de inmediato, ya que todos los números pares mayores que 2 no son primos.
    Para números impares mayores que 3, la función comprueba si existen divisores hasta la raíz cuadrada de n, incrementando de 2 en 2 (verificando solo números impares).
    Si se encuentra un divisor, devuelve 'false', y si no se encuentran divisores, devuelve 'true'.
*/

bool esPrimo(uint32_t n)
{
  // Si el número es menor que 2, no es primo
  if (n <= 1)
    return false;
  // Caso base para 2 y 3
  if (n == 2 || n == 3)
    return true;
  // Eliminar los números pares
  if (n % 2 == 0)
    return false;
  // Solo verificamos hasta la raíz cuadrada de n
  uint32_t limite = sqrt(n); // Usar la raíz cuadrada para limitar las verificaciones
  // Verificar divisibilidad solo con números impares
  for (uint32_t i = 3; i <= limite; i += 2)
  {
    if (n % i == 0)
    {
      return false; // Si encontramos un divisor, el número no es primo
    }
  }
  return true; // Si no encontramos divisores, el número es primo
}

/*
  Función: _fhour
  Propósito: Calcula la hora en formato decimal, donde la parte entera representa las horas completas y la parte decimal
  representa la fracción del minuto y el segundo transcurridos.

  Parámetros:
    - timeinfo: Una referencia constante a una estructura `tm` que contiene la información de tiempo (hora, minuto, segundo).
      - timeinfo.tm_hour: Hora (entero de 0 a 23).
      - timeinfo.tm_min: Minuto (entero de 0 a 59).
      - timeinfo.tm_sec: Segundo (entero de 0 a 59).

  Retorno:
    - Devuelve un valor de tipo `double` que representa la hora como un número decimal.
      - La parte entera de este valor corresponde a las horas (`timeinfo.tm_hour`).
      - La parte decimal se calcula a partir de los minutos y segundos, mapeándolos a un rango de [0.0, 1.0].

  Descripción:
    Esta función toma la hora, los minutos y los segundos de la estructura `tm` y los convierte en un valor decimal representando
    las horas transcurridas en el día. La función utiliza la función `map` para transformar los segundos del minuto (`timeinfo.tm_min * 60 + timeinfo.tm_sec`)
    en una fracción entre 0 y 1, que se suma a las horas.
    El valor retornado es una representación continua del tiempo, donde la parte entera son las horas y la parte decimal
    refleja el tiempo adicional transcurrido en fracciones de hora.
*/

double moonPhase::_fhour(const struct tm &timeinfo)
{
  return timeinfo.tm_hour + map((timeinfo.tm_min * 60) + timeinfo.tm_sec, 0, 3600, 0.0, 1.0);
}

/*
  Función: _Julian
  Propósito: Calcula la fecha juliana a partir de una fecha gregoriana especificada por el año, mes y día.

  Parámetros:
    - year: El año de la fecha en formato de 4 dígitos (ejemplo: 2025).
    - month: El mes de la fecha (1 = enero, 12 = diciembre).
    - day: El día del mes (valor decimal, por ejemplo, 15.5 representaría el 15 de un mes a la mitad del día).

  Retorno:
    - Devuelve un valor de tipo `double` que representa la fecha juliana correspondiente a la fecha proporcionada.
      La fecha juliana es el número de días (y fracción de días) desde el mediodía del 1 de enero de 4713 a.C.

  Descripción:
    Esta función convierte una fecha en el calendario gregoriano (proporcionada en términos de año, mes y día) a una fecha juliana,
    que es utilizada comúnmente en astronomía para cálculos de tiempos largos y para evitar los problemas con los calendarios.
    La función implementa la corrección del calendario gregoriano, que fue introducida después del 15 de octubre de 1582, momento en el cual
    el calendario juliano fue reemplazado. El ajuste es hecho a través del valor de `b`, que depende del año, mes y día.
    La fórmula utilizada es:
    - Si la fecha es posterior al 15 de octubre de 1582, se aplica el ajuste en `b`.
    - Se calcula el número de días del año (365.25 * year), y se ajusta con el factor `30.6001 * (month + 1)` para el mes y día.
    - Se suman todos los componentes, incluyendo el valor base de 1720994.5, que corresponde a la fecha juliana del 1 de enero de 4713 a.C.

    El valor retornado es un número decimal que representa el número de días transcurridos desde la fecha de inicio de la fecha juliana.

  Ejemplo:
    Para la fecha 15 de octubre de 2025, la función calcularía el número correspondiente en el calendario juliano.
*/

static double _Julian(int32_t year, int32_t month, const double &day)
{
  int32_t b, c, e;
  b = 0;
  if (month < 3)
  {
    year--;
    month += 12;
  }
  if (year > 1582 || (year == 1582 && month > 10) || (year == 1582 && month == 10 && day > 15))
  {
    int32_t a;
    a = year / 100;
    b = 2 - a + a / 4;
  }
  c = 365.25 * year;
  e = 30.6001 * (month + 1);
  return b + c + e + day + 1720994.5;
}

/*
  Función: _sun_position
  Propósito: Calcula la posición del sol en el eclíptico, dado un valor de fecha juliana (j).

  Parámetros:
    - j: El valor de la fecha juliana (un número decimal que representa la fecha en el calendario juliano).

  Retorno:
    - Devuelve un valor de tipo `double` que representa la longitud del sol en grados eclípticos (0-360°),
      es decir, la posición del sol a lo largo de su órbita en el plano de la eclíptica en un momento específico.

  Descripción:
    Esta función calcula la posición del sol a lo largo de su órbita elíptica en el sistema solar, basándose en la fecha juliana proporcionada.
    El cálculo se realiza utilizando la fórmula estándar de la astronomía para la longitud del sol, que incluye varios parámetros y correcciones
    para obtener una mayor precisión.

    El proceso incluye los siguientes pasos:
    1. Se calcula el ángulo de la órbita del sol (n) con la fórmula `360 / 365.2422 * j`, donde `j` es la fecha juliana.
    2. Se ajusta el valor de `n` para asegurarse de que esté dentro del rango de 0 a 360 grados.
    3. Se obtiene el ángulo eclíptico inicial (x), y se ajusta para asegurarse de que esté dentro del rango adecuado.
    4. Luego, se resuelve una ecuación iterativa (Newton-Raphson) para obtener una corrección precisa a la longitud del sol (`dl`).
    5. Finalmente, la longitud del sol `l` se calcula sumando un término constante a `v`, el valor obtenido de la función trigonométrica `atan`,
       que depende del ángulo corregido `e`.
    6. El valor final de `l` es normalizado para que esté entre 0 y 360 grados, y este es el valor retornado, que representa la longitud del sol.

    La longitud del sol es importante en astronomía para determinar diversas variables, como la posición del sol respecto a las estrellas fijas,
    y se utiliza en el cálculo de fenómenos astronómicos como los equinoccios y los solsticios.

  Ejemplo:
    Si se pasa un valor de `j = 2451545.0` (la fecha juliana correspondiente al 1 de enero de 2000), la función calcularía la posición del sol
    en esa fecha, devolviendo un valor entre 0 y 360 grados.
*/

static double _sun_position(const double &j)
{
  double n, x, e, l, dl, v;
  int32_t i;
  n = 360 / 365.2422 * j;
  i = n / 360;
  n = n - i * 360.0;
  x = n - 3.762863;
  x += (x < 0) ? 360 : 0;
  x *= DEG_TO_RAD;
  e = x;
  do
  {
    dl = e - .016718 * sin(e) - x;
    e = e - dl / (1 - .016718 * cos(e));
  } while (fabs(dl) >= 1e-12);
  v = 360 / PI * atan(1.01686011182 * tan(e / 2));
  l = v + 282.596403;
  i = l / 360;
  l = l - i * 360.0;
  return l;
}

/*
  Función: _moon_position
  Propósito: Calcula la posición de la Luna en el eclíptico, dado un valor de fecha juliana (j) y la longitud del sol (ls).

  Parámetros:
    - j: El valor de la fecha juliana (un número decimal que representa la fecha en el calendario juliano).
    - ls: La longitud del sol en grados eclípticos, calculada previamente. Representa la posición del sol en su órbita.

  Retorno:
    - Devuelve un valor de tipo `double` que representa la longitud de la Luna en grados eclípticos (0-360°), es decir, la posición de la Luna
      a lo largo de su órbita en el plano de la eclíptica en un momento específico.

  Descripción:
    Esta función calcula la posición de la Luna a lo largo de su órbita en el sistema solar, basándose en la fecha juliana proporcionada y
    la longitud del sol. El cálculo se realiza utilizando una serie de correcciones empíricas que se utilizan comúnmente en astronomía para
    modelar el movimiento lunar.

    El proceso incluye los siguientes pasos:
    1. Se calcula el ángulo del movimiento medio de la Luna (`ms`), que depende de la fecha juliana.
    2. Se calcula la longitud media del sol (`l`), ajustada para asegurarse de que esté dentro del rango adecuado.
    3. Se calcula la longitud media de la Luna (`mm`), ajustada también para estar dentro de un rango adecuado.
    4. Se calcula el valor de la variación lunar (`ev`), que es una corrección basada en la diferencia de las longitudes entre la Luna y el sol.
    5. Se calcula la excentricidad (`ae`), que es una corrección adicional que depende del ángulo `ms`.
    6. Se ajusta la longitud de la Luna (`mm`) mediante la corrección de variación y excentricidad.
    7. Se calcula la corrección de la Luna debido a la excentricidad (`ec`), que se basa en el ángulo `mm`.
    8. Finalmente, se ajusta la longitud de la Luna (`l`) utilizando la corrección de variación, la corrección de excentricidad y una corrección adicional
       que involucra un término de segundo orden.

    El resultado final de `l` es la posición de la Luna en el plano eclíptico, y es utilizado para cálculos de la fase lunar, eclipses y otros
    fenómenos relacionados con el movimiento de la Luna.

  Ejemplo:
    Si se pasa un valor de `j = 2451545.0` (la fecha juliana correspondiente al 1 de enero de 2000) y `ls = 280.0` (la longitud del sol en esa fecha),
    la función calcularía la posición de la Luna en esa fecha, devolviendo un valor entre 0 y 360 grados.
*/

static double _moon_position(const double &j, const double &ls)
{
  double ms, l, mm, ev, sms, ae, ec;
  int32_t i;
  ms = 0.985647332099 * j - 3.762863;
  ms += (ms < 0) ? 360.0 : 0;
  l = 13.176396 * j + 64.975464;
  i = l / 360;
  l = l - i * 360.0;
  l += (l < 0) ? 360 : 0;
  mm = l - 0.1114041 * j - 349.383063;
  i = mm / 360;
  mm -= i * 360.0;
  ev = 1.2739 * sin((2 * (l - ls) - mm) * DEG_TO_RAD);
  sms = sin(ms * DEG_TO_RAD);
  ae = 0.1858 * sms;
  mm += ev - ae - 0.37 * sms;
  ec = 6.2886 * sin(mm * DEG_TO_RAD);
  l += ev + ec - ae + 0.214 * sin(2 * mm * DEG_TO_RAD);
  l = 0.6583 * sin(2 * (l - ls) * DEG_TO_RAD) + l;
  return l;
}

/*
  Función: _getPhase
  Propósito: Calcula la fase de la Luna en un momento específico, dada una fecha y una hora.

  Parámetros:
    - year: El año en formato entero (ej. 2025).
    - month: El mes en formato entero (1-12).
    - day: El día del mes en formato entero (1-31).
    - hour: La hora del día en formato decimal (por ejemplo, 2.5 para 2 horas y 30 minutos).

  Retorno:
    - Devuelve un objeto de tipo `moonData_t` que contiene dos valores:
      1. El ángulo de la fase lunar en grados (0-360), representando la posición angular de la Luna en su órbita respecto al Sol.
      2. El porcentaje de la Luna iluminada (un valor entre 0 y 1).

  Descripción:
    Esta función calcula la fase de la Luna en un momento específico utilizando la fecha (año, mes, día) y la hora proporcionada. Se basa en dos cálculos
    astronómicos fundamentales:
    1. La posición del Sol en la órbita de la Tierra (utilizando la función `_sun_position`).
    2. La posición de la Luna en su órbita (utilizando la función `_moon_position`).

    El proceso general es el siguiente:
    1. Se calcula el valor de la fecha juliana con la función `_Julian` ajustando la hora proporcionada para obtener la fracción del día.
    2. Se calcula la posición del Sol (`ls`) usando la fecha juliana.
    3. Se calcula la posición de la Luna (`lm`) usando la fecha juliana y la posición del Sol.
    4. Se calcula el ángulo de la fase lunar (`angle`) como la diferencia entre la posición de la Luna y la del Sol.
       - Si el ángulo es negativo, se ajusta para que esté en el rango de 0-360 grados.
    5. El ángulo calculado (`angle`) representa la fase lunar, indicando la posición de la Luna en su órbita.
    6. Se calcula el porcentaje de la Luna iluminada, utilizando la fórmula `(1 - cos(angle)) / 2`. Esta fórmula se basa en el ángulo de la fase y describe la proporción de la superficie lunar iluminada por el Sol.

    El objeto `moonData_t` que se retorna contiene:
    - `angle`: El ángulo de fase de la Luna (0-360 grados), que indica en qué fase se encuentra la Luna en relación con el Sol (nueva, creciente, llena, menguante, etc.).
    - El porcentaje de la Luna iluminada, que varía de 0 (Luna nueva) a 1 (Luna llena).

  Ejemplo:
    Si se pasa `year = 2025`, `month = 2`, `day = 10`, y `hour = 12.0` (mediodía), la función calculará la fase lunar y el porcentaje de iluminación para ese momento específico, devolviendo un objeto `moonData_t` con los resultados.

  Notas:
    - El valor de `angle` se encuentra en el rango [0, 360], donde:
      - 0 grados = Luna nueva
      - 180 grados = Luna llena
    - El valor de la fracción iluminada es un número real entre 0 y 1, donde:
      - 0 = Luna nueva
      - 1 = Luna llena
*/

moonData_t moonPhase::_getPhase(const int32_t year, const int32_t month, const int32_t day, const double &hour)
{
  const double j{_Julian(year, month, (double)day + hour / 24.0) - 2444238.5};
  const double ls{_sun_position(j)};
  const double lm{_moon_position(j, ls)};
  double angle = lm - ls;
  angle += (angle < 0) ? 360 : 0;
  const moonData_t returnValue{
      (int32_t)angle,
      (1.0 - cos((lm - ls) * DEG_TO_RAD)) / 2};
  return returnValue;
}

/*
  Función: incrementCounter
  Propósito: Incrementa un contador de segundos y actualiza el índice de color y el nombre mostrado según el valor del contador.

  Descripción:
    Esta función incrementa un contador de segundos (`secondCounter`) y, cuando alcanza el valor de 59, se reinicia a 0 y se selecciona un nuevo índice de color aleatorio.
    Además, actualiza una variable de texto (`nombrecillo`) dependiendo del valor de `colorIndex`, el cual se ajusta cada vez que el contador llega a 59.

  Proceso:
    1. Se incrementa el valor de `secondCounter` en cada llamada a la función.
    2. Si el valor de `secondCounter` alcanza 59 (lo que indica que un minuto ha pasado), se realiza lo siguiente:
       - Se selecciona aleatoriamente un nuevo valor para `colorIndex` usando `esp_random()`, el cual es un número aleatorio entre 0 y el número total de colores disponibles en el array `colors`.
       - El contador `secondCounter` se reinicia a 0.
    3. Si el valor de `secondCounter` es menor que 59, se comprueba si `colorIndex` es par o impar:
       - Si `colorIndex` es par, se asigna el texto `" - M 8 A X -"`.
       - Si `colorIndex` es impar, se asigna el texto `"- MvIiIaX -"`.

  Efectos secundarios:
    - Se actualiza `colorIndex` con un valor aleatorio cuando el contador llega a 59.
    - Se cambia el valor de `nombrecillo` dependiendo del valor de `colorIndex`.

  Ejemplo:
    Si `secondCounter` es 59, el valor de `colorIndex` será actualizado a un nuevo valor aleatorio, y `nombrecillo` podría cambiar a `" - M 8 A X -"`, dependiendo si el valor de `colorIndex` es par o impar.

  Notas:
    - La función `esp_random()` genera un número aleatorio entre 0 y el tamaño del array `colors` para seleccionar un índice aleatorio de color.
    - El valor de `secondCounter` se mantiene entre 0 y 59, representando los segundos de un minuto.
*/

void incrementCounter()
{
  secondCounter++;
  if (secondCounter >= 59)
  {
    colorIndex = esp_random() % (sizeof(colors) / sizeof(colors[0]));
    secondCounter = 0;
  }
  else
  {
    if (colorIndex < 4)
    {
      nombrecillo = "EHD-MDDD";
    }
    else if (colorIndex % 2 == 0)
    {
      nombrecillo = " - M 8 A X -";
    }
    else
    {
      nombrecillo = "- MvIiIaX -";
    }
  }
}

/*
  Función: tDisplay_MinerScreen
  Propósito: Muestra la pantalla de minería con datos en tiempo real como hashrate, shares, temperatura, entre otros.

  Descripción:
    Esta función se encarga de mostrar la pantalla principal de la minería, que incluye datos como el hashrate actual, el total de hashes, los bloques validados, la temperatura y más.
    La información mostrada se actualiza cada vez que se llama a esta función, utilizando datos proporcionados por otras funciones auxiliares como `getMiningData`, `getClockData`, y `getCoinData`.
    Además, el fondo de la pantalla se actualiza según ciertos parámetros, como la hora del día o el estado de ciertos contadores.

  Proceso:
    1. **Obtención de datos**:
       Se obtienen varios datos relacionados con la minería, la hora, y la criptomoneda actual utilizando funciones como `getMiningData`, `getClockData`, y `getCoinData`.

    2. **Renderización del fondo**:
       Se dibuja un fondo de pantalla utilizando `background.pushImage()` con la imagen de la pantalla de minería (`MinerScreen`).

    3. **Hashrate de día y noche**:
       Dependiendo de la hora actual (`horiac`), se elige un color de texto diferente para el hashrate (negro para la noche y blanco para el día). Esto se renderiza en la pantalla con `render.rdrawString()`.

    4. **Mostrar datos de minería**:
       Se dibujan otros datos de la minería como el total de hashes, las plantillas de bloques, la mejor dificultad, el número de shares completados y la hora de minería. Estos datos se renderizan utilizando `render.drawString()` y `render.rdrawString()` con diferentes tamaños y colores de texto.

    5. **Mostrar texto de nombre**:
       Se muestra un texto personalizado basado en la variable `nombrecillo` que cambia dependiendo del índice de color (`colorIndex`).

    6. **Mostrar bloques válidos**:
       Se verifica si el número de bloques válidos (`valids`) es 0, y si es así, se muestra en color rojo, de lo contrario, se muestra en color verde/amarillo.

    7. **Mostrar temperatura**:
       Se imprime la temperatura de la minería utilizando `render.rdrawString()`.

    8. **Mostrar hora**:
       Se muestra la hora de minería (`currentTime`) con un tamaño de fuente pequeño.

    9. **Efecto visual con "..." o ". ."**:
       Basado en la hora de minería (últimos dos dígitos), se decide si mostrar "..." o ". ." de manera intermitente. Esto se hace dependiendo de si el valor es par o impar.

    10. **Empujar imagen final**:
        Finalmente, se actualiza la pantalla con `background.pushSprite()` para reflejar todos los cambios realizados en la interfaz.

  Efectos secundarios:
    - La pantalla se actualiza con la nueva información de minería y hora.
    - Se modifican los elementos visuales basados en condiciones como la hora del día o la paridad de los segundos.

  Notas:
    - Se utiliza un fondo estático que se actualiza con la imagen de la minería.
    - El uso de `esp_random()` para la intermitencia de los puntos (". .", "...") agrega un efecto visual aleatorio que hace que la pantalla sea más dinámica.
    - Los datos de minería y de reloj se obtienen de otras funciones, lo que implica que las funciones como `getMiningData`, `getClockData` y `getCoinData` deben estar implementadas correctamente para obtener la información correcta.
    - Añadido RSSI de la conexión WiFi, en la parte central superior de la pantalla.
*/

void tDisplay_MinerScreen(unsigned long mElapsed)
{
  mining_data data = getMiningData(mElapsed);
  clock_data dataa = getClockData(mElapsed);
  monedilla = getCoinData(mElapsed);
  relojete = getClockData(mElapsed);
  mineria = getMiningData(mElapsed);
  int horiac = dataa.currentTime.substring(0, 2).toInt();
  int lastTwoInt = atoi(data.timeMining.c_str() + data.timeMining.length() - 2);
  incrementCounter();
  actualizarcalen = 0;
  actualizarc = 0;
  actual = 0;
  actuanot = 0;
  correccion = 0;
  background.pushImage(0, 0, MinerWidth, MinerHeight, MinerScreen);
  Serial.printf("M8AX - >>> Completados %s Share(s), %s Khashes, Prom. Hashrate %s KH/s %s°\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str(), data.temp.c_str());
  // Total Mhashes
  render.setFontSize(18);
  render.rdrawString(data.totalMHashes.c_str(), 268, 139, TFT_BLACK);
  // Block templates
  render.setFontSize(18);
  render.drawString(data.templates.c_str(), 186, 20, 0xDEDB);
  // Best diff
  render.drawString(data.bestDiff.c_str(), 186, 47, 0xDEDB);
  // 32Bit shares
  render.setFontSize(18);
  render.drawString(data.completedShares.c_str(), 186, 74, 0xDEDB);
  // Hores
  render.setFontSize(14);
  render.rdrawString(data.timeMining.c_str(), 315, 104, 0xDEDB);
  // By M8AX
  if (lastTwoInt % 30 == 0)
  {
    colorrrr = esp_random() % 9;
  }
  if (horiac >= 22 || horiac < 8)
  {
    background.setFreeFont(FSSBO9);
    background.setTextSize(0);
    background.setTextDatum(TL_DATUM);
    background.setTextColor(TFT_BLACK);
  }
  else
  {
    background.setFreeFont(FSSBO9);
    background.setTextSize(0);
    background.setTextDatum(TL_DATUM);
    background.setTextColor(coloris[colorrrr]);
  }
  background.drawString(nombrecillo, (nombrecillo == "EHD-MDDD") ? 20 : 23, 101, GFXFF);
  // Valid Blocks
  int validInt = atoi(data.valids.c_str());
  if (validInt == 0)
  {
    render.setFontSize(24);
    render.drawString(data.valids.c_str(), 281, 56, TFT_RED);
  }
  else
  {
    render.setFontSize(24);
    render.drawString(data.valids.c_str(), 281, 56, TFT_GREENYELLOW);
  }
  // Print Temp
  render.setFontSize(10);
  render.rdrawString(data.temp.c_str(), 239, 1, TFT_BLACK);
  render.setFontSize(4);
  render.rdrawString(String(0).c_str(), 244, 2, TFT_BLACK);
  // Print Hour
  render.setFontSize(10);
  render.rdrawString(data.currentTime.c_str(), 286, 1, TFT_BLACK);
  if (lastTwoInt % 2 == 0)
  {
    // Es par
    background.setFreeFont(FSSBO9);
    background.setTextSize(2);
    background.setTextDatum(TL_DATUM);
    background.setTextColor(colors[colorIndex]);
    if ((esp_random() % 10 + 1) % 2 == 0)
    {
      background.drawString("...", 275, 68, GFXFF);
    }
    else
    {
      background.drawString(". .", 275, 68, GFXFF);
    }
  }
  else
  {
    // Es impar
    background.setFreeFont(FSSBO9);
    background.setTextSize(2);
    background.setTextDatum(TL_DATUM);
    background.setTextColor(colors[colorIndex]);
    if ((esp_random() % 10 + 1) % 2 == 0)
    {
      background.drawString(".  ", 275, 68, GFXFF);
    }
    else
    {
      background.drawString("   ", 275, 68, GFXFF);
    }
  }
  // Hashrate Day And Night
  if (horiac >= 22 || horiac < 8)
  {
    render.setFontSize(30);
    render.setFontColor(TFT_BLACK);
    render.rdrawString(data.currentHashRate.c_str(), 128, 120, TFT_BLACK);
    background.pushSprite(0, 0);
    tft.setCursor(4, 160);
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE);
    tft.print("Max HR " + String(maxkh) + " Min HR " + String(minkh));
    tft.setTextColor(TFT_ORANGE);
    tft.setCursor(137, 8);
    tft.print("RSSI " + String(WiFi.RSSI()));
  }
  else
  {
    render.setFontSize(30);
    render.setFontColor(TFT_WHITE);
    render.rdrawString(data.currentHashRate.c_str(), 128, 120, TFT_WHITE);
    background.pushSprite(0, 0);
    tft.setCursor(4, 160);
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE);
    tft.print("Max HR " + String(maxkh) + " Min HR " + String(minkh));
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(137, 8);
    tft.print("RSSI " + String(WiFi.RSSI()));
  }
}

/*
  Función: tDisplay_ClockScreen
  Propósito: Mostrar la pantalla del reloj con datos en tiempo real, incluyendo la hora, fecha, precio del BTC, hashrate y fase lunar.

  Descripción:
    Esta función actualiza la pantalla con información relevante del reloj y la minería, incluyendo la hora actual, la fecha con el día de la semana, el hashrate, el precio del BTC y la fase de la luna.
    La pantalla de fondo se actualiza antes de dibujar los textos, asegurando una visualización clara de la información.

  Proceso:
    1. **Obtención de datos**:
       Se recuperan los datos de la minería, el reloj y la moneda usando `getClockData()`, `getMiningData()` y `getCoinData()`.

    2. **Renderización del fondo**:
       Se dibuja la imagen de fondo de la pantalla de reloj con `background.pushImage()`.

    3. **Muestra de hashrate**:
       Se imprime el hashrate en negro en la pantalla utilizando `render.rdrawString()`.

    4. **Muestra del precio del BTC**:
       Se imprime el precio actual del BTC en la parte superior derecha de la pantalla.

    5. **Muestra de la altura del bloque**:
       Se imprime la altura actual del bloque minado usando `render.rdrawString()`.

    6. **Muestra de la hora y la fecha**:
       - La hora actual se imprime con una fuente grande y en color blanco.
       - La fecha incluye el día de la semana obtenido con `obtenerDiaSemana()` y se imprime en color basado en `colors[colorIndex]`.

    7. **Cálculo de la fase de la luna**:
       - Se extrae la fecha y la hora actual en variables separadas.
       - Se usa la estructura `tm` y `mktime()` para convertir la fecha a formato `time_t`.
       - Se obtiene la fase de la luna con `myMoonPhase.getPhase()`.
       - Se calcula el porcentaje iluminado de la luna y se imprime en la pantalla.

    8. **Actualización final**:
       Se actualiza la pantalla con `background.pushSprite()` para mostrar todos los datos en tiempo real.

  Efectos secundarios:
    - Se modifica el fondo de pantalla con la imagen de `minerClockScreen`.
    - Se cambian los valores mostrados según la fecha, la hora y los datos obtenidos en tiempo real.
    - Se usa un color dinámico para algunos textos según el `colorIndex`.

  Notas:
    - La función `obtenerDiaSemana()` convierte una fecha en string en el nombre del día de la semana correspondiente.
    - Se usa `mktime()` para calcular la fecha en `time_t`, lo que permite obtener la fase lunar correctamente.
    - `esp_random()` no se usa aquí, pero podría añadirse para variar la visualización de ciertos elementos dinámicos.

*/

void tDisplay_ClockScreen(unsigned long mElapsed)
{
  clock_data data = getClockData(mElapsed);
  mining_data dataa = getMiningData(mElapsed);
  monedilla = getCoinData(mElapsed);
  relojete = getClockData(mElapsed);
  mineria = getMiningData(mElapsed);
  moonData_t moon;
  int horiac = dataa.currentTime.substring(0, 2).toInt();
  unsigned long segundo = timeClient.getSeconds();
  actualizarcalen = 0;
  actualizarc = 0;
  actual = 0;
  actuanot = 0;
  correccion = 0;
  incrementCounter();
  // Print background screen
  background.pushImage(0, 0, minerClockWidth, minerClockHeight, minerClockScreen);
  Serial.printf("M8AX - >>> Completados %s Share(s), %s Khashes, Prom. Hashrate %s KH/s %s°\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str(), dataa.temp.c_str());
  // Print BTC Price
  background.setFreeFont(FSSB9);
  background.setTextSize(1);
  background.setTextDatum(TL_DATUM);
  background.setTextColor(TFT_BLACK);
  background.drawString(data.btcPrice.c_str(), 198, 3, GFXFF);
  // Print BlockHeight
  render.setFontSize(18);
  render.rdrawString(data.blockHeight.c_str(), 254, 139, TFT_BLACK);
  // Print Hour And Date
  background.setFreeFont(FF23);
  background.setTextSize(2);
  background.setTextColor(TFT_WHITE);
  background.drawString(data.currentTime.c_str(), 130, 29, GFXFF);
  background.setFreeFont(FSSBO9);
  background.setTextSize(1);
  background.setTextDatum(TL_DATUM);
  background.setTextColor(colors[colorIndex]);
  std::string quediase = obtenerDiaSemana(std::string(data.currentDate.c_str()));
  String fechacondiasemana = data.currentDate + " " + String(quediase.c_str());
  background.drawString(fechacondiasemana, 164, 86, GFXFF);
  int dia = data.currentDate.substring(0, 2).toInt();
  int mes = data.currentDate.substring(3, 5).toInt();
  int anio = data.currentDate.substring(6, 10).toInt();
  int hora = data.currentTime.substring(0, 2).toInt();
  int minuto = data.currentTime.substring(3, 5).toInt();
  // Inicializar la estructura tm
  struct tm timeinfo;
  timeinfo.tm_year = anio - 1900; // Año desde 1900
  timeinfo.tm_mon = mes - 1;      // Mes (0 = enero)
  timeinfo.tm_mday = dia;         // Día del mes
  timeinfo.tm_hour = hora;        // Hora
  timeinfo.tm_min = minuto;       // Minutos
  timeinfo.tm_sec = segundo;      // Segundos
  timeinfo.tm_isdst = -1;         // Determina si es horario de verano (automático)
  // Convertir a time_t
  time_t cadenaDeTiempo = mktime(&timeinfo);
  moon = mymoonPhase.getPhase(cadenaDeTiempo);
  double porcentajeIluminado = moon.percentLit * 100;
  char porcentajeTexto[10];
  snprintf(porcentajeTexto, sizeof(porcentajeTexto), "%.2f%%", porcentajeIluminado);
  background.setFreeFont(FSSBO9);
  background.setTextSize(1);
  background.setTextDatum(TL_DATUM);
  background.setTextColor(colors[colorIndex]);
  String textoFinal = "Luna.Ilu - " + String(porcentajeTexto);
  background.drawString(textoFinal, 156, 106, GFXFF);
  // Hashrate
  render.setFontSize(21);
  render.setCursor(19, 126);
  if (horiac >= 22 || horiac < 8)
  {
    render.setFontColor(TFT_BLACK);
    render.rdrawString(data.currentHashRate.c_str(), 94, 133, TFT_BLACK);
    background.pushSprite(0, 0);
    tft.setCursor(4, 123);
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE);
    tft.print("+ " + String(maxkh) + " - " + String(minkh));
  }
  else
  {
    render.setFontColor(TFT_WHITE);
    render.rdrawString(data.currentHashRate.c_str(), 94, 133, TFT_WHITE);
    background.pushSprite(0, 0);
    tft.setCursor(4, 123);
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE);
    tft.print("+ " + String(maxkh) + " - " + String(minkh));
  }
}

/*
  Función: tDisplay_GlobalHashScreen
  Propósito: Mostrar información global de minería, incluyendo el precio del BTC, hashrate global, dificultad de red y progreso del bloque.

  Descripción:
    Esta función actualiza la pantalla con información en tiempo real relacionada con el estado de la red de minería.
    Muestra el precio del BTC, el hashrate global, la altura del bloque, la dificultad de la red y el porcentaje de progreso hacia el siguiente bloque.

  Proceso:
    1. **Obtención de datos**:
       Se recuperan los datos de la moneda (`getCoinData()`), minería (`getMiningData()`) y reloj (`getClockData()`).

    2. **Renderización del fondo**:
       Se dibuja la imagen de fondo de la pantalla con `background.pushImage()`.

    3. **Impresión de datos clave**:
       - **Precio del BTC**: Se imprime en la parte superior derecha.
       - **Hora actual**: Se imprime en la parte superior izquierda en color blanco.
       - **Hashrate actual**: Se muestra en un color dinámico basado en `colorIndex`.
       - **Último bloque de la pool**: Se imprime con color `0x9C92` en la parte derecha de la pantalla.
       - **Dificultad de la red**: Se imprime en la parte derecha con el mismo color `0x9C92`.
       - **Hashrate global**: Se imprime en la parte inferior en negro.
       - **Altura del bloque**: Se muestra en grande en el centro de la pantalla.

    4. **Dibujo del rectángulo de progreso**:
       - Se calcula la longitud del rectángulo en función del porcentaje de progreso del bloque (`progressPercent`).
       - Se usa `fillRect()` para representar gráficamente el progreso hacia el siguiente bloque.

    5. **Impresión de bloques restantes**:
       - Se imprime el número de bloques restantes hasta el próximo ajuste de dificultad.

    6. **Actualización final**:
       - Se actualiza la pantalla con `background.pushSprite()` para mostrar todos los datos en tiempo real.

  Efectos secundarios:
    - Se modifica el fondo de pantalla con `globalHashScreen`.
    - Se cambia dinámicamente el color del hashrate en función de `colorIndex`.
    - Se actualiza el progreso del bloque de forma visual con una barra de progreso.

  Notas:
    - `data.progressPercent` determina el avance visual del bloque.
    - `colorIndex` se actualiza con `incrementCounter()`, lo que puede modificar los colores en cada actualización.
    - `data.globalHashRate` representa la potencia total de cómputo de la red en KH/s.

*/

void tDisplay_GlobalHashScreen(unsigned long mElapsed)
{
  coin_data data = getCoinData(mElapsed);
  mining_data dataa = getMiningData(mElapsed);
  monedilla = getCoinData(mElapsed);
  relojete = getClockData(mElapsed);
  mineria = getMiningData(mElapsed);
  incrementCounter();
  actualizarcalen = 0;
  // Print background screen
  background.pushImage(0, 0, globalHashWidth, globalHashHeight, globalHashScreen);
  Serial.printf("M8AX - >>> Completados %s Share(s), %s Khashes, Prom. Hashrate %s KH/s %s°\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str(), dataa.temp.c_str());
  // Print BTC Price
  background.setFreeFont(FSSB9);
  background.setTextSize(1);
  background.setTextDatum(TL_DATUM);
  background.setTextColor(TFT_BLACK);
  background.drawString(data.btcPrice.c_str(), 198, 3, GFXFF);
  // Print Hour
  background.setFreeFont(FSSB9);
  background.setTextSize(1);
  background.setTextDatum(TL_DATUM);
  background.setTextColor(TFT_WHITE);
  background.drawString(data.currentTime.c_str(), 46, 0, GFXFF);
  // Print HR
  background.setFreeFont(FSSB9);
  background.setTextSize(1);
  background.setTextDatum(TL_DATUM);
  background.setTextColor(colors[colorIndex]);
  background.drawString(String(data.currentHashRate) + " KH/s", 1, 64, GFXFF);
  // Print Last Pool Block
  background.setFreeFont(FSS9);
  background.setTextDatum(TR_DATUM);
  background.setTextColor(0x9C92);
  background.drawString(data.halfHourFee.c_str(), 302, 52, GFXFF);
  // Print Difficulty
  background.setFreeFont(FSS9);
  background.setTextDatum(TR_DATUM);
  background.setTextColor(0x9C92);
  background.drawString(data.netwrokDifficulty.c_str(), 302, 88, GFXFF);
  // Print Global Hashrate
  render.setFontSize(17);
  render.rdrawString(data.globalHashRate.c_str(), 274, 142, TFT_BLACK);
  // Print BlockHeight
  render.setFontSize(28);
  render.rdrawString(data.blockHeight.c_str(), 140, 104, 0xDEDB);
  // Draw percentage rectangle
  int x2 = 2 + (138 * data.progressPercent / 100);
  background.fillRect(2, 149, x2, 168, 0xDEDB);
  // Print Remaining BLocks
  background.setTextFont(FONT2);
  background.setTextSize(1);
  background.setTextDatum(MC_DATUM);
  background.setTextColor(TFT_BLACK);
  background.drawString(data.remainingBlocks.c_str(), 72, 159, FONT2);
  // Push prepared background to screen
  background.pushSprite(0, 0);
}

/**
 * Actualiza la pantalla con datos en tiempo real sobre minería de Bitcoin.
 *
 * Esta función obtiene y muestra información relevante en la pantalla, incluyendo:
 * - Hashrate actual del minero.
 * - Altura del bloque en la blockchain.
 * - Hora actual con segundos sincronizados.
 * - Precio de Bitcoin en tiempo real.
 * - Temperatura del minero.
 * - Un mensaje que cambia cada 30 segundos.
 *
 * @param mElapsed Tiempo transcurrido en milisegundos desde la última actualización.
 */

void tDisplay_BTCprice(unsigned long mElapsed)
{
  clock_data data = getClockData(mElapsed);
  mining_data dataa = getMiningData(mElapsed);
  monedilla = getCoinData(mElapsed);
  relojete = getClockData(mElapsed);
  mineria = getMiningData(mElapsed);
  unsigned long segundo = timeClient.getSeconds();
  int segundos = segundo % 60;
  int horiac = dataa.currentTime.substring(0, 2).toInt();
  incrementCounter();
  actualizarcalen = 0;
  actualizarc = 0;
  actual = 0;
  actuanot = 0;
  correccion = 0;
  // Print background screen
  background.pushImage(0, 0, priceScreenWidth, priceScreenHeight, priceScreen);
  Serial.printf("M8AX - >>> Completados %s Share(s), %s Khashes, Prom. Hashrate %s KH/s %s°\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str(), dataa.temp.c_str());
  // Hashrate
  render.setFontSize(22);
  if (horiac >= 22 || horiac < 8)
  {
    render.setFontColor(TFT_BLACK);
    render.rdrawString(data.currentHashRate.c_str(), 94, 131, TFT_BLACK);
  }
  else
  {
    render.setFontColor(TFT_WHITE);
    render.rdrawString(data.currentHashRate.c_str(), 94, 131, TFT_WHITE);
  }
  // Print BlockHeight
  render.setFontSize(18);
  render.rdrawString(data.blockHeight.c_str(), 254, 138, TFT_WHITE);
  // Print Hour
  background.setFreeFont(FSSB9);
  background.setTextSize(1);
  background.setTextDatum(TL_DATUM);
  background.setTextColor(TFT_BLACK);
  char segundosFormateados[3];
  sprintf(segundosFormateados, "%02d", segundos);
  background.drawString(data.currentTime.c_str() + String(":") + String(segundosFormateados), 177, 3, GFXFF);
  background.drawString(dataa.temp.c_str(), 256, 3, GFXFF);
  render.setFontSize(4);
  render.rdrawString(String(0).c_str(), 281, 5, TFT_BLACK);
  // Print BTC Price
  background.setFreeFont(FSSB9);
  background.setTextDatum(TR_DATUM);
  background.setTextSize(2);
  background.setTextColor(TFT_WHITE);
  background.drawString(data.btcPrice.c_str(), 245, 26, GFXFF);
  background.setTextSize(1);
  background.setTextColor(colors[colorIndex]);

  if (segundos <= 30)
  {
    textoFinalm8ax1 = "El Futuro No Esta";
    textoFinalm8ax2 = "Establecido, Solo";
    textoFinalm8ax3 = "Existe El Que";
    textoFinalm8ax4 = "Nosotros Hacemos.";
  }
  else
  {
    textoFinalm8ax1 = "Por Muchas Vueltas";
    textoFinalm8ax2 = "Que Demos, Siempre";
    textoFinalm8ax3 = "Tendremos El";
    textoFinalm8ax4 = "Culo Atras...";
  }

  background.drawString(textoFinalm8ax1.c_str(), 310, 60, GFXFF);
  background.drawString(textoFinalm8ax2.c_str(), 310, 76, GFXFF);
  background.drawString(textoFinalm8ax3.c_str(), 310, 95, GFXFF);
  background.drawString(textoFinalm8ax4.c_str(), 310, 111, GFXFF);
  background.pushSprite(0, 0);
  tft.setCursor(4, 123);
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);
  tft.print("+ " + String(maxkh) + " - " + String(minkh));
  tft.setCursor(4, 162);
  tft.print("BTC 24H - " + subebaja);
}

/*
  Función: tDisplay_m8axScreen1
  Propósito: Mostrar información sobre la fecha, hora, estado de la minería y fase de la luna en la pantalla.

  Descripción:
    Esta función obtiene los datos actuales de minería, hora y fecha, calcula la fase de la luna, y los muestra en la pantalla junto con una imagen de fondo.

  Proceso:
    1. **Obtención de datos**:
       - Se recuperan datos de minería (`getMiningData()`), reloj (`getClockData()`) y moneda (`getCoinData()`).
       - Se obtiene el día de la semana con `obtenerDiaSemana()` basado en la fecha actual.

    2. **Construcción de cadenas de texto**:
       - Se genera la fecha completa con el día de la semana (`fechacondiasemana`).
       - Se muestra la fecha en la pantalla con `background.drawString()`.

    3. **Conversión de fecha y hora a estructura `tm`**:
       - Se extraen día, mes, año, hora y minutos desde las cadenas `dataa.currentDate` y `dataa.currentTime`.
       - Se llena la estructura `tm` con estos valores y se convierte a `time_t` con `mktime()`.

    4. **Cálculo de la fase lunar**:
       - Se obtiene la fase de la luna con `mymoonPhase.getPhase(cadenaDeTiempo)`.
       - Se calcula el porcentaje de iluminación de la luna y se convierte a texto (`porcentajeTexto`).
       - Se construye la cadena `datoslunarelogrande` con la información lunar.

    5. **Renderización de la interfaz**:
       - Se imprime la imagen de fondo `ImagenM8AX` en la pantalla.
       - Se muestra el estado de minería en el puerto serie con `Serial.printf()`.
       - Se imprimen en la pantalla:
         - **Hora actual** (con fuente grande).
         - **Fecha completa** con el día de la semana.
         - **Estado de iluminación lunar**.

    6. **Actualización final**:
       - Se usa `background.pushSprite(0, 0);` para refrescar la pantalla con los nuevos datos.

  Notas:
    - `data.currentTime` muestra la hora actual en formato HH:MM.
    - `dataa.currentDate` es la fecha en formato DD/MM/YYYY.
    - `mymoonPhase.getPhase()` calcula la fase lunar basada en `time_t`.
    - `percentLit` indica el porcentaje de iluminación de la luna.

  Efectos secundarios:
    - Se actualizan dinámicamente la hora, fecha y fase lunar en pantalla.
    - Se imprime información de minería en el puerto serie.

*/

void tDisplay_m8axScreen1(unsigned long mElapsed)
{
  mining_data data = getMiningData(mElapsed);
  clock_data dataa = getClockData(mElapsed);
  monedilla = getCoinData(mElapsed);
  relojete = getClockData(mElapsed);
  mineria = getMiningData(mElapsed);
  moonData_t moon;
  incrementCounter();
  actualizarcalen = 0;
  actualizarc = 0;
  mirarTiempo = 0;
  std::string quediase = obtenerDiaSemana(std::string(dataa.currentDate.c_str()));
  String fechita = dataa.currentDate + " " + String(quediase.c_str());
  String fechacondiasemana = dataa.currentDate + " " + String(quediase.c_str());
  background.drawString(fechacondiasemana, 164, 86, GFXFF);
  int dia = dataa.currentDate.substring(0, 2).toInt();
  int mes = dataa.currentDate.substring(3, 5).toInt();
  int anio = dataa.currentDate.substring(6, 10).toInt();
  int hora = dataa.currentTime.substring(0, 2).toInt();
  int minuto = dataa.currentTime.substring(3, 5).toInt();
  unsigned long segundo = timeClient.getSeconds();
  // Inicializar la estructura tm
  struct tm timeinfo;
  timeinfo.tm_year = anio - 1900; // Año desde 1900
  timeinfo.tm_mon = mes - 1;      // Mes (0 = enero)
  timeinfo.tm_mday = dia;         // Día del mes
  timeinfo.tm_hour = hora;        // Hora
  timeinfo.tm_min = minuto;       // Minutos
  timeinfo.tm_sec = segundo;      // Segundos
  timeinfo.tm_isdst = -1;         // Determina si es horario de verano (automático)
  // Convertir a time_t
  time_t cadenaDeTiempo = mktime(&timeinfo);
  moon = mymoonPhase.getPhase(cadenaDeTiempo);
  double porcentajeIluminado = moon.percentLit * 100;
  char porcentajeTexto[10];
  snprintf(porcentajeTexto, sizeof(porcentajeTexto), "%.2f%%", porcentajeIluminado);
  String datoslunarelogrande = "Luna.Ilu - " + String(porcentajeTexto);
  background.pushImage(0, 0, ImagenM8AXWidth, ImagenM8AXHeight, ImagenM8AX);
  Serial.printf("M8AX - >>> Completados %s Share(s), %s Khashes, Prom. Hashrate %s KH/s %s°\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str(), data.temp.c_str());
  background.setFreeFont(FSSB9);
  background.setTextSize(7);
  background.setTextColor(colors[colorIndex]);
  background.drawString(data.currentTime.c_str(), 0, 33, GFXFF);
  background.setTextSize(2);
  background.drawString(fechita, 34, 0, GFXFF);
  background.drawString(datoslunarelogrande, 12, 140, GFXFF);
  background.pushSprite(0, 0);
}

/*
 * Función: tDisplay_m8axScreen2
 * ------------------------------------
 *  Esta función actualiza la pantalla con información de minería y reloj.
 *
 *  Parámetro:
 *    - mElapsed: Tiempo transcurrido desde la última actualización.
 *
 *  Flujo de ejecución:
 *  1. Obtiene los datos de minería, moneda y reloj.
 *  2. Reinicia varias variables de control usadas en la lógica de actualización.
 *  3. Dibuja la imagen de fondo de la pantalla.
 *  4. Muestra en la consola (Serial) la información de minería:
 *      - Shares completados
 *      - Total de Khashes minados
 *      - Hashrate promedio
 *      - Temperatura
 *  5. Comprueba si el usuario ha minado alguna recompensa:
 *      - Si no ha obtenido validaciones, muestra el mensaje "AUN NO ERES RICO".
 *      - Si ha obtenido al menos una validación, muestra "ERES MILLONARIO".
 *  6. Configura el estilo del texto en la pantalla.
 *  7. Muestra en pantalla:
 *      - Hashrate actual en la parte superior.
 *      - Mensaje sobre la temperatura.
 *      - Dirección de BTC parcialmente oculta con un identificador.
 *      - Dirección y puerto de la pool.
 *      - Otros valores visuales relacionados con la minería.
 *  8. Muestra la hora actual y el tiempo total de minería.
 */

void tDisplay_m8axScreen2(unsigned long mElapsed)
{
  mining_data data = getMiningData(mElapsed);
  monedilla = getCoinData(mElapsed);
  relojete = getClockData(mElapsed);
  mineria = getMiningData(mElapsed);
  incrementCounter();
  actualizarcalen = 0;
  actualizarc = 0;
  actual = 0;
  actuanot = 0;
  correccion = 0;
  background.pushSprite(0, 0);
  background.pushImage(0, 0, ImagenM8AXWidth, ImagenM8AXHeight, ImagenM8AX);
  Serial.printf("M8AX - >>> Completados %s Share(s), %s Khashes, Prom. Hashrate %s KH/s %s°\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str(), data.temp.c_str());

  int millonario = atoi(data.valids.c_str());
  const char *mensajericono;
  if (millonario == 0)
  {
    mensajericono = "--- AUN NO ERES RICO ---";
  }
  else
  {
    mensajericono = "--- ERES MILLONARIO ---";
  }
  background.setFreeFont(FSSB9);
  background.setTextSize(6);
  background.setTextColor(colors[colorIndex]);
  background.drawString(data.currentHashRate.c_str(), 0, 2, GFXFF);
  background.setFreeFont(FSSBO9);
  background.setTextSize(1);
  background.setTextDatum(TL_DATUM);
  background.setTextColor(colors[colorIndex]);
  tft.setTextColor(colors[colorIndex]);
  background.drawString("TEMP", 16, 93, GFXFF);
  background.drawString(mensajericono, 80, 150, GFXFF);
  background.setTextSize(2);
  background.drawString(data.temp.c_str(), 15, 111, GFXFF);
  background.setTextSize(1);
  String wallet = Settings.BtcWallet;
  int puntoIndex = wallet.indexOf('.');
  String direccion_btc;
  String identificador;
  String nueva_wallet;
  if (puntoIndex != -1)
  {
    // Si hay punto, separar dirección e identificador
    direccion_btc = wallet.substring(0, puntoIndex);
    identificador = wallet.substring(puntoIndex + 1);
    // Generar la nueva wallet con identificador + últimos 6 caracteres
    nueva_wallet = identificador + direccion_btc.substring(direccion_btc.length() - 6);
  }
  else
  {
    // Si no hay punto, toda la cadena es la dirección BTC
    direccion_btc = wallet;
    // Generar la nueva wallet con los últimos 12 caracteres
    nueva_wallet = "xxxxxxxxxxxx" + direccion_btc.substring(direccion_btc.length() - 12);
  }
  tft.setCursor(82, 117);
  tft.print("WBTC " + nueva_wallet);
  background.drawString(Settings.PoolAddress + ":" + String(Settings.PoolPort), 80, 127, GFXFF);
  background.drawString("c", 62, 107, GFXFF);
  background.setTextColor(colors[colorIndex]);
  background.drawString("d", 125, 92, GFXFF);
  background.drawString("KH/s", 275, 87, GFXFF);
  background.drawString(relojete.currentTime, 12, 150, GFXFF);
  render.setCursor(11, 128);
  render.setFontSize(14);
  render.rdrawString(data.timeMining.c_str(), 232, 92, colors[colorIndex]);
}

/*
 * Función: tDisplay_m8axScreen3
 * ------------------------------------
 *  Esta función actualiza la pantalla con información del reloj y el calendario.
 *
 *  Parámetro:
 *    - mElapsed: Tiempo transcurrido desde la última actualización.
 *
 *  Flujo de ejecución:
 *  1. Obtiene los datos de:
 *      - Reloj (fecha y hora).
 *      - Minería (estadísticas de hashrate y shares).
 *      - Moneda y reloj adicionales.
 *  2. Incrementa el contador de actualización y reinicia la variable de corrección.
 *  3. Muestra en la consola (Serial) los datos de minería:
 *      - Shares completados.
 *      - Total de Khashes minados.
 *      - Hashrate promedio.
 *      - Temperatura del dispositivo.
 *  4. Extrae la fecha actual y la convierte a valores enteros:
 *      - Día, mes y año.
 *  5. Extrae los dígitos individuales de la hora y los minutos.
 *  6. Cada 30 ciclos (o la primera vez que se ejecuta):
 *      - Dibuja la imagen de fondo.
 *      - Llama a `mostrarCalendario()` para visualizar la fecha y hora de forma gráfica.
 *  7. Incrementa los contadores de actualización para controlar la frecuencia de actualización del calendario.
 */

void tDisplay_m8axScreen3(unsigned long mElapsed)
{
  clock_data dataa = getClockData(mElapsed);
  mining_data data = getMiningData(mElapsed);
  monedilla = getCoinData(mElapsed);
  relojete = getClockData(mElapsed);
  mineria = getMiningData(mElapsed);
  incrementCounter();
  correccion = 0;
  Serial.printf("M8AX - >>> Completados %s Share(s), %s Khashes, Prom. Hashrate %s KH/s %s°\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str(), data.temp.c_str());
  int dia = dataa.currentDate.substring(0, 2).toInt();
  int mes = dataa.currentDate.substring(3, 5).toInt();
  int anio = dataa.currentDate.substring(6, 10).toInt();
  int num1 = dataa.currentTime.charAt(0) - '0'; // Primer dígito de la hora
  int num2 = dataa.currentTime.charAt(1) - '0'; // Segundo dígito de la hora
  int num3 = dataa.currentTime.charAt(3) - '0'; // Primer dígito de los minutos
  int num4 = dataa.currentTime.charAt(4) - '0'; // Segundo dígito de los minutos
  if (actualizarcalen % 30 == 0 || actual == 0)
  {
    background.pushImage(0, 0, ImagenM8AXWidth, ImagenM8AXHeight, ImagenM8AX);
    background.pushSprite(0, 0);
    mostrarCalendario(dia, mes, anio, num1, num2, num3, num4);
  }
  actualizarcalen++;
  actual++;
  actualizarc = 0;
}

/*
 * Función: tDisplay_m8axScreen4
 * ------------------------------------
 *  Esta función actualiza la pantalla con información de minería, un número aleatorio primo y datos del sistema.
 *
 *  Parámetro:
 *    - mElapsed: Tiempo transcurrido desde la última actualización.
 *
 *  Flujo de ejecución:
 *  1. Obtiene los datos de:
 *      - Minería (estadísticas de hashrate y shares).
 *      - Moneda.
 *      - Reloj.
 *  2. Incrementa el contador de actualización y reinicia la variable `actualizarcalen`.
 *  3. Genera un número aleatorio (`rndnumero`) usando `esp_random()`.
 *  4. Muestra en la consola (Serial) los datos de minería:
 *      - Shares completados.
 *      - Total de Khashes minados.
 *      - Hashrate promedio.
 *      - Temperatura del dispositivo.
 *  5. Si el número aleatorio es primo (`esPrimo(rndnumero)`):
 *      - Muestra una pantalla de créditos.
 *      - Dibuja el número primo en la pantalla.
 *  6. Cada 30 ciclos (`actualizarc % 30 == 0`):
 *      - Muestra la pantalla de créditos.
 *      - Genera y muestra números de la lotería "Primitiva".
 *      - Divide los números en dos grupos de tres para mostrarlos en la pantalla.
 *      - Muestra datos del sistema, como:
 *          - Hora actual.
 *          - Dirección IP pública y local.
 *          - Espacio libre en la memoria Flash.
 *          - Memoria RAM total (Heap + PSRAM).
 *          - Frecuencia de la CPU y número de núcleos.
 *  7. Incrementa `actualizarc` para controlar la frecuencia de actualización.
 */

void tDisplay_m8axScreen4(unsigned long mElapsed)
{
  mining_data data = getMiningData(mElapsed);
  monedilla = getCoinData(mElapsed);
  relojete = getClockData(mElapsed);
  mineria = getMiningData(mElapsed);
  incrementCounter();
  actualizarcalen = 0;
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(1);
  rndnumero = esp_random();
  Serial.printf("M8AX - >>> Completados %s Share(s), %s Khashes, Prom. Hashrate %s KH/s %s°\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str(), data.temp.c_str());
  if (esPrimo(rndnumero))
  {
    background.pushImage(0, 0, CreditosScreenWidth, CreditosScreenHeight, CreditosScreen);
    String rndnumeroStr = String(rndnumero);
    tft.fillRect(167, 137, 123, 19, TFT_BLACK);
    tft.setCursor(169, 139);
    tft.setTextSize(2);
    tft.print(rndnumeroStr.c_str());
  }
  if (actualizarc % 30 == 0)
  {
    background.pushImage(0, 0, CreditosScreenWidth, CreditosScreenHeight, CreditosScreen);
    background.pushSprite(0, 0);
    String numerosp = generarNumerosPrimitiva();
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(1);
    String primeros_tres = "";
    String ultimos_tres = "";
    int comaIndex = numerosp.indexOf(',');
    // Primeros tres números
    primeros_tres = numerosp.substring(0, comaIndex); // Extrae los tres primeros números
    numerosp.remove(0, comaIndex + 1);                // Elimina lo que ya hemos extraído (primeros tres)

    comaIndex = numerosp.indexOf(',');
    primeros_tres += "," + numerosp.substring(0, comaIndex); // Agregar el segundo número
    numerosp.remove(0, comaIndex + 1);                       // Elimina el segundo número

    comaIndex = numerosp.indexOf(',');
    primeros_tres += "," + numerosp.substring(0, comaIndex); // Agregar el tercer número
    numerosp.remove(0, comaIndex + 1);                       // Elimina el tercer número

    // Últimos tres números
    ultimos_tres = numerosp.substring(0); // Lo que queda es la parte de los últimos tres números
    tft.setCursor(27, 123);
    tft.print(primeros_tres);
    tft.setCursor(27, 134);
    tft.print(ultimos_tres);
    tft.setCursor(212, 4);
    tft.setTextColor(colors[colorIndex]);
    tft.setTextSize(2);
    tft.print(data.currentTime.c_str());
    tft.setTextSize(1);
    tft.setCursor(225, 28);
    tft.print("PUBLIC IP");
    tft.setCursor(225, 39);
    tft.print(getPublicIP());
    tft.setCursor(225, 50);
    tft.print("LOCAL IP");
    tft.setCursor(225, 61);
    tft.print(WiFi.localIP());
    tft.setCursor(225, 72);
    tft.print("FREE FLASH");
    tft.setCursor(225, 83);
    tft.println(ESP.getFreeSketchSpace());
    tft.setCursor(225, 94);
    tft.print("RAM TOTAL");
    unsigned long ramTotal = ESP.getHeapSize();
    unsigned long psramTotal = ESP.getPsramSize(); // Método directo para PSRAM
    tft.setCursor(225, 105);
    tft.print(ramTotal + psramTotal);
    tft.setCursor(225, 116);
    tft.print("CPU: ");
    tft.print(getCpuFrequencyMhz());
    tft.print(" MHz,");
    tft.print(ESP.getChipCores());
    tft.print("C");
  }
  actualizarc++;
}

/*
 * Función: tDisplay_m8axScreen5
 * ------------------------------------
 *  Esta función actualiza la pantalla con información de minería, elige una imagen aleatoria y muestra el texto desplazándose.
 *
 *  Parámetro:
 *    - mElapsed: Tiempo transcurrido desde la última actualización.
 *
 *  Flujo de ejecución:
 *  1. Incrementa el contador global `incrementCounter()`.
 *  2. Reinicia `actualizarcalen` a 0.
 *  3. Obtiene datos de:
 *      - Minería (shares, hashrate, temperatura).
 *      - Moneda.
 *      - Reloj.
 *  4. Formatea los datos de minería en un buffer (`bbuffer`) y los imprime en la consola (Serial).
 *  5. Si `columna` es 0, se genera un número aleatorio (`random_number`) entre 1 y 4 para seleccionar una imagen.
 *  6. Dependiendo del número aleatorio:
 *      - `1`: Muestra `ImagenM8AX`.
 *      - `2`: Muestra `M8AXRelojLunar`.
 *      - `3`: Muestra `M8AXQuote1`.
 *      - `4`: Muestra `M8AXQuote2`.
 *  7. Muestra el texto con el color de índice `colorIndex`, ajusta el tamaño del texto y lo imprime en la posición `(2, columna)`.
 *  8. Incrementa `columna` para el desplazamiento del texto en la pantalla.
 *  9. Si `columna` alcanza 166, se reinicia a 0 para repetir el ciclo.
 */

void tDisplay_m8axScreen5(unsigned long mElapsed)
{
  incrementCounter();
  actualizarcalen = 0;
  mining_data data = getMiningData(mElapsed);
  monedilla = getCoinData(mElapsed);
  relojete = getClockData(mElapsed);
  mineria = getMiningData(mElapsed);
  char bbuffer[100];
  snprintf(bbuffer, sizeof(bbuffer), "C. %s Share(s) %s KH. M-HR %s KH/s - %sg",
           data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str(), data.temp.c_str());
  Serial.printf("M8AX - >>> Completados %s Share(s), %s Khashes, Prom. Hashrate %s KH/s %s°\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str(), data.temp.c_str());
  String lineaa = bbuffer;
  if (columna == 0)
  {
    random_number = 1 + (esp_random() % 4);
  }
  if (random_number == 1)
  {
    tft.pushImage(0, 0, ImagenM8AXWidth, ImagenM8AXHeight, ImagenM8AX);
  }
  else if (random_number == 2)
  {
    tft.pushImage(0, 0, M8AXRelojLunarWidth, M8AXRelojLunarHeight, M8AXRelojLunar);
  }
  else if (random_number == 3)
  {
    tft.pushImage(0, 0, M8AXQuote1Width, M8AXQuote1Height, M8AXQuote1);
  }
  else if (random_number == 4)
  {
    tft.pushImage(0, 0, M8AXQuote2Width, M8AXQuote2Height, M8AXQuote2);
  }
  tft.setTextColor(colors[colorIndex]);
  tft.setTextSize(1);
  tft.setCursor(2, columna);
  tft.print(lineaa);
  columna += 1;
  if (columna >= 166)
  {
    columna = 0; // Reinicia la columna
  }
}

/*
 * Función: tDisplay_m8axScreen6
 * ------------------------------------
 *  Esta función actualiza la pantalla con la imagen final y muestra datos de minería.
 *
 *  Parámetro:
 *    - mElapsed: Tiempo transcurrido desde la última actualización.
 *
 *  Flujo de ejecución:
 *  1. Incrementa el contador global de actualización (`incrementCounter()`).
 *  2. Reinicia múltiples variables de control (`actualizarcalen`, `actualizarc`, `actual`, `actuanot`, `correccion`).
 *  3. Obtiene los datos de:
 *      - Minería (shares completados, hashrate, temperatura).
 *      - Información de moneda.
 *      - Hora actual del reloj.
 *  4. Imprime en la consola (Serial) las estadísticas de minería:
 *      - Shares completados.
 *      - Total de Khashes minados.
 *      - Hashrate promedio.
 *      - Temperatura del dispositivo.
 *  5. Muestra en pantalla la imagen final (`ImagenFinalPM8AX`).
 *  6. Establece el color y tamaño del texto para la información superpuesta.
 *  7. Muestra en pantalla:
 *      - La hora actual en `(64,160)`.
 *      - El hashrate en `(137,160)`, seguido de `"KH/s "`.
 *      - La temperatura en `(236,160)`, seguida de `"g"` (grados).
 */

void tDisplay_m8axScreen6(unsigned long mElapsed)
{
  incrementCounter();
  actualizarcalen = 0;
  actualizarc = 0;
  actual = 0;
  actuanot = 0;
  correccion = 0;
  mining_data data = getMiningData(mElapsed);
  monedilla = getCoinData(mElapsed);
  relojete = getClockData(mElapsed);
  mineria = getMiningData(mElapsed);
  Serial.printf("M8AX - >>> Completados %s Share(s), %s Khashes, Prom. Hashrate %s KH/s %s°\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str(), data.temp.c_str());
  tft.pushImage(0, 0, ImagenFinalPWidth, ImagenFinalPHeight, ImagenFinalPM8AX); // Muestra la imagen
  tft.setTextColor(colors[colorIndex]);
  tft.setTextSize(1);
  tft.setCursor(64, 160);
  tft.print(data.currentTime);
  tft.setCursor(137, 160);
  tft.print(data.currentHashRate + "KH/s ");
  tft.setCursor(236, 160);
  tft.print(data.temp + "g");
}

/*
 * Función: tDisplay_m8axScreen7
 * ------------------------------------
 *  Esta función actualiza la pantalla con un reloj, la fecha y los datos de minería.
 *
 *  Parámetro:
 *    - mElapsed: Tiempo transcurrido desde la última actualización.
 *
 *  Flujo de ejecución:
 *  1. Obtiene los datos actualizados de:
 *      - Minería (`data`): información sobre shares, Khashes, hashrate y temperatura.
 *      - Reloj (`dataa`): hora y fecha actual.
 *      - Moneda (`monedilla`).
 *  2. Convierte `data.valids` a un entero (`millonario`).
 *  3. Reinicia `actualizarc` y llama a `incrementCounter()`.
 *  4. Si `actualizarcalen` es 0, inicia el cliente NTP (`timeClient.begin()`).
 *  5. Cada 1800 ciclos, actualiza la hora desde el servidor NTP (`timeClient.update()`).
 *  6. Obtiene los segundos actuales del servidor NTP (`timeClient.getSeconds()`).
 *  7. Extrae y convierte la hora y los minutos de `dataa.currentTime`.
 *  8. Calcula los segundos actuales (`segundos = segundo % 60`).
 *  9. Imprime en consola los datos de minería.
 * 10. Obtiene el día de la semana con `obtenerDiaSemana()`.
 * 11. Extrae el día, mes y año de la fecha (`dataa.currentDate`).
 * 12. Convierte la hora a formato de 12 horas si es necesario.
 * 13. Incrementa `actualizarcalen`.
 * 14. Llama a `dibujarReloj()` con la hora, fecha y datos de minería para actualizar la pantalla.
 * 15. Reinicia `actual` a 0.
 */

void tDisplay_m8axScreen7(unsigned long mElapsed)
{
  mining_data data = getMiningData(mElapsed);
  clock_data dataa = getClockData(mElapsed);
  monedilla = getCoinData(mElapsed);
  relojete = getClockData(mElapsed);
  mineria = getMiningData(mElapsed);
  int millonario = atoi(data.valids.c_str());
  actualizarc = 0;
  incrementCounter();
  if (actualizarcalen == 0)
  {
    timeClient.begin();
  }
  if (actualizarcalen % 1800 == 0)
  {
    timeClient.update();
  }
  unsigned long segundo = timeClient.getSeconds();
  int horas = dataa.currentTime.substring(0, 2).toInt();
  int horras = horas;
  int minutos = dataa.currentTime.substring(3, 5).toInt();
  int segundos = segundo % 60;
  Serial.printf("M8AX - >>> Completados %s Share(s), %s Khashes, Prom. Hashrate %s KH/s %s°\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str(), data.temp.c_str());
  std::string quediase = obtenerDiaSemana(std::string(dataa.currentDate.c_str()));
  String dia = dataa.currentDate.substring(0, 2);
  String mes = dataa.currentDate.substring(3, 5);
  String anio = dataa.currentDate.substring(6, 10);

  // Ajusta al formato de 12 horas
  if (horas >= 12)
  {
    horas -= 12;
  }

  // Llama a la función para dibujar el reloj
  actualizarcalen++;
  dibujarReloj(horras, minutos, segundos, dia, mes, anio, quediase, data.currentHashRate.c_str(), data.temp.c_str(), millonario);
  actual = 0;
}

/*
 * Función: tDisplay_m8axScreen8
 * ------------------------------------
 *  Esta función actualiza la pantalla con una frase aleatoria, imagen de fondo y datos de minería.
 *
 *  Parámetro:
 *    - mElapsed: Tiempo transcurrido desde la última actualización.
 *
 *  Flujo de ejecución:
 *  1. Obtiene los datos actualizados de:
 *      - Minería (`data`): información sobre shares, Khashes, hashrate y temperatura.
 *      - Reloj (`dataa`): hora y fecha actual.
 *      - Moneda (`monedilla`).
 *  2. Llama a `incrementCounter()`.
 *  3. Imprime en consola los datos de minería.
 *  4. Si `actualizarcalen` es múltiplo de 15 o `correccion` es 0:
 *      - Aumenta el contador de frases (`numfrases`).
 *      - Elige una imagen aleatoria (1 a 4) para mostrar de fondo:
 *        1. Imagen de M8AX.
 *        2. Reloj Lunar.
 *        3. Cita 1.
 *        4. Cita 2.
 *      - Muestra la imagen seleccionada en el fondo.
 *      - Solicita una cita aleatoria a través de la función `getQuote()`.
 *      - Muestra la cita en la pantalla con `displayQuote()`.
 *      - Imprime el número de la frase.
 *  5. Establece la corrección de visualización a 1 (`correccion = 1`).
 *  6. Convierte `data.valids` a un entero (`millonario`).
 *  7. Si `millonario` es 0, imprime "NO RICO". De lo contrario, imprime "SI RICO" junto con los datos de minería (hora, fecha, hashrate, temperatura).
 *  8. Incrementa `actualizarcalen`, reinicia `actualizarc` y `actuanot`.
 */

void tDisplay_m8axScreen8(unsigned long mElapsed)
{
  mining_data data = getMiningData(mElapsed);
  clock_data dataa = getClockData(mElapsed);
  monedilla = getCoinData(mElapsed);
  relojete = getClockData(mElapsed);
  mineria = getMiningData(mElapsed);
  incrementCounter();
  Serial.printf("M8AX - >>> Completados %s Share(s), %s Khashes, Prom. Hashrate %s KH/s %s°\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str(), data.temp.c_str());
  if (actualizarcalen % 15 == 0 || correccion == 0)
  {
    numfrases++;
    int random_number = 1 + (esp_random() % 4);
    if (random_number == 1)
    {
      background.pushImage(0, 0, ImagenM8AXWidth, ImagenM8AXHeight, ImagenM8AX);
      background.pushSprite(0, 0);
    }
    else if (random_number == 2)
    {
      background.pushImage(0, 0, M8AXRelojLunarWidth, M8AXRelojLunarHeight, M8AXRelojLunar);
      background.pushSprite(0, 0);
    }
    else if (random_number == 3)
    {
      background.pushImage(0, 0, M8AXQuote1Width, M8AXQuote1Height, M8AXQuote1);
      background.pushSprite(0, 0);
    }
    else if (random_number == 4)
    {
      background.pushImage(0, 0, M8AXQuote2Width, M8AXQuote2Height, M8AXQuote2);
      background.pushSprite(0, 0);
    }

    HTTPClient http;
    String quote = getQuote();
    displayQuote(quote);
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(112, 160);
    tft.setTextSize(1);
    tft.print("M8AX-Frase Num-" + String(numfrases));
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(1, 6);
    tft.setTextSize(1);
    correccion = 1;
    int millonario = atoi(data.valids.c_str());
    if (millonario == 0)
    {
      tft.print(data.currentTime + " - " + dataa.currentDate + " - " + data.currentHashRate + " KH/s - " + data.temp + " Grados." + " NO RICO");
    }
    else
    {
      tft.print(data.currentTime + " - " + dataa.currentDate + " - " + data.currentHashRate + " KH/s - " + data.temp + " Grados." + " SI RICO");
    }
  }
  actualizarcalen++;
  actualizarc = 0;
  actuanot = 0;
}

/*
 * Función: tDisplay_m8axScreen9
 * ------------------------------------
 *  Esta función actualiza la pantalla con la hora en formato romano, la fecha, el estado de la minería,
 *  la temperatura del procesador, y un indicador de riqueza ("ERES RICO" o "NO RICO").
 *
 *  Parámetros:
 *    - mElapsed: Tiempo transcurrido desde la última actualización.
 *
 *  Flujo de ejecución:
 *  1. Obtiene los datos actualizados de:
 *      - Minería (`data`): información sobre shares completados, Khashes, hashrate y temperatura.
 *      - Reloj (`dataa`): hora y fecha actual.
 *      - Moneda (`monedilla`).
 *  2. Convierte la hora, minutos, segundos, día, mes, año, y temperatura en formato romano utilizando `convertirARomanos()`.
 *  3. Si `segundos % 59 == 0`, reinicia el contador de actualización de pantalla (`actualizarc`).
 *  4. Si `actualizarc` es 0, selecciona aleatoriamente una imagen de fondo (entre 4 opciones).
 *  5. Muestra la imagen seleccionada como fondo.
 *  6. Imprime en la pantalla la hora, los valores romanos de día, mes, año y temperatura, junto con el estado de la minería.
 *  7. Si el valor de `data.valids` es 1, imprime "ERES RICO", de lo contrario, imprime "NO RICO".
 *  8. Muestra los valores de "V.BLOCKS" (número de bloques de minería) y la temperatura del procesador en formato romano.
 *  9. El texto se muestra con colores aleatorios seleccionados de un arreglo `colors[]`.
 *  10. Incrementa `actualizarc` y realiza actualizaciones periódicas de la pantalla.
 */

void tDisplay_m8axScreen9(unsigned long mElapsed)
{
  mining_data data = getMiningData(mElapsed);
  clock_data dataa = getClockData(mElapsed);
  monedilla = getCoinData(mElapsed);
  relojete = getClockData(mElapsed);
  mineria = getMiningData(mElapsed);
  int millonario = atoi(data.valids.c_str());
  incrementCounter();
  unsigned long segundo = timeClient.getSeconds();
  int horas = dataa.currentTime.substring(0, 2).toInt();
  int minutos = dataa.currentTime.substring(3, 5).toInt();
  int segundos = segundo % 60;
  if (segundos % 59 == 0)
  {
    actualizarc = 0;
  }
  std::string quediase = obtenerDiaSemana(std::string(dataa.currentDate.c_str()));
  int dia = dataa.currentDate.substring(0, 2).toInt();
  int mes = dataa.currentDate.substring(3, 5).toInt();
  int anio = dataa.currentDate.substring(6, 10).toInt();
  String hRoman = convertirARomanos(horas);
  String mRoman = convertirARomanos(minutos);
  String sRoman = convertirARomanos(segundos);
  String RDia = convertirARomanos(dia);
  String RMes = convertirARomanos(mes);
  String RAnio = convertirARomanos(anio);
  String TempCPU = convertirARomanos(std::stoi(data.temp.c_str()));
  if (actualizarc == 0)
  {
    random_number = 1 + (esp_random() % 4);
  }
  if (random_number == 1)
  {
    background.pushImage(0, 0, ImagenM8AXWidth, ImagenM8AXHeight, ImagenM8AX);
    background.pushSprite(0, 0);
  }
  else if (random_number == 2)
  {
    background.pushImage(0, 0, M8AXRelojLunarWidth, M8AXRelojLunarHeight, M8AXRelojLunar);
    background.pushSprite(0, 0);
  }
  else if (random_number == 3)
  {
    background.pushImage(0, 0, M8AXQuote1Width, M8AXQuote1Height, M8AXQuote1);
    background.pushSprite(0, 0);
  }
  else if (random_number == 4)
  {
    background.pushImage(0, 0, M8AXQuote2Width, M8AXQuote2Height, M8AXQuote2);
    background.pushSprite(0, 0);
  }
  actualizarc++;
  Serial.printf("M8AX - >>> Completados %s Share(s), %s Khashes, Prom. Hashrate %s KH/s %s°\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str(), data.temp.c_str());
  tft.setTextSize(6);
  tft.setCursor(3, 2);
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  tft.setTextColor(colors[colorI]);
  tft.print(hRoman);
  tft.setCursor(3, 62);
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  tft.setTextColor(colors[colorI]);
  tft.print(mRoman);
  tft.setCursor(3, 122);
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  tft.setTextColor(colors[colorI]);
  tft.print(sRoman);
  tft.setTextSize(1);
  tft.setCursor(250, 2);
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  tft.setTextColor(colors[colorI]);
  tft.print("d " + RDia);
  tft.setCursor(250, 16);
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  tft.setTextColor(colors[colorI]);
  tft.print("m " + RMes);
  tft.setCursor(250, 30);
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  tft.setTextColor(colors[colorI]);
  tft.print("a " + RAnio);
  tft.setCursor(250, 44);
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  tft.setTextColor(colors[colorI]);
  tft.print(String(quediase.c_str()));
  tft.setCursor(250, 58);
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  tft.setTextColor(colors[colorI]);
  tft.print(String(data.currentHashRate.c_str()) + " KH/s");
  if (millonario == 1)
  {
    tft.setCursor(250, 72);
    colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
    tft.setTextColor(colors[colorI]);
    tft.print("ERES RICO");
  }
  else
  {
    tft.setCursor(250, 72);
    colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
    tft.setTextColor(colors[colorI]);
    tft.print("NO RICO");
  }
  tft.setCursor(250, 93);
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  tft.setTextColor(colors[colorI]);
  tft.print("V.BLOCKS");
  tft.setCursor(260, 109);
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  tft.setTextColor(colors[colorI]);
  tft.setTextSize(5);
  tft.print(millonario);
  tft.setTextSize(1);
  tft.setCursor(250, 153);
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  tft.setTextColor(colors[colorI]);
  tft.print("T " + TempCPU + "g");
}

/*
 * Función: tDisplay_m8axScreen10
 * ------------------------------------
 *  Esta función actualiza la pantalla con la hora en formato binario, la fecha en formato binario,
 *  el estado de la minería, la temperatura del procesador y un indicador de riqueza ("ERES RICO" o "NO RICO").
 *
 *  Parámetros:
 *    - mElapsed: Tiempo transcurrido desde la última actualización.
 *
 *  Flujo de ejecución:
 *  1. Obtiene los datos actualizados de:
 *      - Minería (`data`): información sobre shares completados, Khashes, hashrate y temperatura.
 *      - Reloj (`dataa`): hora y fecha actual.
 *      - Moneda (`monedilla`).
 *  2. Convierte la hora, minutos, segundos, día, mes, año y temperatura en formato binario utilizando `ABinario()`.
 *  3. Si `segundos % 59 == 0`, reinicia el contador de actualización de pantalla (`actualizarc`).
 *  4. Si `actualizarc` es 0, selecciona aleatoriamente una imagen de fondo (entre 4 opciones).
 *  5. Muestra la imagen seleccionada como fondo.
 *  6. Imprime en la pantalla la hora, los valores binarios de día, mes, año y temperatura, junto con el estado de la minería.
 *  7. Si el valor de `data.valids` es 1, imprime "ERES RICO", de lo contrario, imprime "NO RICO".
 *  8. Muestra los valores de "V.BLOCKS" (número de bloques de minería) y la temperatura del procesador en formato binario.
 *  9. El texto se muestra con colores aleatorios seleccionados de un arreglo `colors[]`.
 *  10. Incrementa `actualizarc` y realiza actualizaciones periódicas de la pantalla.
 */

void tDisplay_m8axScreen10(unsigned long mElapsed)
{
  mining_data data = getMiningData(mElapsed);
  clock_data dataa = getClockData(mElapsed);
  monedilla = getCoinData(mElapsed);
  relojete = getClockData(mElapsed);
  mineria = getMiningData(mElapsed);
  int millonario = atoi(data.valids.c_str());
  incrementCounter();
  unsigned long segundo = timeClient.getSeconds();
  int horas = dataa.currentTime.substring(0, 2).toInt();
  int minutos = dataa.currentTime.substring(3, 5).toInt();
  int segundos = segundo % 60;
  if (segundos % 59 == 0)
  {
    actualizarc = 0;
  }
  std::string quediase = obtenerDiaSemana(std::string(dataa.currentDate.c_str()));
  int dia = dataa.currentDate.substring(0, 2).toInt();
  int mes = dataa.currentDate.substring(3, 5).toInt();
  int anio = dataa.currentDate.substring(6, 10).toInt();
  String hRoman = ABinario(horas);
  String mRoman = ABinario(minutos);
  String sRoman = ABinario(segundos);
  String RDia = ABinario(dia);
  String RMes = ABinario(mes);
  String RAnio = ABinario(anio);
  String TempCPU = ABinario(std::stoi(data.temp.c_str()));
  if (actualizarc == 0)
  {
    random_number = 1 + (esp_random() % 4);
  }
  if (random_number == 1)
  {
    background.pushImage(0, 0, ImagenM8AXWidth, ImagenM8AXHeight, ImagenM8AX);
    background.pushSprite(0, 0);
  }
  else if (random_number == 2)
  {
    background.pushImage(0, 0, M8AXRelojLunarWidth, M8AXRelojLunarHeight, M8AXRelojLunar);
    background.pushSprite(0, 0);
  }
  else if (random_number == 3)
  {
    background.pushImage(0, 0, M8AXQuote1Width, M8AXQuote1Height, M8AXQuote1);
    background.pushSprite(0, 0);
  }
  else if (random_number == 4)
  {
    background.pushImage(0, 0, M8AXQuote2Width, M8AXQuote2Height, M8AXQuote2);
    background.pushSprite(0, 0);
  }
  actualizarc++;
  Serial.printf("M8AX - >>> Completados %s Share(s), %s Khashes, Prom. Hashrate %s KH/s %s°\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str(), data.temp.c_str());
  tft.setTextSize(6);
  tft.setCursor(3, 2);
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  tft.setTextColor(colors[colorI]);
  tft.print(hRoman);
  tft.setCursor(3, 62);
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  tft.setTextColor(colors[colorI]);
  tft.print(mRoman);
  tft.setCursor(3, 122);
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  tft.setTextColor(colors[colorI]);
  tft.print(sRoman);
  tft.setTextSize(1);
  tft.setCursor(240, 2);
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  tft.setTextColor(colors[colorI]);
  tft.print("d " + RDia);
  tft.setCursor(240, 16);
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  tft.setTextColor(colors[colorI]);
  tft.print("m " + RMes);
  tft.setCursor(240, 30);
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  tft.setTextColor(colors[colorI]);
  tft.print("a " + RAnio);
  tft.setCursor(240, 44);
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  tft.setTextColor(colors[colorI]);
  tft.print(String(quediase.c_str()));
  tft.setCursor(240, 58);
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  tft.setTextColor(colors[colorI]);
  tft.print(String(data.currentHashRate.c_str()) + " KH/s");
  if (millonario == 1)
  {
    tft.setCursor(240, 72);
    colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
    tft.setTextColor(colors[colorI]);
    tft.print("ERES RICO");
  }
  else
  {
    tft.setCursor(240, 72);
    colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
    tft.setTextColor(colors[colorI]);
    tft.print("NO RICO");
  }
  tft.setCursor(240, 93);
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  tft.setTextColor(colors[colorI]);
  tft.print("V.BLOCKS");
  tft.setCursor(250, 109);
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  tft.setTextColor(colors[colorI]);
  tft.setTextSize(5);
  tft.print(millonario);
  tft.setTextSize(1);
  tft.setCursor(240, 153);
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  tft.setTextColor(colors[colorI]);
  tft.print("T " + TempCPU + "g");
}

/*
 * Función: tDisplay_m8axScreen11
 * ------------------------------------
 *  Esta función actualiza la pantalla con la hora, fecha, el estado de la minería, la temperatura del procesador
 *  y un indicador de riqueza ("ERES RICO" o "NO RICO"), pero con una variación en el manejo de la fecha y el año
 *  en comparación con la función anterior.
 *
 *  Parámetros:
 *    - mElapsed: Tiempo transcurrido desde la última actualización.
 *
 *  Flujo de ejecución:
 *  1. Obtiene los datos actualizados de:
 *      - Minería (`data`): información sobre shares completados, Khashes, hashrate y temperatura.
 *      - Reloj (`dataa`): hora y fecha actual.
 *      - Moneda (`monedilla`).
 *  2. Extrae la hora, minutos y segundos actuales, además de la fecha completa (día, mes, año).
 *  3. Divide el año en dos partes (los primeros dos dígitos y los últimos dos dígitos).
 *  4. Convierte la hora, minutos, segundos, día, mes y año en formato Morse utilizando la función `Amorse()`.
 *  5. Si `segundos % 59 == 0`, reinicia el contador de actualización de pantalla (`actualizarc`).
 *  6. Si `actualizarc` es 0, selecciona aleatoriamente una imagen de fondo (entre 4 opciones).
 *  7. Muestra la imagen seleccionada como fondo.
 *  8. Imprime en la pantalla la hora en formato Morse, junto con los valores binarios de día, mes, año y temperatura.
 *  9. Si el valor de `data.valids` es 1, imprime "ERES RICO", de lo contrario, imprime "NO RICO".
 *  10. Muestra los valores de "V.BLOCKS" (número de bloques de minería) y la temperatura del procesador en formato Morse.
 *  11. El texto se muestra con colores aleatorios seleccionados de un arreglo `colors[]`.
 *  12. Incrementa `actualizarc` y realiza actualizaciones periódicas de la pantalla.
 */

void tDisplay_m8axScreen11(unsigned long mElapsed)
{
  mining_data data = getMiningData(mElapsed);
  clock_data dataa = getClockData(mElapsed);
  monedilla = getCoinData(mElapsed);
  relojete = getClockData(mElapsed);
  mineria = getMiningData(mElapsed);
  unsigned long segundo = timeClient.getSeconds();
  int millonario = atoi(data.valids.c_str());
  int horas = dataa.currentTime.substring(0, 2).toInt();
  int minutos = dataa.currentTime.substring(3, 5).toInt();
  int segundos = segundo % 60;
  incrementCounter();

  if (segundos % 59 == 0)
  {
    actualizarc = 0;
  }

  std::string quediase = obtenerDiaSemana(std::string(dataa.currentDate.c_str()));
  int dia = dataa.currentDate.substring(0, 2).toInt();
  int mes = dataa.currentDate.substring(3, 5).toInt();
  String anio = dataa.currentDate.substring(6, 10);
  int anio2d = anio.substring(0, 2).toInt();  // Extraer los primeros dos dígitos
  int aniol2d = anio.substring(2, 4).toInt(); // Extraer los últimos dos dígitos
  String TempCPU = Amorse(std::stoi(data.temp.c_str()));
  String hRoman = Amorse(horas);
  String mRoman = Amorse(minutos);
  String sRoman = Amorse(segundos);
  String RDia = Amorse(dia);
  String RMes = Amorse(mes);
  String RAnio = Amorse(anio2d);
  String RAnio2 = Amorse(aniol2d);
  if (actualizarc == 0)
  {
    random_number = 1 + (esp_random() % 4);
  }
  if (random_number == 1)
  {
    background.pushImage(0, 0, ImagenM8AXWidth, ImagenM8AXHeight, ImagenM8AX);
    background.pushSprite(0, 0);
  }
  else if (random_number == 2)
  {
    background.pushImage(0, 0, M8AXRelojLunarWidth, M8AXRelojLunarHeight, M8AXRelojLunar);
    background.pushSprite(0, 0);
  }
  else if (random_number == 3)
  {
    background.pushImage(0, 0, M8AXQuote1Width, M8AXQuote1Height, M8AXQuote1);
    background.pushSprite(0, 0);
  }
  else if (random_number == 4)
  {
    background.pushImage(0, 0, M8AXQuote2Width, M8AXQuote2Height, M8AXQuote2);
    background.pushSprite(0, 0);
  }
  actualizarc++;
  Serial.printf("M8AX - >>> Completados %s Share(s), %s Khashes, Prom. Hashrate %s KH/s %s°\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str(), data.temp.c_str());
  tft.setTextSize(3);
  tft.setCursor(3, 0);
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  tft.setTextColor(colors[colorI]);
  tft.print(hRoman);
  tft.setCursor(3, 75);
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  tft.setTextColor(colors[colorI]);
  tft.print(mRoman);
  tft.setCursor(3, 150);
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  tft.setTextColor(colors[colorI]);
  tft.print(sRoman);
  tft.setTextSize(1);
  tft.setCursor(240, 2);
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  tft.setTextColor(colors[colorI]);
  tft.print("d " + RDia);
  tft.setCursor(240, 16);
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  tft.setTextColor(colors[colorI]);
  tft.print("m " + RMes);
  tft.setCursor(240, 30);
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  tft.setTextColor(colors[colorI]);
  tft.print("a " + RAnio);
  tft.setCursor(252, 38);
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  tft.setTextColor(colors[colorI]);
  tft.print(RAnio2);
  tft.setCursor(240, 52);
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  tft.setTextColor(colors[colorI]);
  tft.print(String(quediase.c_str()));
  tft.setCursor(240, 66);
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  tft.setTextColor(colors[colorI]);
  tft.print(String(data.currentHashRate.c_str()) + " KH/s");
  if (millonario == 1)
  {
    tft.setCursor(240, 80);
    colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
    tft.setTextColor(colors[colorI]);
    tft.print("ERES RICO");
  }
  else
  {
    tft.setCursor(240, 80);
    colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
    tft.setTextColor(colors[colorI]);
    tft.print("NO RICO");
  }
  tft.setCursor(240, 95);
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  tft.setTextColor(colors[colorI]);
  tft.print("V.BLOCKS");
  tft.setCursor(250, 113);
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  tft.setTextColor(colors[colorI]);
  tft.setTextSize(5);
  tft.print(millonario);
  tft.setTextSize(1);
  tft.setCursor(225, 155);
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  tft.setTextColor(colors[colorI]);
  tft.print("T " + TempCPU + "g");
}

/*
 * Función: tDisplay_m8axScreen12
 * ------------------------------------
 *  Esta función actualiza la pantalla con información sobre la minería, un mensaje personalizado y gráficos aleatorios,
 *  además de cambiar el fondo entre varias opciones. También se genera un patrón aleatorio de píxeles en la pantalla.
 *
 *  Parámetros:
 *    - mElapsed: Tiempo transcurrido desde la última actualización.
 *
 *  Flujo de ejecución:
 *  1. Obtiene los datos actualizados de:
 *      - Minería (`data`): información sobre shares completados, Khashes, hashrate y temperatura.
 *      - Moneda (`monedilla`).
 *      - Reloj (`relojete`).
 *  2. Incrementa el contador de actualizaciones (`incrementCounter()`).
 *  3. Imprime en el monitor serial información sobre el progreso de la minería.
 *  4. Selecciona aleatoriamente una imagen de fondo (entre 4 opciones) usando un número aleatorio generado por `esp_random()`.
 *  5. Si el número es par, se establece un valor aleatorio para `limite` entre 0 y 15000, de lo contrario, entre 0 y 5000.
 *  6. Muestra el hashrate actual en la pantalla con un tamaño de texto grande, utilizando un color aleatorio.
 *  7. Muestra un mensaje en el centro de la pantalla ("... GRACIAS ...").
 *  8. Muestra una URL debajo del mensaje principal (https://youtube.com/m8ax).
 *  9. Dibuja un patrón de píxeles aleatorios en la pantalla. Cada píxel tiene un color aleatorio de un conjunto `colors[]`,
 *      y las posiciones se generan aleatoriamente dentro de los límites de la pantalla (ancho 0 a 340, alto 0 a 170).
 *  10. Reinicia varias variables (`actualizarcalen`, `actualizarc`, `actual`, `actuanot`, `correccion`) para preparar la pantalla
 *      para la siguiente actualización.
 */

void tDisplay_m8axScreen12(unsigned long mElapsed)
{
  mining_data data = getMiningData(mElapsed);
  clock_data dataa = getClockData(mElapsed);
  monedilla = getCoinData(mElapsed);
  relojete = getClockData(mElapsed);
  mineria = getMiningData(mElapsed);
  incrementCounter();
  Serial.printf("M8AX - >>> Completados %s Share(s), %s Khashes, Prom. Hashrate %s KH/s %s°\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str(), data.temp.c_str());
  int random_number = 1 + (esp_random() % 4);
  switch (random_number)
  {
  case 1:
    background.pushImage(0, 0, ImagenM8AXWidth, ImagenM8AXHeight, ImagenM8AX);
    background.pushSprite(0, 0);
    break;
  case 2:
    background.pushImage(0, 0, M8AXRelojLunarWidth, M8AXRelojLunarHeight, M8AXRelojLunar);
    background.pushSprite(0, 0);
    break;
  case 3:
    background.pushImage(0, 0, M8AXQuote1Width, M8AXQuote1Height, M8AXQuote1);
    background.pushSprite(0, 0);
    break;
  case 4:
    background.pushImage(0, 0, M8AXQuote2Width, M8AXQuote2Height, M8AXQuote2);
    background.pushSprite(0, 0);
    break;
  default:
    // En caso de que el número esté fuera de rango, puedes poner una imagen predeterminada o hacer algo más.
    break;
  }
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  if (colorI % 2 == 0)
  {
    limite = esp_random() % 15000; // Rango: 0 a 54400
  }
  else
  {
    limite = esp_random() % 5000; // Rango: 0 a 54400
  }
  tft.setCursor(30, 60);
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  tft.setTextColor(colors[colorI]);
  tft.setTextSize(7);
  tft.print(String(data.currentHashRate.c_str()));
  tft.setCursor(25, 15);
  tft.setTextSize(3);
  tft.print("... GRACIAS ...");
  tft.setCursor(16, 132);
  tft.setTextSize(2);
  tft.print("https://youtube.com/m8ax");
  for (int i = 0; i < limite; i++)
  {
    colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
    int ancho = esp_random() % 341; // Rango: 0 a 340
    int alto = esp_random() % 171;  // Rango: 0 a 170
    tft.drawPixel(ancho, alto, colors[colorI]);
  }
  tft.setTextSize(1);
  actualizarcalen = 0;
  actualizarc = 0;
  actual = 0;
  actuanot = 0;
  correccion = 0;
}

/*
 * Función: tDisplay_m8axScreen13
 * -----------------------------------
 *  Esta función es responsable de actualizar la pantalla con información sobre la minería,
 *  el precio de las criptomonedas y un mensaje personalizado. Además, selecciona aleatoriamente una imagen de fondo y
 *  muestra una lista de criptomonedas con sus precios actualizados.
 *
 *  Parámetros:
 *    - mElapsed: Tiempo transcurrido desde la última actualización.
 *
 *  Flujo de ejecución:
 *  1. Obtiene los datos actualizados de:
 *      - Minería (`data`): Información sobre shares completados, Khashes, hashrate y temperatura.
 *      - Moneda (`monedilla`).
 *      - Reloj (`relojete`).
 *  2. Realiza una comprobación para saber si es el momento de actualizar la pantalla basándose en el minuto y segundo actuales.
 *  3. Si es el momento de actualizar, selecciona aleatoriamente una de las 4 imágenes posibles para usar como fondo de pantalla.
 *  4. Imprime en el monitor serial información sobre el progreso de la minería.
 *  5. Para cada una de las 20 criptomonedas (a través del índice `iii`), se obtiene el precio y se muestra en la pantalla:
 *      - Se distribuyen las criptomonedas en 4 columnas en función del índice `iii`.
 *      - Si el precio de la criptomoneda es válido, se muestra el nombre y el precio en dólares.
 *      - Si el precio no es válido (error al obtenerlo), se muestra un mensaje de error para esa criptomoneda.
 *  6. Si la actualización de la pantalla se realiza, se reinician varias variables relacionadas con el ciclo de actualización.
 *  7. Se dibuja una línea en la parte superior de la pantalla para separar la información.
 */

void tDisplay_m8axScreen13(unsigned long mElapsed)
{
  mining_data data = getMiningData(mElapsed);
  clock_data dataa = getClockData(mElapsed);
  monedilla = getCoinData(mElapsed);
  relojete = getClockData(mElapsed);
  mineria = getMiningData(mElapsed);
  int x, y;
  incrementCounter();
  unsigned long segundo = timeClient.getSeconds();
  int horas = dataa.currentTime.substring(0, 2).toInt();
  int minutos = dataa.currentTime.substring(3, 5).toInt();
  int segundos = segundo % 60;
  abortar = 0;
  if (minutos % 2 == 0 && segundos <= 1)
  {
    actualizarc = 0;
  }
  if (actualizarc == 0)
  {
    random_number = 1 + (esp_random() % 4);
    if (random_number == 1)
    {
      background.pushImage(0, 0, ImagenM8AXWidth, ImagenM8AXHeight, ImagenM8AX);
      background.pushSprite(0, 0);
    }
    else if (random_number == 2)
    {
      background.pushImage(0, 0, M8AXRelojLunarWidth, M8AXRelojLunarHeight, M8AXRelojLunar);
      background.pushSprite(0, 0);
    }
    else if (random_number == 3)
    {
      background.pushImage(0, 0, M8AXQuote1Width, M8AXQuote1Height, M8AXQuote1);
      background.pushSprite(0, 0);
    }
    else if (random_number == 4)
    {
      background.pushImage(0, 0, M8AXQuote2Width, M8AXQuote2Height, M8AXQuote2);
      background.pushSprite(0, 0);
    }
  }
  Serial.printf("M8AX - >>> Completados %s Share(s), %s Khashes, Prom. Hashrate %s KH/s %s°\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str(), data.temp.c_str());
  if (actualizarc == 0)
  {
    for (int iii = 0; iii < 20; iii++)
    {
      if (abortar == 1)
        iii = 19;
      float precio = obtenerPrecio(criptomonedas[iii]);
      if (precio != -1)
      {
        if (iii < 5)
        {
          // Primera columna
          x = 5;
          y = 20 + iii * 30;
        }
        else if (iii < 10)
        {
          // Segunda columna
          x = tft.width() / 4 + 10;
          y = 20 + (iii - 5) * 30;
        }
        else if (iii < 15)
        {
          // Tercera columna
          x = 2 * (tft.width() / 4) + 15;
          y = 20 + (iii - 10) * 30;
        }
        else
        {
          // Cuarta columna
          x = 3 * (tft.width() / 4) + 20;
          y = 20 + (iii - 15) * 30;
        }
        colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
        tft.setTextColor(colors[colorI]);
        tft.setTextSize(1);
        tft.drawLine(0, 13, 320, 13, colors[colorI]); // Dibujar línea de (0, y) a (320, y)
        String Textito = "M8AX - CryptoChrono 2Min - " + String(horas) + ":" + String(minutos) + ":" + String(minutos) + " - " + data.currentHashRate + " KH/s";
        tft.setCursor(16, 2);
        tft.print(Textito);
        tft.setCursor(x, y);
        colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
        tft.setTextColor(colors[colorI]);
        tft.print(criptomonedas[iii]);
        tft.setCursor(x, y + 15);
        tft.print("$");
        tft.print(precio);
      }
      else
      {
        if (iii < 5)
        {
          // Primera columna
          x = 5;
          y = 20 + iii * 30;
        }
        else if (iii < 10)
        {
          // Segunda columna
          x = tft.width() / 4 + 10;
          y = 20 + (iii - 5) * 30;
        }
        else if (iii < 15)
        {
          // Tercera columna
          x = 2 * (tft.width() / 4) + 15;
          y = 20 + (iii - 10) * 30;
        }
        else
        {
          // Cuarta columna
          x = 3 * (tft.width() / 4) + 20;
          y = 20 + (iii - 15) * 30;
        }
        colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
        tft.setTextColor(colors[colorI]);
        tft.setTextSize(1);
        tft.drawLine(0, 13, 320, 13, colors[colorI]); // Dibujar línea de (0, y) a (320, y)
        String Textito = "M8AX - CryptoChrono 2Min - " + String(horas) + ":" + String(minutos) + ":" + String(minutos) + " - " + data.currentHashRate + " KH/s";
        tft.setCursor(16, 2);
        tft.print(Textito);
        tft.setCursor(x, y);
        colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
        tft.setTextColor(colors[colorI]);
        tft.print(criptomonedas[iii]);
        tft.setCursor(x, y + 15);
        tft.print("$-ERROR-$");
        Serial.print("M8AX - Error Al Obtener El Precio De ");
        Serial.println(criptomonedas[iii]);
      }
    }
    actualizarc++;
    actualizarcalen = 0;
    actual = 0;
    actuanot = 0;
    correccion = 0;
  }
}

/*
 * Función: tDisplay_m8axScreen14
 * ------------------------------
 *  Esta función se encarga de actualizar la pantalla con información sobre el estado de la minería,
 *  noticias aleatorias y un mensaje sobre el "estado de riqueza" basado en la cantidad de shares validados.
 *  La información se presenta de una manera gráfica con un fondo y se actualiza en intervalos de tiempo específicos.
 *
 *  Parámetros:
 *    - mElapsed: Tiempo transcurrido desde la última actualización.
 *
 *  Flujo de ejecución:
 *  1. Obtiene los datos actualizados sobre:
 *      - Minería (`data`): Información sobre shares completados, Khashes, hashrate y temperatura.
 *      - Reloj (`relojete`).
 *  2. Si es el momento adecuado (basado en el valor de `actuanot` y `correccion`), selecciona aleatoriamente uno de los 4 fondos disponibles:
 *      - `ImagenM8AX`, `M8AXRelojLunar`, `M8AXQuote1`, o `M8AXQuote2`.
 *  3. Obtiene las noticias a través de la función `obtenerNoticias()`.
 *  4. Actualiza la pantalla con el texto "M8AX-Noticias Num-" seguido del número de noticias actuales (`numnotis`).
 *  5. Dibuja una línea horizontal en la parte superior de la pantalla.
 *  6. Imprime información adicional sobre el tiempo actual, la fecha, el hashrate y la temperatura en la pantalla.
 *      - Si el número de shares válidos (`data.valids`) es igual a 0, se imprime "NO RICO".
 *      - Si el número de shares válidos es mayor a 0, se imprime "SI RICO".
 *  7. Incrementa el contador `actuanot` y reinicia las variables relacionadas con el ciclo de actualización (`actualizarcalen`, `actualizarc`, `actual`).
 */

void tDisplay_m8axScreen14(unsigned long mElapsed)
{
  mining_data data = getMiningData(mElapsed);
  clock_data dataa = getClockData(mElapsed);
  monedilla = getCoinData(mElapsed);
  relojete = getClockData(mElapsed);
  mineria = getMiningData(mElapsed);
  incrementCounter();
  Serial.printf("M8AX - >>> Completados %s Share(s), %s Khashes, Prom. Hashrate %s KH/s %s°\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str(), data.temp.c_str());
  if (actuanot % 59 == 0 || correccion == 0)
  {
    numnotis++;
    int random_number = 1 + (esp_random() % 4);
    if (random_number == 1)
    {
      background.pushImage(0, 0, ImagenM8AXWidth, ImagenM8AXHeight, ImagenM8AX);
      background.pushSprite(0, 0);
    }
    else if (random_number == 2)
    {
      background.pushImage(0, 0, M8AXRelojLunarWidth, M8AXRelojLunarHeight, M8AXRelojLunar);
      background.pushSprite(0, 0);
    }
    else if (random_number == 3)
    {
      background.pushImage(0, 0, M8AXQuote1Width, M8AXQuote1Height, M8AXQuote1);
      background.pushSprite(0, 0);
    }
    else if (random_number == 4)
    {
      background.pushImage(0, 0, M8AXQuote2Width, M8AXQuote2Height, M8AXQuote2);
      background.pushSprite(0, 0);
    }

    obtenerNoticias();
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(112, 160);
    tft.setTextSize(1);
    tft.print("M8AX-Noticias Num-" + String(numnotis));
    tft.setTextColor(TFT_WHITE);
    tft.drawLine(0, 20, 320, 20, colors[colorI]); // Dibujar línea de (0, y) a (320, y)
    tft.setCursor(1, 6);
    tft.setTextSize(1);
    correccion = 1;
    int millonario = atoi(data.valids.c_str());
    if (millonario == 0)
    {
      tft.print(data.currentTime + " - " + dataa.currentDate + " - " + data.currentHashRate + " KH/s - " + data.temp + " Grados." + " NO RICO");
    }
    else
    {
      tft.print(data.currentTime + " - " + dataa.currentDate + " - " + data.currentHashRate + " KH/s - " + data.temp + " Grados." + " SI RICO");
    }
  }
  actuanot++;
  actualizarcalen = 0;
  actualizarc = 0;
  actual = 0;
}

/*
 * Función: tDisplay_m8axScreen15
 * ------------------------------
 *  Esta función es responsable de actualizar la pantalla con información sobre el estado de la minería,
 *  incluyendo el hash rate, temperaturas, y datos adicionales como la factorización de un número aleatorio.
 *  También visualiza barras de progreso que representan los segundos, horas y minutos actuales.
 *
 *  Parámetros:
 *    - mElapsed: Tiempo transcurrido desde la última actualización.
 *
 *  Flujo de ejecución:
 *  1. Obtiene los datos actualizados sobre:
 *      - Minería (`data`): Información sobre shares completados, Khashes, hashrate y temperatura.
 *      - Reloj (`dataa`): Información sobre la hora y fecha.
 *  2. Calcula el tiempo actual (segundos, minutos, horas) y actualiza las barras de progreso visualizando los valores en tiempo real.
 *  3. Si `actualizarcalen` es 0, establece una imagen de fondo inicial en la pantalla y la actualiza.
 *  4. Cada 30 segundos (segundo % 30 == 0), selecciona aleatoriamente una de las 4 imágenes de fondo y la muestra en la pantalla.
 *  5. Convierte el valor de `currentHashRate` a un número flotante y lo usa para dibujar un gráfico analógico del hashrate.
 *  6. Muestra un número aleatorio generado (`rndnumero2`) y su factorización.
 *  7. Muestra el estado de la minería, especificando si está "SI RICO" o "NO RICO" basado en el número de shares válidos.
 *  8. Muestra datos adicionales como:
 *      - Hashes por segundo.
 *      - Operaciones por segundo.
 *      - TFLOPS, GOPS, MOPS, TOPS.
 *  9. Muestra la temperatura de la minería en la pantalla.
 * 10. Dibuja tres barras de progreso:
 *      - Una para los segundos (basada en `segundos`).
 *      - Otra para las horas (basada en `horas`).
 *      - Otra para los minutos (basada en `minutos`).
 *  11. Resetea varios contadores (`actualizarcalen`, `actualizarc`, `actuanot`) al final de la función.
 */

void tDisplay_m8axScreen15(unsigned long mElapsed)
{
  mining_data data = getMiningData(mElapsed);
  clock_data dataa = getClockData(mElapsed);
  monedilla = getCoinData(mElapsed);
  relojete = getClockData(mElapsed);
  mineria = getMiningData(mElapsed);
  unsigned long segundo = timeClient.getSeconds();
  int segundos = segundo % 60;
  int horas = dataa.currentTime.substring(0, 2).toInt();
  int minutos = dataa.currentTime.substring(3, 5).toInt();
  incrementCounter();
  if (actualizarcalen == 0)
  {
    background.pushImage(0, 0, ImagenM8AXWidth, ImagenM8AXHeight, ImagenM8AX);
    background.pushSprite(0, 0);
    actualizarcalen++;
  }
  background.pushSprite(0, 0);
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  Serial.printf("M8AX - >>> Completados %s Share(s), %s Khashes, Prom. Hashrate %s KH/s %s°\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str(), data.temp.c_str());
  if (segundo % 30 == 0)
  {
    int random_number = 1 + (esp_random() % 4);
    if (random_number == 1)
    {
      background.pushImage(0, 0, ImagenM8AXWidth, ImagenM8AXHeight, ImagenM8AX);
      background.pushSprite(0, 0);
    }
    else if (random_number == 2)
    {
      background.pushImage(0, 0, M8AXRelojLunarWidth, M8AXRelojLunarHeight, M8AXRelojLunar);
      background.pushSprite(0, 0);
    }
    else if (random_number == 3)
    {
      background.pushImage(0, 0, M8AXQuote1Width, M8AXQuote1Height, M8AXQuote1);
      background.pushSprite(0, 0);
    }
    else if (random_number == 4)
    {
      background.pushImage(0, 0, M8AXQuote2Width, M8AXQuote2Height, M8AXQuote2);
      background.pushSprite(0, 0);
    }
  }
  const char *hashRateStr = dataa.currentHashRate.c_str(); // Obtener la cadena
  float hashRa = atof(hashRateStr);                        // Convertirla a flotante (float)
  dibujaAnalogKH(hashRa);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(1);
  rndnumero2 = esp_random();
  const char *factorizacion = factorize(rndnumero2);
  tft.setCursor(4, 154);
  tft.print(String(rndnumero2) + " - " + (factorizacion));
  Serial.println("M8AX - Factorizando Número - " + String(rndnumero2) + " - " + (factorizacion));
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  tft.setCursor(4, 144);
  tft.setTextColor(colors[colorI]);
  tft.print("Factorizacion Nerd");
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(2, 6);
  tft.setTextSize(1);
  int millonario = atoi(data.valids.c_str());
  if (millonario == 0)
  {
    tft.print(data.currentTime + " - " + dataa.currentDate + " - " + data.currentHashRate + " KH/s - " + data.temp + " Grados." + " NO RICO");
  }
  else
  {
    tft.print(data.currentTime + " - " + dataa.currentDate + " - " + data.currentHashRate + " KH/s - " + data.temp + " Grados." + " SI RICO");
  }
  tft.setCursor(4, 23);
  tft.setTextSize(1);
  tft.setTextColor(colors[colorI]);
  tft.print("... DATOS NERD I ...");
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(4, 40);
  tft.setTextColor(colors[colorI]);
  tft.print("Hashes/Seg");
  tft.setTextColor(TFT_WHITE);
  float hasheseg = hashRa * 1000;
  tft.setCursor(4, 50);
  tft.print((uint32_t)hasheseg);
  tft.setCursor(4, 65);
  tft.setTextColor(colors[colorI]);
  tft.print("+-/Ops_Simp/Seg");
  tft.setTextColor(TFT_WHITE);
  float opsseg = (hashRa * 1000) * 2560;
  tft.setCursor(4, 75);
  tft.print((uint32_t)opsseg);
  tft.setCursor(4, 90);
  tft.setTextColor(colors[colorI]);
  tft.print("TFLOPS");
  tft.setTextColor(TFT_WHITE);
  float tflops = ((hashRa * 1000) * 2560) / 1000000000000;
  tft.setCursor(4, 100);
  char buffer[10];               // Buffer para almacenar el número formateado
  dtostrf(tflops, 8, 6, buffer); // (valor, ancho mínimo, decimales, buffer)
  tft.print(buffer);
  tft.setCursor(4, 115);
  tft.setTextColor(colors[colorI]);
  tft.print("GOPS");
  tft.setTextColor(TFT_WHITE);
  float gops = ((hashRa * 1000) * 2560) / 1000000;
  tft.setCursor(4, 125);
  tft.print(gops);
  tft.setTextColor(colors[colorI]);
  tft.setCursor(65, 115);
  tft.print("MH/s");
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(65, 125);
  float mhss = (hashRa / 1000);
  tft.print(mhss);
  tft.setCursor(193, 23);
  tft.setTextColor(colors[colorI]);
  tft.print("... DATOS NERD II ...");
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(230, 40);
  tft.setTextColor(colors[colorI]);
  tft.print("MOPS");
  tft.setTextColor(TFT_WHITE);
  float Mops = ((hashRa * 1000) * 2560) / 1000000;
  tft.setCursor(230, 50);
  char bufferrr[10];             // Buffer para almacenar el número formateado
  dtostrf(Mops, 8, 6, bufferrr); // (valor, ancho mínimo, decimales, buffer)
  tft.print(bufferrr);
  tft.setCursor(230, 65);
  tft.setTextColor(colors[colorI]);
  tft.print("TOPS");
  tft.setTextColor(TFT_WHITE);
  float Tops = ((hashRa * 1000) * 2560) / 1000000000;
  tft.setCursor(230, 75);
  char bufferr[10];
  dtostrf(Tops, 8, 6, bufferr);
  tft.print(bufferr);
  tft.drawLine(230, 90, 310, 90, TFT_WHITE);
  tft.setCursor(236, 95);
  tft.setTextColor(colors[colorI]);
  tft.print("TEMPERATURA");
  tft.setTextSize(3);
  tft.setCursor(246, 107);
  tft.setTextColor(TFT_WHITE);
  tft.print(data.temp);
  tft.setTextSize(1);
  tft.setCursor(283, 103);
  tft.print("o");
  tft.setCursor(208, 138);
  tft.print("HMS");
  std::string quediase = obtenerDiaSemana(std::string(dataa.currentDate.c_str()));
  tft.setCursor(153, 68);
  tft.setTextColor(colors[colorI]);
  tft.print(quediase.c_str());
  tft.setTextColor(TFT_WHITE);
  int X_INICIO = 230;                                      // Columna fija
  int X_FINAL = 310;                                       // Base de la barra
  int Y_POS = 147;                                         // Punto más alto
  int longitud_total = X_FINAL - X_INICIO;                 // Longitud total de la barra (80 píxeles)
  int longitud_pintada = (longitud_total * segundos) / 59; // Porcentaje de la barra según los segundos
  tft.fillRect(X_INICIO, Y_POS - 1, longitud_total + 1, 3, TFT_BLACK);
  tft.drawLine(X_INICIO, Y_POS, X_INICIO + longitud_pintada, Y_POS, TFT_WHITE);
  Y_POS = 135;
  longitud_pintada = (longitud_total * horas) / 23; // Porcentaje de la barra según las horas
  tft.fillRect(X_INICIO, Y_POS - 1, longitud_total + 1, 3, TFT_BLACK);
  tft.drawLine(X_INICIO, Y_POS, X_INICIO + longitud_pintada, Y_POS, TFT_WHITE);
  Y_POS = 141;
  longitud_pintada = (longitud_total * minutos) / 59; // Porcentaje de la barra según los minutos
  tft.fillRect(X_INICIO, Y_POS - 1, longitud_total + 1, 3, TFT_BLACK);
  tft.drawLine(X_INICIO, Y_POS, X_INICIO + longitud_pintada, Y_POS, TFT_WHITE);
  actualizarcalen = 0;
  actualizarc = 0;
  actuanot = 0;
}

// Función tDisplay_m8axScreen16: Actualiza la pantalla con datos de minería, hora mundial, y datos de configuración del sistema.
// Utiliza imágenes de fondo que cambian aleatoriamente cada 30 segundos y muestra la hora de 20 ciudades en varias columnas.
// Además, incluye la visualización de los datos de minería, temperatura y estado actual.

void tDisplay_m8axScreen16(unsigned long mElapsed)
{
  mining_data data = getMiningData(mElapsed);
  clock_data dataa = getClockData(mElapsed);
  monedilla = getCoinData(mElapsed);
  relojete = getClockData(mElapsed);
  mineria = getMiningData(mElapsed);
  unsigned long segundo = timeClient.getSeconds();
  int segundos = segundo % 60;
  int horita = dataa.currentTime.substring(0, 2).toInt();
  int minutitos = dataa.currentTime.substring(3, 5).toInt();
  int currentDay = dataa.currentDate.substring(0, 2).toInt();
  int currentMonth = dataa.currentDate.substring(3, 5).toInt();
  incrementCounter();
  if (actualizarcalen == 0)
  {
    background.pushImage(0, 0, ImagenM8AXWidth, ImagenM8AXHeight, ImagenM8AX);
    background.pushSprite(0, 0);
    actualizarcalen++;
  }
  background.pushSprite(0, 0);
  Serial.printf("M8AX - >>> Completados %s Share(s), %s Khashes, Prom. Hashrate %s KH/s %s°\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str(), data.temp.c_str());
  if (segundo % 30 == 0)
  {
    int random_number = 1 + (esp_random() % 4);
    if (random_number == 1)
    {
      background.pushImage(0, 0, ImagenM8AXWidth, ImagenM8AXHeight, ImagenM8AX);
      background.pushSprite(0, 0);
    }
    else if (random_number == 2)
    {
      background.pushImage(0, 0, M8AXRelojLunarWidth, M8AXRelojLunarHeight, M8AXRelojLunar);
      background.pushSprite(0, 0);
    }
    else if (random_number == 3)
    {
      background.pushImage(0, 0, M8AXQuote1Width, M8AXQuote1Height, M8AXQuote1);
      background.pushSprite(0, 0);
    }
    else if (random_number == 4)
    {
      background.pushImage(0, 0, M8AXQuote2Width, M8AXQuote2Height, M8AXQuote2);
      background.pushSprite(0, 0);
    }
  }
  int y = 30; // Coordenada Y inicial
  int horitaUTC = horita - (esHorarioDeVerano(currentMonth, currentDay) ? 2 : 1);
  for (int i = 0; i < 20; i++)
  {
    // Calcula la hora local para cada ciudad, ajustando con la zona horaria de cada ciudad
    int ciudadHora = horitaUTC + zonasHorarias[i]; // Ajusta la hora sumando la zona horaria de cada ciudad

    // Ajusta la hora si excede 24 horas o es menor a 0
    if (ciudadHora < 0)
    {
      ciudadHora += 24;
    }
    if (ciudadHora >= 24)
    {
      ciudadHora -= 24;
    }
    // Formatea la hora (sin segundos)
    char timeStr[9];
    sprintf(timeStr, "%02d:%02d:%02d", ciudadHora, minutitos, segundos);

    // Calcula la posición X en base a la columna (4 columnas)
    int x = (i % 4) * 80; // Las ciudades se distribuyen en 4 columnas

    // Muestra la ciudad y la hora
    colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
    tft.drawLine(0, 20, 320, 20, colors[colorI]); // Dibujar línea de (0, y) a (320, y)
    tft.setTextColor(colors[colorI]);
    tft.setTextSize(1);
    tft.setCursor(x + 5, y);
    tft.print(ciudades[i]);
    tft.setCursor(x + 5, y + 10); // Posición de la hora
    tft.print(timeStr);
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(1, 6);
    tft.setTextSize(1);
    int millonario = atoi(data.valids.c_str());
    if (millonario == 0)
    {
      tft.print(data.currentTime + " - " + dataa.currentDate + " - " + data.currentHashRate + " KH/s - " + data.temp + " Grados." + " NO RICO");
    }
    else
    {
      tft.print(data.currentTime + " - " + dataa.currentDate + " - " + data.currentHashRate + " KH/s - " + data.temp + " Grados." + " SI RICO");
    }

    // Cambia la posición Y para la siguiente fila (5 filas)
    if ((i + 1) % 4 == 0)
    {          // Se cambia después de cada 4 columnas
      y += 30; // Aumenta la posición Y después de cada 4 ciudades
    }
  }
  actualizarc = 0;
  actuanot = 0;
}

// Función tDisplay_m8axScreen17: Actualiza la pantalla con información de minería, dirección BTC y muestra un código QR.
// La pantalla muestra información de minería, una dirección de Bitcoin, y un QR asociado a esa dirección.
// Además, cada 30 segundos cambia la imagen de fondo de manera aleatoria, y se muestran letras relacionadas con Bitcoin en diferentes posiciones de la pantalla.

void tDisplay_m8axScreen17(unsigned long mElapsed)
{
  mining_data data = getMiningData(mElapsed);
  monedilla = getCoinData(mElapsed);
  relojete = getClockData(mElapsed);
  mineria = getMiningData(mElapsed);
  String btcm8 = "bitcoin:bc1qljq00pm2plq2l9jxzdzt0xc8t79j9wcmu7r8em";
  unsigned long segundo = timeClient.getSeconds();
  incrementCounter();
  if (actualizarcalen == 0)
  {
    background.pushImage(0, 0, ImagenM8AXWidth, ImagenM8AXHeight, ImagenM8AX);
    background.pushSprite(0, 0);
    actualizarcalen++;
  }
  background.pushSprite(0, 0);
  Serial.printf("M8AX - >>> Completados %s Share(s), %s Khashes, Prom. Hashrate %s KH/s %s°\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str(), data.temp.c_str());
  if (segundo % 30 == 0)
  {
    int random_number = 1 + (esp_random() % 4);
    if (random_number == 1)
    {
      background.pushImage(0, 0, ImagenM8AXWidth, ImagenM8AXHeight, ImagenM8AX);
      background.pushSprite(0, 0);
    }
    else if (random_number == 2)
    {
      background.pushImage(0, 0, M8AXRelojLunarWidth, M8AXRelojLunarHeight, M8AXRelojLunar);
      background.pushSprite(0, 0);
    }
    else if (random_number == 3)
    {
      background.pushImage(0, 0, M8AXQuote1Width, M8AXQuote1Height, M8AXQuote1);
      background.pushSprite(0, 0);
    }
    else if (random_number == 4)
    {
      background.pushImage(0, 0, M8AXQuote2Width, M8AXQuote2Height, M8AXQuote2);
      background.pushSprite(0, 0);
    }
  }
  actualizarc = 0;
  actual = 0;
  actuanot = 0;
  correccion = 0;
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  tft.setTextColor(colors[colorI]);
  tft.setTextSize(3);
  tft.setCursor(20, 1);
  tft.print("D");
  tft.setCursor(20, 30);
  tft.print("O");
  tft.setCursor(20, 60);
  tft.print("N");
  tft.setCursor(20, 90);
  tft.print("A");
  tft.setCursor(20, 120);
  tft.print("M");
  tft.setCursor(20, 149);
  tft.print("E");
  tft.setCursor(50, 1);
  tft.print("B");
  tft.setCursor(50, 26);
  tft.print("I");
  tft.setCursor(50, 51);
  tft.print("T");
  tft.setCursor(50, 76);
  tft.print("C");
  tft.setCursor(50, 101);
  tft.print("O");
  tft.setCursor(50, 126);
  tft.print("I");
  tft.setCursor(50, 151);
  tft.print("N");
  tft.setCursor(242, 20);
  tft.print("S");
  tft.setCursor(242, 55);
  tft.print("I");
  tft.setCursor(242, 106);
  tft.print("T");
  tft.setCursor(242, 131);
  tft.print("E");
  tft.setCursor(272, 1);
  tft.print("M");
  tft.setCursor(272, 26);
  tft.print("O");
  tft.setCursor(272, 51);
  tft.print("L");
  tft.setCursor(272, 76);
  tft.print("A");
  tft.setCursor(272, 101);
  tft.print(" ");
  tft.setCursor(272, 126);
  tft.print("M");
  tft.setCursor(272, 151);
  tft.print("I");
  tft.setCursor(300, 1);
  tft.print("T");
  tft.setCursor(300, 26);
  tft.print("R");
  tft.setCursor(300, 51);
  tft.print("A");
  tft.setCursor(300, 76);
  tft.print("B");
  tft.setCursor(300, 101);
  tft.print("A");
  tft.setCursor(300, 126);
  tft.print("J");
  tft.setCursor(300, 151);
  tft.print("O");
  tft.setCursor(128, 162);
  tft.setTextSize(1);
  tft.print(data.currentHashRate + " KH/s");
  uint16_t colorss[] = {TFT_WHITE, TFT_YELLOW, TFT_CYAN, TFT_GREENYELLOW}; // Array de colores
  int colorrr = esp_random() % 4;
  dibujaQR(btcm8, (320 - 150) / 2, (170 - 150) / 2, 150, colorss[colorrr]); // Dibuja el QR centrado en la pantalla 320x170
}

// tDisplay_m8axScreen18: Esta función se encarga de actualizar la pantalla TFT con diversos datos relacionados con el proceso de minería y el reloj.
// La pantalla muestra estadísticas de minería, un reloj actualizado, números aleatorios, y un "objetivo" para los números generados,
// así como varios elementos gráficos (como líneas, imágenes, y códigos QR). Además, se calcula un valor relacionado con la minería y el tiempo,
// y se actualizan los valores visualizados en la pantalla dependiendo del estado actual de la minería.

// Parámetros:
// - mElapsed: Un valor de tiempo transcurrido utilizado para obtener datos de minería y del reloj.

// Procedimiento:
// 1. Obtiene los datos de minería, reloj y moneda mediante las funciones `getMiningData`, `getClockData` y `getCoinData`.
// 2. Extrae la hora, minuto y segundos actuales, además de generar números aleatorios y un destino para el "juego de números".
// 3. Si el segundo actual es divisible entre 30, se elige aleatoriamente una imagen para mostrar en la pantalla (por ejemplo, una imagen de fondo o una cita).
// 4. Actualiza la pantalla con texto informativo: estadísticas de minería (como shares completados, tasa de hash y temperatura),
//    y muestra los números generados junto con el objetivo a alcanzar en el "juego de números".
// 5. Se dibujan líneas en la pantalla para separar las secciones de información.
// 6. Calcula las operaciones para el "juego de números" y muestra el resultado en la pantalla.
// 7. Dibuja códigos QR con la hora, minuto y segundo actuales, utilizando colores aleatorios.
// 8. Muestra un reloj visualizado en la parte superior con los valores de hora, minuto y segundo.
// 9. Finalmente, imprime en el monitor serial información relevante sobre los números generados y el objetivo.

void tDisplay_m8axScreen18(unsigned long mElapsed)
{
  mining_data data = getMiningData(mElapsed);
  clock_data dataa = getClockData(mElapsed);
  monedilla = getCoinData(mElapsed);
  relojete = getClockData(mElapsed);
  mineria = getMiningData(mElapsed);
  int horita = dataa.currentTime.substring(0, 2).toInt();
  int minutitos = dataa.currentTime.substring(3, 5).toInt();
  unsigned long segundo = timeClient.getSeconds();
  int segundos = segundo % 60;
  incrementCounter();
  if (actualizarcalen == 0)
  {
    background.pushImage(0, 0, ImagenM8AXWidth, ImagenM8AXHeight, ImagenM8AX);
    background.pushSprite(0, 0);
    actualizarcalen++;
  }
  background.pushSprite(0, 0);
  Serial.printf("\nM8AX - >>> Completados %s Share(s), %s Khashes, Prom. Hashrate %s KH/s %s°\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str(), data.temp.c_str());
  if (segundo % 30 == 0)
  {
    int random_number = 1 + (esp_random() % 4);
    if (random_number == 1)
    {
      background.pushImage(0, 0, ImagenM8AXWidth, ImagenM8AXHeight, ImagenM8AX);
      background.pushSprite(0, 0);
    }
    else if (random_number == 2)
    {
      background.pushImage(0, 0, M8AXRelojLunarWidth, M8AXRelojLunarHeight, M8AXRelojLunar);
      background.pushSprite(0, 0);
    }
    else if (random_number == 3)
    {
      background.pushImage(0, 0, M8AXQuote1Width, M8AXQuote1Height, M8AXQuote1);
      background.pushSprite(0, 0);
    }
    else if (random_number == 4)
    {
      background.pushImage(0, 0, M8AXQuote2Width, M8AXQuote2Height, M8AXQuote2);
      background.pushSprite(0, 0);
    }
  }
  actualizarc = 0;
  actual = 0;
  correccion = 0;
  actuanot = 0;
  tft.setTextColor(TFT_WHITE);
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  tft.setTextSize(1);
  tft.setCursor(1, 90);
  tft.print("'Las Cifras' Num - " + String(totalci) + ". AC - " + String(aciertos) + " FA - " + String(fallos));
  tft.drawLine(0, 100, 320, 100, colors[colorI]); // Dibujar línea de (0, y) a (320, y)
  int numeritos[6];
  int destino = 1 + (esp_random() % 1000); // Usa esp_random() en lugar de rand()
  generate_random_numbers(numeritos, 6, 1, 100);
  // Imprime los números generados
  tft.setCursor(2, 104);
  tft.print("Numeros - ");
  tft.print(numeritos[0]);
  tft.print(" ");
  tft.print(numeritos[1]);
  tft.print(" ");
  tft.print(numeritos[2]);
  tft.print(" ");
  tft.print(numeritos[3]);
  tft.print(" ");
  tft.print(numeritos[4]);
  tft.print(" ");
  tft.print(numeritos[5]);
  tft.setCursor(2, 114);
  tft.print("Numero Objetivo - " + String(destino));
  tft.setCursor(160, 114);
  int millonario = atoi(data.valids.c_str());
  if (millonario == 0)
  {
    tft.print("NoRICO - ");
    tft.print(data.currentHashRate);
    tft.print(" KH/s. ");
    tft.print(data.temp);
    tft.print(" g");
  }
  else
  {
    tft.print("SiRICO - ");
    tft.print(data.currentHashRate);
    tft.print(" KH/s. ");
    tft.print(data.temp);
    tft.print(" g");
  }
  tft.drawLine(0, 124, 320, 124, colors[colorI]); // Dibujar línea de (0, y) a (320, y)
  tft.setCursor(2, 128);
  Serial.print("M8AX - Números: ");
  for (int i = 0; i < 6; i++)
  {
    Serial.print(numeritos[i]); // Imprime el número
    Serial.print(" ");          // Imprime un espacio
  }
  Serial.println();
  Serial.print("M8AX - Objetivo: ");
  Serial.println("M8AX . " + String(destino));

  int X_INICIO = 173;                                      // Columna fija
  int X_FINAL = 315;                                       // Base de la barra
  int Y_POS = 107;                                         // Punto más alto
  int longitud_total = X_FINAL - X_INICIO;                 // Longitud total de la barra (80 píxeles)
  int longitud_pintada = (longitud_total * segundos) / 59; // Porcentaje de la barra según los segundos
  tft.fillRect(X_INICIO, Y_POS - 1, longitud_total + 1, 3, TFT_BLACK);
  tft.drawLine(X_INICIO, Y_POS, X_INICIO + longitud_pintada, Y_POS, TFT_WHITE);

  // Calcula las operaciones

  calculate_operations(numeritos, destino, result);
  tft.print(result);
  uint16_t colorss[] = {TFT_WHITE, TFT_YELLOW, TFT_CYAN, TFT_GREENYELLOW, TFT_LIGHTGREY, TFT_SILVER}; // Array de colores
  int colorrr = esp_random() % 6;
  dibujaQR(String(horita), 0, 0, 98, colorss[colorrr]); // Dibuja el QR centrado en la pantalla 320x170
  colorrr = esp_random() % 6;
  dibujaQR(String(minutitos), 115, 0, 98, colorss[colorrr]); // Dibuja el QR centrado en la pantalla 320x170
  colorrr = esp_random() % 6;
  dibujaQR(String(segundos), 228, 0, 98, colorss[colorrr]); // Dibuja el QR centrado en la pantalla 320x170
  tft.setTextSize(5);
  tft.setCursor(87, 30);
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  tft.setTextColor(colors[colorI]);
  tft.print(":");
  tft.setCursor(202, 30);
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  tft.setTextColor(colors[colorI]);
  tft.setTextColor(TFT_WHITE);
  tft.print(":");
  Serial.print("M8AX - " + String(result));
  tft.setTextSize(1);
}

// Función principal para mostrar la simulación de un dado y una moneda en la pantalla,
// junto con información relacionada con la minería de criptomonedas, la hora y la temperatura.
// Esta función se ejecuta repetidamente en cada ciclo de actualización, mostrando datos
// como la tasa de hashrate, el estado del dado (número entre 1 y 6), y el resultado de la
// moneda (cara o cruz) junto con la hora actual y la temperatura de la minería

void monedaYdado(unsigned long mElapsed)
{
  mining_data data = getMiningData(mElapsed);
  clock_data dataa = getClockData(mElapsed);
  monedilla = getCoinData(mElapsed);
  relojete = getClockData(mElapsed);
  mineria = getMiningData(mElapsed);
  int horita = dataa.currentTime.substring(0, 2).toInt();
  int minutitos = dataa.currentTime.substring(3, 5).toInt();
  unsigned long segundo = timeClient.getSeconds();
  int segundos = segundo % 60;
  char horaStr[9]; // Buffer para almacenar la hora en formato HH:MM:SS
  sprintf(horaStr, "%02d:%02d:%02d", horita, minutitos, segundos);
  incrementCounter();
  if (actualizarc == 0)
  {
    background.pushImage(0, 0, ImagenM8AXWidth, ImagenM8AXHeight, ImagenM8AX);
    background.pushSprite(0, 0);
    actualizarc++;
  }
  background.pushSprite(0, 0);
  Serial.printf("M8AX ->>> Completados %s Share(s), %s Khashes, Prom. Hashrate %s KH/s %s°\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str(), data.temp.c_str());
  if (segundo % 30 == 0)
  {
    int random_number = 1 + (esp_random() % 4);
    if (random_number == 1)
    {
      background.pushImage(0, 0, ImagenM8AXWidth, ImagenM8AXHeight, ImagenM8AX);
      background.pushSprite(0, 0);
    }
    else if (random_number == 2)
    {
      background.pushImage(0, 0, M8AXRelojLunarWidth, M8AXRelojLunarHeight, M8AXRelojLunar);
      background.pushSprite(0, 0);
    }
    else if (random_number == 3)
    {
      background.pushImage(0, 0, M8AXQuote1Width, M8AXQuote1Height, M8AXQuote1);
      background.pushSprite(0, 0);
    }
    else if (random_number == 4)
    {
      background.pushImage(0, 0, M8AXQuote2Width, M8AXQuote2Height, M8AXQuote2);
      background.pushSprite(0, 0);
    }
  }
  actual = 0;
  correccion = 0;
  actuanot = 0;
  actualizarcalen = 0;
  int dado = esp_random() % 6 + 1; // Números del 1 al 6
  int moneda = esp_random() % 2;   // 0 (cara), 1 (cruz)
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(108, 88);
  tft.setTextSize(1);
  tft.print("DADO - ");
  tft.print(dado);
  tft.setCursor(108, 99);
  tft.print("MONEDA - ");
  tft.print(moneda == 0 ? "CARA" : "CRUZ");
  tft.setCursor(108, 110);
  tft.print(horaStr);
  tft.setCursor(108, 121);
  tft.print("TEMP - ");
  tft.print(data.temp.c_str());
  tft.print(" G");
  tft.setTextSize(2);
  tft.setCursor(46, 4);
  tft.print("EL DADO Y LA MONEDA");
  tft.setCursor(46, 27);
  tft.print("DE LA SUERTE");
  String cadena = data.currentHashRate.c_str(); // O "234,65" si necesitas reemplazar la coma
  cadena.replace(',', '.');                     // Si es necesario
  String valor = data.currentHashRate;
  int parteEntera = valor.substring(0, valor.indexOf(".")).toInt();
  int parteDecimal = valor.substring(valor.indexOf(".") + 1).toInt();
  dibujarDado(dado, 60, 85);
  dibujarMoneda(moneda, 250, 80);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(0, 132);
  tft.setTextSize(2);
  tft.setTextDatum(MC_DATUM); // Centra el texto
  tft.print(numeroAEscrito(parteEntera, false) + " Con ");
  tft.print(numeroAEscrito(parteDecimal, true) + " KH/s.");
  tft.setCursor(115, 50);
  tft.setTextSize(2);
  tft.print(cadena);
  tft.setCursor(114, 69);
  tft.print(" KH/s");
  tft.setTextSize(1);
}

// Funcionalidad:
// 1. Recupera los datos de minería utilizando la función `getMiningData()` para mostrar información sobre
//    el número de "shares" completados, los "khashes" totales, la tasa de hashrate y la temperatura.
// 2. Si el contador `actuanot` alcanza un múltiplo de 10 o si `correccion` es igual a 0, selecciona
//    aleatoriamente un fondo de una lista de imágenes predefinidas para mostrar en la pantalla, usando
//    la función `esp_random()` para generar el índice aleatorio.
// 3. Actualiza la pantalla con el fondo seleccionado y dibuja el texto centrado "M 8 A X", "JUEGO DE", y "LA VIDA",
//    indicativos del nombre del juego y su propósito visual.
// 4. Rellena la pantalla con un fondo negro y reinicia la cuadrícula del "Juego de la Vida" llamando a las
//    funciones `initGrid()` y `drawGrid()`.
// 5. Inicia una simulación del "Juego de la Vida" con un número máximo de generaciones (`MAX_GEN_COUNT`).
//    En cada generación, se calcula el siguiente estado de la cuadrícula (celulas vivas/muertas) mediante la
//    función `computeCA()`, luego se actualiza la cuadrícula y se vuelve a dibujar en pantalla, haciendo uso de
//    la función `drawGrid()`. Este ciclo se repite con un pequeño retraso (`GEN_DELAY`) entre cada paso.
// 6. Los valores de la cuadrícula se actualizan entre generaciones con la lógica de pasar de `newgrid` a `grid`,
//    donde las celdas se actualizan en función de las reglas del "Juego de la Vida".
// 7. Después de completar todas las generaciones, se restablecen varios contadores y banderas como `actual`,
//    `actualizarc`, `actualizarcalen` y `correccion` para la próxima iteración de la función.
// 8. La función se asegura de dibujar y mostrar información visual dinámica como el "Juego de la Vida" y las
//    estadísticas de minería, en conjunto con las imágenes de fondo aleatorias, creando una experiencia visual
//    completa e interactiva para el usuario.

void tDisplay_m8axvida(unsigned long mElapsed)
{
  mining_data data = getMiningData(mElapsed);
  monedilla = getCoinData(mElapsed);
  relojete = getClockData(mElapsed);
  mineria = getMiningData(mElapsed);
  incrementCounter();
  Serial.printf("M8AX ->>> Completados %s Share(s), %s Khashes, Prom. Hashrate %s KH/s %s°\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str(), data.temp.c_str());
  if (actuanot % 10 == 0 || correccion == 0)
  {
    int random_number = 1 + (esp_random() % 4);
    if (random_number == 1)
    {
      background.pushImage(0, 0, ImagenM8AXWidth, ImagenM8AXHeight, ImagenM8AX);
      background.pushSprite(0, 0);
    }
    else if (random_number == 2)
    {
      background.pushImage(0, 0, M8AXRelojLunarWidth, M8AXRelojLunarHeight, M8AXRelojLunar);
      background.pushSprite(0, 0);
    }
    else if (random_number == 3)
    {
      background.pushImage(0, 0, M8AXQuote1Width, M8AXQuote1Height, M8AXQuote1);
      background.pushSprite(0, 0);
    }
    else if (random_number == 4)
    {
      background.pushImage(0, 0, M8AXQuote2Width, M8AXQuote2Height, M8AXQuote2);
      background.pushSprite(0, 0);
    }
    correccion = 1;
    tft.setTextSize(2);
    drawCenteredText("M 8 A X", 30, 100);
    drawCenteredText("J U E G O   D E", 70, 100);
    drawCenteredText("L A   V I D A", 110, 100);
    tft.setTextSize(1);
    tft.setCursor(0, 0);
    tft.fillScreen(TFT_BLACK);
    initGrid();
    genCount = MAX_GEN_COUNT;
    drawGrid();
    for (int gen = 0; gen < genCount; gen++)
    {
      computeCA();
      drawGrid();
      delay(GEN_DELAY);
      for (int16_t x = 1; x < GRIDX - 1; x++)
      {
        for (int16_t y = 1; y < GRIDY - 1; y++)
        {
          grid[x][y] = newgrid[x][y];
        }
      }
    }
  }
  actualizarcalen = 0;
  actualizarc = 0;
  actual = 0;
}

// Función que gestiona la visualización de un reloj en formato de números romanos en la pantalla TFT,
// junto con información sobre minería, temperatura, tiempo y día, en una interfaz de usuario interactiva.
// La función actualiza la pantalla con la hora actual, el día de la semana, la temperatura, el hash rate y
// otros datos en un formato llamativo y dinámico, con colores aleatorios para los textos y barras de progreso
// para representar el tiempo y la actividad de minería.
//
// Parámetros:
// - mElapsed: Tiempo transcurrido desde el inicio o la última actualización.
//
// Funcionalidad:
// 1. Recupera los datos de minería, reloj y moneda utilizando las funciones `getMiningData()`, `getClockData()` y
//    `getCoinData()` para obtener la información que se mostrará en la pantalla.
// 2. Extrae y calcula los componentes de la hora, minuto y segundo a partir de los datos de reloj, dividiendo los
//    valores de horas, minutos y segundos en dígitos individuales para su visualización.
// 3. Calcula el tiempo transcurrido en el día en segundos (`segundosDelDia`) y convierte las horas, minutos y
//    segundos en números romanos utilizando la función `numeroAEscrito()` para mostrarlos en la pantalla.
// 4. Recupera el día de la semana y la fecha actual en formato "día/mes/año" para mostrar información adicional
//    sobre la fecha y el estado del reloj.
// 5. Si el valor de `actualizarc` es 0, selecciona aleatoriamente un fondo visual de una lista predefinida de
//    imágenes usando el valor generado por `esp_random()`, y luego lo muestra en la pantalla.
// 6. Muestra la hora en formato de números romanos (de 1 a 10 para las horas, minutos y segundos) en diferentes
//    posiciones de la pantalla con colores aleatorios.
// 7. Muestra la fecha, el hash rate, y si el usuario es "millonario" (según un cálculo basado en `valids` de minería)
//    en varias posiciones de la pantalla, usando colores aleatorios para cada texto.
// 8. Actualiza barras de progreso que representan el tiempo transcurrido en el día (0-24 horas) y el nivel de
//    actividad de minería (0-360 KH/s). Las barras cambian de color aleatoriamente según el valor calculado.
// 9. La función también actualiza la información en la pantalla de manera continua con cada llamada, mostrando
//    datos actualizados sobre el reloj, minería y temperatura, lo que permite tener un reloj interactivo con
//    detalles sobre la actividad de la minería en tiempo real.
// 10. Al final de la ejecución, todos los textos y gráficos se muestran con colores aleatorios para mejorar
//     la estética visual.

void RelojDeNumeros(unsigned long mElapsed)
{
  mining_data data = getMiningData(mElapsed);
  clock_data dataa = getClockData(mElapsed);
  monedilla = getCoinData(mElapsed);
  relojete = getClockData(mElapsed);
  mineria = getMiningData(mElapsed);
  int millonario = atoi(data.valids.c_str());
  incrementCounter();
  mirarTiempo = 0;
  unsigned long segundo = timeClient.getSeconds();
  int horas = dataa.currentTime.substring(0, 2).toInt();
  int minutos = dataa.currentTime.substring(3, 5).toInt();
  int segundos = segundo % 60;
  int horis1 = horas / 10;       // Primer dígito de las horas
  int horis2 = horas % 10;       // Segundo dígito de las horas
  int minutis1 = minutos / 10;   // Primer dígito de los minutos
  int minutis2 = minutos % 10;   // Segundo dígito de los minutos
  int segundis1 = segundos / 10; // Primer dígito de los segundos
  int segundis2 = segundos % 10; // Segundo dígito de los segundos
  if (segundos % 59 == 0)
  {
    actualizarc = 0;
  }
  std::string quediase = obtenerDiaSemana(std::string(dataa.currentDate.c_str()));
  int dia = dataa.currentDate.substring(0, 2).toInt();
  int mes = dataa.currentDate.substring(3, 5).toInt();
  int anio = dataa.currentDate.substring(6, 10).toInt();
  int horis = dataa.currentTime.substring(0, 2).toInt();
  int mins = dataa.currentTime.substring(3, 5).toInt();
  int segundosDelDia = (horis * 3600) + (mins * 60) + segundos;
  String hRoman1 = numeroAEscrito(horis1);
  hRoman1.toUpperCase();
  String hRoman2 = numeroAEscrito(horis2);
  hRoman2.toUpperCase();
  String mRoman1 = numeroAEscrito(minutis1);
  mRoman1.toUpperCase();
  String mRoman2 = numeroAEscrito(minutis2);
  mRoman2.toUpperCase();
  String sRoman1 = numeroAEscrito(segundis1);
  sRoman1.toUpperCase();
  String sRoman2 = numeroAEscrito(segundis2);
  sRoman2.toUpperCase();
  String RDia = String(dia);
  String RMes = String(mes);
  String RAnio = String(anio);
  String TempCPU = String(data.temp.c_str());
  if (actualizarc == 0)
  {
    random_number = 1 + (esp_random() % 4);
  }
  if (random_number == 1)
  {
    background.pushImage(0, 0, ImagenM8AXWidth, ImagenM8AXHeight, ImagenM8AX);
    background.pushSprite(0, 0);
  }
  else if (random_number == 2)
  {
    background.pushImage(0, 0, M8AXRelojLunarWidth, M8AXRelojLunarHeight, M8AXRelojLunar);
    background.pushSprite(0, 0);
  }
  else if (random_number == 3)
  {
    background.pushImage(0, 0, M8AXQuote1Width, M8AXQuote1Height, M8AXQuote1);
    background.pushSprite(0, 0);
  }
  else if (random_number == 4)
  {
    background.pushImage(0, 0, M8AXQuote2Width, M8AXQuote2Height, M8AXQuote2);
    background.pushSprite(0, 0);
  }
  actualizarc++;
  Serial.printf("M8AX - >>> Completados %s Share(s), %s Khashes, Prom. Hashrate %s KH/s %s°\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str(), data.temp.c_str());
  tft.setTextSize(3);
  tft.setCursor(3, 2);
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  tft.setTextColor(colors[colorI]);
  tft.print(hRoman1);
  tft.setCursor(133, 2);
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  tft.setTextColor(colors[colorI]);
  tft.print(hRoman2);
  tft.setCursor(3, 77);
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  tft.setTextColor(colors[colorI]);
  tft.print(mRoman1);
  tft.setCursor(133, 77);
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  tft.setTextColor(colors[colorI]);
  tft.print(mRoman2);
  tft.setCursor(3, 148);
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  tft.setTextColor(colors[colorI]);
  tft.print(sRoman1);
  tft.setCursor(133, 148);
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  tft.setTextColor(colors[colorI]);
  tft.print(sRoman2);
  tft.setTextSize(1);
  tft.setCursor(250, 2);
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  tft.setTextColor(colors[colorI]);
  tft.print("d " + RDia);
  tft.setCursor(250, 16);
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  tft.setTextColor(colors[colorI]);
  tft.print("m " + RMes);
  tft.setCursor(250, 30);
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  tft.setTextColor(colors[colorI]);
  tft.print("a " + RAnio);
  tft.setCursor(250, 44);
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  tft.setTextColor(colors[colorI]);
  tft.print(String(quediase.c_str()));
  tft.setCursor(250, 58);
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  tft.setTextColor(colors[colorI]);
  tft.print(String(data.currentHashRate.c_str()) + " KH/s");
  if (millonario == 1)
  {
    tft.setCursor(250, 72);
    colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
    tft.setTextColor(colors[colorI]);
    tft.print("ERES RICO");
  }
  else
  {
    tft.setCursor(250, 72);
    colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
    tft.setTextColor(colors[colorI]);
    tft.print("NO RICO");
  }
  tft.setCursor(250, 93);
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  tft.setTextColor(colors[colorI]);
  tft.print("V.BLOCKS");
  tft.setCursor(260, 109);
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  tft.setTextColor(colors[colorI]);
  tft.setTextSize(5);
  tft.print(millonario);
  tft.setTextSize(1);
  tft.setCursor(250, 153);
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  tft.setTextColor(colors[colorI]);
  tft.print("T " + TempCPU + "g");
  tft.setTextSize(1);
  tft.setCursor(5, 50);
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  tft.setTextColor(colors[colorI]);
  tft.print("0 KH/s ");
  tft.setCursor(192, 50);
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  tft.setTextColor(colors[colorI]);
  tft.print("360 KH/s");
  int grosor = 5;
  int X_INICIO = 5;                                                             // Columna fija
  int X_FINAL = 242;                                                            // Base de la barra
  int Y_POS = 43;                                                               // Punto más alto
  int longitud_total = X_FINAL - X_INICIO;                                      // Longitud total de la barra (80 píxeles)
  int longitud_pintada = (longitud_total * data.currentHashRate.toInt()) / 360; // Porcentaje de la barra según los segundos
  tft.fillRect(X_INICIO, Y_POS - (grosor / 2), longitud_total + 1, grosor, TFT_BLACK);
  tft.fillRect(X_INICIO, Y_POS - (grosor / 2), longitud_pintada, grosor, colors[colorI]);
  tft.setCursor(5, 124);
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  tft.setTextColor(colors[colorI]);
  tft.print("0 Horas");
  tft.setCursor(192, 124);
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  tft.setTextColor(colors[colorI]);
  tft.print("24 Horas");
  grosor = 5;
  X_INICIO = 4;                                                 // Columna fija
  X_FINAL = 242;                                                // Base de la barra
  Y_POS = 117;                                                  // Punto más alto
  longitud_total = X_FINAL - X_INICIO;                          // Longitud total de la barra (80 píxeles)
  longitud_pintada = (longitud_total * segundosDelDia) / 86400; // Porcentaje de la barra según los segundos
  tft.fillRect(X_INICIO, Y_POS - (grosor / 2), longitud_total + 1, grosor, TFT_BLACK);
  tft.fillRect(X_INICIO, Y_POS - (grosor / 2), longitud_pintada, grosor, colors[colorI]);
}

// Función que maneja la actualización de la pantalla con información en texto plano sobre minería y reloj,
// mostrando una imagen de fondo seleccionada aleatoriamente, y actualizando los datos de la minería cada ciertos
// intervalos de tiempo. Además, gestiona la visualización continua de información textual sobre el estado de la
// minería y la actividad en tiempo real. También incluye la gestión de imágenes aleatorias para dar variedad visual.
//
// Parámetros:
// - mElapsed: Tiempo transcurrido desde el inicio o la última actualización.
//
// Funcionalidad:
// 1. Recupera los datos de minería, reloj y moneda utilizando las funciones `getMiningData()`, `getClockData()`
//    y `getCoinData()` para obtener la información que se mostrará en la pantalla.
// 2. Se obtiene el tiempo actual (segundos) utilizando la función `timeClient.getSeconds()` para poder manejar
//    los intervalos de tiempo para la actualización de la pantalla.
// 3. Se incrementa el contador mediante `incrementCounter()` para hacer un seguimiento del tiempo transcurrido o
//    el número de actualizaciones.
// 4. Si el valor de `actualizarc` es 0, se selecciona una imagen de fondo aleatoria y se muestra en la pantalla.
//    Las imágenes de fondo pueden ser: `ImagenM8AX`, `M8AXRelojLunar`, `M8AXQuote1` o `M8AXQuote2`.
//    Esta imagen se muestra al inicio de la función y cada vez que `segundo % 30 == 0` para crear un cambio visual.
// 5. La función `datosPantallaTextoPlano()` se encarga de actualizar la información textual que se muestra sobre la
//    pantalla, como el estado de la minería, el tiempo y otros detalles en formato de texto plano.
// 6. El uso de `background.pushImage()` y `background.pushSprite()` asegura que se actualice la pantalla correctamente
//    con la nueva imagen o los datos mostrados.
// 7. La función imprime en el monitor serie el estado de la minería, mostrando el número de shares completados, el
//    total de Khashes, el hash rate actual y la temperatura.
// 8. Cada 30 segundos (`segundo % 30 == 0`), se elige aleatoriamente una nueva imagen de fondo y se muestra en la
//    pantalla, proporcionando una experiencia visual dinámica.
// 9. Cada 5 segundos (`segundo % 5 == 0`), se actualiza nuevamente la información textual sobre la pantalla para
//    mostrar los datos más recientes sobre la minería.
// 10. Al final de la ejecución, los valores de las variables `actual`, `correccion`, `actuanot` y `actualizarcalen`
//     se restablecen a 0 para preparar la función para la siguiente iteración.

void datoTextPlano(unsigned long mElapsed)
{
  mining_data data = getMiningData(mElapsed);
  clock_data dataa = getClockData(mElapsed);
  monedilla = getCoinData(mElapsed);
  relojete = getClockData(mElapsed);
  mineria = getMiningData(mElapsed);
  unsigned long segundo = timeClient.getSeconds();
  incrementCounter();
  if (actualizarc == 0)
  {
    background.pushImage(0, 0, ImagenM8AXWidth, ImagenM8AXHeight, ImagenM8AX);
    background.pushSprite(0, 0);
    actualizarc++;
    background.pushSprite(0, 0);
    datosPantallaTextoPlano();
  }
  Serial.printf("M8AX ->>> Completados %s Share(s), %s Khashes, Prom. Hashrate %s KH/s %s°\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str(), data.temp.c_str());
  if (segundo % 30 == 0)
  {
    int random_number = 1 + (esp_random() % 4);
    if (random_number == 1)
    {
      background.pushImage(0, 0, ImagenM8AXWidth, ImagenM8AXHeight, ImagenM8AX);
      background.pushSprite(0, 0);
    }
    else if (random_number == 2)
    {
      background.pushImage(0, 0, M8AXRelojLunarWidth, M8AXRelojLunarHeight, M8AXRelojLunar);
      background.pushSprite(0, 0);
    }
    else if (random_number == 3)
    {
      background.pushImage(0, 0, M8AXQuote1Width, M8AXQuote1Height, M8AXQuote1);
      background.pushSprite(0, 0);
    }
    else if (random_number == 4)
    {
      background.pushImage(0, 0, M8AXQuote2Width, M8AXQuote2Height, M8AXQuote2);
      background.pushSprite(0, 0);
    }
  }
  if (segundo % 5 == 0)
  {
    background.pushSprite(0, 0);
    datosPantallaTextoPlano();
  }
  actual = 0;
  correccion = 0;
  actuanot = 0;
  actualizarcalen = 0;
}

// Función que muestra una pantalla de carga con efectos visuales aleatorios al iniciar el sistema,
// mostrando una imagen de inicio y el texto que indica la versión actual del programa.
//
// Funcionalidad:
// 1. Se genera un número aleatorio entre 0 y 5 utilizando `esp_random() % 6`, lo que determina cuál de los
//    efectos visuales se aplicará durante la pantalla de carga. Los efectos posibles son:
//    - `cortinas()`
//    - `M8AXTicker2()`
//    - `M8AXTicker()`
//    - `M8AXTicker3()`
//    - `M8AXTicker4()`
//    - `nevar3()`
//    - `animacionInicio()`
//    Cada uno de estos efectos está definido en otras funciones, y se ejecuta uno de ellos de forma aleatoria.
// 2. Después de aplicar el efecto visual correspondiente, se limpia la pantalla con `tft.fillScreen(TFT_BLACK)`
//    para asegurarse de que no haya restos de otras pantallas previas.
// 3. Se dibuja la imagen de inicio en la pantalla con `tft.pushImage(0, 0, initWidth, initHeight, initScreen)`,
//    donde `initScreen` es la imagen de inicio que se va a mostrar, utilizando sus dimensiones `initWidth`
//    y `initHeight`.
// 4. Se establece el color del texto a negro con `tft.setTextColor(TFT_BLACK)` y el tamaño de texto a 1 con
//    `tft.setTextSize(1)` para las siguientes cadenas de texto.
// 5. Se dibuja el texto de la versión actual del programa en las coordenadas (25, 148) usando la fuente
//    `FONT2` y el texto correspondiente a la constante `CURRENT_VERSION`. Esto muestra la versión del software
//    en la pantalla de carga.

void tDisplay_LoadingScreen(void)
{
  int effect = esp_random() % 8;
  switch (effect)
  {
  case 0:
    cortinas();
    break;
  case 1:
    M8AXTicker2();
    break;
  case 2:
    M8AXTicker();
    break;
  case 3:
    M8AXTicker3();
    break;
  case 4:
    M8AXTicker4();
    break;
  case 5:
    nevar3();
    break;
  case 6:
    animacionInicio();
    break;
  case 7:
    M8AXTicker5();
    break;
  }
  tft.fillScreen(TFT_BLACK);
  tft.pushImage(0, 0, initWidth, initHeight, initScreen);
  tft.setTextColor(TFT_BLACK);
  tft.setTextSize(1);
  tft.drawString(CURRENT_VERSION, 25, 148, FONT2);
}

// Función que muestra la pantalla de configuración del dispositivo o programa.
//
// Funcionalidad:
// 1. Se utiliza la función `tft.pushImage` para mostrar una imagen de la pantalla de configuración en el
//    display TFT.
// 2. Los parámetros pasados a `tft.pushImage` son:
//    - `(0, 0)`: Las coordenadas de inicio donde se colocará la imagen en la pantalla, en este caso, la
//      esquina superior izquierda de la pantalla (0,0).
//    - `setupModeWidth`: El ancho de la imagen que se quiere mostrar.
//    - `setupModeHeight`: La altura de la imagen que se quiere mostrar.
//    - `setupModeScreen`: El array de bytes que contiene la imagen que se debe mostrar.

void tDisplay_SetupScreen(void)
{
  tft.pushImage(0, 0, setupModeWidth, setupModeHeight, setupModeScreen);
}

/**
 * Analiza y actualiza datos cada segundo.
 *
 * Esta función realiza múltiples tareas en cada ejecución:
 *
 * 1. Obtiene la hora actual en formato UNIX (epoch) y la convierte a una estructura de tiempo local.
 * 2. Si el sistema ya ha arrancado (startTime > 0), actualiza los valores máximos y mínimos de:
 *    - Hashrate (KH/s)
 *    - Temperatura del minero
 *    - Cuenta las veces que la CPU pasa de los 70°C
 * 3. Ejecuta una animación de nieve en pantalla si es Navidad (20 de diciembre - 6 de enero).
 * 4. Muestra mensajes motivacionales en la pantalla en ciertos momentos aleatorios.
 * 5. Inicializa los datos del bot de Telegram al inicio y registra la hora de arranque.
 * 6. Envía un mensaje a Telegram cada 2 horas si ya pasó el tiempo mínimo de arranque.
 *
 * La función también controla la variable `ContadorEspecial` y reinicia algunos parámetros
 * cuando se activan los eventos especiales (Navidad, motivación).
 *
 * Si la temperatura de la CPU sobrepasa los 80 grados, la CPU entrará en modo deep sleep
 * durante 10 minutos, minutos en los cuales la CPU bajará la temperatura. Pasados
 * los 10 minutos, el NerdMinerV2 rearrancará solo y comenzará a minar con la CPU Más Fría... ( VIGILAR )
 *
 * @param frame Número de fotograma actual (usado en ciertas animaciones o cálculos).
 */

void analiCadaSegundo(unsigned long frame)
{
  unsigned long epochTime = timeClient.getEpochTime(); // Obtener segundos desde 1970
  time_t epoch = (time_t)epochTime;                    // Convertir a time_t
  struct tm *timeinfo = localtime(&epoch);             // Convertir a una estructura de tiempo local
  int dia = timeinfo->tm_mday;                         // Día del mes (1 a 31)
  int mes = timeinfo->tm_mon + 1;                      // Mes (1 a 12)
  int anio = timeinfo->tm_year + 1900;                 // Año (por defecto es desde 1900)
  int horita = timeinfo->tm_hour;                      // Hora
  int minutitos = timeinfo->tm_min;                    // Minutos
  int segundos = timeinfo->tm_sec;                     // Segundos

  if (startTime > 0 && uncontadormas > 50)
  {
    if (mineria.currentHashRate.toFloat() > maxkh)
    {
      maxkh = mineria.currentHashRate.toFloat(); // Actualiza el máximo de kh/s
    }

    if (mineria.currentHashRate.toFloat() < minkh)
    {
      minkh = mineria.currentHashRate.toFloat(); // Actualiza el mínimo de kh/s
    }

    if (mineria.temp.toInt() > maxtemp)
    {
      maxtemp = mineria.temp.toInt(); // Actualiza el máximo de temperatura
    }

    if (mineria.temp.toInt() < mintemp)
    {
      mintemp = mineria.temp.toInt(); // Actualiza el mínimo de temperatura
    }

    if (ContadorEspecial % 300 == 0 && mineria.temp.toInt() > 70)
    {
      Serial.println("M8AX - Temperatura De CPU, Ha Superado Los 70°C...");
      alertatemp++;
    }

    if (ContadorEspecial % 30 == 0 && mineria.temp.toInt() > 80)
    {
      Serial.println("M8AX - ¡ Temperatura Muy Alta ! 80°C Superados. Entrando En Deep Sleep Por 10 Minutos Para Enfriar La CPU...");
      esp_sleep_enable_timer_wakeup(600e6);
      esp_deep_sleep_start();
    }
  }

  // Felicitar La Navidad O El Año Nuevo

  if (((mes == 12 && dia >= 20) || (mes == 1 && dia <= 6)) && anio != 1970)
  {
    if (minutitos == 30 && ((horita >= 8 && horita <= 15) || (horita >= 19 && horita <= 23) || (horita >= 0 && horita <= 2)) && (horita % 2 == 0))
    {
      if (segundos == 0 && dia % 2 == 0)
      {
        if (mes == 12)
        {
          nevar();
          Serial.println("M8AX - Felicitando La Navidad...");
          ContadorEspecial = 0;
        }
        else if (mes == 1)
        {
          nevar2();
          Serial.println("M8AX - Felicitando El Año Nuevo...");
          ContadorEspecial = 0;
        }
        return;
      }
    }
  }

  rndnumero = esp_random();
  if (rndnumero <= 10031977 && segundos <= 20 && segundos % 2 == 0 && dia % 2 == 0)
  {
    Serial.printf(">>> M8AX-NerdMinerV2 Dando Ánimos Y Esperanza Al Usuario...\n");
    actualizarcalen = 0;
    actualizarc = 0;
    actual = 0;
    actuanot = 0;
    uncontadormas = 51;
    ContadorEspecial = 0;
    correccion = 0;
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(colors[colorIndex]);
    tft.setTextSize(3);
    tft.setCursor(0, 0);
    if (rndnumero % 2 == 0)
    {
      tft.println("La Esperanza Es");
      tft.println("Lo Ultimo Que");
      tft.println("Se Pierde...");
      tft.println("Lo Conseguiremos!");
      tft.println("");
      tft.println("     VAMOS !");
      tft.setTextSize(1);
      tft.println("");
      tft.setTextSize(2);
      tft.setTextColor(TFT_WHITE);
      tft.println("     ... By M8AX ...");
    }
    else
    {
      tft.println("Hope Is The Last");
      tft.println("Thing To Lose, We");
      tft.println("Will Archieve It!");
      tft.println("");
      tft.println("    COME ON !");
      tft.setTextSize(2);
      tft.setTextColor(TFT_WHITE);
      tft.println("");
      tft.println("");
      tft.println("     ... By M8AX ...");
    }
    delay(3000);
  }

  if (startTime == 0)
  {
    BOT_TOKEN = Settings.botTelegram;    // Bot De Telegram
    CHAT_ID = Settings.ChanelIDTelegram; // ID Del Canal De Telegram
    startTime = epochTime;               // Guardar el tiempo de inicio cuando el dispositivo arranca
    diadecambios = dia;
    anterBTC = monedilla.btcPrice.toFloat();
  }

  if (anterBTC <= 0.0)
  {
    anterBTC = monedilla.btcPrice.toFloat();
  }

  if (dia != diadecambios)
  {
    diadecambios = dia;
    float variacion = (anterBTC > 0) ? ((monedilla.btcPrice.toFloat() - anterBTC) / anterBTC) * 100 : 0;
    subebaja = (anterBTC > 0 && monedilla.btcPrice.toFloat() > 0) ? ((variacion >= 0) ? "+" : "-") + String(fabs(variacion), 5) + "%" : ". ERROR .";
    anterBTC = monedilla.btcPrice.toFloat();
  }

  // Si ya ha pasado el tiempo de arranque mínimo (por ejemplo, 10 minutos) y han pasado 2 horas desde el último mensaje
  if (epochTime - startTime >= minStartupTime && epochTime - lastTelegramEpochTime >= interval)
  {
    // Verificar si los datos de Telegram están configurados
    if (BOT_TOKEN != "NO CONFIGURADO" && CHAT_ID != "NO CONFIGURADO")
    {
      recopilaTelegram();                // Envia el mensaje a Telegram
      lastTelegramEpochTime = epochTime; // Actualiza el tiempo de la última ejecución
    }
  }
}

// Función encargada de gestionar la animación de la pantalla actual.
// 1. Incrementa el contador especial (ContadorEspecial) en cada llamada.
// 2. Si el contador es divisible por 5, llama a la función 'analiCadaSegundo' para realizar acciones basadas en el tiempo.
//    Esta función se encarga de verificar el tiempo, felicitar eventos especiales (Navidad/Año Nuevo),
//    mostrar mensajes motivacionales y realizar tareas de Telegram en intervalos específicos de tiempo.
// 3. La animación de la pantalla o las actualizaciones relacionadas con el frame también se manejan dentro de esta función.
//    Sin embargo, el código para las animaciones no se incluye explícitamente en esta función.

void tDisplay_AnimateCurrentScreen(unsigned long frame)
{
  ContadorEspecial++;
  uncontadormas++;
  if (ContadorEspecial % 5 == 0)
  {
    analiCadaSegundo(frame);
  }
}

// Aún sin implementación

void tDisplay_DoLedStuff(unsigned long frame)
{
}

// Definición de la estructura `tDisplayDriver` que encapsula las funciones necesarias para controlar las pantallas y sus interacciones.
// Esta estructura contiene las siguientes funciones:
// - `tDisplay_Init`: Inicializa la pantalla y otros componentes necesarios.
// - `tDisplay_AlternateScreenState`: Permite alternar entre diferentes estados de la pantalla.
// - `tDisplay_AlternateRotation`: Maneja la rotación de la pantalla.
// - `tDisplay_LoadingScreen`: Muestra la pantalla de carga con efectos aleatorios.
// - `tDisplay_SetupScreen`: Muestra la pantalla de configuración.
// - `tDisplayCyclicScreens`: Array de funciones cíclicas que se muestran de manera periódica, permitiendo la transición entre pantallas, como la pantalla del minero, cotización de BTC, hora, etc.
// - `tDisplay_AnimateCurrentScreen`: Actualiza las pantallas de forma cíclica y con animaciones, como la del minero o las cotizaciones.
// - `tDisplay_DoLedStuff`: Función vacía en este momento, reservada para gestionar efectos relacionados con LEDs.
// Además, contiene detalles de la pantalla, como el tamaño y el número total de pantallas definidas en `tDisplayCyclicScreens` (calculado con `SCREENS_ARRAY_SIZE`).
// La estructura `tDisplayDriver` centraliza el control de todas las operaciones relacionadas con la pantalla, permitiendo la ejecución secuencial o cíclica de diferentes pantallas en el dispositivo.

CyclicScreenFunction tDisplayCyclicScreens[] = {tDisplay_MinerScreen, tDisplay_GlobalHashScreen, tDisplay_BTCprice, tDisplay_m8axScreen15, tDisplay_m8axScreen2, datoTextPlano, tDisplay_m8axScreen5, tDisplay_m8axScreen16, tDisplay_ClockScreen, tDisplay_m8axScreen1, tDisplay_m8axScreen7, RelojDeNumeros, tDisplay_m8axScreen9, tDisplay_m8axScreen10, tDisplay_m8axScreen11, tDisplay_m8axScreen18, tDisplay_m8axScreen3, tDisplay_m8axScreen13, tDisplay_m8axScreen14, tDisplay_m8axScreen8, tDisplay_m8axScreen4, tDisplay_m8axScreen6, tDisplay_m8axScreen17, monedaYdado, tDisplay_m8axScreen12, tDisplay_m8axvida};

DisplayDriver tDisplayDriver = {
    tDisplay_Init,
    tDisplay_AlternateScreenState,
    tDisplay_AlternateRotation,
    tDisplay_LoadingScreen,
    tDisplay_SetupScreen,
    tDisplayCyclicScreens,
    tDisplay_AnimateCurrentScreen,
    tDisplay_DoLedStuff,
    SCREENS_ARRAY_SIZE(tDisplayCyclicScreens),
    0,
    WIDTH,
    HEIGHT};
#endif