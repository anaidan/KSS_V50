#include <Wire.h>
#include <megaTinyCore.h>
#include <INA236.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>


INA236 INA236_bat(0x40);
INA236 INA236_mppt(0x41);

#define MOT_STBY   PIN_PC3 

#define MOT_EN_1   PIN_PC1
#define MOT_STEP_1 PIN_PA3 
#define MOT_DIR_1  PIN_PA4
#define HALL_IN_1 PIN_PC0

#define MOT_EN_2   PIN_PA6
#define MOT_STEP_2 PIN_PA7
#define MOT_DIR_2  PIN_PB2
#define HALL_IN_2 PIN_PA5

#define MPPT_SHDN PIN_PB5
#define CAP_SNS_OUT PIN_PC2


volatile uint8_t minute_counter = 0;  // Keep track of elapsed minutes
volatile uint8_t hour_flag = 0;  
volatile uint8_t cap_sns_flag = 0;  

long step_1_cnt = 0;

long step_1_pos = 0;
long step_2_pos = 0; 

long max_pos = 3000;
long min_pos = 0;

int dir_1 = 0; // Towards panels
int dir_2 = 0; // towards panels

void setup() {

  pinMode_init();
  digitalWrite(MPPT_SHDN, LOW);

  Serial.pins(PIN_PA1, PIN_PA2);
  Serial.begin(9600);
  
  Wire.pins(PIN_PB1, PIN_PB0);
  Wire.begin();
  Wire.setClock(50000);

  INA236_bat.setMaxCurrentShunt(0.41, 0.050, false);
  INA236_mppt.setMaxCurrentShunt(0.41, 0.050, false);

  //printConfig(INA236_bat); 
  //printConfig(INA236_mppt); 

  TMC2300_enable();
  //measure(5, INA236_bat);
  //measure(5, INA236_mppt);
  TMC2300_disable();
  //measure(1, INA236_bat);
  //measure(5, INA236_mppt);

   ADC0.CTRLA &= ~ADC_ENABLE_bm;
   PORTC.PIN2CTRL = PORT_ISC_RISING_gc;
   RTC_init();
   sei();
   set_sleep_mode(SLEEP_MODE_PWR_DOWN);  /* Set sleep mode to POWER DOWN mode */
   sleep_enable();                       /* Enable sleep mode, but not going to sleep yet */
   sleep_cpu();
}

void loop() {
  
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);  // Set sleep mode to POWER DOWN mode 
  sleep_enable();                       // Enable sleep mode, but not going to sleep yet
  sleep_cpu();

  sleep_disable();

  if(cap_sns_flag==1 || hour_flag==1){

    TMC2300_enable();

    //digitalWrite(MOT_DIR_1, HIGH);
    //digitalWrite(MOT_DIR_2, LOW);

    home();


    Serial.println(digitalRead(HALL_IN_1));
    Serial.println(digitalRead(HALL_IN_2));

    TMC2300_disable();

    if(cap_sns_flag==1 || hour_flag==1){
      cap_sns_flag = 0;
      hour_flag = 0;
    }

    /*if(digitalRead(HALL_IN_1)==0){
      digitalWrite(MOT_DIR_1, LOW);
      Serial.println("end stop");
      step_1_cnt = 0;
      
    }

    if(step_1_cnt > 2000){
      digitalWrite(MOT_DIR_1, HIGH);    
      Serial.println("reverse");
    }*/

  }



}

void home(){
  digitalWrite(MOT_DIR_1, HIGH);
  digitalWrite(MOT_DIR_2, LOW);

  step_motors(10000, 2000);

  step_1_pos = 0;
  step_2_pos = 0;
 

}

void step_motors(long step_cnt, long speed){
    for(int i=0; i<step_cnt; i++){
      
      digitalWrite(MOT_STEP_1, LOW);
      digitalWrite(MOT_STEP_2, LOW);

      delayMicroseconds(speed);
      
      if(digitalRead(HALL_IN_1)!=0){
        digitalWrite(MOT_STEP_1, HIGH);
      }
      if(digitalRead(HALL_IN_2)!=0){
        digitalWrite(MOT_STEP_2, HIGH);
      }
      
      delayMicroseconds(speed);

      if(digitalRead(HALL_IN_1)==0 && HALL_IN_2==0){
        break;
      }  
    }  
}

void set_pos(long set_point_1, long speed_1, long set_point_2, long speed_2){
  if(set_point_1>step_1_pos){
    digitalWrite(MOT_DIR_1, LOW);
    dir_1 = 1;
  }
  else{
    digitalWrite(MOT_DIR_1, HIGH);
    dir_1 = -1;
  }

  if(set_point_2>step_2_pos){
    digitalWrite(MOT_DIR_2, HIGH);
    dir_2 = 1;
  }
  else{
    digitalWrite(MOT_DIR_2, LOW);
    dir_2 = -1;
  }  

  long current_time = micros();
  long prev_time    = micros();
  digitalWrite(MOT_STEP_1, LOW);

  while((abs(set_point_1-step_1_pos)>10) || (abs(set_point_1-step_1_pos)>10)){
    
    current_time = micros();
    
    if((current_time - prev_time) >= speed_1 && (abs(set_point_1-step_1_pos)>10)){
      digitalWrite(MOT_STEP_1, HIGH);
      step_1_pos+=dir_1;
      prev_time = current_time;
    }
    digitalWrite(MOT_STEP_1, LOW);

    current_time = micros();

    if((current_time - prev_time) >= speed_2 && (abs(set_point_2-step_2_pos)>10)){
      digitalWrite(MOT_STEP_2, HIGH);
      step_2_pos+=dir_2;
      prev_time = current_time;
    }
    digitalWrite(MOT_STEP_2, LOW);
  }

}

ISR(PORTC_PORT_vect) {
    // Clear the interrupt flag
    PORTC.INTFLAGS = CAP_SNS_OUT;
    cap_sns_flag = 1; 
}

// RTC PIT Interrupt Service Routine (fires every minute)
ISR(RTC_PIT_vect) {
    RTC.PITINTFLAGS = RTC_PI_bm;  // Clear interrupt flag

    minute_counter++;
    if (minute_counter >= 20) {  // 60 minutes = 1 hour
        minute_counter = 0;
        hour_flag = 1;
    }
}

void RTC_init(){
  while (RTC.STATUS > 0);  // Wait for synchronization
  
  // Enable the internal 32.768kHz oscillator for RTC
  RTC.CLKSEL = RTC_CLKSEL_INT32K_gc;

  // Enable RTC PIT with a 1-minute interval
  //RTC.PITCTRLA = RTC_PERIOD_CYC32768_gc | RTC_PITEN_bm;  // 64-second tick
  RTC.PITCTRLA = RTC_PERIOD_CYC32768_gc | RTC_PITEN_bm;  // 1-second tick
  RTC.PITINTCTRL = RTC_PI_bm;  // Enable periodic interrupt

}

void printConfig(INA236 INA)
{
  Serial.print("LSB:\t");
  Serial.println(INA.getCurrentLSB(), 10);
  Serial.print("LSB_uA:\t");
  Serial.println(INA.getCurrentLSB_uA(), 3);
  Serial.print("shunt:\t");
  Serial.println(INA.getShunt(), 3);
  Serial.print("maxCur:\t");
  Serial.println(INA.getMaxCurrent(), 3);
  Serial.println();
}

void measure(uint8_t count, INA236 INA)
{
  // delay(3000);
  Serial.println("\nBUS\tSHUNT\tCURRENT\tPOWER");
  Serial.println(" V\t mV\t mA\t mW");
  for (int i = 0; i < count; i++)
  {
    Serial.print(INA.getBusVoltage(), 3);
    Serial.print("\t");
    Serial.print(INA.getShuntVoltage_mV(), 3);
    Serial.print("\t");
    Serial.print(INA.getCurrent_mA(), 3);
    Serial.print("\t");
    Serial.print(INA.getPower_mW(), 3);
    Serial.println();
    delay(100);
  }
}

void INA236_log(INA236 INA_bus, INA236 INA_mppt){
  Serial.print(INA_bus.getBusVoltage(), 3);
  Serial.print("\t");
  Serial.print(INA_bus.getShuntVoltage_mV(), 3);
  Serial.print("\t");
  Serial.print(INA_bus.getCurrent_mA(), 3);
  Serial.print("\t");
  Serial.print(INA_bus.getPower_mW(), 3);
    
  Serial.println();  
  
  Serial.print(INA_mppt.getBusVoltage(), 3);
  Serial.print("\t");
  Serial.print(INA_mppt.getShuntVoltage_mV(), 3);
  Serial.print("\t");
  Serial.print(INA_mppt.getCurrent_mA(), 3);
  Serial.print("\t");
  Serial.print(INA_mppt.getPower_mW(), 3);

  Serial.println();  

}

void TMC2300_enable(){
  // Bring all MCU outputs low interfacing with motor driver, VIO/STBY ON
  digitalWrite(MOT_STEP_1, LOW);
  digitalWrite(MOT_STEP_2, LOW);
  //digitalWrite(MOT_DIR_1, HIGH);
  //digitalWrite(MOT_DIR_2, LOW);
  digitalWrite(MOT_EN_1, LOW);
  digitalWrite(MOT_EN_2, LOW);
  delay(5);
  
  // Turn VIO/STBY OFF
  digitalWrite(MOT_STBY, HIGH);
  delay(5);
  
  // Turn VIO/STBY ON
  digitalWrite(MOT_STBY, LOW);
  delay(500);
  
  // Turn Enables ON
  digitalWrite(MOT_EN_1, HIGH);
  digitalWrite(MOT_EN_2, HIGH);
  delay(5);
}

void TMC2300_disable(){
  // Bring all MCU outputs low interfacing with motor driver, VIO/STBY ON
  digitalWrite(MOT_STEP_1, LOW);
  digitalWrite(MOT_STEP_2, LOW);
  digitalWrite(MOT_DIR_1, LOW);
  digitalWrite(MOT_DIR_2, LOW);
  digitalWrite(MOT_EN_1, LOW);
  digitalWrite(MOT_EN_2, LOW);
  delay(5);
  
  // Turn VIO/STBY OFF
  digitalWrite(MOT_STBY, HIGH);
}

void pinMode_init(){
  pinMode(CAP_SNS_OUT, INPUT);
  pinMode(MOT_STBY, OUTPUT);
  pinMode(CAP_SNS_OUT, INPUT);
  
  pinMode(MOT_EN_1, OUTPUT);   
  pinMode(MOT_STEP_1, OUTPUT);  
  pinMode(MOT_DIR_1, OUTPUT);   
  pinMode(HALL_IN_1, INPUT);

  pinMode(MOT_EN_2, OUTPUT);    
  pinMode(MOT_STEP_2, OUTPUT);  
  pinMode(MOT_DIR_2, OUTPUT);  
  pinMode(HALL_IN_2, INPUT);

  pinMode(MPPT_SHDN, OUTPUT);
}