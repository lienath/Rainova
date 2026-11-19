/*
  Rainova - Regadera inteligente con agua de lluvia
  ESP8266 NodeMCU (ESP-12E)

  Basado en el codigo funcional sin WiFi del proyecto.
  Anade: SoftAP (192.168.4.1), dashboard PWA, API JSON, historico,
  control manual, calibracion persistente y hora sincronizada desde el celular.

  Pines: humedad A0 | flotador D5 | rele D6 | LCD SDA D2 / SCL D1
  La LOGICA DE RIEGO sigue funcionando aunque el WiFi o el celular fallEN.
*/
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
#include "web.h"

// ---------- PINES ----------
const int PIN_HUMEDAD = A0;
const int PIN_NIVEL   = D5;
const int PIN_RELE    = D6;
const bool RELE_ACTIVO_EN_LOW = true;

// ---------- MODO ----------
const bool MODO_CALIBRACION = false;

// ---------- CALIBRACION Y PIN (se guardan en EEPROM) ----------
struct CalData { int seco; int mojado; float ml; };
CalData cal = {782, 325, 15.0f};
const int EEPROM_SIZE   = 64;
const int EEPROM_PIN_DIR = 16;
uint16_t pinCal = 1234;          // PIN inicial de 4 digitos
int intentosPin = 0;
unsigned long tBloqueoPin = 0;

// ---------- UMBRALES Y TIEMPOS ----------
const int UMBRAL_SECO   = 35;
const int UMBRAL_HUMEDO = 60;
const unsigned long PULSO_RIEGO_MS    = 4000;
const unsigned long ESPERA_ABSORB_MS  = 8000;
const unsigned long MAX_CICLO_MS      = 90000;
const unsigned long PAUSA_ALERTA_MS   = 30000;
const unsigned long INTERVALO_LECT_MS = 250;
const unsigned long INTERVALO_LCD_MS  = 500;
const int LECTURAS_FLOTADOR = 4;
const float ML_REFERENCIA_MANUAL = 350.0f;  // mL de un riego manual habitual

// ---------- WIFI / WEB ----------
const char* AP_SSID = "Rainova-ESP";
ESP8266WebServer server(80);

// ---------- ESTADO ----------
enum Estado { REPOSO, BOMBEANDO, ESPERANDO, ALERTA };
Estado estado = REPOSO;

unsigned long tEstado = 0, inicioCiclo = 0, tUltLect = 0, tUltLCD = 0;
unsigned long bombaTotalMs = 0, cicloMs = 0;
unsigned long tUltMuestra = 0, tUltTendencia = 0, tSecoDetectado = 0;
int humedad = 0, lecturaCruda = 0, cuentaAgua = 0;
bool agua = true;
bool pausado = false;
bool manualPulso = false;

float mlHoy = 0, mlTotal = 0, mlUltimoCiclo = 0;
int riegosHoy = 0, riegosManualesHoy = 0;
uint32_t ultimoRiegoEpoch = 0, ultimoRiegoMs = 0;
bool manualPendiente = false;
uint32_t epochBase = 0, diaActual = 0, tendenciaEpoch = 0;
float tendencia = 0;
int humRef = 0;
String alertaActual = "";

// ---------- HISTORICO ----------
struct Sample { uint32_t t; uint32_t ms; int8_t h; uint8_t agua; uint8_t estado; };
const int MAX_MUESTRAS = 120;
Sample muestras[MAX_MUESTRAS];
int nMuestras = 0, idxMuestras = 0;

struct Evento { uint32_t t; uint32_t ms; char d[30]; };
const int MAX_EVENTOS = 24;
Evento eventos[MAX_EVENTOS];
int nEventos = 0, idxEventos = 0;

static char jsonBuf[1400];
static char histBuf[5000];
static char csvBuf[4000];

LiquidCrystal_I2C *lcd = NULL;
bool lcdOk = false;

// ============================================================
// Utilidades
// ============================================================
uint32_t ahoraEpoch() {
  return epochBase ? (epochBase + millis() / 1000) : 0;
}

const char* estadoTexto() {
  if (pausado) return "PAUSA";
  switch (estado) {
    case BOMBEANDO: return "REGANDO";
    case ESPERANDO: return "ESPERA";
    case ALERTA:    return "PAUSA";
    default:        return "OK";
  }
}

float ahorroPct() {
  if (mlUltimoCiclo <= 0 || ML_REFERENCIA_MANUAL <= 0) return 0;
  float a = (ML_REFERENCIA_MANUAL - mlUltimoCiclo) / ML_REFERENCIA_MANUAL * 100.0f;
  if (a < 0) a = 0;
  if (a > 100) a = 100;
  return a;
}

float minutosProximoRiego() {
  if (!agua) return -2;
  if (tendencia >= -0.02f || humedad <= UMBRAL_SECO) return -1;
  float m = (humedad - UMBRAL_SECO) / (-tendencia);
  if (m > 1440) m = 1440;
  return m;
}

void registrarEvento(const char* texto) {
  Evento &e = eventos[idxEventos];
  e.ms = millis();
  e.t = ahoraEpoch();   // 0 si todavia no llego la hora del celular; se rellena al sincronizar
  strncpy(e.d, texto, sizeof(e.d) - 1);
  e.d[sizeof(e.d) - 1] = 0;
  idxEventos = (idxEventos + 1) % MAX_EVENTOS;
  if (nEventos < MAX_EVENTOS) nEventos++;
}

void guardarMuestra() {
  Sample &s = muestras[idxMuestras];
  s.ms = millis();
  s.t = ahoraEpoch();
  s.h = (int8_t)humedad;
  s.agua = agua ? 1 : 0;
  s.estado = (uint8_t)estado;
  idxMuestras = (idxMuestras + 1) % MAX_MUESTRAS;
  if (nMuestras < MAX_MUESTRAS) nMuestras++;
}

void revisarDia() {
  if (!epochBase) return;
  uint32_t dia = ahoraEpoch() / 86400UL;
  if (diaActual == 0) diaActual = dia;
  else if (dia != diaActual) {
    diaActual = dia;
    mlHoy = 0;
    riegosHoy = 0;
    riegosManualesHoy = 0;
    ultimoRiegoEpoch = 0;
    ultimoRiegoMs = 0;
    registrarEvento("Nuevo dia");
  }
}

void actualizarAlerta() {
  if (!agua) alertaActual = "Tanque vacio: la bomba esta detenida";
  else if (estado == ALERTA) alertaActual = "Ciclo maximo alcanzado, pausa de seguridad";
  else if (pausado) alertaActual = "Riego automatico en pausa";
  else if (tSecoDetectado && millis() - tSecoDetectado > 600000UL)
    alertaActual = "Humedad baja prolongada sin riego";
  else alertaActual = "";
}

// ============================================================
// Sensores y LCD (logica original)
// ============================================================
byte buscarPantalla() {
  for (byte dir = 0x20; dir <= 0x3F; dir++) {
    Wire.beginTransmission(dir);
    if (Wire.endTransmission() == 0) return dir;
  }
  return 0;
}

// Relé activo en LOW con lógica de 3,3 V:
//   encender = pin en LOW
//   apagar   = pin suelto (INPUT), así el relé de 5 V sí se apaga
void bomba(bool encender) {
  if (RELE_ACTIVO_EN_LOW) {
    if (encender) { pinMode(PIN_RELE, OUTPUT); digitalWrite(PIN_RELE, LOW); }
    else          { pinMode(PIN_RELE, INPUT); }
  } else {
    pinMode(PIN_RELE, OUTPUT);
    digitalWrite(PIN_RELE, encender ? HIGH : LOW);
  }
}

void leerSensores() {
  long suma = 0;
  for (int i = 0; i < 10; i++) { suma += analogRead(PIN_HUMEDAD); delay(2); }
  lecturaCruda = suma / 10;
  int pct = map(lecturaCruda, cal.seco, cal.mojado, 0, 100);
  humedad = constrain(pct, 0, 100);

  bool crudo = (digitalRead(PIN_NIVEL) == LOW);
  if (crudo == agua) {
    cuentaAgua = 0;
  } else {
    cuentaAgua++;
    if (cuentaAgua >= LECTURAS_FLOTADOR) {
      agua = crudo;
      cuentaAgua = 0;
      registrarEvento(agua ? "Tanque con agua" : "Tanque sin agua");
    }
  }

  if (humedad < UMBRAL_SECO) {
    if (!tSecoDetectado) tSecoDetectado = millis();
  } else {
    tSecoDetectado = 0;
  }
}

void calcularTendencia() {
  if (!tendenciaEpoch) { humRef = humedad; tendenciaEpoch = millis(); return; }
  unsigned long dt = millis() - tendenciaEpoch;
  if (dt < 25000) return;
  tendencia = (humedad - humRef) / (dt / 60000.0f);
  humRef = humedad;
  tendenciaEpoch = millis();
}

void actualizarLCD() {
  if (!lcdOk) return;

  static byte contador = 0;
  if (++contador >= 10) {
    contador = 0;
    lcd->init();
    lcd->backlight();
  }

  char l0[17], l1[17];
  if (MODO_CALIBRACION) {
    snprintf(l0, sizeof(l0), "CALIBRACION     ");
    snprintf(l1, sizeof(l1), "Lectura: %-6d  ", lecturaCruda);
  } else {
    snprintf(l0, sizeof(l0), "Humedad: %3d%%   ", humedad);
    snprintf(l1, sizeof(l1), "%-8s%4dmL  ", estadoTexto(), (int)mlTotal);
  }
  lcd->setCursor(0, 0); lcd->print(l0);
  lcd->setCursor(0, 1); lcd->print(l1);
}

void graficar() {
  if (MODO_CALIBRACION) {
    Serial.print("Lectura:"); Serial.println(lecturaCruda);
  } else {
    Serial.print("Humedad:");        Serial.print(humedad);
    Serial.print(",Umbral_seco:");   Serial.print(UMBRAL_SECO);
    Serial.print(",Umbral_humedo:"); Serial.print(UMBRAL_HUMEDO);
    Serial.print(",Agua:");          Serial.print(agua ? 50 : 0);
    Serial.print(",Bomba:");         Serial.println(estado == BOMBEANDO ? 100 : 0);
  }
}

// ============================================================
// Logica de riego (misma maquina de estados, con control manual)
// ============================================================
void iniciarCiclo(unsigned long ahora, bool manual) {
  inicioCiclo = ahora;
  tEstado = ahora;
  cicloMs = 0;
  manualPulso = manual;
  estado = BOMBEANDO;
  bomba(true);
  registrarEvento(manual ? "Riego manual iniciado" : "Riego automatico iniciado");
}

void finalizarCiclo(unsigned long ahora, bool conAlerta) {
  bool fueManual = manualPulso;
  float cantidad = cicloMs / 1000.0f * cal.ml;
  mlUltimoCiclo = cantidad;
  mlHoy += cantidad;
  mlTotal += cantidad;
  riegosHoy++;
  if (fueManual) riegosManualesHoy++;
  ultimoRiegoMs = millis();
  ultimoRiegoEpoch = ahoraEpoch();
  if (conAlerta) {
    registrarEvento("Ciclo cortado por seguridad");
  } else {
    char evt[30];
    snprintf(evt, sizeof(evt), "%s %.0f mL", fueManual ? "Riego manual" : "Riego auto", cantidad);
    registrarEvento(evt);
  }
  manualPulso = false;
  if (conAlerta) {
    tEstado = ahora;
    estado = ALERTA;
  } else {
    estado = REPOSO;
  }
}

void controlRiego(unsigned long ahora) {
  if (MODO_CALIBRACION) { bomba(false); return; }

  if (pausado) {
    if (estado == BOMBEANDO) {
      unsigned long d = ahora - tEstado;
      bomba(false);
      bombaTotalMs += d;
      cicloMs += d;
    }
    estado = REPOSO;
    manualPulso = false;
    return;
  }

  bool sinAgua  = !agua;
  bool humedoOk = (humedad >= UMBRAL_HUMEDO);
  bool excedido = (ahora - inicioCiclo > MAX_CICLO_MS);

  switch (estado) {
    case REPOSO:
      // Un riego manual pedido durante otro ciclo se ejecuta apenas termina.
      if (manualPendiente && agua) {
        manualPendiente = false;
        iniciarCiclo(ahora, true);
        break;
      }
      if (humedad < UMBRAL_SECO && agua) iniciarCiclo(ahora, false);
      break;

    case BOMBEANDO:
      if (sinAgua || (!manualPulso && humedoOk) || excedido) {
        unsigned long d = ahora - tEstado;
        bomba(false);
        bombaTotalMs += d;
        cicloMs += d;
        finalizarCiclo(ahora, sinAgua || excedido);
      } else if (ahora - tEstado >= PULSO_RIEGO_MS) {
        unsigned long d = ahora - tEstado;
        bomba(false);
        bombaTotalMs += d;
        cicloMs += d;
        tEstado = ahora;
        if (manualPulso) finalizarCiclo(ahora, false);
        else estado = ESPERANDO;
      }
      break;

    case ESPERANDO:
      if (sinAgua || humedoOk) {
        finalizarCiclo(ahora, sinAgua);
      } else if (excedido) {
        finalizarCiclo(ahora, true);
      } else if (ahora - tEstado >= ESPERA_ABSORB_MS) {
        bomba(true);
        tEstado = ahora;
        estado = BOMBEANDO;
      }
      break;

    case ALERTA:
      bomba(false);
      if (ahora - tEstado >= PAUSA_ALERTA_MS) estado = REPOSO;
      break;
  }
}

// ============================================================
// API
// ============================================================
void handleEstado() {
  snprintf(jsonBuf, sizeof(jsonBuf),
    "{\"humedad\":%d,\"crudo\":%d,\"agua\":%s,\"estado\":\"%s\",\"bomba\":%s,"
    "\"pausado\":%s,\"mlHoy\":%.0f,\"mlTotal\":%.0f,\"mlUltimo\":%.0f,"
    "\"mlManual\":%.0f,\"ahorro\":%.0f,\"riegosHoy\":%d,\"manualHoy\":%d,\"ultimoRiego\":%lu,"
    "\"hora\":%lu,\"uptime\":%lu,\"wifi\":true,\"alerta\":\"%s\","
    "\"tendencia\":%.3f,\"proximo\":%.0f,\"seco\":%d,\"mojado\":%d,\"mlSeg\":%.1f}",
    humedad, lecturaCruda, agua ? "true" : "false", estadoTexto(),
    (estado == BOMBEANDO) ? "true" : "false", pausado ? "true" : "false",
    mlHoy, mlTotal, mlUltimoCiclo, ML_REFERENCIA_MANUAL, ahorroPct(),
    riegosHoy, riegosManualesHoy, (unsigned long)ultimoRiegoEpoch, (unsigned long)ahoraEpoch(),
    millis() / 1000, alertaActual.c_str(), tendencia, minutosProximoRiego(),
    cal.seco, cal.mojado, cal.ml);
  server.send(200, "application/json", jsonBuf);
}

void handleHistorial() {
  int len = snprintf(histBuf, sizeof(histBuf), "{\"samples\":[");
  for (int i = 0; i < nMuestras; i++) {
    int k = (idxMuestras - nMuestras + i + MAX_MUESTRAS) % MAX_MUESTRAS;
    int n = snprintf(histBuf + len, sizeof(histBuf) - len,
                     "%s{\"t\":%lu,\"h\":%d}", i ? "," : "",
                     (unsigned long)muestras[k].t, (int)muestras[k].h);
    if (n <= 0 || len + n >= (int)sizeof(histBuf)) break;
    len += n;
  }
  int n = snprintf(histBuf + len, sizeof(histBuf) - len, "],\"eventos\":[");
  if (n > 0) len += n;
  for (int i = 0; i < nEventos; i++) {
    int k = (idxEventos - nEventos + i + MAX_EVENTOS) % MAX_EVENTOS;
    n = snprintf(histBuf + len, sizeof(histBuf) - len,
                 "%s{\"t\":%lu,\"d\":\"%s\"}", i ? "," : "",
                 (unsigned long)eventos[k].t, eventos[k].d);
    if (n <= 0 || len + n >= (int)sizeof(histBuf)) break;
    len += n;
  }
  snprintf(histBuf + len, sizeof(histBuf) - len, "]}");
  server.send(200, "application/json", histBuf);
}

void handleCsv() {
  int len = snprintf(csvBuf, sizeof(csvBuf), "t,humedad_pct,agua,estado,ml_acumulado\n");
  for (int i = 0; i < nMuestras; i++) {
    int k = (idxMuestras - nMuestras + i + MAX_MUESTRAS) % MAX_MUESTRAS;
    int n = snprintf(csvBuf + len, sizeof(csvBuf) - len, "%lu,%d,%d,%d,%.0f\n",
                     (unsigned long)muestras[k].t, (int)muestras[k].h,
                     (int)muestras[k].agua, (int)muestras[k].estado, mlTotal);
    if (n <= 0 || len + n >= (int)sizeof(csvBuf)) break;
    len += n;
  }
  server.send(200, "text/csv", csvBuf);
}

unsigned long jsonNum(const String& body, const char* key, bool decimal) {
  int p = body.indexOf(key);
  if (p < 0) return 0;
  int c = body.indexOf(':', p);
  if (c < 0) return 0;
  String valor = body.substring(c + 1);   // mantener vivo el String mientras se lee su c_str()
  const char* s = valor.c_str();
  while (*s == ' ' || *s == '"') s++;     // acepta numericos o entre comillas
  if (decimal) return (unsigned long)(strtod(s, NULL) * 10.0);
  return strtoul(s, NULL, 10);
}

// Los eventos ocurridos antes de recibir la hora se guardaron con t=0 y su
// millis() correspondiente. Al sincronizar se les calcula la hora real.
void rellenarHorasPendientes() {
  for (int i = 0; i < nEventos; i++) {
    int k = (idxEventos - nEventos + i + MAX_EVENTOS) % MAX_EVENTOS;
    if (eventos[k].t == 0) eventos[k].t = epochBase + eventos[k].ms / 1000;
  }
  for (int i = 0; i < nMuestras; i++) {
    int k = (idxMuestras - nMuestras + i + MAX_MUESTRAS) % MAX_MUESTRAS;
    if (muestras[k].t == 0) muestras[k].t = epochBase + muestras[k].ms / 1000;
  }
  if (ultimoRiegoEpoch == 0 && ultimoRiegoMs)
    ultimoRiegoEpoch = epochBase + ultimoRiegoMs / 1000;
}

void handleHora() {
  String body = server.arg("plain");
  unsigned long e = jsonNum(body, "epoch", false);
  if (e > 1000000000UL) {
    epochBase = (uint32_t)e - millis() / 1000;
    rellenarHorasPendientes();
    diaActual = 0;
    revisarDia();
    registrarEvento("Hora sincronizada");
  }
  server.send(200, "application/json", "{\"ok\":true}");
}

void handleAccion() {
  String body = server.arg("plain");
  unsigned long ahora = millis();
  String resp = "{\"ok\":true";

  if (body.indexOf("regar") >= 0) {
    if (pausado)              resp += ",\"manual\":\"pausa\"";
    else if (!agua)           resp += ",\"manual\":\"no_agua\"";
    else if (estado == REPOSO) {
      iniciarCiclo(ahora, true);
      resp += ",\"manual\":\"iniciado\"";
    } else {
      manualPendiente = true;
      resp += ",\"manual\":\"pendiente\"";
    }
  } else if (body.indexOf("pausa") >= 0) {
    pausado = true;
    registrarEvento("Pausa manual");
  } else if (body.indexOf("reanudar") >= 0) {
    pausado = false;
    registrarEvento("Riego reanudado");
  } else if (body.indexOf("reset") >= 0) {
    mlHoy = 0;
    riegosHoy = 0;
    riegosManualesHoy = 0;
    ultimoRiegoEpoch = 0;
    ultimoRiegoMs = 0;
    manualPendiente = false;
    registrarEvento("Contadores reiniciados");
  }
  resp += "}";
  server.send(200, "application/json", resp);
}

void handleCalibracion() {
  String body = server.arg("plain");

  if (tBloqueoPin && millis() < tBloqueoPin) {
    server.send(429, "application/json", "{\"ok\":false,\"error\":\"bloqueado\"}");
    return;
  }
  unsigned long pin = jsonNum(body, "pin", false);
  if (pin != pinCal) {
    if (++intentosPin >= 5) {
      intentosPin = 0;
      tBloqueoPin = millis() + 30000UL;
      server.send(429, "application/json", "{\"ok\":false,\"error\":\"bloqueado\"}");
    } else {
      server.send(403, "application/json", "{\"ok\":false,\"error\":\"pin\"}");
    }
    return;
  }
  intentosPin = 0;

  int seco = (int)jsonNum(body, "seco", false);
  int mojado = (int)jsonNum(body, "mojado", false);
  float ml = jsonNum(body, "ml", true) / 10.0f;
  bool cambio = false;
  if (seco > 50 && seco < 1024 && mojado > 0 && mojado < seco) {
    cal.seco = seco;
    cambio = true;
  }
  if (mojado > 0 && mojado < 1024 && mojado < cal.seco) {
    cal.mojado = mojado;
    cambio = true;
  }
  if (ml > 0.1f && ml < 200.0f) {
    cal.ml = ml;
    cambio = true;
  }

  if (body.indexOf("nuevoPin") >= 0) {
    unsigned long nuevo = jsonNum(body, "nuevoPin", false);
    if (nuevo >= 0 && nuevo <= 9999) {
      pinCal = (uint16_t)nuevo;
      EEPROM.put(EEPROM_PIN_DIR, pinCal);
      cambio = true;
    }
  }
  EEPROM.put(0, cal);
  EEPROM.commit();
  registrarEvento(cambio ? "Calibracion actualizada" : "Calibracion sin cambios");
  server.send(200, "application/json", "{\"ok\":true}");
}

void cargarConfig() {
  EEPROM.begin(EEPROM_SIZE);
  CalData tmp;
  EEPROM.get(0, tmp);
  if (tmp.seco > 50 && tmp.seco < 1024 && tmp.mojado > 0 &&
      tmp.mojado < tmp.seco && tmp.ml > 0.1f && tmp.ml < 200.0f) {
    cal = tmp;
  } else {
    EEPROM.put(0, cal);
    EEPROM.commit();
  }
  uint16_t p;
  EEPROM.get(EEPROM_PIN_DIR, p);
  if (p <= 9999) pinCal = p;
  else {
    pinCal = 1234;
    EEPROM.put(EEPROM_PIN_DIR, pinCal);
    EEPROM.commit();
  }
}

// ============================================================
// Setup / Loop
// ============================================================
void setup() {
  Serial.begin(9600);

  cargarConfig();

  pinMode(PIN_NIVEL, INPUT_PULLUP);
  bomba(false);
  agua = (digitalRead(PIN_NIVEL) == LOW);

  delay(500);            // espera a que la pantalla reciba energía
  Wire.begin(D2, D1);    // SDA = D2, SCL = D1

  byte dir = buscarPantalla();
  if (dir != 0) {
    lcd = new LiquidCrystal_I2C(dir, 16, 2);
    lcd->init();
    lcd->backlight();
    lcdOk = true;
    lcd->setCursor(0, 0); lcd->print("Rainova-ESP");
    lcd->setCursor(0, 1); lcd->print("192.168.4.1");
    Serial.print("\nPantalla encontrada en 0x"); Serial.println(dir, HEX);
    delay(2500);
    lcd->setCursor(0, 0); lcd->print("Riego automatico");
    lcd->setCursor(0, 1); lcd->print("con agua lluvia");
    delay(1500);
  } else {
    Serial.println("\nNO se encontro la pantalla. Revisa VCC, GND, SDA(D2), SCL(D1).");
  }

  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
  WiFi.softAP(AP_SSID);

  server.on("/", HTTP_GET, []() { server.send_P(200, "text/html", INDEX_HTML); });
  server.on("/api/estado", HTTP_GET, handleEstado);
  server.on("/api/historial", HTTP_GET, handleHistorial);
  server.on("/exportar.csv", HTTP_GET, handleCsv);
  server.on("/api/hora", HTTP_POST, handleHora);
  server.on("/api/accion", HTTP_POST, handleAccion);
  server.on("/api/calibracion", HTTP_POST, handleCalibracion);
  server.begin();

  Serial.println("\nRainova listo");
  Serial.print("WiFi: "); Serial.println(AP_SSID);
  Serial.print("IP:   "); Serial.println(WiFi.softAPIP());
  Serial.println("Abre http://192.168.4.1 en tu celular");
  registrarEvento("Sistema iniciado");
}

void loop() {
  unsigned long ahora = millis();

  if (ahora - tUltLect >= INTERVALO_LECT_MS) {
    tUltLect = ahora;
    leerSensores();
    graficar();
  }

  revisarDia();
  actualizarAlerta();
  controlRiego(ahora);

  static Estado previo = REPOSO;
  if (previo == BOMBEANDO && estado != BOMBEANDO && lcdOk) {
    delay(50);
    lcd->init();
    lcd->backlight();
  }
  previo = estado;

  if (estado != BOMBEANDO && ahora - tUltLCD >= INTERVALO_LCD_MS) {
    tUltLCD = ahora;
    actualizarLCD();
  }

  if (ahora - tUltMuestra >= 5000) {
    tUltMuestra = ahora;
    guardarMuestra();
  }
  calcularTendencia();

  // Watchdog del AP: si el WiFi quedo en un estado invalido (o dejo de anunciarse
  // sin clientes conectados), se vuelve a levantar sin reiniciar el ESP.
  static unsigned long tUltApWatch = 0, tUltApKeep = 0;
  if (ahora - tUltApWatch >= 15000) {
    tUltApWatch = ahora;
    bool apOk = (WiFi.getMode() == WIFI_AP) && (WiFi.softAPIP()[0] != 0);
    if (!apOk) {
      WiFi.mode(WIFI_AP);
      WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
      WiFi.softAP(AP_SSID);
      Serial.println("[AP] Watchdog: AP reiniciado");
    } else if (WiFi.softAPgetStationNum() == 0 && ahora - tUltApKeep >= 300000UL) {
      tUltApKeep = ahora;
      WiFi.softAP(AP_SSID);   // reanuncio periodico, solo si no hay nadie conectado
      Serial.println("[AP] Watchdog: AP reanunciado");
    }
  }

  static unsigned long tUltimoDebug = 0;
  if (ahora - tUltimoDebug >= 15000) {
    tUltimoDebug = ahora;
    Serial.print("[AP] SSID="); Serial.print(WiFi.softAPSSID());
    Serial.print(" IP="); Serial.print(WiFi.softAPIP());
    Serial.print(" clientes="); Serial.println(WiFi.softAPgetStationNum());
  }

  server.handleClient();
  yield();
}
