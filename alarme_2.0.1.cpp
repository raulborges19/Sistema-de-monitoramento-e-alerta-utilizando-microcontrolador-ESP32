#include <WiFi.h>                  // Biblioteca Wi-Fi do ESP32
#include <WiFiClientSecure.h>      // Cliente HTTPS seguro
#include <UniversalTelegramBot.h>  // Biblioteca do bot do Telegram

// ====== Wi-Fi / Telegram (preencha no seu PC) ======
const char* ssid = ""; //seu Wifi
const char* password = ""; //senha do seu Wifi

#define BOT_TOKEN "" // token do seu bot no telegram
#define CHAT_ID   "" //id do seu chat do telegram

// Objetos usados para comunicação segura com o Telegram
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
// Máquina de estados principal do sistema
enum Estado { DESARMADO, SAIDA, ARMADO, ALARME };
Estado estado = DESARMADO;

// ===================== SEQUÊNCIA =====================
// Armazena sequência dos botões pressionados
int seq[3];
int idx = 0;
unsigned long ultimoBotaoMs = 0;

// ===================== SAÍDA (pisca) =====================
// Controle da contagem de saída e do LED piscando
unsigned long inicioSaidaMs = 0;
unsigned long ultimoPiscaMs = 0;
bool pisca = false;

// ===================== SIRENE =====================
// Controle do padrão sonoro do buzzer
unsigned long sireneInicioCicloMs = 0;
bool buzzerLigado = false;

// ===================== WIFI NÃO BLOQUEANTE =====================
// Controle para reconectar sem travar o programa
bool wifiTentando = false;
unsigned long wifiInicioTentativaMs = 0;
unsigned long wifiUltimaTentativaMs = 0;

// ===================== FLAGS DE NOTIFICAÇÃO =====================
// Flags que evitam envio repetido das mesmas mensagens
bool msgArmedEnviada = false;          // evita repetir "armado"
bool msgDisarmedEnviada = false;       // evita repetir "desarmado"
bool msgMovimentoEnviada = false;      // evita repetir "movimento detectado"
bool msgMovCessouEnviada = false;      // evita repetir "movimento cessou"

bool telegramOk() {
  // Telegram só funciona se houver conexão Wi-Fi
  return (WiFi.status() == WL_CONNECTED);
}

void enviarTelegram(const String& txt) {
  // Só tenta enviar se houver conexão
  if (!telegramOk()) return;
  bot.sendMessage(CHAT_ID, txt, "");
}

// ===================== WIFI =====================
void iniciarWiFiSemTravar() {
  unsigned long agora = millis();
  if (wifiTentando) return;                   // Já está tentando conectar
  if (WiFi.status() == WL_CONNECTED) return;  // Já está conectado
  if (agora - wifiUltimaTentativaMs < WIFI_RETRY_MS) return; // Ainda não é hora de tentar de novo

  wifiUltimaTentativaMs = agora;
  wifiTentando = true;
  wifiInicioTentativaMs = agora;

  Serial.println("\n[WIFI] Iniciando tentativa...");

  // Reinicializa o Wi-Fi para limpar estados anteriores
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
    // Conexão realizada com sucesso
    wifiTentando = false;
    Serial.println("[WIFI] Conectado!");
    Serial.print("[WIFI] IP: ");
    Serial.println(WiFi.localIP());

    client.setInsecure(); // HTTPS
    enviarTelegram("📶 ESP32 online. Alarme pronto.");
    return;
  }

  if (millis() - wifiInicioTentativaMs > WIFI_TIMEOUT_MS) {
    // Se demorou demais, encerra a tentativa
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

  // Tempo corrido dentro do ciclo da sirene
  unsigned long t = agora - sireneInicioCicloMs;
  if (t >= 1600) { sireneInicioCicloMs = agora; t = 0; }

  // 300 ON, 200 OFF, 300 ON, 800 OFF
  bool deveLigar = (t < 300) || (t >= 500 && t < 800);

  // Atualiza o buzzer apenas quando necessário
  if (deveLigar != buzzerLigado) {
    buzzerLigado = deveLigar;
    digitalWrite(BUZZER, buzzerLigado ? HIGH : LOW);
  }
}

void buzzerOff() {
  // Desliga o buzzer e reinicia o ciclo da sirene
  sireneInicioCicloMs = 0;
  buzzerLigado = false;
  digitalWrite(BUZZER, LOW);
}

// ===================== ESTADOS =====================
void setEstado(Estado novo) {
  // Atualiza o estado geral do sistema
  estado = novo;

  if (estado == DESARMADO) {
    // Sistema desarmado
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
    // Início da contagem de saída
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
    // Sistema armado e vigiando
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
    // Estado de alarme disparado
    digitalWrite(LED_DESARMADO, LOW);
    digitalWrite(LED_ARMADO, HIGH);
    // sirene depende do PIR em loop
  }
}

// ===================== SEQUÊNCIA DOS BOTÕES =====================
void resetSequencia() { 
  // Zera a sequência armazenada
  idx = 0; 
  ultimoBotaoMs = 0; 
}

void registrarBotao(int b) {
  unsigned long agora = millis();

  // Se houver demora excessiva entre teclas, limpa a sequência
  if (ultimoBotaoMs != 0 && (agora - ultimoBotaoMs) > TIMEOUT_TECLAS_MS) {
    resetSequencia();
  }

  // Registra o botão pressionado
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

    // Após verificar, limpa a sequência
    resetSequencia();
  }
}

void lerBotoes() {
  // Leitura simples dos botões com debounce via delay
  if (digitalRead(B1) == LOW) { registrarBotao(1); delay(180); }
  if (digitalRead(B2) == LOW) { registrarBotao(2); delay(180); }
  if (digitalRead(B3) == LOW) { registrarBotao(3); delay(180); }
}

// ===================== SETUP / LOOP =====================
void setup() {
  Serial.begin(115200);

  // Configurações dos pinos
  pinMode(PIR, INPUT);

  pinMode(LED_ARMADO, OUTPUT);
  pinMode(LED_DESARMADO, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  pinMode(B1, INPUT_PULLUP);
  pinMode(B2, INPUT_PULLUP);
  pinMode(B3, INPUT_PULLUP);

  // Inicia em estado desarmado
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