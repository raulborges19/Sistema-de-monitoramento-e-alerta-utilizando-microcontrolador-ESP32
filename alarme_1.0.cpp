// Definição dos pinos usados no projeto
#define PIR 33              // Sensor de movimento PIR
#define LED_ARMADO 26       // LED que indica sistema armado
#define LED_DESARMADO 27    // LED que indica sistema desarmado
#define BUZZER 25           // Buzzer (alarme sonoro)

// Botões usados para inserir a sequência de senha
#define B1 14
#define B2 12
#define B3 13

// Variável que guarda se o sistema está armado ou não
bool armado = false;

// Vetor para armazenar a sequência digitada nos botões
int sequencia[3];

// Índice da posição atual da sequência
int indice = 0;

void setup() {

  // Configura o sensor PIR como entrada
  pinMode(PIR, INPUT);

  // Configura os LEDs como saída
  pinMode(LED_ARMADO, OUTPUT);
  pinMode(LED_DESARMADO, OUTPUT);

  // Configura o buzzer como saída
  pinMode(BUZZER, OUTPUT);

  // Configura os botões como entrada com pull-up interno
  // Assim, o botão fica em HIGH normalmente e vai para LOW quando pressionado
  pinMode(B1, INPUT_PULLUP);
  pinMode(B2, INPUT_PULLUP);
  pinMode(B3, INPUT_PULLUP);

  // Ao iniciar, o sistema começa desarmado
  // Portanto, o LED de desarmado acende
  digitalWrite(LED_DESARMADO, HIGH);

  // Inicia a comunicação serial para mensagens de depuração
  Serial.begin(115200);
}

void loop() {

  // Lê os botões pressionados e registra a sequência
  lerBotoes();

  // Verifica se a sequência digitada corresponde à senha de armar ou desarmar
  verificarSenha();

  // Só monitora movimento se o sistema estiver armado
  if(armado){

    // Lê o estado do sensor PIR
    int movimento = digitalRead(PIR);

    // Se detectar movimento, liga o buzzer
    if(movimento == HIGH){
      digitalWrite(BUZZER, HIGH);
    }
    else{
      // Se não houver movimento, desliga o buzzer
      digitalWrite(BUZZER, LOW);
    }

  }
}

void lerBotoes(){

  // Se o botão B1 for pressionado, registra o valor 1 na sequência
  if(digitalRead(B1) == LOW){
    sequencia[indice++] = 1;
    delay(300); // Pequeno atraso para evitar múltiplas leituras do mesmo clique
  }

  // Se o botão B2 for pressionado, registra o valor 2 na sequência
  if(digitalRead(B2) == LOW){
    sequencia[indice++] = 2;
    delay(300); // Debounce simples
  }

  // Se o botão B3 for pressionado, registra o valor 3 na sequência
  if(digitalRead(B3) == LOW){
    sequencia[indice++] = 3;
    delay(300); // Debounce simples
  }

}

void verificarSenha(){

  // Só verifica a senha quando 3 botões já tiverem sido pressionados
  if(indice == 3){

    // ARMAR
    // Se a sequência digitada for 1, 2, 3:
    if(sequencia[0]==1 && sequencia[1]==2 && sequencia[2]==3){

      // O sistema passa a ficar armado
      armado = true;

      // Acende o LED de armado e apaga o de desarmado
      digitalWrite(LED_ARMADO, HIGH);
      digitalWrite(LED_DESARMADO, LOW);

      // Mostra mensagem no monitor serial
      Serial.println("Sistema Armado");

    }

    // DESARMAR
    // Se a sequência digitada for 3, 2, 1:
    if(sequencia[0]==3 && sequencia[1]==2 && sequencia[2]==1){

      // O sistema passa a ficar desarmado
      armado = false;

      // Atualiza os LEDs de status
      digitalWrite(LED_ARMADO, LOW);
      digitalWrite(LED_DESARMADO, HIGH);

      // Garante que o buzzer seja desligado ao desarmar
      digitalWrite(BUZZER, LOW);

      // Mostra mensagem no monitor serial
      Serial.println("Sistema Desarmado");

    }

    // Reinicia o índice para começar uma nova sequência
    indice = 0;
  }

}