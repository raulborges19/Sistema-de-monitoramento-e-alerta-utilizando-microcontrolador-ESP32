#include <WiFi.h>                  // Biblioteca para conexão Wi-Fi no ESP32
#include <WiFiClientSecure.h>      // Cliente seguro HTTPS
#include <UniversalTelegramBot.h>  // Biblioteca para interação com o bot do Telegram

// ====== Wi-Fi / Telegram (preencha no seu PC) ======
const char* ssid = ""; //seu Wifi
const char* password = ""; //senha do seu Wifi

#define BOT_TOKEN "" // token do seu bot no telegram
#define CHAT_ID   "" //id do seu chat do telegram

// Cria o cliente HTTPS e o objeto do bot Telegram
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
// Tempo de saída ao armar o sistema
const unsigned long TEMPO_SAIDA_MS        = 10000; // 10s

// Tempo máximo entre pressões de botões
const unsigned long TIMEOUT_TECLAS_MS     = 3000;  // 3s

// Intervalo entre mensagens repetidas de alerta no Telegram
const unsigned long ALERTA_COOLDOWN_MS    = 60000; // 60s (repetição opcional)

// Intervalo mínimo entre tentativas de reconexão Wi-Fi
const unsigned long WIFI_RETRY_MS         = 20000; // tenta reconectar a cada 20s

// Tempo máximo de cada tentativa de conexão Wi-Fi
const unsigned long WIFI_TIMEOUT_MS       = 12000; // tempo máximo tentando por tentativa

// ====== ESTADOS DO ALARME ======
// Máquina de estados do sistema
enum Estado { DESARMADO, SAIDA, ARMADO, ALARME };
Estado estado = DESARMADO;

// ====== SEQUÊNCIA ======
// Armazena os 3 botões pressionados
int seq[3];
int idx = 0;
unsigned long ultimoBotaoMs = 0;

// ====== SAÍDA (pisca) ======
// Variáveis de tempo para controlar o LED piscando no estado SAIDA
unsigned long inicioSaidaMs = 0;
unsigned long ultimoPiscaMs = 0;
bool pisca = false;

// ====== SIRENE ======
// Controle do padrão sonoro da sirene
unsigned long sireneInicioCicloMs = 0;
bool buzzerLigado = false;

// ====== TELEGRAM ======
// Indica se já foi enviado alerta no disparo atual
bool alertaEnviadoNoDisparo = false;

// Guarda o instante do último alerta enviado
unsigned long ultimoAlertaMs = 0;

// ====== WIFI NÃO BLOQUEANTE ======
// Controla tentativas de conexão sem travar o loop principal
bool wifiTentando = false;
unsigned long wifiInicioTentativaMs = 0;
unsigned long wifiUltimaTentativaMs = 0;

void iniciarWiFiSemTravar() {
  // só inicia nova tentativa se passou o intervalo
  unsigned long agora = millis();
  if (wifiTentando) return;                 // Já está tentando conectar
  if (WiFi.status() == WL_CONNECTED) return; // Já está conectado
  if (agora - wifiUltimaTentativaMs < WIFI_RETRY_MS) return; // Ainda não passou o tempo de nova tentativa

  wifiUltimaTentativaMs = agora;
  wifiTentando = true;
  wifiInicioTentativaMs = agora;

  Serial.println("\n[WIFI] Iniciando tentativa...");

  // limpa estado anterior (evita “cannot set config”)
  WiFi.mode(WIFI_OFF);
  delay(50);
  WiFi.disconnect(true, true);
  delay(50);

  // Reinicia a interface Wi-Fi em modo estação
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

  // Tempo dentro do ciclo atual da sirene
  unsigned long t = agora - sireneInicioCicloMs;
  if (t >= 1600) { sireneInicioCicloMs = agora; t = 0; }

  // Define o padrão ligado/desligado
  bool deveLigar = (t < 300) || (t >= 500 && t < 800);

  // Só altera o buzzer se necessário
  if (deveLigar != buzzerLigado) {
    buzzerLigado = deveLigar;
    digitalWrite(BUZZER, buzzerLigado ? HIGH : LOW);
  }
}

void buzzerOff() {
  // Reseta o ciclo da sirene e desliga o buzzer
  sireneInicioCicloMs = 0;
  buzzerLigado = false;
  digitalWrite(BUZZER, LOW);
}

// ====== ESTADOS ======
void setEstado(Estado novo) {
  // Atualiza o estado geral do sistema
  estado = novo;

  if (estado == DESARMADO) {
    // Sistema desarmado
    digitalWrite(LED_DESARMADO, HIGH);
    digitalWrite(LED_ARMADO, LOW);
    buzzerOff();
    alertaEnviadoNoDisparo = false;
  }

  if (estado == SAIDA) {
    // Contagem para saída antes de armar de fato
    digitalWrite(LED_DESARMADO, LOW);
    inicioSaidaMs = millis();
    ultimoPiscaMs = millis();
    pisca = true;
    digitalWrite(LED_ARMADO, HIGH);
    buzzerOff();
  }

  if (estado == ARMADO) {
    // Sistema armado e pronto para detectar movimento
    digitalWrite(LED_DESARMADO, LOW);
    digitalWrite(LED_ARMADO, HIGH);
    buzzerOff();
    alertaEnviadoNoDisparo = false;
  }

  if (estado == ALARME) {
    // Sistema disparado
    digitalWrite(LED_DESARMADO, LOW);
    digitalWrite(LED_ARMADO, HIGH);
  }
}

void resetSequencia() { 
  // Limpa a sequência digitada e o tempo da última tecla
  idx = 0; 
  ultimoBotaoMs = 0; 
}

void registrarBotao(int b) {
  unsigned long agora = millis();

  // Se demorou demais entre teclas, reinicia a sequência
  if (ultimoBotaoMs != 0 && (agora - ultimoBotaoMs) > TIMEOUT_TECLAS_MS) {
    resetSequencia();
  }

  // Armazena o botão pressionado
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
    // Após verificar, limpa a sequência
    resetSequencia();
  }
}

void lerBotoes() {
  // Leitura dos botões com pequeno debounce
  if (digitalRead(B1) == LOW) { registrarBotao(1); delay(180); }
  if (digitalRead(B2) == LOW) { registrarBotao(2); delay(180); }
  if (digitalRead(B3) == LOW) { registrarBotao(3); delay(180); }
}

void setup() {
  // Inicializa comunicação serial
  Serial.begin(115200);

  // Configura os pinos
  pinMode(PIR, INPUT);

  pinMode(LED_ARMADO, OUTPUT);
  pinMode(LED_DESARMADO, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  pinMode(B1, INPUT_PULLUP);
  pinMode(B2, INPUT_PULLUP);
  pinMode(B3, INPUT_PULLUP);

  // Inicia em estado desarmado
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