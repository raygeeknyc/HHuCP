/*
 *
 * Basic test code for the HHuCP
 * 2019, Raymond Blum <raygeeknyc@gmail.com>
 */

#include <AnalogSensor.h>
#include <PingSensor.h>
#include <RgbLed.h>
#include <ks0108.h>
#include <Arial14.h>         // proportional font
#include <SystemFont5x7.h>   // system font
#include <Arial14.h>         // font 

#include <IRremote.h>
#include <DS1302.h>

#define SONAR_PIN 39

#define IR_RECV_PIN 28
IRrecv irrecv(IR_RECV_PIN);
decode_results results;

#define DS1302_SCLK_PIN 29
#define DS1302_IO_PIN 30
#define DS1302_CE_PIN 31
DS1302 rtc(DS1302_CE_PIN, DS1302_IO_PIN, DS1302_SCLK_PIN);
DS1302_RAM DS1302RAMBuffer;

#define LED_PIN 6
#define CDS_PIN 42
AnalogSensor cds_light_sensor("CDS", CDS_PIN, 3, 20, 0, false, 200);
unsigned long int led_until;

#define RGB_B_PIN 33 
#define RGB_R_PIN 34
#define RGB_G_PIN 35
RgbLedCommonCathode rgb_led(RGB_R_PIN, RGB_G_PIN, RGB_B_PIN);

#define PBA_PIN 43
#define PBB_PIN 44
#define TOGGLE_PIN 45

#define AMBI_LIGHT_PIN 38
AnalogSensor ambi_light_sensor("AMBI", AMBI_LIGHT_PIN, 3, 20, 0, false, 200);

#define IR_LED_PIN 32

#define SPEAKER_PIN 27

#define PULSE_DURATION_MS 500
#define TONE_DURATION_MS 250
unsigned long int ir_until;

unsigned long startMillis;
unsigned int loops = 0;
unsigned int iter = 0;
int ledtoggle;
char* TOGGLE;
char* PBA;
char* PBB;
String IR_CODE;
char* ir_code_chars;
int light_level;
int eeprom_count;

int buspins[13] = {0,1,2,3,4,5,6,24,25,26,40,41};

int current_buspin_idx = 0;

void setup(){

  Serial.begin(9600);
  Serial.println("HHuCP Setup");
  GLCD.Init(NON_INVERTED);   // initialize the library, non inverted writes pixels onto a clear screen
  GLCD.ClearScreen();  

  pinMode(PBA_PIN, INPUT);
  pinMode(PBB_PIN, INPUT);
  pinMode(TOGGLE_PIN, INPUT);
  
  pinMode(SPEAKER_PIN, OUTPUT);
  pinMode(AMBI_LIGHT_PIN, INPUT);
  pinMode(IR_LED_PIN, OUTPUT);

  // Set the clock to run-mode, and enable the write protection
  rtc.halt(false);
  rtc.writeProtect(false);
  eeprom_count = rtc.peek(1)*256 + rtc.peek(0);
  eeprom_count += 1;
  int count_byte;
  count_byte = (eeprom_count - rtc.peek(0))/256;
  rtc.poke(1, count_byte);
  count_byte = eeprom_count - (count_byte * 256);
  rtc.poke(0, count_byte);
  
  /**
  rtc.setDOW(SATURDAY);        // Set Day-of-Week to SATURDAY
  rtc.setTime(12, 0, 0);     // Set the time to 12:00:00 (24hr format)
  rtc.setDate(6, 8, 2010);   // Set the date to August 6th, 2010
  */
  
  rtc.writeProtect(true);

 irrecv.enableIRIn(); // Start the receiver
 resetBuspins();
 
 pinMode(SONAR_PIN, INPUT);
 pinMode(LED_PIN, OUTPUT);
 pinMode(CDS_PIN, INPUT);
 }


void  loop(){   // run over and over again
   
  boolean play = false;
  int frequency = 0;
  
  int rgb_color = 0;
  ambi_light_sensor.takeSample();
  cds_light_sensor.takeSample();
 
  if (digitalRead(TOGGLE_PIN) == 1) {if (TOGGLE != "X") { TOGGLE = "X"; play = true;}} else { TOGGLE = "O"; }
  if (digitalRead(PBA_PIN) == 0) {if (PBA != "X") { PBA = "X"; play = true;}} else { PBA = "O"; }
  if (digitalRead(PBB_PIN) == 0) {if (PBB != "X") { PBB = "X"; play = true;}} else { PBB = "O"; }

  if (play) {
    frequency = ((digitalRead(TOGGLE_PIN) == 1)?250:0) +
      ((digitalRead(PBA_PIN) == 0)?250:0) +
      ((digitalRead(PBB_PIN) == 0)?250:0);
    tone(SPEAKER_PIN, frequency, TONE_DURATION_MS);
  }

  if (irrecv.decode(&results)) {
    IR_CODE = String(results.value, HEX);
    irrecv.resume(); // Receive the next value
    Serial.print("IR: ");
    Serial.println(IR_CODE);
    advanceBusPin();
  }
  
  if (ir_code_chars) {
    delete(ir_code_chars);
    ir_code_chars = 0L;
  }

  ir_code_chars = (char*)malloc(IR_CODE.length() + 1);
  IR_CODE.toCharArray(ir_code_chars, IR_CODE.length());
  ir_code_chars[IR_CODE.length()] = '\0';
  
  light_level = ambi_light_sensor.lastState();

  GLCD.DrawRect(0, 4, 127, 59, BLACK); // rectangle in left side of screen

  GLCD.SelectFont(Arial_14);
  GLCD.CursorTo(3, 0);
  GLCD.Puts("H H u C P");
  GLCD.Puts("  :  ");
  GLCD.PrintNumber(eeprom_count);
  
  GLCD.SelectFont(System5x7); // switch to fixed width system font 

  // Show Day-of-Week
  GLCD.CursorTo(1, 2);
  GLCD.Puts(rtc.getDOWStr());
  GLCD.CursorTo(1,3);  
  GLCD.Puts(rtc.getDateStr());

  GLCD.CursorTo(1, 4);
  GLCD.Puts(rtc.getTimeStr());

  GLCD.CursorTo(12, 2);              // positon cursor  
  GLCD.Puts("TOGGLE=");             // print a text string
  GLCD.Puts(TOGGLE);

  GLCD.CursorTo(12, 3);              // positon cursor  
  GLCD.Puts("PBA=");               // print a text string
  GLCD.Puts(PBA);
  
  GLCD.CursorTo(12, 4);              // positon cursor  
  GLCD.Puts("PBB=");               // print a text string
  GLCD.Puts(PBB);

  GLCD.CursorTo(6, 5);
  GLCD.Puts("Dist=");
  GLCD.PrintNumber(analogRead(SONAR_PIN));
  GLCD.Puts("  ");
  
  GLCD.CursorTo(2, 6);              // positon cursor  
  GLCD.Puts("LITE=");           // print a text string
  GLCD.PrintNumber(light_level);
  GLCD.Puts("   ");

  GLCD.CursorTo(11, 6);              // positon cursor  
  GLCD.Puts("IR=");           // print a text string
  GLCD.Puts(ir_code_chars);
  GLCD.Puts(" ");
  if (strlen(ir_code_chars) > 0) {
    GLCD.PrintNumber(buspins[current_buspin_idx]);
  }
  GLCD.Puts(" ");


  rgb_color += (PBA == "X")?Color::RED:Color::NONE;
  rgb_color += (PBB == "X")?Color::BLUE:Color::NONE;
  rgb_color += (TOGGLE == "X")?Color::GREEN:Color::NONE;
  rgb_led.setColor(rgb_color);
  
  if (ambi_light_sensor.isFiring()) {
    pulseIR();
  }
  expireIRPulse();
  
  if (cds_light_sensor.isFiring()) {
    pulseLED();
  }
  expireLEDPulse();
}

void resetBuspins() {
  for (int i=0; i<12; i++) {
    pinMode(buspins[i], OUTPUT);
    digitalWrite(buspins[i], LOW);
  }
}

void advanceBusPin() {
  resetBuspins();  
  current_buspin_idx+=1;
  if (current_buspin_idx >= 12) current_buspin_idx = 0;
  digitalWrite(buspins[current_buspin_idx], HIGH);
}

void pulseIR() {
  ir_until = millis() + PULSE_DURATION_MS;
  digitalWrite(IR_LED_PIN, HIGH);
}
void pulseLED() {
  led_until = millis() + PULSE_DURATION_MS;
  digitalWrite(LED_PIN, HIGH);
}

boolean expireIRPulse() {
  if (millis() > ir_until) {
    digitalWrite(IR_LED_PIN, LOW);
  }
}
boolean expireLEDPulse() {
  if (millis() > led_until) {
    digitalWrite(LED_PIN, LOW);
  }
}
