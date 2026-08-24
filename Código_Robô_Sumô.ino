/*
   ============================================================
                ROBÔ DE SUMÔ - ARDUINO MEGA 2560
   ============================================================

   HARDWARE:
   - Arduino Mega 2560

   - 2 sensores ultrassônicos HC-SR04
       Frente
       Trás

   - 3 sensores de cor/refletância
       Frente esquerda
       Frente direita
       Trás

   - Arena BRANCA
   - Borda PRETA

   - 2 drivers BTS7960
       1 motor esquerdo
       1 motor direito

   - Rampa frontal

   ============================================================
                      PRIORIDADE DO ROBÔ
   ============================================================

   1 - Detectar borda preta
   2 - Escapar da borda
   3 - Atacar adversário à frente
   4 - Se adversário estiver atrás, virar
   5 - Procurar adversário

   ============================================================
*/

// ============================================================
//                    PINOS DOS SENSORES DE COR
// ============================================================

const byte PIN_COR_FRENTE_ESQ = A0;
const byte PIN_COR_FRENTE_DIR = A1;
const byte PIN_COR_TRAS = A2;

// ============================================================
//                       ULTRASSÔNICO FRENTE
// ============================================================

const byte TRIG_FRENTE = 30;
const byte ECHO_FRENTE = 31;

// ============================================================
//                       ULTRASSÔNICO TRASEIRO
// ============================================================

const byte TRIG_TRAS = 32;
const byte ECHO_TRAS = 33;

// ============================================================
//                    BTS7960 - MOTOR ESQUERDO
// ============================================================

const byte MOTOR_E_RPWM = 5;
const byte MOTOR_E_LPWM = 6;

const byte MOTOR_E_REN = 22;
const byte MOTOR_E_LEN = 23;

// ============================================================
//                    BTS7960 - MOTOR DIREITO
// ============================================================

const byte MOTOR_D_RPWM = 9;
const byte MOTOR_D_LPWM = 10;

const byte MOTOR_D_REN = 24;
const byte MOTOR_D_LEN = 25;

// ============================================================
//                INVERSÃO DOS MOTORES
// ============================================================
//
// Se algum motor girar ao contrário, NÃO precisa trocar fios.
//
// Basta mudar:
// false -> true
//
// ============================================================

const bool INVERTER_MOTOR_ESQUERDO = false;
const bool INVERTER_MOTOR_DIREITO = false;

// ============================================================
//                  CALIBRAÇÃO DOS SENSORES
// ============================================================

const bool PRETO_QUANDO_VALOR_MENOR = true;

// ============================================================
//                       LIMIARES
// ============================================================
//
// VALORES INICIAIS.
// DEPOIS VAMOS CALIBRAR COM OS SENSORES REAIS.
//
// ============================================================

int LIMIAR_ESQUERDO = 500;
int LIMIAR_DIREITO = 500;
int LIMIAR_TRASEIRO = 500;

// ============================================================
//                   DISTÂNCIAS DO ULTRASSÔNICO
// ============================================================

const int DISTANCIA_DETECCAO_CM = 70;

const int DISTANCIA_ATAQUE_FORTE_CM = 35;

const unsigned long TIMEOUT_ULTRASSOM_US = 6000;

// ============================================================
//                       VELOCIDADES
// ============================================================
//
// Como agora a busca/giro será feita com UMA RODA PARADA,
// usamos velocidade máxima para deixar a rotação mais rápida.
//
// ============================================================

const int VELOCIDADE_BUSCA = 255;

const int VELOCIDADE_GIRO = 255;

const int VELOCIDADE_ATAQUE = 255;

const int VELOCIDADE_RE = 255;

const int VELOCIDADE_FUGA = 255;

// ============================================================
//                   TEMPOS DE MANOBRA
// ============================================================

const unsigned long TEMPO_RE_BORDA_MS = 230;

const unsigned long TEMPO_GIRO_BORDA_MS = 260;

const unsigned long TEMPO_FRENTE_BORDA_TRASEIRA_MS = 260;

const unsigned long TEMPO_MAX_GIRO_TRASEIRO_MS = 1000;

const unsigned long MEMORIA_ALVO_MS = 180;

// ============================================================
//                 LEITURA DOS ULTRASSÔNICOS
// ============================================================

float distanciaFrente = 999;
float distanciaTras = 999;

// Evita os dois HC-SR04 dispararem juntos

unsigned long ultimoPing = 0;

const unsigned long INTERVALO_PING_MS = 35;

bool proximoSensorFrente = true;

// ============================================================
//                      ESTADOS DO ROBÔ
// ============================================================

enum EstadoRobo { PROCURANDO, ATACANDO, GIRANDO_TRASEIRO, ESCAPANDO_BORDA };

EstadoRobo estado = PROCURANDO;

// ============================================================
//                    ESTADOS DA FUGA
// ============================================================

enum TipoFuga {
  FUGA_NENHUMA,

  FUGA_FRENTE_ESQ,
  FUGA_FRENTE_DIR,
  FUGA_FRENTE_AMBOS,

  FUGA_TRASEIRA
};

TipoFuga tipoFuga = FUGA_NENHUMA;

// ============================================================
//                       ETAPAS DA FUGA
// ============================================================

enum EtapaFuga { ETAPA_RECUAR, ETAPA_GIRAR, ETAPA_AVANCAR };

EtapaFuga etapaFuga = ETAPA_RECUAR;

unsigned long inicioEtapaFuga = 0;

// ============================================================
//                MEMÓRIA DO ADVERSÁRIO
// ============================================================

unsigned long ultimoAlvoFrente = 0;

// ============================================================
//                    SENTIDO DA BUSCA
// ============================================================
//
// 1  = direita
// -1 = esquerda
//
// ============================================================

int sentidoBusca = 1;

// ============================================================
//                     FUNÇÃO MOTOR BTS7960
// ============================================================
//
// velocidade:
// -255 = ré máxima
//  0   = parado
// +255 = frente máxima
//
// ============================================================

void controlarMotor(int velocidade, byte RPWM, byte LPWM, bool inverter) {
  velocidade = constrain(velocidade, -255, 255);

  if (inverter) {
    velocidade = -velocidade;
  }

  // MOTOR PARADO

  if (velocidade == 0) {
    analogWrite(RPWM, 0);
    analogWrite(LPWM, 0);

    return;
  }

  // FRENTE

  if (velocidade > 0) {
    analogWrite(RPWM, velocidade);
    analogWrite(LPWM, 0);
  }

  // RÉ

  else {
    analogWrite(RPWM, 0);
    analogWrite(LPWM, -velocidade);
  }
}

// ============================================================
//                     MOTOR ESQUERDO
// ============================================================

void motorEsquerdo(int velocidade) {
  controlarMotor(velocidade, MOTOR_E_RPWM, MOTOR_E_LPWM,
                 INVERTER_MOTOR_ESQUERDO);
}

// ============================================================
//                      MOTOR DIREITO
// ============================================================

void motorDireito(int velocidade) {
  controlarMotor(velocidade, MOTOR_D_RPWM, MOTOR_D_LPWM,
                 INVERTER_MOTOR_DIREITO);
}

// ============================================================
//                    CONTROLAR DOIS MOTORES
// ============================================================

void mover(int esquerdo, int direito) {
  motorEsquerdo(esquerdo);
  motorDireito(direito);
}

// ============================================================
//                         PARAR
// ============================================================

void parar() { mover(0, 0); }

// ============================================================
//                       ANDAR PARA FRENTE
// ============================================================

void frente(int velocidade) { mover(velocidade, velocidade); }

// ============================================================
//                             RÉ
// ============================================================

void re(int velocidade) { mover(-velocidade, -velocidade); }

// ============================================================
//                   GIRO PARA A DIREITA
// ============================================================
//
// NOVA ESTRATÉGIA:
//
// Motor esquerdo = ligado
// Motor direito  = desligado
//
// Isso faz o robô girar apoiado na roda direita.
//
// ============================================================

void girarDireita(int velocidade) {
  motorEsquerdo(velocidade);
  motorDireito(0);
}

// ============================================================
//                   GIRO PARA A ESQUERDA
// ============================================================
//
// Motor esquerdo = desligado
// Motor direito  = ligado
//
// Isso faz o robô girar apoiado na roda esquerda.
//
// ============================================================

void girarEsquerda(int velocidade) {
  motorEsquerdo(0);
  motorDireito(velocidade);
}

// ============================================================
//                  VERIFICAR SE É PRETO
// ============================================================

bool detectarPreto(int valor, int limiar) {
  if (PRETO_QUANDO_VALOR_MENOR) {
    return valor < limiar;
  } else {
    return valor > limiar;
  }
}

// ============================================================
//                   LEITURA DOS SENSORES DE COR
// ============================================================

int valorCorEsquerdo = 0;
int valorCorDireito = 0;
int valorCorTraseiro = 0;

bool bordaEsquerda = false;
bool bordaDireita = false;
bool bordaTraseira = false;

void lerSensoresBorda() {
  valorCorEsquerdo = analogRead(PIN_COR_FRENTE_ESQ);

  valorCorDireito = analogRead(PIN_COR_FRENTE_DIR);

  valorCorTraseiro = analogRead(PIN_COR_TRAS);

  bordaEsquerda = detectarPreto(valorCorEsquerdo, LIMIAR_ESQUERDO);

  bordaDireita = detectarPreto(valorCorDireito, LIMIAR_DIREITO);

  bordaTraseira = detectarPreto(valorCorTraseiro, LIMIAR_TRASEIRO);
}

// ============================================================
//                 LEITURA DO HC-SR04
// ============================================================

float lerUltrassonico(byte trig, byte echo) {
  digitalWrite(trig, LOW);

  delayMicroseconds(2);

  digitalWrite(trig, HIGH);

  delayMicroseconds(10);

  digitalWrite(trig, LOW);

  unsigned long duracao = pulseIn(echo, HIGH, TIMEOUT_ULTRASSOM_US);

  // Nada encontrado

  if (duracao == 0) {
    return 999;
  }

  float distancia = duracao * 0.0343 / 2.0;

  return distancia;
}

// ============================================================
//             ATUALIZAR ULTRASSÔNICOS ALTERNADAMENTE
// ============================================================

void atualizarUltrassonicos() {
  unsigned long agora = millis();

  if (agora - ultimoPing < INTERVALO_PING_MS) {
    return;
  }

  ultimoPing = agora;

  if (proximoSensorFrente) {
    distanciaFrente = lerUltrassonico(TRIG_FRENTE, ECHO_FRENTE);
  } else {
    distanciaTras = lerUltrassonico(TRIG_TRAS, ECHO_TRAS);
  }

  proximoSensorFrente = !proximoSensorFrente;
}

// ============================================================
//                  ADVERSÁRIO NA FRENTE?
// ============================================================

bool adversarioFrente() {
  return (distanciaFrente > 2 && distanciaFrente <= DISTANCIA_DETECCAO_CM);
}

// ============================================================
//                   ADVERSÁRIO ATRÁS?
// ============================================================

bool adversarioTras() {
  return (distanciaTras > 2 && distanciaTras <= DISTANCIA_DETECCAO_CM);
}

// ============================================================
//                  INICIAR FUGA DA BORDA
// ============================================================

void iniciarFuga(TipoFuga novaFuga) {
  estado = ESCAPANDO_BORDA;

  tipoFuga = novaFuga;

  inicioEtapaFuga = millis();

  if (novaFuga == FUGA_TRASEIRA) {
    etapaFuga = ETAPA_AVANCAR;
  } else {
    etapaFuga = ETAPA_RECUAR;
  }
}

// ============================================================
//                    DETECTAR SITUAÇÃO DE BORDA
// ============================================================

bool verificarBorda() {
  // ----------------------------------------------------------
  // BORDA TRASEIRA
  // ----------------------------------------------------------

  if (bordaTraseira) {
    iniciarFuga(FUGA_TRASEIRA);

    return true;
  }

  // ----------------------------------------------------------
  // DOIS SENSORES FRONTAIS
  // ----------------------------------------------------------

  if (bordaEsquerda && bordaDireita) {
    iniciarFuga(FUGA_FRENTE_AMBOS);

    return true;
  }

  // ----------------------------------------------------------
  // SOMENTE ESQUERDA
  // ----------------------------------------------------------

  if (bordaEsquerda) {
    iniciarFuga(FUGA_FRENTE_ESQ);

    return true;
  }

  // ----------------------------------------------------------
  // SOMENTE DIREITA
  // ----------------------------------------------------------

  if (bordaDireita) {
    iniciarFuga(FUGA_FRENTE_DIR);

    return true;
  }

  return false;
}

// ============================================================
//                      EXECUTAR FUGA
// ============================================================

void executarFuga() {
  unsigned long agora = millis();

  // ==========================================================
  // BORDA TRASEIRA
  // ==========================================================

  if (tipoFuga == FUGA_TRASEIRA) {
    frente(VELOCIDADE_FUGA);

    if (agora - inicioEtapaFuga >= TEMPO_FRENTE_BORDA_TRASEIRA_MS) {
      estado = PROCURANDO;

      tipoFuga = FUGA_NENHUMA;
    }

    return;
  }

  // ==========================================================
  // PRIMEIRA ETAPA:
  // RÉ
  // ==========================================================

  if (etapaFuga == ETAPA_RECUAR) {
    re(VELOCIDADE_RE);

    if (agora - inicioEtapaFuga >= TEMPO_RE_BORDA_MS) {
      etapaFuga = ETAPA_GIRAR;

      inicioEtapaFuga = agora;
    }

    return;
  }

  // ==========================================================
  // SEGUNDA ETAPA:
  // GIRAR PARA DENTRO DA ARENA
  //
  // AGORA O GIRO USA APENAS UMA RODA.
  // ==========================================================

  if (etapaFuga == ETAPA_GIRAR) {
    // --------------------------------------------------------
    // Borda na esquerda:
    // gira para direita usando somente motor esquerdo
    // --------------------------------------------------------

    if (tipoFuga == FUGA_FRENTE_ESQ) {
      girarDireita(VELOCIDADE_GIRO);
    }

    // --------------------------------------------------------
    // Borda na direita:
    // gira para esquerda usando somente motor direito
    // --------------------------------------------------------

    else if (tipoFuga == FUGA_FRENTE_DIR) {
      girarEsquerda(VELOCIDADE_GIRO);
    }

    // --------------------------------------------------------
    // Os dois sensores encontraram preto.
    // Alternamos o lado da busca.
    // --------------------------------------------------------

    else if (tipoFuga == FUGA_FRENTE_AMBOS) {
      if (sentidoBusca > 0) {
        girarDireita(VELOCIDADE_GIRO);
      } else {
        girarEsquerda(VELOCIDADE_GIRO);
      }

      sentidoBusca = -sentidoBusca;
    }

    if (agora - inicioEtapaFuga >= TEMPO_GIRO_BORDA_MS) {
      estado = PROCURANDO;

      tipoFuga = FUGA_NENHUMA;
    }

    return;
  }
}

// ============================================================
//                      ATAQUE FRONTAL
// ============================================================

void atacar() {
  // Muito perto:
  // potência total.

  if (distanciaFrente <= DISTANCIA_ATAQUE_FORTE_CM) {
    frente(255);
  }

  else {
    // Mantém velocidade alta.

    frente(230);
  }

  ultimoAlvoFrente = millis();
}

// ============================================================
//             INICIAR GIRO PARA ADVERSÁRIO TRASEIRO
// ============================================================

unsigned long inicioGiroTraseiro = 0;

void iniciarGiroTraseiro() {
  estado = GIRANDO_TRASEIRO;

  inicioGiroTraseiro = millis();

  // Alterna o lado da rotação.

  sentidoBusca = -sentidoBusca;
}

// ============================================================
//               GIRAR PARA QUEM ESTÁ ATRÁS
// ============================================================
//
// IMPORTANTE:
//
// Agora o giro é feito com UMA RODA PARADA.
//
// ============================================================

void executarGiroTraseiro() {
  // ----------------------------------------------------------
  // Primeiro verifica se já conseguiu colocar o adversário
  // na frente.
  // ----------------------------------------------------------

  if (adversarioFrente()) {
    estado = ATACANDO;

    ultimoAlvoFrente = millis();

    atacar();

    return;
  }

  // ----------------------------------------------------------
  // GIRO DE BUSCA
  //
  // Uma roda funciona.
  // A outra permanece parada.
  // ----------------------------------------------------------

  if (sentidoBusca > 0) {
    girarDireita(VELOCIDADE_GIRO);
  } else {
    girarEsquerda(VELOCIDADE_GIRO);
  }

  // ----------------------------------------------------------
  // Segurança:
  // se nunca encontrar pela frente,
  // não gira para sempre.
  // ----------------------------------------------------------

  if (millis() - inicioGiroTraseiro > TEMPO_MAX_GIRO_TRASEIRO_MS) {
    estado = PROCURANDO;
  }
}

// ============================================================
//                      PROCURAR ADVERSÁRIO
// ============================================================
//
// ESTA É A PRINCIPAL MUDANÇA.
//
// Antes:
//   uma roda para frente
//   outra para trás
//
// Agora:
//   UMA RODA PARADA
//   OUTRA RODA GIRANDO
//
// Isso faz o robô procurar o adversário fazendo um pivô.
//
// ============================================================

void procurar() {
  if (sentidoBusca > 0) {
    // Motor esquerdo ligado
    // Motor direito desligado

    motorEsquerdo(VELOCIDADE_BUSCA);
    motorDireito(0);
  }

  else {
    // Motor esquerdo desligado
    // Motor direito ligado

    motorEsquerdo(0);
    motorDireito(VELOCIDADE_BUSCA);
  }
}

// ============================================================
//                         SETUP
// ============================================================

void setup() {
  Serial.begin(115200);

  // ==========================================================
  // ULTRASSÔNICOS
  // ==========================================================

  pinMode(TRIG_FRENTE, OUTPUT);
  pinMode(ECHO_FRENTE, INPUT);

  pinMode(TRIG_TRAS, OUTPUT);
  pinMode(ECHO_TRAS, INPUT);

  digitalWrite(TRIG_FRENTE, LOW);
  digitalWrite(TRIG_TRAS, LOW);

  // ==========================================================
  // BTS7960 MOTOR ESQUERDO
  // ==========================================================

  pinMode(MOTOR_E_RPWM, OUTPUT);
  pinMode(MOTOR_E_LPWM, OUTPUT);

  pinMode(MOTOR_E_REN, OUTPUT);
  pinMode(MOTOR_E_LEN, OUTPUT);

  // ==========================================================
  // BTS7960 MOTOR DIREITO
  // ==========================================================

  pinMode(MOTOR_D_RPWM, OUTPUT);
  pinMode(MOTOR_D_LPWM, OUTPUT);

  pinMode(MOTOR_D_REN, OUTPUT);
  pinMode(MOTOR_D_LEN, OUTPUT);

  // ==========================================================
  // HABILITA OS BTS7960
  // ==========================================================

  digitalWrite(MOTOR_E_REN, HIGH);
  digitalWrite(MOTOR_E_LEN, HIGH);

  digitalWrite(MOTOR_D_REN, HIGH);
  digitalWrite(MOTOR_D_LEN, HIGH);

  parar();

  // ==========================================================
  // MENSAGENS INICIAIS
  // ==========================================================

  Serial.println();
  Serial.println("==============================");
  Serial.println(" ROBO DE SUMO - MEGA 2560");
  Serial.println("==============================");
  Serial.println();

  Serial.println("Arena: BRANCA");
  Serial.println("Borda: PRETA");

  Serial.println();

  Serial.println("Sistema iniciado.");
}

// ============================================================
//                          LOOP
// ============================================================

void loop() {
  // ==========================================================
  // 1 - SENSORES DA BORDA
  // ==========================================================

  lerSensoresBorda();

  // ==========================================================
  // 2 - ULTRASSÔNICOS
  // ==========================================================

  atualizarUltrassonicos();

  // ==========================================================
  // 3 - SE ESTAMOS ESCAPANDO,
  // continuamos a manobra.
  // ==========================================================

  if (estado == ESCAPANDO_BORDA) {
    executarFuga();

    return;
  }

  // ==========================================================
  // 4 - BORDA SEMPRE TEM PRIORIDADE
  // ==========================================================

  if (verificarBorda()) {
    executarFuga();

    return;
  }

  // ==========================================================
  // 5 - SE ESTAMOS GIRANDO PORQUE VIMOS O ADVERSÁRIO ATRÁS
  // ==========================================================

  if (estado == GIRANDO_TRASEIRO) {
    executarGiroTraseiro();

    return;
  }

  // ==========================================================
  // 6 - ADVERSÁRIO NA FRENTE
  // ==========================================================

  if (adversarioFrente()) {
    estado = ATACANDO;

    atacar();

    return;
  }

  // ==========================================================
  // 7 - ADVERSÁRIO ATRÁS
  // ==========================================================

  if (adversarioTras()) {
    iniciarGiroTraseiro();

    executarGiroTraseiro();

    return;
  }

  // ==========================================================
  // 8 - MEMÓRIA DO ATAQUE
  // ==========================================================

  if (estado == ATACANDO && millis() - ultimoAlvoFrente < MEMORIA_ALVO_MS) {
    frente(VELOCIDADE_ATAQUE);

    return;
  }

  // ==========================================================
  // 9 - NENHUM ADVERSÁRIO
  //
  // PROCURA COM UMA RODA PARADA
  // ==========================================================

  estado = PROCURANDO;

  procurar();
}