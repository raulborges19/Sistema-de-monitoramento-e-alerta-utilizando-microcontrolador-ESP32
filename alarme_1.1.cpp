// ====== PINOS (os mesmos que você usou) ======
#define PIR 33
#define LED_ARMADO 26      // vermelho
#define LED_DESARMADO 27   // verde
#define BUZZER 25

#define B1 14
#define B2 12
#define B3 13

// ====== CONFIG ======
const unsigned long TEMPO_SAIDA_MS = 10000;   // 10s para sair
const unsigned long TIMEOUT_TECLAS_MS = 3000; // zera sequência se demorar

// ====== ESTADOS ======
enum Estado { DESARMADO, SAIDA, ARMADO, ALARME };
Estado estado = DESARMADO;

// ====== SEQUÊNCIA DOS BOTÕES ======
int seq[3];
int idx = 0;
unsigned long ultimoBotaoMs = 0;

// ====== TIMERS ======
unsigned long inicioSaidaMs = 0;
unsigned long ultimoPiscaMs = 0;
bool pisca = false;

// ====== SIRENE ======
unsigned long sireneUltimoMs = 0;
unsigned long sireneInicioCicloMs = 0;
bool buzzerLigado = false;

// Sirene: 300ms ON, 200ms OFF, 300ms ON, 800ms OFF (repete) => "piiii piiiii"
void atualizarSirene() {
  unsigned long agora = millis();

  // Inicia ciclo se necessário
  if (sireneInicioCicloMs == 0) {
    sireneInicioCicloMs = agora;
    sireneUltimoMs = agora;
    buzzerLigado = true;
    digitalWrite(BUZZER, HIGH);
    return;
  }

  unsigned long t = agora - sireneInicioCicloMs;

  // janela total do ciclo: 1600ms
  if (t >= 1600) {
    sireneInicioCicloMs = agora;
    t = 0;
  }

  // Define se deve estar ligado/desligado em cada trecho
  bool deveLigar =
      (t < 300) ||              // 0-300 ON
      (t >= 500 && t < 800);    // 500-800 ON

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

void setEstado(Estado novo) {
  estado = novo;

  if (estado == DESARMADO) {
    digitalWrite(LED_DESARMADO, HIGH);
    digitalWrite(LED_ARMADO, LOW);
    buzzerOff();
  }

  if (estado == SAIDA) {
    digitalWrite(LED_DESARMADO, LOW);
    // LED vermelho vai piscar durante a saída
    inicioSaidaMs = millis();
    ultimoPiscaMs = millis();
    pisca = true;
    digitalWrite(LED_ARMADO, HIGH);
    buzzerOff();
  }

  if (estado == ARMADO) {
    digitalWrite(LED_DESARMADO, LOW);
    digitalWrite(LED_ARMADO, HIGH); // vermelho fixo
    buzzerOff();
  }

  if (estado == ALARME) {
    digitalWrite(LED_DESARMADO, LOW);
    digitalWrite(LED_ARMADO, HIGH); // mantém vermelho
    // sirene começa pelo atualizarSirene()
  }
}

void resetSequencia() {
  idx = 0;
  ultimoBotaoMs = 0;
}

void registrarBotao(int b) {
  unsigned long agora = millis();

  // timeout entre teclas
  if (ultimoBotaoMs != 0 && (agora - ultimoBotaoMs) > TIMEOUT_TECLAS_MS) {
    resetSequencia();
  }

  seq[idx++] = b;
  ultimoBotaoMs = agora;

  // quando completa 3 teclas, verifica
  if (idx == 3) {
    // ARMAR: 1-2-3
    if (seq[0]==1 && seq[1]==2 && seq[2]==3) {
      if (estado == DESARMADO) setEstado(SAIDA);
    }
    // DESARMAR: 3-2-1 (desarma de qualquer estado)
    if (seq[0]==3 && seq[1]==2 && seq[2]==1) {
      setEstado(DESARMADO);
    }
    resetSequencia();
  }
}

void lerBotoes() {
  if (digitalRead(B1) == LOW) { registrarBotao(1); delay(200); }
  if (digitalRead(B2) == LOW) { registrarBotao(2); delay(200); }
  if (digitalRead(B3) == LOW) { registrarBotao(3); delay(200); }
}

void setup() {
  pinMode(PIR, INPUT);

  pinMode(LED_ARMADO, OUTPUT);
  pinMode(LED_DESARMADO, OUTPUT);

  pinMode(BUZZER, OUTPUT);

  pinMode(B1, INPUT_PULLUP);
  pinMode(B2, INPUT_PULLUP);
  pinMode(B3, INPUT_PULLUP);

  Serial.begin(115200);

  setEstado(DESARMADO);

  // Dica: PIR pode levar alguns segundos para estabilizar
  delay(2000);
}

void loop() {
  lerBotoes();

  unsigned long agora = millis();

  // ====== ESTADO SAÍDA (10s para sair) ======
  if (estado == SAIDA) {
    // pisca LED vermelho
    if (agora - ultimoPiscaMs >= 300) {
      ultimoPiscaMs = agora;
      pisca = !pisca;
      digitalWrite(LED_ARMADO, pisca ? HIGH : LOW);
    }

    // terminou contagem
    if (agora - inicioSaidaMs >= TEMPO_SAIDA_MS) {
      digitalWrite(LED_ARMADO, HIGH); // vermelho fixo
      setEstado(ARMADO);
    }
  }

  // ====== ESTADO ARMADO ======
  if (estado == ARMADO) {
    if (digitalRead(PIR) == HIGH) {
      setEstado(ALARME);
    }
  }

  // ====== ESTADO ALARME ======
  if (estado == ALARME) {
    atualizarSirene();
    // desarmar é sempre 3-2-1 (já tratado em lerBotoes)
  }
}