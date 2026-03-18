// ====== PINOS (os mesmos que você usou) ======
#define PIR 33
#define LED_ARMADO 26      // vermelho
#define LED_DESARMADO 27   // verde
#define BUZZER 25

#define B1 14
#define B2 12
#define B3 13

// ====== CONFIG ======
// Tempo de saída após armar o sistema: permite que a pessoa saia antes do alarme começar a vigiar
const unsigned long TEMPO_SAIDA_MS = 10000;   // 10s para sair

// Tempo máximo entre pressões de botões antes de zerar a sequência digitada
const unsigned long TIMEOUT_TECLAS_MS = 3000; // zera sequência se demorar

// ====== ESTADOS ======
// Máquina de estados do sistema
enum Estado { DESARMADO, SAIDA, ARMADO, ALARME };
Estado estado = DESARMADO;

// ====== SEQUÊNCIA DOS BOTÕES ======
// Armazena a sequência pressionada
int seq[3];

// Índice atual da sequência
int idx = 0;

// Momento da última tecla pressionada
unsigned long ultimoBotaoMs = 0;

// ====== TIMERS ======
// Momento em que começou a contagem de saída
unsigned long inicioSaidaMs = 0;

// Controle de piscar o LED durante a saída
unsigned long ultimoPiscaMs = 0;
bool pisca = false;

// ====== SIRENE ======
// Variáveis para controlar o padrão sonoro da sirene
unsigned long sireneUltimoMs = 0;
unsigned long sireneInicioCicloMs = 0;
bool buzzerLigado = false;

// Sirene: 300ms ON, 200ms OFF, 300ms ON, 800ms OFF (repete) => "piiii piiiii"
void atualizarSirene() {
  unsigned long agora = millis();

  // Inicia ciclo se necessário
  // Se ainda não começou um ciclo da sirene, inicia agora
  if (sireneInicioCicloMs == 0) {
    sireneInicioCicloMs = agora;
    sireneUltimoMs = agora;
    buzzerLigado = true;
    digitalWrite(BUZZER, HIGH);
    return;
  }

  // Tempo decorrido dentro do ciclo atual da sirene
  unsigned long t = agora - sireneInicioCicloMs;

  // janela total do ciclo: 1600ms
  // Quando chega no fim do ciclo, reinicia
  if (t >= 1600) {
    sireneInicioCicloMs = agora;
    t = 0;
  }

  // Define se deve estar ligado/desligado em cada trecho
  bool deveLigar =
      (t < 300) ||              // 0-300 ON
      (t >= 500 && t < 800);    // 500-800 ON

  // Só muda o buzzer se o estado desejado for diferente do atual
  if (deveLigar != buzzerLigado) {
    buzzerLigado = deveLigar;
    digitalWrite(BUZZER, buzzerLigado ? HIGH : LOW);
  }
}

void buzzerOff() {
  // Reseta o ciclo da sirene e garante o buzzer desligado
  sireneInicioCicloMs = 0;
  buzzerLigado = false;
  digitalWrite(BUZZER, LOW);
}

void setEstado(Estado novo) {
  // Atualiza o estado geral do sistema
  estado = novo;

  if (estado == DESARMADO) {
    // Sistema desarmado: LED verde aceso, vermelho apagado, buzzer desligado
    digitalWrite(LED_DESARMADO, HIGH);
    digitalWrite(LED_ARMADO, LOW);
    buzzerOff();
  }

  if (estado == SAIDA) {
    // Estado de saída: usuário acabou de armar e tem 10s para sair
    digitalWrite(LED_DESARMADO, LOW);
    // LED vermelho vai piscar durante a saída
    inicioSaidaMs = millis();
    ultimoPiscaMs = millis();
    pisca = true;
    digitalWrite(LED_ARMADO, HIGH);
    buzzerOff();
  }

  if (estado == ARMADO) {
    // Sistema armado: LED vermelho fixo
    digitalWrite(LED_DESARMADO, LOW);
    digitalWrite(LED_ARMADO, HIGH); // vermelho fixo
    buzzerOff();
  }

  if (estado == ALARME) {
    // Estado de alarme: mantém vermelho aceso
    digitalWrite(LED_DESARMADO, LOW);
    digitalWrite(LED_ARMADO, HIGH); // mantém vermelho
    // sirene começa pelo atualizarSirene()
  }
}

void resetSequencia() {
  // Zera a sequência digitada
  idx = 0;
  ultimoBotaoMs = 0;
}

void registrarBotao(int b) {
  unsigned long agora = millis();

  // timeout entre teclas
  // Se demorou demais entre uma tecla e outra, descarta a sequência anterior
  if (ultimoBotaoMs != 0 && (agora - ultimoBotaoMs) > TIMEOUT_TECLAS_MS) {
    resetSequencia();
  }

  // Registra o botão pressionado
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
    // Após verificar, limpa a sequência
    resetSequencia();
  }
}

void lerBotoes() {
  // Leitura dos botões com debounce simples via delay
  if (digitalRead(B1) == LOW) { registrarBotao(1); delay(200); }
  if (digitalRead(B2) == LOW) { registrarBotao(2); delay(200); }
  if (digitalRead(B3) == LOW) { registrarBotao(3); delay(200); }
}

void setup() {
  // Configuração dos pinos
  pinMode(PIR, INPUT);

  pinMode(LED_ARMADO, OUTPUT);
  pinMode(LED_DESARMADO, OUTPUT);

  pinMode(BUZZER, OUTPUT);

  pinMode(B1, INPUT_PULLUP);
  pinMode(B2, INPUT_PULLUP);
  pinMode(B3, INPUT_PULLUP);

  // Inicializa a serial
  Serial.begin(115200);

  // Inicia o sistema desarmado
  setEstado(DESARMADO);

  // Dica: PIR pode levar alguns segundos para estabilizar
  delay(2000);
}

void loop() {
  // Sempre lê os botões
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
    // Se detectar movimento, entra no estado de alarme
    if (digitalRead(PIR) == HIGH) {
      setEstado(ALARME);
    }
  }

  // ====== ESTADO ALARME ======
  if (estado == ALARME) {
    // Atualiza o padrão sonoro da sirene continuamente
    atualizarSirene();
    // desarmar é sempre 3-2-1 (já tratado em lerBotoes)
  }
}