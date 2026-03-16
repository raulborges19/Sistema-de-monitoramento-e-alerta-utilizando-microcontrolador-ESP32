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
Sensor (OUT)	GPIO 33
Buzzer (+)	GPIO 25
Buzzer (-)	GND
LED Vermelho (+) GPIO 26
LED Vermelho (-) resistor 220Ω GND 
LED Verde (+) GPIO 27
LED Verde (-) resistor 220Ω GND
Botão 1 (+) GPIO 14
Botão 1 (-) GND
Botão 2 (+) GPIO 12 
Botão 2 (-) GND
Botão 3 (+) GPIO 13
Botão 3 (-) GND

Os pinos GPIO são os mesmos em todas as versões dos códigos apresentados.

📡 Diagrama de Comunicação

Sensor → ESP32 → Alarme
              ↓
           Wi-Fi
              ↓
       Notificação Telegram
💻 Estrutura dos Códigos

O projeto possui diferentes versões do código que representam evoluções do sistema.

/ALARME_1.0
/ALARME_1.1
/ALARME_2.0
/ALARME_2.0.1
/ALARME_2.1

Funções de cada versão


1.0
-comandos físicos de arme e desarme
-sinal sonoro
-sinal visual

1.1
-comandos físicos de arme e desarme
-sinal sonoro
-sinal visual
-melhorias de bugs

2.0 
-mensagem em caso de disparo
-comandos físicos de arme e desarme
-sinal sonoro
-sinal visual

2.0.1
-envia mensagem de disparo
-envia mensagem de reestabelecimento
-comandos físicos de arme e desarme
-sinal sonoro
-sinal visual
-melhorias de bugs

2.1
-comandos remotos
-comandos físicos de arme e desarme
-sinal sonoro
-sinal visual
-envia mensagem de disparos
-envia mensagem de reestabelecimento
-envia mensagem de status (quando armado e desarmado)
-comando de ajuda (/help)
-comando de status do alarme (/status)

🚀 Como Reproduzir o Projeto

Montar o circuito na protoboard conforme descrito.

Abrir o código na Arduino IDE.

Configurar no código:

SSID da rede Wi-Fi

senha da rede

token do bot do Telegram

id do seu chat do Telegram

Fazer upload do código para o ESP32.

Testar o sistema gerando movimentação no sensor.

📚 Tecnologias Utilizadas

ESP32

Arduino IDE

Wi-Fi

Telegram Bot API

C++
