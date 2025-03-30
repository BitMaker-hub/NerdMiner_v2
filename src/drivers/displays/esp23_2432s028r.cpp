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
 *
 *           Un minero de Bitcoin es un dispositivo o software que realiza cálculos
 *           matemáticos complejos para verificar y validar transacciones en la red.
 *           Los mineros compiten para resolver estos problemas y añadir un bloque
 *           a la cadena. A cambio, reciben bitcoins recién creados como recompensa.
 *
 *
 *
 *                     Tmp. De Programación 2H - 1655 Líneas De Código
 *                     ------------------------------------------------
 *
 ********************************************************************************************/

// Invocando las poderosas librerías que hacen posible esta obra maestra del minado nerd

#include "displayDriver.h"
#if defined ESP32_2432S028R || ESP32_2432S028_2USB
#include <TFT_eSPI.h>
#include <TFT_eTouch.h>
#include "media/images_320_170.h"
#include "media/images_bottom_320_70.h"
#include "media/myFonts.h"
#include "media/Free_Fonts.h"
#include "version.h"
#include "monitor.h"
#include "OpenFontRender.h"
#include <HTTPClient.h>
#include <SPI.h>
#include "rotation.h"
#include <clientntp.h>
#include <time.h>
#include <ctime>
#include <WiFi.h>
#include <urlencode.h>
#include "drivers/storage/nvMemory.h"
#include "drivers/storage/storage.h"

#define WIDTH 130 // 320
#define HEIGHT 170
#define MAX_RESULT_LENGTH 500

extern nvMemory nvMem;
extern monitor_data mMonitor;
extern pool_data pData;
extern DisplayDriver *currentDisplayDriver;
extern bool invertColors;
extern TSettings Settings;
bool hasChangedScreen = true;

OpenFontRender render;
TFT_eSPI tft = TFT_eSPI();                  // Invoke library, pins defined in platformio.ini
TFT_eSprite background = TFT_eSprite(&tft); // Invoke library sprite
SPIClass hSPI(HSPI);
TFT_eTouch<TFT_eSPI> touch(tft, ETOUCH_CS, 0xFF, hSPI);

int colorI = 0, sumatele = 1, colorIndex = 0, maxtemp = 0, mintemp = 1000;
uint16_t colors[] = {TFT_WHITE, TFT_RED, TFT_GREEN, TFT_BLUE, TFT_YELLOW, TFT_CYAN, TFT_MAGENTA, TFT_ORANGE, TFT_GREENYELLOW, TFT_PINK, TFT_LIGHTGREY, TFT_SKYBLUE, TFT_OLIVE, TFT_GOLD, TFT_SILVER};
uint32_t rndnumero;
uint32_t uncontadormas = 0;
uint32_t refresca = 0;
uint32_t cuentita = 0;
unsigned long lastTelegramEpochTime = 0;       // Guarda el tiempo de la última ejecución (en segundos desde Epoch)
unsigned long startTime = 0;                   // Para guardar Epoch de inicio
const unsigned long interval = 60 * 2 * 60;    // 2 horas en segundos (2 horas * 60 minutos * 60 segundos)
const unsigned long minStartupTime = interval; // Segundos para que no envíe mensaje a telegram si esta configurado, nada más arrancar
float maxkh = 0.00, minkh = 1000.00;
char result[MAX_RESULT_LENGTH];
const char *urlsm8ax[] = {
    "YT - https://youtube.com/m8ax",
    "OS - https://opensea.io/es/m8ax",
    "OC - https://oncyber.io/m8ax",
    "FW - https://m8ax.github.io/MvIiIaX-NerdMiner_V2-DeV/",
    "GH - https://github.com/m8ax"};
String BOT_TOKEN;
String CHAT_ID;
clock_data relojete;
mining_data mineria;

void getChipInfo(void)
{
  Serial.print("M8AX - Chip: ");
  Serial.println(ESP.getChipModel());
  Serial.print("M8AX - ChipRevision: ");
  Serial.println(ESP.getChipRevision());
  Serial.print("M8AX - Psram size: ");
  Serial.print(ESP.getPsramSize() / 1024);
  Serial.println("KB");
  Serial.print("M8AX - Flash size: ");
  Serial.print(ESP.getFlashChipSize() / 1024);
  Serial.println("KB");
  Serial.print("M8AX - CPU frequency: ");
  Serial.print(ESP.getCpuFreqMHz());
  Serial.println("MHz");
}

const char *obtenerEstadoTemperatura(int temperatura)
{
  if (temperatura >= 20 && temperatura <= 45)
  {
    return "Temperatura Baja | ";
  }
  else if (temperatura >= 46 && temperatura <= 65)
  {
    return "Temperatura Normal | ";
  }
  else if (temperatura >= 66 && temperatura <= 75)
  {
    return "Temperatura Alta | ";
  }
  else
  {
    return "Temperatura Muy Alta | ";
  }
}

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
    tft.setCursor(76, 82);
    (numeroSaludo % 2 == 0) ? tft.print("HOLA") : tft.print("KAIXO");
  }
  else
  {
    tft.setCursor(60, 82);
    (numeroSaludo % 2 == 0) ? tft.print("HELLO") : tft.print("ALOHA");
  }

  tft.setTextSize(1);
}

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

void cortinas()
{
  tft.fillScreen(TFT_BLACK); // Fondo negro
  cortinas2();
  drawCenteredText("M 8 A X", 50, 100);
  drawCenteredText("NerdMiner V2", 90, 100);
  drawCenteredText("V  10 . 03 . 77", 130, 100);
  delay(500);
  television();
  delay(1000);
  tft.setTextSize(1);
}

void M8AXTicker()
{
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(colors[colorI], TFT_BLACK);
  tft.setTextSize(2);
  drawCenteredText("M v I i I a X", 50, 100);
  drawCenteredText("NerdMiner V2", 990, 100);
  drawCenteredText("V  10 . 03 . 77", 130, 100);
  delay(500);
  television();
  delay(1000);
  tft.setTextSize(1);
}

void nevar3()
{
  tft.fillScreen(TFT_BLACK);      // Fondo negro (puedes cambiarlo)
  const int NUM_COPOS = 500;      // Número de copos de nieve en pantalla
  int x[NUM_COPOS], y[NUM_COPOS]; // Coordenadas de los copos

  // Inicializa copos en posiciones aleatorias
  for (int i = 0; i < NUM_COPOS; i++)
  {
    x[i] = esp_random() % 536;
    y[i] = esp_random() % 240;
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
      if (y[i] > 240)
      { // Si sale de la pantalla, reaparece arriba
        y[i] = 0;
        x[i] = esp_random() % 536;
      }
    }
    int nnumeroAleatorio = esp_random() % 20 + 1;
    delay(nnumeroAleatorio); // Controla la velocidad de la animación
  }
  M8AXTicker();
}

void M8AXTicker2()
{
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(colors[colorI], TFT_BLACK);
  tft.setTextSize(2);
  drawCenteredText("B T C  M I N E R", 50, 100);
  drawCenteredText("NerdMiner V2", 90, 100);
  drawCenteredText("V  10 . 03 . 77", 130, 100);
  delay(500);
  television();
  delay(1000);
  tft.setTextSize(1);
}

void M8AXTicker3()
{
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(colors[colorI], TFT_BLACK);
  tft.setTextSize(2);
  drawCenteredText("M I N E  T E C H", 50, 100);
  drawCenteredText("NerdMiner V2", 90, 100);
  drawCenteredText("V  10 . 03 . 77", 130, 100);
  delay(500);
  television();
  delay(1000);
  tft.setTextSize(1);
}

void M8AXTicker4()
{
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(colors[colorI], TFT_BLACK);
  tft.setTextSize(2);
  drawCenteredText("I M O D   T E C H", 50, 100);
  drawCenteredText("NerdMiner V2", 90, 100);
  drawCenteredText("V  10 . 03 . 77", 130, 100);
  delay(500);
  television();
  delay(1000);
  tft.setTextSize(1);
}

void M8AXTicker5()
{
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(colors[colorI], TFT_BLACK);
  tft.setTextSize(2);
  drawCenteredText("E H D  -  M D D D", 50, 100);
  drawCenteredText("NerdMiner V2", 90, 100);
  drawCenteredText("V  10 . 03 . 77", 130, 100);
  delay(500);
  television();
  delay(1000);
  tft.setTextSize(1);
}

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

void nevar()
{
  tft.fillScreen(TFT_BLACK); // Fondo negro (puedes cambiarlo)

  const int NUM_COPOS = 100;      // Número de copos de nieve en pantalla
  int x[NUM_COPOS], y[NUM_COPOS]; // Coordenadas de los copos
  // Inicializa copos en posiciones aleatorias
  for (int i = 0; i < NUM_COPOS; i++)
  {
    x[i] = (esp_random() % 536); // Genera un número entre 0 y 319
    y[i] = (esp_random() % 240); // Genera un número entre 0 y 169
  }

  unsigned long startTime = millis();

  while (millis() - startTime < 5000)
  {                            // 5 segundos
    tft.fillScreen(TFT_BLACK); // Borra pantalla

    for (int i = 0; i < NUM_COPOS; i++)
    {
      tft.drawPixel(x[i], y[i], TFT_WHITE); // Dibuja copo de nieve
      y[i] += (esp_random() % 4) + 1;       // Baja la posición del copo

      if (y[i] > 240)
      { // Si sale de la pantalla, reaparece arriba
        y[i] = 0;
        x[i] = esp_random() % 536;
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

void nevar2()
{
  tft.fillScreen(TFT_BLACK); // Fondo negro (puedes cambiarlo)

  const int NUM_COPOS = 200;      // Número de copos de nieve en pantalla
  int x[NUM_COPOS], y[NUM_COPOS]; // Coordenadas de los copos

  // Inicializa copos en posiciones aleatorias
  for (int i = 0; i < NUM_COPOS; i++)
  {
    x[i] = esp_random() % 536;
    y[i] = esp_random() % 240;
  }

  unsigned long startTime = millis();

  while (millis() - startTime < 5000)
  {                            // 5 segundos
    tft.fillScreen(TFT_BLACK); // Borra pantalla

    for (int i = 0; i < NUM_COPOS; i++)
    {
      tft.drawPixel(x[i], y[i], TFT_WHITE); // Dibuja copo de nieve
      y[i] += (esp_random() % 5) + 1;       // Baja Posición Del Copo
      if (y[i] > 240)
      { // Si sale de la pantalla, reaparece arriba
        y[i] = 0;
        x[i] = esp_random() % 536;
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
    colorIndex = esp_random() % (sizeof(colors) / sizeof(colors[0]));
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

const char *convertirTiempo(uint32_t segundos)
{
  uint32_t dias, horas, minutos, segs;
  dias = segundos / 86400;
  segundos %= 86400;
  horas = segundos / 3600;
  segundos %= 3600;
  minutos = segundos / 60;
  segs = segundos % 60;
  static char buffer[20];
  snprintf(buffer, sizeof(buffer), "%02lud %02luh %02lum %02lus", (unsigned long)dias, (unsigned long)horas, (unsigned long)minutos, (unsigned long)segs);
  return buffer;
}

uint32_t floatToUint32(float num)
{
  if (num < 0)
    return 0;
  if (num > UINT32_MAX)
    return UINT32_MAX;
  return static_cast<uint32_t>(round(num));
}

float calcularDiferenciaDias()
{
  struct tm compileTime = {0};
  char fechaHoraCompilacion[25];
  snprintf(fechaHoraCompilacion, sizeof(fechaHoraCompilacion), "%s %s", __DATE__, __TIME__);
  strptime(fechaHoraCompilacion, "%b %d %Y %H:%M:%S", &compileTime);
  time_t compiledEpoch = mktime(&compileTime);
  unsigned long epochTime = timeClient.getEpochTime();
  time_t epoch = (time_t)epochTime;
  struct tm *timeinfo = localtime(&epoch);
  time_t currentEpoch = mktime(timeinfo);
  float diferenciaDias = (float)(currentEpoch - compiledEpoch) / (60 * 60 * 24);
  return diferenciaDias;
}

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
  String cadenaEnvio;
  cadenaEnvio.reserve(4000);
  cadenaEnvio = "";
  cadenaEnvio = F("------------------------------------------------------------------------------------------------\n");
  cadenaEnvio += "----------------------- M8AX - NerdMinerV2PG-" + String(u4digits) + " DATOS DE MINERÍA - M8AX ----------------------\n";
  cadenaEnvio += "----------------------------------- " + String(fechaFormateada) + " " + quediase.c_str() + " - " + horaFormateada + " ----------------------------------\n";
  cadenaEnvio += F("------------------------------------------------------------------------------------------------\n");
  quediase.clear();
  quediase.shrink_to_fit();
  if (sumatele <= 3999)
  {
    cadenaEnvio += "Mensaje Número - " + convertirARomanos(sumatele) + " | F.C.FW - " + String(__DATE__) + " A Las " + String(__TIME__) + " | Hace - " + String(convertirTiempo(floatToUint32(calcularDiferenciaDias() * 86400))).c_str() + "\n";
  }
  else
  {
    cadenaEnvio += "Mensaje Número - " + String(sumatele) + " | F.C.FW - " + String(__DATE__) + " A Las " + String(__TIME__) + " | Hace - " + String(convertirTiempo(floatToUint32(calcularDiferenciaDias() * 86400))).c_str() + "\n";
  }
  cadenaEnvio += "Tiempo Minando - " + (mineria.timeMining.substring(0, mineria.timeMining.indexOf(" ")).length() == 1 ? "0" + mineria.timeMining.substring(0, mineria.timeMining.indexOf(" ")) : mineria.timeMining.substring(0, mineria.timeMining.indexOf(" "))) + " Días" + mineria.timeMining.substring(mineria.timeMining.indexOf(" ") + 1) + "\n";
  cadenaEnvio += "HR Actual - " + mineria.currentHashRate + " KH/s ( MAX - " + String(maxkh) + " | MIN - " + String(minkh) + " )\n";
  cadenaEnvio += "Temp. De CPU - " + mineria.temp + "° ( MAX - " + String(maxtemp) + "° | MIN - " + String(mintemp) + "° ) | " + obtenerEstadoTemperatura(mineria.temp.toInt());
  cadenaEnvio += "RAM Libre - " + String(ESP.getFreeHeap()) + " Bytes\n";
  cadenaEnvio += "Plantillas De Bloque - " + mineria.templates + "\n";
  cadenaEnvio += "Shares Enviados A La Pool - " + mineria.completedShares + "\n";
  cadenaEnvio += "Mejor Dificultad Alcanzada - " + mineria.bestDiff + "\n";
  cadenaEnvio += "Cómputo Total - " + mineria.totalKHashes + " KH - ( " + String(atof(mineria.totalKHashes.c_str()) / 1000, 3) + " MH )\n";
  cadenaEnvio += "Altura De Bloque - " + relojete.blockHeight + "\n";
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

int obtenerZonaHoraria()
{
  time_t now = timeClient.getEpochTime();
  struct tm timeinfo;
  localtime_r(&now, &timeinfo);
  return (timeinfo.tm_isdst > 0) ? 1 : 2;
}

void ajustarZonaHoraria()
{
  if (Settings.Timezone == 1 || Settings.Timezone == 2)
  {
    int zonilla = obtenerZonaHoraria();
    if (zonilla == 1)
    {
      if (Settings.Timezone != zonilla)
      {
        Settings.Timezone = 1;
        nvMem.saveConfig(&Settings);
        int offset = Settings.Timezone * 3600;
        timeClient.setTimeOffset(3600 * Settings.Timezone);
        configTime(offset, 0, "pool.ntp.org", "time.nist.gov");
        Serial.println("M8AX - Esperando Sincronización Con NTP...");
        time_t now;
        while ((now = time(nullptr)) < 100000)
        {
          Serial.print(".");
          delay(200);
        }
        Serial.println("M8AX - Hora Sincronizada Correctamente...");
        Serial.println("M8AX - Cambiando TimeZone A Horario De Invierno... Que Actualmente Es UTC +" + String(Settings.Timezone));
      }
    }
    else if (zonilla == 2)
    {
      if (Settings.Timezone != zonilla)
      {
        Settings.Timezone = 2;
        int offset = Settings.Timezone * 3600;
        timeClient.setTimeOffset(3600 * Settings.Timezone);
        configTime(offset, 0, "pool.ntp.org", "time.nist.gov");
        Serial.println("M8AX - Esperando Sincronización Con NTP...");
        time_t now;
        while ((now = time(nullptr)) < 100000)
        {
          Serial.print(".");
          delay(200);
        }
        Serial.println("M8AX - Hora Sincronizada Correctamente...");
        nvMem.saveConfig(&Settings);
        Serial.println("M8AX - Cambiando TimeZone A Horario De Verano... Que Actualmente Es UTC +" + String(Settings.Timezone));
      }
    }
  }
}

void esp32_2432S028R_Init(void)
{
  // getChipInfo();
  tft.init();
  if (nvMem.loadConfig(&Settings))
  {
    // Serial.print("Invert Colors: ");
    // Serial.println(Settings.invertColors);
    invertColors = Settings.invertColors;
  }
  tft.invertDisplay(invertColors);
  tft.setRotation(1);
  tft.setSwapBytes(true); // Swap the colour byte order when rendering
  if (invertColors)
  {
    tft.writecommand(ILI9341_GAMMASET);
    tft.writedata(2);
    delay(120);
    tft.writecommand(ILI9341_GAMMASET); // Gamma curve selected
    tft.writedata(1);
  }
  hSPI.begin(TOUCH_CLK, TOUCH_MISO, TOUCH_MOSI, ETOUCH_CS);
  touch.init();

  TFT_eTouchBase::Calibation calibation = {233, 3785, 3731, 120, 2};
  touch.setCalibration(calibation);

  // Configuring screen backlight brightness using ledcontrol channel 0.
  // Using 5000Hz in 8bit resolution, which gives 0-255 possible duty cycle setting.
  ledcSetup(0, 5000, 8);
  ledcAttachPin(TFT_BL, 0);
  ledcWrite(0, Settings.Brightness);

  // background.createSprite(WIDTH, HEIGHT); // Background Sprite
  // background.setSwapBytes(true);
  // render.setDrawer(background);  // Link drawing object to background instance (so font will be rendered on background)
  // render.setLineSpaceRatio(0.9); // Espaciado entre texto
  // Load the font and check it can be read OK
  // if (render.loadFont(NotoSans_Bold, sizeof(NotoSans_Bold)))
  if (render.loadFont(DigitalNumbers, sizeof(DigitalNumbers)))
  {
    Serial.println("M8AX - Inicialización Errónea");
    return;
  }

  pinMode(LED_PIN, OUTPUT);
  pinMode(LED_PIN_B, OUTPUT);
  pinMode(LED_PIN_G, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  digitalWrite(LED_PIN_B, HIGH);
  digitalWrite(LED_PIN_G, HIGH);
  pData.bestDifficulty = "0";
  pData.workersHash = "0";
  pData.workersCount = 0;
  // Serial.println("=========== Fim Display ==============") ;
}

void esp32_2432S028R_AlternateScreenState(void)
{
  Serial.println("M8AX - Cambiando Estado De Pantalla");
  int screen_state_duty = ledcRead(0);
  // Switching the duty cycle for the ledc channel, where the TFT_BL pin is attached.
  if (screen_state_duty > 0)
  {
    ledcWrite(0, 0);
  }
  else
  {
    ledcWrite(0, Settings.Brightness);
  }
}

void esp32_2432S028R_AlternateRotation(void)
{
  tft.setRotation(flipRotation(tft.getRotation()));
  hasChangedScreen = true;
}

bool bottomScreenBlue = true;

void printheap()
{
  Serial.print("M8AX - $$ Free Heap:");
  Serial.println(ESP.getFreeHeap());
  // Serial.printf("### stack WMark usage: %d\n", uxTaskGetStackHighWaterMark(NULL));
}

bool createBackgroundSprite(int16_t wdt, int16_t hgt)
{                                    // Set the background and link the render, used multiple times to fit in heap
  background.createSprite(wdt, hgt); // Background Sprite
  // printheap();
  if (background.created())
  {
    background.setColorDepth(16);
    background.setSwapBytes(true);
    render.setDrawer(background); // Link drawing object to background instance (so font will be rendered on background)
    render.setLineSpaceRatio(0.9);
  }
  else
  {
    Serial.println("M8AX - #### Sprite Error ####");
    Serial.printf("M8AX - Size w:%d h:%d \n", wdt, hgt);
    printheap();
  }
  return background.created();
}

extern unsigned long mPoolUpdate;

void printPoolData()
{
  if ((hasChangedScreen) || (mPoolUpdate == 0) || (millis() - mPoolUpdate > UPDATE_POOL_min * 60 * 1000))
  {
    if (Settings.PoolAddress != "tn.vkbit.com")
    {
      pData = getPoolData();
      background.createSprite(320, 50); // Background Sprite
      if (!background.created())
      {
        Serial.println("M8AX - ###### POOL SPRITE ERROR ######");
        // Serial.printf("Pool data W:%d H:%s D:%s\n", pData.workersCount, pData.workersHash, pData.bestDifficulty);
        printheap();
      }
      background.setSwapBytes(true);
      if (bottomScreenBlue)
      {
        background.pushImage(0, -20, 320, 70, bottonPoolScreen);
        tft.pushImage(0, 170, 320, 20, bottonPoolScreen);
      }
      else
      {
        background.pushImage(0, -20, 320, 70, bottonPoolScreen_g);
        tft.pushImage(0, 170, 320, 20, bottonPoolScreen_g);
      }

      render.setDrawer(background); // Link drawing object to background instance (so font will be rendered on background)
      render.setLineSpaceRatio(1);

      render.setFontSize(24);
      render.cdrawString(String(pData.workersCount).c_str(), 157, 16, TFT_BLACK);
      render.setFontSize(18);
      render.setAlignment(Align::BottomRight);
      render.cdrawString(pData.workersHash.c_str(), 265, 14, TFT_BLACK);
      render.setAlignment(Align::BottomLeft);
      render.cdrawString(pData.bestDifficulty.c_str(), 54, 14, TFT_BLACK);
      background.pushSprite(0, 190);
      background.deleteSprite();
    }
    else
    {
      pData.bestDifficulty = "TESTNET";
      pData.workersHash = "TESTNET";
      pData.workersCount = 1;
      tft.fillRect(0, 170, 320, 70, TFT_DARKGREEN);
      background.createSprite(320, 40); // Background Sprite
      background.fillSprite(TFT_DARKGREEN);
      if (!background.created())
      {
        Serial.println("M8AX - ###### POOL SPRITE ERROR ######");
        // Serial.printf("Pool data W:%d H:%s D:%s\n", pData.workersCount, pData.workersHash, pData.bestDifficulty);
        printheap();
      }
      background.setFreeFont(FF24);
      background.setTextDatum(TL_DATUM);
      background.setTextSize(1);
      background.setTextColor(TFT_WHITE, TFT_DARKGREEN);
      background.drawString("TESTNET", 50, 0, GFXFF);
      background.pushSprite(0, 185);
      mPoolUpdate = millis();
      Serial.println("Testnet");
      background.deleteSprite();
    }
  }
}

void esp32_2432S028R_MinerScreen(unsigned long mElapsed)
{
  mining_data data = getMiningData(mElapsed);
  mineria = getMiningData(mElapsed);
  relojete = getClockData(mElapsed);
  printPoolData();
  if (hasChangedScreen)
    tft.pushImage(0, 0, initWidth, initHeight, MinerScreen);
  hasChangedScreen = false;
  int wdtOffset = 190;
  // Recreate sprite to the right side of the screen
  createBackgroundSprite(WIDTH - 5, HEIGHT - 7);
  // Print background screen
  background.pushImage(-190, 0, MinerWidth, MinerHeight, MinerScreen);
  // Total hashes
  render.setFontSize(18);
  render.rdrawString(data.totalMHashes.c_str(), 268 - wdtOffset, 138, TFT_BLACK);
  // Block templates
  render.setFontSize(18);
  render.setAlignment(Align::TopLeft);
  render.drawString(data.templates.c_str(), 189 - wdtOffset, 20, 0xDEDB);
  // Best diff
  render.drawString(data.bestDiff.c_str(), 189 - wdtOffset, 48, 0xDEDB);
  // 32Bit shares
  render.setFontSize(18);
  render.drawString(data.completedShares.c_str(), 189 - wdtOffset, 76, 0xDEDB);
  // Hores
  render.setFontSize(14);
  render.rdrawString(data.timeMining.c_str(), 315 - wdtOffset, 104, 0xDEDB);
  // Valid Blocks
  render.setFontSize(24);
  render.setAlignment(Align::TopCenter);
  render.drawString(data.valids.c_str(), 287 - wdtOffset, 56, 0xDEDB);
  // Print Temp
  render.setFontSize(10);
  render.rdrawString(data.temp.c_str(), 239 - wdtOffset, 1, TFT_BLACK);
  render.setFontSize(4);
  render.rdrawString(String(0).c_str(), 244 - wdtOffset, 3, TFT_BLACK);
  // Print Hour
  render.setFontSize(10);
  render.rdrawString(data.currentTime.c_str(), 286 - wdtOffset, 1, TFT_BLACK);
  // Push prepared background to screen
  background.pushSprite(190, 0);
  // Delete sprite to free the memory heap
  background.deleteSprite();
  // printheap();
  // Serial.println("=========== Mining Display ==============") ;
  // Create background sprite to print data at once
  createBackgroundSprite(WIDTH - 7, HEIGHT - 100); // initHeight); //Background Sprite
  // Print background screen
  background.pushImage(0, -90, MinerWidth, MinerHeight, MinerScreen);
  // Hashrate
  render.setFontSize(30);
  render.setCursor(24, 121);
  render.setFontColor(TFT_BLACK);
  render.rdrawString(data.currentHashRate.c_str(), 121, 118 - 90, TFT_BLACK);
  // Push prepared background to screen
  background.pushSprite(0, 90);
  // Delete sprite to free the memory heap
  background.deleteSprite();
  Serial.printf("M8AX - >>> Completados %s Share(s), %s Khashes, Prom. Hashrate %s KH/s %s°\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str(), data.temp.c_str());
  cuentita++;
  if (cuentita == 15)
  {
    ajustarZonaHoraria();
  }
#ifdef DEBUG_MEMORY
  // Print heap
  printheap();
#endif
}

void esp32_2432S028R_ClockScreen(unsigned long mElapsed)
{
  if (hasChangedScreen)
    tft.pushImage(0, 0, minerClockWidth, minerClockHeight, minerClockScreen);
  printPoolData();
  hasChangedScreen = false;
  clock_data data = getClockData(mElapsed);
  mining_data data2 = getMiningData(mElapsed);
  mineria = getMiningData(mElapsed);
  relojete = getClockData(mElapsed);
  // Create background sprite to print data at once
  createBackgroundSprite(270, 36);
  // Print background screen
  background.pushImage(0, -130, minerClockWidth, minerClockHeight, minerClockScreen);
  // Hashrate
  render.setFontSize(22);
  render.setFontColor(TFT_BLACK);
  render.rdrawString(data.currentHashRate.c_str(), 95, 0, TFT_BLACK);
  // Print BlockHeight
  render.setFontSize(18);
  render.rdrawString(data.blockHeight.c_str(), 254, 9, TFT_BLACK);
  // Push prepared background to screen
  background.pushSprite(0, 130);
  // Delete sprite to free the memory heap
  background.deleteSprite();
  createBackgroundSprite(169, 105);
  // Print background screen
  background.pushImage(-130, -3, minerClockWidth, minerClockHeight, minerClockScreen);
  // Print BTC Price
  background.setFreeFont(FSSB9);
  background.setTextSize(1);
  background.setTextDatum(TL_DATUM);
  background.setTextColor(TFT_BLACK);
  background.drawString(data.btcPrice.c_str(), 202 - 130, 0, GFXFF);
  // Print Hour
  background.setFreeFont(FF23);
  background.setTextSize(2);
  background.setTextColor(TFT_WHITE);
  background.drawString(data.currentTime.c_str(), 0, 50, GFXFF);
  // Push prepared background to screen
  background.pushSprite(130, 3);
  // Delete sprite to free the memory heap
  background.deleteSprite();
  Serial.printf("M8AX - >>> Completados %s Share(s), %s Khashes, Prom. Hashrate %s KH/s %s°\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str(), data2.temp.c_str());
  cuentita++;
  if (cuentita == 15)
  {
    ajustarZonaHoraria();
  }
#ifdef DEBUG_MEMORY
  // Print heap
  printheap();
#endif
}

void esp32_2432S028R_m8axScreen3(unsigned long mElapsed)
{
  clock_data dataa = getClockData(mElapsed);
  mining_data data = getMiningData(mElapsed);
  mineria = getMiningData(mElapsed);
  relojete = getClockData(mElapsed);
  int dia = dataa.currentDate.substring(0, 2).toInt();
  int mes = dataa.currentDate.substring(3, 5).toInt();
  int anio = dataa.currentDate.substring(6, 10).toInt();
  int num1 = dataa.currentTime.charAt(0) - '0'; // Primer dígito de la hora
  int num2 = dataa.currentTime.charAt(1) - '0'; // Segundo dígito de la hora
  int num3 = dataa.currentTime.charAt(3) - '0'; // Primer dígito de los minutos
  int num4 = dataa.currentTime.charAt(4) - '0'; // Segundo dígito de los minutos
  if (hasChangedScreen)
    tft.pushImage(0, 0, initWidth, initHeight, ImagenM8AX);
  printPoolData();
  refresca++;
  if (refresca > 4)
  {
    tft.pushImage(0, 0, initWidth, initHeight, ImagenM8AX);
    refresca = 0;
  }
  hasChangedScreen = false;
  int wdtOffset = 190;
  // Recreate sprite to the right side of the screen
  createBackgroundSprite(WIDTH - 5, HEIGHT - 7);
  // Print background screen
  background.pushImage(-190, 0, ImagenM8AXWidth, ImagenM8AXHeight, ImagenM8AX);
  mostrarCalendario(dia, mes, anio, num1, num2, num3, num4);
  Serial.printf("M8AX - >>> Completados %s Share(s), %s Khashes, Prom. Hashrate %s KH/s %s°\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str(), data.temp.c_str());
  cuentita++;
  if (cuentita == 15)
  {
    ajustarZonaHoraria();
  }
#ifdef DEBUG_MEMORY
  // Print heap
  printheap();
#endif
}

void esp32_2432S028R_BTCprice(unsigned long mElapsed)
{
  if (hasChangedScreen)
    tft.pushImage(0, 0, priceScreenWidth, priceScreenHeight, priceScreen);
  printPoolData();
  hasChangedScreen = false;
  clock_data data = getClockData(mElapsed);
  mining_data dataa = getMiningData(mElapsed);
  mineria = getMiningData(mElapsed);
  relojete = getClockData(mElapsed);
  // Create background sprite to print data at once
  createBackgroundSprite(270, 36);
  // Print background screen
  background.pushImage(0, -130, priceScreenWidth, priceScreenHeight, priceScreen);
  // Hashrate
  render.setFontSize(23);
  render.setFontColor(TFT_BLACK);
  render.rdrawString(data.currentHashRate.c_str(), 95, 0, TFT_BLACK);
  // Print BlockHeight
  render.setFontSize(18);
  render.rdrawString(data.blockHeight.c_str(), 254, 9, TFT_WHITE);
  // Push prepared background to screen
  background.pushSprite(0, 130);
  // Delete sprite to free the memory heap
  background.deleteSprite();
  createBackgroundSprite(169, 105);
  // Print background screen
  background.pushImage(-130, -3, priceScreenWidth, priceScreenHeight, priceScreen);
  // Print Hour
  background.setFreeFont(FSSB9);
  background.setTextSize(1);
  background.setTextDatum(TL_DATUM);
  background.setTextColor(TFT_BLACK);
  background.drawString(data.currentTime.c_str(), 202 - 130, 0, GFXFF);
  // Print BTC Price
  background.setFreeFont(FF24);
  background.setTextDatum(TL_DATUM);
  background.setTextSize(1);
  background.setTextColor(TFT_WHITE);
  background.drawString(data.btcPrice.c_str(), 0, 50, GFXFF);
  // Push prepared background to screen
  background.pushSprite(130, 3);
  // Delete sprite to free the memory heap
  background.deleteSprite();
  Serial.printf("M8AX - >>> Completados %s Share(s), %s Khashes, Prom. Hashrate %s KH/s %s°\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str(), dataa.temp.c_str());
  cuentita++;
  if (cuentita == 15)
  {
    ajustarZonaHoraria();
  }
#ifdef DEBUG_MEMORY
  // Print heap
  printheap();
#endif
}

void esp32_2432S028R_LoadingScreen(void)
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
  tft.pushImage(0, 33, initWidth, initHeight, initScreen);
  tft.setTextColor(TFT_BLACK);
  tft.drawString(CURRENT_VERSION, 24, 184, FONT2);
  // delay(2000);
  // tft.fillScreen(TFT_BLACK);
  // tft.pushImage(0, 0, initWidth, initHeight, MinerScreen);
}

void esp32_2432S028R_SetupScreen(void)
{
  tft.fillScreen(TFT_BLACK);
  tft.pushImage(0, 33, setupModeWidth, setupModeHeight, setupModeScreen);
}

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

  if (uncontadormas % 30 == 0 && mineria.temp.toInt() > 80)
  {
    Serial.println("M8AX - ¡ Temperatura Muy Alta ! 80°C Superados. Entrando En Deep Sleep Por 10 Minutos Para Enfriar La CPU...");
    esp_sleep_enable_timer_wakeup(600e6);
    esp_deep_sleep_start();
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
          uncontadormas = 0;
        }
        else if (mes == 1)
        {
          nevar2();
          Serial.println("M8AX - Felicitando El Año Nuevo...");
          uncontadormas = 0;
        }
        return;
      }
    }
  }

  if (startTime == 0)
  {
    BOT_TOKEN = Settings.botTelegram;    // Bot De Telegram
    CHAT_ID = Settings.ChanelIDTelegram; // ID Del Canal De Telegram
    startTime = epochTime;               // Guardar el tiempo de inicio cuando el dispositivo arranca
  }

  if (startTime > 0 && uncontadormas > 50)
  {
    float currentHashRate = mineria.currentHashRate.toFloat();
    int currentTemp = mineria.temp.toInt();

    if (currentHashRate > maxkh)
    {
      maxkh = currentHashRate; // Actualiza el máximo de kh/s
    }

    if (currentHashRate < minkh)
    {
      minkh = currentHashRate; // Actualiza el mínimo de kh/s
    }

    if (currentTemp > maxtemp)
    {
      maxtemp = currentTemp; // Actualiza el máximo de temperatura
    }

    if (currentTemp < mintemp)
    {
      mintemp = currentTemp; // Actualiza el mínimo de temperatura
    }
  }

  // Si ya ha pasado el tiempo de arranque mínimo (por ejemplo, 10 minutos) y han pasado 2 horas desde el último mensaje de Telegram
  if (epochTime - startTime >= minStartupTime && epochTime - lastTelegramEpochTime >= interval)
  {
    // Ajustar la zona horaria si es necesario
    ajustarZonaHoraria();
    // Verificar si los datos de Telegram están configurados
    if (BOT_TOKEN != "NO CONFIGURADO" && CHAT_ID != "NO CONFIGURADO")
    {
      recopilaTelegram();                // Envia el mensaje a Telegram
      lastTelegramEpochTime = epochTime; // Actualiza el tiempo de la última ejecución
    }
  }
}

void esp32_2432S028R_AnimateCurrentScreen(unsigned long frame)
{
  uncontadormas++;
  if (uncontadormas % 5 == 0)
  {
    analiCadaSegundo(frame);
    if (uncontadormas > 1000000)
      uncontadormas = 50;
  }
}

// Variables para controlar el parpadeo con millis()

unsigned long previousMillis = 0;
unsigned long previousTouchMillis = 0;
char currentScreen = 0;

void esp32_2432S028R_DoLedStuff(unsigned long frame)
{
  unsigned long currentMillis = millis();
  // / Check the touch coordinates 110x185 210x240
  if (currentMillis - previousTouchMillis >= 500)
  {
    int16_t t_x, t_y; // To store the touch coordinates
    bool pressed = touch.getXY(t_x, t_y);
    if (pressed)
    {
      if (((t_x > 109) && (t_x < 211)) && ((t_y > 185) && (t_y < 241)))
      {
        bottomScreenBlue ^= true;
        hasChangedScreen = true;
      }
      else if ((t_x > 235) && ((t_y > 0) && (t_y < 16)))
      {
        // Touching the top right corner of the screen, roughly in the gray status label.
        // Disabling the screen backlight.
        esp32_2432S028R_AlternateScreenState();
      }
      else if (t_x > 160)
      {
        // next screen
        // Serial.printf("Next screen touch( x:%d y:%d )\n", t_x, t_y);
        currentDisplayDriver->current_cyclic_screen = (currentDisplayDriver->current_cyclic_screen + 1) % currentDisplayDriver->num_cyclic_screens;
      }
      else if (t_x < 160)
      {
        // Previus screen
        // Serial.printf("Previus screen touch( x:%d y:%d )\n", t_x, t_y);
        /* Serial.println(currentDisplayDriver->current_cyclic_screen); */
        currentDisplayDriver->current_cyclic_screen = currentDisplayDriver->current_cyclic_screen - 1;
        if (currentDisplayDriver->current_cyclic_screen < 0)
          currentDisplayDriver->current_cyclic_screen = currentDisplayDriver->num_cyclic_screens - 1;
      }
    }
    previousTouchMillis = currentMillis;
  }
  if (currentScreen != currentDisplayDriver->current_cyclic_screen)
    hasChangedScreen ^= true;
  currentScreen = currentDisplayDriver->current_cyclic_screen;
  switch (mMonitor.NerdStatus)
  {
  case NM_waitingConfig:
    digitalWrite(LED_PIN, LOW); // LED encendido de forma continua
    break;
  case NM_Connecting:
    if (currentMillis - previousMillis >= 500)
    { // 0.5sec blink
      previousMillis = currentMillis;
      // Serial.print("C");
      digitalWrite(LED_PIN, HIGH);
      digitalWrite(LED_PIN_B, !digitalRead(LED_PIN)); // Cambia el estado del LED
    }
    break;
  case NM_hashing:
    if (currentMillis - previousMillis >= 500)
    { // 0.1sec blink
      // Serial.print("h");
      previousMillis = currentMillis;
      digitalWrite(LED_PIN_B, HIGH);
      digitalWrite(LED_PIN, !digitalRead(LED_PIN)); // Cambia el estado del LED
    }
    break;
  }
}

CyclicScreenFunction esp32_2432S028RCyclicScreens[] = {esp32_2432S028R_MinerScreen, esp32_2432S028R_ClockScreen, esp32_2432S028R_m8axScreen3, esp32_2432S028R_BTCprice};

DisplayDriver esp32_2432S028RDriver = {
    esp32_2432S028R_Init,
    esp32_2432S028R_AlternateScreenState,
    esp32_2432S028R_AlternateRotation,
    esp32_2432S028R_LoadingScreen,
    esp32_2432S028R_SetupScreen,
    esp32_2432S028RCyclicScreens,
    esp32_2432S028R_AnimateCurrentScreen,
    esp32_2432S028R_DoLedStuff,
    SCREENS_ARRAY_SIZE(esp32_2432S028RCyclicScreens),
    0,
    WIDTH,
    HEIGHT};
#endif

/***********************************************************************************************************************************
 *
 *                        ===============================================================================
 *
 *                                                  F   I   N          D   E
 *
 *                                                P   R   O   G   R   A   M   A
 *
 *                        ===============================================================================
 *
 *                                       ¡   H   A   S   T   A          O   T   R   A   !
 *
 *                        ===============================================================================
 *                                         ___  ___   _     _   _   _   _       ___  __    __
 *                                        /   |/   | | |   / / | | | | | |     /   | \ \  / /
 *                                       / /|   /| | | |  / /  | | | | | |    / /| |  \ \/ /
 *                                      / / |__/ | | | | / /   | | | | | |   / / | |   )  (
 *                                     / /       | | | |/ /    | | | | | |  / / -| |  / /\ \
 *                                    /_/        |_| |___/     |_| |_| |_| /_/   |_| /_/  \_\
 *
 *                        ===============================================================================
 *
 ***********************************************************************************************************************************/