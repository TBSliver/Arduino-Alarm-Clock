int menu_level = 0;
/* Menu Levels
 * 0: Front Page
 * 1: Main Menu
 * 2: Set Time
 * 3-9: Alarms
 */
int menu_state = 0;

void menu_system () {
  button_debounce();

  switch (menu_level) {
    case 0:
      menu_system_view_front();
      break;
    case 1:
      menu_system_view_main();
      break;
    case 2:
      menu_system_view_set_time();
      break;
    case 3:
      menu_system_view_set_alarm();
      break;
  }

  if ( button_debounced != btnNONE ) {
    switch(menu_level) {
      case 0:
        // Front menu, default
        menu_system_btn_front();
        break;
      case 1:
        menu_system_btn_main();
        break;
      case 2:
        menu_system_btn_set_time();
        break;
      case 3:
        menu_system_btn_set_alarm();
        break;
    }
  }

  button_debounced = btnNONE;
}

void menu_system_view_front() {
  lcd.setCursor(0,0);
  lcd.print(date_time());
  
  lcd.setCursor(0,1);
  lcd.write((uint8_t)charUPDOWN);
  if (strip_val < 100) lcd.print(" ");
  if (strip_val < 10) lcd.print(" ");
  lcd.print(strip_val);
  
  lcd.setCursor(4,1);
  lcd.print(" Alarm ");
  
  lcd.setCursor(11,1);
  lcd.print("00:00");
}

void menu_system_btn_front() {
  switch (button_debounced) {
    case btnUP:
      strip_val += 25;
      break;
    case btnDOWN:
      strip_val -= 25;
      break;
    case btnLEFT:
      if (bl_val < 127) {
        blocking_fade_backlight(0, 255, 500);
      } else {
        blocking_fade_backlight(255, 0, 500);
      }
      break;
    case btnRIGHT:
      if (strip_val < 127) {
        strip_val = 255;
      } else {
        strip_val = 0;
      }
      break;
    case btnSELECT:
      menu_level++;
      break;
  }
}

const char* main_menu_txt[] = {
  "   Set Time    >", // 0
  "<  Set Alarm   >", // 1
  "<  Exit         "  // 2
};

void menu_system_view_main() {
  lcd.setCursor(0,0);
  lcd.print("Main Menu  ");
  lcd.print(time_now());
  lcd.setCursor(0,1);
  lcd.print(main_menu_txt[menu_state]);
}

void menu_system_btn_main() {
  switch (button_debounced) {
    case btnLEFT:
      if (menu_state > 0) menu_state--;
      break;
    case btnRIGHT:
      if (menu_state < 2) menu_state++;
      break;
    case btnSELECT:
      switch (menu_state) {
        case 0:
          // set time
          menu_level = 2;
          menu_state = 0;
          break;
        case 1:
          // alarm mon
          menu_level = 3;
          menu_state = 0;
          break;
        case 2:
          // exit
          menu_level = 0;
          menu_state = 0;
          break;
      }
      break;
  }
}

const char* set_time_menu_opts[] {
  "  Minutes: ",
  "< Hours:   ",
  "< Day:     ",
  "< Month:   ",
  "< Year:    "
};

void menu_system_view_set_time() {
  lcd.setCursor(0,0);
  lcd.print("    Set Time    ");
  lcd.setCursor(0,1);
  lcd.print(set_time_menu_opts[menu_state]);
  lcd.write((uint8_t)charUPDOWN);
  DateTime now = rtc.now();
  switch(menu_state) {
    case 0:
      if(now.minute() < 10) lcd.print(0);
      lcd.print(now.minute());
      lcd.print(" >");
      break;
    case 1:
      if(now.hour() < 10) lcd.print(0);
      lcd.print(now.hour());
      lcd.print(" >");
      break;
    case 2:
      if(now.day() < 10) lcd.print(0);
      lcd.print(now.day());
      lcd.print(" >");
      break;
    case 3:
      if(now.month() < 10) lcd.print(0);
      lcd.print(now.month());
      lcd.print(" >");
      break;
    case 4:
      lcd.print(now.year());
      break;
  }
}

void menu_system_btn_set_time() {
  switch (button_debounced) {
    case btnUP:
      menu_system_btn_set_time_edit(true);
      break;
    case btnDOWN:
      menu_system_btn_set_time_edit(false);
      break;
    case btnLEFT:
      if (menu_state > 0) menu_state--;
      break;
    case btnRIGHT:
      if (menu_state < 4) menu_state++;
      break;
    case btnSELECT:
      menu_level = 1;
      menu_state = 0; // go back to the previous menu entry
      break;
  }
}

int month_length[] {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

// dir = true is up, dir = false is down.
void menu_system_btn_set_time_edit(bool dir) {
  DateTime now = rtc.now();
  DateTime edit;
  TimeSpan span;
  switch (menu_state) {
    case 0:
      span = TimeSpan(0,0,1,0);
      break;
    case 1:
      span = TimeSpan(0,1,0,0);
      break;
    case 2:
      span = TimeSpan(1,0,0,0);
      break;
    case 3:
      span = TimeSpan(month_length[now.month() - 1],0,0,0);
      break;
    case 4:
      span = TimeSpan(365,0,0,0);
      break;
  }
  if (dir) {
    edit = now + span;
  } else {
    edit = now - span;
  }
  rtc.adjust(edit);
}

const char* alarm_menu_opts[] {
  "  Hour: ",
  "< Mins: ",
  "< Fade: "
};

void menu_system_view_set_alarm() {
  lcd.setCursor(0,0);
  lcd.print("   Set Alarm    ");
  lcd.setCursor(0,1);
  lcd.print(alarm_menu_opts[menu_state]);
  lcd.write((uint8_t)charUPDOWN);
  switch (menu_state) {
    case 0:
      if (alarm_time[0] < 10) lcd.print(0);
      lcd.print(alarm_time[0]);
      lcd.print("    >");
      break;
    case 1:
      if (alarm_time[1] < 10) lcd.print(0);
      lcd.print(alarm_time[1]);
      lcd.print("    >");
      break;
    case 2:
      lcd.print(alarm_time[2]);
      lcd.print("       "); // more than enough to clear the screen
      break;
  }
}

void menu_system_btn_set_alarm() {
  switch (button_debounced) {
    case btnLEFT:
      if (menu_state > 0) menu_state--;
      break;
    case btnRIGHT:
      if (menu_state < 2) menu_state++;
      break;
    case btnSELECT:
      menu_level = 1;
      menu_state = 1; // go back to the previous menu entry
      break;
  }
}

