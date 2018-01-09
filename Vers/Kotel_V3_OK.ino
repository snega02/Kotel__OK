
//Контроллер упраления котлом 1 канал.ino
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <EEPROM.h>


LiquidCrystal_I2C lcd(0x27,20,4);



//**********   Константы и структуры    *********************
const int V1_Pin = 9;      // ПИН Вентилятора
const int N1_Pin = 10;      // ПИН Насоса

struct ROM_DATA{ //ROM структура данных
  int _ROM_Iteracia;  //текущая итерация
  int _ROM_V1_rastopka; //вентилятор растопка
  int _ROM_V1_rabota;   //вентилятор растопка
  int _ROM_V1_zavershenie;     //вентилятор растопка
  int _ROM_V1;
  float _ROM_T_Rastopka;
  float _ROM_T_Zavershenie;
  float _ROM_T_Stop;
  float _ROM_T_Avariya; //
};
ROM_DATA _ROM_DATA;


byte incomingByte = 0; int ss=0;  // переменная для хранения полученного байта

//**********   глобальные переменные    *********************
int _V1_rastopka; int _V1_rabota; int _V1_zavershenie; int _V1; //скорость работы вентилятора в процентах
int _Iteracia;// 1-старт, 2-работы, 3-стоп, 4-авария, 5 - покой
float _T1 = 20; float _T_Rastopka; float _T_Zavershenie; float _T_Stop; float _T_Avariya;
char _Time_rastopka[10];  char _Time_rabota[10]; char _Time_zavershenie[10]; char _Time_Avariya[10]; char _Time_Stop[10];  // время сработки кождой итерации


byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
char week[8][10] = {"Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"};


//**********     ФУНКЦИИ                *********************
 ///// часы ..
byte decToBcd(byte val){
  return ( (val/10*16) + (val%10) );
}

byte bcdToDec(byte val){
  return ( (val/16*10) + (val%16) );
}

void setDateDs1307(byte second,        // 0-59
                   byte minute,        // 0-59
                   byte hour,          // 1-23
                   byte dayOfWeek,     // 1-7
                   byte dayOfMonth,    // 1-28/29/30/31
                   byte month,         // 1-12
                   byte year)          // 0-99
{
   Wire.beginTransmission(0x68);
   Wire.write(0);
   Wire.write(decToBcd(second));    
   Wire.write(decToBcd(minute));
   Wire.write(decToBcd(hour));     
   Wire.write(decToBcd(dayOfWeek));
   Wire.write(decToBcd(dayOfMonth));
   Wire.write(decToBcd(month));
   Wire.write(decToBcd(year));
   Wire.endTransmission();
}

void getDateDs1307(byte *second,byte *minute,byte *hour,byte *dayOfWeek,byte *dayOfMonth,byte *month,byte *year)
{

  Wire.beginTransmission(0x68);
  Wire.write(0);
  Wire.endTransmission();

  Wire.requestFrom(0x68, 7);

  *second     = bcdToDec(Wire.read() & 0x7f);
  *minute     = bcdToDec(Wire.read());
  *hour       = bcdToDec(Wire.read() & 0x3f); 
  *dayOfWeek  = bcdToDec(Wire.read());
  *dayOfMonth = bcdToDec(Wire.read());
  *month      = bcdToDec(Wire.read());
  *year       = bcdToDec(Wire.read());
}
  
/////    температура ..
float get3231Temp(){
  byte tMSB, tLSB; 
  float temp3231;

  Wire.beginTransmission(0x68);
  Wire.write(0x11);
  Wire.endTransmission();
  Wire.requestFrom(0x68, 2);

  if(Wire.available()) {
    tMSB = Wire.read(); //2's complement int portion
    tLSB = Wire.read(); //fraction portion

    temp3231 = (tMSB & B01111111); //do 2's math on Tmsb
    temp3231 += ( (tLSB >> 6) * 0.25 ); //only care about bits 7 & 8
  }
  else {
    //oh noes, no data!
  }

  return temp3231;
}

//////    EEPROM 
//получение
void getEEPROM_DATA(){
  EEPROM.get(0, _ROM_DATA);

  _Iteracia = _ROM_DATA._ROM_Iteracia;  //текущая итерация
  _V1_rastopka = _ROM_DATA._ROM_V1_rastopka; //вентилятор растопка
  _V1_rabota = _ROM_DATA._ROM_V1_rabota;   //вентилятор растопка
  _V1_zavershenie = _ROM_DATA._ROM_V1_zavershenie;   //вентилятор растопка
  _V1 = _ROM_DATA._ROM_V1;
  _T_Rastopka = _ROM_DATA._ROM_T_Rastopka;
  _T_Zavershenie = _ROM_DATA._ROM_T_Zavershenie;
  _T_Stop = _ROM_DATA._ROM_T_Stop;
  _T_Avariya = _ROM_DATA._ROM_T_Avariya;
}

//EEPROM сохранение 
void putEEPROM_DATA(){

  _ROM_DATA._ROM_Iteracia = _Iteracia;  //текущая итерация
  _ROM_DATA._ROM_V1_rastopka = _V1_rastopka; //вентилятор растопка
  _ROM_DATA._ROM_V1_rabota = _V1_rabota;   //вентилятор растопка
  _ROM_DATA._ROM_V1_zavershenie = _V1_zavershenie;     //вентилятор растопка
  _ROM_DATA._ROM_V1 = _V1;
  _ROM_DATA._ROM_T_Rastopka = _T_Rastopka;
  _ROM_DATA._ROM_T_Zavershenie = _T_Zavershenie;
  _ROM_DATA._ROM_T_Stop = _T_Stop;
  _ROM_DATA._ROM_T_Avariya = _T_Avariya;

  EEPROM.put(0, _ROM_DATA);
}

//Настройки по умолчанию
void putDefoult_DATA(){

  _Iteracia = 5;  //текущая итерация
  _V1_rastopka = 60; //вентилятор растопка
  _V1_rabota = 40;   //вентилятор растопка
  _V1_zavershenie = 20;     //вентилятор растопка
  _V1 = 20;
  _T_Rastopka = 55;
  _T_Zavershenie = 50;
  _T_Stop = 30;
  _T_Avariya = 85;

 putEEPROM_DATA(); 
}

//Вывести состояние настроек
void print_Prefference(){

  Serial.print("_Iteracia =       "); Serial.println(_Iteracia);
  Serial.print("_V1_rastopka =    "); Serial.println(_V1_rastopka);
  Serial.print("_V1_rabota =      "); Serial.println(_V1_rabota);
  Serial.print("_V1_zavershenie = "); Serial.println(_V1_zavershenie);
  Serial.print("_V1 =             "); Serial.println(_V1);
  Serial.print("_T_Rastopka =     "); Serial.println(_T_Rastopka);
  Serial.print("_T_Zavershenie =  "); Serial.println(_T_Zavershenie);
  Serial.print("_T_Stop =         "); Serial.println(_T_Stop);
  Serial.print("_T_Avariya =      "); Serial.println(_T_Avariya);
}

void setup() {
  // put your setup code here, to run once:
  // put your setup code here, to run once:
  Serial.begin(9600);
  lcd.init();
  lcd.backlight(); 
  
  pinMode(13, OUTPUT);
  pinMode(V1_Pin, OUTPUT); //0-255  меняет 0-5 В analogWrite(ledPin, brightness);
  pinMode(N1_Pin, OUTPUT); //analogWrite(Pin, Value 0-255);

  //putDefoult_DATA();  // записать дефолтные данные
  getEEPROM_DATA();
  print_Prefference();

}

void loop() {
  // put your main code here, to run repeatedly:
  
//Запрос текущего времени
getDateDs1307(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);
char time[10];  char data[11];
snprintf(time, sizeof(time),"%02d:%02d:%02d",hour, minute, second);
snprintf(data, sizeof(data), "%02d/%02d/%02d",dayOfMonth, month, year);
lcd.setCursor(0,0); lcd.print(data);
lcd.setCursor(0,1); lcd.print(time);

//*************** цифра с серийного монитора ***********
if (Serial.available() > 0) {  //если есть доступные данные
        // считываем байт
        incomingByte = Serial.read();
        // отсылаем то, что получили
        Serial.print("I received: ");
        Serial.println(incomingByte, DEC);

        if (ss== 0)
        {
          _T1 = (incomingByte-48) * 10; // 48 - смещение
          ss=1;
        }
        else
        {
          ss = 0;
        }
}


//_T1 = get3231Temp();

lcd.setCursor(10,0);lcd.print("It-");lcd.print(_Iteracia);
lcd.setCursor(10,1);lcd.print("T1-");lcd.print(_T1);


switch (_Iteracia){  //режимы работы
  case 1: //Старт
     if (_T1 >= _T_Rastopka)
     { //Если температура вылосла до 60 градусов
       _Iteracia = 2;

     }
     else
     {
      analogWrite(V1_Pin, _V1_rastopka); //включить вентилятор
      digitalWrite(N1_Pin, LOW); // выключить насос
     }


  break;
  case 2: //Работа
     if (_T1 >= _T_Avariya)
     { //Если температура вылосла до 85 градусов
       _Iteracia = 4;  //АВАРИЯ
       break;
     }

     if (_T1 <= _T_Zavershenie)
     { //Если температура упала до 50 градусов
       _Iteracia = 3; // Топливо сгорело, перейти к этапу окончания
       break;
     }
     
     //если температура в норме то работаем...
     analogWrite(V1_Pin, _V1_rabota); // включить вентилятор
     digitalWrite(N1_Pin, HIGH);      // выключить насос
     
  break;
  case 3: // Стоп
      if (_T1 <= 30){ //Если температура уменьшилась ниже 30 градусов
        analogWrite(V1_Pin, 0);    // включить вентилятор
        digitalWrite(N1_Pin, LOW); // выключить насос    
      }
      else{  // иначе поддердивать режим догорания топлива
        analogWrite(V1_Pin, _V1_zavershenie); // включить вентилятор
        digitalWrite(N1_Pin, HIGH);    // выключить насос  
      }



  break;
    case 4: //авария
      analogWrite(V1_Pin, 0);     // выключить вентилятор
    digitalWrite(N1_Pin, HIGH); // включить насос  


    break;
    case 5: // Покой
      analogWrite(V1_Pin, 0);     // выключить вентилятор
    digitalWrite(N1_Pin, LOW); // включить насос  


    break;

}






  //13 blink
    digitalWrite(13, HIGH);    delay(1000);    digitalWrite(13, LOW);    delay(1000);
}
