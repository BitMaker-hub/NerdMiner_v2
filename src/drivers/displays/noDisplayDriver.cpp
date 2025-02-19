#include "displayDriver.h"
#ifdef NO_DISPLAY
#include <Arduino.h>
#include "monitor.h"
#include "wManager.h"
#include "time.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include "drivers/storage/storage.h"
#include "drivers/devices/device.h"
#include <urlencode.h>
#include <iostream>
#include <string>
#include <ctime>

extern TSettings Settings;

#define m8ax 2
#define MAX_RESULT_LENGTH 500
#define MAX_NUMBERS 6
#define MAX_DEPTH 4

char result[MAX_RESULT_LENGTH];
int sumatele = 1;
int alertatemp = 0;
int maxtemp = 0;
int mintemp = 1000;
int aciertos = 0;
int fallos = 0;
int totalci = 0;
uint32_t cuenta = 0;
uint32_t rndnumero = 0;
float maxkh = 0.00;
float minkh = 1000.00;
float porcentaje = 0.00;
float eficiencia = 0.00;
float consumo = 1.18;
String BOT_TOKEN;
String CHAT_ID;
mining_data data;

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
    snprintf(current.operation, sizeof(current.operation), "Inicio: %d", numbers[i]);
    memset(used, 0, sizeof(used));
    used[i] = 1;
    find_operations(numbers, target, current, &best, 1, used);
  }
  snprintf(result, 500, "%s\n>>> M8AX - Resultado: %d", best.operation, best.value);
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
  sprintf(output, "%d Días, %02d Horas, %02d Minutos, %02d Segundos",
          dias, horas, minutos, segundos);
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
  mensaje = "";
  mensajeCodificado.replace("\n", "%0A");
  Serial.println("M8AX - Enviando Mensaje A Telegram...");
  String url = "https://api.telegram.org/bot" + BOT_TOKEN + "/sendMessage?chat_id=" + CHAT_ID + "&text=" + mensajeCodificado;
  mensajeCodificado = "";

  if (client.connect("api.telegram.org", 443))
  {
    client.print(String("GET ") + url + " HTTP/1.1\r\n" + "Host: api.telegram.org\r\n" + "Connection: close\r\n\r\n");
    delay(1500);
    Serial.println("M8AX - Mensaje Enviado A Telegram...");
    client.flush();
    client.stop();
    sumatele += 1;
  }
  else
  {
    Serial.println("M8AX - Error De Conexión, Mensaje A Telegram Falló...");
    client.flush();
    client.stop();
  }
}

void recopilaTelegram()
{
  int horas, minutos, segundos, dia, mes, anio;
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
  char horaFormateada[9];
  char fechaFormateada[11];
  sprintf(horaFormateada, "%02d:%02d:%02d", horas, minutos, segundos);
  sprintf(fechaFormateada, "%02d/%02d/%04d", dia, mes, anio);
  uint8_t mac[6];
  WiFi.macAddress(mac);
  String u4digits = String(mac[4], HEX) + String(mac[5], HEX);
  std::string quediase = obtenerDiaSemana(std::string(fechaFormateada));
  String cadenaEnvio = F("------------------------------------------------------------------------------------------------\n");
  cadenaEnvio += "-------------------- M8AX - PLACA-WROOM-ESP32D-" + u4digits + " DATOS DE MINERÍA - M8AX --------------------\n";
  cadenaEnvio += F("------------------------------------------------------------------------------------------------\n");
  cadenaEnvio += "----------------------------------- " + String(fechaFormateada) + " " + quediase.c_str() + " - " + horaFormateada + " ----------------------------------\n";
  cadenaEnvio += F("------------------------------------------------------------------------------------------------\n");
  cadenaEnvio += "Mensaje Número - " + convertirARomanos(sumatele) + "\n";
  char output[50];
  convertirTiempo(data.timeMining.c_str(), output);
  cadenaEnvio += "Tiempo Minando - " + String(output) + "\n";
  cadenaEnvio += "HR Actual - " + data.currentHashRate + " KH/s ( MAX - " + String(maxkh) + " | MIN - " + String(minkh) + " )\n";
  eficiencia = data.currentHashRate.toFloat() / consumo;
  float eficiencia_redondeada = round(eficiencia * 1000) / 1000;
  cadenaEnvio += "Eficiencia Energética - ≈ " + String(eficiencia_redondeada, 3) + " KH/s/W\n";
  cadenaEnvio += "Temp. De CPU - " + data.temp + "° ( MAX - " + String(maxtemp) + "° | MIN - " + String(mintemp) + "° | TMP>70° - " + String(alertatemp) + " )\n";
  cadenaEnvio += "Plantillas De Bloque - " + data.templates + "\n";
  cadenaEnvio += "Shares Enviados A La Pool - " + data.completedShares + "\n";
  cadenaEnvio += "Mejor Dificultad Alcanzada - " + data.bestDiff + "\n";
  cadenaEnvio += "Cómputo Total - " + data.totalKHashes + " KH - ( " + String(atof(data.totalKHashes.c_str()) / 1000, 3) + " MH )\n";
  cadenaEnvio += "Pool De Minería - " + Settings.PoolAddress + "\n";
  cadenaEnvio += "Puerto Del Pool - " + String(Settings.PoolPort) + "\n";
  cadenaEnvio += "Tu Wallet De BTC - " + String(Settings.BtcWallet) + "\n";
  cadenaEnvio += F("------------------------------------------------------------------------------------------------\n");

  if (data.valids.toInt() == 1)
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
  cadenaEnvio += "Factorización De Número - 1 - " + String(rndnumero) + " -> " + FactorizaM8AX(rndnumero) + "\n";
  rndnumero = esp_random();
  cadenaEnvio += "Factorización De Número - 2 - " + String(rndnumero) + " -> " + FactorizaM8AX(rndnumero) + "\n";
  rndnumero = esp_random();
  cadenaEnvio += "Factorización De Número - 3 - " + String(rndnumero) + " -> " + FactorizaM8AX(rndnumero) + "\n";
  cadenaEnvio += F("------------------------------------------------------------------------------------------------\n");
  int numeritos[6];
  int destino = 1 + (esp_random() % 1000);
  generate_random_numbers(numeritos, 6, 1, 100);
  cadenaEnvio += ("Juego De Cifras Número - " + String(totalci) + "\n");
  cadenaEnvio += ("Aciertos - " + String(aciertos) + " | Fallos - " + String(fallos) + "\n");
  cadenaEnvio += ("Números: ");

  for (int i = 0; i < 6; i++)
  {
    cadenaEnvio += (numeritos[i]);
    cadenaEnvio += (" ");
  }

  cadenaEnvio += ("\nObjetivo: " + String(destino) + "\n");
  calculate_operations(numeritos, destino, result);
  String resultadoStr = String(result);
  resultadoStr.replace(">>> M8AX - ", "");
  cadenaEnvio += (String(resultadoStr) + "\n");
  cadenaEnvio += "------------------------------------------------------------------------------------------------\n                                       By M8AX Corp. " + convertirARomanos(anio);
  cadenaEnvio += F("\n------------------------------------------------------------------------------------------------");
  enviarMensajeATelegram(cadenaEnvio);
  cadenaEnvio = "";
}

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

const int morseLength = sizeof(morse) / sizeof(morse[0]);

void fiestaLED()
{
  for (int i = 0; i < 4; i++)
  {
    digitalWrite(m8ax, HIGH);
    vTaskDelay(pdMS_TO_TICKS(25));
    digitalWrite(m8ax, LOW);
    vTaskDelay(pdMS_TO_TICKS(25));
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
}

void noDisplay_NoScreen(unsigned long mElapsed)
{
  data = getMiningData(mElapsed);
  int horas, minutos, segundos, dia, mes, anio;
  int temperatura = data.temp.toInt();
  int ganador = data.valids.toInt();
  float hhashrate = data.currentHashRate.toFloat();
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

  cuenta++;

  if (cuenta > 15)
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
      if (hhashrate > 300)
      {
        digitalWrite(m8ax, HIGH);
        vTaskDelay(pdMS_TO_TICKS((cuenta % 2 == 0) ? 1 : 5));
        digitalWrite(m8ax, LOW);
      }
      else
      {
        digitalWrite(m8ax, HIGH);
        vTaskDelay(pdMS_TO_TICKS((cuenta % 2 == 0) ? 100 : 300));
        digitalWrite(m8ax, LOW);
      }
    }
    else
    {
      digitalWrite(m8ax, LOW);
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
    eficiencia = data.currentHashRate.toFloat() / consumo;
    float eficiencia_redondeada = round(eficiencia * 1000) / 1000;
    Serial.print("\n--------------------------------------------------------------------------------");
    Serial.printf("\n>>> M8AX - Bloques Válidos: %s\n", data.valids.c_str());
    Serial.printf(">>> M8AX - Plantillas De Bloques: %s\n", data.templates.c_str());
    Serial.printf(">>> M8AX - Mejor Dificultad Alcanzada: %s\n", data.bestDiff.c_str());
    Serial.printf(">>> M8AX - Shares Enviados A La Pool: %s\n", data.completedShares.c_str());
    Serial.printf(">>> M8AX - HashRate: %s KH/s\n", String(hhashrate));
    Serial.printf(">>> M8AX - Max HashRate: %s KH/s\n", String(maxkh));
    Serial.printf(">>> M8AX - Min HashRate: %s KH/s\n", String(minkh));
    Serial.printf(">>> M8AX - Eficiencia Energética - ≈ %.3f KH/s/W\n", eficiencia_redondeada);
    Serial.printf(">>> M8AX - Temperatura: %s°\n", data.temp.c_str());
    Serial.printf(">>> M8AX - Max Temperatura: %s°\n", String(maxtemp));
    Serial.printf(">>> M8AX - Min Temperatura: %s°\n", String(mintemp));
    Serial.printf(">>> M8AX - Temoeratura > 70°: %s Veces\n", String(alertatemp));
    Serial.printf(">>> M8AX - Cómputo Total ( MH ): %s\n", data.totalMHashes.c_str());
    char output[50];
    convertirTiempo(data.timeMining.c_str(), output);
    Serial.printf(">>> M8AX - Tiempo Minando: %s\n", output);
    int numeritos[6];
    int destino = 1 + (esp_random() % 1000);
    generate_random_numbers(numeritos, 6, 1, 100);
    Serial.print("--------------------------------------------------------------------------------\n");
    Serial.print(">>> M8AX - MEGA NERD - M8AX\n");
    Serial.print("--------------------------------------------------------------------------------\n");
    rndnumero = esp_random();
    Serial.print(">>> M8AX - Factorización De Número - 1 - " + String(rndnumero) + " -> " + FactorizaM8AX(rndnumero) + "\n");
    rndnumero = esp_random();
    Serial.print(">>> M8AX - Factorización De Número - 2 - " + String(rndnumero) + " -> " + FactorizaM8AX(rndnumero) + "\n");
    rndnumero = esp_random();
    Serial.print(">>> M8AX - Factorización De Número - 3 - " + String(rndnumero) + " -> " + FactorizaM8AX(rndnumero) + "\n");
    Serial.print("--------------------------------------------------------------------------------\n");
    Serial.print(">>> M8AX - Juego De Cifras Número - " + String(totalci) + "\n");
    Serial.print(">>> M8AX - Aciertos - " + String(aciertos) + " | Fallos - " + String(fallos) + "\n");
    Serial.print(">>> M8AX - Números: ");

    for (int i = 0; i < 6; i++)
    {
      Serial.print(numeritos[i]);
      Serial.print(" ");
    }

    Serial.println();
    Serial.print(">>> M8AX - Objetivo: " + String(destino) + "\n");
    calculate_operations(numeritos, destino, result);
    Serial.print(">>> M8AX - " + String(result) + "\n");
    Serial.print("--------------------------------------------------------------------------------\n\n");
  }

  String fechaHora = String(timeinfo.tm_hour < 10 ? "0" : "") + String(timeinfo.tm_hour) + ":" +
                     String(timeinfo.tm_min < 10 ? "0" : "") + String(timeinfo.tm_min) + ":" +
                     String(timeinfo.tm_sec < 10 ? "0" : "") + String(timeinfo.tm_sec);

  Serial.printf("%s >>> M8AX - Completados %s Share(s), %s Khashes, Med. HashRate %s KH/s - %s°\n",
                fechaHora.c_str(), data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str(), data.temp.c_str());

  if (minutos == 30 && segundos == 0)
  {
    digitalWrite(m8ax, HIGH);
    vTaskDelay(pdMS_TO_TICKS(300));
    digitalWrite(m8ax, LOW);
    vTaskDelay(pdMS_TO_TICKS(300));
  }

  if (minutos == 0 && segundos == 0)
  {
    digitalWrite(m8ax, HIGH);
    vTaskDelay(pdMS_TO_TICKS(300));
    digitalWrite(m8ax, LOW);
    vTaskDelay(pdMS_TO_TICKS(300));
    digitalWrite(m8ax, HIGH);
    vTaskDelay(pdMS_TO_TICKS(300));
    digitalWrite(m8ax, LOW);
    vTaskDelay(pdMS_TO_TICKS(300));
  }

  if (temperatura > 70)
  {
    Serial.println("M8AX - La Temperatura De La CPU Ha Superado Los 70°C...");
    alertatemp++;

    if (temperatura > 80)
    {
      Serial.println("M8AX - ¡ Temperatura Muy Alta ! 80°C Superados. Entrando En Deep Sleep Por 10 Minutos Para Enfriar La CPU...");
      esp_sleep_enable_timer_wakeup(600e6);
      esp_deep_sleep_start();
    }
  }

  if (cuenta % 7200 == 0)
  {
    BOT_TOKEN = Settings.botTelegram;
    CHAT_ID = Settings.ChanelIDTelegram;
    sincronizarTiempo();
    if (BOT_TOKEN != "NO CONFIGURADO" && CHAT_ID != "NO CONFIGURADO")
    {
      digitalWrite(m8ax, HIGH);
      recopilaTelegram();
    }
  }

  if (cuenta == 5)
  {
    sincronizarTiempo();
  }
}

void noDisplay_LoadingScreen(void)
{
  pinMode(m8ax, OUTPUT);
  Serial.println("... M8AX - ARRANCANDO ...");
  for (int i = 0; i < morseLength; i++)
  {
    const char *letra = morse[i];
    for (int j = 0; letra[j] != '\0'; j++)
    {
      if (letra[j] == '.')
      {
        digitalWrite(m8ax, HIGH);
        vTaskDelay(pdMS_TO_TICKS(200));
        digitalWrite(m8ax, LOW);
        vTaskDelay(pdMS_TO_TICKS(200));
      }
      else if (letra[j] == '-')
      {
        digitalWrite(m8ax, HIGH);
        vTaskDelay(pdMS_TO_TICKS(600));
        digitalWrite(m8ax, LOW);
        vTaskDelay(pdMS_TO_TICKS(200));
      }
    }
    vTaskDelay(pdMS_TO_TICKS(600));
    if (i == 3)
    {
      vTaskDelay(pdMS_TO_TICKS(1500));
    }
  }
  vTaskDelay(pdMS_TO_TICKS(500));
}

void noDisplay_SetupScreen(void)
{
  Serial.println("... M8AX - PANTALLA DE CONFIGURACIÓN ...");
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