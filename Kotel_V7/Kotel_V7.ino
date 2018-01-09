
//Контроллер упраления котлом 1 канал.ino
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <EEPROM.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Bounce.h>

// Data wire is plugged into port 2 DALLAS temperature
#define ONE_WIRE_BUS 2
#define PIN_NASOS 3
#define PIN_VENT 13
#define BUTTON_LEFT 9
#define BUTTON_RIGHT 12
#define BUTTON_UP 10
#define BUTTON_DOWN 11

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

int _V1_rastopka = 0; int _V1_rabota = 0; int _V1_zavershenie = 0; int _N1; int _V1; //скорость работы вентилятора в процентах

int _Iteracia;// 1-старт, 2-работы, 3-стоп, 4-авария, 5 - покой
float _T1 = 20; int _T_Rastopka = 0; int _T_Zavershenie = 0; int _T_Stop = 0; int _T_Avariya = 0;


byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
char week[8][10] = {"Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"};

char _Rezhim[6][6] = {"","Start", "Work ", "Stop ", "Alarm", "Sleep"};


// Миллисекунды
unsigned long _m, _m_rastopka, _m_rabota, _m_zavershenie, _m_Avariya, _m_Stop, _m_Start;
char _Time[10]; char _Time_rastopka[10];  char _Time_rabota[10]; char _Time_zavershenie[10]; char _Time_Avariya[10]; char _Time_Stop[10];  // время сработки кождой итерации

//переменные для отсчёта границ секунд
int  t1; int t2; int ts1 = 0; int ts2 = 0;


//Переменные Меню
//текущий пунет меню,  флаг отображения меню
int curMenu = 0; bool b_ShowmMenu = 0;
//кол пунктов меню
const int CountMenu = 7;

//создание кнопок через класс Bounce
Bounce bLeft = Bounce( BUTTON_LEFT, 5 ); Bounce bRight = Bounce( BUTTON_RIGHT, 5 ); Bounce bUp = Bounce( BUTTON_UP, 5 ); Bounce bDown = Bounce( BUTTON_DOWN, 5 ); 

//массив элементов меню
struct MENU_ITEM{ 
  char name[17];
  int *val;
};


//инициализация меню
//строковое имя, адрес переменной которую надо менять
MENU_ITEM mMenu[CountMenu]= {
"V1_rastopka   1", &_V1_rastopka,
"V1_rabota     2", &_V1_rabota,
"V1_zavershen  3", &_V1_zavershenie,
"T_Rastopka    4", &_T_Rastopka,
"T_Zavershenie 5", &_T_Zavershenie,
"T_Stop        6", &_T_Stop,
"T_Avariya     7", &_T_Avariya
};


//**********     ФУНКЦИИ                *********************
//функия выполнения меню
void menu(){
 //  bRight  //след пункт меню по кругу
if (bRight.update())
  if (bRight.read() == HIGH)
    if (curMenu == CountMenu - 1)
      curMenu = 0;
         else   curMenu++;

//  bLeft  выход из меню
if (bLeft.update())
  if (bLeft.read() == HIGH)0
    b_ShowmMenu = 0;
    
//  bUp  +1 значение
if (bUp.update())
  if (bUp.read() == HIGH)
    (*mMenu[curMenu].val)++;

  
 //  bDown -1 зачение
if (bDown.update())
  if (bDown.read() == HIGH)
    (*mMenu[curMenu].val)--;


  //вывод использую буффур
  char ch[16];  
  snprintf(ch, 16, "%d         ", *mMenu[curMenu].val); 

  lcd.setCursor(0,0); lcd.print(mMenu[curMenu].name);
  lcd.setCursor(0,1); lcd.print(ch); 
}


// Запрос температуры
float GetTempetarure(DallasTemperature sensor1)
{
  //sensor1.requestTemperaturesByIndex;
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
}


void mainFunc()
{
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


//************   Инициализация    **********
void setup() {
  // put your setup code here, to run once:
  // put your setup code here, to run once:  
  //Serial.begin(9600);
  lcd.init();
  lcd.backlight();   

  _T1 = GetTempetarure(_sensor1);  
  _m = millis();
  millisToStr(_m, _Time);  
  
  //Пины вентилятора и насоса
  pinMode(13, OUTPUT);
  pinMode(V1_Pin, OUTPUT); //0-255  меняет 0-5 В analogWrite(ledPin, brightness);
  pinMode(N1_Pin, OUTPUT); //analogWrite(Pin, Value 0-255);

  //инициализация пинов кнопок уаравления меню
  pinMode( BUTTON_LEFT, INPUT );
  pinMode( BUTTON_UP, INPUT );
  pinMode( BUTTON_DOWN, INPUT );
  pinMode( BUTTON_RIGHT, INPUT );

  //putDefoult_DATA();  // записать дефолтные данные
  getEEPROM_DATA();
  print_Prefference();

  // Start DALLAS
  _sensor1.begin();

  //
  __t1 = millis();
  __t2 = __t1;  
  _Iteracia = 1;

  WriteToDisplay();  
}


//************   Основной цикл    ************
void loop() {


  _Count++;



  //0.1 секунда
  ts2 = millis();
  if (ts2-ts1 > 100){
    ts1 = millis(); t1 = millis();
  //if (_V1 > 0)
         digitalWrite(PIN_VENT, HIGH);

  }

t2 = millis();
if (t2-t1 > _V1){
    digitalWrite(PIN_VENT, LOW);   // turn the LED on (HIGH is the voltage level) 

//на спадающем фронте проверить 5 сек
  __t2 = millis();
  //  5 секунды
  if((__t2 - __t1)> 5000){
    if (!b_ShowmMenu) WriteToDisplay();  

      __t1 = __t2;
    _Count = 0;

    _T1 = GetTempetarure(_sensor1);  
    _m = millis();
    millisToStr(_m, _Time);

    mainFunc();  
    
  }
}  


//вывод меню
  if (b_ShowmMenu) {
    menu();
  }else{
    if ( bLeft.update() )
      if ( bLeft.read() )
         b_ShowmMenu = 1;  
  }




//_T1 = GetTempetarure(_sensor1);
//_T1 = get3231Temp();  

//Запрос текущего времени
//getDateDs1307(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);
//char time[10];  char data[11];
//snprintf(time, sizeof(time),"%02d:%02d:%02d",hour, minute, second);
//snprintf(data, sizeof(data), "%02d/%02d/%02d",dayOfMonth, month, year);
//lcd.setCursor(0,0); lcd.print(data);
//lcd.setCursor(0,1); lcd.print(time);

}
