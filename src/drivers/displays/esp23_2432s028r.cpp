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
 *                     Tmp. De Programación 3H - 3440 Líneas De Código
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
#include "moonPhase.h"
#include <ArduinoJson.h>
#include <SPI.h>
#include <qrcode.h>
#include "rotation.h"
#include <clientntp.h>
#include <WiFiClient.h>
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

int colorI = 0, colorIndex = 0, maxtemp = 0, mintemp = 1000;
int limite = 0, zonilla, solounavez = 0, solouna = 0, sumatele = 1;
uint16_t colors[] = {TFT_WHITE, TFT_RED, TFT_GREEN, TFT_BLUE, TFT_YELLOW, TFT_CYAN, TFT_MAGENTA, TFT_ORANGE, TFT_GREENYELLOW, TFT_PINK, TFT_LIGHTGREY, TFT_SKYBLUE, TFT_OLIVE, TFT_GOLD, TFT_SILVER};
uint32_t rndnumero, uncontadormas = 0;
uint32_t refresca = 0, cuentita = 0, numnotis = 0, numfrases = 0;
unsigned long lastTelegramEpochTime = 0;       // Guarda el tiempo de la última ejecución (en segundos desde Epoch)
unsigned long startTime = 0;                   // Para guardar Epoch de inicio
const unsigned long interval = 60 * 2 * 60;    // 2 horas en segundos (2 horas * 60 minutos * 60 segundos)
const unsigned long minStartupTime = interval; // Segundos para que no envíe mensaje a telegram si esta configurado, nada más arrancar
float maxkh = 0.00, minkh = 1000.00;
char result[MAX_RESULT_LENGTH];
const char *serverName = "https://favqs.com/api/qotd";
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
std::pair<String, String> Tresultado;
String BOT_TOKEN;
String CHAT_ID;
String cadenanoti = "";
mining_data mineria;
moonPhase mymoonPhase;

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

String capitalizar(String palabra)
{
  if (palabra.length() > 0)
  {
    palabra[0] = toupper(palabra[0]);
  }
  return palabra;
}

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

void displayQuote(String quote)
{
  tft.setTextColor(TFT_WHITE); // Establecer color de texto
  // Configuración de la fuente
  int numCaracteres = quote.length();
  if (numCaracteres <= 200)
  {
    tft.setTextSize(2);
  }
  else
  {
    tft.setTextSize(1);
  }
  tft.setCursor(0, 28);
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  tft.setTextColor(colors[colorI]);
  tft.drawLine(0, 20, 320, 20, colors[colorI]); // Dibujar línea de (0, y) a (320, y)
  tft.print(quitarAcentos(quote));
  quote = "";
  quote.reserve(0);
  taskYIELD();
}

String getQuote()
{
  HTTPClient http;
  http.begin(serverName);
  http.addHeader("Content-Type", "application/json");
  String quote = "ERROR AL OBTENER LA CITA... Mi Lema Es - ( ... Por Muchas Vueltas Que Demos, Siempre Tendremos El Culo Atras ... )\n\n- M8AX"; // Valor por defecto en caso de error
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
      payload = "";
      payload.reserve(0);

      // Extraer la cita y el autor
      String quoteText = doc["quote"]["body"].as<String>();
      String author = doc["quote"]["author"].as<String>();

      quote = "\"" + quoteText + "\"\n\n- " + author;
      Serial.println("M8AX - Frase Número - " + String(numfrases) + " - " + quote + " - " + String(quote.length()) + " Caracteres");
      quoteText = "";
      author = "";
      doc.clear();
      doc.shrinkToFit();
      taskYIELD();
      quoteText.reserve(0);
      author.reserve(0);
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

double moonPhase::_fhour(const struct tm &timeinfo)
{
  return timeinfo.tm_hour + map((timeinfo.tm_min * 60) + timeinfo.tm_sec, 0, 3600, 0.0, 1.0);
}

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
    payload = "";
    payload.reserve(0);
    tft.setCursor(1, 26);
    tft.setTextSize(1);
    colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
    tft.setTextColor(colors[colorI]);
    tft.print(quitarAcentos(cadenanoti));
    cadenanoti = "";
    cadenanoti.reserve(0);

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

std::pair<String, String> obtenerCiudadYTemperatura(const String &ip)
{
  String ciudad = "ERROR";
  String temperatura = "ERROR";
  String latitud = "";
  String longitud = "";
  HTTPClient http;
  String urlGeo = "http://ip-api.com/json/" + ip + "?fields=city,lat,lon";
  http.begin(urlGeo);
  int httpCode = http.GET();
  if (httpCode > 0)
  {
    String payload = http.getString();
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, payload);
    payload = "";
    payload.reserve(0);
    if (error)
    {
      Serial.println("M8AX - Error Al Parsear JSON De Geolocalización");
      return std::make_pair(ciudad, temperatura);
    }
    ciudad = doc["city"].as<String>();
    latitud = String(doc["lat"].as<float>(), 6);
    longitud = String(doc["lon"].as<float>(), 6);
    Serial.println("M8AX - Obtenida Ciudad");
  }
  else
  {
    Serial.println("M8AX - Error Al Obtener Datos De Geolocalización");
    return std::make_pair(ciudad, temperatura);
  }
  http.end();
  if (latitud != "" && longitud != "")
  {
    WiFiClientSecure client;
    client.setInsecure();
    String urlTemp = "https://wttr.in/" + latitud + "," + longitud + "?format=%t";
    http.begin(client, urlTemp);
    httpCode = http.GET();
    urlTemp.reserve(0);
    if (httpCode > 0)
    {
      temperatura = http.getString();
      Serial.println("M8AX - Obtenida Temperatura");
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
  return std::make_pair(ciudad, temperatura);
}

void obtenerLocYTemp()
{
  Tresultado.first.clear();
  Tresultado.second.clear();
  Tresultado.first.reserve(0);
  Tresultado.second.reserve(0);
  Tresultado = obtenerCiudadYTemperatura(getPublicIP());
}

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

const char *evaluarRSSI(const String &rssiStr)
{
  int rssi = rssiStr.toInt();
  if (rssi >= -50)
  {
    return "******";
  }
  else if (rssi >= -60)
  {
    return "*****";
  }
  else if (rssi >= -70)
  {
    return "****";
  }
  else if (rssi >= -80)
  {
    return "***";
  }
  else if (rssi >= -90)
  {
    return "**";
  }
  else
  {
    return "*";
  }
}

void relojAnalogicoM8AX(int hours, int minutes, int seconds)
{
  // Definir área específica para el reloj (parte superior)
  const int clockWidth = 320;
  const int clockHeight = 170;
  const int clockAreaX = (tft.width() - clockWidth) / 2; // Centrado horizontal
  const int clockAreaY = 0;                              // Parte superior

  // Calcular centro y radio del reloj dentro del área designada
  const int centerX = clockAreaX + clockWidth / 2;
  const int centerY = clockAreaY + clockHeight / 2;
  const int clockRadius = min(clockWidth, clockHeight) * 0.45; // 45% del área

  // Función interna para dibujar manecillas
  auto drawHand = [&](float angle, int length, uint16_t color, int width)
  {
    int endX = centerX + length * cos(angle);
    int endY = centerY + length * sin(angle);

    if (width > 1)
    {
      float perpAngle = angle + PI / 2;
      float dx = (width / 2) * cos(perpAngle);
      float dy = (width / 2) * sin(perpAngle);

      tft.fillTriangle(
          centerX + dx, centerY + dy,
          centerX - dx, centerY - dy,
          endX - dx, endY - dy,
          color);
      tft.fillTriangle(
          centerX + dx, centerY + dy,
          endX - dx, endY - dy,
          endX + dx, endY + dy,
          color);
    }
    else
    {
      tft.drawLine(centerX, centerY, endX, endY, color);
    }
  };

  // Limpiar solo el área del reloj
  tft.fillRect(clockAreaX, clockAreaY, clockWidth, clockHeight, TFT_BLACK);

  // Dibujar círculos del reloj
  tft.drawCircle(centerX, centerY, clockRadius, TFT_WHITE);
  tft.drawCircle(centerX, centerY, clockRadius * 0.4, colors[colorIndex]);

  // Dibujar marcas de horas (12)
  for (int i = 0; i < 12; i++)
  {
    float angle = i * PI / 6;
    int markLength = (i % 3 == 0) ? 12 : 8; // Marcas más largas cada 3 horas
    tft.drawLine(
        centerX + (clockRadius - markLength) * cos(angle),
        centerY + (clockRadius - markLength) * sin(angle),
        centerX + clockRadius * cos(angle),
        centerY + clockRadius * sin(angle),
        TFT_WHITE);
  }

  // Dibujar marcas de minutos (60)
  for (int i = 0; i < 60; i++)
  {
    if (i % 5 != 0)
    { // Saltar donde ya hay marcas de hora
      float angle = i * PI / 30;
      tft.drawPixel(
          centerX + (clockRadius - 5) * cos(angle),
          centerY + (clockRadius - 5) * sin(angle),
          TFT_WHITE);
    }
  }

  // Dibujar marcas de 24 horas
  for (int i = 0; i < 24; i++)
  {
    float angle = i * PI / 12;
    if (i % 6 == 0)
    { // Marcas más visibles cada 6 horas
      tft.drawLine(
          centerX + (clockRadius * 0.4 - 6) * cos(angle),
          centerY + (clockRadius * 0.4 - 6) * sin(angle),
          centerX + clockRadius * 0.4 * cos(angle),
          centerY + clockRadius * 0.4 * sin(angle),
          TFT_CYAN);
    }
    else
    {
      tft.drawPixel(
          centerX + (clockRadius * 0.4 - 4) * cos(angle),
          centerY + (clockRadius * 0.4 - 4) * sin(angle),
          TFT_CYAN);
    }
  }

  // Calcular ángulos con movimiento suave
  float hourAngle = (hours % 12 + minutes / 60.0) * PI / 6 - PI / 2;
  float minuteAngle = (minutes + seconds / 60.0) * PI / 30 - PI / 2;
  float secondAngle = seconds * PI / 30 - PI / 2;
  float hour24Angle = hours * PI / 12 - PI / 2;

  // Dibujar manecillas
  drawHand(hour24Angle, clockRadius * 0.35, TFT_CYAN, 3); // Manecilla 24h azul
  drawHand(hourAngle, clockRadius * 0.5, TFT_WHITE, 5);   // Hora blanca
  drawHand(minuteAngle, clockRadius * 0.7, TFT_WHITE, 3); // Minuto blanco
  drawHand(secondAngle, clockRadius * 0.85, TFT_RED, 1);  // Segundo rojo

  // Dibujar centro del reloj
  tft.fillCircle(centerX, centerY, 5, TFT_RED);
}

const char *obtenerEstadoTemperatura(int temperatura)
{
  if (temperatura >= 20 && temperatura <= 45)
  {
    return "Temperatura Baja";
  }
  else if (temperatura >= 46 && temperatura <= 65)
  {
    return "Temperatura Normal";
  }
  else if (temperatura >= 66 && temperatura <= 75)
  {
    return "Temperatura Alta";
  }
  else
  {
    return "Temperatura Muy Alta";
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
    tft.setCursor(66, 100);
    (numeroSaludo % 2 == 0) ? tft.print("HOLA") : tft.print("EYEY");
  }
  else
  {
    tft.setCursor(55, 100);
    (numeroSaludo % 2 == 0) ? tft.print("HELLO") : tft.print("KAIXO");
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
  tft.setTextDatum(MC_DATUM);                // Centra el texto
  tft.setFreeFont(FSB18);                    // Fuente grande (cambia si es necesario)
  tft.drawString("FELIZ NAVIDAD", 160, 100); // Texto en el centro
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
  tft.setTextDatum(MC_DATUM);                 // Centra el texto
  tft.setFreeFont(FSB18);                     // Fuente grande (cambia si es necesario)
  tft.drawString("HAPPY NEW YEAR", 160, 100); // Texto en el centro
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

String lunitaporc(String currentDate, String currentTime)
{
  moonData_t moon;
  int dia = currentDate.substring(0, 2).toInt();
  int mes = currentDate.substring(3, 5).toInt();
  int anio = currentDate.substring(6, 10).toInt();
  int hora = currentTime.substring(0, 2).toInt();
  int minuto = currentTime.substring(3, 5).toInt();
  unsigned long segundo = timeClient.getSeconds();

  struct tm timeinfo;
  timeinfo.tm_year = anio - 1900; // Año desde 1900
  timeinfo.tm_mon = mes - 1;      // Mes (0 = enero)
  timeinfo.tm_mday = dia;         // Día del mes
  timeinfo.tm_hour = hora;        // Hora
  timeinfo.tm_min = minuto;       // Minutos
  timeinfo.tm_sec = segundo;      // Segundos
  timeinfo.tm_isdst = -1;         // Automático DST

  // Convertir a time_t
  time_t cadenaDeTiempo = mktime(&timeinfo);

  // Obtener fase de la luna
  moon = mymoonPhase.getPhase(cadenaDeTiempo);
  double porcentajeIluminado = moon.percentLit * 100;

  // Formatear texto
  char porcentajeTexto[10];
  snprintf(porcentajeTexto, sizeof(porcentajeTexto), "%.2f%%", porcentajeIluminado);

  return "Luna Iluminada Al - " + String(porcentajeTexto);
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
  String LUNAP = lunitaporc(fechaFormateada, horaFormateada);
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
  if (Tresultado.first != "" && Tresultado.second != "")
  {
    Serial.println("M8AX - Ciudad: " + Tresultado.first);
    Serial.println("M8AX - Temperatura: " + Tresultado.second);
  }
  else
  {
    Tresultado.first = "Quien Sabe Donde";
    Tresultado.second = "xxC";
    Serial.println("M8AX - No Se Pudo Obtener La Ciudad O La Temperatura.");
  }
  cadenaEnvio += quitarAcentos(Tresultado.first) + ", " + Tresultado.second + " | UTC " + ((zonilla >= 0) ? "+" : "") + String(zonilla) + " | " + LUNAP + "\n";
  cadenaEnvio += "Tiempo Minando - " + (mineria.timeMining.substring(0, mineria.timeMining.indexOf(" ")).length() == 1 ? "0" + mineria.timeMining.substring(0, mineria.timeMining.indexOf(" ")) : mineria.timeMining.substring(0, mineria.timeMining.indexOf(" "))) + " Días" + mineria.timeMining.substring(mineria.timeMining.indexOf(" ") + 1) + "\n";
  cadenaEnvio += "HR Actual - " + mineria.currentHashRate + " KH/s ( MAX - " + String(maxkh) + " | MIN - " + String(minkh) + " )\n";
  cadenaEnvio += "Temp. De CPU - " + mineria.temp + "° ( MAX - " + String(maxtemp) + "° | MIN - " + String(mintemp) + "° ) | " + obtenerEstadoTemperatura(mineria.temp.toInt()) + "\n";
  cadenaEnvio += "RAM Libre - " + String(ESP.getFreeHeap()) + " Bytes\n";
  cadenaEnvio += "Plantillas De Bloque - " + mineria.templates + "\n";
  cadenaEnvio += "Shares Enviados A La Pool - " + mineria.completedShares + "\n";
  cadenaEnvio += "Mejor Dificultad Alcanzada - " + mineria.bestDiff + "\n";
  cadenaEnvio += "Cómputo Total - " + mineria.totalKHashes + " KH - ( " + String(atof(mineria.totalKHashes.c_str()) / 1000, 3) + " MH )\n";
  cadenaEnvio += "Pool De Minería - " + Settings.PoolAddress + "\n";
  cadenaEnvio += "Puerto Del Pool - " + String(Settings.PoolPort) + "\n";
  cadenaEnvio += "Tu Wallet De BTC - " + String(Settings.BtcWallet) + "\n";
  cadenaEnvio += "Tu IP - " + getPublicIP() + "\n";
  cadenaEnvio += "WiFi RSSI " + String(WiFi.RSSI()) + " | Señal 1 A 6 - " + evaluarRSSI(String(WiFi.RSSI())) + "\n";
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
  Serial.println("M8AX - Hora Sincronizada Correctamente...");
  if (Settings.Timezone == 1 || Settings.Timezone == 2)
  {
    zonilla = obtenerZonaHoraria();
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
      tft.setTextSize(1);
      tft.setTextColor(TFT_BLACK);
      tft.setCursor(tft.width() - tft.textWidth("By M8AX") - 5, tft.height() - tft.fontHeight() - 2);
      tft.print("By M8AX");
      tft.setCursor(5, tft.height() - tft.fontHeight() - 2);
      tft.print("youtube.com/m8ax");
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
  mineria = getMiningData(mElapsed);
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
  render.rdrawString(mineria.totalMHashes.c_str(), 268 - wdtOffset, 138, TFT_BLACK);
  // Block templates
  render.setFontSize(18);
  render.setAlignment(Align::TopLeft);
  render.drawString(mineria.templates.c_str(), 189 - wdtOffset, 20, 0xDEDB);
  // Best diff
  render.drawString(mineria.bestDiff.c_str(), 189 - wdtOffset, 48, 0xDEDB);
  // 32Bit shares
  render.setFontSize(18);
  render.drawString(mineria.completedShares.c_str(), 189 - wdtOffset, 76, 0xDEDB);
  // Hores
  render.setFontSize(14);
  render.rdrawString(mineria.timeMining.c_str(), 315 - wdtOffset, 104, 0xDEDB);
  // Valid Blocks
  render.setFontSize(24);
  render.setAlignment(Align::TopCenter);
  render.drawString(mineria.valids.c_str(), 287 - wdtOffset, 56, 0xDEDB);
  // Print Temp
  render.setFontSize(10);
  render.rdrawString(mineria.temp.c_str(), 239 - wdtOffset, 1, TFT_BLACK);
  render.setFontSize(4);
  render.rdrawString(String(0).c_str(), 244 - wdtOffset, 3, TFT_BLACK);
  // Print Hour
  render.setFontSize(10);
  render.rdrawString(mineria.currentTime.c_str(), 286 - wdtOffset, 1, TFT_BLACK);
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
  render.rdrawString(mineria.currentHashRate.c_str(), 121, 118 - 90, TFT_BLACK);
  // Push prepared background to screen
  background.pushSprite(0, 90);
  // Delete sprite to free the memory heap
  background.deleteSprite();
  Serial.printf("M8AX - >>> Completados %s Share(s), %s Khashes, Prom. Hashrate %s KH/s %s°\n",
                mineria.completedShares.c_str(), mineria.totalKHashes.c_str(), mineria.currentHashRate.c_str(), mineria.temp.c_str());
  cuentita++;
  if (cuentita == 15)
  {
    ajustarZonaHoraria();
    obtenerLocYTemp();
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
  mineria = getMiningData(mElapsed);
  // Create background sprite to print data at once
  createBackgroundSprite(270, 36);
  // Print background screen
  background.pushImage(0, -130, minerClockWidth, minerClockHeight, minerClockScreen);
  // Hashrate
  render.setFontSize(22);
  render.setFontColor(TFT_BLACK);
  render.rdrawString(mineria.currentHashRate.c_str(), 95, 0, TFT_BLACK);
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
  background.drawString(mineria.currentTime, 0, 50, GFXFF);
  // Push prepared background to screen
  background.pushSprite(130, 3);
  // Delete sprite to free the memory heap
  background.deleteSprite();
  Serial.printf("M8AX - >>> Completados %s Share(s), %s Khashes, Prom. Hashrate %s KH/s %s°\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str(), mineria.temp.c_str());
  cuentita++;
  if (cuentita == 15)
  {
    ajustarZonaHoraria();
    obtenerLocYTemp();
  }
#ifdef DEBUG_MEMORY
  // Print heap
  printheap();
#endif
}

void esp32_2432S028R_m8axScreen2(unsigned long mElapsed)
{
  clock_data data = getClockData(mElapsed);
  mineria = getMiningData(mElapsed);
  int dia = data.currentDate.substring(0, 2).toInt();
  int mes = data.currentDate.substring(3, 5).toInt();
  int anio = data.currentDate.substring(6, 10).toInt();
  int num1 = data.currentTime.charAt(0) - '0'; // Primer dígito de la hora
  int num2 = data.currentTime.charAt(1) - '0'; // Segundo dígito de la hora
  int num3 = data.currentTime.charAt(3) - '0'; // Primer dígito de los minutos
  int num4 = data.currentTime.charAt(4) - '0'; // Segundo dígito de los minutos
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
                mineria.completedShares.c_str(), mineria.totalKHashes.c_str(), mineria.currentHashRate.c_str(), mineria.temp.c_str());
  cuentita++;
  if (cuentita == 15)
  {
    ajustarZonaHoraria();
    obtenerLocYTemp();
  }
#ifdef DEBUG_MEMORY
  // Print heap
  printheap();
#endif
}

void RelojDeNumeros(unsigned long mElapsed)
{
  clock_data data = getClockData(mElapsed);
  mineria = getMiningData(mElapsed);
  int millonario = atoi(mineria.valids.c_str());
  unsigned long segundo = timeClient.getSeconds();
  int horas = mineria.currentTime.substring(0, 2).toInt();
  int minutos = mineria.currentTime.substring(3, 5).toInt();
  int segundos = segundo % 60;
  int horis1 = horas / 10;       // Primer dígito de las horas
  int horis2 = horas % 10;       // Segundo dígito de las horas
  int minutis1 = minutos / 10;   // Primer dígito de los minutos
  int minutis2 = minutos % 10;   // Segundo dígito de los minutos
  int segundis1 = segundos / 10; // Primer dígito de los segundos
  int segundis2 = segundos % 10; // Segundo dígito de los segundos
  std::string quediase = obtenerDiaSemana(std::string(data.currentDate.c_str()));
  int dia = data.currentDate.substring(0, 2).toInt();
  int mes = data.currentDate.substring(3, 5).toInt();
  int anio = data.currentDate.substring(6, 10).toInt();
  int horis = mineria.currentTime.substring(0, 2).toInt();
  int mins = mineria.currentTime.substring(3, 5).toInt();
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
  String TempCPU = String(mineria.temp.c_str());
  if (hasChangedScreen)
    tft.pushImage(0, 0, initWidth, initHeight, ImagenM8AX);
  printPoolData();
  refresca++;
  if (refresca > 0)
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
  Serial.printf("M8AX - >>> Completados %s Share(s), %s Khashes, Prom. Hashrate %s KH/s %s°\n",
                mineria.completedShares.c_str(), mineria.totalKHashes.c_str(), mineria.currentHashRate.c_str(), mineria.temp.c_str());
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
  tft.print(String(mineria.currentHashRate.c_str()) + " KH/s");
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
  int X_INICIO = 5;                                                                // Columna fija
  int X_FINAL = 242;                                                               // Base de la barra
  int Y_POS = 43;                                                                  // Punto más alto
  int longitud_total = X_FINAL - X_INICIO;                                         // Longitud total de la barra (80 píxeles)
  int longitud_pintada = (longitud_total * mineria.currentHashRate.toInt()) / 360; // Porcentaje de la barra según los segundos
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
  cuentita++;
  if (cuentita == 15)
  {
    ajustarZonaHoraria();
    obtenerLocYTemp();
  }
#ifdef DEBUG_MEMORY
  // Print heap
  printheap();
#endif
}

void esp32_2432S028R_m8axScreen1(unsigned long mElapsed)
{
  clock_data data = getClockData(mElapsed);
  mineria = getMiningData(mElapsed);
  moonData_t moon;
  int dia = data.currentDate.substring(0, 2).toInt();
  int mes = data.currentDate.substring(3, 5).toInt();
  int anio = data.currentDate.substring(6, 10).toInt();
  int hora = data.currentTime.substring(0, 2).toInt();
  int minuto = data.currentTime.substring(3, 5).toInt();
  unsigned long segundo = timeClient.getSeconds();
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
  String textoFinal = String(porcentajeTexto);
  int horis = timeClient.getHours();
  int minutis = timeClient.getMinutes();
  int secondis = timeClient.getSeconds();
  if (hasChangedScreen)
    tft.fillRect(0, 0, 320, 170, TFT_BLACK);
  printPoolData();
  refresca++;
  if (refresca > 0)
  {
    tft.fillRect(0, 0, 320, 170, TFT_BLACK);
    refresca = 0;
  }
  hasChangedScreen = false;
  int wdtOffset = 190;
  Serial.printf("M8AX - >>> Completados %s Share(s), %s Khashes, Prom. Hashrate %s KH/s %s°\n",
                mineria.completedShares.c_str(), mineria.totalKHashes.c_str(), mineria.currentHashRate.c_str(), mineria.temp.c_str());
  colorIndex = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  relojAnalogicoM8AX(horis, minutis, secondis);
  tft.setCursor(4, 42);
  tft.setTextColor(colors[colorIndex]);
  tft.setTextSize(3);
  tft.print(dia);
  tft.setCursor(4, 74);
  String mesecillo = obtenerNombreMes(mes).substring(0, 3);
  tft.print(mesecillo);
  tft.setCursor(4, 106);
  char fechaFormateada[11];
  sprintf(fechaFormateada, "%02d/%02d/%04d", dia, mes, anio);
  std::string quediase = obtenerDiaSemana(std::string(fechaFormateada));
  tft.print(String(quediase.c_str()));
  quediase.clear();
  quediase.shrink_to_fit();
  tft.setCursor(4, 138);
  tft.print(anio);
  tft.setCursor(230, 150);
  tft.setTextSize(2);
  tft.print(mineria.currentHashRate);
  tft.setCursor(250, 126);
  tft.print("KH/s");
  tft.setCursor(250, 37);
  tft.print("RSSI");
  tft.setCursor(250, 69);
  tft.print(String(WiFi.RSSI()) + "dB");
  tft.setCursor(240, 101);
  tft.print(evaluarRSSI(String(WiFi.RSSI())));
  tft.setCursor(4, 5);
  tft.setTextSize(1);
  tft.print("MvIiIaX - M8AX");
  if (Tresultado.first != "" && Tresultado.second != "")
  {
    Serial.println("M8AX - Ciudad: " + Tresultado.first);
    Serial.println("M8AX - Temperatura: " + Tresultado.second);
    tft.setCursor(4, 17);
    tft.print(quitarAcentos(Tresultado.first));
    tft.setCursor(4, 27);
    Tresultado.second.replace("°", "");
    tft.print(Tresultado.second);
    tft.setCursor(38, 27);
    tft.print(textoFinal);
  }
  else
  {
    Serial.println("M8AX - No Se Pudo Obtener La Ciudad O La Temperatura.");
    tft.setCursor(4, 17);
    tft.print("Quien Sabe Donde");
    tft.setCursor(4, 27);
    tft.print("xxC");
    tft.setCursor(38, 27);
    tft.print(textoFinal);
    obtenerLocYTemp();
  }
  tft.setCursor(215, 5);
  tft.print("youtube.com/m8ax");
  tft.setCursor(215, 15);
  tft.print("paypal.me/m8ax/2");
  cuentita++;
  dibujarPorcentajeLunar(61, 57, 16, porcentajeIluminado);
  if (cuentita == 15)
  {
    ajustarZonaHoraria();
    obtenerLocYTemp();
  }
  if (cuentita % 1800 == 0)
  {
    obtenerLocYTemp();
  }
#ifdef DEBUG_MEMORY
  // Print heap
  printheap();
#endif
}

void tDisplay_m8axScreen4(unsigned long mElapsed)
{
  mineria = getMiningData(mElapsed);
  if (hasChangedScreen)
    tft.pushImage(0, 0, initWidth, initHeight, M8AXRelojLunar);
  printPoolData();
  refresca++;
  hasChangedScreen = false;
  int wdtOffset = 190;
  // Recreate sprite to the right side of the screen
  createBackgroundSprite(WIDTH - 5, HEIGHT - 7);
  // Print background screen
  if (refresca > 0)
  {
    refresca = 0;
    tft.fillRect(0, 0, 320, 170, TFT_BLACK);
  }
  Serial.printf("M8AX - >>> Completados %s Share(s), %s Khashes, Prom. Hashrate %s KH/s %s°\n",
                mineria.completedShares.c_str(), mineria.totalKHashes.c_str(), mineria.currentHashRate.c_str(), mineria.temp.c_str());
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
  tft.print(String(mineria.currentHashRate.c_str()));
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
  cuentita++;
  if (cuentita == 15)
  {
    ajustarZonaHoraria();
    obtenerLocYTemp();
  }
#ifdef DEBUG_MEMORY
  // Print heap
  printheap();
#endif
}

void tDisplay_m8axScreen5(unsigned long mElapsed)
{
  clock_data data = getClockData(mElapsed);
  mineria = getMiningData(mElapsed);
  solouna = 0;
  Serial.printf("M8AX - >>> Completados %s Share(s), %s Khashes, Prom. Hashrate %s KH/s %s°\n",
                mineria.completedShares.c_str(), mineria.totalKHashes.c_str(), mineria.currentHashRate.c_str(), mineria.temp.c_str());
  if (hasChangedScreen)
    tft.pushImage(0, 0, initWidth, initHeight, M8AXRelojLunar);
  printPoolData();
  refresca++;
  hasChangedScreen = false;
  int wdtOffset = 190;
  // Recreate sprite to the right side of the screen
  createBackgroundSprite(WIDTH - 5, HEIGHT - 7);
  if (solounavez == 0)
  {
    numnotis++;
    refresca = 0;
    tft.fillRect(0, 0, 320, 170, TFT_BLACK);
    solounavez = 1;
    obtenerNoticias();
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(112, 160);
    tft.setTextSize(1);
    tft.print("M8AX-Noticias Num-" + String(numnotis));
    tft.setTextColor(TFT_WHITE);
    tft.drawLine(0, 20, 320, 20, colors[colorI]); // Dibujar línea de (0, y) a (320, y)
    tft.setCursor(1, 6);
    tft.setTextSize(1);
    int millonario = atoi(mineria.valids.c_str());
    if (millonario == 0)
    {
      tft.print(mineria.currentTime + " - " + data.currentDate + " - " + mineria.currentHashRate + " KH/s - " + mineria.temp + " Grados." + " NO RICO");
    }
    else
    {
      tft.print(mineria.currentTime + " - " + data.currentDate + " - " + mineria.currentHashRate + " KH/s - " + mineria.temp + " Grados." + " SI RICO");
    }
  }
  if (refresca > 60)
  {
    refresca = 0;
    tft.fillRect(0, 0, 320, 170, TFT_BLACK);
    numnotis++;
    obtenerNoticias();
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(112, 160);
    tft.setTextSize(1);
    tft.print("M8AX-Noticias Num-" + String(numnotis));
    tft.setTextColor(TFT_WHITE);
    tft.drawLine(0, 20, 320, 20, colors[colorI]); // Dibujar línea de (0, y) a (320, y)
    tft.setCursor(1, 6);
    tft.setTextSize(1);
    int millonario = atoi(mineria.valids.c_str());
    if (millonario == 0)
    {
      tft.print(mineria.currentTime + " - " + data.currentDate + " - " + mineria.currentHashRate + " KH/s - " + mineria.temp + " Grados." + " NO RICO");
    }
    else
    {
      tft.print(mineria.currentTime + " - " + data.currentDate + " - " + mineria.currentHashRate + " KH/s - " + mineria.temp + " Grados." + " SI RICO");
    }
  }
  cuentita++;
  if (cuentita == 15)
  {
    ajustarZonaHoraria();
    obtenerLocYTemp();
  }
#ifdef DEBUG_MEMORY
  // Print heap
  printheap();
#endif
}

void tDisplay_m8axScreen6(unsigned long mElapsed)
{
  clock_data data = getClockData(mElapsed);
  mineria = getMiningData(mElapsed);
  solounavez = 0;
  Serial.printf("M8AX - >>> Completados %s Share(s), %s Khashes, Prom. Hashrate %s KH/s %s°\n",
                mineria.completedShares.c_str(), mineria.totalKHashes.c_str(), mineria.currentHashRate.c_str(), mineria.temp.c_str());
  if (hasChangedScreen)
    tft.pushImage(0, 0, initWidth, initHeight, M8AXRelojLunar);
  printPoolData();
  refresca++;
  hasChangedScreen = false;
  int wdtOffset = 190;
  // Recreate sprite to the right side of the screen
  createBackgroundSprite(WIDTH - 5, HEIGHT - 7);
  if (solouna == 0)
  {
    numfrases++;
    solouna = 1;
    refresca = 0;
    tft.fillRect(0, 0, 320, 170, TFT_BLACK);
    HTTPClient http;
    String quote = getQuote();
    displayQuote(quote);
    quote = "";
    quote.reserve(0);
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(112, 160);
    tft.setTextSize(1);
    tft.print("M8AX-Frase Num-" + String(numfrases));
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(1, 6);
    tft.setTextSize(1);
    int millonario = atoi(mineria.valids.c_str());
    if (millonario == 0)
    {
      tft.print(mineria.currentTime + " - " + data.currentDate + " - " + mineria.currentHashRate + " KH/s - " + mineria.temp + " Grados." + " NO RICO");
    }
    else
    {
      tft.print(mineria.currentTime + " - " + data.currentDate + " - " + mineria.currentHashRate + " KH/s - " + mineria.temp + " Grados." + " SI RICO");
    }
  }
  if (refresca > 10)
  {
    refresca = 0;
    tft.fillRect(0, 0, 320, 170, TFT_BLACK);
    numfrases++;
    HTTPClient http;
    String quote = getQuote();
    displayQuote(quote);
    quote = "";
    quote.reserve(0);
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(112, 160);
    tft.setTextSize(1);
    tft.print("M8AX-Frase Num-" + String(numfrases));
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(1, 6);
    tft.setTextSize(1);
    int millonario = atoi(mineria.valids.c_str());
    if (millonario == 0)
    {
      tft.print(mineria.currentTime + " - " + data.currentDate + " - " + mineria.currentHashRate + " KH/s - " + mineria.temp + " Grados." + " NO RICO");
    }
    else
    {
      tft.print(mineria.currentTime + " - " + data.currentDate + " - " + mineria.currentHashRate + " KH/s - " + mineria.temp + " Grados." + " SI RICO");
    }
  }
  cuentita++;
  if (cuentita == 15)
  {
    ajustarZonaHoraria();
    obtenerLocYTemp();
  }
#ifdef DEBUG_MEMORY
  // Print heap
  printheap();
#endif
}

void tDisplay_m8axScreen7(unsigned long mElapsed)
{
  mineria = getMiningData(mElapsed);
  clock_data data = getClockData(mElapsed);
  Serial.printf("M8AX - >>> Completados %s Share(s), %s Khashes, Prom. Hashrate %s KH/s %s°\n",
                mineria.completedShares.c_str(), mineria.totalKHashes.c_str(), mineria.currentHashRate.c_str(), mineria.temp.c_str());
  if (hasChangedScreen)
    tft.pushImage(0, 0, initWidth, initHeight, M8AXRelojLunar);
  printPoolData();
  refresca++;
  hasChangedScreen = false;
  int wdtOffset = 190;
  // Recreate sprite to the right side of the screen
  createBackgroundSprite(WIDTH - 5, HEIGHT - 7);
  if (refresca > 0)
  {
    refresca = 0;
    tft.fillRect(0, 0, 320, 170, TFT_BLACK);
  }
  int horita = data.currentTime.substring(0, 2).toInt();
  int minutitos = data.currentTime.substring(3, 5).toInt();
  unsigned long segundo = timeClient.getSeconds();
  int segundos = segundo % 60;
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
  tft.setTextSize(2);
  tft.setCursor(3, 92);
  int numero = random(1, 51);
  if (numero == 1)
  {
    tft.print("Por Muchas Vueltas Que Demos, Siempre Tendremos El Culo Atras...");
  }
  else if (numero == 2)
  {
    tft.print("El Futuro No Esta Establecido, No Hay Destino, Solo Existe El Que Nosotros Hacemos...");
  }
  else if (numero == 3)
  {
    tft.print("Oro Digital: Minado Con Codigo, Asegurado Con Blockchain...");
  }
  else if (numero == 4)
  {
    tft.print("Quien Controla El Dinero, Controla La Sociedad. Bitcoin Le Devuelve El Poder A La Gente.");
  }
  else if (numero == 5)
  {
    tft.print("No Confies, Verifica. La Cadena De Bloques No Miente, Pero Los Gobiernos Si.");
  }
  else if (numero == 6)
  {
    tft.print("El Dinero Fuerte Crea Civilizaciones Fuertes; La Inflacion Es El Impuesto De Los Tiranos.");
  }
  else if (numero == 7)
  {
    tft.print("El Oro Fue Dinero Durante Siglos, Bitcoin Lo Es En La Era Digital.");
  }
  else if (numero == 8)
  {
    tft.print("Las Reglas Del Sistema Financiero Tradicional Benefician A Pocos; Bitcoin Cambia El Juego.");
  }
  else if (numero == 9)
  {
    tft.print("El Tiempo Es La Moneda Mas Valiosa. Bitcoin Es La Unica Que No Se Devalua.");
  }
  else if (numero == 10)
  {
    tft.print("Los Bancos Imprimen Dinero Sin Control, Bitcoin Solo 21 Millones. Haz Los Calculos.");
  }
  else if (numero == 11)
  {
    tft.print("Blockchain No Duerme, No Cierra, No Pide Permiso.");
  }
  else if (numero == 12)
  {
    tft.print("Si Tu Dinero No Es Tuyo, Es Del Banco.");
  }
  else if (numero == 13)
  {
    tft.print("Bitcoin No Se Quema, No Se Devalua, No Se Congela.");
  }
  else if (numero == 14)
  {
    tft.print("Los Billeteros Pierden Llaves, Bitcoin No Pierde Valor.");
  }
  else if (numero == 15)
  {
    tft.print("Fiat Es Promesa, Bitcoin Es Codigo.");
  }
  else if (numero == 16)
  {
    tft.print("Las Ballenas Compran En La Caida, Los Novatos Venden.");
  }
  else if (numero == 17)
  {
    tft.print("Bitcoin Es Paciencia, No Loteria.");
  }
  else if (numero == 18)
  {
    tft.print("El Precio Sube Y Baja, Pero Bitcoin Se Queda.");
  }
  else if (numero == 19)
  {
    tft.print("Si No Controlas Tus Llaves, No Controlas Tu Dinero.");
  }
  else if (numero == 20)
  {
    tft.print("El Dinero Bueno Expulsa Al Malo, Bitcoin Gana.");
  }
  else if (numero == 21)
  {
    tft.print("Bitcoin No Pide Permiso, Se Usa.");
  }
  else if (numero == 22)
  {
    tft.print("Las Reglas De Bitcoin Son Claras, Las Del Banco Cambian Diario.");
  }
  else if (numero == 23)
  {
    tft.print("Si Puedes Imprimirlo, No Es Escaso.");
  }
  else if (numero == 24)
  {
    tft.print("Tu Banco Puede Cerrar, Tu Wallet No.");
  }
  else if (numero == 25)
  {
    tft.print("El Mercado Es Volatil, Pero La Inflacion Es Segura.");
  }
  else if (numero == 26)
  {
    tft.print("Minar Es Convertir Electricidad En Dinero.");
  }
  else if (numero == 27)
  {
    tft.print("Blockchain No Pregunta Quién Eres, Solo Si Tienes La Llave.");
  }
  else if (numero == 28)
  {
    tft.print("El Futuro Financiero Es Descentralizado.");
  }
  else if (numero == 29)
  {
    tft.print("Bitcoin No Es Anonimo, Pero Es Libre.");
  }
  else if (numero == 30)
  {
    tft.print("El Primer Bitcoin Que Compras Es El Mas Dificil.");
  }
  else if (numero == 31)
  {
    tft.print("Tu Abuela No Entiende Bitcoin, Pero Tampoco Entiende SWIFT.");
  }
  else if (numero == 32)
  {
    tft.print("El Dinero Facil Se Imprime, Bitcoin Se Gana.");
  }
  else if (numero == 33)
  {
    tft.print("El Verdadero Hodler No Se Preocupa Por La Caida.");
  }
  else if (numero == 34)
  {
    tft.print("Bitcoin No Te Hace Rico Rapido, Pero Te Protege De Hacerte Pobre.");
  }
  else if (numero == 35)
  {
    tft.print("Si La Luz Se Apaga, El Efectivo No Funciona; Bitcoin Si.");
  }
  else if (numero == 36)
  {
    tft.print("Los Bancos Se Rescatan, Los Bitcoiners Se Adaptan.");
  }
  else if (numero == 37)
  {
    tft.print("La Revolucion No Sera Televisada, Sera Minada.");
  }
  else if (numero == 38)
  {
    tft.print("Bitcoin No Se Imprime, Se Descubre.");
  }
  else if (numero == 39)
  {
    tft.print("La Verdad Es Inmutable, Como La Blockchain.");
  }
  else if (numero == 40)
  {
    tft.print("Bitcoin No Es Para Todos, Solo Para Quienes Lo Entienden.");
  }
  else if (numero == 41)
  {
    tft.print("Tu Banco Te Cobra Por Guardar Tu Dinero, Bitcoin No.");
  }
  else if (numero == 42)
  {
    tft.print("Las Transacciones Son Baratas Hasta Que El Banco Cobra Comision.");
  }
  else if (numero == 43)
  {
    tft.print("Bitcoin No Tiene Fronteras, Pero Los Bancos Si.");
  }
  else if (numero == 44)
  {
    tft.print("No Es Demasiado Tarde, Solo Es Mas Caro Que Ayer.");
  }
  else if (numero == 45)
  {
    tft.print("Bitcoin No Necesita Rescates, Solo Tiempo.");
  }
  else if (numero == 46)
  {
    tft.print("El Precio No Es Importante, La Adopcion Lo Es.");
  }
  else if (numero == 47)
  {
    tft.print("El Efecto Red Hace Bitcoin Mas Fuerte Cada Dia.");
  }
  else if (numero == 48)
  {
    tft.print("El Dinero Centralizado Es Herramienta De Control.");
  }
  else if (numero == 49)
  {
    tft.print("Bitcoin Es Un Experimento Que Funciona.");
  }
  else if (numero == 50)
  {
    tft.print("No Es Magia, Es Matematica.");
  }
  tft.setCursor(128, 160);
  tft.setTextSize(1);
  tft.print(mineria.currentHashRate + " KH/s");
  cuentita++;
  if (cuentita == 15)
  {
    ajustarZonaHoraria();
    obtenerLocYTemp();
  }
#ifdef DEBUG_MEMORY
  // Print heap
  printheap();
#endif
}

void tDisplay_m8axScreen3(unsigned long mElapsed)
{
  mineria = getMiningData(mElapsed);
  solounavez = 0;
  solouna = 0;
  String btcm8 = "bitcoin:bc1qljq00pm2plq2l9jxzdzt0xc8t79j9wcmu7r8em";
  Serial.printf("M8AX - >>> Completados %s Share(s), %s Khashes, Prom. Hashrate %s KH/s %s°\n",
                mineria.completedShares.c_str(), mineria.totalKHashes.c_str(), mineria.currentHashRate.c_str(), mineria.temp.c_str());
  if (hasChangedScreen)
    tft.pushImage(0, 0, initWidth, initHeight, M8AXRelojLunar);
  printPoolData();
  refresca++;
  hasChangedScreen = false;
  int wdtOffset = 190;
  // Recreate sprite to the right side of the screen
  createBackgroundSprite(WIDTH - 5, HEIGHT - 7);
  if (refresca > 0)
  {
    refresca = 0;
    tft.fillRect(0, 0, 320, 170, TFT_BLACK);
  }
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
  tft.setCursor(128, 160);
  tft.setTextSize(1);
  tft.print(mineria.currentHashRate + " KH/s");
  uint16_t colorss[] = {TFT_WHITE, TFT_YELLOW, TFT_CYAN, TFT_GREENYELLOW}; // Array de colores
  int colorrr = esp_random() % 4;
  dibujaQR(btcm8, (320 - 150) / 2, (170 - 150) / 2, 150, colorss[colorrr]); // Dibuja el QR centrado en la pantalla 320x170
  cuentita++;
  if (cuentita == 15)
  {
    ajustarZonaHoraria();
    obtenerLocYTemp();
  }
#ifdef DEBUG_MEMORY
  // Print heap
  printheap();
#endif
}

void tDisplay_m8axScreenMegaNerd(unsigned long mElapsed)
{
  clock_data data = getClockData(mElapsed);
  mineria = getMiningData(mElapsed);
  unsigned long segundo = timeClient.getSeconds();
  solounavez = 0;
  int segundos = segundo % 60;
  int horas = data.currentTime.substring(0, 2).toInt();
  int minutos = data.currentTime.substring(3, 5).toInt();
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  Serial.printf("M8AX - >>> Completados %s Share(s), %s Khashes, Prom. Hashrate %s KH/s %s°\n",
                mineria.completedShares.c_str(), mineria.totalKHashes.c_str(), mineria.currentHashRate.c_str(), mineria.temp.c_str());
  if (hasChangedScreen)
    tft.pushImage(0, 0, initWidth, initHeight, M8AXRelojLunar);
  printPoolData();
  refresca++;
  if (refresca > 0)
  {
    tft.pushImage(0, 0, initWidth, initHeight, M8AXRelojLunar);
    refresca = 0;
  }
  hasChangedScreen = false;
  int wdtOffset = 190;
  // Recreate sprite to the right side of the screen
  createBackgroundSprite(WIDTH - 5, HEIGHT - 7);
  // Print background screen
  background.pushImage(-190, 0, ImagenM8AXWidth, ImagenM8AXHeight, M8AXRelojLunar);
  const char *hashRateStr = data.currentHashRate.c_str(); // Obtener la cadena
  float hashRa = atof(hashRateStr);                       // Convertirla a flotante (float)
  dibujaAnalogKH(hashRa);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(1);
  uint32_t rndnumero2 = esp_random();
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
  int millonario = atoi(mineria.valids.c_str());
  if (millonario == 0)
  {
    tft.print(mineria.currentTime + " - " + data.currentDate + " - " + mineria.currentHashRate + " KH/s - " + mineria.temp + " Grados." + " NO RICO");
  }
  else
  {
    tft.print(mineria.currentTime + " - " + data.currentDate + " - " + mineria.currentHashRate + " KH/s - " + mineria.temp + " Grados." + " SI RICO");
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
  tft.print(mineria.temp);
  tft.setTextSize(1);
  tft.setCursor(283, 103);
  tft.print("o");
  tft.setCursor(208, 138);
  tft.print("HMS");
  std::string quediase = obtenerDiaSemana(std::string(data.currentDate.c_str()));
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
  cuentita++;
  if (cuentita == 15)
  {
    ajustarZonaHoraria();
    obtenerLocYTemp();
  }
#ifdef DEBUG_MEMORY
  // Print heap
  printheap();
#endif
}

void esp32_2432S028R_BTCprice(unsigned long mElapsed)
{
  clock_data data = getClockData(mElapsed);
  mineria = getMiningData(mElapsed);
  int hora = timeClient.getHours();
  int minuto = timeClient.getMinutes();
  int segundo = timeClient.getSeconds();
  // Formatear la hora
  char hora_formateada[9]; // "HH:MM:SS" + null
  snprintf(hora_formateada, sizeof(hora_formateada), "%02d:%02d:%02d", hora, minuto, segundo);
  if (hasChangedScreen)
    tft.pushImage(0, 0, priceScreenWidth, priceScreenHeight, priceScreen);
  printPoolData();
  hasChangedScreen = false;
  // Create background sprite to print data at once
  createBackgroundSprite(270, 36);
  // Print background screen
  background.pushImage(0, -130, priceScreenWidth, priceScreenHeight, priceScreen);
  // Hashrate
  render.setFontSize(23);
  render.setFontColor(TFT_BLACK);
  render.rdrawString(mineria.currentHashRate.c_str(), 95, 0, TFT_BLACK);
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
  background.drawString(hora_formateada, 202 - 135, 0, GFXFF);
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
                mineria.completedShares.c_str(), mineria.totalKHashes.c_str(), mineria.currentHashRate.c_str(), mineria.temp.c_str());
  cuentita++;
  if (cuentita == 15)
  {
    ajustarZonaHoraria();
    obtenerLocYTemp();
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
          uncontadormas = 50;
        }
        else if (mes == 1)
        {
          nevar2();
          Serial.println("M8AX - Felicitando El Año Nuevo...");
          uncontadormas = 50;
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
    obtenerLocYTemp();
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
      previousMillis = currentMillis;
      digitalWrite(LED_PIN_B, HIGH);
      digitalWrite(LED_PIN, !digitalRead(LED_PIN)); // Cambia el estado del LED
    }
    break;
  }
}

CyclicScreenFunction esp32_2432S028RCyclicScreens[] = {esp32_2432S028R_MinerScreen, esp32_2432S028R_BTCprice, esp32_2432S028R_ClockScreen, RelojDeNumeros, tDisplay_m8axScreen7, esp32_2432S028R_m8axScreen1, esp32_2432S028R_m8axScreen2, tDisplay_m8axScreenMegaNerd, tDisplay_m8axScreen5, tDisplay_m8axScreen6, tDisplay_m8axScreen3, tDisplay_m8axScreen4};

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