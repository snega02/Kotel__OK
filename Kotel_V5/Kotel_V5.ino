
//Контроллер упраления котлом 1 канал.ino
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <EEPROM.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Data wire is plugged into port 2 DALLAS temperature
#define ONE_WIRE_BUS 2
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature _sensor1(&oneWire);

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
int _Count = -3000;  int __t1; int __t2;

int _V1_rastopka; int _V1_rabota; int _V1_zavershenie; int _N1; int _V1; //скорость работы вентилятора в процентах

int _Iteracia;// 1-старт, 2-работы, 3-стоп, 4-авария, 5 - покой
float _T1 = 20; float _T_Rastopka; float _T_Zavershenie; float _T_Stop; float _T_Avariya;


byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
char week[8][10] = {"Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"};

char _Rezhim[6][6] = {"","Start", "Work ", "Stop ", "Alarm", "Sleep"};


// Миллисекунды
unsigned long _m, _m_rastopka, _m_rabota, _m_zavershenie, _m_Avariya, _m_Stop, _m_Start;
char _Time[10]; char _Time_rastopka[10];  char _Time_rabota[10]; char _Time_zavershenie[10]; char _Time_Avariya[10]; char _Time_Stop[10];  // время сработки кождой итерации


//**********     ФУНКЦИИ                *********************
// Запрос температуры
float GetTempetarure(DallasTemperature sensor1)
{
   // sensor1.requestTemperaturesByIndex;
  sensor1.requestTemperatures();
  return sensor1.getTempCByIndex(0);
}


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
  
/////    температура из модуля часов
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
  _T_Rastopka = 26;     //55
  _T_Zavershenie = 25;  //50
  _T_Stop = 24;         //30
  _T_Avariya = 30;      //85

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

//Конвертация миллисекунд 
void millisToStr(long int mil, char * str){
  int h, m, s;
  h = abs(mil/1000/3600); // целые часы
  m = abs((mil/1000 - h*3600) / 60); // целые минуты
  s = abs((mil/1000 - h*3600 - m*60) );

  snprintf(str, 10, "%02d:%02d:%02d",h, m, s); 
}


//вывод на дисплей
void WriteToDisplay(){
  char s0[16];
  char s1[16];

  snprintf(s0, 16, "%s T-%d.%d V-%02d",_Time, int(_T1),int((_T1 - int(_T1))*10 )); 
  snprintf(s1, 16, "%s  N-%d V-%d", _Rezhim[_Iteracia] ,   _N1, _V1); 

  lcd.setCursor(0,0); lcd.print(s0);
  lcd.setCursor(0,1); lcd.print(s1);


  switch (_Iteracia){  //режимы работы
    case 1: //Старт, розжиг
    //lcd.setCursor(0,1); lcd.print("N-0MM");lcd.setCursor(5,1);lcd.print("V-");lcd.print(_V1_rastopka);

    break;
    case 2: //Работа
    //lcd.setCursor(0,1); lcd.print("N-*");lcd.setCursor(5,1);lcd.print("V-");lcd.print(_V1_rabota);

    break;
    case 3: // Стоп
    //lcd.setCursor(0,1); lcd.print("N-*");lcd.setCursor(5,1);lcd.print("V-");lcd.print(_V1_zavershenie);

    break;
    case 4:
    //lcd.setCursor(0,1); lcd.print("N-*");lcd.setCursor(5,1);lcd.print("V-");lcd.print(0);

    break;
    case 5:
    //lcd.setCursor(0,1); lcd.print("N-0");lcd.setCursor(5,1);lcd.print("V-");lcd.print(0);

    break;
  }


//lcd.setCursor(10,0);lcd.print(_Rezhim[_Iteracia]);
//lcd.setCursor(10,1);lcd.print("T-");lcd.print(_T1);
}


//************   Инициализация    **********
void setup() {
  // put your setup code here, to run once:
  // put your setup code here, to run once:  
  //Serial.begin(9600);
  lcd.init();
  lcd.backlight();   
  
  pinMode(13, OUTPUT);
  pinMode(V1_Pin, OUTPUT); //0-255  меняет 0-5 В analogWrite(ledPin, brightness);
  pinMode(N1_Pin, OUTPUT); //analogWrite(Pin, Value 0-255);

  //putDefoult_DATA();  // записать дефолтные данные
  getEEPROM_DATA();
  print_Prefference();

  // Start DALLAS
  _sensor1.begin();

  //
  __t1 = millis();
  __t2 = __t1;  
}


//************   Основной цикл    ************
void loop() {
  // put your main code here, to run repeatedly:

  _Count++;
  __t2 = millis();
  if((__t2 - __t1)> 1000){
    WriteToDisplay();  

    
    __t1 = __t2;
    _Count = 0;

    _T1 = GetTempetarure(_sensor1);
  }

  
//Запрос текущего времени
//getDateDs1307(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);
//char time[10];  char data[11];
//snprintf(time, sizeof(time),"%02d:%02d:%02d",hour, minute, second);
//snprintf(data, sizeof(data), "%02d/%02d/%02d",dayOfMonth, month, year);
//lcd.setCursor(0,0); lcd.print(data);
//lcd.setCursor(0,1); lcd.print(time);


_m = millis();
millisToStr(_m, _Time);

//_T1 = GetTempetarure(_sensor1);
//_T1 = get3231Temp();


switch (_Iteracia){  //режимы работы
  case 1: //Старт

     if (_T1 >= _T_Rastopka)
     { //Если температура вылосла до 60 градусов
       _Iteracia = 2;
       _m_rastopka = _m;
     }
     else
     {
      analogWrite(V1_Pin, _V1_rastopka); _V1 = _V1_rastopka;//включить вентилятор
      digitalWrite(N1_Pin, LOW); _N1 = 0; // выключить насос
      
     }

  break;
  case 2: //Работа
     if (_T1 >= _T_Avariya)
     { //Если температура вылосла до 85 градусов
       _Iteracia = 4;  //АВАРИЯ
       _m_Avariya = _m;
       break;
     }

     if (_T1 <= _T_Zavershenie)
     { //Если температура упала до 50 градусов
       _Iteracia = 3; // Топливо сгорело, перейти к этапу окончания
       _m_zavershenie = _m;
       break;
     }
     
     //если температура в норме то работаем...
     analogWrite(V1_Pin, _V1_rabota); _V1 = _V1_rabota;// включить вентилятор
     digitalWrite(N1_Pin, HIGH);  _N1 = 1;//  насос +
     
  break;
  case 3: // Стоп
      if (_T1 <= _T_Stop){ //Если температура уменьшилась ниже 30 градусов
        analogWrite(V1_Pin, 0); _V1 = 0;   // включить вентилятор
        digitalWrite(N1_Pin, LOW); _N1 = 0; // выключить насос    
        _Iteracia = 5;
        _m_Stop = _m;        
      }
      else{  // иначе поддердивать режим догорания топлива
        analogWrite(V1_Pin, _V1_zavershenie); _V1 = _V1_zavershenie;// включить вентилятор
        digitalWrite(N1_Pin, HIGH);    _N1 = 1;//  насос  +
        
      }

  break;
    case 4: //авария
      analogWrite(V1_Pin, 0);   _V1 = 0;  // выключить вентилятор
      digitalWrite(N1_Pin, HIGH); _N1 = 1;// включить насос  
      

    break;
    case 5: // Покой
      analogWrite(V1_Pin, 0);  _V1 = 0;   // выключить вентилятор
      digitalWrite(N1_Pin, LOW); _N1 = 0;// включить насос  
      _m_Start = _m;
      
    break;
}

  
}
