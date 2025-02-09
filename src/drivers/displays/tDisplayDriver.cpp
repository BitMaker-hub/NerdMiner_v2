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
#include <iostream>
#include <string>
#include <ctime>
#include <clientntp.h>
#include "drivers/storage/storage.h"
#include "drivers/devices/device.h"

extern TSettings Settings;

#define WIDTH 340
#define HEIGHT 170
#define MIN_KH 0    // Valor mínimo de KH/s
#define MAX_KH 360  // Valor máximo de KH/s
#define MAX_RESULT_LENGTH 500
#define MAX_NUMBERS 6
#define MAX_DEPTH 4  // Limitar la profundidad de búsqueda para mejorar el rendimiento
#define MAX_GEN_COUNT 1000
#define GRIDX 320
#define GRIDY 170
#define CELLXY 2
#define GEN_DELAY 1

OpenFontRender render;
TFT_eSPI tft = TFT_eSPI();                   // Invoke library, pins defined in User_Setup.h
TFT_eSprite background = TFT_eSprite(&tft);  // Invoke library sprite
uint8_t grid[GRIDX][GRIDY];
uint8_t newgrid[GRIDX][GRIDY];
uint16_t genCount = 0;
uint16_t colors[] = { TFT_WHITE, TFT_RED, TFT_GREEN, TFT_BLUE, TFT_YELLOW, TFT_CYAN, TFT_MAGENTA, TFT_ORANGE, TFT_GREENYELLOW, TFT_PINK, TFT_LIGHTGREY, TFT_SKYBLUE, TFT_OLIVE, TFT_GOLD, TFT_SILVER };
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
int pantallaEncendida=0;
int maxtemp=0;
int mintemp=50000;
float porcentaje = 0.00;
float maxkh=0.00;
float minkh=50000.00;
const char* nombrecillo;
const char* apiUrl = "http://ip-api.com/json/";
const char* serverName = "https://zenquotes.io/api/random";
const char* criptomonedas[] = {
  "BTC-USD",    // Bitcoin
  "ETH-USD",    // Ethereum
  "BNB-USD",    // Binance Coin
  "SOL-USD",    // Solana
  "XRP-USD",    // Ripple
  "ADA-USD",    // Cardano
  "AVAX-USD",   // Avalanche
  "DOGE-USD",   // Dogecoin (Elon Musk)
  "DOT-USD",    // Polkadot
  "LINK-USD",   // Chainlink
  "MATIC-USD",  // Polygon (Matic)
  "ATOM-USD",   // Cosmos
  "HBAR-USD",   // Hedera
  "UNI-USD",    // Uniswap
  "ALGO-USD",   // Algorand
  "FET-USD",    // Fetch.AI
  "NEAR-USD",   // Near Protocol (añadido)
  "APT-USD",    // Aptos (añadido)
  "ARB-USD",    // Arbitrum (añadido)
  "TRUMP-USD"   // TrumpCoin
};
const char* ciudades[] = {
  "Nueva York", "Londres", "Paris", "Tokio", "Sidney",
  "Los Angeles", "Beijing", "Moscu", "Delhi", "Buenos Aires",
  "Berlin", "Mexico", "Madrid", "Seul", "Roma",
  "El Cairo", "Amsterdam", "Toronto", "Sao Paulo", "Cape Town"
};
String urls[] = {
  "https://cointelegraph.com/rss",
  "https://es.cointelegraph.com/rss/tag/altcoin",
  "https://es.cointelegraph.com/rss/category/analysis",
  "https://es.cointelegraph.com/rss/tag/regulation",
  "https://es.cointelegraph.com/rss/tag/bitcoin",
  "https://es.cointelegraph.com/rss/tag/blockchain",
  "https://es.cointelegraph.com/rss/tag/ethereum",
  "https://es.cointelegraph.com/rss/category/top-10-cryptocurrencies",
  "https://es.cointelegraph.com/rss/category/market-analysis"
};
int zonasHorarias[] = {
  -5, 0, 1, 9, 11,  // Nueva York, Londres, Paris, Tokio, Sidney
  -8, 8, 3, 5, -3,  // Los Angeles, Beijing, Moscu, Delhi, Buenos Aires
  1, -6, 1, 9, 1,   // Berlin, Mexico, Madrid, Seul, Roma
  2, 1, -5, -3, 2   // El Cairo, Amsterdam, Toronto, Sao Paulo, Cape Town
};
bool esHorarioDeVerano(int mes, int dia);
bool transicionEjecutada = false;
char result[MAX_RESULT_LENGTH];
String textoFinalm8ax1;
String textoFinalm8ax2;
String textoFinalm8ax3;
String textoFinalm8ax4;
String cadenanoti = "";
String ciudad = "";
String tempciudad = "";
String BOT_TOKEN = Settings.botTelegram;
String CHAT_ID = Settings.ChanelIDTelegram;
uint32_t rndnumero;
uint32_t rndnumero2 = 0;
uint32_t rndnumero3 = 0;
uint32_t actualizarcalen = 0;
uint32_t actuanot = 0;
uint32_t actualizarc = 0;
uint32_t actual = 0;
uint32_t correccion = 0;
uint32_t numfrases = 0;
uint32_t numnotis = 0;
uint32_t ContadorEspecial = 0;
uint32_t sumatelegram=0;
uint32_t tempalert=0;
WiFiUDP udp;
HTTPClient http;
mining_data mineria;
clock_data relojete;
coin_data monedilla;
moonPhase moonPhase;


unsigned long ppreviousMillis = 0;  // Variable para almacenar el tiempo anterior
const long iinterval = 3000;        // Intervalo de 3 segundos (en milisegundos)
bool ppantallaEncendida = true;      // Variable para saber si la pantalla está encendida o apagada

typedef struct {
  int value;
  char operation[500];
} State;

template<typename T, typename T2>

inline T map(T2 val, T2 in_min, T2 in_max, T out_min, T out_max) {
  return (val - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void tDisplay_Init(void) {
  //Init pin 15 to eneble 5V external power (LilyGo bug)
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
  tft.setSwapBytes(true);                  // Swap the colour byte order when rendering
  background.createSprite(WIDTH, HEIGHT);  // Background Sprite
  background.setSwapBytes(true);
  render.setDrawer(background);   // Link drawing object to background instance (so font will be rendered on background)
  render.setLineSpaceRatio(0.9);  // Espaciado entre texto

  // Load the font and check it can be read OK
  // if (render.loadFont(NotoSans_Bold, sizeof(NotoSans_Bold))) {
  if (render.loadFont(DigitalNumbers, sizeof(DigitalNumbers))) {
    Serial.println("Initialise error");
    return;
  }
}

void tDisplay_AlternateScreenState(void) {
  int screen_state = digitalRead(TFT_BL);
  Serial.println("Switching display state");
  digitalWrite(TFT_BL, !screen_state);
  pantallaEncendida=screen_state;
}

void tDisplay_AlternateRotation(void) {
  tft.setRotation(flipRotation(tft.getRotation()));
}

void manejarPantalla() {
  unsigned long currentMillis = millis();  // Obtenemos el tiempo actual
  
  if (currentMillis - ppreviousMillis >= iinterval) {  // Si han pasado 10 segundos
    // Guardamos el tiempo actual
    ppreviousMillis = currentMillis;

    // Apagar o encender la pantalla
    if (ppantallaEncendida) {
      digitalWrite(TFT_BL, LOW);  // Apagar la pantalla
      ppantallaEncendida = false;  // Actualizar el estado
      vTaskDelay(pdMS_TO_TICKS(2000));
    } else {
      digitalWrite(TFT_BL, HIGH);  // Encender la pantalla
      ppantallaEncendida = true;    // Actualizar el estado
      vTaskDelay(pdMS_TO_TICKS(2000));
    }
  }
}

String convertirARomanos(int num) {
  if (num == 0) {
    return "CERO";
  }

  String result = "";
  int values[] = { 1000, 900, 500, 400, 100, 90, 50, 40, 10, 9, 5, 4, 1 };
  String romans[] = { "M", "CM", "D", "CD", "C", "XC", "L", "XL", "X", "IX", "V", "IV", "I" };

  for (int i = 0; i < 13; i++) {
    while (num >= values[i]) {
      result += romans[i];
      num -= values[i];
    }
  }

  return result;
}

std::string obtenerDiaSemana(const std::string& fecha) {
  // Array con los nombres de los días en español, abreviados
  const char* diasem[] = { "Dom", "Lun", "Mar", "Mie", "Jue", "Vie", "Sab" };

  // Extraer día, mes y año de la cadena de fecha
  int dia = std::stoi(fecha.substr(0, 2));      // Primeros 2 caracteres: día
  int mes = std::stoi(fecha.substr(3, 2)) - 1;  // Mes (0-indexado)
  int anio = std::stoi(fecha.substr(6, 4));     // Año

  // Crear una estructura tm para la fecha
  std::tm timeStruct = {};
  timeStruct.tm_mday = dia;          // Día del mes
  timeStruct.tm_mon = mes;           // Mes (0 = enero, 11 = diciembre)
  timeStruct.tm_year = anio - 1900;  // Año desde 1900

  // Convertir a time_t y obtener el día de la semana
  std::mktime(&timeStruct);
  int diaSemana = timeStruct.tm_wday;  // 0 = domingo, 1 = lunes, ...

  // Devolver el nombre del día
  return diasem[diaSemana];
}

String getPublicIP() {
  HTTPClient http;
  String publicIP = "";

  // Realizar la solicitud GET a un servicio que devuelva la IP pública
  http.begin("http://api.ipify.org");  // Servicio que devuelve la IP pública
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    publicIP = http.getString();  // Obtener la respuesta del servidor
  }

  http.end();
  return publicIP;
}

String quitarAcentos(String str) {
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
  str.replace("¿", "");  // Eliminar signos de interrogación al principio de la frase
  str.replace("¡", "");  // Eliminar signos de exclamación al principio de la frase
  return str;
}

void borrarDesdeUltimoGuion(char* str) {
  char* pos = strrchr(str, '-');  // Encuentra el último '-'
  if (pos) {
    *pos = '\0';  // Corta la cadena en ese punto
  }
}

// Función recursiva para encontrar operaciones
void find_operations(int numbers[], int target, State current, State* best, int depth, int used[]) {
  // Si alcanzamos el objetivo, actualizamos el mejor resultado
  if (current.value == target) {
    *best = current;
    return;
  }

  // Si hemos usado todos los números o hemos alcanzado la profundidad máxima, verificamos si este resultado es mejor
  if (depth == MAX_DEPTH) {
    int diff = abs(current.value - target);
    int mejor_diff = abs(best->value - target);
    if (diff < mejor_diff) {
      *best = current;
    }
    return;
  }

  // Iterar sobre todos los números disponibles
  for (int i = 0; i < MAX_NUMBERS; i++) {
    if (!used[i]) {
      used[i] = 1;  // Marcar el número como usado

      // Suma
      int new_value = current.value + numbers[i];
      if (new_value >= 0) {  // No se permiten resultados negativos
        State new_state;
        new_state.value = new_value;
        snprintf(new_state.operation, sizeof(new_state.operation), "%s -> (%d + %d) = %d", current.operation, current.value, numbers[i], new_value);
        find_operations(numbers, target, new_state, best, depth + 1, used);
      }

      // Resta (solo si no genera negativos)
      if (current.value - numbers[i] >= 0) {
        State new_state;
        new_state.value = current.value - numbers[i];
        snprintf(new_state.operation, sizeof(new_state.operation), "%s -> (%d - %d) = %d", current.operation, current.value, numbers[i], new_state.value);
        find_operations(numbers, target, new_state, best, depth + 1, used);
      }

      // Multiplicación (solo si no es innecesario)
      int mult_value = current.value * numbers[i];
      if (mult_value >= 0 && mult_value <= target + 100) {  // Limitar el rango de multiplicación
        State new_state;
        new_state.value = mult_value;
        snprintf(new_state.operation, sizeof(new_state.operation), "%s -> (%d * %d) = %d", current.operation, current.value, numbers[i], mult_value);
        find_operations(numbers, target, new_state, best, depth + 1, used);
      }

      // División exacta (solo si no genera fracciones)
      if (numbers[i] != 0 && current.value % numbers[i] == 0) {
        State new_state;
        new_state.value = current.value / numbers[i];
        snprintf(new_state.operation, sizeof(new_state.operation), "%s -> (%d / %d) = %d", current.operation, current.value, numbers[i], new_state.value);
        find_operations(numbers, target, new_state, best, depth + 1, used);
      }

      used[i] = 0;  // Desmarcar el número
    }
  }
}

void calculate_operations(int numbers[], int target, char* result) {
  int used[MAX_NUMBERS] = { 0 };    // Para rastrear números usados
  State best = { numbers[0], "" };  // Inicializamos con el primer número

  // Empezamos probando con cada número en la lista como punto de inicio
  for (int i = 0; i < MAX_NUMBERS; i++) {
    State current;
    current.value = numbers[i];  // Probar cada número como inicio
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
  porcentaje = (aciertos * 100) / totalci;
  if (best.value == target) {
    aciertos++;
    strcat(result, ", ( EXACTO ) -");
    char porcentaje_str[20];
    snprintf(porcentaje_str, sizeof(porcentaje_str), " ( AC %.2f%% )", porcentaje);
    strcat(result, porcentaje_str);
  } else {
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

void generate_random_numbers(int numbers[], int size, int min, int max) {
  for (int i = 0; i < size; i++) {
    numbers[i] = min + (esp_random() % (max - min + 1));
  }
}

const char* factorize(uint32_t number) {
  char buffer[20];        // Buffer temporal para formatear factores
  uint32_t factors[16];   // Almacena factores primos
  uint8_t exponents[16];  // Almacena exponentes
  uint8_t count = 0;      // Número de factores encontrados

  // Factorización por 2
  if (number % 2 == 0) {
    factors[count] = 2;
    exponents[count] = 0;
    while (number % 2 == 0) {
      number >>= 1;  // Dividir por 2 usando desplazamiento de bits
      exponents[count]++;
    }
    count++;
  }

  // Factorización por divisores impares
  uint32_t sqrt_num = sqrt(number);
  for (uint32_t divisor = 3; divisor <= sqrt_num; divisor += 2) {
    if (number % divisor == 0) {
      factors[count] = divisor;
      exponents[count] = 0;
      while (number % divisor == 0) {
        number /= divisor;
        exponents[count]++;
      }
      count++;
      sqrt_num = sqrt(number);
    }
  }

  // Si el número restante es primo
  if (number > 1) {
    factors[count] = number;
    exponents[count] = 1;
    count++;
  }

  // Construir la cadena de resultado
  result[0] = '\0';  // Inicializar el buffer de resultado
  for (uint8_t i = 0; i < count; i++) {
    if (i > 0) {
      strcat(result, " * ");  // Separador
    }
    if (exponents[i] == 1) {
      sprintf(buffer, "%u", factors[i]);
    } else {
      sprintf(buffer, "%ue%u", factors[i], exponents[i]);  // Formato "2e3"
    }
    strcat(result, buffer);
  }

  // Verificar si el número era primo
  if (count == 1 && exponents[0] == 1) {
    strcat(result, " ( PRIMO )");
  }

  return result;
}

// Las 2 Funciones Siguientes Son Para Telegram. Por Si Alguno Le Interesa Puede Poner En Las Declaraciones De Variables Su BOT E ID Del Grupo De Telegram Y El NerdMinerv2 Le Enviará Mensajes De Estado Cada 30m...

void enviarMensajeATelegram(String mensaje) {
  WiFiClientSecure client;
  client.setInsecure();
  // Reemplazar los saltos de línea con %0A
  String mensajeCodificado = urlEncode(mensaje);
  mensajeCodificado.replace("\n", "%0A");
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("No hay conexión WiFi.");
    return;
  }

  Serial.println("Enviando mensaje...");

  String url = "https://api.telegram.org/bot" + BOT_TOKEN + "/sendMessage?chat_id=" + CHAT_ID + "&text=" + mensajeCodificado;

  if (client.connect("api.telegram.org", 443)) {
    client.print(String("GET ") + url + " HTTP/1.1\r\n" + "Host: api.telegram.org\r\n" + "Connection: close\r\n\r\n");
    delay(1000);  // Espera para dar tiempo a la respuesta

    while (client.available()) {
      String line = client.readStringUntil('\r');
      Serial.print(line);
    }
    Serial.println("\nMensaje enviado");
  } else {
    Serial.println("Error de conexión");
  }
  client.stop();
}

void recopilaTelegram() {
  unsigned long epochTime = timeClient.getEpochTime();  // Obtener segundos desde 1970
  time_t epoch = (time_t)epochTime;                     // Convertir a time_t
  struct tm* timeinfo = localtime(&epoch);              // Convertir a una estructura de tiempo local
  int dia = timeinfo->tm_mday;                          // Día del mes (1 a 31)
  int mes = timeinfo->tm_mon + 1;                       // Mes (1 a 12)
  int anio = timeinfo->tm_year + 1900;                  // Año (por defecto es desde 1900)
  int horita = timeinfo->tm_hour;                       // Hora
  int minutitos = timeinfo->tm_min;                     // Minutos
  int segundos = timeinfo->tm_sec;                      // Segundos
  // Formatear la hora en "00:00:00"
  char horaFormateada[9];
  sprintf(horaFormateada, "%02d:%02d:%02d", horita, minutitos, segundos);
  // Formatear la fecha en "dia/mes/año" 
  char fechaFormateada[11];
  sprintf(fechaFormateada, "%02d/%02d/%04d", dia, mes, anio);
  std::string quediase = obtenerDiaSemana(std::string(fechaFormateada));
  uint8_t mac[6];
  WiFi.macAddress(mac);
  // Extraer los últimos 4 dígitos de la mac
  String u4digits = String(mac[4], HEX) + String(mac[5], HEX);
  String telrb = monedilla.remainingBlocks;
  String cadenaEnvio = "--------------------------------------------------------------------------------------------------------------\n";
  cadenaEnvio += "------------------------------- M8AX - NerdMinerV2-" + u4digits + " DATOS DE MINERÍA - M8AX ------------------------------\n";
  cadenaEnvio += "--------------------------------------------------------------------------------------------------------------\n";
  cadenaEnvio = cadenaEnvio + "------------------------------------------- " + fechaFormateada + " " + quediase.c_str()+" - " + horaFormateada + " ----------------------------------------\n";
  cadenaEnvio += "--------------------------------------------------------------------------------------------------------------\n";
  cadenaEnvio += "Mensaje Enviado Número - "+convertirARomanos(sumatelegram)+"\n";
  cadenaEnvio += "Modelo De Chip - "+String(ESP.getChipModel())+"\n";
  cadenaEnvio += "Versión De SDK - "+String(ESP.getSdkVersion())+"\n";
  cadenaEnvio += "Tiempo Minando - " + mineria.timeMining.substring(0, mineria.timeMining.indexOf(" ")) + " Días" + mineria.timeMining.substring(mineria.timeMining.indexOf(" ") + 1) + "\n";
  cadenaEnvio += "HashRate Actual - " + mineria.currentHashRate + " KH/s"+ " ( Max - "+String(maxkh)+" KH/s | Min - "+String(minkh)+" KH/s )\n";
  cadenaEnvio += "Temperatura De CPU De NerdMinerV2 - " + mineria.temp + "° ( Max - "+String(maxtemp)+"° | Min - "+String(mintemp)+"° )\n";
  cadenaEnvio += "Alertas De Temperatura, Más De 70° - "+String(tempalert)+" Veces\n";
  cadenaEnvio += "Plantillas De Bloque - " + mineria.templates + "\n";
  cadenaEnvio += "Shares Completados Y Enviados A La Pool - " + mineria.completedShares + "\n";
  cadenaEnvio += "Mejor Dificultad Alcanzada - " + mineria.bestDiff + "\n";
  cadenaEnvio += "Dificultad De La Red - " + monedilla.netwrokDifficulty + "\n";
  cadenaEnvio += "Cómputo Total (KH) - " + mineria.totalKHashes + "\n";
  cadenaEnvio += "Cómputo Total (MH) - " + String(atof(mineria.totalKHashes.c_str()) / 1000, 5) + "\n";
  cadenaEnvio += "Hash Rate Global - " + monedilla.globalHashRate + " EH/s\n";
  cadenaEnvio += "Precio De BITCOIN - " + monedilla.btcPrice + "\n";
  cadenaEnvio += "Promedio Por Transacción, FEE - " + monedilla.halfHourFee + "\n";
  cadenaEnvio += "Altura De Bloque - " + relojete.blockHeight + "\n";
  telrb.replace("BLOCKS", "Bloques");
  cadenaEnvio += "Total De Bloques Entre Halvings - 210000 Bloques\n";
  cadenaEnvio += "Bloques Restantes Para El Próximo Halving - " + String(telrb) + "\n";
  long int hechos = 210000 - telrb.toInt();
  cadenaEnvio += "Bloques Minados Desde El Último Halving - " + String(hechos) + " Bloques\n";
  char buffer[10];
  dtostrf((hechos * 100.0) / 210000.0, 6, 5, buffer);  // Convierte float a string con 5 decimales
  cadenaEnvio += "Porcentaje Completado Desde El Último Halving - " + String(buffer) + "%\n";
  cadenaEnvio += "Porcentaje Restante Para Próximo Halving - " + String(100.00000 - round(atof(buffer) * 100000) / 100000, 5) + "%\n";
  cadenaEnvio += "Pool De Minería - " + Settings.PoolAddress + "\n";
  cadenaEnvio += "Puerto Del Pool - " + String(Settings.PoolPort) + "\n";
  cadenaEnvio += "Tu Wallet De Bitcoin - " + String(Settings.BtcWallet) + "\n";
  cadenaEnvio += "Tu IP - " + getPublicIP() + "\n";
  cadenaEnvio += "--------------------------------------------------------------------------------------------------------------\n";
  if (mineria.valids.toInt() == 1) {
    cadenaEnvio += "El Valor De Bloques Válidos Es 1. ||| HAS MINADO UN BLOQUE, ASÍ QUE TIENES PASTA EN TU BILLETERA :) |||\n";
  } else {
    cadenaEnvio += "El Valor De Bloques Válidos Es 0. ||| AÚN NO HAS MINADO UN BLOQUE, BUFFF!, AÚN NO ERES RICO, PACIENCIA... |||\n";
  }
  cadenaEnvio += "--------------------------------------------------------------------------------------------------------------\n";
  cadenaEnvio += "----------------------------------------- M8AX - DATOS NERD - M8AX -------------------------------------------\n";
  cadenaEnvio += "--------------------------------------------------------------------------------------------------------------\n";
  rndnumero = esp_random();
  cadenaEnvio += "Factorización De Número - " + String(rndnumero) + " -> " + factorize(rndnumero) + "\n";
  cadenaEnvio += "--------------------------------------------------------------------------------------------------------------\n";
  int numeritos[6];
  int destino = 1 + (esp_random() % 1000);
  generate_random_numbers(numeritos, 6, 1, 100);
  cadenaEnvio += "Números - ";
  for (int i = 0; i < 6; i++) {
    if (i != 5) {
      cadenaEnvio += String(numeritos[i]) + ", ";
    } else {
      cadenaEnvio += String(numeritos[i]);
    }
  }
  cadenaEnvio += "\nDestino A Alcanzar - ( + - * / ) -> " + String(destino) + "\n";
  calculate_operations(numeritos, destino, result);
  borrarDesdeUltimoGuion(result);
  cadenaEnvio += result;
  cadenaEnvio += "\n--------------------------------------------------------------------------------------------------------------\n                                              By M8AX Corp. " + String(anio);
  cadenaEnvio += "\n--------------------------------------------------------------------------------------------------------------\n";
  enviarMensajeATelegram(cadenaEnvio);
  Serial.println(cadenaEnvio);
  cadenaEnvio = "";
}

void recopilaTelegram2() {
  unsigned long epochTime = timeClient.getEpochTime();  // Obtener segundos desde 1970
  time_t epoch = (time_t)epochTime;                     // Convertir a time_t
  struct tm* timeinfo = localtime(&epoch);              // Convertir a una estructura de tiempo local
  int dia = timeinfo->tm_mday;                          // Día del mes (1 a 31)
  int mes = timeinfo->tm_mon + 1;                       // Mes (1 a 12)
  int anio = timeinfo->tm_year + 1900;                  // Año (por defecto es desde 1900)
  int horita = timeinfo->tm_hour;                       // Hora
  int minutitos = timeinfo->tm_min;                     // Minutos
  int segundos = timeinfo->tm_sec;                      // Segundos
  String telrb = monedilla.remainingBlocks;
  String cadenaEnvio2 = "";
  cadenaEnvio2 += relojete.currentDate + " - " + relojete.currentTime;
  cadenaEnvio2 += " Tiempo Minando - " + mineria.timeMining.substring(0, mineria.timeMining.indexOf(" ")) + " Días" + mineria.timeMining.substring(mineria.timeMining.indexOf(" ") + 1);
  cadenaEnvio2 += " HashRate Actual - " + mineria.currentHashRate + " KH/s";
  cadenaEnvio2 += " Temperatura De CPU De NerdMinerV2 - " + mineria.temp + "g";
  cadenaEnvio2 += " Plantillas De Bloque - " + mineria.templates;
  cadenaEnvio2 += " Shares Completados Y Enviados A La Pool - " + mineria.completedShares;
  cadenaEnvio2 += " Mejor Dificultad Alcanzada - " + mineria.bestDiff;
  cadenaEnvio2 += " Dificultad De La Red - " + monedilla.netwrokDifficulty;
  cadenaEnvio2 += " Cómputo Total (KH) - " + mineria.totalKHashes;
  cadenaEnvio2 += " Cómputo Total (MH) - " + String(atof(mineria.totalKHashes.c_str()) / 1000, 5);
  cadenaEnvio2 += " Hash Rate Global - " + monedilla.globalHashRate + " EH/s";
  cadenaEnvio2 += " Precio De BITCOIN - " + monedilla.btcPrice;
  cadenaEnvio2 += " Promedio Por Transacción, FEE - " + monedilla.halfHourFee;
  cadenaEnvio2 += " Altura De Bloque - " + relojete.blockHeight;
  telrb.replace("BLOCKS", "Bloques");
  cadenaEnvio2 += " Total De Bloques Entre Halvings - 210000 Bloques";
  cadenaEnvio2 += " Bloques Restantes Para El Próximo Halving - " + String(telrb);
  long int hechos = 210000 - telrb.toInt();
  cadenaEnvio2 += " Bloques Minados Desde El Último Halving - " + String(hechos) + " Bloques";
  char buffer[10];
  dtostrf((hechos * 100.0) / 210000.0, 6, 5, buffer);  // Convierte float a string con 5 decimales
  cadenaEnvio2 += " Porcentaje Completado Desde El Último Halving - " + String(buffer);
  cadenaEnvio2 += " Porcentaje Restante Para Próximo Halving - " + String(100.00000 - round(atof(buffer) * 100000) / 100000, 5);
  if (mineria.valids.toInt() == 1) {
    cadenaEnvio2 += " El Valor De Bloques Válidos Es 1. ||| HAS MINADO UN BLOQUE, ASÍ QUE TIENES PASTA EN TU BILLETERA |||";
  } else {
    cadenaEnvio2 += " El Valor De Bloques Válidos Es 0. ||| AÚN NO HAS MINADO UN BLOQUE, BUFFF!, AÚN NO ERES RICO, PACIENCIA... |||";
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
}

std::pair<String, String> obtenerCiudadYTemperatura(const String& ip) {
  String ciudad = "";
  String temperatura = "";

  // Obtener la ciudad usando la API de geolocalización
  HTTPClient http;
  String urlGeo = "http://ip-api.com/json/" + ip + "?fields=city";  // Obtener solo la ciudad
  http.begin(urlGeo);

  int httpCode = http.GET();
  if (httpCode > 0) {
    String payload = http.getString();

    // Parsear el JSON para obtener la ciudad
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, payload);
    if (error) {
      Serial.println("Error al parsear JSON de la ciudad");
      return std::make_pair(ciudad, temperatura);  // Devolver vacías en caso de error
    }
    ciudad = doc["city"].as<String>();  // Ciudad obtenida
  } else {
    Serial.println("Error al obtener ciudad");
    return std::make_pair(ciudad, temperatura);  // Devolver vacías si hay error
  }

  http.end();

  // Obtener la temperatura usando wttr.in con HTTPS
  if (ciudad != "") {
    // Usamos WiFiClientSecure para HTTPS
    WiFiClientSecure client;
    client.setInsecure();  // Permitimos conexiones HTTPS sin verificar certificado (menos seguro)

    String urlTemp = "https://wttr.in/" + ciudad + "?format=%t";  // Usar https para obtener temperatura
    http.begin(client, urlTemp);                                  // Iniciar la solicitud HTTPS
    httpCode = http.GET();

    if (httpCode > 0) {
      temperatura = http.getString();  // Obtener la temperatura
    } else {
      Serial.println("Error al obtener temperatura");
    }
    http.end();
  } else {
    Serial.println("Ciudad no encontrada para la IP.");
  }

  return std::make_pair(ciudad, temperatura);  // Devolver ciudad y temperatura
}

void drawGrid(void) {
  uint16_t color = color = TFT_WHITE;
  for (int16_t x = 1; x < GRIDX - 1; x++) {
    for (int16_t y = 1; y < GRIDY - 1; y++) {
      if ((grid[x][y]) != (newgrid[x][y])) {
        if (newgrid[x][y] == 1) color = 0xFFFF;  //random(0xFFFF);
        else color = 0;
        tft.fillRect(CELLXY * x, CELLXY * y, CELLXY, CELLXY, color);
      }
    }
  }
}

int getNumberOfNeighbors(int x, int y) {
  return grid[x - 1][y] + grid[x - 1][y - 1] + grid[x][y - 1] + grid[x + 1][y - 1] + grid[x + 1][y] + grid[x + 1][y + 1] + grid[x][y + 1] + grid[x - 1][y + 1];
}

void initGrid(void) {
  for (int16_t x = 0; x < GRIDX; x++) {
    for (int16_t y = 0; y < GRIDY; y++) {
      newgrid[x][y] = 0;
      if (x == 0 || x == GRIDX - 1 || y == 0 || y == GRIDY - 1) {
        grid[x][y] = 0;
      } else {
        if (random(3) == 1)
          grid[x][y] = 1;
        else
          grid[x][y] = 0;
      }
    }
  }
}

//Compute the CA. Basically everything related to CA starts here
void computeCA() {
  for (int16_t x = 1; x < GRIDX; x++) {
    for (int16_t y = 1; y < GRIDY; y++) {
      int neighbors = getNumberOfNeighbors(x, y);
      if (grid[x][y] == 1 && (neighbors == 2 || neighbors == 3)) {
        newgrid[x][y] = 1;
      } else if (grid[x][y] == 1) newgrid[x][y] = 0;
      if (grid[x][y] == 0 && (neighbors == 3)) {
        newgrid[x][y] = 1;
      } else if (grid[x][y] == 0) newgrid[x][y] = 0;
    }
  }
}

String capitalizar(String palabra) {
  if (palabra.length() > 0) {
    palabra[0] = toupper(palabra[0]);
  }
  return palabra;
}

String numeroAEscrito(int num, bool esDecimal = false) {
  if (num < 0 || num > 9999) return "Número Fuera De Rango";

  String unidades[] = { "Cero", "Uno", "Dos", "Tres", "Cuatro", "Cinco", "Seis", "Siete", "Ocho", "Nueve" };
  String especiales[] = { "Diez", "Once", "Doce", "Trece", "Catorce", "Quince",
                          "Dieciséis", "Diecisiete", "Dieciocho", "Diecinueve" };
  String decenas[] = { "", "", "Veinte", "Treinta", "Cuarenta", "Cincuenta",
                       "Sesenta", "Setenta", "Ochenta", "Noventa" };
  String centenas[] = { "", "Ciento", "Doscientos", "Trescientos", "Cuatrocientos",
                        "Quinientos", "Seiscientos", "Setecientos", "Ochocientos", "Novecientos" };

  if (num == 100) return "Cien";  // Caso especial
  if (num == 0) return "Cero";

  String resultado = "";

  // Si es la parte decimal y el número es menor que 10, agregar "Cero"
  if (esDecimal && num < 10) {
    resultado += "Cero ";
  }

  // Miles
  if (num >= 1000) {
    if (num / 1000 == 1) {
      resultado += "Mil";
    } else {
      resultado += capitalizar(unidades[num / 1000]) + " Mil";
    }
    num %= 1000;  // Eliminamos los miles
    if (num > 0) resultado += " ";
  }

  // Centenas
  if (num >= 100) {
    resultado += capitalizar(centenas[num / 100]);
    num %= 100;  // Eliminamos las centenas
    if (num > 0) resultado += " ";
  }

  // Decenas y unidades
  if (num >= 10 && num <= 19) {
    resultado += capitalizar(especiales[num - 10]);  // Números entre 10 y 19
  } else {
    if (num >= 20) {
      resultado += capitalizar(decenas[num / 10]);
      if (num % 10 > 0) resultado += " Y " + capitalizar(unidades[num % 10]);  // "Veintiuno", "Treinta Y Uno"
    } else if (num > 0) {
      resultado += capitalizar(unidades[num]);  // Unidades solas
    }
  }

  return resultado;
}

void dibujarDado(int numero, int x, int y) {
  // Dibuja el borde del dado
  tft.drawRect(x - 30, y - 30, 60, 60, TFT_WHITE);  // Cuadrado del dado

  int r = 5;  // Radio de los puntos

  // Posiciones de los puntos del dado según el número (del 1 al 6)
  // Definimos los puntos en un arreglo para que siempre se muestren correctamente
  switch (numero) {
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

void dibujarMoneda(int moneda, int x, int y) {
  int radio = 45;  // Un tamaño más grande para la moneda

  tft.drawCircle(x, y, radio, TFT_WHITE);  // Dibujar la moneda

  if (moneda == 0) {
    tft.setTextColor(TFT_GREEN);
    tft.setTextSize(2);
    tft.drawCentreString("C", x, y - 20, 4);  // Dibuja "C" para cara
  } else {
    tft.setTextColor(TFT_RED);
    tft.setTextSize(2);
    tft.drawCentreString("X", x, y - 20, 4);  // Dibuja "X" para cruz
  }
}

void drawCenteredText(const char* text, int y, int delayTime) {
  int screenWidth = tft.width();          // Ancho de la pantalla
  int textWidth = tft.textWidth(text);    // Ancho del texto
  int x = (screenWidth - textWidth) / 5;  // Posición X para centrar el texto

  // Efecto de escritura letra por letra
  for (int i = 0; i < strlen(text); i++) {
    colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
    tft.setTextColor(colors[colorI], TFT_BLACK);
    tft.drawChar(text[i], x, y, 2);           // Dibuja una letra
    x += tft.textWidth(String(text[i])) + 5;  // Ajusta la posición X para la siguiente letra
    delay(delayTime);                         // Retardo para el efecto
  }
}

void television() {

  int barWidth = random(5, 41);  // Ancho de las barras
  int speed = random(5, 26);     // Velocidad de movimiento (ajusta para más rápido o más lento)

  // Barras horizontales
  for (int y = 0; y < tft.height(); y += barWidth) {
    colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
    tft.fillRect(0, y, tft.width(), barWidth, colors[colorI]);
    delay(speed);
    tft.fillRect(0, y, tft.width(), barWidth, TFT_BLACK);
  }

  // Barras verticales
  for (int x = 0; x < tft.width(); x += barWidth) {
    colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
    tft.fillRect(x, 0, barWidth, tft.height(), colors[colorI]);
    delay(speed);
    tft.fillRect(x, 0, barWidth, tft.height(), TFT_BLACK);
  }

  // Barras diagonales (de esquina a esquina)
  for (int i = 0; i < tft.width() + tft.height(); i += barWidth) {
    colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
    tft.drawLine(i, 0, 0, i, colors[colorI]);  // Diagonal de arriba a la izquierda
    delay(speed);
    tft.drawLine(i, 0, 0, i, TFT_BLACK);  // Borrar la línea
  }

  for (int i = 0; i < tft.width() + tft.height(); i += barWidth) {
    colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
    tft.drawLine(tft.width() - i, tft.height(), tft.width(), tft.height() - i, colors[colorI]);  // Diagonal de abajo a la derecha
    delay(speed);
    tft.drawLine(tft.width() - i, tft.height(), tft.width(), tft.height() - i, TFT_BLACK);  // Borrar la línea
  }

  // Finalizar con la pantalla en negro
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(7);
  tft.setCursor(76, 52);
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  tft.setTextColor(colors[colorI], TFT_BLACK);
  tft.print("HOLA");
  tft.setTextSize(1);
}

void nevar() {
  tft.fillScreen(TFT_BLACK);  // Fondo negro (puedes cambiarlo)

  const int NUM_COPOS = 100;       // Número de copos de nieve en pantalla
  int x[NUM_COPOS], y[NUM_COPOS];  // Coordenadas de los copos

  // Inicializa copos en posiciones aleatorias
  for (int i = 0; i < NUM_COPOS; i++) {
    x[i] = random(0, 320);
    y[i] = random(0, 170);
  }

  unsigned long startTime = millis();

  while (millis() - startTime < 5000) {  // 5 segundos
    tft.fillScreen(TFT_BLACK);           // Borra pantalla

    for (int i = 0; i < NUM_COPOS; i++) {
      tft.drawPixel(x[i], y[i], TFT_WHITE);  // Dibuja copo de nieve
      y[i] += random(1, 5);                  // Baja la posición del copo

      if (y[i] > 170) {  // Si sale de la pantalla, reaparece arriba
        y[i] = 0;
        x[i] = random(0, 320);
      }
    }

    delay(35);  // Controla la velocidad de la animación
  }

  // Muestra "FELIZ NAVIDAD" al final
  tft.fillScreen(TFT_BLACK);
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  tft.setTextColor(colors[colorI], TFT_BLACK);
  tft.setTextDatum(MC_DATUM);                // Centra el texto
  tft.setFreeFont(FSB18);                    // Fuente grande (cambia si es necesario)
  tft.drawString("FELIZ NAVIDAD", 160, 85);  // Texto en el centro
  delay(1500);
  tft.setFreeFont(NULL);
}

void nevar2() {
  tft.fillScreen(TFT_BLACK);  // Fondo negro (puedes cambiarlo)

  const int NUM_COPOS = 200;       // Número de copos de nieve en pantalla
  int x[NUM_COPOS], y[NUM_COPOS];  // Coordenadas de los copos

  // Inicializa copos en posiciones aleatorias
  for (int i = 0; i < NUM_COPOS; i++) {
    x[i] = random(0, 320);
    y[i] = random(0, 170);
  }

  unsigned long startTime = millis();

  while (millis() - startTime < 5000) {  // 5 segundos
    tft.fillScreen(TFT_BLACK);           // Borra pantalla

    for (int i = 0; i < NUM_COPOS; i++) {
      tft.drawPixel(x[i], y[i], TFT_WHITE);  // Dibuja copo de nieve
      y[i] += random(1, 5);                  // Baja la posición del copo

      if (y[i] > 170) {  // Si sale de la pantalla, reaparece arriba
        y[i] = 0;
        x[i] = random(0, 320);
      }
    }

    delay(25);  // Controla la velocidad de la animación
  }

  // Muestra "FELIZ --- AÑO" al final
  tft.fillScreen(TFT_BLACK);
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  tft.setTextColor(colors[colorI], TFT_BLACK);
  tft.setTextDatum(MC_DATUM);                 // Centra el texto
  tft.setFreeFont(FSB18);                     // Fuente grande (cambia si es necesario)
  tft.drawString("HAPPY NEW YEAR", 160, 85);  // Texto en el centro
  delay(1500);
  tft.setFreeFont(NULL);
}

void M8AXTicker() {
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

void nevar3() {
  tft.fillScreen(TFT_BLACK);  // Fondo negro (puedes cambiarlo)

  const int NUM_COPOS = 500;       // Número de copos de nieve en pantalla
  int x[NUM_COPOS], y[NUM_COPOS];  // Coordenadas de los copos

  // Inicializa copos en posiciones aleatorias
  for (int i = 0; i < NUM_COPOS; i++) {
    x[i] = random(0, 320);
    y[i] = random(0, 170);
  }

  unsigned long startTime = millis();

  while (millis() - startTime < 2000) {  // 5 segundos
    tft.fillScreen(TFT_BLACK);           // Borra pantalla
    for (int i = 0; i < NUM_COPOS; i++) {
      colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
      tft.drawPixel(x[i], y[i], colors[colorI]);  // Dibuja copo de nieve
      y[i] += random(1, 5);                       // Baja la posición del copo

      if (y[i] > 170) {  // Si sale de la pantalla, reaparece arriba
        y[i] = 0;
        x[i] = random(0, 320);
      }
    }
    int nnumeroAleatorio = random(1, 20);
    delay(nnumeroAleatorio);  // Controla la velocidad de la animación
  }
  M8AXTicker();
}

void cortinas2() {
  int centerX = tft.width() / 2;   // Centro de la pantalla en X
  int centerY = tft.height() / 2;  // Centro de la pantalla en Y

  for (int i = 0; i <= centerX; i++) {
    // Dibuja líneas verticales que se alejan del centro
    tft.drawFastVLine(centerX - i, 0, tft.height(), TFT_BLACK);  // Izquierda
    tft.drawFastVLine(centerX + i, 0, tft.height(), TFT_BLACK);  // Derecha
    delay(10);                                                   // Ajusta la velocidad de la animación
  }
}

void cortinas() {
  tft.fillScreen(TFT_BLACK);  // Fondo negro
  cortinas2();
  drawCenteredText("M 8 A X", 30, 100);
  drawCenteredText("NerdMiner V2", 70, 100);
  drawCenteredText("V  10 . 03 . 77", 110, 100);
  delay(500);
  television();
  delay(1000);
  tft.setTextSize(1);
}

void M8AXTicker2() {
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

void M8AXTicker3() {
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

void M8AXTicker4() {
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

void dibujaQR(String data, int xPos, int yPos, int qrSize, int color) {
  QRCode qrcode;
  uint8_t qrcodeData[qrcode_getBufferSize(3)];  // Tamaño del QR, nivel 3

  // Inicializa el QR con los datos
  qrcode_initText(&qrcode, qrcodeData, 3, 0, data.c_str());

  // Tamaño real del QR en módulos
  int qrRealSize = qrcode.size;

  // Calcular el tamaño de cada bloque (módulo) según el tamaño total deseado
  int blockSize = qrSize / qrRealSize;

  // Dibujar los módulos (pixeles) del QR en la pantalla
  for (int y = 0; y < qrRealSize; y++) {
    for (int x = 0; x < qrRealSize; x++) {
      // Si es un módulo negro, dibujarlo
      if (qrcode_getModule(&qrcode, x, y)) {
        tft.fillRect(xPos + x * blockSize, yPos + y * blockSize, blockSize, blockSize, color);
      }
      // Si es un módulo blanco, NO hacer nada (deja el fondo transparente)
    }
  }
}

bool esHorarioDeVerano(int mes, int dia) {
  if (mes > 3 && mes < 10) {
    return true;
  }
  if (mes == 3 && dia >= 25) {
    return true;
  }
  if (mes == 10 && dia <= 31) {
    return false;
  }

  return false;
}

void dibujaAnalogKH(float khs) {
  int centerX = 160;
  int centerY = 85;
  int radius = 60;
  tft.setTextSize(1);
  tft.drawCircle(centerX, centerY, radius, TFT_WHITE);
  for (int i = 0; i < 360; i += 15) {
    float angle = (i - 90) * DEG_TO_RAD;
    int outerX = centerX + radius * cos(angle);
    int outerY = centerY + radius * sin(angle);
    int innerX = centerX + (radius - 5) * cos(angle);
    int innerY = centerY + (radius - 5) * sin(angle);
    if (i % 90 == 0) {
      innerX = centerX + (radius - 10) * cos(angle);
      innerY = centerY + (radius - 10) * sin(angle);
      tft.drawLine(innerX, innerY, outerX, outerY, TFT_WHITE);
    } else {
      tft.drawLine(innerX, innerY, outerX, outerY, TFT_WHITE);
    }
    if (i % 90 == 0) {
      tft.setTextColor(TFT_WHITE);
      int textX = centerX + (radius - 20) * cos(angle);  // Coordenadas para el texto
      int textY = centerY + (radius - 20) * sin(angle);
      tft.setCursor(textX - 5, textY - 5);  // Ajustar el texto
      if (i == 0)
        tft.print("360");  // 12 horas -> 360 KH/s
      else if (i == 90)
        tft.print("90");  // 15 minutos -> 90 KH/s
      else if (i == 180)
        tft.print("180");  // 30 minutos -> 180 KH/s
      else if (i == 270)
        tft.print("270");  // 45 minutos -> 270 KH/s
    }
  }

  // No dibujar la aguja si khs es 0
  if (khs == 0) {
    return;
  }

  // Mapear el valor de KH/s a un ángulo de -90 (0 KH/s) a 90 (360 KH/s)
  float angle = map(khs, 0, 360, -90, 270);  // Amplitud del reloj ajustada a 360 grados

  // Convertir el ángulo a radianes
  float radianes = angle * DEG_TO_RAD;

  // Coordenadas de la punta de la aguja (calculada a partir del ángulo)
  int agujaX = centerX + radius * cos(radianes);
  int agujaY = centerY + radius * sin(radianes);

  // Dibujar la aguja
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  tft.drawLine(centerX, centerY, agujaX, agujaY, TFT_GREENYELLOW);  // Dibuja la aguja
  tft.setCursor(142, 96);
  tft.setTextSize(1);
  tft.setTextColor(colors[colorI]);
  tft.print(String(khs));
  tft.setCursor(142, 105);
  tft.print(" KH/s");
}

void obtenerNoticias() {
  HTTPClient http;
  int indice = esp_random() % 9;
  String urlnot = urls[indice];
  http.begin(urlnot);         // URL del feed RSS
  int httpCode = http.GET();  // Realiza la solicitud GET
  cadenanoti = "";
  if (httpCode == HTTP_CODE_OK) {  // Si la solicitud es exitosa
    String payload = http.getString();
    Serial.println("M8AX - Noticias Obtenidas. Procesando...");

    int itemIndex = 0;
    int startIndex = 0;
    int endIndex = 0;

    // Extraer hasta 5 titulares
    while (itemIndex < 5) {
      // Buscar la etiqueta <item> que marca una nueva noticia
      startIndex = payload.indexOf("<item>", endIndex);  // Encuentra <item>
      if (startIndex == -1) break;

      endIndex = payload.indexOf("</item>", startIndex);  // Encuentra </item>
      if (endIndex == -1) break;

      // Extraer el título dentro de <title> de cada <item>
      int titleStart = payload.indexOf("<title>", startIndex);
      int titleEnd = payload.indexOf("</title>", titleStart);
      String title = payload.substring(titleStart + 7, titleEnd);  // Extrae el título

      // Eliminar la parte CDATA si está presente
      int cdataStart = title.indexOf("<![CDATA[");
      int cdataEnd = title.indexOf("]]>");
      if (cdataStart != -1 && cdataEnd != -1) {
        title = title.substring(cdataStart + 9, cdataEnd);  // Elimina CDATA
      }

      // Si el título es del canal (ej. "Cointelegraph.com News"), busca otra noticia
      if (title.indexOf("Cointelegraph.com News") != -1) {
        continue;  // Si el título es del canal, salta a la siguiente noticia
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
    if (itemIndex == 0) {
      Serial.println("No se encontraron noticias válidas.");
    }
  } else {
    Serial.println("Error Al Obtener El Feed RSS. Código HTTP: " + String(httpCode));
  }

  http.end();  // Finaliza la solicitud HTTP
}

float obtenerPrecio(String currency_pair) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi no conectado, intentando reconectar...");
    WiFi.disconnect();
    WiFi.reconnect();
    delay(500);
  }

  WiFiClientSecure client;
  client.setInsecure();  // Desactiva verificación SSL (Coinbase usa HTTPS)

  const char* host = "api.coinbase.com";
  const int port = 443;  // HTTPS usa el puerto 443

  Serial.print("Conectando a ");
  Serial.println(host);

  if (!client.connect(host, port)) {
    Serial.println("Error: No se pudo conectar al servidor");
    return -1;
  }

  // Construimos la petición HTTP manualmente
  String url = "/v2/prices/" + currency_pair + "/spot";
  client.print(String("GET ") + url + " HTTP/1.1\r\n" + "Host: " + host + "\r\n" + "User-Agent: ESP32\r\n" + "Connection: close\r\n\r\n");

  // Esperamos la respuesta del servidor
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {  // Timeout de 5 segundos
      Serial.println("Error: Timeout esperando respuesta del servidor");
      client.stop();
      return -1;
    }
  }

  // Leemos y descartamos los headers HTTP
  while (client.available()) {
    String linea = client.readStringUntil('\n');
    if (linea == "\r") {
      break;  // Fin de los headers
    }
  }

  // Leemos solo el JSON de la respuesta
  String payload = "";
  while (client.available()) {
    payload += client.readString();
  }

  Serial.println("Respuesta de la API:");
  Serial.println(payload);

  client.stop();  // Cerramos la conexión

  // Parseamos el JSON con menos consumo de RAM
  DynamicJsonDocument doc(256);
  DeserializationError error = deserializeJson(doc, payload);

  if (error) {
    Serial.print("Error al parsear JSON: ");
    Serial.println(error.c_str());
    return -1;
  }

  // Extraemos el precio si existe
  if (doc.containsKey("data") && doc["data"].containsKey("amount")) {
    float precio = doc["data"]["amount"].as<float>();
    return precio;
  } else {
    Serial.println("El JSON no tiene el formato esperado.");
    return -1;
  }
}

String Amorse(int n) {
  // Diccionario de los números y sus representaciones en código morse
  const String morse_dict[] = {
    "-----", ".----", "..---", "...--", "....-", ".....", "-....", "--...", "---..", "----."
  };

  // Convertir el número a cadena de caracteres
  String num_str = String(n);

  // Variable para almacenar el resultado en código Morse
  String morse_code = "";

  // Convertir cada dígito del número a morse y concatenarlo a la cadena resultante
  for (unsigned int i = 0; i < num_str.length(); i++) {
    char digit = num_str.charAt(i);
    morse_code += morse_dict[digit - '0'] + " ";  // Restamos '0' para convertir el carácter a un índice
  }

  return morse_code;
}

String ABinario(int num) {
  String binary = "";

  if (num == 0) {
    return "0";  // Si el número es 0, devuelve "0"
  }

  while (num > 0) {
    binary = (num % 2 == 0 ? "0" : "1") + binary;  // Agregar el bit al principio
    num /= 2;                                      // Divide el número entre 2
  }

  return binary;
}

String getQuote() {
  HTTPClient http;
  http.begin(serverName);
  http.addHeader("Content-Type", "application/json");

  String quote = "ERROR AL OBTENER LA CITA... Por Muchas Vueltas Que Demos, Siempre Tendremos El Culo Atras...";  // Valor por defecto en caso de error

  int httpResponseCode = 0;
  int attempts = 0;
  const int maxAttempts = 5;  // Número máximo de intentos

  while (httpResponseCode <= 0 && attempts < maxAttempts) {
    // Realizar la solicitud GET
    httpResponseCode = http.GET();
    attempts++;
    taskYIELD();
    if (httpResponseCode > 0) {
      String payload = http.getString();

      // Parsear el JSON
      DynamicJsonDocument doc(1024);
      deserializeJson(doc, payload);

      // Extraer la cita y el autor
      String quoteText = doc[0]["q"].as<String>();
      String author = doc[0]["a"].as<String>();

      quote = "\"" + quoteText + "\"\n- " + author;
      Serial.println("M8AX - Frase Número - " + String(numfrases) + " - " + quote);
    } else {
      Serial.println("Error Al Recibir La Cita. Reintentando...");
      delay(1000);  // Esperar 1 segundos antes de volver a intentar
    }
  }

  if (attempts == maxAttempts) {
    Serial.println("Se Alcanzó El Número Máximo De Intentos...");
  }
  http.end();
  return quote;
}

void displayQuote(String quote) {

  tft.setTextColor(TFT_WHITE);  // Establecer color de texto

  // Configuración de la fuente
  tft.setTextSize(2);
  tft.setCursor(0, 28);
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  tft.setTextColor(colors[colorI]);
  tft.drawLine(0, 20, 320, 20, colors[colorI]);  // Dibujar línea de (0, y) a (320, y)
  tft.print(quitarAcentos(quote));
  taskYIELD();
}


String obtenerNombreMes(int mes) {
  switch (mes) {
    case 1: return "Enero";
    case 2: return "Febrero";
    case 3: return "Marzo";
    case 4: return "Abril";
    case 5: return "Mayo";
    case 6: return "Junio";
    case 7: return "Julio";
    case 8: return "Agosto";
    case 9: return "Septiembre";
    case 10: return "Octubre";
    case 11: return "Noviembre";
    case 12: return "Diciembre";
    default: return "Mes inválido";
  }
}

void dibujarPorcentajeLunar(int centroX, int centroY, int radio, float porcentaje) {
  // Aseguramos que el porcentaje esté en un rango válido (0% a 100%)
  if (porcentaje < 0) porcentaje = 0;
  if (porcentaje > 100) porcentaje = 100;

  // Calculamos la posición horizontal hasta donde se debe rellenar
  int limiteX = centroX + (porcentaje / 100.0) * (2 * radio) - radio;  // Punto X del límite

  // Dibujar el círculo completo con el color de fondo
  tft.fillCircle(centroX, centroY, radio, TFT_TRANSPARENT);

  // Dibujar el porcentaje iluminado horizontalmente
  for (int x = -radio; x <= radio; x++) {    // Recorrer el ancho del círculo
    for (int y = -radio; y <= radio; y++) {  // Recorrer la altura del círculo
      // Verificar si el punto está dentro del círculo
      if (x * x + y * y <= radio * radio) {
        // Verificar si el punto está dentro del porcentaje horizontal
        if (centroX + x <= limiteX) {
          tft.drawPixel(centroX + x, centroY + y, colors[colorIndex]);  // Color del área iluminada
        }
      }
    }
  }
}

void dibujarReloj(int horas, int minutos, int segundos, String dia, String mes, String anio, std::string quediase, String HRate, String tempera, int millonario) {
  moonData_t moon;
  int diaa = dia.toInt();
  int mess = mes.toInt();
  int anioo = anio.toInt();
  int horaa = horas;
  int minutoo = minutos;
  String mesecillo = obtenerNombreMes(mess).substring(0, 3);
  // Inicializar la estructura tm
  struct tm timeinfo;
  timeinfo.tm_year = anioo - 1900;  // Año desde 1900
  timeinfo.tm_mon = mess - 1;       // Mes (0 = enero)
  timeinfo.tm_mday = diaa;          // Día del mes
  timeinfo.tm_hour = horaa;         // Hora
  timeinfo.tm_min = minutoo;        // Minutos
  timeinfo.tm_sec = 0;              // Segundos
  timeinfo.tm_isdst = -1;           // Determina si es horario de verano (automático)
  // Convertir a time_t
  time_t cadenaDeTiempo = mktime(&timeinfo);
  moon = moonPhase.getPhase(cadenaDeTiempo);
  double porcentajeIluminado = moon.percentLit * 100;
  char porcentajeTexto[10];
  snprintf(porcentajeTexto, sizeof(porcentajeTexto), "%.2f%%", porcentajeIluminado);
  String datoslunarelogrande = String(porcentajeTexto);
  // Limpia la pantalla para redibujar
  background.pushImage(0, 0, M8AXRelojLunarWidth, M8AXRelojLunarHeight, M8AXRelojLunar);
  background.pushSprite(0, 0);
  // Centro del reloj
  int centroX = 160;      // Centro horizontal
  int centroY = 170 / 2;  // Centro vertical
  int radio = 80;         // Aumenta el radio para hacer el reloj más grande

  // Dibuja el círculo del reloj
  tft.drawCircle(centroX, centroY, radio, colors[colorIndex]);

  // Calcula los ángulos para las manecillas
  float anguloSegundos = (segundos * 6) - 90;                          // 360° / 60 = 6° por segundo
  float anguloMinutos = (minutos * 6) - 90;                            // 360° / 60 = 6° por minuto
  float anguloHoras = (horas % 12) * 30 - 90 + (minutos / 60.0) * 30;  // 360° / 12 = 30° por hora

  // Dibuja la manecilla de los segundos
  int xSegundos = centroX + radio * 0.85 * cos(radians(anguloSegundos));  // Ajusta la longitud de la manecilla
  int ySegundos = centroY + radio * 0.85 * sin(radians(anguloSegundos));
  tft.drawLine(centroX, centroY, xSegundos, ySegundos, TFT_RED);

  // Dibuja la manecilla de los minutos
  int xMinutos = centroX + radio * 0.75 * cos(radians(anguloMinutos));  // Manecilla de minutos más larga
  int yMinutos = centroY + radio * 0.75 * sin(radians(anguloMinutos));
  tft.drawLine(centroX, centroY, xMinutos, yMinutos, TFT_WHITE);

  // Dibuja la manecilla de las horas
  int xHoras = centroX + radio * 0.60 * cos(radians(anguloHoras));  // Manecilla de horas más corta
  int yHoras = centroY + radio * 0.60 * sin(radians(anguloHoras));
  tft.drawLine(centroX, centroY, xHoras, yHoras, TFT_WHITE);

  // Dibuja las marcas de las horas
  for (int i = 0; i < 12; i++) {
    float anguloMarca = i * 30 - 90;  // 360° / 12 = 30° por hora
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
  tft.setCursor(241, 121);
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
  if (millonario == 0) {
    tft.setTextColor(colors[colorI]);
    tft.setCursor(230, 150);
    tft.setTextSize(2);
    tft.print("NoRICO");
  } else {
    tft.setTextColor(colors[colorI]);
    tft.setCursor(254, 5);
    tft.setTextSize(2);
    tft.print("RICO");
  }
  dibujarPorcentajeLunar(281, 84, 25, porcentajeIluminado);
  if (mirarTiempo == 0 || ciudad == "ERROR" || (minutos % 10 == 0 && segundos == 0)) {
    mirarTiempo = 1;
    // Aquí pones lo que deseas hacer cuando se cumple la condición
    std::pair<String, String> resultado = obtenerCiudadYTemperatura(getPublicIP());
    // Verificar si los valores fueron obtenidos correctamente
    if (resultado.first != "" && resultado.second != "") {
      // Imprimir los resultados en el Monitor Serie
      Serial.println("Ciudad: " + resultado.first);
      Serial.println("Temperatura: " + resultado.second);
      ciudad = resultado.first;
      tempciudad = resultado.second;

    } else {
      Serial.println("No se pudo obtener la ciudad o la temperatura.");
      ciudad = "ERROR";
      tempciudad = "ERROR";
    }
  }

  actualizarc = 0;
}

int aleaESP32(int min, int max) {
  return (esp_random() % (max - min + 1)) + min;
}

// Función para generar 6 números únicos y devolverlos como String
String generarNumerosPrimitiva() {
  const int MIN = 1;
  const int MAX = 49;
  const int NUM_COUNT = 6;

  int numeros[NUM_COUNT];
  int contador = 0;

  while (contador < NUM_COUNT) {
    int num = aleaESP32(MIN, MAX);  // Generar número aleatorio
    bool repetido = false;

    // Comprobar si el número ya existe en el array
    for (int i = 0; i < contador; i++) {
      if (numeros[i] == num) {
        repetido = true;
        break;
      }
    }

    // Si no está repetido, lo añadimos al array
    if (!repetido) {
      numeros[contador++] = num;
    }
  }

  // Convertir los números a un String
  String resultado = "";
  for (int i = 0; i < NUM_COUNT; i++) {
    resultado += String(numeros[i]);
    if (i < NUM_COUNT - 1) {
      resultado += ", ";  // Separador entre números
    }
  }

  return resultado;  // Retornar como String
}

// Función para calcular el primer día del mes
int calcularPrimerDia(int dia, int mes, int anio) {
  // Si el mes es enero o febrero, se ajusta como en el algoritmo de Zeller
  if (mes == 1) {
    mes = 13;
    anio--;
  }
  if (mes == 2) {
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

void mostrarCalendario(int dia, int mes, int anio, int h1, int h2, int m1, int m2) {
  // Calcular el primer día del mes usando la función calcularPrimerDia
  int primerDia = calcularPrimerDia(1, mes, anio);  // El día 1 del mes
  // Días de la semana
  String diasSemana[7] = { "LUN", "MAR", "MIE", "JUE", "VIE", "SAB", "DOM" };
  // Número de días en el mes (esto debe tenerse en cuenta para cada mes)
  int diasDelMes = 31;                                                 // Por defecto, asumir 31 días
  if (mes == 4 || mes == 6 || mes == 9 || mes == 11) diasDelMes = 30;  // Meses con 30 días
  if (mes == 2) {                                                      // Febrero, comprobar si es bisiesto
    if ((anio % 4 == 0 && anio % 100 != 0) || (anio % 400 == 0)) {
      diasDelMes = 29;  // Año bisiesto
    } else {
      diasDelMes = 28;  // Año no bisiesto
    }
  }


  // Mostrar los días de la semana
  for (int i = 0; i < 7; i++) {
    tft.setCursor(20 + (i * 40), 10);
    tft.setTextColor(colors[colorIndex]);
    tft.setTextSize(1);
    tft.print(diasSemana[i]);
  }

  // Mostrar los días del mes
  int x = primerDia * 40;
  int y = 30;
  for (int i = 1; i <= diasDelMes; i++) {
    // Si es el primer día, comenzamos en la posición correcta según el primer día del mes
    int diaDeLaSemana = (primerDia + i - 1) % 7;
    // Imprimir el día
    tft.setCursor(20 + x, y);
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE);
    if (i == dia) {
      tft.setTextColor(TFT_YELLOW);
      tft.setTextSize(2);
    } else if (diaDeLaSemana == 5 || diaDeLaSemana == 6) {
      tft.setTextColor(TFT_ORANGE);
    } else {  // Días regulares
      tft.setTextColor(TFT_WHITE);
    }
    tft.print(i);
    // Mover a la siguiente columna
    x += 40;

    // Si hemos llegado al final de la fila (7 días), pasamos a la siguiente fila
    if (x >= 7 * 40) {
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
bool esPrimo(uint32_t n) {
  // Si el número es menor que 2, no es primo
  if (n <= 1) return false;

  // Caso base para 2 y 3
  if (n == 2 || n == 3) return true;

  // Eliminar los números pares
  if (n % 2 == 0) return false;

  // Solo verificamos hasta la raíz cuadrada de n
  uint32_t limite = sqrt(n);  // Usar la raíz cuadrada para limitar las verificaciones

  // Verificar divisibilidad solo con números impares
  for (uint32_t i = 3; i <= limite; i += 2) {
    if (n % i == 0) {
      return false;  // Si encontramos un divisor, el número no es primo
    }
  }

  return true;  // Si no encontramos divisores, el número es primo
}

double moonPhase::_fhour(const struct tm& timeinfo) {
  return timeinfo.tm_hour + map((timeinfo.tm_min * 60) + timeinfo.tm_sec, 0, 3600, 0.0, 1.0);
}

static double _Julian(int32_t year, int32_t month, const double& day) {
  int32_t b, c, e;
  b = 0;
  if (month < 3) {
    year--;
    month += 12;
  }
  if (year > 1582 || (year == 1582 && month > 10) || (year == 1582 && month == 10 && day > 15)) {
    int32_t a;
    a = year / 100;
    b = 2 - a + a / 4;
  }
  c = 365.25 * year;
  e = 30.6001 * (month + 1);
  return b + c + e + day + 1720994.5;
}

static double _sun_position(const double& j) {
  double n, x, e, l, dl, v;
  int32_t i;
  n = 360 / 365.2422 * j;
  i = n / 360;
  n = n - i * 360.0;
  x = n - 3.762863;
  x += (x < 0) ? 360 : 0;
  x *= DEG_TO_RAD;
  e = x;
  do {
    dl = e - .016718 * sin(e) - x;
    e = e - dl / (1 - .016718 * cos(e));
  } while (fabs(dl) >= 1e-12);
  v = 360 / PI * atan(1.01686011182 * tan(e / 2));
  l = v + 282.596403;
  i = l / 360;
  l = l - i * 360.0;
  return l;
}

static double _moon_position(const double& j, const double& ls) {
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

moonData_t moonPhase::_getPhase(const int32_t year, const int32_t month, const int32_t day, const double& hour) {
  /*
  Calculates the phase of the moon at the given epoch.
  returns the moon phase angle as an int (0-360)
  returns the moon percentage that is lit as a real number (0-1)
*/
  const double j{ _Julian(year, month, (double)day + hour / 24.0) - 2444238.5 };
  const double ls{ _sun_position(j) };
  const double lm{ _moon_position(j, ls) };
  double angle = lm - ls;
  angle += (angle < 0) ? 360 : 0;
  const moonData_t returnValue{
    (int32_t)angle,
    (1.0 - cos((lm - ls) * DEG_TO_RAD)) / 2
  };
  return returnValue;
}

void incrementCounter() {
  secondCounter++;
  if (secondCounter >= 59) {
    colorIndex = esp_random() % (sizeof(colors) / sizeof(colors[0]));
    secondCounter = 0;
  } else {
    if (colorIndex % 2 == 0) {
      nombrecillo = " - M 8 A X -";
    } else {
      nombrecillo = "- MvIiIaX -";
    }
  }
}

void tDisplay_MinerScreen(unsigned long mElapsed) {
  mining_data data = getMiningData(mElapsed);
  clock_data dataa = getClockData(mElapsed);
  monedilla = getCoinData(mElapsed);
  relojete = getClockData(mElapsed);
  mineria = getMiningData(mElapsed);
  int horiac = dataa.currentTime.substring(0, 2).toInt();
  incrementCounter();
  actualizarcalen = 0;
  actualizarc = 0;
  actual = 0;
  actuanot = 0;
  correccion = 0;
  background.pushImage(0, 0, MinerWidth, MinerHeight, MinerScreen);
  Serial.printf("M8AX - >>> Completados %s Share(s), %s Khashes, Prom. Hashrate %s KH/s %s°\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str(), data.temp.c_str());

  // Hashrate Day And Night
  render.setFontSize(30);
  render.setCursor(11, 128);
  if (horiac >= 22 || horiac < 8) {
    render.setFontColor(TFT_BLACK);
    render.rdrawString(data.currentHashRate.c_str(), 128, 120, TFT_BLACK);
  } else {
    render.setFontColor(TFT_WHITE);
    render.rdrawString(data.currentHashRate.c_str(), 128, 120, TFT_WHITE);
  }
  // Total hashes
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
  background.setFreeFont(FSSBO9);
  background.setTextSize(0);
  background.setTextDatum(TL_DATUM);
  background.setTextColor(TFT_BLACK);
  background.drawString(nombrecillo, 25, 101, GFXFF);
  // Valid Blocks
  int validInt = atoi(data.valids.c_str());
  if (validInt == 0) {
    render.setFontSize(24);
    render.drawString(data.valids.c_str(), 281, 56, TFT_RED);
  } else {
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
  // Push prepared background to screen
  String lastTwoDigits = data.timeMining;
  String lastTwo = lastTwoDigits.substring(lastTwoDigits.length() - 2);
  int lastTwoInt = lastTwo.toInt();
  int randomTick = (esp_random() % 10) + 1;
  if (lastTwoInt % 2 == 0) {
    // Es par
    background.setFreeFont(FSSBO9);
    background.setTextSize(2);
    background.setTextDatum(TL_DATUM);
    background.setTextColor(colors[colorIndex]);
    if (randomTick % 2 == 0) {
      background.drawString("...", 275, 68, GFXFF);
    } else {
      background.drawString(". .", 275, 68, GFXFF);
    }
  } else {
    // Es impar
    background.setFreeFont(FSSBO9);
    background.setTextSize(2);
    background.setTextDatum(TL_DATUM);
    background.setTextColor(colors[colorIndex]);
    if (randomTick % 2 == 0) {
      background.drawString(".  ", 275, 68, GFXFF);
    } else {
      background.drawString("   ", 275, 68, GFXFF);
    }
  }
  background.pushSprite(0, 0);
}

void tDisplay_ClockScreen(unsigned long mElapsed) {
  clock_data data = getClockData(mElapsed);
  mining_data dataa = getMiningData(mElapsed);
  monedilla = getCoinData(mElapsed);
  relojete = getClockData(mElapsed);
  mineria = getMiningData(mElapsed);
  moonData_t moon;
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

  // Hashrate
  render.setFontSize(21);
  render.setCursor(19, 126);
  render.setFontColor(TFT_BLACK);
  render.rdrawString(data.currentHashRate.c_str(), 94, 133, TFT_BLACK);

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
  timeinfo.tm_year = anio - 1900;  // Año desde 1900
  timeinfo.tm_mon = mes - 1;       // Mes (0 = enero)
  timeinfo.tm_mday = dia;          // Día del mes
  timeinfo.tm_hour = hora;         // Hora
  timeinfo.tm_min = minuto;        // Minutos
  timeinfo.tm_sec = 0;             // Segundos
  timeinfo.tm_isdst = -1;          // Determina si es horario de verano (automático)

  // Convertir a time_t
  time_t cadenaDeTiempo = mktime(&timeinfo);
  moon = moonPhase.getPhase(cadenaDeTiempo);
  double porcentajeIluminado = moon.percentLit * 100;
  char porcentajeTexto[10];
  snprintf(porcentajeTexto, sizeof(porcentajeTexto), "%.2f%%", porcentajeIluminado);
  background.setFreeFont(FSSBO9);
  background.setTextSize(1);
  background.setTextDatum(TL_DATUM);
  background.setTextColor(colors[colorIndex]);
  String textoFinal = "Luna.Ilu - " + String(porcentajeTexto);
  background.drawString(textoFinal, 156, 106, GFXFF);

  // Push prepared background to screen
  background.pushSprite(0, 0);
}

void tDisplay_GlobalHashScreen(unsigned long mElapsed) {
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


void tDisplay_BTCprice(unsigned long mElapsed) {
  clock_data data = getClockData(mElapsed);
  mining_data dataa = getMiningData(mElapsed);
  monedilla = getCoinData(mElapsed);
  relojete = getClockData(mElapsed);
  mineria = getMiningData(mElapsed);
  unsigned long segundo = timeClient.getSeconds();
  int segundos = segundo % 60;
  // data.currentDate ="01/12/2023";
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
  render.setCursor(19, 126);
  render.setFontColor(TFT_BLACK);
  render.rdrawString(data.currentHashRate.c_str(), 94, 133, TFT_BLACK);

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
  rndnumero2++;
  if (rndnumero2 % 2 == 0) {
    textoFinalm8ax1 = "El Futuro No Esta";
    textoFinalm8ax2 = "Establecido, Solo";
    textoFinalm8ax3 = "Existe El Que";
    textoFinalm8ax4 = "Nosotros Hacemos.";
  } else {
    textoFinalm8ax1 = "Por Muchas Vueltas";
    textoFinalm8ax2 = "Que Demos, Siempre";
    textoFinalm8ax3 = "Tendremos El";
    textoFinalm8ax4 = "Culo Atras...";
  }
  background.drawString(textoFinalm8ax1.c_str(), 310, 60, GFXFF);
  background.drawString(textoFinalm8ax2.c_str(), 310, 76, GFXFF);
  background.drawString(textoFinalm8ax3.c_str(), 310, 95, GFXFF);
  background.drawString(textoFinalm8ax4.c_str(), 310, 111, GFXFF);

  // Push prepared background to screen
  background.pushSprite(0, 0);
}

void tDisplay_m8axScreen1(unsigned long mElapsed) {
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
  // Inicializar la estructura tm
  struct tm timeinfo;
  timeinfo.tm_year = anio - 1900;  // Año desde 1900
  timeinfo.tm_mon = mes - 1;       // Mes (0 = enero)
  timeinfo.tm_mday = dia;          // Día del mes
  timeinfo.tm_hour = hora;         // Hora
  timeinfo.tm_min = minuto;        // Minutos
  timeinfo.tm_sec = 0;             // Segundos
  timeinfo.tm_isdst = -1;          // Determina si es horario de verano (automático)
  // Convertir a time_t
  time_t cadenaDeTiempo = mktime(&timeinfo);
  moon = moonPhase.getPhase(cadenaDeTiempo);
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
  background.drawString(datoslunarelogrande, 15, 140, GFXFF);
  background.pushSprite(0, 0);
}

void tDisplay_m8axScreen2(unsigned long mElapsed) {
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
  const char* mensajericono;
  if (millonario == 0) {
    mensajericono = "--- AUN NO ERES RICO ---";
  } else {
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
  if (puntoIndex != -1) {
    // Si hay punto, separar dirección e identificador
    direccion_btc = wallet.substring(0, puntoIndex);
    identificador = wallet.substring(puntoIndex + 1);
    // Generar la nueva wallet con identificador + últimos 6 caracteres
    nueva_wallet = identificador + direccion_btc.substring(direccion_btc.length() - 6);
  } else {
    // Si no hay punto, toda la cadena es la dirección BTC
    direccion_btc = wallet;
    // Generar la nueva wallet con los últimos 12 caracteres
    nueva_wallet = "xxxxxxxxxxxx"+direccion_btc.substring(direccion_btc.length() - 12);
  }
  tft.setCursor(82,117);
  tft.print("WBTC "+nueva_wallet);
  background.drawString(Settings.PoolAddress + ":" + String(Settings.PoolPort), 80, 127, GFXFF);
  background.drawString("c", 62, 107, GFXFF);
  background.setTextColor(colors[colorIndex]);
  background.drawString("d", 125, 92, GFXFF);
  background.drawString("KH/s", 275, 87, GFXFF);
  background.drawString(relojete.currentTime, 12,150, GFXFF);
  render.setCursor(11, 128);
  render.setFontSize(14);
  render.rdrawString(data.timeMining.c_str(), 232, 92, colors[colorIndex]);
}

void tDisplay_m8axScreen3(unsigned long mElapsed) {
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
  int num1 = dataa.currentTime.charAt(0) - '0';  // Primer dígito de la hora
  int num2 = dataa.currentTime.charAt(1) - '0';  // Segundo dígito de la hora
  int num3 = dataa.currentTime.charAt(3) - '0';  // Primer dígito de los minutos
  int num4 = dataa.currentTime.charAt(4) - '0';  // Segundo dígito de los minutos
  if (actualizarcalen % 30 == 0 || actual == 0) {
    background.pushImage(0, 0, ImagenM8AXWidth, ImagenM8AXHeight, ImagenM8AX);
    background.pushSprite(0, 0);
    mostrarCalendario(dia, mes, anio, num1, num2, num3, num4);
  }
  actualizarcalen++;
  actual++;
  actualizarc = 0;
}

void tDisplay_m8axScreen4(unsigned long mElapsed) {
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
  if (esPrimo(rndnumero)) {
    background.pushImage(0, 0, CreditosScreenWidth, CreditosScreenHeight, CreditosScreen);
    String rndnumeroStr = String(rndnumero);
    tft.fillRect(167, 137, 123, 19, TFT_BLACK);
    tft.setCursor(169, 139);
    tft.setTextSize(2);
    tft.print(rndnumeroStr.c_str());
  }
  if (actualizarc % 30 == 0) {
    background.pushImage(0, 0, CreditosScreenWidth, CreditosScreenHeight, CreditosScreen);
    background.pushSprite(0, 0);
    String numerosp = generarNumerosPrimitiva();
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(1);
    String primeros_tres = "";
    String ultimos_tres = "";
    int comaIndex = numerosp.indexOf(',');
    // Primeros tres números
    primeros_tres = numerosp.substring(0, comaIndex);  // Extrae los tres primeros números
    numerosp.remove(0, comaIndex + 1);                 // Elimina lo que ya hemos extraído (primeros tres)

    comaIndex = numerosp.indexOf(',');
    primeros_tres += "," + numerosp.substring(0, comaIndex);  // Agregar el segundo número
    numerosp.remove(0, comaIndex + 1);                        // Elimina el segundo número

    comaIndex = numerosp.indexOf(',');
    primeros_tres += "," + numerosp.substring(0, comaIndex);  // Agregar el tercer número
    numerosp.remove(0, comaIndex + 1);                        // Elimina el tercer número

    // Últimos tres números
    ultimos_tres = numerosp.substring(0);  // Lo que queda es la parte de los últimos tres números
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
    unsigned long psramTotal = ESP.getPsramSize();  // Método directo para PSRAM
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

void tDisplay_m8axScreen5(unsigned long mElapsed) {
  incrementCounter();
  actualizarcalen = 0;
  mining_data data = getMiningData(mElapsed);
  monedilla = getCoinData(mElapsed);
  relojete = getClockData(mElapsed);
  mineria = getMiningData(mElapsed);
  char bbuffer[100];
  snprintf(bbuffer, sizeof(bbuffer), "C. %s Share(s) %s kH Med. HR %s KH/s %sg",
           data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str(), data.temp.c_str());
  Serial.printf("M8AX - >>> Completados %s Share(s), %s Khashes, Prom. Hashrate %s KH/s %s°\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str(), data.temp.c_str());
  String lineaa = bbuffer;
  if (columna == 0) {
    random_number = 1 + (esp_random() % 4);
  }
  if (random_number == 1) {
    tft.pushImage(0, 0, ImagenM8AXWidth, ImagenM8AXHeight, ImagenM8AX);
  } else if (random_number == 2) {
    tft.pushImage(0, 0, M8AXRelojLunarWidth, M8AXRelojLunarHeight, M8AXRelojLunar);
  } else if (random_number == 3) {
    tft.pushImage(0, 0, M8AXQuote1Width, M8AXQuote1Height, M8AXQuote1);
  } else if (random_number == 4) {
    tft.pushImage(0, 0, M8AXQuote2Width, M8AXQuote2Height, M8AXQuote2);
  }
  tft.setTextColor(colors[colorIndex]);
  tft.setTextSize(1);
  tft.setCursor(2, columna);
  tft.print(lineaa);
  columna += 1;
  if (columna >= 166) {
    columna = 0;  // Reinicia la columna
  }
}

void tDisplay_m8axScreen6(unsigned long mElapsed) {
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
  tft.pushImage(0, 0, ImagenFinalPWidth, ImagenFinalPHeight, ImagenFinalPM8AX);  // Muestra la imagen
  tft.setTextColor(colors[colorIndex]);
  tft.setTextSize(1);
  tft.drawString(data.currentTime + "             " + data.currentHashRate + "KH/s " + data.temp + "g", 65, 160);
}

void tDisplay_m8axScreen7(unsigned long mElapsed) {
  mining_data data = getMiningData(mElapsed);
  clock_data dataa = getClockData(mElapsed);
  monedilla = getCoinData(mElapsed);
  relojete = getClockData(mElapsed);
  mineria = getMiningData(mElapsed);
  int millonario = atoi(data.valids.c_str());
  actualizarc = 0;
  incrementCounter();
  if (actualizarcalen == 0) {
    timeClient.begin();
  }
  if (actualizarcalen % 1800 == 0) {
    timeClient.update();
    ;
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
  if (horas >= 12) {
    horas -= 12;
  }

  // Llama a la función para dibujar el reloj
  actualizarcalen++;
  dibujarReloj(horras, minutos, segundos, dia, mes, anio, quediase, data.currentHashRate.c_str(), data.temp.c_str(), millonario);
  actual = 0;
}

void tDisplay_m8axScreen8(unsigned long mElapsed) {
  mining_data data = getMiningData(mElapsed);
  clock_data dataa = getClockData(mElapsed);
  monedilla = getCoinData(mElapsed);
  relojete = getClockData(mElapsed);
  mineria = getMiningData(mElapsed);
  incrementCounter();
  Serial.printf("M8AX - >>> Completados %s Share(s), %s Khashes, Prom. Hashrate %s KH/s %s°\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str(), data.temp.c_str());
  if (actualizarcalen % 15 == 0 || correccion == 0) {
    numfrases++;
    int random_number = 1 + (esp_random() % 4);
    if (random_number == 1) {
      background.pushImage(0, 0, ImagenM8AXWidth, ImagenM8AXHeight, ImagenM8AX);
      background.pushSprite(0, 0);
    } else if (random_number == 2) {
      background.pushImage(0, 0, M8AXRelojLunarWidth, M8AXRelojLunarHeight, M8AXRelojLunar);
      background.pushSprite(0, 0);
    } else if (random_number == 3) {
      background.pushImage(0, 0, M8AXQuote1Width, M8AXQuote1Height, M8AXQuote1);
      background.pushSprite(0, 0);
    } else if (random_number == 4) {
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
    if (millonario == 0) {
      tft.print(data.currentTime + " - " + dataa.currentDate + " - " + data.currentHashRate + " KH/s - " + data.temp + " Grados." + " NO RICO");
    } else {
      tft.print(data.currentTime + " - " + dataa.currentDate + " - " + data.currentHashRate + " KH/s - " + data.temp + " Grados." + " SI RICO");
    }
  }
  actualizarcalen++;
  actualizarc = 0;
  actuanot = 0;
}

void tDisplay_m8axScreen9(unsigned long mElapsed) {
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
  if (segundos % 59 == 0) {
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
  if (actualizarc == 0) {
    random_number = 1 + (esp_random() % 4);
  }
  if (random_number == 1) {
    background.pushImage(0, 0, ImagenM8AXWidth, ImagenM8AXHeight, ImagenM8AX);
    background.pushSprite(0, 0);
  } else if (random_number == 2) {
    background.pushImage(0, 0, M8AXRelojLunarWidth, M8AXRelojLunarHeight, M8AXRelojLunar);
    background.pushSprite(0, 0);
  } else if (random_number == 3) {
    background.pushImage(0, 0, M8AXQuote1Width, M8AXQuote1Height, M8AXQuote1);
    background.pushSprite(0, 0);
  } else if (random_number == 4) {
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
  if (millonario == 1) {
    tft.setCursor(250, 72);
    colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
    tft.setTextColor(colors[colorI]);
    tft.print("ERES RICO");
  } else {
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

void tDisplay_m8axScreen10(unsigned long mElapsed) {
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
  if (segundos % 59 == 0) {
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
  if (actualizarc == 0) {
    random_number = 1 + (esp_random() % 4);
  }
  if (random_number == 1) {
    background.pushImage(0, 0, ImagenM8AXWidth, ImagenM8AXHeight, ImagenM8AX);
    background.pushSprite(0, 0);
  } else if (random_number == 2) {
    background.pushImage(0, 0, M8AXRelojLunarWidth, M8AXRelojLunarHeight, M8AXRelojLunar);
    background.pushSprite(0, 0);
  } else if (random_number == 3) {
    background.pushImage(0, 0, M8AXQuote1Width, M8AXQuote1Height, M8AXQuote1);
    background.pushSprite(0, 0);
  } else if (random_number == 4) {
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
  if (millonario == 1) {
    tft.setCursor(240, 72);
    colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
    tft.setTextColor(colors[colorI]);
    tft.print("ERES RICO");
  } else {
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

void tDisplay_m8axScreen11(unsigned long mElapsed) {
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
  if (segundos % 59 == 0) {
    actualizarc = 0;
  }
  std::string quediase = obtenerDiaSemana(std::string(dataa.currentDate.c_str()));
  int dia = dataa.currentDate.substring(0, 2).toInt();
  int mes = dataa.currentDate.substring(3, 5).toInt();
  String anio = dataa.currentDate.substring(6, 10);
  int anio2d = anio.substring(0, 2).toInt();   // Extraer los primeros dos dígitos
  int aniol2d = anio.substring(2, 4).toInt();  // Extraer los últimos dos dígitos
  String TempCPU = Amorse(std::stoi(data.temp.c_str()));
  String hRoman = Amorse(horas);
  String mRoman = Amorse(minutos);
  String sRoman = Amorse(segundos);
  String RDia = Amorse(dia);
  String RMes = Amorse(mes);
  String RAnio = Amorse(anio2d);
  String RAnio2 = Amorse(aniol2d);
  if (actualizarc == 0) {
    random_number = 1 + (esp_random() % 4);
  }
  if (random_number == 1) {
    background.pushImage(0, 0, ImagenM8AXWidth, ImagenM8AXHeight, ImagenM8AX);
    background.pushSprite(0, 0);
  } else if (random_number == 2) {
    background.pushImage(0, 0, M8AXRelojLunarWidth, M8AXRelojLunarHeight, M8AXRelojLunar);
    background.pushSprite(0, 0);
  } else if (random_number == 3) {
    background.pushImage(0, 0, M8AXQuote1Width, M8AXQuote1Height, M8AXQuote1);
    background.pushSprite(0, 0);
  } else if (random_number == 4) {
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
  tft.setCursor(240, 38);
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
  if (millonario == 1) {
    tft.setCursor(240, 80);
    colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
    tft.setTextColor(colors[colorI]);
    tft.print("ERES RICO");
  } else {
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

void tDisplay_m8axScreen12(unsigned long mElapsed) {
  mining_data data = getMiningData(mElapsed);
  clock_data dataa = getClockData(mElapsed);
  monedilla = getCoinData(mElapsed);
  relojete = getClockData(mElapsed);
  mineria = getMiningData(mElapsed);
  incrementCounter();
  Serial.printf("M8AX - >>> Completados %s Share(s), %s Khashes, Prom. Hashrate %s KH/s %s°\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str(), data.temp.c_str());
  int random_number = 1 + (esp_random() % 4);
  switch (random_number) {
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
  if (colorI % 2 == 0) {
    limite = esp_random() % 15000;  // Rango: 0 a 54400
  } else {
    limite = esp_random() % 5000;  // Rango: 0 a 54400
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
  for (int i = 0; i < limite; i++) {
    colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
    int ancho = esp_random() % 341;  // Rango: 0 a 340
    int alto = esp_random() % 171;   // Rango: 0 a 170
    tft.drawPixel(ancho, alto, colors[colorI]);
  }
  tft.setTextSize(1);
  actualizarcalen = 0;
  actualizarc = 0;
  actual = 0;
  actuanot = 0;
  correccion = 0;
}

void tDisplay_m8axScreen13(unsigned long mElapsed) {
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
  if (minutos % 2 == 0 && segundos <= 1) {
    actualizarc = 0;
  }
  if (actualizarc == 0) {
    random_number = 1 + (esp_random() % 4);
    if (random_number == 1) {
      background.pushImage(0, 0, ImagenM8AXWidth, ImagenM8AXHeight, ImagenM8AX);
      background.pushSprite(0, 0);
    } else if (random_number == 2) {
      background.pushImage(0, 0, M8AXRelojLunarWidth, M8AXRelojLunarHeight, M8AXRelojLunar);
      background.pushSprite(0, 0);
    } else if (random_number == 3) {
      background.pushImage(0, 0, M8AXQuote1Width, M8AXQuote1Height, M8AXQuote1);
      background.pushSprite(0, 0);
    } else if (random_number == 4) {
      background.pushImage(0, 0, M8AXQuote2Width, M8AXQuote2Height, M8AXQuote2);
      background.pushSprite(0, 0);
    }
  }
  Serial.printf("M8AX - >>> Completados %s Share(s), %s Khashes, Prom. Hashrate %s KH/s %s°\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str(), data.temp.c_str());
  if (actualizarc == 0) {
    for (int i = 0; i < 20; i++) {
      float precio = obtenerPrecio(criptomonedas[i]);
      taskYIELD();
      if (precio != -1) {
        if (i < 5) {
          // Primera columna
          x = 5;
          y = 20 + i * 30;
        } else if (i < 10) {
          // Segunda columna
          x = tft.width() / 4 + 10;
          y = 20 + (i - 5) * 30;
        } else if (i < 15) {
          // Tercera columna
          x = 2 * (tft.width() / 4) + 15;
          y = 20 + (i - 10) * 30;
        } else {
          // Cuarta columna
          x = 3 * (tft.width() / 4) + 20;
          y = 20 + (i - 15) * 30;
        }
        colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
        tft.setTextColor(colors[colorI]);
        tft.setTextSize(1);
        tft.drawLine(0, 13, 320, 13, colors[colorI]);  // Dibujar línea de (0, y) a (320, y)
        String Textito = "M8AX - CryptoChrono 2Min - " + String(horas) + ":" + String(minutos) + ":" + String(minutos) + " - " + data.currentHashRate + " KH/s";
        tft.setCursor(16, 2);
        tft.print(Textito);
        tft.setCursor(x, y);
        colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
        tft.setTextColor(colors[colorI]);
        tft.print(criptomonedas[i]);
        tft.setCursor(x, y + 15);
        tft.print("$");
        tft.print(precio);
      } else {
        if (i < 5) {
          // Primera columna
          x = 5;
          y = 20 + i * 30;
        } else if (i < 10) {
          // Segunda columna
          x = tft.width() / 4 + 10;
          y = 20 + (i - 5) * 30;
        } else if (i < 15) {
          // Tercera columna
          x = 2 * (tft.width() / 4) + 15;
          y = 20 + (i - 10) * 30;
        } else {
          // Cuarta columna
          x = 3 * (tft.width() / 4) + 20;
          y = 20 + (i - 15) * 30;
        }
        colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
        tft.setTextColor(colors[colorI]);
        tft.setTextSize(1);
        tft.drawLine(0, 13, 320, 13, colors[colorI]);  // Dibujar línea de (0, y) a (320, y)
        String Textito = "M8AX - CryptoChrono 2Min - " + String(horas) + ":" + String(minutos) + ":" + String(minutos) + " - " + data.currentHashRate + " KH/s";
        tft.setCursor(16, 2);
        tft.print(Textito);
        tft.setCursor(x, y);
        colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
        tft.setTextColor(colors[colorI]);
        tft.print(criptomonedas[i]);
        tft.setCursor(x, y + 15);
        tft.print("$-ERROR-$");
        Serial.print("Error al obtener el precio de ");
        Serial.println(criptomonedas[i]);
      }
    }
    actualizarc++;
    actualizarcalen = 0;
    actual = 0;
    actuanot = 0;
    correccion = 0;
  }
}

void tDisplay_m8axScreen14(unsigned long mElapsed) {
  mining_data data = getMiningData(mElapsed);
  clock_data dataa = getClockData(mElapsed);
  monedilla = getCoinData(mElapsed);
  relojete = getClockData(mElapsed);
  mineria = getMiningData(mElapsed);
  incrementCounter();
  Serial.printf("M8AX - >>> Completados %s Share(s), %s Khashes, Prom. Hashrate %s KH/s %s°\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str(), data.temp.c_str());
  if (actuanot % 59 == 0 || correccion == 0) {
    numnotis++;
    int random_number = 1 + (esp_random() % 4);
    if (random_number == 1) {
      background.pushImage(0, 0, ImagenM8AXWidth, ImagenM8AXHeight, ImagenM8AX);
      background.pushSprite(0, 0);
    } else if (random_number == 2) {
      background.pushImage(0, 0, M8AXRelojLunarWidth, M8AXRelojLunarHeight, M8AXRelojLunar);
      background.pushSprite(0, 0);
    } else if (random_number == 3) {
      background.pushImage(0, 0, M8AXQuote1Width, M8AXQuote1Height, M8AXQuote1);
      background.pushSprite(0, 0);
    } else if (random_number == 4) {
      background.pushImage(0, 0, M8AXQuote2Width, M8AXQuote2Height, M8AXQuote2);
      background.pushSprite(0, 0);
    }

    obtenerNoticias();
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(112, 160);
    tft.setTextSize(1);
    tft.print("M8AX-Noticia Num-" + String(numnotis));
    tft.setTextColor(TFT_WHITE);
    tft.drawLine(0, 20, 320, 20, colors[colorI]);  // Dibujar línea de (0, y) a (320, y)
    tft.setCursor(1, 6);
    tft.setTextSize(1);
    correccion = 1;
    int millonario = atoi(data.valids.c_str());
    if (millonario == 0) {
      tft.print(data.currentTime + " - " + dataa.currentDate + " - " + data.currentHashRate + " KH/s - " + data.temp + " Grados." + " NO RICO");
    } else {
      tft.print(data.currentTime + " - " + dataa.currentDate + " - " + data.currentHashRate + " KH/s - " + data.temp + " Grados." + " SI RICO");
    }
  }
  actuanot++;
  actualizarcalen = 0;
  actualizarc = 0;
  actual = 0;
}

void tDisplay_m8axScreen15(unsigned long mElapsed) {
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
  if (actualizarcalen == 0) {
    background.pushImage(0, 0, ImagenM8AXWidth, ImagenM8AXHeight, ImagenM8AX);
    background.pushSprite(0, 0);
    actualizarcalen++;
  }
  background.pushSprite(0, 0);
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  Serial.printf("M8AX - >>> Completados %s Share(s), %s Khashes, Prom. Hashrate %s KH/s %s°\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str(), data.temp.c_str());
  if (segundo % 30 == 0) {
    int random_number = 1 + (esp_random() % 4);
    if (random_number == 1) {
      background.pushImage(0, 0, ImagenM8AXWidth, ImagenM8AXHeight, ImagenM8AX);
      background.pushSprite(0, 0);
    } else if (random_number == 2) {
      background.pushImage(0, 0, M8AXRelojLunarWidth, M8AXRelojLunarHeight, M8AXRelojLunar);
      background.pushSprite(0, 0);
    } else if (random_number == 3) {
      background.pushImage(0, 0, M8AXQuote1Width, M8AXQuote1Height, M8AXQuote1);
      background.pushSprite(0, 0);
    } else if (random_number == 4) {
      background.pushImage(0, 0, M8AXQuote2Width, M8AXQuote2Height, M8AXQuote2);
      background.pushSprite(0, 0);
    }
  }
  const char* hashRateStr = dataa.currentHashRate.c_str();  // Obtener la cadena
  float hashRa = atof(hashRateStr);                         // Convertirla a flotante (float)
  dibujaAnalogKH(hashRa);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(1);
  rndnumero3 = esp_random();
  const char* factorizacion = factorize(rndnumero3);
  tft.setCursor(4, 154);
  tft.print(String(rndnumero3) + " - " + (factorizacion));
  Serial.println("M8AX - Factorizando Número - " + String(rndnumero3) + " - " + (factorizacion));
  colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
  tft.setCursor(4, 144);
  tft.setTextColor(colors[colorI]);
  tft.print("Factorizacion Nerd");
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(2, 6);
  tft.setTextSize(1);
  int millonario = atoi(data.valids.c_str());
  if (millonario == 0) {
    tft.print(data.currentTime + " - " + dataa.currentDate + " - " + data.currentHashRate + " KH/s - " + data.temp + " Grados." + " NO RICO");
  } else {
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
  char buffer[10];                // Buffer para almacenar el número formateado
  dtostrf(tflops, 8, 6, buffer);  // (valor, ancho mínimo, decimales, buffer)
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
  char bufferrr[10];              // Buffer para almacenar el número formateado
  dtostrf(Mops, 8, 6, bufferrr);  // (valor, ancho mínimo, decimales, buffer)
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
  int X_INICIO = 230;                                       // Columna fija
  int X_FINAL = 310;                                        // Base de la barra
  int Y_POS = 147;                                          // Punto más alto
  int longitud_total = X_FINAL - X_INICIO;                  // Longitud total de la barra (80 píxeles)
  int longitud_pintada = (longitud_total * segundos) / 59;  // Porcentaje de la barra según los segundos
  tft.fillRect(X_INICIO, Y_POS - 1, longitud_total + 1, 3, TFT_BLACK);
  tft.drawLine(X_INICIO, Y_POS, X_INICIO + longitud_pintada, Y_POS, TFT_WHITE);
  Y_POS = 135;
  longitud_pintada = (longitud_total * horas) / 23;  // Porcentaje de la barra según las horas
  tft.fillRect(X_INICIO, Y_POS - 1, longitud_total + 1, 3, TFT_BLACK);
  tft.drawLine(X_INICIO, Y_POS, X_INICIO + longitud_pintada, Y_POS, TFT_WHITE);
  Y_POS = 141;
  longitud_pintada = (longitud_total * minutos) / 59;  // Porcentaje de la barra según los minutos
  tft.fillRect(X_INICIO, Y_POS - 1, longitud_total + 1, 3, TFT_BLACK);
  tft.drawLine(X_INICIO, Y_POS, X_INICIO + longitud_pintada, Y_POS, TFT_WHITE);
  actualizarcalen = 0;
  actualizarc = 0;
  actuanot = 0;
}

void tDisplay_m8axScreen16(unsigned long mElapsed) {
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
  if (actualizarcalen == 0) {
    background.pushImage(0, 0, ImagenM8AXWidth, ImagenM8AXHeight, ImagenM8AX);
    background.pushSprite(0, 0);
    actualizarcalen++;
  }
  background.pushSprite(0, 0);
  Serial.printf("M8AX - >>> Completados %s Share(s), %s Khashes, Prom. Hashrate %s KH/s %s°\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str(), data.temp.c_str());
  if (segundo % 30 == 0) {
    int random_number = 1 + (esp_random() % 4);
    if (random_number == 1) {
      background.pushImage(0, 0, ImagenM8AXWidth, ImagenM8AXHeight, ImagenM8AX);
      background.pushSprite(0, 0);
    } else if (random_number == 2) {
      background.pushImage(0, 0, M8AXRelojLunarWidth, M8AXRelojLunarHeight, M8AXRelojLunar);
      background.pushSprite(0, 0);
    } else if (random_number == 3) {
      background.pushImage(0, 0, M8AXQuote1Width, M8AXQuote1Height, M8AXQuote1);
      background.pushSprite(0, 0);
    } else if (random_number == 4) {
      background.pushImage(0, 0, M8AXQuote2Width, M8AXQuote2Height, M8AXQuote2);
      background.pushSprite(0, 0);
    }
  }
  int y = 30;  // Coordenada Y inicial
  int horitaUTC = horita - (esHorarioDeVerano(currentMonth, currentDay) ? 2 : 1);
  for (int i = 0; i < 20; i++) {
    // Calcula la hora local para cada ciudad, ajustando con la zona horaria de cada ciudad
    int ciudadHora = horitaUTC + zonasHorarias[i];  // Ajusta la hora sumando la zona horaria de cada ciudad

    // Ajusta la hora si excede 24 horas o es menor a 0
    if (ciudadHora < 0) {
      ciudadHora += 24;
    }
    if (ciudadHora >= 24) {
      ciudadHora -= 24;
    }

    // Formatea la hora (sin segundos)
    char timeStr[9];
    sprintf(timeStr, "%02d:%02d:%02d", ciudadHora, minutitos, segundos);

    // Calcula la posición X en base a la columna (4 columnas)
    int x = (i % 4) * 80;  // Las ciudades se distribuyen en 4 columnas

    // Muestra la ciudad y la hora
    colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
    tft.drawLine(0, 20, 320, 20, colors[colorI]);  // Dibujar línea de (0, y) a (320, y)
    tft.setTextColor(colors[colorI]);
    tft.setTextSize(1);
    tft.setCursor(x + 5, y);
    tft.print(ciudades[i]);
    tft.setCursor(x + 5, y + 10);  // Posición de la hora
    tft.print(timeStr);
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(1, 6);
    tft.setTextSize(1);
    int millonario = atoi(data.valids.c_str());
    if (millonario == 0) {
      tft.print(data.currentTime + " - " + dataa.currentDate + " - " + data.currentHashRate + " KH/s - " + data.temp + " Grados." + " NO RICO");
    } else {
      tft.print(data.currentTime + " - " + dataa.currentDate + " - " + data.currentHashRate + " KH/s - " + data.temp + " Grados." + " SI RICO");
    }

    // Cambia la posición Y para la siguiente fila (5 filas)
    if ((i + 1) % 4 == 0) {  // Se cambia después de cada 4 columnas
      y += 30;               // Aumenta la posición Y después de cada 4 ciudades
    }
  }
  actualizarc = 0;
  actuanot = 0;
}

void tDisplay_m8axScreen17(unsigned long mElapsed) {
  mining_data data = getMiningData(mElapsed);
  monedilla = getCoinData(mElapsed);
  relojete = getClockData(mElapsed);
  mineria = getMiningData(mElapsed);
  String btcm8 = "bitcoin:bc1qljq00pm2plq2l9jxzdzt0xc8t79j9wcmu7r8em";
  unsigned long segundo = timeClient.getSeconds();
  incrementCounter();
  if (actualizarcalen == 0) {
    background.pushImage(0, 0, ImagenM8AXWidth, ImagenM8AXHeight, ImagenM8AX);
    background.pushSprite(0, 0);
    actualizarcalen++;
  }
  background.pushSprite(0, 0);
  Serial.printf("M8AX - >>> Completados %s Share(s), %s Khashes, Prom. Hashrate %s KH/s %s°\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str(), data.temp.c_str());
  if (segundo % 30 == 0) {
    int random_number = 1 + (esp_random() % 4);
    if (random_number == 1) {
      background.pushImage(0, 0, ImagenM8AXWidth, ImagenM8AXHeight, ImagenM8AX);
      background.pushSprite(0, 0);
    } else if (random_number == 2) {
      background.pushImage(0, 0, M8AXRelojLunarWidth, M8AXRelojLunarHeight, M8AXRelojLunar);
      background.pushSprite(0, 0);
    } else if (random_number == 3) {
      background.pushImage(0, 0, M8AXQuote1Width, M8AXQuote1Height, M8AXQuote1);
      background.pushSprite(0, 0);
    } else if (random_number == 4) {
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
  tft.setCursor(128, 159);
  tft.setTextSize(1);
  tft.print(data.currentHashRate + " KH/s");
  uint16_t colorss[] = { TFT_WHITE, TFT_YELLOW, TFT_CYAN, TFT_GREENYELLOW };  // Array de colores
  int colorrr = random(0, 4);
  dibujaQR(btcm8, (320 - 150) / 2, (170 - 150) / 2, 150, colorss[colorrr]);  // Dibuja el QR centrado en la pantalla 320x170
}

void tDisplay_m8axScreen18(unsigned long mElapsed) {
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
  if (actualizarcalen == 0) {
    background.pushImage(0, 0, ImagenM8AXWidth, ImagenM8AXHeight, ImagenM8AX);
    background.pushSprite(0, 0);
    actualizarcalen++;
  }
  background.pushSprite(0, 0);
  Serial.printf("M8AX - >>> Completados %s Share(s), %s Khashes, Prom. Hashrate %s KH/s %s°\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str(), data.temp.c_str());
  if (segundo % 30 == 0) {
    int random_number = 1 + (esp_random() % 4);
    if (random_number == 1) {
      background.pushImage(0, 0, ImagenM8AXWidth, ImagenM8AXHeight, ImagenM8AX);
      background.pushSprite(0, 0);
    } else if (random_number == 2) {
      background.pushImage(0, 0, M8AXRelojLunarWidth, M8AXRelojLunarHeight, M8AXRelojLunar);
      background.pushSprite(0, 0);
    } else if (random_number == 3) {
      background.pushImage(0, 0, M8AXQuote1Width, M8AXQuote1Height, M8AXQuote1);
      background.pushSprite(0, 0);
    } else if (random_number == 4) {
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
  tft.drawLine(0, 100, 320, 100, colors[colorI]);  // Dibujar línea de (0, y) a (320, y)
  int numeritos[6];
  int destino = 1 + (esp_random() % 1000);  // Usa esp_random() en lugar de rand()
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
  if (millonario == 0) {
    tft.print("NoRICO - ");
    tft.print(data.currentHashRate);
    tft.print(" KH/s. ");
    tft.print(data.temp);
    tft.print(" g");
  } else {
    tft.print("SiRICO - ");
    tft.print(data.currentHashRate);
    tft.print(" KH/s. ");
    tft.print(data.temp);
    tft.print(" g");
  }
  tft.drawLine(0, 124, 320, 124, colors[colorI]);  // Dibujar línea de (0, y) a (320, y)
  tft.setCursor(2, 128);
  Serial.print("M8AX - Números: ");
  for (int i = 0; i < 6; i++) {
    Serial.print(numeritos[i]);  // Imprime el número
    Serial.print(" ");           // Imprime un espacio
  }
  Serial.println();
  // Imprime el objetivo
  Serial.print("M8AX - Objetivo: ");
  Serial.println("M8AX . " + String(destino));

  int X_INICIO = 173;                                       // Columna fija
  int X_FINAL = 315;                                        // Base de la barra
  int Y_POS = 107;                                          // Punto más alto
  int longitud_total = X_FINAL - X_INICIO;                  // Longitud total de la barra (80 píxeles)
  int longitud_pintada = (longitud_total * segundos) / 59;  // Porcentaje de la barra según los segundos
  tft.fillRect(X_INICIO, Y_POS - 1, longitud_total + 1, 3, TFT_BLACK);
  tft.drawLine(X_INICIO, Y_POS, X_INICIO + longitud_pintada, Y_POS, TFT_WHITE);

  // Calcula las operaciones

  calculate_operations(numeritos, destino, result);
  tft.print(result);
  uint16_t colorss[] = { TFT_WHITE, TFT_YELLOW, TFT_CYAN, TFT_GREENYELLOW, TFT_LIGHTGREY, TFT_SILVER };  // Array de colores
  int colorrr = random(0, 6);
  dibujaQR(String(horita), 0, 0, 98, colorss[colorrr]);  // Dibuja el QR centrado en la pantalla 320x170
  colorrr = random(0, 6);
  dibujaQR(String(minutitos), 115, 0, 98, colorss[colorrr]);  // Dibuja el QR centrado en la pantalla 320x170
  colorrr = random(0, 6);
  dibujaQR(String(segundos), 228, 0, 98, colorss[colorrr]);  // Dibuja el QR centrado en la pantalla 320x170
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

void monedaYdado(unsigned long mElapsed) {
  mining_data data = getMiningData(mElapsed);
  clock_data dataa = getClockData(mElapsed);
  monedilla = getCoinData(mElapsed);
  relojete = getClockData(mElapsed);
  mineria = getMiningData(mElapsed);
  int horita = dataa.currentTime.substring(0, 2).toInt();
  int minutitos = dataa.currentTime.substring(3, 5).toInt();
  unsigned long segundo = timeClient.getSeconds();
  int segundos = segundo % 60;
  char horaStr[9];  // Buffer para almacenar la hora en formato HH:MM:SS
  sprintf(horaStr, "%02d:%02d:%02d", horita, minutitos, segundos);
  incrementCounter();
  if (actualizarc == 0) {
    background.pushImage(0, 0, ImagenM8AXWidth, ImagenM8AXHeight, ImagenM8AX);
    background.pushSprite(0, 0);
    actualizarc++;
  }
  background.pushSprite(0, 0);
  Serial.printf("M8AX ->>> Completados %s Share(s), %s Khashes, Prom. Hashrate %s KH/s %s°\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str(), data.temp.c_str());
  if (segundo % 30 == 0) {
    int random_number = 1 + (esp_random() % 4);
    if (random_number == 1) {
      background.pushImage(0, 0, ImagenM8AXWidth, ImagenM8AXHeight, ImagenM8AX);
      background.pushSprite(0, 0);
    } else if (random_number == 2) {
      background.pushImage(0, 0, M8AXRelojLunarWidth, M8AXRelojLunarHeight, M8AXRelojLunar);
      background.pushSprite(0, 0);
    } else if (random_number == 3) {
      background.pushImage(0, 0, M8AXQuote1Width, M8AXQuote1Height, M8AXQuote1);
      background.pushSprite(0, 0);
    } else if (random_number == 4) {
      background.pushImage(0, 0, M8AXQuote2Width, M8AXQuote2Height, M8AXQuote2);
      background.pushSprite(0, 0);
    }
  }
  actual = 0;
  correccion = 0;
  actuanot = 0;
  actualizarcalen = 0;
  int dado = esp_random() % 6 + 1;  // Números del 1 al 6
  int moneda = esp_random() % 2;    // 0 (cara), 1 (cruz)
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
  String cadena = data.currentHashRate.c_str();  // O "234,65" si necesitas reemplazar la coma
  cadena.replace(',', '.');                      // Si es necesario
  float valor = cadena.toFloat();
  int parteEntera = (int)valor;
  int parteDecimal = (int)((valor - parteEntera) * 100 + 0.5);
  dibujarDado(dado, 60, 85);
  dibujarMoneda(moneda, 250, 80);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(0, 132);
  tft.setTextSize(2);
  tft.setTextDatum(MC_DATUM);  // Centra el texto
  tft.print(numeroAEscrito(parteEntera, false) + " Con ");
  tft.print(numeroAEscrito(parteDecimal, true) + " KH/s.");
  tft.setCursor(115, 50);
  tft.setTextSize(2);
  tft.print(cadena);
  tft.setCursor(114, 69);
  tft.print(" KH/s");
  tft.setTextSize(1);
}

void tDisplay_m8axvida(unsigned long mElapsed) {
  mining_data data = getMiningData(mElapsed);
  monedilla = getCoinData(mElapsed);
  relojete = getClockData(mElapsed);
  mineria = getMiningData(mElapsed);
  incrementCounter();
  Serial.printf("M8AX ->>> Completados %s Share(s), %s Khashes, Prom. Hashrate %s KH/s %s°\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str(), data.temp.c_str());
  if (actuanot % 10 == 0 || correccion == 0) {
    int random_number = 1 + (esp_random() % 4);
    if (random_number == 1) {
      background.pushImage(0, 0, ImagenM8AXWidth, ImagenM8AXHeight, ImagenM8AX);
      background.pushSprite(0, 0);
    } else if (random_number == 2) {
      background.pushImage(0, 0, M8AXRelojLunarWidth, M8AXRelojLunarHeight, M8AXRelojLunar);
      background.pushSprite(0, 0);
    } else if (random_number == 3) {
      background.pushImage(0, 0, M8AXQuote1Width, M8AXQuote1Height, M8AXQuote1);
      background.pushSprite(0, 0);
    } else if (random_number == 4) {
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
    // Computar Generaciones
    for (int gen = 0; gen < genCount; gen++) {
      computeCA();
      drawGrid();
      delay(GEN_DELAY);
      for (int16_t x = 1; x < GRIDX - 1; x++) {
        for (int16_t y = 1; y < GRIDY - 1; y++) {
          grid[x][y] = newgrid[x][y];
        }
      }
    }
  }
  actualizarcalen = 0;
  actualizarc = 0;
  actual = 0;
}

void RelojDeNumeros(unsigned long mElapsed) {
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
  int horis1 = horas / 10;        // Primer dígito de las horas
  int horis2 = horas % 10;        // Segundo dígito de las horas
  int minutis1 = minutos / 10;    // Primer dígito de los minutos
  int minutis2 = minutos % 10;    // Segundo dígito de los minutos
  int segundis1 = segundos / 10;  // Primer dígito de los segundos
  int segundis2 = segundos % 10;  // Segundo dígito de los segundos

  if (segundos % 59 == 0) {
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
  String hRoman2 = numeroAEscrito(horis2);
  String mRoman1 = numeroAEscrito(minutis1);
  String mRoman2 = numeroAEscrito(minutis2);
  String sRoman1 = numeroAEscrito(segundis1);
  String sRoman2 = numeroAEscrito(segundis2);
  String RDia = numeroAEscrito(dia);
  String RMes = numeroAEscrito(mes);
  String RAnio = String(anio);
  String TempCPU = String(data.temp.c_str());
  if (actualizarc == 0) {
    random_number = 1 + (esp_random() % 4);
  }
  if (random_number == 1) {
    background.pushImage(0, 0, ImagenM8AXWidth, ImagenM8AXHeight, ImagenM8AX);
    background.pushSprite(0, 0);
  } else if (random_number == 2) {
    background.pushImage(0, 0, M8AXRelojLunarWidth, M8AXRelojLunarHeight, M8AXRelojLunar);
    background.pushSprite(0, 0);
  } else if (random_number == 3) {
    background.pushImage(0, 0, M8AXQuote1Width, M8AXQuote1Height, M8AXQuote1);
    background.pushSprite(0, 0);
  } else if (random_number == 4) {
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
  if (millonario == 1) {
    tft.setCursor(250, 72);
    colorI = esp_random() % (sizeof(colors) / sizeof(colors[0]));
    tft.setTextColor(colors[colorI]);
    tft.print("ERES RICO");
  } else {
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
  int X_INICIO = 5;                                                              // Columna fija
  int X_FINAL = 242;                                                             // Base de la barra
  int Y_POS = 43;                                                                // Punto más alto
  int longitud_total = X_FINAL - X_INICIO;                                       // Longitud total de la barra (80 píxeles)
  int longitud_pintada = (longitud_total * data.currentHashRate.toInt()) / 360;  // Porcentaje de la barra según los segundos
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
  X_INICIO = 4;                                                  // Columna fija
  X_FINAL = 242;                                                 // Base de la barra
  Y_POS = 117;                                                   // Punto más alto
  longitud_total = X_FINAL - X_INICIO;                           // Longitud total de la barra (80 píxeles)
  longitud_pintada = (longitud_total * segundosDelDia) / 86400;  // Porcentaje de la barra según los segundos
  tft.fillRect(X_INICIO, Y_POS - (grosor / 2), longitud_total + 1, grosor, TFT_BLACK);
  tft.fillRect(X_INICIO, Y_POS - (grosor / 2), longitud_pintada, grosor, colors[colorI]);
}

void datoTextPlano(unsigned long mElapsed) {
  mining_data data = getMiningData(mElapsed);
  clock_data dataa = getClockData(mElapsed);
  monedilla = getCoinData(mElapsed);
  relojete = getClockData(mElapsed);
  mineria = getMiningData(mElapsed);
  unsigned long segundo = timeClient.getSeconds();
  incrementCounter();
  if (actualizarc == 0) {
    background.pushImage(0, 0, ImagenM8AXWidth, ImagenM8AXHeight, ImagenM8AX);
    background.pushSprite(0, 0);
    actualizarc++;
    background.pushSprite(0, 0);
    recopilaTelegram2();
  }
  Serial.printf("M8AX ->>> Completados %s Share(s), %s Khashes, Prom. Hashrate %s KH/s %s°\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str(), data.temp.c_str());
  if (segundo % 30 == 0) {
    int random_number = 1 + (esp_random() % 4);
    if (random_number == 1) {
      background.pushImage(0, 0, ImagenM8AXWidth, ImagenM8AXHeight, ImagenM8AX);
      background.pushSprite(0, 0);
    } else if (random_number == 2) {
      background.pushImage(0, 0, M8AXRelojLunarWidth, M8AXRelojLunarHeight, M8AXRelojLunar);
      background.pushSprite(0, 0);
    } else if (random_number == 3) {
      background.pushImage(0, 0, M8AXQuote1Width, M8AXQuote1Height, M8AXQuote1);
      background.pushSprite(0, 0);
    } else if (random_number == 4) {
      background.pushImage(0, 0, M8AXQuote2Width, M8AXQuote2Height, M8AXQuote2);
      background.pushSprite(0, 0);
    }
  }
  if (segundo % 5 == 0) {
    background.pushSprite(0, 0);
    recopilaTelegram2();
  }
  actual = 0;
  correccion = 0;
  actuanot = 0;
  actualizarcalen = 0;
}

void tDisplay_LoadingScreen(void) {
  int effect = random(6);  // Selecciona un efecto aleatorio
  switch (effect) {
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
  }
  tft.fillScreen(TFT_BLACK);
  tft.pushImage(0, 0, initWidth, initHeight, initScreen);
  tft.setTextColor(TFT_BLACK);
  tft.setTextSize(1);
  tft.drawString(CURRENT_VERSION, 25, 148, FONT2);
}

void tDisplay_SetupScreen(void) {
  tft.pushImage(0, 0, setupModeWidth, setupModeHeight, setupModeScreen);
}

void analiCadaSegundo(unsigned long frame) {
  unsigned long epochTime = timeClient.getEpochTime();  // Obtener segundos desde 1970
  time_t epoch = (time_t)epochTime;                     // Convertir a time_t
  struct tm* timeinfo = localtime(&epoch);              // Convertir a una estructura de tiempo local
  int dia = timeinfo->tm_mday;                          // Día del mes (1 a 31)
  int mes = timeinfo->tm_mon + 1;                       // Mes (1 a 12)
  int anio = timeinfo->tm_year + 1900;                  // Año (por defecto es desde 1900)
  int horita = timeinfo->tm_hour;                       // Hora
  int minutitos = timeinfo->tm_min;                     // Minutos
  int segundos = timeinfo->tm_sec;                      // Segundos
  int temper=mineria.temp.toInt();
  float currentRate = mineria.currentHashRate.toFloat();

  BOT_TOKEN = Settings.botTelegram;
  CHAT_ID = Settings.ChanelIDTelegram;

  if (currentRate > maxkh) {
    maxkh = currentRate; // Asignar el nuevo valor máximo
  }  

  if (currentRate < minkh) {
     minkh = currentRate; // Asignar el nuevo valor máximo   
  }

  if (temper > maxtemp) {
    maxtemp= temper; // Asignar el nuevo valor máximo
  }  

  if (temper < mintemp) {
     mintemp = temper; // Asignar el nuevo valor máximo
  }

  if (pantallaEncendida == 0 && temper > 70) {
    digitalWrite(TFT_BL, LOW);  // Apaga la pantalla (esto depende de tu configuración)
  }

  // Condición para encender la pantalla si está encendida originalmente (pantallaEncendida == 0) 
  // y la temperatura es menor de 60

  if (pantallaEncendida == 0 && temper < 60) {
    // Enciende la pantalla solo si estaba encendida originalmente
    digitalWrite(TFT_BL, HIGH);  // Enciende la pantalla
  }

  if (ContadorEspecial % 6000 == 0 && temper > 70) {
     tempalert+=1;
  }
  
  if (temper> 84) {
    Serial.println("M8AX - Apagando M8AX-NerdMinerV2 Durante 5 Minutos, Para Proteger La CPU De Temperaturas Muy Altas...");
    esp_sleep_enable_timer_wakeup(300000000);  // 5 minutos en microsegundos (300,000 ms)
    esp_deep_sleep_start();
  }

  // Felicitar La Navidad O El Año Nuevo

  if (((mes == 12 && dia >= 20) || (mes == 1 && dia <= 6)) && anio != 1970) {
    if (minutitos == 30 && ((horita >= 8 && horita <= 15) || (horita >= 19 && horita <= 23) || (horita >= 0 && horita <= 2)) && (horita % 2 == 0)) {
      if (segundos == 0 && dia % 2 == 0) {
        if (mes == 12) {
          nevar();
          Serial.println("M8AX - Felicitando La Navidad...");
          ContadorEspecial = 0;
        } else if (mes == 1) {
          nevar2();
          Serial.println("M8AX - Felicitando El Año Nuevo...");
          ContadorEspecial = 0;
        }
        return;
      }
    }
  }

  rndnumero = esp_random();
  if (rndnumero <= 10031977 && segundos <= 10 && segundos % 2 == 0 && dia % 2 != 0) {
    Serial.printf(">>> M8AX-NerdMinerV2 Dando Ánimos Y Esperanza Al Usuario...\n");
    actualizarcalen = 0;
    actualizarc = 0;
    actual = 0;
    actuanot = 0;
    ContadorEspecial = 0;
    correccion = 0;
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(colors[colorIndex]);
    tft.setTextSize(3);   // Tamaño del texto
    tft.setCursor(0, 0);  // Posición del cursor
    if (rndnumero % 2 == 0) {
      tft.println("La Esperanza Es");
      tft.println("Lo Ultimo Que");
      tft.println("Se Pierde...");
      tft.println("Lo Conseguiremos!");
      tft.println("");
      tft.println("     VAMOS !");
      tft.setTextSize(1);  // Tamaño del texto
      tft.println("");
      tft.setTextSize(2);  // Tamaño del texto
      tft.setTextColor(TFT_WHITE);
      tft.println("     ... By M8AX ...");
    } else {
      tft.println("Hope Is The Last");
      tft.println("Thing To Lose, We");
      tft.println("Will Archieve It!");
      tft.println("");
      tft.println("    COME ON !");
      tft.setTextSize(2);  // Tamaño del texto
      tft.setTextColor(TFT_WHITE);
      tft.println("");
      tft.println("");
      tft.println("     ... By M8AX ...");
    }
    delay(3000);
  }
  
  if (horita % 2 == 0) {
    if (((horita >= 8 && horita < 24) || (horita == 0)) && minutitos == 0 && segundos == 0 && BOT_TOKEN != "NO CONFIGURADO" && CHAT_ID != "NO CONFIGURADO") {
      sumatelegram+=1;
      recopilaTelegram();
    }  
  }
}

void tDisplay_AnimateCurrentScreen(unsigned long frame) {
  ContadorEspecial++;
  if (ContadorEspecial % 5 == 0) {
    analiCadaSegundo(frame);
  }
}

void tDisplay_DoLedStuff(unsigned long frame) {
}

CyclicScreenFunction tDisplayCyclicScreens[] = { tDisplay_MinerScreen, tDisplay_GlobalHashScreen, tDisplay_BTCprice, tDisplay_m8axScreen15, tDisplay_m8axScreen2, datoTextPlano, tDisplay_m8axScreen5, tDisplay_m8axScreen16, tDisplay_ClockScreen, tDisplay_m8axScreen1, tDisplay_m8axScreen7, RelojDeNumeros, tDisplay_m8axScreen9, tDisplay_m8axScreen10, tDisplay_m8axScreen11, tDisplay_m8axScreen18, tDisplay_m8axScreen3, tDisplay_m8axScreen13, tDisplay_m8axScreen14, tDisplay_m8axScreen8, tDisplay_m8axScreen4, tDisplay_m8axScreen6, tDisplay_m8axScreen17, monedaYdado, tDisplay_m8axScreen12, tDisplay_m8axvida };

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
  HEIGHT
};
#endif