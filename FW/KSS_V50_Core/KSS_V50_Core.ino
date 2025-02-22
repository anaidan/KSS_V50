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

const unsigned long hourInterval = 3600000; // 1 hour in milliseconds
const unsigned long taskDuration = 300000*1; // 5 minutes in milliseconds
unsigned long previousMillis = 0; // Tracks the last time the hourly task started

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
   //PORTC.DIRCLR = PIN_PC2;
   PORTC.PIN2CTRL = PORT_ISC_RISING_gc;
   sei();
   set_sleep_mode(SLEEP_MODE_PWR_DOWN);  /* Set sleep mode to POWER DOWN mode */
   sleep_enable();                       /* Enable sleep mode, but not going to sleep yet */
   sleep_cpu();
}

void loop() {
  
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);  /* Set sleep mode to POWER DOWN mode */
  sleep_enable();                       /* Enable sleep mode, but not going to sleep yet */
  sleep_cpu();

  sleep_disable();
 /*
  unsigned long currentMillis = millis(); // Get the current time

  // Check if an hour has passed
  if (currentMillis - previousMillis >= hourInterval) {
    previousMillis = currentMillis; // Update the previous time
    //performHourlyTask(); // Call the function that runs for 5 minutes
  }
 */
  
  TMC2300_enable();

  for(int i=0; i<500; i++){
    digitalWrite(MOT_STEP_1, LOW);
    digitalWrite(MOT_STEP_2, LOW);
    delayMicroseconds(1500);
    digitalWrite(MOT_STEP_1, HIGH);
    digitalWrite(MOT_STEP_2, HIGH);
    delayMicroseconds(1500);
  }
  //delay(5000);
  //Serial.println("alive");

  TMC2300_disable();



}

// Define the function to run every hour and last for 5 minutes
void performHourlyTask() {
  //Serial.println("Starting hourly task...");
  TMC2300_enable();
  unsigned long taskStartMillis = millis(); // Record the start time of the task
  
  // Stay in the task for 5 minutes
  while (millis() - taskStartMillis < taskDuration) {
    // Perform your task here
    //Serial.println("Task is running...");
    for(int i=0; i<1000; i++){
      digitalWrite(MOT_STEP_1, LOW);
      digitalWrite(MOT_STEP_2, LOW);
      delayMicroseconds(300);
      digitalWrite(MOT_STEP_1, HIGH);
      digitalWrite(MOT_STEP_2, HIGH);
      delayMicroseconds(300);
    }
    INA236_log(INA236_bat, INA236_mppt);

    //delay(10000); // Simulate work with a 1-second delay (adjust as needed)
  }

  TMC2300_disable();
  
  //Serial.println("Hourly task completed.");
}

// sleep mode todo:
// create 1 hr interrupt
// create interrupt on CAP_SNS_OUT

ISR(PORTC_PORT_vect) {
    // Clear the interrupt flag
    PORTC.INTFLAGS = CAP_SNS_OUT;
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
  digitalWrite(MOT_DIR_1, LOW);
  digitalWrite(MOT_DIR_2, LOW);
  digitalWrite(MOT_EN_1, LOW);
  digitalWrite(MOT_EN_2, LOW);
  delay(5);
  
  // Turn VIO/STBY OFF
  digitalWrite(MOT_STBY, HIGH);
  delay(5);
  
  // Turn VIO/STBY ON
  digitalWrite(MOT_STBY, LOW);
  delay(5);
  
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