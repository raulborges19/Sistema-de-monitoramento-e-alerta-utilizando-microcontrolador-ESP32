#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

// ====== Wi-Fi / Telegram (preencha no seu PC) ======
const char* ssid = ""; //seu Wifi
const char* password = ""; //senha do seu Wifi

#define BOT_TOKEN "" // token do seu bot no telegram
#define CHAT_ID   "" //id do seu chat do telegram

WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);

// ====== PINOS (os seus) ======
#define PIR 33
#define LED_ARMADO 26      // vermelho
#define LED_DESARMADO 27   // verde
#define BUZZER 25

#define B1 14
#define B2 12
#define B3 13

// ====== CONFIG ======
const unsigned long TEMPO_SAIDA_MS        = 10000; // 10s
const unsigned long TIMEOUT_TECLAS_MS     = 3000;  // 3s
const unsigned long ALERTA_COOLDOWN_MS    = 60000; // 60s (repetição opcional)
const unsigned long WIFI_RETRY_MS         = 20000; // tenta reconectar a cada 20s
const unsigned long WIFI_TIMEOUT_MS       = 12000; // tempo máximo tentando por tentativa

// ====== ESTADOS DO ALARME ======
enum Estado { DESARMADO, SAIDA, ARMADO, ALARME };
Estado estado = DESARMADO;

// ====== SEQUÊNCIA ======
int seq[3];
int idx = 0;
unsigned long ultimoBotaoMs = 0;

// ====== SAÍDA (pisca) ======
unsigned long inicioSaidaMs = 0;
unsigned long ultimoPiscaMs = 0;
bool pisca = false;

// ====== SIRENE ======
unsigned long sireneInicioCicloMs = 0;
bool buzzerLigado = false;

// ====== TELEGRAM ======
bool alertaEnviadoNoDisparo = false;
unsigned long ultimoAlertaMs = 0;

// ====== WIFI NÃO BLOQUEANTE ======
bool wifiTentando = false;
unsigned long wifiInicioTentativaMs = 0;
unsigned long wifiUltimaTentativaMs = 0;

void iniciarWiFiSemTravar() {
  // só inicia nova tentativa se passou o intervalo
  unsigned long agora = millis();
  if (wifiTentando) return;
  if (WiFi.status() == WL_CONNECTED) return;
  if (agora - wifiUltimaTentativaMs < WIFI_RETRY_MS) return;

  wifiUltimaTentativaMs = agora;
  wifiTentando = true;
  wifiInicioTentativaMs = agora;

  Serial.println("\n[WIFI] Iniciando tentativa...");

  // limpa estado anterior (evita “cannot set config”)
  WiFi.mode(WIFI_OFF);
  delay(50);
  WiFi.disconnect(true, true);
  delay(50);

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(ssid, password);
}

void atualizarWiFiSemTravar() {
  if (!wifiTentando) return;

  // conectou?
  if (WiFi.status() == WL_CONNECTED) {
    wifiTentando = false;
    Serial.println("[WIFI] Conectado!");
    Serial.print("[WIFI] IP: ");
    Serial.println(WiFi.localIP());

    // Telegram usa HTTPS
    client.setInsecure();

    // Mensagem de teste (opcional)
    bot.sendMessage(CHAT_ID, "📶 ESP32 online. Alarme conectado!", "");
    return;
  }

  // estourou o timeout da tentativa?
  if (millis() - wifiInicioTentativaMs > WIFI_TIMEOUT_MS) {
    wifiTentando = false;
    Serial.println("[WIFI] Timeout. Alarme continua OFFLINE.");
    WiFi.disconnect(true, true);
    WiFi.mode(WIFI_OFF);
  }
}

// ====== SIRENE “piiii piiiii” ======
void atualizarSirene() {
  unsigned long agora = millis();
  if (sireneInicioCicloMs == 0) sireneInicioCicloMs = agora;

  unsigned long t = agora - sireneInicioCicloMs;
  if (t >= 1600) { sireneInicioCicloMs = agora; t = 0; }

  bool deveLigar = (t < 300) || (t >= 500 && t < 800);

  if (deveLigar != buzzerLigado) {
    buzzerLigado = deveLigar;
    digitalWrite(BUZZER, buzzerLigado ? HIGH : LOW);
  }
}

void buzzerOff() {
  sireneInicioCicloMs = 0;
  buzzerLigado = false;
  digitalWrite(BUZZER, LOW);
}

// ====== ESTADOS ======
void setEstado(Estado novo) {
  estado = novo;

  if (estado == DESARMADO) {
    digitalWrite(LED_DESARMADO, HIGH);
    digitalWrite(LED_ARMADO, LOW);
    buzzerOff();
    alertaEnviadoNoDisparo = false;
  }

  if (estado == SAIDA) {
    digitalWrite(LED_DESARMADO, LOW);
    inicioSaidaMs = millis();
    ultimoPiscaMs = millis();
    pisca = true;
    digitalWrite(LED_ARMADO, HIGH);
    buzzerOff();
  }

  if (estado == ARMADO) {
    digitalWrite(LED_DESARMADO, LOW);
    digitalWrite(LED_ARMADO, HIGH);
    buzzerOff();
    alertaEnviadoNoDisparo = false;
  }

  if (estado == ALARME) {
    digitalWrite(LED_DESARMADO, LOW);
    digitalWrite(LED_ARMADO, HIGH);
  }
}

void resetSequencia() { idx = 0; ultimoBotaoMs = 0; }

void registrarBotao(int b) {
  unsigned long agora = millis();

  if (ultimoBotaoMs != 0 && (agora - ultimoBotaoMs) > TIMEOUT_TECLAS_MS) {
    resetSequencia();
  }

  seq[idx++] = b;
  ultimoBotaoMs = agora;

  if (idx == 3) {
    // ARMAR: 1-2-3
    if (seq[0]==1 && seq[1]==2 && seq[2]==3) {
      if (estado == DESARMADO) setEstado(SAIDA);
    }
    // DESARMAR: 3-2-1
    if (seq[0]==3 && seq[1]==2 && seq[2]==1) {
      setEstado(DESARMADO);
    }
    resetSequencia();
  }
}

void lerBotoes() {
  if (digitalRead(B1) == LOW) { registrarBotao(1); delay(180); }
  if (digitalRead(B2) == LOW) { registrarBotao(2); delay(180); }
  if (digitalRead(B3) == LOW) { registrarBotao(3); delay(180); }
}

void setup() {
  Serial.begin(115200);

  pinMode(PIR, INPUT);

  pinMode(LED_ARMADO, OUTPUT);
  pinMode(LED_DESARMADO, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  pinMode(B1, INPUT_PULLUP);
  pinMode(B2, INPUT_PULLUP);
  pinMode(B3, INPUT_PULLUP);

  setEstado(DESARMADO);

  // PIR estabiliza
  delay(2000);

  // começa tentativa de Wi-Fi (sem travar)
  iniciarWiFiSemTravar();
}

void loop() {
  // SEMPRE lê botões (não depende do Wi-Fi)
  lerBotoes();

  // Wi-Fi em paralelo (não trava)
  iniciarWiFiSemTravar();
  atualizarWiFiSemTravar();

  unsigned long agora = millis();

  // SAÍDA (10s) com LED vermelho piscando
  if (estado == SAIDA) {
    if (agora - ultimoPiscaMs >= 300) {
      ultimoPiscaMs = agora;
      pisca = !pisca;
      digitalWrite(LED_ARMADO, pisca ? HIGH : LOW);
    }
    if (agora - inicioSaidaMs >= TEMPO_SAIDA_MS) {
      digitalWrite(LED_ARMADO, HIGH);
      setEstado(ARMADO);
    }
  }

  // ARMADO -> detectou movimento -> ALARME
  if (estado == ARMADO) {
    if (digitalRead(PIR) == HIGH) {
      setEstado(ALARME);
    }
  }

  // ALARME -> sirene + Telegram (se online)
  if (estado == ALARME) {
    atualizarSirene();

    // manda 1x quando dispara (se estiver conectado)
    if (!alertaEnviadoNoDisparo && WiFi.status() == WL_CONNECTED) {
      bot.sendMessage(CHAT_ID, "🚨 ALERTA! Movimento detectado — alarme DISPARADO!", "");
      alertaEnviadoNoDisparo = true;
      ultimoAlertaMs = agora;
    }

    // opcional: repetir a cada 60s
    if (WiFi.status() == WL_CONNECTED && (agora - ultimoAlertaMs) > ALERTA_COOLDOWN_MS) {
      bot.sendMessage(CHAT_ID, "🚨 Alarme ainda ativo! Verifique o local.", "");
      ultimoAlertaMs = agora;
    }
  }
}