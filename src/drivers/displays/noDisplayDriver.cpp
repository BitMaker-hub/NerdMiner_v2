/***********************************************************************************************************************************
 *
 *   Escrito por: M8AX
 *
 *   Descripción:
 *   ------------
 *
 *   Versión para placas WROOM ESP32D, optimizada para minar a 395 KH/s sin pantalla.
 *   Utilizaremos el LED para indicar estados importantes.
 *
 *   Comportamiento del LED:
 *   -----------------------
 *
 *   I     - **Al arrancar** → "Hola M8AX" en código Morse con el LED.
 *   II    - **Led encendido constante tras ligeros parpadeos rápidos** → Modo Configuración
 *   III   - **Sincronización de hora exitosa al arrancar** → Parpadeo super rápido del LED.
 *   IV    - **Parpadeo fuerte 2 ticks + 2 ticks** → Minando a más de 350 KH/s - ( de 8h a 19h )
 *   V     - **Parpadeo tenue 2 ticks + 2 ticks** → Minando a más de 350 KH/s - ( de 20h a 7h )
 *   VI    - **Parpadeo fuerte tick medio largo y uno más corto** → Minando a menos de 350 KH/s.
 *   VII   - **Sin LED azul** → No está minando.
 *   VIII  - **LED encendiéndose y apagándose a lo loco no simétricos** → ¡Has minado un bloque! ¡ERES RICO!
 *   IX    - **Parpadeo estilo "pi pi" de reloj Casio** → Es una hora en punto.
 *   X     - **Encendido corto "pi"** → Son y media.
 *   XI    - **Encendido largo (~2s)** → Enviando mensaje a Telegram con estadísticas y datos Nerd. ( si está configurado )
 *   XII   - **Parpadeo rápido corto después de enviar mensaje a Telegram** → Mensaje enviado correctamente.
 *   XIII  - **Parpadeo rápido corto + pausa 2s + parpadeo rápido corto después de enviar mensaje a Telegram** → Error En Envío :(
 *   XIV   - **Parpadeos largos** → Temperatura superior a 75°C.
 *   XV    - **Share enviado a la pool** → 5 ticks rapidos del LED.
 *   XVI   - NOTA - Si La Temperatura Pasa De 80°C, El Dispositivo Entrará En Deep Sleep 10Min, Pasados Los 10Min, Rearrancará.
 *   XVII  - ESPERO QUE OS GUSTE Y MINEIS UN BLOQUE Y SI ES ASÍ ¡ DADME ALGO COÑO !
 *   XVIII - ¡ A MINAR !
 *
 *
 *   Blockchain es una base de datos descentralizada que almacena registros de transacciones en bloques enlazados.
 *   Cada bloque tiene información sobre las transacciones, un hash único y el hash del bloque anterior.
 *   La blockchain es pública y transparente, permitiendo a cualquier persona verificar las transacciones.
 *   Los bloques son añadidos mediante un proceso llamado minería, que asegura la integridad de la cadena.
 *   Minería es el proceso de resolver complejas ecuaciones matemáticas mediante poder computacional.
 *   Hashrate es la medida de la capacidad de procesamiento de un minero, cuántos intentos de hash puede hacer por segundo.
 *   Un minero de Bitcoin es un dispositivo o programa que participa en la minería de Bitcoin.
 *   Los mineros validan y agrupan transacciones en bloques, resolviendo un problema matemático complejo.
 *   El primer minero en resolver el problema obtiene una recompensa en bitcoins por su trabajo.
 *   Bitcoin mining se basa en el algoritmo SHA-256 para resolver estos problemas de hash.
 *   Los mineros también validan que las transacciones sean legítimas antes de añadirlas a la blockchain.
 *   El proceso de minería es competitivo, ya que varios mineros intentan resolver el problema al mismo tiempo.
 *   Cuanto mayor sea el hashrate, más probabilidades tiene un minero de ganar la recompensa.
 *   Los mineros ayudan a mantener la seguridad y el funcionamiento descentralizado de la red de Bitcoin.
 *
 *
 *              ///\\\ --- Minimizando código, maximizando funcionalidad. Solo 1735 líneas de código en 5h --- ///\\\
 *
 *                                                     .M8AX Corp. - ¡A Minar!
 *
 *                                                            MARZO 2025
 *
 ***********************************************************************************************************************************/

#include "displayDriver.h"
#ifdef NO_DISPLAY
#include <Arduino.h>
#include "monitor.h"
#include "wManager.h"
#include "time.h"
#include <ctime>
#include "moonPhase.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "drivers/storage/storage.h"
#include "drivers/devices/device.h"
#include <urlencode.h>
#include <string>
#include "mbedtls/sha256.h"

#define m8ax 2
#define MAX_RESULT_LENGTH 500
#define MAX_NUMBERS 6
#define MAX_DEPTH 4
#define MAX_HASHRATE 420
#define BAR_LENGTH 42
#define GOTAAGUA 0.00005
#define pasoskm 0.0007

extern TSettings Settings;

const char *urlsm8ax[] = {
    "YT - https://youtube.com/m8ax",
    "OS - https://opensea.io/es/m8ax",
    "OC - https://oncyber.io/m8ax",
    "FW - https://m8ax.github.io/MvIiIaX-NerdMiner_V2-DeV/",
    "GH - https://github.com/m8ax"};
const char *meses[] = {
    "ENERITO", "FEBRERITO", "MARZITO", "ABRILITO", "MAYITO", "JUNITO",
    "JULITO", "AGOSTITO", "SEPTIEMBRITO", "OCTUBRITO", "NOVIEMBRITO", "DICIEMBRITO"};
const char *morse[] = {
    "....",  // H
    "---",   // O
    ".-..",  // L
    ".-",    // A
    "--",    // M
    "---..", // 8
    ".-",    // A
    "-..-",  // X
};
const char *digitosAscii[] = {
    " 000 \n0   0\n0   0\n0   0\n 000 ", // 0
    "  1  \n 11  \n  1  \n  1  \n 111 ", // 1
    " 222 \n2   2\n  2  \n 2   \n22222", // 2
    " 333 \n3   3\n  33 \n3   3\n 333 ", // 3
    "4   4\n4   4\n 4444\n    4\n    4", // 4
    "55555\n5    \n5554 \n    5\n5555 ", // 5
    " 666 \n6    \n6664 \n6   6\n 666 ", // 6
    "77777\n    7\n   7 \n  7  \n 7   ", // 7
    " 888 \n8   8\n 888 \n8   8\n 888 ", // 8
    " 999 \n9   9\n 9999\n    9\n 999 "  // 9
};
char result[MAX_RESULT_LENGTH];
char porcentajeTexto[10];
String BOT_TOKEN;
String CHAT_ID;
String enviados;
String ipPublica = "";
String subebaja = "... ESPERANDO ...", subebaja2 = "... ESPERANDO ...";
const int morseLength = sizeof(morse) / sizeof(morse[0]);
int sumatele = 1;
int maxtemp = 0;
int mintemp = 1000;
int aciertos = 0;
int fallos = 0;
int totalci = 0;
int sumacalen = 0;
int cambioDeDia = 0;
int solouna = 0;
uint32_t nominando = 0;
uint32_t cuenta = 0;
uint32_t rndnumero = 0;
uint32_t alertatemp = 0;
uint32_t totalparpadeosled = 0;
float maxkh = 0.00;
float minkh = 1000.00;
float porcentaje = 0.00;
float eficiencia = 0.00;
float consumo = 1.18;
float costo_mensual = 0.00;
float distanciaLuna = 384400;
float distanciaSol = 149600000;
float distanciadiamsol = 1390900;
float circumluna = 10921;
float precioDeBTC = 0.0;
float anterBTC = 0.0;
float anterBTC2 = 0.0;
double parpadeosPorSegundo = 1.0;
const unsigned long interval = 60 * 2 * 60;
const unsigned long minStartupTime = interval;
unsigned long lastTelegramEpochTime = 0;
unsigned long startTime = 0;
unsigned long epochTime;
unsigned long tiempoInicio;
unsigned long tiempoTranscurrido;
mining_data data;
moonPhase mymoonPhase;

typedef struct
{
  int value;
  char operation[500];
} State;

void noDisplay_Init(void)
{
}

void noDisplay_AlternateScreenState(void)
{
}

void noDisplay_AlternateRotation(void)
{
}

String horaEnIngles(int hora, int minutos, int segundos)
{
  String periodo = (hora < 12) ? "AM" : "PM";
  String horaStr, minutosStr, segundosStr;
  const char *unidades[] = {"", "One", "Two", "Three", "Four", "Five", "Six", "Seven", "Eight", "Nine"};
  const char *decenas[] = {"", "Ten", "Twenty", "Thirty", "Forty", "Fifty"};
  const char *especiales[] = {"Eleven", "Twelve", "Thirteen", "Fourteen", "Fifteen", "Sixteen", "Seventeen", "Eighteen", "Nineteen"};
  if (hora == 0)
    hora = 12;
  else if (hora > 12)
    hora -= 12;
  if (hora < 10)
    horaStr = unidades[hora];
  else if (hora == 10)
    horaStr = decenas[1];
  else if (hora == 12)
    horaStr = especiales[1];
  else
    horaStr = especiales[hora - 11];
  if (minutos == 0)
  {
    minutosStr = "O'Clock";
  }
  else if (minutos < 10)
  {
    minutosStr = "Oh " + String(unidades[minutos]);
  }
  else if (minutos < 20 && minutos > 10)
  {
    minutosStr = especiales[minutos - 11];
  }
  else
  {
    minutosStr = decenas[minutos / 10];
    if (minutos % 10 != 0)
    {
      minutosStr += "-" + String(unidades[minutos % 10]);
    }
  }
  if (segundos == 0)
  {
    segundosStr = "";
  }
  else if (segundos < 10)
  {
    segundosStr = "Oh " + String(unidades[segundos]) + String(" Seconds");
  }
  else if (segundos < 20 && segundos > 10)
  {
    segundosStr = especiales[segundos - 11] + String(" Seconds");
  }
  else
  {
    segundosStr = decenas[segundos / 10];
    if (segundos % 10 != 0)
    {
      segundosStr += "-" + String(unidades[segundos % 10]);
    }
    segundosStr += " Seconds";
  }
  String resultado = horaStr;
  if (minutosStr != "")
  {
    resultado += " " + minutosStr;
  }
  if (segundosStr != "")
  {
    resultado += " And " + segundosStr;
  }
  resultado += " " + periodo;
  return resultado;
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

const char *interpretarRangoTemperatura(float tempRang)
{
  if (tempRang < 10)
  {
    return "( Bajo )";
  }
  else if (tempRang < 20)
  {
    return "( Normal )";
  }
  else if (tempRang < 30)
  {
    return "( Alto )";
  }
  else
  {
    return "( Crítico )";
  }
}

bool esPrimillo(uint32_t num)
{
  if (num <= 1)
    return false;
  for (uint32_t i = 2; i * i <= num; i++)
  {
    if (num % i == 0)
      return false;
  }
  return true;
}

String abinyprimo(uint32_t num)
{
  String binario = "";
  for (int i = 31; i >= 0; i--)
  {
    binario += (num >> i) & 1 ? "1" : "0";
  }
  String primeLabel = esPrimillo(num) ? " - ( ES PRIMO )" : " - ( NO ES PRIMO )";
  return binario + primeLabel;
}

float calcularMbPorSegundo(float hashrateKHs)
{
  return (hashrateKHs * 1000 * 64) / 1048576.0;
}

float getPrecioBTC()
{
  const char *apiUrl = "https://api.coinbase.com/v2/prices/BTC-USD/spot";
  HTTPClient http;
  http.begin(apiUrl);
  int httpCode = http.GET();
  float btcPrice = 0.0;
  if (httpCode == HTTP_CODE_OK)
  {
    DynamicJsonDocument doc(192);
    DeserializationError error = deserializeJson(doc, http.getStream());
    http.end();
    if (!error)
    {
      btcPrice = doc["data"]["amount"].as<float>();
    }
  }
  else
  {
    http.end();
  }
  Serial.println("M8AX - Obtenido Precio De BTC...");
  return btcPrice;
}

const char *evaluarRSSI(const String &rssiStr)
{
  int rssi = rssiStr.toInt();
  if (rssi >= -50)
  {
    return "Conexión Excelente";
  }
  else if (rssi >= -60)
  {
    return "Conexión Muy Buena";
  }
  else if (rssi >= -70)
  {
    return "Conexión Buena";
  }
  else if (rssi >= -80)
  {
    return "Conexión Regular";
  }
  else if (rssi >= -90)
  {
    return "Conexión Mala";
  }
  else
  {
    return "Conexión Muy Mala";
  }
}

String getPublicIP()
{
  String publicIP = "";
  HTTPClient http;
  http.begin("http://api.ipify.org");
  int httpCode = http.GET();
  if (httpCode > 0)
  {
    Serial.println("M8AX - Obtenida IP Pública...");
    publicIP = http.getString();
  }
  else
  {
    Serial.println("M8AX - Error Al Obtener La IP Pública...");
  }
  http.end();
  return publicIP;
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

String arteASCII(int number)
{
  String numberStr = (number <= 9) ? "0" + String(number) : String(number);
  String asciiArt[5] = {"", "", "", "", ""};
  for (size_t i = 0; i < numberStr.length(); i++)
  {
    int digit = numberStr[i] - '0';
    String digitArt = digitosAscii[digit];
    int lineIndex = 0;
    for (size_t j = 0; j < digitArt.length(); j++)
    {
      if (digitArt[j] == '\n')
      {
        lineIndex++;
      }
      else
      {
        asciiArt[lineIndex] += digitArt[j];
      }
    }
    for (int j = 0; j < 5; j++)
    {
      asciiArt[j] += "  ";
    }
  }
  String finalArt = "";
  for (int i = 0; i < 5; i++)
  {
    finalArt += asciiArt[i] + "\n";
  }
  return finalArt;
}

String generarBarraHash(float hashrate)
{
  if (hashrate > MAX_HASHRATE)
    hashrate = MAX_HASHRATE;
  if (hashrate < 0)
    hashrate = 0;
  int filled_blocks = (hashrate / MAX_HASHRATE) * BAR_LENGTH;
  String bar = "[";
  for (int i = 0; i < filled_blocks; i++)
    bar += "█";
  for (int i = filled_blocks; i < BAR_LENGTH; i++)
    bar += "-";
  bar += "] ";
  bar += String(hashrate, 2);
  bar += " KH/s";
  return bar;
}

int numSemana(time_t t)
{
  struct tm *timeinfo = localtime(&t);
  return (timeinfo->tm_yday / 7) + 1;
}

void fiestaLED()
{
  for (int i = 0; i < 5; i++)
  {
    digitalWrite(m8ax, HIGH);
    vTaskDelay(pdMS_TO_TICKS(25));
    digitalWrite(m8ax, LOW);
    vTaskDelay(pdMS_TO_TICKS(25));
    totalparpadeosled++;
  }
  digitalWrite(m8ax, HIGH);
  vTaskDelay(pdMS_TO_TICKS(100));
  digitalWrite(m8ax, LOW);
  vTaskDelay(pdMS_TO_TICKS(50));
  digitalWrite(m8ax, HIGH);
  vTaskDelay(pdMS_TO_TICKS(50));
  digitalWrite(m8ax, LOW);
  vTaskDelay(pdMS_TO_TICKS(100));
  digitalWrite(m8ax, HIGH);
  vTaskDelay(pdMS_TO_TICKS(200));
  digitalWrite(m8ax, LOW);
  vTaskDelay(pdMS_TO_TICKS(100));
  digitalWrite(m8ax, HIGH);
  vTaskDelay(pdMS_TO_TICKS(100));
  digitalWrite(m8ax, LOW);
  totalparpadeosled += 4;
}

void sincroLED()
{
  for (int i = 0; i <= 50; i++)
  {
    digitalWrite(m8ax, HIGH);
    vTaskDelay(pdMS_TO_TICKS(25));
    digitalWrite(m8ax, LOW);
    vTaskDelay(pdMS_TO_TICKS(25));
    totalparpadeosled++;
  }
  enviados = data.completedShares;
}

void sincroLED2()
{
  for (int i = 0; i <= 20; i++)
  {
    digitalWrite(m8ax, HIGH);
    vTaskDelay(pdMS_TO_TICKS(25));
    digitalWrite(m8ax, LOW);
    vTaskDelay(pdMS_TO_TICKS(25));
    totalparpadeosled++;
  }
}

void sincroLED3()
{
  for (int i = 0; i <= 20; i++)
  {
    if (i == 10)
    {
      vTaskDelay(pdMS_TO_TICKS(2000));
    }
    digitalWrite(m8ax, HIGH);
    vTaskDelay(pdMS_TO_TICKS(25));
    digitalWrite(m8ax, LOW);
    vTaskDelay(pdMS_TO_TICKS(25));
    totalparpadeosled++;
  }
}

double calcularKilometrosPorSegundo(double hashrateKH)
{
  const double caracteresPorHash = 64.0;
  const double anchoCaracterMm = 2.5;
  double hashrateHS = hashrateKH * 1000.0;
  double caracteresPorSegundo = hashrateHS * caracteresPorHash;
  double longitudMmPorSegundo = caracteresPorSegundo * anchoCaracterMm;
  double longitudKmPorSegundo = longitudMmPorSegundo / 1e6;
  return longitudKmPorSegundo;
}

String ABinario(int num)
{
  String binary = "";
  if (num == 0)
  {
    return "0";
  }
  while (num > 0)
  {
    binary = (num % 2 == 0 ? "0" : "1") + binary;
    num /= 2;
  }
  return binary;
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
    return "Cien";
  if (num == 0)
    return "Cero";
  String resultado = "";
  if (esDecimal && num < 10)
  {
    resultado += "Cero ";
  }
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
    num %= 1000;
    if (num > 0)
      resultado += " ";
  }
  if (num >= 100)
  {
    resultado += capitalizar(centenas[num / 100]);
    num %= 100;
    if (num > 0)
      resultado += " ";
  }
  if (num >= 10 && num <= 19)
  {
    resultado += capitalizar(especiales[num - 10]);
  }
  else
  {
    if (num >= 20)
    {
      if (num < 30)
      {
        resultado += "Veinti";
        if (num % 10 > 0)
          resultado += unidades[num % 10];
      }
      else
      {
        resultado += capitalizar(decenas[num / 10]);
        if (num % 10 > 0)
          resultado += " Y " + capitalizar(unidades[num % 10]);
      }
    }
    else if (num > 0)
    {
      resultado += capitalizar(unidades[num]);
    }
  }
  return resultado;
}

int obtenerDiasEnMes(int annio, int mes)
{
  if (mes == 2)
  {
    if ((annio % 4 == 0 && annio % 100 != 0) || (annio % 400 == 0))
    {
      return 29;
    }
    else
    {
      return 28;
    }
  }
  return (mes == 4 || mes == 6 || mes == 9 || mes == 11) ? 30 : 31;
}

int obtenerDiaSemanaInicio(int annio, int mes)
{
  if (mes < 3)
  {
    mes += 12;
    annio -= 1;
  }
  int k = annio % 100;
  int j = annio / 100;
  int dia = 1;
  int h = (dia + (13 * (mes + 1)) / 5 + k + (k / 4) + (j / 4) + (5 * j)) % 7;
  return (h + 5) % 7;
}

String generarCalendario(int annio, int mes)
{
  String calendario;
  const char *diasSemana[] = {"Lun", "Mar", "Mié", "Jue", "Vie", "Sáb", "Dom"};
  int diasEnMes = obtenerDiasEnMes(annio, mes);
  int diaInicio = obtenerDiaSemanaInicio(annio, mes);
  calendario += "\nM8AX - Calendario -> " + String(meses[mes - 1]) + " / " + String(annio) + "\n\n";
  for (int i = 0; i < 7; i++)
  {
    calendario += String(diasSemana[i]) + "\t";
  }
  calendario += "\n";
  for (int i = 0; i < diaInicio; i++)
  {
    calendario += "\t";
  }
  for (int dia = 1; dia <= diasEnMes; dia++)
  {
    calendario += String(dia) + "\t";
    diaInicio++;
    if (diaInicio == 7)
    {
      calendario += "\n";
      diaInicio = 0;
    }
  }
  calendario += (diaInicio != 0) ? "\n" : "";
  return calendario;
}

const char *convertirTiempoNoMinando(uint32_t segundos)
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

void find_operations(int numbers[], int target, State current, State *best, int depth, int used[])
{
  if (current.value == target)
  {
    *best = current;
    return;
  }
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
  for (int i = 0; i < MAX_NUMBERS; i++)
  {
    if (!used[i])
    {
      used[i] = 1;
      int new_value = current.value + numbers[i];
      if (new_value >= 0)
      {
        State new_state;
        new_state.value = new_value;
        snprintf(new_state.operation, sizeof(new_state.operation), "%s -> (%d + %d) = %d", current.operation, current.value, numbers[i], new_value);
        find_operations(numbers, target, new_state, best, depth + 1, used);
      }
      if (current.value - numbers[i] >= 0)
      {
        State new_state;
        new_state.value = current.value - numbers[i];
        snprintf(new_state.operation, sizeof(new_state.operation), "%s -> (%d - %d) = %d", current.operation, current.value, numbers[i], new_state.value);
        find_operations(numbers, target, new_state, best, depth + 1, used);
      }
      int mult_value = current.value * numbers[i];
      if (mult_value >= 0 && mult_value <= target + 100)
      {
        State new_state;
        new_state.value = mult_value;
        snprintf(new_state.operation, sizeof(new_state.operation), "%s -> (%d * %d) = %d", current.operation, current.value, numbers[i], mult_value);
        find_operations(numbers, target, new_state, best, depth + 1, used);
      }
      if (numbers[i] != 0 && current.value % numbers[i] == 0)
      {
        State new_state;
        new_state.value = current.value / numbers[i];
        snprintf(new_state.operation, sizeof(new_state.operation), "%s -> (%d / %d) = %d", current.operation, current.value, numbers[i], new_state.value);
        find_operations(numbers, target, new_state, best, depth + 1, used);
      }
      used[i] = 0;
    }
  }
}

void calculate_operations(int numbers[], int target, char *result)
{
  int used[MAX_NUMBERS] = {0};
  State best = {numbers[0], ""};
  for (int i = 0; i < MAX_NUMBERS; i++)
  {
    State current;
    current.value = numbers[i];
    snprintf(current.operation, sizeof(current.operation), "Inicio - %d", numbers[i]);
    memset(used, 0, sizeof(used));
    used[i] = 1;
    find_operations(numbers, target, current, &best, 1, used);
  }
  snprintf(result, 500, "%s\n>>> M8AX - Resultado - %d", best.operation, best.value);
  if (totalci == 0)
  {
    porcentaje = (static_cast<float>(aciertos) * 100) / 1;
  }
  else
  {
    porcentaje = (static_cast<float>(aciertos) * 100) / totalci;
  }
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

void generate_random_numbers(int numbers[], int size, int min, int max)
{
  for (int i = 0; i < size; i++)
  {
    numbers[i] = min + (esp_random() % (max - min + 1));
  }
}

void convertirTiempo(const char *input, char *output)
{
  int dias, horas, minutos, segundos;
  sscanf(input, "%d %d:%d:%d", &dias, &horas, &minutos, &segundos);
  sprintf(output, "%02dd %02dh %02dm %02ds", dias, horas, minutos, segundos);
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
  int diaSemana = timeStruct.tm_wday;
  return diasem[diaSemana];
}

void sincronizarTiempo()
{
  int offset = Settings.Timezone * 3600;
  configTime(offset, 0, "pool.ntp.org", "time.nist.gov");
  Serial.println("M8AX - Esperando Sincronización Con NTP...");
  time_t now;
  while ((now = time(nullptr)) < 100000)
  {
    Serial.print(".");
    delay(200);
  }
  Serial.println("M8AX - Hora Sincronizada Correctamente...");
  if (cuenta <= 5)
  {
    sincroLED();
  }
}

const char *FactorizaM8AX(uint32_t number)
{
  char buffer[20];
  uint32_t factors[16];
  uint8_t exponents[16];
  uint8_t count = 0;
  if (number % 2 == 0)
  {
    factors[count] = 2;
    exponents[count] = 0;
    while (number % 2 == 0)
    {
      number >>= 1;
      exponents[count]++;
    }
    count++;
  }
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
  if (number > 1)
  {
    factors[count] = number;
    exponents[count] = 1;
    count++;
  }
  result[0] = '\0';
  for (uint8_t i = 0; i < count; i++)
  {
    if (i > 0)
    {
      strcat(result, " * ");
    }
    if (exponents[i] == 1)
    {
      sprintf(buffer, "%u", factors[i]);
    }
    else
    {
      sprintf(buffer, "%ue%u", factors[i], exponents[i]);
    }
    strcat(result, buffer);
  }
  if (count == 1 && exponents[0] == 1)
  {
    strcat(result, " ( PRIMO )");
  }
  return result;
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

void enviarMensajeATelegram(String mensaje)
{
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("M8AX - Reconectando WiFi...");
    WiFi.disconnect();
    WiFi.reconnect();
    delay(5000);
    if (WiFi.status() != WL_CONNECTED)
    {
      Serial.println("M8AX - No Se Pudo Reconectar WiFi...");
      return;
    }
  }
  WiFiClientSecure client;
  client.setInsecure();
  String mensajeCodificado = urlEncode(mensaje);
  mensajeCodificado.reserve(mensajeCodificado.length() + 100);
  mensaje = "";
  mensaje.reserve(0);
  mensajeCodificado.replace("\n", "%0A");
  Serial.println("M8AX - Enviando Mensaje A Telegram...");
  String url = "https://api.telegram.org/bot" + BOT_TOKEN + "/sendMessage?chat_id=" + CHAT_ID + "&text=" + mensajeCodificado;
  url.reserve(url.length() + 50);
  mensajeCodificado = "";
  mensajeCodificado.reserve(0);
  if (client.connect("api.telegram.org", 443))
  {
    client.print(String("GET ") + url + " HTTP/1.1\r\n" + "Host: api.telegram.org\r\n" + "Connection: close\r\n\r\n");
    delay(1500);
    Serial.println("M8AX - Mensaje Enviado A Telegram...");
    client.flush();
    client.stop();
    sumatele += 1;
    url = "";
    url.reserve(0);
    sincroLED2();
  }
  else
  {
    Serial.println("M8AX - Error De Conexión, Mensaje A Telegram Falló...");
    client.flush();
    client.stop();
    url = "";
    url.reserve(0);
    sincroLED3();
  }
}

void recopilaTelegram()
{
  int horas, minutos, segundos, dia, mes, anio;
  int indice = esp_random() % 5;
  time_t now;
  struct tm timeinfo;
  time(&now);
  localtime_r(&now, &timeinfo);
  horas = timeinfo.tm_hour;
  minutos = timeinfo.tm_min;
  segundos = timeinfo.tm_sec;
  dia = timeinfo.tm_mday;
  mes = timeinfo.tm_mon + 1;
  anio = timeinfo.tm_year + 1900;
  tiempoTranscurrido = millis() - tiempoInicio;
  char horaFormateada[9];
  char fechaFormateada[11];
  sprintf(horaFormateada, "%02d:%02d:%02d", horas, minutos, segundos);
  sprintf(fechaFormateada, "%02d/%02d/%04d", dia, mes, anio);
  uint8_t mac[6];
  WiFi.macAddress(mac);
  char u4digits[5];
  sprintf(u4digits, "%02X%02X", mac[4], mac[5]);
  std::string quediase = obtenerDiaSemana(std::string(fechaFormateada));
  String cadenaEnvio;
  cadenaEnvio.reserve(6000);
  cadenaEnvio = "";
  cadenaEnvio = F("--------------------------------------------------------------------------------------------------\n");
  cadenaEnvio += "--------------------- M8AX - PLACA-WROOM-ESP32D-" + String(u4digits) + " DATOS DE MINERÍA - M8AX ---------------------\n";
  cadenaEnvio += "---------------------- " + String(fechaFormateada) + " " + quediase.c_str() + " - " + horaFormateada + " - % Luna Visible - " + String(porcentajeTexto) + " ----------------------\n";
  cadenaEnvio += F("--------------------------------------------------------------------------------------------------\n");
  quediase.clear();
  quediase.shrink_to_fit();
  String numdesemana = convertirARomanos(numSemana(now));
  if (sumatele <= 3999)
  {
    cadenaEnvio += "Mensaje Número - " + convertirARomanos(sumatele) + " | Semana Del Año Número - " + numdesemana + "\n";
  }
  else
  {
    cadenaEnvio += "Mensaje Número - " + String(sumatele) + " | Semana Del Año Número - " + numdesemana + "\n";
  }
  String horaFinal = horaEnIngles(horas, minutos, segundos);
  cadenaEnvio += "Hora En Inglés - " + horaFinal + "\n";
  horaFinal = "";
  horaFinal.reserve(0);
  numdesemana.reserve(0);
  cadenaEnvio += "Señal WiFi ( RSSI ) -> " + String(WiFi.RSSI()) + " dBm -> ( " + evaluarRSSI(String(WiFi.RSSI())) + " ) -> ";
  cadenaEnvio += "Canal WiFi - " + String(WiFi.channel()) + "\n";
  char *hostname = strdup(WiFi.getHostname());
  cadenaEnvio += String("HostName - ((( --- ") + strupr(hostname) + " --- )))\n";
  free(hostname);
  cadenaEnvio += String("Dir.MAC - ((( --- ") + WiFi.macAddress().c_str() + " --- )))\n";
  cadenaEnvio += String("IP Pública - ((( --- ") + ipPublica + " --- )))\n";
  cadenaEnvio += String("IP Local - ((( --- ") + WiFi.localIP().toString() + " --- )))\n";
  float Tops = ((data.currentHashRate.toFloat() * 1000) * 2560) / 1000000000;
  char tbuffer[10];
  dtostrf(Tops, 8, 6, tbuffer);
  cadenaEnvio += "Modelo Del Chip - " + String(ESP.getChipModel()) + ", " + String(ESP.getCpuFreqMHz()) + " Mhz, " + String(ESP.getChipCores()) + " Núcleos, ((( " + String(tbuffer) + " RT.TOPS )))\n";
  if (tiempoTranscurrido > 0)
  {
    parpadeosPorSegundo = (double)totalparpadeosled / ((double)tiempoTranscurrido / 1000.0);
  }
  cadenaEnvio += "M.RAM Libre - " + String(ESP.getFreeHeap()) + " Bytes\n";
  cadenaEnvio += "Parpadeos Del Led - " + String(totalparpadeosled) + " | Media De Parpadeos/s - " + String(parpadeosPorSegundo, 4) + "\n";
  cadenaEnvio += "Precio De BTC - " + String(precioDeBTC, 2) + " USD | En 2H -> " + subebaja + " | En 24H -> " + subebaja2 + "\n";
  char output[50];
  convertirTiempo(data.timeMining.c_str(), output);
  cadenaEnvio += "Tiempo Minando - " + String(output) + "\n";
  cadenaEnvio += "Tiempo No Minando - ";
  cadenaEnvio += (String(convertirTiempoNoMinando(nominando)).c_str());
  String valor = data.currentHashRate;
  cadenaEnvio += "\nHR Actual - " + valor + " KH/s ( MAX - " + String(maxkh) + " | MIN - " + String(minkh) + " )\n";
  cadenaEnvio += "Barra De Velocidad KH/s - " + generarBarraHash(valor.toFloat()) + "\n";
  int parteEntera = valor.substring(0, valor.indexOf(".")).toInt();
  int parteDecimal = valor.substring(valor.indexOf(".") + 1).toInt();
  cadenaEnvio += "HR Escrito - " + numeroAEscrito(parteEntera, false) + " Con " + numeroAEscrito(parteDecimal, true) + " KH/s\n";
  cadenaEnvio += "HR Binario - " + ABinario(parteEntera) + " Con " + ABinario(parteDecimal) + " KH/s\n";
  cadenaEnvio += "HR Hex - " + String(parteEntera, HEX) + " Con " + String(parteDecimal, HEX) + " KH/s\n";
  cadenaEnvio += "HR Romano - " + convertirARomanos(parteEntera) + " Con " + convertirARomanos(parteDecimal) + " KH/s\n";
  float hrgfloat = data.currentHashRate.toFloat() / 1000000.0;
  float hrtfloat = data.currentHashRate.toFloat() / 1000000000.0;
  float hrpfloat = data.currentHashRate.toFloat() / 1000000000000.0;
  float semanagh = hrgfloat * 60 * 60 * 24 * 7;
  float semanath = hrtfloat * 60 * 60 * 24 * 7;
  float semanaph = hrpfloat * 60 * 60 * 24 * 7;
  cadenaEnvio += "+/- GH/SEM - " + String(semanagh, 2) + " GH/Semana | +/- TH/SEM - " + String(semanath, 2) + " TH/Semana | +/- PH/SEM - " + String(semanaph, 6) + " PH/Semana\n";
  uint8_t entrada[valor.length() + 1];
  memcpy(entrada, valor.c_str(), valor.length() + 1);
  uint8_t salida[32];
  mbedtls_sha256_context ctx;
  mbedtls_sha256_init(&ctx);
  mbedtls_sha256_starts(&ctx, 0);
  mbedtls_sha256_update(&ctx, entrada, sizeof(entrada) - 1);
  mbedtls_sha256_finish(&ctx, salida);
  char sha256String[65];
  for (int i = 0; i < 32; i++)
  {
    sprintf(&sha256String[i * 2], "%02x", salida[i]);
  }
  cadenaEnvio += "HR SHA-256 - " + String(sha256String) + " KH/s";
  mbedtls_sha256_free(&ctx);
  if (valor.toFloat() > 0)
  {
    cadenaEnvio += "\nSi Cada Hash (64 Caracteres), Se Escribiera En Línea Recta Con\n";
    cadenaEnvio += "Tamaño De Letra De Libro, Tu Minero Generaría Texto A - " + String(calcularKilometrosPorSegundo(valor.toFloat())) + " Km/s\n";
    float tiempoLunaSegundos = distanciaLuna / calcularKilometrosPorSegundo(valor.toFloat());
    float tiempoSolSegundos = distanciaSol / calcularKilometrosPorSegundo(valor.toFloat());
    float tiempoDiaSolSegundos = distanciadiamsol / calcularKilometrosPorSegundo(valor.toFloat());
    float tiemporecorrerluna = circumluna / calcularKilometrosPorSegundo(valor.toFloat());
    float litrostotales = ((valor.toFloat() * 1000) * 64) * GOTAAGUA;
    float kmandando = valor.toFloat() * pasoskm;
    float quijoteseg = (valor.toFloat() * 1000 * 64) / 2034611.0;
    float mbteseg = calcularMbPorSegundo(valor.toFloat());
    float tbdia = mbteseg * 86400 / 1024.0 / 1024.0;
    cadenaEnvio += "A La Luna Llegaríamos En - ";
    cadenaEnvio += (String(convertirTiempoNoMinando(tiempoLunaSegundos)).c_str());
    cadenaEnvio += "\nRecorreríamos La Circunferencia De La Luna En - ";
    cadenaEnvio += (String(convertirTiempoNoMinando(tiemporecorrerluna)).c_str());
    cadenaEnvio += "\nAl Sol Llegaríamos En - ";
    cadenaEnvio += (String(convertirTiempoNoMinando(tiempoSolSegundos)).c_str());
    cadenaEnvio += "\nRecorreríamos El Diámetro Del Sol En - ";
    cadenaEnvio += (String(convertirTiempoNoMinando(tiempoDiaSolSegundos)).c_str());
    cadenaEnvio += "\nSi Cada Carácter De Un Hash, Fuera Una Gota De Agua - ";
    cadenaEnvio += String(litrostotales) + " Litros/s";
    cadenaEnvio += "\nLlenaríamos Una Piscina Olímpica En - ";
    cadenaEnvio += (String(convertirTiempoNoMinando((uint32_t)(2500000 / litrostotales))).c_str());
    cadenaEnvio += "\nSi Cada KH/s, Fuera Un Paso, Andarías A - ";
    cadenaEnvio += String(kmandando, 5) + " Km/s | " + String(kmandando * 3600.0, 5) + " Km/h";
    cadenaEnvio += "\nLibros Del Quijote Escritos - ";
    cadenaEnvio += String(quijoteseg, 2) + " QJS/s";
    cadenaEnvio += "\nSi Grabamos Hashes En TxT, La Velocidad Sería - ";
    cadenaEnvio += String(mbteseg, 2) + " MB/s - " + String(tbdia, 2) + " TB/Día";
  }
  eficiencia = data.currentHashRate.toFloat() / consumo;
  float eficiencia_redondeada = round(eficiencia * 1000) / 1000;
  float tempRange = maxtemp - mintemp;
  costo_mensual = consumo * (24 * 30 / 1000.0f) * 0.15;
  cadenaEnvio += "\nEficiencia Energética - ≈ " + String(eficiencia_redondeada, 3) + " KH/s/W - " + String(consumo) + "W - Al Mes - " + String(costo_mensual, 4) + "€\n";
  cadenaEnvio += "Temperatura De CPU - " + data.temp + "° ( MAX - " + String(maxtemp) + "° | MIN - " + String(mintemp) + "° | RANGO - " + String((int)tempRange) + "° - " + interpretarRangoTemperatura(tempRange) + " | TMP>75° - " + String(alertatemp) + " Veces )\n";
  cadenaEnvio += obtenerEstadoTemperatura(data.temp.toInt());
  cadenaEnvio += "Tiempo De CPU A Más De 75° - ";
  cadenaEnvio += (String(convertirTiempoNoMinando(alertatemp)).c_str());
  cadenaEnvio += "\nPlantillas De Bloque - " + data.templates + "\n";
  cadenaEnvio += "Shares Enviados A La Pool - " + data.completedShares + "\n";
  cadenaEnvio += "Mejor Dificultad Alcanzada - " + data.bestDiff + "\n";
  cadenaEnvio += "Cómputo Total - " + data.totalKHashes + " KH - ( " + String(atof(data.totalKHashes.c_str()) / 1000, 3) + " MH )\n";
  cadenaEnvio += "Pool De Minería - " + Settings.PoolAddress + "\n";
  cadenaEnvio += "Puerto Del Pool - " + String(Settings.PoolPort) + "\n";
  cadenaEnvio += "Tu Wallet De BTC - " + String(Settings.BtcWallet) + "\n";
  cadenaEnvio += F("--------------------------------------------------------------------------------------------------\n");
  cadenaEnvio += urlsm8ax[indice];
  cadenaEnvio += F("\n--------------------------------------------------------------------------------------------------\n");
  if (data.valids.toInt() == 1)
  {
    cadenaEnvio += "||| ¡ BLOQUE MINADO ! ¡ A COBRAR ! :) |||\n";
  }
  else
  {
    cadenaEnvio += "||| ¡ SIN PASTA, SIN GLORIA ! ¡ A SEGUIR CON LA HISTORIA ! |||\n";
  }
  cadenaEnvio += F("--------------------------------------------------------------------------------------------------\n");
  cadenaEnvio += F("----------------------------------- M8AX - DATOS NERD - M8AX -------------------------------------\n");
  cadenaEnvio += F("--------------------------------------------------------------------------------------------------\n");
  rndnumero = esp_random();
  cadenaEnvio += "Factorización De Número - 1 - " + String(rndnumero) + " -> " + FactorizaM8AX(rndnumero) + "\n";
  rndnumero = esp_random();
  cadenaEnvio += "Factorización De Número - 2 - " + String(rndnumero) + " -> " + FactorizaM8AX(rndnumero) + "\n";
  rndnumero = esp_random();
  cadenaEnvio += "Factorización De Número - 3 - " + String(rndnumero) + " -> " + FactorizaM8AX(rndnumero) + "\n";
  cadenaEnvio += F("--------------------------------------------------------------------------------------------------\n");
  int numeritos[6];
  int destino = 1 + (esp_random() % 1000);
  generate_random_numbers(numeritos, 6, 1, 100);
  cadenaEnvio += ("Juego De Cifras Número - " + String(totalci) + "\n");
  cadenaEnvio += ("Aciertos - " + String(aciertos) + " | Fallos - " + String(fallos) + "\n");
  cadenaEnvio += ("Números - ");
  for (int i = 0; i < 6; i++)
  {
    cadenaEnvio += (numeritos[i]);
    cadenaEnvio += (" ");
  }
  cadenaEnvio += ("\nObjetivo - " + String(destino) + "\n");
  calculate_operations(numeritos, destino, result);
  String resultadoStr = String(result);
  memset(result, 0, sizeof(result));
  resultadoStr.replace(">>> M8AX - ", "");
  cadenaEnvio += (String(resultadoStr) + "\n");
  cadenaEnvio += "--------------------------------------------------------------------------------------------------\n                                       By M8AX Corp. " + convertirARomanos(anio);
  cadenaEnvio += F("\n--------------------------------------------------------------------------------------------------");
  enviarMensajeATelegram(cadenaEnvio);
  resultadoStr = "";
  resultadoStr.reserve(0);
  cadenaEnvio = "";
  cadenaEnvio.reserve(0);
  valor = "";
  valor.reserve(0);
}

void noDisplay_NoScreen(unsigned long mElapsed)
{
  data = getMiningData(mElapsed);
  int horas, minutos, segundos, dia, mes, anio;
  int temperatura = data.temp.toInt();
  int ganador = data.valids.toInt();
  float hhashrate = data.currentHashRate.toFloat();
  moonData_t moon;
  epochTime = time(nullptr);
  time_t now;
  struct tm timeinfo;
  time(&now);
  localtime_r(&now, &timeinfo);
  timeinfo.tm_isdst = -1;
  time_t cadenaDeTiempo = mktime(&timeinfo);
  moon = mymoonPhase.getPhase(cadenaDeTiempo);
  double porcentajeIluminado = moon.percentLit * 100;
  snprintf(porcentajeTexto, sizeof(porcentajeTexto), "%.3f%%", porcentajeIluminado);
  horas = timeinfo.tm_hour;
  minutos = timeinfo.tm_min;
  segundos = timeinfo.tm_sec;
  dia = timeinfo.tm_mday;
  mes = timeinfo.tm_mon + 1;
  anio = timeinfo.tm_year + 1900;
  cuenta++;
  if (cuenta == 5 && startTime == 0)
  {
    sincronizarTiempo();
    ipPublica = getPublicIP();
    epochTime = time(nullptr);
    startTime = epochTime;
    precioDeBTC = getPrecioBTC();
    anterBTC = precioDeBTC;
    anterBTC2 = precioDeBTC;
    BOT_TOKEN = Settings.botTelegram;
    CHAT_ID = Settings.ChanelIDTelegram;
  }
  cambioDeDia = (cambioDeDia == 0 && cuenta == 7) ? dia : cambioDeDia;
  if (cuenta > 25)
  {
    if (hhashrate > maxkh)
    {
      maxkh = hhashrate;
    }
    if (hhashrate < minkh)
    {
      minkh = hhashrate;
    }
    if (temperatura > maxtemp)
    {
      maxtemp = temperatura;
    }
    if (temperatura < mintemp)
    {
      mintemp = temperatura;
    }
  }
  if (ganador == 0)
  {
    if (hhashrate > 0)
    {
      if (temperatura > 75 && temperatura <= 80)
      {
        digitalWrite(m8ax, HIGH);
        vTaskDelay(pdMS_TO_TICKS((cuenta % 2 == 0) ? 1000 : 1000));
        digitalWrite(m8ax, LOW);
        totalparpadeosled++;
      }
      else
      {
        if (hhashrate > 350)
        {
          if (horas >= 20 || horas < 8)
          {
            digitalWrite(m8ax, HIGH);
            vTaskDelay(pdMS_TO_TICKS((cuenta % 2 == 0) ? 1 : 2));
            digitalWrite(m8ax, LOW);
            vTaskDelay(pdMS_TO_TICKS((cuenta % 2 == 0) ? 200 : 100));
            digitalWrite(m8ax, HIGH);
            vTaskDelay(pdMS_TO_TICKS((cuenta % 2 == 0) ? 1 : 2));
            digitalWrite(m8ax, LOW);
            totalparpadeosled += 2;
          }
          else
          {
            digitalWrite(m8ax, HIGH);
            vTaskDelay(pdMS_TO_TICKS((cuenta % 2 == 0) ? 25 : 50));
            digitalWrite(m8ax, LOW);
            vTaskDelay(pdMS_TO_TICKS((cuenta % 2 == 0) ? 200 : 100));
            digitalWrite(m8ax, HIGH);
            vTaskDelay(pdMS_TO_TICKS((cuenta % 2 == 0) ? 25 : 50));
            digitalWrite(m8ax, LOW);
            totalparpadeosled += 2;
          }
        }
        else
        {
          digitalWrite(m8ax, HIGH);
          vTaskDelay(pdMS_TO_TICKS((cuenta % 2 == 0) ? 25 : 125));
          digitalWrite(m8ax, LOW);
          totalparpadeosled++;
        }
      }
    }
    else
    {
      digitalWrite(m8ax, LOW);
      nominando++;
      Serial.printf(">>> M8AX - La CPU, No Ha Minado Por - %s\n", String(convertirTiempoNoMinando(nominando)).c_str());
    }
  }
  if (ganador != 0)
  {
    if (cuenta % 5 == 0)
    {
      Serial.println("\n>>> M8AX - ERES MILLONARIO - M8AX\n");
    }
    fiestaLED();
  }
  if (cuenta % 30 == 0)
  {
    uint8_t mac[6];
    WiFi.macAddress(mac);
    char u4digits[5];
    sprintf(u4digits, "%02X%02X", mac[4], mac[5]);
    char horaFormateada[9];
    char fechaFormateada[11];
    sprintf(horaFormateada, "%02d:%02d:%02d", horas, minutos, segundos);
    sprintf(fechaFormateada, "%02d/%02d/%04d", dia, mes, anio);
    std::string quediase = obtenerDiaSemana(std::string(fechaFormateada));
    eficiencia = data.currentHashRate.toFloat() / consumo;
    costo_mensual = consumo * (24 * 30 / 1000.0f) * 0.15;
    float eficiencia_redondeada = round(eficiencia * 1000) / 1000;
    String numdesemana = convertirARomanos(numSemana(now));
    tiempoTranscurrido = millis() - tiempoInicio;
    Serial.print("\n-------------------------------------------------------------------------------------------------------------");
    Serial.printf("\n>>> M8AX - Datos Serial Número - %s | PLACA-WROOM-ESP32D-%s\n", String(sumacalen + 1), String(u4digits));
    Serial.printf(">>> M8AX - Fecha - %s %s | Hora - %s | Semana Del Año Número - %s | %% Luna Visible - %s\n", String(fechaFormateada), quediase.c_str(), horaFormateada, numdesemana, String(porcentajeTexto).c_str());
    String horaFinal = horaEnIngles(horas, minutos, segundos);
    Serial.printf(">>> M8AX - Hora En Inglés - %s\n", horaFinal.c_str());
    horaFinal = "";
    horaFinal.reserve(0);
    Serial.printf(">>> M8AX - Señal WiFi ( RSSI ) -> %d dBm -> ( %s ) -> Canal WiFi - %s\n", WiFi.RSSI(), String(evaluarRSSI(String(WiFi.RSSI()))).c_str(), String(WiFi.channel()));
    quediase.clear();
    quediase.shrink_to_fit();
    char *hostname = strdup(WiFi.getHostname());
    Serial.printf(">>> M8AX - HostName - ((( --- %s --- )))\n", strupr(hostname));
    free(hostname);
    Serial.printf(">>> M8AX - Dir.MAC - ((( --- %s --- )))\n", WiFi.macAddress().c_str());
    Serial.printf(">>> M8AX - IP Pública - ((( --- %s --- )))\n", ipPublica);
    Serial.printf(">>> M8AX - IP Local - ((( --- %s --- )))\n", WiFi.localIP().toString());
    float Tops = ((data.currentHashRate.toFloat() * 1000) * 2560) / 1000000000;
    char tbuffer[10];
    dtostrf(Tops, 8, 6, tbuffer);
    Serial.printf(">>> M8AX - Modelo Del Chip - %s, %d MHz, %d Núcleos, ((( %s RT.TOPS )))\n", String(ESP.getChipModel()), ESP.getCpuFreqMHz(), ESP.getChipCores(), String(tbuffer));
    if (tiempoTranscurrido > 0)
    {
      parpadeosPorSegundo = (double)totalparpadeosled / ((double)tiempoTranscurrido / 1000.0);
    }
    Serial.printf(">>> M8AX - M.RAM Libre - %d Bytes | Parpadeos Del Led - %u | Media De Parpadeos/s - %.4f\n", ESP.getFreeHeap(), totalparpadeosled, parpadeosPorSegundo);
    Serial.printf(">>> M8AX - Precio De BTC - %.2f USD | En 2H -> %s | En 24H -> %s\n", precioDeBTC, subebaja.c_str(), subebaja2.c_str());
    Serial.printf(">>> M8AX - Bloques Válidos - %s\n", data.valids.c_str());
    Serial.printf(">>> M8AX - Plantillas De Bloques - %s\n", data.templates.c_str());
    Serial.printf(">>> M8AX - Mejor Dificultad Alcanzada - %s\n", data.bestDiff.c_str());
    Serial.printf(">>> M8AX - Shares Enviados A La Pool - %s\n", data.completedShares.c_str());
    Serial.printf(">>> M8AX - HashRate - %s KH/s\n", String(hhashrate));
    Serial.printf(">>> M8AX - Barra De Velocidad KH/s - %s\n", generarBarraHash(hhashrate).c_str());
    String valor = String(hhashrate);
    int parteEntera = valor.substring(0, valor.indexOf(".")).toInt();
    int parteDecimal = valor.substring(valor.indexOf(".") + 1).toInt();
    Serial.printf(">>> M8AX - HR Escrito - %s Con %s KH/s\n", numeroAEscrito(parteEntera, false).c_str(), numeroAEscrito(parteDecimal, true).c_str());
    Serial.printf(">>> M8AX - HR Binario - %s Con %s KH/s\n", ABinario(parteEntera).c_str(), ABinario(parteDecimal).c_str());
    Serial.printf(">>> M8AX - HR Hex - %s Con %s KH/s\n", String(parteEntera, HEX).c_str(), String(parteDecimal, HEX).c_str());
    Serial.printf(">>> M8AX - HR Romano - %s Con %s KH/s\n", convertirARomanos(parteEntera).c_str(), convertirARomanos(parteDecimal).c_str());
    float hrgfloat = data.currentHashRate.toFloat() / 1000000.0;
    float hrtfloat = data.currentHashRate.toFloat() / 1000000000.0;
    float hrpfloat = data.currentHashRate.toFloat() / 1000000000000.0;
    float semanagh = hrgfloat * 60 * 60 * 24 * 7;
    float semanath = hrtfloat * 60 * 60 * 24 * 7;
    float semanaph = hrpfloat * 60 * 60 * 24 * 7;
    Serial.printf(">>> M8AX - +/- GH/SEM - %s GH/Semana | +/- TH/SEM - %s TH/Semana | +/- PH/SEM - %s PH/Semana\n", String(semanagh, 2), String(semanath, 2), String(semanaph, 6));
    uint8_t entrada[valor.length() + 1];
    memcpy(entrada, valor.c_str(), valor.length() + 1);
    uint8_t salida[32];
    mbedtls_sha256_context ctx;
    mbedtls_sha256_init(&ctx);
    mbedtls_sha256_starts(&ctx, 0);
    mbedtls_sha256_update(&ctx, entrada, sizeof(entrada) - 1);
    mbedtls_sha256_finish(&ctx, salida);
    char sha256String[65];
    for (int i = 0; i < 32; i++)
    {
      sprintf(&sha256String[i * 2], "%02x", salida[i]);
    }
    Serial.printf(">>> M8AX - HR SHA-256 - %s KH/s\n", String(sha256String).c_str());
    mbedtls_sha256_free(&ctx);
    Serial.printf(">>> M8AX - Max HashRate - %s KH/s\n", String(maxkh));
    Serial.printf(">>> M8AX - Min HashRate - %s KH/s\n", String(minkh));
    if (hhashrate > 0)
    {
      Serial.println(">>> M8AX - Si Cada Hash (64 Caracteres), Se Escribiera En Línea Recta Con");
      Serial.printf(">>> M8AX - Tamaño De Letra De Libro, Tu Minero Generaría Texto A - %s Km/s\n", String(calcularKilometrosPorSegundo(valor.toFloat())));
      float tiempoLunaSegundos = distanciaLuna / calcularKilometrosPorSegundo(hhashrate);
      float tiempoSolSegundos = distanciaSol / calcularKilometrosPorSegundo(hhashrate);
      float tiempoDiaSolSegundos = distanciadiamsol / calcularKilometrosPorSegundo(hhashrate);
      float tiemporecorrerluna = circumluna / calcularKilometrosPorSegundo(hhashrate);
      float litrostotales = ((valor.toFloat() * 1000) * 64) * GOTAAGUA;
      float kmandando = valor.toFloat() * pasoskm;
      float quijoteseg = (hhashrate * 1000 * 64) / 2034611.0;
      float mbteseg = calcularMbPorSegundo(valor.toFloat());
      float tbdia = mbteseg * 86400 / 1024.0 / 1024.0;
      Serial.printf(">>> M8AX - A La Luna Llegaríamos En - %s\n", String(convertirTiempoNoMinando(tiempoLunaSegundos)).c_str());
      Serial.printf(">>> M8AX - Recorreríamos La Circunferencia De La Luna En - %s\n", String(convertirTiempoNoMinando(tiemporecorrerluna)).c_str());
      Serial.printf(">>> M8AX - Al Sol Llegaríamos En - %s\n", String(convertirTiempoNoMinando(tiempoSolSegundos)).c_str());
      Serial.printf(">>> M8AX - Recorreríamos El Diámetro Del Sol En - %s\n", String(convertirTiempoNoMinando(tiempoDiaSolSegundos)).c_str());
      Serial.printf(">>> M8AX - Si Cada Carácter De Un Hash, Fuera Una Gota De Agua - %s Litros/s\n", String(litrostotales).c_str());
      Serial.printf(">>> M8AX - Llenaríamos Una Piscina Olímpica En - %s\n", (String(convertirTiempoNoMinando((uint32_t)(2500000 / litrostotales))).c_str()));
      Serial.printf(">>> M8AX - Si Cada KH/s, Fuera Un Paso, Andarías A - %s Km/s | %s Km/h\n", String(kmandando, 5).c_str(), String(kmandando * 3600.0, 5).c_str());
      Serial.printf(">>> M8AX - Libros Del Quijote Escritos - %.2f QJS/s\n", quijoteseg);
      Serial.printf(">>> M8AX - Si Grabamos Hashes En TxT, La Velocidad Sería - %.2f MB/s - %.2f TB/Día\n", mbteseg, tbdia);
    }
    Serial.printf(">>> M8AX - Eficiencia Energética - ≈ %.3f KH/s/W - %sW - Al Mes - %s€\n", eficiencia_redondeada, String(consumo), String(costo_mensual, 4));
    Serial.printf(">>> M8AX - %sTemperatura - %s°\n", obtenerEstadoTemperatura(data.temp.toInt()), data.temp.c_str());
    float tempRange = maxtemp - mintemp;
    Serial.printf(">>> M8AX - Max Temperatura - %s°\n", String(maxtemp));
    Serial.printf(">>> M8AX - Min Temperatura - %s°\n", String(mintemp));
    Serial.printf(">>> M8AX - Rango De Temperatura - %d° - %s\n", (int)tempRange, interpretarRangoTemperatura(tempRange));
    Serial.printf(">>> M8AX - Temperatura A Más De 75° - %s Veces\n", String(alertatemp));
    Serial.printf(">>> M8AX - Tiempo De CPU A Más De 75° - %s\n", String(convertirTiempoNoMinando(alertatemp)).c_str());
    Serial.printf(">>> M8AX - Cómputo Total ( MH ) - %s\n", String(atof(data.totalKHashes.c_str()) / 1000, 3));
    char output[50];
    convertirTiempo(data.timeMining.c_str(), output);
    Serial.printf(">>> M8AX - Tiempo Minando - %s\n", output);
    Serial.printf(">>> M8AX - Tiempo No Minando - %s\n", String(convertirTiempoNoMinando(nominando)).c_str());
    int numeritos[6];
    int destino = 1 + (esp_random() % 1000);
    generate_random_numbers(numeritos, 6, 1, 100);
    Serial.print("-------------------------------------------------------------------------------------------------------------\n");
    Serial.print(">>> M8AX - MEGA NERD - M8AX\n");
    Serial.print("-------------------------------------------------------------------------------------------------------------\n");
    rndnumero = esp_random();
    Serial.print(">>> M8AX - Factorización De Número - 1 - " + String(rndnumero) + " -> " + FactorizaM8AX(rndnumero) + "\n");
    rndnumero = esp_random();
    Serial.print(">>> M8AX - Factorización De Número - 2 - " + String(rndnumero) + " -> " + FactorizaM8AX(rndnumero) + "\n");
    rndnumero = esp_random();
    Serial.print(">>> M8AX - Factorización De Número - 3 - " + String(rndnumero) + " -> " + FactorizaM8AX(rndnumero) + "\n");
    Serial.print("-------------------------------------------------------------------------------------------------------------\n");
    rndnumero = esp_random();
    Serial.print(">>> M8AX - El Número - 1 - " + String(rndnumero) + " -> " + abinyprimo(rndnumero) + "\n");
    rndnumero = esp_random();
    Serial.print(">>> M8AX - El Número - 2 - " + String(rndnumero) + " -> " + abinyprimo(rndnumero) + "\n");
    rndnumero = esp_random();
    Serial.print(">>> M8AX - El Número - 3 - " + String(rndnumero) + " -> " + abinyprimo(rndnumero) + "\n");
    Serial.print("-------------------------------------------------------------------------------------------------------------\n");
    Serial.print(">>> M8AX - Juego De Cifras Número - " + String(totalci) + "\n");
    Serial.print(">>> M8AX - Aciertos - " + String(aciertos) + " | Fallos - " + String(fallos) + "\n");
    Serial.print(">>> M8AX - Números - ");
    for (int i = 0; i < 6; i++)
    {
      Serial.print(numeritos[i]);
      Serial.print(" ");
    }
    Serial.println();
    Serial.print(">>> M8AX - Objetivo - " + String(destino) + "\n");
    calculate_operations(numeritos, destino, result);
    Serial.print(">>> M8AX - " + String(result) + "\n");
    memset(result, 0, sizeof(result));
    Serial.print("-------------------------------------------------------------------------------------------------------------\n");
    for (int i = 1; i <= 12; i++)
    {
      String calendario = generarCalendario(anio + sumacalen, i);
      Serial.println(calendario);
      Serial.print("\n-------------------------------------------------------------------------------------------------------------\n");
      calendario.clear();
      calendario.reserve(0);
    }
    Serial.print("\n");
    sumacalen++;
    Serial.print("---------------------------------------------KH/S-HASHRATE-ASCII---------------------------------------------\n\n");
    Serial.print(arteASCII(parteEntera));
    Serial.print("\n       PUNTO       \n\n");
    Serial.print(arteASCII(parteDecimal));
    Serial.print("\n-------------------------------------------------------------------------------------------------------------");
    Serial.print("\n\n-------------------------------------------------------------------------------------------------------------\n\n");
  }
  String fechaHora = String(timeinfo.tm_hour < 10 ? "0" : "") + String(timeinfo.tm_hour) + ":" +
                     String(timeinfo.tm_min < 10 ? "0" : "") + String(timeinfo.tm_min) + ":" +
                     String(timeinfo.tm_sec < 10 ? "0" : "") + String(timeinfo.tm_sec);
  Serial.printf("%s >>> M8AX - Completados %s Share(s), %s Khashes, Med. HashRate %s KH/s - %s°\n",
                fechaHora.c_str(), data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str(), data.temp.c_str());
  fechaHora.clear();
  fechaHora.reserve(0);
  if (minutos == 30 && segundos == 0)
  {
    digitalWrite(m8ax, HIGH);
    vTaskDelay(pdMS_TO_TICKS(500));
    digitalWrite(m8ax, LOW);
    vTaskDelay(pdMS_TO_TICKS(500));
    precioDeBTC = getPrecioBTC();
    totalparpadeosled++;
  }
  if (minutos == 0 && segundos == 0)
  {
    digitalWrite(m8ax, HIGH);
    vTaskDelay(pdMS_TO_TICKS(400));
    digitalWrite(m8ax, LOW);
    vTaskDelay(pdMS_TO_TICKS(150));
    digitalWrite(m8ax, HIGH);
    vTaskDelay(pdMS_TO_TICKS(400));
    digitalWrite(m8ax, LOW);
    vTaskDelay(pdMS_TO_TICKS(500));
    totalparpadeosled += 2;
  }
  if (data.completedShares != enviados && cuenta > 10)
  {
    enviados = data.completedShares;
    Serial.println("M8AX - Enviando Share A La Pool...");
    for (int a = 0; a <= 5; a++)
    {
      digitalWrite(m8ax, HIGH);
      vTaskDelay(pdMS_TO_TICKS(125));
      digitalWrite(m8ax, LOW);
      vTaskDelay(pdMS_TO_TICKS(300));
      totalparpadeosled++;
    }
  }
  if (temperatura > 75)
  {
    Serial.println("M8AX - La Temperatura De La CPU Ha Superado Los 75°C...");
    alertatemp++;
    if (temperatura > 80)
    {
      Serial.println("M8AX - ¡ Temperatura Muy Alta ! 80°C Superados. Entrando En Deep Sleep Por 10 Minutos Para Enfriar La CPU...");
      esp_sleep_enable_timer_wakeup(600e6);
      esp_deep_sleep_start();
    }
  }
  if (cambioDeDia != dia && cuenta > 10)
  {
    precioDeBTC = getPrecioBTC();
    cambioDeDia = dia;
    float variacion2 = (anterBTC2 > 0) ? ((precioDeBTC - anterBTC2) / anterBTC2) * 100 : 0;
    subebaja2 = (anterBTC2 > 0 && precioDeBTC > 0) ? ((variacion2 >= 0) ? "+" : "-") + String(fabs(variacion2), 5) + "%" : "... ERROR ...";
    anterBTC2 = precioDeBTC;
  }
  if (epochTime - startTime >= minStartupTime && epochTime - lastTelegramEpochTime >= interval)
  {
    sincronizarTiempo();
    vTaskDelay(pdMS_TO_TICKS(250));
    ipPublica = getPublicIP();
    vTaskDelay(pdMS_TO_TICKS(250));
    precioDeBTC = getPrecioBTC();
    vTaskDelay(pdMS_TO_TICKS(250));
    float variacion = (anterBTC > 0) ? ((precioDeBTC - anterBTC) / anterBTC) * 100 : 0;
    subebaja = (anterBTC > 0 && precioDeBTC > 0) ? ((variacion >= 0) ? "+" : "-") + String(fabs(variacion), 5) + "%" : "... ERROR ...";
    if (BOT_TOKEN != "NO CONFIGURADO" && CHAT_ID != "NO CONFIGURADO")
    {
      digitalWrite(m8ax, HIGH);
      totalparpadeosled++;
      vTaskDelay(pdMS_TO_TICKS(2000));
      if (cuenta > 30)
      {
        recopilaTelegram();
      }
      lastTelegramEpochTime = epochTime;
    }
    anterBTC = precioDeBTC;
  }
}

void noDisplay_LoadingScreen(void)
{
  pinMode(m8ax, OUTPUT);
  tiempoInicio = millis();
  Serial.println("\n... M8AX - ARRANCANDO - M8AX ...\n\n... M8AX - SALUDO EN MORSE - M8AX ...\n");
  for (int i = 0; i < morseLength; i++)
  {
    const char *letra = morse[i];
    for (int j = 0; letra[j] != '\0'; j++)
    {
      if (letra[j] == '.')
      {
        Serial.print(".");
        digitalWrite(m8ax, HIGH);
        vTaskDelay(pdMS_TO_TICKS(200));
        digitalWrite(m8ax, LOW);
        vTaskDelay(pdMS_TO_TICKS(200));
        totalparpadeosled++;
      }
      else if (letra[j] == '-')
      {
        Serial.print("-");
        digitalWrite(m8ax, HIGH);
        vTaskDelay(pdMS_TO_TICKS(600));
        digitalWrite(m8ax, LOW);
        vTaskDelay(pdMS_TO_TICKS(200));
        totalparpadeosled++;
      }
    }
    Serial.print(" ");
    vTaskDelay(pdMS_TO_TICKS(600));
    if (i == 3)
    {
      Serial.print("   ");
      vTaskDelay(pdMS_TO_TICKS(1500));
    }
  }
  Serial.print("\n\n");
  vTaskDelay(pdMS_TO_TICKS(500));
}

void noDisplay_SetupScreen(void)
{
  if (solouna == 0)
  {
    solouna++;
    Serial.println("-----------------------------------------------------------------");
    Serial.println("... M8AX - PANTALLA DE CONFIGURACIÓN ...");
    Serial.println("-----------------------------------------------------------------");
    Serial.println("... M8AX - GRÁCIAS POR USAR MI FIRMWARE ...");
    Serial.println("... M8AX - NO TE OLVIDES DE VISITAR MI CANAL DE YOUTUBE ...");
    Serial.println("... M8AX - HTTPS://YOUTUBE.COM/M8AX ...");
    Serial.println("-----------------------------------------------------------------");
    for (int i = 1; i <= 30; i++)
    {
      digitalWrite(m8ax, HIGH);
      vTaskDelay(pdMS_TO_TICKS((esp_random() % 50) + 1));
      digitalWrite(m8ax, LOW);
      vTaskDelay(pdMS_TO_TICKS((esp_random() % 200) + 1));
      totalparpadeosled++;
    }
  }
  digitalWrite(m8ax, HIGH);
  totalparpadeosled++;
}

void noDisplay_DoLedStuff(unsigned long frame)
{
}

void noDisplay_AnimateCurrentScreen(unsigned long frame)
{
}

CyclicScreenFunction noDisplayCyclicScreens[] = {noDisplay_NoScreen};

DisplayDriver noDisplayDriver = {
    noDisplay_Init,
    noDisplay_AlternateScreenState,
    noDisplay_AlternateRotation,
    noDisplay_LoadingScreen,
    noDisplay_SetupScreen,
    noDisplayCyclicScreens,
    noDisplay_AnimateCurrentScreen,
    noDisplay_DoLedStuff,
    SCREENS_ARRAY_SIZE(noDisplayCyclicScreens),
    0,
    0,
    0,
};
#endif

/***********************************************************************************************************************************
 *
 *                                                  F   I   N          D   E
 *
 *                                                P   R   O   G   R   A   M   A
 *
 *                                       ¡   H   A   S   T   A          O   T   R   A   !
 *
 ***********************************************************************************************************************************/