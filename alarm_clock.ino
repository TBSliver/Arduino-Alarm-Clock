#include <LiquidCrystal.h>
#include <Wire.h>
#include "RTClib.h"

RTC_DS3231 rtc;

// hours, minutes, and fade time in seconds
int alarm_time[] = {7, 40, 1200}; // 20 minutes

// select the pins used on the LCD panel
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

#define charUPDOWN 0
byte up_down[8] = {
  0b00100,
  0b01010,
  0b10001,
  0b00100,
  0b00100,
  0b10001,
  0b01010,
  0b00100
};

#define pinBACKLIGHT 10
int bl_val = 0;

#define pinSTRIP 11
int strip_val = 0;

// define some values used by the panel and buttons
int lcd_key     = 0;
int adc_key_in  = 0;
#define btnRIGHT  0
#define btnUP     1
#define btnDOWN   2
#define btnLEFT   3
#define btnSELECT 4
#define btnNONE   5

// read the buttons
int read_LCD_buttons()
{
  // read the value from the sensor 
  adc_key_in = analogRead(0);
  // We make this the 1st option for speed reasons since it will be the most likely result
  if (adc_key_in > 1000) return btnNONE;
  if (adc_key_in < 50)   return btnRIGHT;  
  if (adc_key_in < 250)  return btnUP; 
  if (adc_key_in < 450)  return btnDOWN; 
  if (adc_key_in < 650)  return btnLEFT; 
  if (adc_key_in < 850)  return btnSELECT;  
  // when all others fail, return this...
  return btnNONE;
}

// 255 is full on, 0 is full off
void set_backlight(int val)
{
  if ( val > 255 ) { val = 255; }
  if ( val < 0 ) { val = 0; }
  bl_val = val;
  analogWrite(pinBACKLIGHT, bl_val);
}

void set_strip(int val)
{
  if ( val > 255 ) { val = 255; }
  if ( val < 0 ) { val = 0; }
  strip_val = val;
  analogWrite(pinSTRIP, strip_val);
}

void update_lights() {
  set_backlight(bl_val);
  set_strip(strip_val);
}

void blocking_fade_backlight(int from, int to, unsigned long fade_time)
{
  if ( from > to ) {
    int diff = from - to;
    unsigned long step_time = fade_time / long(diff);
    for (int i = from; i >= to; i--) {
      set_backlight(i);
      delay(step_time);
    }
    return;
  } else if ( to > from ) {
    int diff = to - from;
    unsigned long step_time = fade_time / long(diff);
    for (int i = from; i <= to; i++) {
      set_backlight(i);
      delay(step_time);
    }
    return;
  }
  // otherwise they were the same, so just set it and be done with it
  set_backlight(from);
}

String date_now() {
  DateTime now = rtc.now();
  String return_date;
  if (now.day() < 10) return_date.concat(0);
  return_date.concat(now.day());
  return_date.concat("-");
  if (now.month() < 10) return_date.concat(0);
  return_date.concat(now.month());
  return_date.concat("-");
  return_date.concat(now.year() - 2000);
  return return_date;
}

String time_now (bool show_seconds) {
  DateTime now = rtc.now();
  String return_time;
  if (now.hour() < 10) return_time.concat(0);
  return_time.concat(now.hour());
  return_time.concat(":");
  if (now.minute() < 10) return_time.concat(0);
  return_time.concat(now.minute());
  if (show_seconds) {
    return_time.concat(":");
    if (now.second() < 10) return_time.concat(0);
    return_time.concat(now.second());
  }

  return return_time;
}

String time_now () {
  return time_now(false);
}

// should be 16 characters long constantly
String date_time () {
  return date_now() + "   " + time_now();
}

int button_debounced = btnNONE;
int button_transient = btnNONE;
unsigned long button_timer_start;
unsigned long button_debounce_delay = 200;
bool button_delay_active = false;

void button_debounce() {
  int current_button = read_LCD_buttons();

  if ( !button_delay_active ) {
    button_debounced = current_button;
    if ( button_debounced == btnNONE ) {
      return;
    }
    button_delay_active = true;
    button_timer_start = millis();
    return;
  }

  if ( (button_timer_start + button_debounce_delay) < millis() ) {
    button_delay_active = false;
  }
}

// MAIN LOOP FUNCTIONS
void setup()
{
  pinMode(pinBACKLIGHT, OUTPUT);
  pinMode(pinSTRIP, OUTPUT);
  set_backlight(0);
  set_strip(0);

  // LCD Initialisation
  lcd.createChar(charUPDOWN, up_down);
  lcd.begin(16, 2);
  lcd.setCursor(0,0);
  lcd.print("Daylight Clock");
  lcd.setCursor(0,1);
  lcd.print("Initialising...");
  
  // 3 second init to cover Serial port init
  blocking_fade_backlight(0, 255, 1000);

  if (! rtc.begin()) {
    lcd.setCursor(0,1);
    lcd.print("No RTC Found");
    // lock here because no use otherwise
    while (1);
  }

  // Finish Initialisation
  lcd.setCursor(0,1);
  lcd.print("                "); // blank line
}

void loop()
{
  menu_system();
  update_lights();
  alarm_check();
}

bool alarm_trigger = false;
uint32_t alarm_trigger_start;
uint32_t alarm_trigger_step_time = 0;
uint32_t alarm_trigger_step_count = 0;
int alarm_step_size = 1;

void alarm_check() {
  DateTime now = rtc.now();

  if (alarm_trigger) {
    if ( (alarm_trigger_start + alarm_time[2]) < now.unixtime()) {
      alarm_trigger = false;
      // alarms come to an end, must be full power now....
      strip_val = 255;
      return;
    }
    alarm_trigger_step_count = ((now.unixtime() - alarm_trigger_start) / alarm_trigger_step_time) * alarm_step_size;
    strip_val = alarm_trigger_step_count;
  } else if (now.hour() == alarm_time[0]) {
    if (now.minute() == alarm_time[1]) {
      alarm_trigger = true;
      alarm_trigger_start = now.unixtime();
      if (alarm_time[2] < 255) {
        alarm_step_size = 25;
      } else {
        alarm_step_size = 1;
      }
      alarm_trigger_step_time = alarm_time[2] / (255 / alarm_step_size);
      return;
    }
  }

}
/*  Front Screen:
 *   
 *  Up/Down: Change Strip brightness
 *  Left: Backlight on/off (10 seconds for on)
 *  Select: Enter menu & backlight on
 *  
 *  Main Menu:
 *  
 *  Left/Right: Select option
 *  Select: Enter
 *  
 *  0 Set Time
 *  1 Alarm Monday
 *  2 Alarm Tuesday
 *  3 Alarm Wednesday
 *  4 Alarm Thursday
 *  5 Alarm Friday
 *  6 Alarm Saturday
 *  7 Alarm Sunday
 *  8 Exit
 *  
 *  
 *  Set Time:
 *  
 *  Left/Right: Select option
 *  Up/Down: change value
 *  Select: Back n Save
 *  
 *  0 Minutes
 *  1 Hours
 *  2 Day
 *  3 Month
 *  4 Year
 *  
 *  Set Alarm:
 *  
 *  Left/Right: Select option
 *  Up/Down: change value
 *  Select: Back n Save
 *  
 *  0 Minutes
 *  1 Hours
 *  3 Fade Time (seconds)
 */

