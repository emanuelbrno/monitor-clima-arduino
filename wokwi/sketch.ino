/*
 * MONITOR DE TEMPERATURA E UMIDADE COM ALARME - Projeto de IoT
 * Placa ...: Arduino UNO
 * Sensor ..: Modulo DHT11 de 3 pinos, marcado "- + S" (esquerda p/ direita)
 *              -  -> GND
 *              +  -> 5V
 *              S  -> D2
 *            O modulo ja tem o resistor de pull-up: NAO use o 10k externo.
 * Alerta ..: LED vermelho (D8 -> LED -> 220 ohm -> GND)
 * Alarme ..: Buzzer (+ -> D7, - -> GND)
 *
 * Biblioteca: "DHT sensor library" (Adafruit) + "Adafruit Unified Sensor"
 */

#include <DHT.h>

// ---------------- CONFIGURACOES ----------------
#define PINO_DHT     2        // pino S do modulo
#define TIPO_DHT     DHT22    // Wokwi so tem DHT22; no projeto real use DHT11
#define PINO_LED     8        // LED vermelho de alerta
#define PINO_BUZZER  7        // buzzer do alarme

const float LIMITE_ALTO  = 22.0;  // liga o alarme a partir deste valor (graus C)
const float LIMITE_BAIXO = 21.0;  // desliga o alarme ate este valor (graus C)

const unsigned long INTERVALO_LEITURA = 2000; // ms entre leituras do sensor
const unsigned long INTERVALO_BIP     = 1000; // ms entre bipes no alarme
const unsigned int  FREQUENCIA_BIP    = 2000; // Hz (som do buzzer)
const unsigned int  DURACAO_BIP       = 200;  // ms de cada bipe
// ------------------------------------------------

DHT dht(PINO_DHT, TIPO_DHT);

bool alertaAtivo = false;
unsigned long ultimaLeitura = 0;
unsigned long ultimoBip = 0;
unsigned int falhasSeguidas = 0;

void setup() {
  Serial.begin(9600);
  pinMode(PINO_LED, OUTPUT);
  pinMode(PINO_BUZZER, OUTPUT);

  dht.begin();

  // Teste de inicializacao: LED acende e buzzer da dois bipes curtos
  digitalWrite(PINO_LED, HIGH);
  tone(PINO_BUZZER, FREQUENCIA_BIP, 100);
  delay(200);
  tone(PINO_BUZZER, FREQUENCIA_BIP, 100);
  delay(300);
  digitalWrite(PINO_LED, LOW);

  // O DHT11 precisa de ~1 s depois de ligado antes da primeira leitura
  delay(1000);

  Serial.println(F("Monitor de Clima iniciado"));
  Serial.print(F("Alarme liga em "));
  Serial.print(LIMITE_ALTO, 1);
  Serial.print(F(" C e desliga em "));
  Serial.print(LIMITE_BAIXO, 1);
  Serial.println(F(" C"));
}

void loop() {
  lerSensor();
  tocarAlarme();
}

// Le o sensor a cada INTERVALO_LEITURA sem travar o programa
void lerSensor() {
  if (millis() - ultimaLeitura < INTERVALO_LEITURA) return;
  ultimaLeitura = millis();

  float umidade     = dht.readHumidity();
  float temperatura = dht.readTemperature(); // em graus Celsius

  if (isnan(umidade) || isnan(temperatura)) {
    falhasSeguidas++;
    diagnosticarSensor();
    return;
  }

  if (falhasSeguidas > 0) {
    Serial.println(F("Sensor OK de novo."));
    falhasSeguidas = 0;
  }

  // Logica do alarme com histerese
  if (!alertaAtivo && temperatura >= LIMITE_ALTO) {
    alertaAtivo = true;
    ultimoBip = 0;              // apita imediatamente ao entrar em alerta
  } else if (alertaAtivo && temperatura <= LIMITE_BAIXO) {
    alertaAtivo = false;
    noTone(PINO_BUZZER);
  }
  digitalWrite(PINO_LED, alertaAtivo ? HIGH : LOW);

  // Formato "Nome:valor" -> legivel no Monitor Serial e
  // desenhado como grafico no Plotter Serial
  Serial.print(F("Temperatura:"));
  Serial.print(temperatura, 1);
  Serial.print(F(" Umidade:"));
  Serial.print(umidade, 0);
  Serial.print(F(" Limite:"));
  Serial.print(LIMITE_ALTO, 1);
  Serial.print(F(" Alarme:"));
  Serial.println(alertaAtivo ? 1 : 0);
}

// Quando a leitura falha, olha o nivel do fio de dados em repouso
// para dizer o que provavelmente esta errado na montagem.
void diagnosticarSensor() {
  // Sem o pull-up interno do Arduino: so o resistor do modulo
  // (alimentado) consegue levar a linha para 5 V.
  pinMode(PINO_DHT, INPUT);
  delay(2);
  bool linhaEmNivelAlto = digitalRead(PINO_DHT) == HIGH;

  Serial.print(F("ERRO "));
  Serial.print(falhasSeguidas);
  Serial.print(F(": o DHT11 nao respondeu. "));
  if (!linhaEmNivelAlto) {
    Serial.println(F("D2 em 0 V. Provavel: modulo SEM ENERGIA "
                     "(pino + fora do 5V ou pino - fora do GND) "
                     "ou fio do D2 fora da coluna do pino S."));
  } else {
    Serial.println(F("D2 em 5 V, mas o modulo nao responde. "
                     "Confira a ordem - + S e se o fio do D2 esta no pino S."));
  }
}

// Enquanto o alerta estiver ativo, da um bipe a cada INTERVALO_BIP
void tocarAlarme() {
  if (!alertaAtivo) return;
  if (ultimoBip != 0 && millis() - ultimoBip < INTERVALO_BIP) return;
  ultimoBip = millis();
  if (ultimoBip == 0) ultimoBip = 1;
  tone(PINO_BUZZER, FREQUENCIA_BIP, DURACAO_BIP);
}
