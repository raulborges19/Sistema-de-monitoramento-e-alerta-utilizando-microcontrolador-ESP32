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

// ===================== CONFIG =====================
const unsigned long TEMPO_SAIDA_MS        = 10000; // 10s para sair
const unsigned long TIMEOUT_TECLAS_MS     = 3000;  // timeout da sequência
const unsigned long WIFI_RETRY_MS         = 20000; // tenta reconectar a cada 20s
const unsigned long WIFI_TIMEOUT_MS       = 12000; // timeout por tentativa

// ===================== ESTADOS =====================
enum Estado { DESARMADO, SAIDA, ARMADO, ALARME };
Estado estado = DESARMADO;

// ===================== SEQUÊNCIA =====================
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

// ===================== FLAGS DE NOTIFICAÇÃO =====================
bool msgArmedEnviada = false;          // evita repetir "armado"
bool msgDisarmedEnviada = false;       // evita repetir "desarmado"
bool msgMovimentoEnviada = false;      // evita repetir "movimento detectado"
bool msgMovCessouEnviada = false;      // evita repetir "movimento cessou"

bool telegramOk() {
  return (WiFi.status() == WL_CONNECTED);
}

void enviarTelegram(const String& txt) {
  if (!telegramOk()) return;
  bot.sendMessage(CHAT_ID, txt, "");
}

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
    enviarTelegram("📶 ESP32 online. Alarme pronto.");
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
// Só toca quando PIR estiver HIGH (no estado ALARME)
void atualizarSirene() {
  unsigned long agora = millis();
  if (sireneInicioCicloMs == 0) sireneInicioCicloMs = agora;

  unsigned long t = agora - sireneInicioCicloMs;
  if (t >= 1600) { sireneInicioCicloMs = agora; t = 0; }

  // 300 ON, 200 OFF, 300 ON, 800 OFF
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

    // notificações (desarmado)
    msgDisarmedEnviada = false; // permite enviar agora (no loop abaixo)
    msgArmedEnviada = false;

    // reset movimento
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

    // ainda não está armado de verdade
    msgArmedEnviada = false;
  }

  if (estado == ARMADO) {
    digitalWrite(LED_DESARMADO, LOW);
    digitalWrite(LED_ARMADO, HIGH);
    buzzerOff();

    // pronto para detectar movimento
    msgMovimentoEnviada = false;
    msgMovCessouEnviada = false;

    // desarmado não faz sentido aqui
    msgDisarmedEnviada = false;
  }

  if (estado == ALARME) {
    digitalWrite(LED_DESARMADO, LOW);
    digitalWrite(LED_ARMADO, HIGH);
    // sirene depende do PIR em loop
  }
}

// ===================== SEQUÊNCIA DOS BOTÕES =====================
void resetSequencia() { idx = 0; ultimoBotaoMs = 0; }

void registrarBotao(int b) {
  unsigned long agora = millis();

  if (ultimoBotaoMs != 0 && (agora - ultimoBotaoMs) > TIMEOUT_TECLAS_MS) {
    resetSequencia();
  }

  seq[idx++] = b;
  ultimoBotaoMs = agora;

  if (idx == 3) {
    // ARMAR 1-2-3 (só se estiver DESARMADO)
    if (seq[0]==1 && seq[1]==2 && seq[2]==3) {
      if (estado == DESARMADO) setEstado(SAIDA);
    }

    // DESARMAR 3-2-1 (de qualquer estado)
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

  // PIR estabiliza um pouco
  delay(2000);

  iniciarWiFiSemTravar();
}

void loop() {
  // sempre lê botões
  lerBotoes();

  // Wi-Fi em paralelo (não trava)
  iniciarWiFiSemTravar();
  atualizarWiFiSemTravar();

  unsigned long agora = millis();

  // ========= Mensagem de DESARMADO (transição) =========
  if (estado == DESARMADO && !msgDisarmedEnviada) {
    enviarTelegram("🔓 Alarme DESARMADO.");
    msgDisarmedEnviada = true;
  }

  // ========= SAÍDA (10s) com LED vermelho piscando =========
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

  // ========= Mensagem de ARMADO (após os 10s) =========
  if (estado == ARMADO && !msgArmedEnviada) {
    enviarTelegram("🔒 Alarme ARMADO.");
    msgArmedEnviada = true;
  }

  // ========= ARMADO -> se PIR detectar movimento entra em ALARME =========
  if (estado == ARMADO) {
    if (digitalRead(PIR) == HIGH) {
      setEstado(ALARME);
    }
  }

  // ========= ALARME: sirene só enquanto PIR estiver HIGH =========
  if (estado == ALARME) {
    int mov = digitalRead(PIR);

    if (mov == HIGH) {
      // sirene ativa
      atualizarSirene();

      // manda "movimento detectado" uma vez
      if (!msgMovimentoEnviada) {
        enviarTelegram("🚨 Movimento detectado!");
        msgMovimentoEnviada = true;
        msgMovCessouEnviada = false; // permite enviar cessou depois
      }

    } else {
      // movimento cessou -> para sirene e avisa
      buzzerOff();

      if (!msgMovCessouEnviada) {
        enviarTelegram("✅ Movimento cessou.");
        msgMovCessouEnviada = true;
      }

      // volta para ARMADO (pronto para próxima detecção)
      setEstado(ARMADO);
    }
  }
}