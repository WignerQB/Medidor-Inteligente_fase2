/*
   Versao 4 do braço 1

   Pinagem do módulo SD na ESP32
   CS: D5
   SCK:D18
   MOSI:D23
   MISO:D19
*/

#include <Wire.h>
#include <DS3231.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "EmonLib.h"
#include <driver/adc.h>
#include "time.h"
#include <SPI.h>

#define SD_CS 5
#define ADC_BITS    10
#define ADC_COUNTS  (1<<ADC_BITS)
#define vCalibration 600
#define currCalibration 4.7
#define phase_shift 1.7

//Declaracao de variáveis String --------------------------------------------
String RTCdata, Dados_de_medicao, Dados_SD, LabelSD;

//Declaracao de variáveis de bibliotecas-------------------------------------
RTClib myRTC;
EnergyMonitor emon;

//Declaracao de variáveis byte-----------------------------------------------
byte Year;
byte Month;
byte Date;
byte DoW;
byte Hour;
byte Minute;
byte Second;

//Declaracao de variáveis unsigned long----------------------------------------
unsigned long timerDelay = 20000, timerDelay2 = 5000, timerDelay3 = 5000;
unsigned long lastTime = 0, lastTime2 = 0, lastTime3 = 0;

//Declaracao de variáveis float-------------------------------------------------
float kWh_FP = 0, kWh_I = 0, kWh_P = 0, TensaoAlimentacao, FatorPotencia, PotenciaAparente, PotenciaReal;
float ConsumoEsperado = 10.00, MetaDiaria, ConsumoDiario = 0, ConsumoTotal = 0, ValorDokWh;//Todos valores simulados

//Declaracao de variáveis double------------------------------------------------
double Irms;

//Declaracao de variáveis int---------------------------------------------------
int ContadorDeDias = 30, DiaAtual, flag_setup = 0;
int blue = 33, green = 32, LedDeErro = 2, ErroNoPlanejamento = 0;
/*
   green -> indica se conseguiu enviar para web
   blue -> indica se conseguiu gravar no SD card
*/
int sumrec_ConsumoDiario[500], sumrec_kWh[500], sumrec_DiaAtual[500], Agrupar_DiaAtual = 0, Agrupar_ConsumoDiario = 0, Agrupar_kWh = 0;
int sumrec_ConsumoTotal[500], Agrupar_ConsumoTotal = 0;
int i = 0 , j, caseTR = 0;

enum ENUM {
  f_medicao,
  incrementar,
  printar
};

ENUM estado = f_medicao, estado_antigo;

//Funçoes ----------------------------------------------------------------------

void appendFile(fs::FS &fs, const char * path, String message) {
  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    Serial.println("Failed to open file for appending");
    return;
  }
  if (file.print(message)) {
    Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
  file.close();
}

void writeFile(fs::FS &fs, const char * path, String message) {
  Serial.printf("Writing file: %s\n", path);

  /* cria uma variável "file" do tipo "File", então chama a função
    open do parâmetro fs recebido. Para abrir, a função open recebe
    os parâmetros "path" e o modo em que o arquivo deve ser aberto
    (nesse caso, em modo de escrita com FILE_WRITE)
  */
  File file = fs.open(path, FILE_WRITE);
  //verifica se foi possivel criar o arquivo
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  /*grava o parâmetro "message" no arquivo. Como a função print
    tem um retorno, ela foi executada dentro de uma condicional para
    saber se houve erro durante o processo.*/
  if (file.print(message)) {
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
}

void readFile(fs::FS &fs, const char * path){
    Serial.printf("Reading file: %s\n", path);
    
    File file = fs.open(path);
    if(!file){
        Serial.println("Failed to open file for reading");
        return;
    }

    
    
    Serial.print("Read from file: ");
    while(file.available()){
        Serial.write(file.read());
    }
    file.close();
}

void SD_config() {
  SD.begin(SD_CS);
  if (!SD.begin()) {
    Serial.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD.cardType();

  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    return;
  }

  Serial.print("SD Card Type: ");
  if (cardType == CARD_MMC) {
    Serial.println("MMC");
  } else if (cardType == CARD_SD) {
    Serial.println("SDSC");
  } else if (cardType == CARD_SDHC) {
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }
  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);

  // Create a file on the SD card and write labels
  File file = SD.open("/ICDataLogger.csv");
  if (!file) {
    LabelSD = "Data \tHorário \tTensao Alimentacao \tIrms \tPotencia Real \tPotencia Aparente \tFator de Potencia \tkWh Horário Fora de Ponta \tkWh Horário Intermediário \tHorário de Ponta\n" ;
    Serial.println("File doens't exist");
    Serial.println("Creating file...");
    writeFile(SD, "/ICDataLogger.csv", LabelSD);
  }
  else {
    Serial.println("File already exists");
  }
  file.close();
  
  readFile(SD, "/ICDataLogger.csv");
}

void setup () {
  Serial.begin(57600);
  Wire.begin();
  adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_11);
  analogReadResolution(10);
  emon.voltage(35, vCalibration, phase_shift); // Voltage: input pin, calibration, phase_shift
  emon.current(34, currCalibration); // Current: input pin, calibration.
  SD_config();
}

void loop () {
  DateTime now = myRTC.now();
  switch (estado) {//Funcionalidades
    case f_medicao://Estado aonde é feita a mensuração da corrente, tensão, as potências ativas e reativas e o fator de potência
      //Nesse estado também é realizado a verificação de possíveis erros dado a imprecisão do sensor de corrente.
      //Serial.println("Estado f_medicao");
      
      emon.calcVI(60, 1000);
      TensaoAlimentacao   = emon.Vrms;
      Irms = emon.calcIrms(1480);  // Calculate Irms only
      PotenciaReal = emon.realPower;
      PotenciaAparente = emon.apparentPower;
      FatorPotencia = emon.powerFactor;
      if (PotenciaReal >= 0){
        estado = incrementar;
      }
      else {
        Serial.println("\n!!Atençao!! Inverta o sentido do sensor de corrente!!\n");
        //        digitalWrite(LedDeErro, HIGH);
        //        digitalWrite(green, LOW);
        //        digitalWrite(blue, LOW);
      }
      break;

    case incrementar://Nesse estado é feito o cálculo do consumo em R$ e em kWh
      //Serial.println("\n\n\n" + String(millis() - lastTime3) + "\n\n\n");
      if ((millis() - lastTime3) >= timerDelay3) {//Horário Intermediário
        if((int(now.hour()) == 18)||(int(now.hour()) == 22)){
          kWh_I = kWh_I + (PotenciaReal / 720) / 1000;
          Dados_de_medicao = String(TensaoAlimentacao) + "\t"  + String(Irms) + "\t"  + String(PotenciaReal) + "\t"  + String(PotenciaAparente) + "\t"  + String(FatorPotencia) + "\t" + String(kWh_FP* 1000000) + "\t" + String(kWh_I* 1000000) + "\t" + String(kWh_P* 1000000);
          Dados_SD = RTCdata + "\t" + Dados_de_medicao + "\n";
          appendFile(SD, "/ICDataLogger.csv", Dados_SD);  
        }
        else if((int(now.hour()) >= 19)&&(int(now.hour()) <= 21)){//Horário de Ponta
          kWh_P = kWh_P + (PotenciaReal / 720) / 1000;  
          Dados_de_medicao = String(TensaoAlimentacao) + "\t"  + String(Irms) + "\t"  + String(PotenciaReal) + "\t"  + String(PotenciaAparente) + "\t"  + String(FatorPotencia) + "\t" + String(kWh_FP* 1000000) + "\t" + String(kWh_I* 1000000) + "\t" + String(kWh_P* 1000000);
          Dados_SD = RTCdata + "\t" + Dados_de_medicao + "\n";
          appendFile(SD, "/ICDataLogger.csv", Dados_SD);
        }
        else{
          kWh_FP = kWh_FP + (PotenciaReal / 720) / 1000;  
          Dados_de_medicao = String(TensaoAlimentacao) + "\t"  + String(Irms) + "\t"  + String(PotenciaReal) + "\t"  + String(PotenciaAparente) + "\t"  + String(FatorPotencia) + "\t" + String(kWh_FP* 1000000) + "\t" + String(kWh_I* 1000000) + "\t" + String(kWh_P* 1000000);
          Dados_SD = RTCdata + "\t" + Dados_de_medicao + "\n";
          appendFile(SD, "/ICDataLogger.csv", Dados_SD);
        }
        lastTime3 = millis();
      }
      estado = printar;
      break;

    case printar://Printa as informações lidas e calculadas
      if ((millis() - lastTime2) >= timerDelay2)
      {

        DateTime now = myRTC.now();

        //A separacao de células é feita pelo \t
        RTCdata = String(now.year()) + "/" + String(now.month()) + "/" + String(now.day()) + "\t" + String(now.hour()) + ":" + String(now.minute()) + ":" + String(now.second());
        Serial.println(RTCdata);
        //Serial.println(now.hour());
        Serial.print(" Vrms: ");
        Serial.print(TensaoAlimentacao);
        Serial.print(" V     ");
        Serial.print("Irms: ");
        Serial.print(Irms);
        Serial.print(" A     ");
        Serial.print("Potência Real: ");
        Serial.print(PotenciaReal);
        Serial.print(" W     ");
        Serial.print("Potência Aparente: ");
        Serial.print(PotenciaAparente);
        Serial.print(" VA");
        Serial.print("Fator de Potência: ");
        Serial.println(FatorPotencia);
        Serial.print("kWh*1000000: ");
        if((int(now.hour()) == 18)||(int(now.hour()) == 22)){
          Serial.println(String(kWh_I * 1000000));
        }
        else if((int(now.hour()) >= 19)&&(int(now.hour()) <= 21)){//Horário de Ponta
          Serial.println(String(kWh_P * 1000000));
        }
        else{
          Serial.println(String(kWh_FP * 1000000));
        }
        estado = f_medicao;
        //delay(400);
        lastTime2 = millis();
      }
      break;

    default :
      //Serial.println("Estado default");
      break;
  }
}
