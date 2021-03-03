/*
 * Versao 6 do braço 2
 * Tentativa de medir "3 e 2 fases"
   
    
   Pinagem do módulo SD na ESP32
   CS: D5
   SCK:D18
   MOSI:D23
   MISO:D19
*/  

#include "EmonLib.h"
#include <driver/adc.h>

#define ADC_BITS    10
#define ADC_COUNTS  (1<<ADC_BITS)
#define vCalibration_f1 165
#define currCalibration_f1 1.16
#define phase_shift_f1 1.7
#define vCalibration_f2 165
#define currCalibration_f2 1.16
#define phase_shift_f2 1.7
#define vCalibration_f3 165
#define currCalibration_f3 1.16
#define phase_shift_f3 1.7

EnergyMonitor emon_f1, emon_f2, emon_f3;

float TensaoAlimentacao_f1, FatorPotencia_f1, PotenciaAparente_f1, PotenciaReal_f1;
float TensaoAlimentacao_f2, FatorPotencia_f2, PotenciaAparente_f2, PotenciaReal_f2;
float TensaoAlimentacao_f3, FatorPotencia_f3, PotenciaAparente_f3, PotenciaReal_f3;

double Irms_f1, Irms_f2, Irms_f3;

unsigned long t, dt;
//Funçoes ---------------------------

void setup () {
  emon_f1.voltage(35, vCalibration_f1, phase_shift_f1); // Voltage: input pin, calibration, phase_shift_f1
  emon_f1.current(34, currCalibration_f1); // Current: input pin, calibration.
  emon_f2.voltage(35, vCalibration_f2, phase_shift_f2); // Voltage: input pin, calibration, phase_shift_f1
  emon_f2.current(32, currCalibration_f2); // Current: input pin, calibration.
  emon_f3.voltage(35, vCalibration_f3, phase_shift_f3); // Voltage: input pin, calibration, phase_shift_f1
  emon_f3.current(33, currCalibration_f3); // Current: input pin, calibration.
  Serial.begin(9600);
}

void loop () {
  
  t = millis();
  emon_f1.calcVI(60, 900);
  emon_f2.calcVI(60, 900);
  emon_f3.calcVI(60, 900);
  dt = millis() - t;
  //Fase 1
  TensaoAlimentacao_f1   = emon_f1.Vrms;
  Irms_f1 = emon_f1.calcIrms(1480);  // Calculate Irms_f1 only
  PotenciaReal_f1 = emon_f1.realPower;
  PotenciaAparente_f1 = emon_f1.apparentPower;
  FatorPotencia_f1 = emon_f1.powerFactor;
  //Fase 2
  TensaoAlimentacao_f2   = emon_f2.Vrms;
  Irms_f2 = emon_f2.calcIrms(1480);  // Calculate Irms_f2 only
  PotenciaReal_f2 = emon_f2.realPower;
  PotenciaAparente_f2 = emon_f2.apparentPower;
  FatorPotencia_f2 = emon_f2.powerFactor;
  //Fase 3
  TensaoAlimentacao_f3   = emon_f3.Vrms;
  Irms_f3 = emon_f3.calcIrms(1480);  // Calculate Irms_f3 only
  PotenciaReal_f3 = emon_f3.realPower;
  PotenciaAparente_f3 = emon_f3.apparentPower;
  FatorPotencia_f3 = emon_f3.powerFactor;
    
  Serial.print("Vs1: " + String(TensaoAlimentacao_f1) + " Irms_f1: " + String(Irms_f1) + " P1: " + String(PotenciaReal_f1));
  Serial.print(" S1: " + String(PotenciaAparente_f1) + " FP1: " + String(FatorPotencia_f1) + "\n");
  Serial.print("Vs2: " + String(TensaoAlimentacao_f2) + " Irms_f2: " + String(Irms_f2) + " P2: " + String(PotenciaReal_f2));
  Serial.print(" S2: " + String(PotenciaAparente_f2) + " FP2: " + String(FatorPotencia_f2) + "\n");
  Serial.print("Vs3: " + String(TensaoAlimentacao_f3) + " Irms_f3: " + String(Irms_f3) + " P3: " + String(PotenciaReal_f3));
  Serial.print(" S3: " + String(PotenciaAparente_f3) + " FP3: " + String(FatorPotencia_f3) + "\n" + String(dt) + "\n\n");
  
}
