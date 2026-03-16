🔔 Sistema de Monitoramento e Alerta com ESP32

Este projeto implementa um sistema de monitoramento e alerta utilizando o microcontrolador ESP32. O sistema é capaz de detectar movimentação no ambiente e executar ações automáticas como acionar um alarme sonoro e enviar notificações em tempo real via Telegram.

A comunicação com o usuário ocorre através da conexão Wi-Fi do ESP32, permitindo monitoramento remoto do sistema.

📌 Descrição do Projeto

O sistema utiliza um sensor de movimento conectado ao ESP32 para monitorar continuamente o ambiente.

Quando um evento é detectado, o microcontrolador processa o sinal recebido e executa as ações programadas, que podem incluir:

🔊 Acionamento de um buzzer (alarme sonoro)

📩 Envio de notificação via Telegram

💡 Indicação visual através de LEDs

📡 Comunicação remota via Wi-Fi

Esse projeto foi desenvolvido de forma evolutiva, com diferentes versões de código que adicionam funcionalidades progressivamente.

⚙️ Funcionamento do Sistema

O sensor de movimento monitora continuamente o ambiente.

Ao detectar movimentação, envia um sinal digital para o ESP32.

O ESP32 interpreta o evento conforme a lógica implementada no código.

O sistema executa automaticamente as ações programadas:

Acionamento do alarme sonoro

Indicação visual por LEDs

Envio de alerta via Telegram

🧩 Estrutura Física (Montagem)

O circuito foi montado em protoboard utilizando os seguintes componentes.

Componentes Utilizados

ESP32

Sensor de movimento (PIR)

Buzzer (alarme sonoro)

LEDs (vermelho e verde)

Botões de controle

Resistores 220Ω

Protoboard

Jumpers

Fonte de alimentação USB

🔌 Conexões com o ESP32
Componente	Conexão ESP32
Sensor VCC	3.3V
Sensor GND	GND
Sensor OUT	GPIO 33
Buzzer (+)	GPIO 25
Buzzer (-)	GND
LED Vermelho (+)	GPIO 26
LED Vermelho (-)	Resistor 220Ω → GND
LED Verde (+)	GPIO 27
LED Verde (-)	Resistor 220Ω → GND
Botão 1 (+)	GPIO 14
Botão 1 (-)	GND
Botão 2 (+)	GPIO 12
Botão 2 (-)	GND
Botão 3 (+)	GPIO 13
Botão 3 (-)	GND

📌 Observação:
Os pinos GPIO permanecem os mesmos em todas as versões do código.

📡 Diagrama de Comunicação
Sensor → ESP32 → Alarme
              ↓
           Wi-Fi
              ↓
       Notificação Telegram
💻 Estrutura dos Códigos

O projeto possui diferentes versões do código representando a evolução do sistema:

/ALARME_1.0
/ALARME_1.1
/ALARME_2.0
/ALARME_2.0.1
/ALARME_2.1
🔄 Evolução das Versões
🔹 Versão 1.0

Comandos físicos de armar e desarmar

Sinal sonoro

Sinal visual

🔹 Versão 1.1

Comandos físicos de armar e desarmar

Sinal sonoro

Sinal visual

Correção de bugs

🔹 Versão 2.0

Envio de mensagem em caso de disparo

Comandos físicos de armar e desarmar

Sinal sonoro

Sinal visual

🔹 Versão 2.0.1

Mensagem de disparo

Mensagem de restabelecimento

Comandos físicos de armar e desarmar

Sinal sonoro

Sinal visual

Correção de bugs

🔹 Versão 2.1 (Atual)

Comandos remotos via Telegram

Comandos físicos de armar e desarmar

Sinal sonoro

Sinal visual

Mensagem de disparo

Mensagem de restabelecimento

Mensagem de status

Comando /help

Comando /status

🚀 Como Reproduzir o Projeto

Monte o circuito na protoboard conforme as conexões descritas.

Abra o código na Arduino IDE.

Configure no código:

SSID da rede Wi-Fi

Senha da rede

Token do bot do Telegram

ID do chat do Telegram

Faça o upload do código para o ESP32.

Teste o sistema gerando movimentação no sensor.

🛠 Tecnologias Utilizadas

ESP32

Arduino IDE

Wi-Fi

Telegram Bot API

C++

💡 Este projeto demonstra como integrar IoT, sensores e comunicação remota, permitindo criar um sistema simples de monitoramento e alerta em tempo real utilizando o ESP32.
