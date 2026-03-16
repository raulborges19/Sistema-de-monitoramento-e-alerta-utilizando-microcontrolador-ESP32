
#define PIR 33
#define LED_ARMADO 26
#define LED_DESARMADO 27
#define BUZZER 25

#define B1 14
#define B2 12
#define B3 13

bool armado = false;

int sequencia[3];
int indice = 0;

void setup() {

  pinMode(PIR, INPUT);

  pinMode(LED_ARMADO, OUTPUT);
  pinMode(LED_DESARMADO, OUTPUT);

  pinMode(BUZZER, OUTPUT);

  pinMode(B1, INPUT_PULLUP);
  pinMode(B2, INPUT_PULLUP);
  pinMode(B3, INPUT_PULLUP);

  digitalWrite(LED_DESARMADO, HIGH);

  Serial.begin(115200);
}

void loop() {

  lerBotoes();
  verificarSenha();

  if(armado){

    int movimento = digitalRead(PIR);

    if(movimento == HIGH){
      digitalWrite(BUZZER, HIGH);
    }
    else{
      digitalWrite(BUZZER, LOW);
    }

  }
}

void lerBotoes(){

  if(digitalRead(B1) == LOW){
    sequencia[indice++] = 1;
    delay(300);
  }

  if(digitalRead(B2) == LOW){
    sequencia[indice++] = 2;
    delay(300);
  }

  if(digitalRead(B3) == LOW){
    sequencia[indice++] = 3;
    delay(300);
  }

}

void verificarSenha(){

  if(indice == 3){

    // ARMAR
    if(sequencia[0]==1 && sequencia[1]==2 && sequencia[2]==3){

      armado = true;

      digitalWrite(LED_ARMADO, HIGH);
      digitalWrite(LED_DESARMADO, LOW);

      Serial.println("Sistema Armado");

    }

    // DESARMAR
    if(sequencia[0]==3 && sequencia[1]==2 && sequencia[2]==1){

      armado = false;

      digitalWrite(LED_ARMADO, LOW);
      digitalWrite(LED_DESARMADO, HIGH);

      digitalWrite(BUZZER, LOW);

      Serial.println("Sistema Desarmado");

    }

    indice = 0;
  }

}