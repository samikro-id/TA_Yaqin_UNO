#include <NewPing.h>
#include <SoftwareSerial.h>

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define BACA_SENSOR_INTERVAL  1000

#define LED_ON                LOW
#define LED_OFF               HIGH

#define LED_MERAH             10
#define LED_KUNING            9
#define LED_HIJAU             8

#define JARAK_MERAH           50  // cm
#define JARAK_KUNING          70  // cm
#define JARAK_HIJAU           PING_DISTANCE_MAX // cm

#define PING_DISTANCE_MAX     450 // Maximum distance (in cm) to ping.
#define PING_ITERATION        5
#define PING_DEPAN_TRIGGER    3   // 
#define PING_DEPAN_ECHO       2   //
#define PING_BELAKANG_TRIGGER 4   //
#define PING_BELAKANG_ECHO    5   //

NewPing ping_depan(PING_DEPAN_TRIGGER, PING_DEPAN_ECHO, PING_DISTANCE_MAX);
NewPing ping_belakang(PING_BELAKANG_TRIGGER, PING_BELAKANG_ECHO, PING_DISTANCE_MAX);

#define SERIAL_ESP_RX         11
#define SERIAL_ESP_TX         12

SoftwareSerial SerialEsp(SERIAL_ESP_RX, SERIAL_ESP_TX);   // RX, TX

LiquidCrystal_I2C lcd(0x27, 16, 2); // Address, Coloumn, Row

typedef struct{
  uint32_t jarak_depan_cm;
  uint32_t jarak_belakang_cm;
  String status;
  bool led_merah;
  bool led_kuning;
  bool led_hijau;
}DATA_TypeDef;

DATA_TypeDef data;

String inputString = "";         // a String to hold incoming data
uint32_t timer_sensor;
uint32_t timer_led;

void setup(){
  delay_ms(3000);

  // put your setup code here, to run once:
  Serial.begin(115200);
  SerialEsp.begin(9600);

  // reserve 200 bytes for the inputString:
  inputString.reserve(200);

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LED_MERAH, OUTPUT);
  pinMode(LED_KUNING, OUTPUT);
  pinMode(LED_HIJAU, OUTPUT);
  
  lcd.init();
  lcd.init();
  lcd.backlight();

  timer_sensor = millis();
}

void loop() {
  // put your main code here, to run repeatedly:
  if(SerialEsp.available()){
    // get the new byte:
    char inChar = (char)SerialEsp.read();
    // add it to the inputString:
    inputString += inChar;
    // if the incoming character is a newline, set a flag so the main loop can
    // do something about it:
    if (inChar == '\n') {
      if(inputString == "DATA"){
        sendData();
      }

      inputString = "";
    }
  }

  if((millis() - timer_sensor) > BACA_SENSOR_INTERVAL){
    bacaSensor();

    timer_sensor = millis();
  }

  if((millis() - timer_led) > 500){
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  }
}

void sendData(){
  char buffer[100];
  sprintf(buffer, "%04d%04d%d%d%d%S", data.jarak_depan_cm, data.jarak_belakang_cm, data.led_merah, data.led_kuning, data.led_hijau, data.status);

  Serial.println(buffer);
  SerialEsp.println(buffer);
}

void bacaSensor(){
  uint32_t echo = 0;

  echo = ping_depan.ping_median(PING_ITERATION);
  data.jarak_depan_cm = ping_depan.convert_cm(echo);

  echo = ping_belakang.ping_median(PING_ITERATION);
  data.jarak_belakang_cm = ping_belakang.convert_cm(echo);

  if( (data.jarak_depan < JARAK_MERAH) || (data.jarak_belakang < JARAK_MERAH)){
    status = "";
    status = "STOP";

    digitalWrite(LED_MERAH, LED_ON);      data.led_merah = true;

    digitalWrite(LED_KUNING, LED_OFF);    data.led_kuning = false;
    digitalWrite(LED_HIJAU, LED_OFF);     data.led_hijau = false;
  }
  else if(  (data.jarak_depan < JARAK_KUNING) || (data.jarak_belakang < JARAK_KUNING)){
    status = "";
    status = "WARNING";

    digitalWrite(LED_KUNING, LED_ON);     data.led_kuning = true;

    digitalWrite(LED_MERAH, LED_OFF);     data.led_merah = false;
    digitalWrite(LED_HIJAU, LED_OFF);     data.led_hijau = false;
  }
  else{
    status = "";
    status = "AMAN";

    digitalWrite(LED_HIJAU, LED_ON);      data.led_hijau = true;
    
    digitalWrite(LED_MERAH, LED_OFF);     data.led_merah = false;
    digitalWrite(LED_KUNING, LED_OFF);    data.led_kuning = false;
  }

  lcd.setCursor(0,0);     lcd.print("        ");
  lcd.setCursor(0,0);     lcd.printf("%d cm", data.jarak_depan);
  
  lcd.setCursor(8,0);     lcd.print("        ");
  lcd.setCursor(8,0);     lcd.printf("%d cm", data.jarak_belakang);

  lcd.setCursor(0,1);     lcd.print("               ");
  lcd.setCursor(0,1);     lcd.print("%s", data.status);

  Serial.printf("D: %04d cm, B: %04d cm, status: %s\r\n", data.jarak_depan, data.jarak_belakang, data.status);
}