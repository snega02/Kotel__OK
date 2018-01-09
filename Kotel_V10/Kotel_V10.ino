//Термометр подключение
//провод USB чёрный, белый красный

//V7 Контроллер упраления котлом 1 канал.ino
//V8 меню обзора значений работы котла
//V9 Переделка под обычный дисплей
//V10 багфикс

#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <EEPROM.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Bounce.h>
#include <LiquidCrystal.h>

// Data wire is plugged into port 2 DALLAS temperature
#define ONE_WIRE_BUS 6  // термометр
#define PIN_NASOS 16  //A2  - насос котловой
#define PIN_NASOS_2 17  //A3  - насос отопления 1
#define PIN_VENT 13
#define BUTTON_LEFT 7
#define BUTTON_RIGHT 10
#define BUTTON_UP 9
#define BUTTON_DOWN 8

// Обращение к ним идет по номерам от 14 (для аналогового входа 0) до 19 (для аналогового входа 5).


OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature _sensor1(&oneWire);

LiquidCrystal lcd(12, 11, 5, 4, 3, 2);


//**********   Константы и структуры    *********************
const int V1_Pin = 13;      // ПИН Вентилятора
const int N1_Pin = 16;      // ПИН Насоса
const int N2_Pin = 17;      // ПИН Насоса

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
int _Count = -3000;  int __t1; int __t2; bool _b_pref_change = false;

int _V1_rastopka = 0; int _V1_rabota = 0; int _V1_zavershenie = 0; int _N1; int _V1; //скорость работы вентилятора в процентах

int _Iteracia;// 1-старт, 2-работы, 3-стоп, 4-авария, 5 - покой
float _T1 = 20; int _T_Rastopka = 0; int _T_Zavershenie = 0; int _T_Stop = 0; int _T_Avariya = 0;


byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
char week[8][10] = {"Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"};

char _Rezhim[6][6] = {"","Start", "Work ", "Stop ", "Alarm", "Sleep"};


// Миллисекунды
unsigned long _m[5];
char _Time[10]; char _Time_rastopka[10];  char _Time_rabota[10]; char _Time_zavershenie[10]; char _Time_Avariya[10]; char _Time_Stop[10];  // время сработки кождой итерации

//переменные для отсчёта границ секунд
int  t1; int t2; int ts1 = 0; int ts2 = 0;

float _dT, _kT;


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
  if (bLeft.read() == HIGH)
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
  char ch[17];  
  snprintf(ch, 17, "%d         ", *mMenu[curMenu].val); 

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
  _T_Rastopka = 50;     //55
  _T_Zavershenie = 45;  //50
  _T_Stop = 40;         //30
  _T_Avariya = 85;      //85

 putEEPROM_DATA(); 
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
  char s0[17];
  char s1[17];

  snprintf(s0, 17, "%s T-%d.%d V-%02d",_Time, int(_T1),int((_T1 - int(_T1))*10 )); 
  snprintf(s1, 17, "%s  N-%d V-%d ", _Rezhim[_Iteracia] ,   _N1, _V1); 

  lcd.setCursor(0,0); lcd.print(s0);
  lcd.setCursor(0,1); lcd.print(s1);
}


void mainFunc()
{
int _old_Iteracia = _Iteracia;
switch (_Iteracia){  //режимы работы
  case 1: //Старт

     if (_T1 >= _T_Rastopka)
     { //Если температура вылосла до 60 градусов
       _Iteracia = 2;       
     }
     else
     {
      _V1 = _V1_rastopka;//включить вентилятор
      _N1 = 1; // включить насос
      
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
     _V1 = _V1_rabota;// включить вентилятор
     //_V1 = _V1 * (_T_Avariya - _T1
     _N1 = 1;//  насос +

     if (_T1 + 10 > _T_Avariya){ // если приближается к аварии 
        // снизить обороры вентирятора пропорционально
        _V1 = int(_V1 * _kT);  //перерасчет вращения от температуры
             
     } 
     
  break;
  case 3: // Стоп
      if (_T1 <= _T_Stop){ //Если температура уменьшилась ниже 30 градусов
         _V1 = 0;   // включить вентилятор
         _N1 = 0; // выключить насос    
        _Iteracia = 5;
      }
      else{  // иначе поддердивать режим догорания топлива
        _V1 = _V1_zavershenie;// включить вентилятор
        _N1 = 1;//  насос  +
        
      }

  break;
    case 4: //авария
      _V1 = 0;  // выключить вентилятор
      _N1 = 1;// включить насос  

      //если температура опустилась, попробовать продолжить работу
      if(_T_Avariya-_T1 > 10) _Iteracia = 2;
      

    break;
    case 5: // Покой
      _V1 = 0;   // выключить вентилятор
      if (_T1 < 35)  //если температура воды больше 35
          _N1 = 0;// выключить насос  
      else
          _N1 = 1;
            
    break;
}

if(_old_Iteracia != _Iteracia)   {
     _b_pref_change = true;
     if (_Iteracia == 1)
          _m[1] = millis();
     else
          _m[_Iteracia] = millis() - _m[1];
}
}

//Вывести состояние настроек
void print_Prefference(){
  Serial.println();
  Serial.print("_Iteracia =       "); Serial.println(_Iteracia);
  Serial.print("_V1_rastopka =    "); Serial.println(_V1_rastopka);
  Serial.print("_V1_rabota =      "); Serial.println(_V1_rabota);
  Serial.print("_V1_zavershenie = "); Serial.println(_V1_zavershenie);
  Serial.print("_V1 =             "); Serial.println(_V1);
  Serial.print("_T_Rastopka =     "); Serial.println(_T_Rastopka);
  Serial.print("_T_Zavershenie =  "); Serial.println(_T_Zavershenie);
  Serial.print("_T_Stop =         "); Serial.println(_T_Stop);
  Serial.print("_T_Avariya =      "); Serial.println(_T_Avariya);
  Serial.print("_kT        =      "); Serial.print("  ");Serial.println(_V1 * _kT);
  Serial.print("_kT        =      "); Serial.print("  ");Serial.println(_kT);
  
}

//************   Инициализация    **********
void setup() {
  lcd.begin(16, 2);
  //Serial.begin(9600);
  //print_Prefference();  //вывод информации в серийный порт

  _T1 = GetTempetarure(_sensor1);  
  _m[1] = millis();
  millisToStr(_m[1], _Time);  
  
  //Пины вентилятора и насоса
  pinMode(13, OUTPUT);
  pinMode(V1_Pin, OUTPUT); //0-255  меняет 0-5 В analogWrite(ledPin, brightness);
  pinMode(N1_Pin, OUTPUT); //analogWrite(Pin, Value 0-255);
  pinMode(N2_Pin, OUTPUT); 

  //инициализация пинов кнопок уаравления меню
  pinMode( BUTTON_LEFT, INPUT );
  pinMode( BUTTON_UP, INPUT );
  pinMode( BUTTON_DOWN, INPUT );
  pinMode( BUTTON_RIGHT, INPUT );

  putDefoult_DATA();  // записать дефолтные данные
  getEEPROM_DATA();  

  // Start DALLAS
  _sensor1.begin();

  //
  __t1 = millis();
  __t2 = __t1;  
  //_Iteracia = 5;

  WriteToDisplay();  
}


//************   Основной цикл    ************
void loop() {


  _Count++;


  /////   ***    упрравление скоростью вентилятора   *****
  //0.1 секунда
  ts2 = millis();
  if (ts2-ts1 > 100){
    ts1 = millis(); t1 = millis();
    if (_Iteracia != 5)
           digitalWrite(PIN_VENT, HIGH);

  }
//на спадающем фронте проверить 5 сек
  __t2 = millis();
  //  5 секунды
  if((__t2 - __t1)> 2000){
    if (!b_ShowmMenu) WriteToDisplay();  

      __t1 = __t2;
    _Count = 0;

    //получить тек температуру  
    _T1 = GetTempetarure(_sensor1);  
    millisToStr(_m[1], _Time);
    //пересчитать коэфициент для вентилятора
    _dT = _T_Avariya - _T_Rastopka;   // дельта темп. аларма и работы
    _kT =(_dT - _T1 + _T_Rastopka)/_dT; // коэфициент корректировки вентилятора
    if (_kT > 1) _kT = 1;
  
    ////    ***    выполнить главную функцию    *****  
    mainFunc(); 
    //выключние отключение насоса
    if (_N1)
          analogWrite(A2, 1024);
          else
          analogWrite(A2,0);
  }

//отключение вентилятора после пройденого % времени
t2 = millis();
if (t2-t1 > _V1){
    digitalWrite(PIN_VENT, LOW);   // turn the LED on (HIGH is the voltage level) 
}  



//   вывод меню
  if (b_ShowmMenu) {
    menu();
  }else{
    if ( bLeft.update() )
      if ( bLeft.read() )
         b_ShowmMenu = 1;  
  }


//   введение изменений на ходу
if (!b_ShowmMenu){  //Если показан основной экран

  // ++ к занчению вентилятора тек итерации
  if (_Iteracia <= 3)
  {
  if ( bUp.update() )
      if ( bUp.read() ){
         (*mMenu[_Iteracia - 1].val)++;  
         _b_pref_change = true;
       }
  // -- к занчению вентилятора тек итерации
  if ( bDown.update() )
      if ( bDown.read() ){
         (*mMenu[_Iteracia - 1].val)--;  
         _b_pref_change = true;
       }
  }


  if ( bRight.update() )
      if ( bRight.read() ){
        if (_Iteracia == 5) //не Покой
           _Iteracia = 1;
           else
           _Iteracia = 5;
        _b_pref_change = true;
      }
}

if (_b_pref_change) { 
  putEEPROM_DATA(); 
  //print_Prefference(); // раккоменнтировать если надо делать дебаг настроек через ком порт
  _b_pref_change = false;};
}
