#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

// ====== Wi-Fi / Telegram (preencha no seu PC) ======
const char* ssid = ""; //seu Wifi
const char* password = ""; //senha do seu Wifi

#define BOT_TOKEN "" // token do seu bot no telegram
#define CHAT_ID   "" //id do seu chat do telegram

const char* PIN_CODE = ""; //defina o seu pin de segurança

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

// ===================== CONFIG =====================
const unsigned long TEMPO_SAIDA_MS        = 10000;
const unsigned long TIMEOUT_TECLAS_MS     = 3000;

const unsigned long WIFI_RETRY_MS         = 20000;
const unsigned long WIFI_TIMEOUT_MS       = 12000;

// Telegram polling
const unsigned long TELEGRAM_POLL_MS      = 3000;

// ===================== ESTADOS =====================
enum Estado { DESARMADO, SAIDA, ARMADO, ALARME };
Estado estado = DESARMADO;

// ===================== SEQUÊNCIA BOTÕES =====================
int seq[3];
int idx = 0;
unsigned long ultimoBotaoMs = 0;

// ===================== SAÍDA (pisca) =====================
unsigned long inicioSaidaMs = 0;
unsigned long ultimoPiscaMs = 0;
bool pisca = false;

// ===================== SIRENE =====================
unsigned long sireneInicioCicloMs = 0;
bool buzzerLigado = false;

// ===================== WIFI NÃO BLOQUEANTE =====================
bool wifiTentando = false;
unsigned long wifiInicioTentativaMs = 0;
unsigned long wifiUltimaTentativaMs = 0;

// ===================== NOTIFICAÇÕES =====================
bool msgArmedEnviada = false;
bool msgDisarmedEnviada = false;
bool msgMovimentoEnviada = false;
bool msgMovCessouEnviada = false;

// ===================== TELEGRAM UPDATES =====================
unsigned long ultimoTelegramPollMs = 0;
long lastUpdateId = 0; // controla mensagens já processadas

bool telegramOk() { return WiFi.status() == WL_CONNECTED; }

void enviarTelegram(const String& txt) {
  if (!telegramOk()) return;
  bot.sendMessage(CHAT_ID, txt, "");
}

// ===== controle de borda dos botões =====
int ultimoB1 = HIGH;
int ultimoB2 = HIGH;
int ultimoB3 = HIGH;

// ===================== WIFI =====================
void iniciarWiFiSemTravar() {
  unsigned long agora = millis();
  if (wifiTentando) return;
  if (WiFi.status() == WL_CONNECTED) return;
  if (agora - wifiUltimaTentativaMs < WIFI_RETRY_MS) return;

  wifiUltimaTentativaMs = agora;
  wifiTentando = true;
  wifiInicioTentativaMs = agora;

  Serial.println("\n[WIFI] Iniciando tentativa...");

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

  if (WiFi.status() == WL_CONNECTED) {
    wifiTentando = false;
    Serial.println("[WIFI] Conectado!");
    Serial.print("[WIFI] IP: ");
    Serial.println(WiFi.localIP());

    client.setInsecure(); // HTTPS
    client.setTimeout(300); 
    enviarTelegram("📶 ESP32 online. Use /status PIN para ver o estado.");
    return;
  }

  if (millis() - wifiInicioTentativaMs > WIFI_TIMEOUT_MS) {
    wifiTentando = false;
    Serial.println("[WIFI] Timeout. Alarme continua OFFLINE.");
    WiFi.disconnect(true, true);
    WiFi.mode(WIFI_OFF);
  }
}

// ===================== SIRENE “piiii piiiii” =====================
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

// ===================== ESTADOS =====================
void setEstado(Estado novo) {
  estado = novo;

  if (estado == DESARMADO) {
    digitalWrite(LED_DESARMADO, HIGH);
    digitalWrite(LED_ARMADO, LOW);
    buzzerOff();

    msgDisarmedEnviada = false;
    msgArmedEnviada = false;

    msgMovimentoEnviada = false;
    msgMovCessouEnviada = false;
  }

  if (estado == SAIDA) {
    digitalWrite(LED_DESARMADO, LOW);
    inicioSaidaMs = millis();
    ultimoPiscaMs = millis();
    pisca = true;
    digitalWrite(LED_ARMADO, HIGH);
    buzzerOff();

    msgArmedEnviada = false;
  }

  if (estado == ARMADO) {
    digitalWrite(LED_DESARMADO, LOW);
    digitalWrite(LED_ARMADO, HIGH);
    buzzerOff();

    msgMovimentoEnviada = false;
    msgMovCessouEnviada = false;

    msgDisarmedEnviada = false;
  }

  if (estado == ALARME) {
    digitalWrite(LED_DESARMADO, LOW);
    digitalWrite(LED_ARMADO, HIGH);
  }
}

String nomeEstado() {
  switch (estado) {
    case DESARMADO: return "DESARMADO";
    case SAIDA:     return "SAIDA (contagem)";
    case ARMADO:    return "ARMADO";
    case ALARME:    return "ALARME";
  }
  return "DESCONHECIDO";
}

// ===================== BOTÕES (sequência física continua valendo) =====================
void resetSequencia() { idx = 0; ultimoBotaoMs = 0; }

void registrarBotao(int b) {
  unsigned long agora = millis();

  if (ultimoBotaoMs != 0 && (agora - ultimoBotaoMs) > TIMEOUT_TECLAS_MS) {
    resetSequencia();
  }

  seq[idx++] = b;
  ultimoBotaoMs = agora;

  if (idx == 3) {
    if (seq[0]==1 && seq[1]==2 && seq[2]==3) {
      if (estado == DESARMADO) setEstado(SAIDA);
    }
    if (seq[0]==3 && seq[1]==2 && seq[2]==1) {
      setEstado(DESARMADO);
    }
    resetSequencia();
  }
}

void lerBotoes() {
  int estadoB1 = digitalRead(B1);
  int estadoB2 = digitalRead(B2);
  int estadoB3 = digitalRead(B3);

  // Detecta transição HIGH -> LOW
  if (ultimoB1 == HIGH && estadoB1 == LOW) {
    registrarBotao(1);
  }

  if (ultimoB2 == HIGH && estadoB2 == LOW) {
    registrarBotao(2);
  }

  if (ultimoB3 == HIGH && estadoB3 == LOW) {
    registrarBotao(3);
  }

  ultimoB1 = estadoB1;
  ultimoB2 = estadoB2;
  ultimoB3 = estadoB3;
}

// ===================== TELEGRAM COMANDOS =====================
// Formato: "/arm 1234", "/disarm 1234", "/status 1234", "/help"
bool pinOk(const String& text) {
  // pega a parte depois do espaço
  int sp = text.indexOf(' ');
  if (sp < 0) return false;
  String pin = text.substring(sp + 1);
  pin.trim();
  return pin == PIN_CODE;
}

void processarComandoTelegram(const String& text) {
  String t = text;
  t.trim();

  if (t.startsWith("/help")) {
    enviarTelegram("Comandos:\n"
                   "/arm PIN\n"
                   "/disarm PIN\n"
                   "/status PIN\n"
                   "Ex: /arm 1234");
    return;
  }

  // Segurança: exige PIN em todos
  if (!(t.startsWith("/arm") || t.startsWith("/disarm") || t.startsWith("/status"))) {
    return;
  }

  if (!pinOk(t)) {
    enviarTelegram("❌ PIN incorreto.");
    return;
  }

  if (t.startsWith("/status")) {
    enviarTelegram("📟 Estado: " + nomeEstado());
    return;
  }

  if (t.startsWith("/arm")) {
    if (estado == DESARMADO) {
      setEstado(SAIDA);
      enviarTelegram("⏱ Armando... (aguarde 10s)");
    } else {
      enviarTelegram("ℹ️ Já está em: " + nomeEstado());
    }
    return;
  }

  if (t.startsWith("/disarm")) {
    setEstado(DESARMADO);
    enviarTelegram("🔓 Desarmando...");
    return;
  }
}

void checarTelegram() {
  if (!telegramOk()) return;

  unsigned long agora = millis();
  if (agora - ultimoTelegramPollMs < TELEGRAM_POLL_MS) return;
  ultimoTelegramPollMs = agora;

  int numNew = bot.getUpdates(lastUpdateId);

  if (numNew <= 0) return;

  // Processa só o lote atual, sem while infinito
  for (int i = 0; i < numNew; i++) {
    String chat_id = bot.messages[i].chat_id;
    if (chat_id != String(CHAT_ID)) continue;

    String text = bot.messages[i].text;
    long update_id = bot.messages[i].update_id;

    lastUpdateId = update_id + 1;
    processarComandoTelegram(text);
  }
}

// ===================== SETUP / LOOP =====================
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

  delay(2000); // PIR estabilizar

  iniciarWiFiSemTravar();
}

void loop() {
  lerBotoes();

  iniciarWiFiSemTravar();
  atualizarWiFiSemTravar();

  checarTelegram();

  unsigned long agora = millis();

  // Mensagem ao entrar em DESARMADO (uma vez)
  if (estado == DESARMADO && !msgDisarmedEnviada) {
    enviarTelegram("🔓 Alarme DESARMADO.");
    msgDisarmedEnviada = true;
  }

  // SAÍDA (10s) piscando
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

  // Mensagem ao ficar ARMADO (após 10s)
  if (estado == ARMADO && !msgArmedEnviada) {
    enviarTelegram("🔒 Alarme ARMADO.");
    msgArmedEnviada = true;
  }

  // ARMADO -> movimento -> ALARME
  if (estado == ARMADO) {
    if (digitalRead(PIR) == HIGH) {
      setEstado(ALARME);
    }
  }

  // ALARME: toca só enquanto PIR HIGH, e manda “cessou” quando voltar LOW
  if (estado == ALARME) {
    int mov = digitalRead(PIR);

    if (mov == HIGH) {
      atualizarSirene();

      if (!msgMovimentoEnviada) {
        enviarTelegram("🚨 ALARME DISPARADO");
        msgMovimentoEnviada = true;
        msgMovCessouEnviada = false;
      }
    } else {
      buzzerOff();

      if (!msgMovCessouEnviada) {
        enviarTelegram("✅ Reestabelecido.");
        msgMovCessouEnviada = true;
      }

      setEstado(ARMADO);
    }
  }
}