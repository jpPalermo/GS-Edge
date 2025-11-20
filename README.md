# WorkSafe IoT – Saúde e Bem-estar no Trabalho 🧠💡

## 1. Visão Geral

O **WorkSafe IoT** é um sistema de monitoramento voltado para **saúde e bem-estar no trabalho**, usando **ESP32 + sensores IoT + MQTT + Node-RED**.

Ele monitora:
- **Ambiente** (temperatura, umidade, luminosidade)
- **Ergonomia** (postura aproximada via distância)
- **Fadiga** (tempo contínuo em atividade + condições ambientais)

E gera **alertas inteligentes** e **pausas recomendadas** para tornar o ambiente de trabalho mais saudável, prevenir dores e melhorar a produtividade.

Tema do projeto:  
**“Saúde e bem-estar no trabalho: monitoramento ambiental, pausas inteligentes, controle de fadiga e ergonomia.”**

---

## 2. Problema Abordado

Profissionais passam muitas horas:

- Sentados em frente ao computador  
- Com postura inadequada  
- Em ambientes quentes, com pouca luz ou desconfortáveis  
- Com poucas pausas durante o expediente  

Isso aumenta:
- Dores lombares e problemas de coluna  
- Fadiga física e mental  
- Queda de produtividade  
- Lesões por esforço repetitivo (LER/DORT)

Falta uma solução **simples, acessível e automatizada** que acompanhe o trabalhador em tempo real, identifique condições ruins e **sugira pausas/ajustes antes que o problema piore**.

---

## 3. Objetivos da Solução

O **WorkSafe IoT** tem como objetivos:

- Monitorar **temperatura**, **umidade** e **luminosidade** do ambiente  
- Monitorar um indicador de **postura/ergonomia** usando distância aproximada ao monitor  
- Calcular um **“índice de fadiga”** baseado nas leituras  
- Recomendar **pausas inteligentes**  
- Enviar dados para um **dashboard em Node-RED** via **MQTT**

---

## 4. Arquitetura da Solução (IoT)

Componentes:

- **ESP32**  
- **DHT22**  
- **LDR + resistor 10kΩ**  
- **HC-SR04**  
- **LED (GPIO 2)**  
- **MQTT Broker (Mosquitto/Node-RED)**  
- **Dashboard Node-RED**

Fluxo:

ESP32 → MQTT → Node-RED → Dashboard

---

## 5. Conexões dos Sensores

**DHT22**  
- VCC → 3.3V  
- DATA → GPIO 15  
- GND → GND  

**LDR + 10kΩ**  
- LDR → 3.3V  
- Nó LDR+resistor → GPIO 34  
- Resistor 10k → GND  

**HC-SR04**  
- VCC → 5V  
- TRIG → GPIO 5  
- ECHO → GPIO 18  
- GND → GND  

**LED**  
- GPIO 2 → LED  
- LED → resistor → GND  

---

## 6. Estrutura do Repositório

```
WorkSafe-IoT/
├── README.md
├── esp32/
│   └── worksafe_iot.ino
├── node-red/
│   └── worksafe_flow.json
├── docs/
│   ├── arquitetura-iot.png
│   ├── diagrama-wokwi.png
│   └── dashboard-node-red.png
└── mqtt/
    └── topicos-e-mensagens.md
```

---

## 7. Como Replicar

### Wokwi
- Abrir o link da simulação  
- Carregar o código .ino  
- Conectar os sensores conforme indicado  

### Node-RED

```bash
node-red
```

Acessar: `http://localhost:1880`

Importar `node-red/worksafe_flow.json`.

---

## 8. MQTT – Tópicos Utilizados

- `worksafe/status`  
- `worksafe/temperatura`  
- `worksafe/umidade`  
- `worksafe/luminosidade`  
- `worksafe/postura`  
- `worksafe/alertas`  

Exemplo payload:

```json
{
  "temp": 25.3,
  "hum": 48.7,
  "ldr": 2100,
  "dist_cm": 40.5,
  "tempo_sentado_min": 32.5,
  "postura_alerta": false,
  "pausa_alerta": false,
  "fatigue_score": 35
}
```

---

## 9. Vídeo Explicativo

O vídeo deve mostrar:
- O problema  
- A solução WorkSafe IoT  
- Simulação no Wokwi  
- Dashboard no Node-RED  
- Alertas acontecendo  

Link do vídeo: `<adicione aqui>`

---

## 10. Integrantes

- **João Pedro Palermo – RM 562077**

---

Desenvolvido para a Global Solution – FIAP.

