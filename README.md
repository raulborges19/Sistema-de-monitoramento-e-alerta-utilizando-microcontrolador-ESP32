# Sistema-de-monitoramento-e-alerta-utilizando-microcontrolador-ESP32
🔔 Sistema de Monitoramento e Alerta com ESP32
📌 Descrição

Este projeto implementa um sistema de monitoramento e alerta utilizando o microcontrolador ESP32. O sistema detecta movimentação através de um sensor conectado ao microcontrolador e executa ações automáticas quando um evento é identificado.

Ao detectar movimento, o ESP32 pode:

Acionar um alarme sonoro

Enviar uma notificação via Telegram

Registrar ou processar o evento conforme a lógica do código

A comunicação com o usuário ocorre através da conexão Wi-Fi do ESP32, permitindo o envio de alertas em tempo real.

⚙️ Funcionamento do Sistema

O sensor monitora continuamente o ambiente.

Ao detectar movimentação, envia um sinal digital para o ESP32.

O ESP32 interpreta o evento conforme a lógica do código.

O sistema executa as ações programadas:

acionamento do alarme

envio de mensagem via Telegram

🧩 Estrutura Física (Montagem)

O circuito foi montado em protoboard utilizando os seguintes componentes:

Componentes

ESP32

Sensor de movimento

Buzzer (alarme)

Protoboard

Jumpers

Fonte de alimentação USB

Conexões principais
Componente	Conexão ESP32
Sensor (VCC)	3.3V
Sensor (GND)	GND
Sensor (OUT)	GPIO
Buzzer (+)	GPIO
Buzzer (-)	GND

Os pinos GPIO podem variar dependendo da versão do código.

📡 Diagrama de Comunicação

Sensor → ESP32 → Alarme
              ↓
           Wi-Fi
              ↓
       Notificação Telegram
💻 Estrutura dos Códigos

O projeto possui diferentes versões do código que representam evoluções do sistema.

/ALARME_beta_1.0
/ALARME_beta_1.1
/ALARME_beta_2.0
/ALARME_beta_2.0.1
/ALARME_beta_2.1
Versões

ALARME_beta_1.0

Primeira implementação do sistema

Leitura básica do sensor

Acionamento do alarme local

ALARME_beta_1.1

Melhor organização do código

Ajustes na lógica de detecção

ALARME_beta_2.0

Integração com Wi-Fi

Preparação para envio de notificações

ALARME_beta_2.0.1

Melhorias no envio de notificações

ALARME_beta2.1

Versão mais atual

Envio de alertas via Telegram

Melhor tratamento de eventos e notificações

🚀 Como Reproduzir o Projeto

Montar o circuito na protoboard conforme descrito.

Abrir o código na Arduino IDE.

Configurar no código:

SSID da rede Wi-Fi

senha da rede

token do bot do Telegram

Fazer upload do código para o ESP32.

Testar o sistema gerando movimentação no sensor.

📚 Tecnologias Utilizadas

ESP32

Arduino IDE

Wi-Fi

Telegram Bot API

C/C++
