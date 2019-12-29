#include <Arduino.h>
#include <U8x8lib.h>
#include "OneButton.h"
#include "A4988.h"
#include <Ticker.h>
#include <EEPROM.h>

OneButton btn_up(18, true);
OneButton btn_center(15, true);
OneButton btn_left(14, true);
OneButton btn_down(16, true);
OneButton btn_right(10, true);

#define MOTOR_STEPS 200
#define RPM 120
#define MICROSTEPS 16

#define DIR 8
#define STEP 9

#define font_width 8
#define font_height 7

uint8_t screen=0;
uint8_t menu_id=0;
uint16_t SHOTS=120;
uint8_t INTERVAL=20;
uint8_t EXPOSURE=5;
uint8_t ANGLE=90;

void ReadButtons(void){
  btn_up.tick();
  btn_center.tick();
  btn_left.tick();
  btn_down.tick();
  btn_right.tick();
}



void click_up() {
  if (menu_id>0) menu_id--; 
  else menu_id=3;
  Serial.println("Button DOWN click.");
  Serial.println(menu_id);
} // click1

void click_down() {
  if (menu_id<3)  menu_id++;
  else menu_id=0;
  Serial.println("Button UP click.");
  Serial.println(menu_id);
} // click1

void click_left() {
  Serial.println("Button LEFT click.");
  switch (menu_id)
  {
  case 0: /* constant-expression */
    /* code */
    if (SHOTS>2) SHOTS--; else SHOTS=300;
    break;
  case 1:
    if (INTERVAL>2) INTERVAL--; else INTERVAL=250;
    break;
  
  case 2:
    if (EXPOSURE>2) EXPOSURE--; else EXPOSURE=250;
    break;

  case 3:
    if (ANGLE>2) ANGLE--; else ANGLE=250;
    break;
  default:
    break;
  }
} // click1

void click_right() {
  Serial.println("Button RIGHT click.");
  switch (menu_id)
  {
  case 0: /* constant-expression */
    /* code */
    if (SHOTS<400) SHOTS++; else SHOTS=30;
    break;
  case 1:
    if (INTERVAL<250) INTERVAL++; else INTERVAL=1;
    break;
  
  case 2:
    if (EXPOSURE<250) EXPOSURE++; else EXPOSURE=1;
    break;

  case 3:
    if (ANGLE<180) ANGLE++; else ANGLE=1;
    break;
  default:
    break;
  }
} // click1

void click_center() {
  Serial.println("Button CENTER click.");
} // click1



U8X8_SSD1306_128X32_UNIVISION_HW_I2C u8x8(/* reset=*/ U8X8_PIN_NONE);
A4988 stepper(MOTOR_STEPS, DIR, STEP);
Ticker timer1(ReadButtons, 20);


void DrawGui(void){
  u8x8.setFont(u8x8_font_saikyosansbold8_u);
  if (menu_id==0) u8x8.setInverseFont(1); else u8x8.setInverseFont(0);
  u8x8.drawString(0,0,"N  SHOTS");
  u8x8.setCursor(9,0);
  u8x8.setInverseFont(0);
  u8x8.print(SHOTS);
  if (menu_id==1) u8x8.setInverseFont(1); else u8x8.setInverseFont(0);
  u8x8.drawString(0,1,"INTERVAL");
  u8x8.setCursor(9,1);
  u8x8.setInverseFont(0);
  u8x8.print(INTERVAL);
  if (menu_id==2) u8x8.setInverseFont(1); else u8x8.setInverseFont(0);
  u8x8.drawString(0,2,"EXPOSURE");
  u8x8.setCursor(9,2);
  u8x8.setInverseFont(0);
  u8x8.print(EXPOSURE);
  if (menu_id==3) u8x8.setInverseFont(1); else u8x8.setInverseFont(0);
  u8x8.drawString(0,3,"ANGLE");
  u8x8.setCursor(9,3);
  u8x8.setInverseFont(0);
  u8x8.print(ANGLE);
}

Ticker timer2(DrawGui, 200);
void setup(void)
{
  btn_up.attachClick(click_up);
  btn_center.attachClick(click_center);
  btn_left.attachClick(click_left);
  btn_down.attachClick(click_down);
  btn_right.attachClick(click_right);

  //button1.attachLongPressStart(longPressStart1);
  //button1.attachLongPressStop(longPressStop1);
  //button1.attachDuringLongPress(longPress1);


  stepper.begin(RPM, MICROSTEPS);
  u8x8.begin();
  u8x8.setPowerSave(0);
  Serial.begin(9600);
  timer1.start();
  timer2.start();
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  Serial.println("Starting TwoButtons...");
  stepper.startMove(100 * (long)MOTOR_STEPS * (long)MICROSTEPS);

  ANGLE=EEPROM.read(0);
}

void loop(void)
{
  timer1.update();
  timer2.update();
}

