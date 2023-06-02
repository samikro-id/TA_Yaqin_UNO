#include <NewPing.h>
#include <SoftwareSerial.h>

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define BACA_SENSOR_INTERVAL  2000

#define JARAK_MERAH           50  // cm
#define JARAK_KUNING          70  // cm
#define JARAK_HIJAU           100 // cm

#define PING_DISTANCE_MAX     600 // Maximum distance (in cm) to ping.
#define PING_ITERATION        3
#define PING_DEPAN_ECHO       2   //
#define PING_DEPAN_TRIGGER    3   // 
#define PING_KIRI_ECHO        4   //
#define PING_KIRI_TRIGGER     5   //
#define PING_KANAN_ECHO       6   //
#define PING_KANAN_TRIGGER    7   //
#define PING_BELAKANG_ECHO    8   //
#define PING_BELAKANG_TRIGGER 9   //


NewPing ping_depan(PING_DEPAN_TRIGGER, PING_DEPAN_ECHO, PING_DISTANCE_MAX);
NewPing ping_kiri(PING_KIRI_TRIGGER, PING_KIRI_ECHO, PING_DISTANCE_MAX);
NewPing ping_kanan(PING_KANAN_TRIGGER, PING_KANAN_ECHO, PING_DISTANCE_MAX);
NewPing ping_belakang(PING_BELAKANG_TRIGGER, PING_BELAKANG_ECHO, PING_DISTANCE_MAX);

#define BUZZER_ON             LOW
#define BUZZER_OFF            HIGH
#define BUZZER                A3

#define LED_ON                LOW
#define LED_OFF               HIGH

#define LED_MERAH             A2
#define LED_KUNING            A1
#define LED_HIJAU             A0

#define SERIAL_ESP_RX         12
#define SERIAL_ESP_TX         11
SoftwareSerial SerialEsp(SERIAL_ESP_RX, SERIAL_ESP_TX);   // RX, TX

LiquidCrystal_I2C lcd(0x27, 16, 2); // Address, Coloumn, Row

typedef struct{
  uint16_t jarak_depan_cm;
  uint16_t jarak_kiri_cm;
  uint16_t jarak_kanan_cm;
  uint16_t jarak_belakang_cm;
  String status;
  bool led_merah;
  bool led_kuning;
  bool led_hijau;
}DATA_TypeDef;

DATA_TypeDef data;

String inputString = "";         // a String to hold incoming data
uint32_t timer_sensor;
uint32_t timer_led;
uint32_t timer_buzzer;
uint32_t timeout_buzzer;

void setup(){
  delay(3000);

  // put your setup code here, to run once:
  Serial.begin(9600);
  SerialEsp.begin(9600);

  // reserve 200 bytes for the inputString:
  inputString.reserve(50);

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LED_MERAH, OUTPUT);
  pinMode(LED_KUNING, OUTPUT);
  pinMode(LED_HIJAU, OUTPUT);
  digitalWrite(LED_MERAH, HIGH);
  digitalWrite(LED_KUNING, HIGH);
  digitalWrite(LED_HIJAU, HIGH);

  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, BUZZER_OFF);

  Serial.println("start");

  lcd.init();
  lcd.init();
  lcd.backlight();

  timer_sensor = millis();
}

void loop() {
  // put your main code here, to run repeatedly:
  receiveData();

  if((millis() - timer_sensor) > BACA_SENSOR_INTERVAL){
    timer_sensor = millis();
    
    bacaSensor();
  }

  if((millis() - timer_led) > 500){
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));

    timer_led = millis();
  }

  if(data.led_merah == true){       timeout_buzzer = 0; }
  else if(data.led_kuning == true){ timeout_buzzer = 1000; }
  else if(data.led_hijau == true){  timeout_buzzer = 2000; }
  else{ timeout_buzzer = 0xFFFFFFFF;  }

  if(timeout_buzzer == 0){
    digitalWrite(BUZZER, BUZZER_ON);
  }
  else if(timeout_buzzer == 0xFFFFFFFF){
    digitalWrite(BUZZER, BUZZER_OFF);
  }
  else{
    if(digitalRead(BUZZER) == BUZZER_ON){
      if((millis() - timer_buzzer) > 500){
        digitalWrite(BUZZER, BUZZER_OFF);
        timer_buzzer = millis();
      }
    }
    else{
      if((millis() - timer_buzzer) > timeout_buzzer){
        digitalWrite(BUZZER, BUZZER_ON);
        timer_buzzer = millis();
      }
    }    
  }
}

void receiveData(){
  if(SerialEsp.available()){
    char inChar = (char)SerialEsp.read();
    if (inChar == '\r'){  }
    else if (inChar == '\n') {
      Serial.println(inputString);
      
      if(inputString == "DATA"){
        sendData();
      }

      inputString = "";
    }
    else{
      inputString += inChar;
    }
  }
}

void sendData(){
  delay(100);
  
  char buffer[100];
  sprintf(buffer, "UNO%04d%04d%04d%04d%d%d%d%s", 
            data.jarak_depan_cm, data.jarak_belakang_cm, 
            data.jarak_kiri_cm, data.jarak_kanan_cm,
            data.led_merah, data.led_kuning, data.led_hijau, 
            data.status.c_str());

  Serial.println(buffer);
  SerialEsp.println(buffer);
}

void bacaSensor(){
  uint32_t echo = 0;

  echo = ping_depan.ping_median(PING_ITERATION);
  data.jarak_depan_cm = ping_depan.convert_cm(echo);

  receiveData();

  echo = ping_kiri.ping_median(PING_ITERATION);
  data.jarak_kiri_cm = ping_kiri.convert_cm(echo);
  
  receiveData();

  echo = ping_kanan.ping_median(PING_ITERATION);
  data.jarak_kanan_cm = ping_kanan.convert_cm(echo);

  receiveData();

  echo = ping_belakang.ping_median(PING_ITERATION);
  data.jarak_belakang_cm = ping_belakang.convert_cm(echo);

  if( (data.jarak_depan_cm < JARAK_MERAH) || (data.jarak_belakang_cm < JARAK_MERAH)
      || (data.jarak_kiri_cm < JARAK_MERAH) || (data.jarak_kanan_cm < JARAK_MERAH) 
  ){
    data.status = "";
    data.status = "STOP";

    digitalWrite(LED_MERAH, LED_ON);      data.led_merah = true;

    digitalWrite(LED_KUNING, LED_OFF);    data.led_kuning = false;
    digitalWrite(LED_HIJAU, LED_OFF);     data.led_hijau = false;
  }
  else if(  (data.jarak_depan_cm < JARAK_KUNING) || (data.jarak_belakang_cm < JARAK_KUNING)
          ||(data.jarak_kiri_cm < JARAK_KUNING) || (data.jarak_kanan_cm < JARAK_KUNING)
  ){
    data.status = "";
    data.status = "WARNING";

    digitalWrite(LED_KUNING, LED_ON);     data.led_kuning = true;

    digitalWrite(LED_MERAH, LED_OFF);     data.led_merah = false;
    digitalWrite(LED_HIJAU, LED_OFF);     data.led_hijau = false;
  }
  else if(  (data.jarak_depan_cm < JARAK_HIJAU) || (data.jarak_belakang_cm < JARAK_HIJAU)
          ||(data.jarak_kiri_cm < JARAK_HIJAU) || (data.jarak_kanan_cm < JARAK_HIJAU)
  ){
    data.status = "";
    data.status = "AMAN";

    digitalWrite(LED_HIJAU, LED_ON);      data.led_hijau = true;
    
    digitalWrite(LED_MERAH, LED_OFF);     data.led_merah = false;
    digitalWrite(LED_KUNING, LED_OFF);    data.led_kuning = false;
  }
  else{
    data.status = "";
    data.status = "JAUH";
    
    digitalWrite(LED_MERAH, LED_OFF);     data.led_merah = false;
    digitalWrite(LED_KUNING, LED_OFF);    data.led_kuning = false;
    digitalWrite(LED_HIJAU, LED_OFF);      data.led_hijau = false;
  }

  lcd.setCursor(0,0);     lcd.print("        ");
  lcd.setCursor(0,0);     lcd.print(data.jarak_depan_cm);
  lcd.setCursor(5,0);     lcd.print("cm");
  
  lcd.setCursor(8,0);     lcd.print("        ");
  lcd.setCursor(8,0);     lcd.print(data.jarak_belakang_cm);
  lcd.setCursor(13,0);    lcd.print("cm");

  lcd.setCursor(0,1);     lcd.print("        ");
  lcd.setCursor(0,1);     lcd.print(data.jarak_kiri_cm);
  lcd.setCursor(5,1);     lcd.print("cm");
  
  lcd.setCursor(8,1);     lcd.print("        ");
  lcd.setCursor(8,1);     lcd.print(data.jarak_kanan_cm);
  lcd.setCursor(13,1);    lcd.print("cm");

  // lcd.setCursor(0,1);     lcd.print("               ");
  // lcd.setCursor(0,1);     lcd.print(data.status);

  char buffer[500];
  sprintf(buffer, "D: %04d cm, B: %04d cm, Kr: %04d cm, Kn: %04d cm, status: ", data.jarak_depan_cm, data.jarak_belakang_cm, data.jarak_kiri_cm, data.jarak_kanan_cm);
  Serial.print(buffer);
  Serial.println(data.status);
}
