#include <Arduino.h>
#include <U8x8lib.h>
#include "OneButton.h"
#include "A4988.h"
#include <Ticker.h>
#include <EEPROM.h>
#include <avr/eeprom.h>
#include "extra.h"
OneButton btn_up(18, true);
OneButton btn_center(15, true);
OneButton btn_left(14, true);
OneButton btn_down(16, true);
OneButton btn_right(10, true);


#define RPM 30
#define MICROSTEPS 16
#define REDUCTION_RATIO 4.12f
#define MOTOR_STEPS 200*REDUCTION_RATIO
#define ADC_DIVIDER 4.24f

#define DIR 9
#define STEP 8
#define PIN_SHUTTER 6 //todo:
#define PIN_FOCUS 5   //todo:
#define PIN_ADC 4
const uint16_t SETTLE_DELAY = 500;
const uint8_t FOCUS_DELAY = 100;
const uint16_t SHUTTER_DELAY = 300;
const uint8_t EXTRA_DELAY = 50;

#define LEFT 0
#define RIGHT 1

U8X8_SSD1306_128X32_UNIVISION_HW_I2C u8x8(/* reset=*/U8X8_PIN_NONE);
A4988 stepper(MOTOR_STEPS, DIR, STEP);

struct guiS
{
  uint8_t current = 0;
  uint8_t previous = 0;
  uint8_t menu_id = 0;
};
guiS SCREEN;

uint8_t CURRENT_PRESET = 0;

uint32_t movedur;
uint32_t elapsed_time;
uint32_t remains_time;
struct dollyS
{
  uint16_t Shots_total;
  volatile uint16_t Shots_remains;
  uint8_t Interval;
  uint8_t countdown;
  const uint8_t Delay = 5;
  uint8_t Exposure;
  uint8_t Angle;
  float AngleStep;
  bool Direction; //0 - влево 1 - вправо
  bool isHoming;
  volatile boolean started=false;
};
dollyS dolly;

struct ADCS
{
  uint32_t voltage_raw;
  uint8_t num_readings;
  uint16_t battery_voltage;
};
ADCS adc;

uint16_t EEMEM SHOTS_preset1_addr;
uint8_t EEMEM INTERVAL_preset1_addr;
uint8_t EEMEM EXPOSURE_preset1_addr;
uint8_t EEMEM ANGLE_preset1_addr;

uint16_t EEMEM SHOTS_preset2_addr;
uint8_t EEMEM INTERVAL_preset2_addr;
uint8_t EEMEM EXPOSURE_preset2_addr;
uint8_t EEMEM ANGLE_preset2_addr;

uint8_t EEMEM CURRENT_PRESET_addr;

void ReadButtons(void)
{
  btn_up.tick();
  btn_center.tick();
  btn_left.tick();
  btn_down.tick();
  btn_right.tick();

  if (adc.num_readings < 100)
  {
    adc.num_readings++;
    adc.voltage_raw += analogRead(PIN_ADC);
  }
  else
  {
    adc.battery_voltage = adc.voltage_raw / adc.num_readings;
    adc.num_readings = 0;
    adc.voltage_raw = 0;
  }
}

void ReadSettings(void)
{
  EEPROM.get((int)&CURRENT_PRESET_addr, CURRENT_PRESET);
  switch (CURRENT_PRESET)
  {
  case 0:
  {
    EEPROM.get((int)&SHOTS_preset1_addr, dolly.Shots_total);
    EEPROM.get((int)&INTERVAL_preset1_addr, dolly.Interval);
    EEPROM.get((int)&EXPOSURE_preset1_addr, dolly.Exposure);
    EEPROM.get((int)&ANGLE_preset1_addr, dolly.Angle);
  }
  break;

  case 1:
  {
    EEPROM.get((int)&SHOTS_preset2_addr, dolly.Shots_total);
    EEPROM.get((int)&INTERVAL_preset2_addr, dolly.Interval);
    EEPROM.get((int)&EXPOSURE_preset2_addr, dolly.Exposure);
    EEPROM.get((int)&ANGLE_preset2_addr, dolly.Angle);
  }
  break;

  default:
    break;
  }
}

void SaveSettings(void)
{
  Serial.println("Saving EEPROM");
  EEPROM.put((int)&CURRENT_PRESET_addr, CURRENT_PRESET);
  switch (CURRENT_PRESET)
  {
  case 0:
  {
    Serial.println("Saving Preset 0");
    EEPROM.put((int)&SHOTS_preset1_addr, dolly.Shots_total);
    EEPROM.put((int)&INTERVAL_preset1_addr, dolly.Interval);
    EEPROM.put((int)&EXPOSURE_preset1_addr, dolly.Exposure);
    EEPROM.put((int)&ANGLE_preset1_addr, dolly.Angle);
  }
  break;
  case 1:
  {
     Serial.println("Saving Preset 1");
    EEPROM.put((int)&SHOTS_preset2_addr, dolly.Shots_total);
    EEPROM.put((int)&INTERVAL_preset2_addr, dolly.Interval);
    EEPROM.put((int)&EXPOSURE_preset2_addr, dolly.Exposure);
    EEPROM.put((int)&ANGLE_preset2_addr, dolly.Angle);
  }
  break;
  default:
    break;
  }
 Serial.println("EEPROM SAVE Done!");
}

void click_up()
{
  if (SCREEN.menu_id > 0)
    SCREEN.menu_id--;
  else
    SCREEN.menu_id = 3;
  Serial.println("Button DOWN click.");
  Serial.println(SCREEN.menu_id);
} // click1

void click_down()
{
  if (SCREEN.menu_id < 3)
    SCREEN.menu_id++;
  else
    SCREEN.menu_id = 0;
  Serial.println("Button UP click.");
  Serial.println(SCREEN.menu_id);
} // click1

void click_left()
{
  Serial.println("Button LEFT click.");
  switch (SCREEN.menu_id)
  {
  case 0:
    if (SCREEN.current == 1)
    {
      if (dolly.Shots_total > 2)
        dolly.Shots_total--;
      else
        dolly.Shots_total = 300;
      u8x8.clearLine(SCREEN.menu_id);
    }
    else if (SCREEN.current == 2)
    {
      if (CURRENT_PRESET > 1)
        CURRENT_PRESET--;
    }
    break;
  case 1:
    if (dolly.Interval > 2)
      dolly.Interval--;
    else
      dolly.Interval = 250;
    u8x8.clearLine(SCREEN.menu_id);
    break;

  case 2:
    if (dolly.Exposure > 2)
      dolly.Exposure--;
    else
      dolly.Exposure = 60;
    u8x8.clearLine(SCREEN.menu_id);
    break;

  case 3:
    if (dolly.Angle > 2)
      dolly.Angle--;
    else
      dolly.Angle = 250;
    u8x8.clearLine(SCREEN.menu_id);
    break;
  default:
    break;
  }
} // click1

void click_right()
{
  Serial.println("Button RIGHT click.");
  switch (SCREEN.menu_id)
  {
  case 0:
    if (SCREEN.current == 1)
    {
      if (dolly.Shots_total < 400)
        dolly.Shots_total++;
      else
        dolly.Shots_total = 30;
    }
    else if (SCREEN.current == 2)
      if (CURRENT_PRESET < 1)
        CURRENT_PRESET++;
    u8x8.clearLine(SCREEN.menu_id);
    break;
  case 1:
    if (dolly.Interval < 250)
      dolly.Interval++;
    else
      dolly.Interval = 1;
    u8x8.clearLine(SCREEN.menu_id);
    break;

  case 2:
    if (dolly.Exposure < 60)
      dolly.Exposure++;
    else
      dolly.Exposure = 1;
    u8x8.clearLine(SCREEN.menu_id);
    break;

  case 3:
    if (dolly.Angle < 180)
      dolly.Angle++;
    else
      dolly.Angle = 1;
    u8x8.clearLine(SCREEN.menu_id);
    break;
  default:
    break;
  }
} // click1

void click_center()
{
  Serial.println("Button CENTER click.");
  if (SCREEN.current < 2)
    SCREEN.current++;
  else
    SCREEN.current = 0;
  if (SCREEN.current != SCREEN.previous)
  {
    u8x8.clearDisplay();
    SCREEN.previous = SCREEN.current;
  }
  SCREEN.menu_id = 0;
  Serial.println(SCREEN.current);
} // click1

void stepmove(void)
{
  uint32_t movestart = millis();
  Serial.print("MOVING: ");
  if (dolly.Direction == RIGHT)
    stepper.rotate(-dolly.AngleStep);
  else if (dolly.Direction == LEFT)
    stepper.rotate(dolly.AngleStep);
  movedur = millis() - movestart;
  Serial.print("Move time: ");
  Serial.println(movedur);
}
/*
Долгое нажатие на центральную клавишу запускает съемку либо останавливает ее
*/
void long_click_center(void)
{
  if (!dolly.started)
  {
    SaveSettings(); //сохранение настроек в EEPROM
    SCREEN.current = 3;
    u8x8.clearDisplay();
    Serial.println("Status: Start");
    dolly.started = true;
    dolly.countdown = dolly.Delay;                     //выставляется первичная задержка запуска
    dolly.AngleStep = dolly.Angle / dolly.Shots_total; //вычисление на какой градус поворачивать голову между кадрами
    dolly.Shots_remains = dolly.Shots_total;
    elapsed_time = 0;
    stepmove();
    remains_time = dolly.Delay * 1000 + (dolly.Exposure + dolly.Interval) * dolly.Shots_total * 1000 + movedur * dolly.Shots_total;
    //todo: 1.38 Стабильная погрешность. ХЗ откуда она взялась.
    Serial.print("dolly.Delay: ");
    Serial.println(dolly.Delay);
    Serial.print("dolly.Exposure: ");
    Serial.println(dolly.Exposure);
    Serial.print("dolly.Interval: ");
    Serial.println(dolly.Interval);
    Serial.print("dolly.Shots_total: ");
    Serial.println(dolly.Shots_total);
    Serial.print("movedur: ");
    Serial.println(movedur);
    Serial.print("remains_time: ");
    Serial.println(remains_time);
    Serial.println("HOMING");
    dolly.isHoming = true;        //камера ставится в центр кадра. И перед началом съемки
    if (dolly.Direction == RIGHT) //поворачивается на половину заданного угла в противоположном направлении.
      stepper.rotate(dolly.Angle / 2);
    else if (dolly.Direction == LEFT)
      stepper.rotate(-dolly.Angle / 2);
    dolly.isHoming = false;
  }
  else
  {
    dolly.started = false;
    Serial.println("Status: Stop");
  }
}

void shot(void)
{
  uint32_t t1, t2;
  uint8_t i = 0;
  Serial.println("Focus pressed");
  u8x8.drawString(14, 0, "F+");
  digitalWrite(PIN_FOCUS, HIGH); //активация автофокуса
  delay(FOCUS_DELAY);            //пауза между фокусировкой и активацией затвора 0,1с
  elapsed_time += FOCUS_DELAY;
  remains_time -= FOCUS_DELAY;
  Serial.println("Shutter pressed");
  u8x8.drawString(14, 0, "S");
  digitalWrite(PIN_SHUTTER, HIGH); //активация затвора
  delay(SHUTTER_DELAY);
  elapsed_time += SHUTTER_DELAY;
  remains_time -= SHUTTER_DELAY;
  digitalWrite(PIN_SHUTTER, LOW); //Деактивация затвора
  Serial.println("Shutter released");
  u8x8.drawString(15, 0, "-");
  digitalWrite(PIN_FOCUS, LOW); //деактивация автофокуса
  Serial.println("Focus released");
  Serial.println("Wait dolly for settle");
  u8x8.drawString(14, 0, "F");
  delay(SETTLE_DELAY); // пауза 0,5 секундная для того чтобы прекратилась тряска после движения
  elapsed_time += SETTLE_DELAY;
  remains_time -= SETTLE_DELAY;
  if (dolly.Exposure == 1) // если выдержка 1 то
  {
    delay(abs(1000 - SETTLE_DELAY - FOCUS_DELAY - SHUTTER_DELAY)); // доп задержка чтобы процедура выполнения кадра уложилась в 1С
    elapsed_time += abs(1000 - SETTLE_DELAY - FOCUS_DELAY - SHUTTER_DELAY);
    remains_time -= abs(1000 - SETTLE_DELAY - FOCUS_DELAY - SHUTTER_DELAY);
    Serial.print("Extra delay: ");
    Serial.println(1000 - SETTLE_DELAY - FOCUS_DELAY - SHUTTER_DELAY);
    u8x8.drawString(0, 0, "ELAPSED");
    u8x8.setCursor(9, 0);
    u8x8.print(dolly.Shots_total - dolly.Shots_remains);
    u8x8.drawString(0, 1, TimeToString(elapsed_time));
    u8x8.drawString(0, 2, "REMAINS");
    u8x8.setCursor(9, 2);
    u8x8.print(dolly.Shots_remains);
    if (dolly.Shots_remains < 10)
    {
      u8x8.setCursor(10, 2);
      u8x8.print(" ");
    }
    u8x8.drawString(0, 3, TimeToString(remains_time));
  }
  else
  {
    t1 = millis();
    t2 = t1;
    while (i != dolly.Exposure)
      if (t2 - t1 > 1000)
      {
        Serial.print("Exposure: ");
        Serial.println(dolly.Exposure - i - 1);
        u8x8.clearLine(3);
        u8x8.drawString(0, 3, "EXPOSURE");
        u8x8.setCursor(9, 3);
        u8x8.print(dolly.Exposure - i - 1);
        i++;
        elapsed_time += 1000;
        remains_time -= 1000;
        t1 = t2;
        u8x8.drawString(0, 0, "ELAPSED");
        u8x8.setCursor(9, 0);
        u8x8.print(dolly.Shots_total - dolly.Shots_remains);
        u8x8.drawString(0, 1, TimeToString(elapsed_time));
        u8x8.drawString(0, 2, "REMAINS");
        u8x8.setCursor(9, 2);
        u8x8.print(dolly.Shots_remains);
        if (dolly.Shots_remains < 10)
        {
          u8x8.setCursor(10, 2);
          u8x8.print(" ");
        }
        u8x8.drawString(0, 3, TimeToString(remains_time));
      }
      else
        t2 = millis();
  }
  Serial.print("Exporure ended: ");
  Serial.println(millis(), 1);
}

void EverySecond(void)
{
  if (dolly.started && !dolly.isHoming) //начало запуска интервалометра
  {
    elapsed_time += 1000;
    remains_time -= 1000;
    if (dolly.Shots_remains != 0) //если еще не все кадры сделаны
    {
      if (dolly.countdown != 0) //Отсчет времени одного кадра
      {
        Serial.print("Countdown: ");
        dolly.countdown--;
        Serial.println(dolly.countdown);
        if (dolly.countdown < 10)
        {
          u8x8.setCursor(14, 0);
          u8x8.print(" ");
          u8x8.setCursor(15, 0);
          u8x8.print(dolly.countdown);
        }
        else
        {
          u8x8.setCursor(14, 0);
          u8x8.print(dolly.countdown);
        }
      }
      else
      {             //пора де лать кадр
        shot();     //сначала первый кадр с домашней позиции
        stepmove(); //сдвиг на следующую позицию
        dolly.countdown = dolly.Interval;
        dolly.Shots_remains--;
      }
    }
    else
    {
      shot(); //последний кадр на финальной позиции
      dolly.started = false;
      dolly.Shots_remains = dolly.Shots_total;
      Serial.println("Dolly stopped");
      u8x8.clearLine(3);
      u8x8.drawString(0, 3, "STOPPED");
    }
  }
}

void DrawGui(void)
{
  u8x8.setFont(u8x8_font_saikyosansbold8_u);
  switch (SCREEN.current)
  {
  case 0:
    u8x8.drawString(0, 0, "IDLE");
    u8x8.drawString(0, 1, "LONG PRESS TO");
    u8x8.drawString(0, 2, "START");
    u8x8.drawString(0, 3, "VCC");
    u8x8.setCursor(9, 3);
    u8x8.print(5 * adc.battery_voltage / 1023.0f * ADC_DIVIDER);
    break;
  case 1:
    if (SCREEN.menu_id == 0)
      u8x8.setInverseFont(1);
    else
      u8x8.setInverseFont(0);
    u8x8.drawString(0, 0, "N  SHOTS");
    u8x8.setCursor(9, 0);
    u8x8.setInverseFont(0);
    u8x8.print(dolly.Shots_total);
    if (SCREEN.menu_id == 1)
      u8x8.setInverseFont(1);
    else
      u8x8.setInverseFont(0);
    u8x8.drawString(0, 1, "INTERVAL");
    u8x8.setCursor(9, 1);
    u8x8.setInverseFont(0);
    u8x8.print(dolly.Interval);
    if (SCREEN.menu_id == 2)
      u8x8.setInverseFont(1);
    else
      u8x8.setInverseFont(0);
    u8x8.drawString(0, 2, "EXPOSURE");
    u8x8.setCursor(9, 2);
    u8x8.setInverseFont(0);
    u8x8.print(dolly.Exposure);
    if (SCREEN.menu_id == 3)
      u8x8.setInverseFont(1);
    else
      u8x8.setInverseFont(0);
    u8x8.drawString(0, 3, "ANGLE");
    u8x8.setCursor(9, 3);
    u8x8.setInverseFont(0);
    u8x8.print(dolly.Angle);
    break;
  case 2:
    if (SCREEN.menu_id == 0)
      u8x8.setInverseFont(1);
    else
      u8x8.setInverseFont(0);
    u8x8.drawString(0, 0, "PRESET");
    u8x8.setCursor(9, 0);
    u8x8.setInverseFont(0);
    u8x8.print(CURRENT_PRESET);
    /*
    if (menu_id==1) u8x8.setInverseFont(1); else u8x8.setInverseFont(0);
    u8x8.drawString(0,1,"INTERVAL");
    u8x8.setCursor(9,1);
    u8x8.setInverseFont(0);
    u8x8.print(dolly.Interval);
    if (menu_id==2) u8x8.setInverseFont(1); else u8x8.setInverseFont(0);
    u8x8.drawString(0,2,"EXPOSURE");
    u8x8.setCursor(9,2);
    u8x8.setInverseFont(0);
    u8x8.print(dolly.Exposure);
    if (menu_id==3) u8x8.setInverseFont(1); else u8x8.setInverseFont(0);
    u8x8.drawString(0,3,"ANGLE");
    u8x8.setCursor(9,3);
    u8x8.setInverseFont(0);
    u8x8.print(dolly.Angle);   
    */
    break;
  default:
    break;
  }
}

Ticker timer1(ReadButtons, 20);
Ticker timer2(DrawGui, 200);
Ticker timer3(EverySecond, 1000);

void setup(void)
{
  btn_up.attachClick(click_up);
  btn_center.attachClick(click_center);
  btn_center.attachLongPressStop(long_click_center);
  btn_left.attachClick(click_left);
  btn_down.attachClick(click_down);
  btn_right.attachClick(click_right);
  //button1.attachLongPressStart(longPressStart1);
  //button1.attachDuringLongPress(longPress1);
  stepper.begin(RPM, MICROSTEPS);
  ReadSettings();
  u8x8.begin();
  u8x8.setPowerSave(0);
  Serial.begin(9600);
  timer1.start();
  timer2.start();
  timer3.start();
  Serial.println("And so it begins...");
  stepper.rotate(360);
}

void loop(void)
{
  timer1.update();
  timer2.update();
  timer3.update();
}
