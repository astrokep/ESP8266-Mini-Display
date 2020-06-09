#include "MiniDisplay.h"


void initMiniDisplay(MiniDisplayProperties* mdp){
  Serial.begin(9600);
  mdp->button_ready_flag = true;
  scroll_position = 0;
  encoder_pin_values[0] = 0;
  encoder_pin_values[1] = 0;
}

void initScreens(MiniDisplayProperties* mdp){
  //init menu screen:
  screens[0].draw = drawMenuScreen;
  screens[0].next_screen = &screens[1];

  //init time screen:
  screens[1].draw = drawTimeScreen;
  screens[1].next_screen = &screens[0];

  //set current_screen to the time screen
  mdp->current_screen = &(screens[1]);
}

void initDisplay(MiniDisplayProperties* mdp, int w, int h, int addr, int rst_pin){
  mdp->screen_w = w;
  mdp->screen_h = h;
  mdp->screen_addr = addr;
  mdp->screen_rst_pin = rst_pin;
  mdp->oled_display = Adafruit_SSD1306(w, h, &Wire, -1);
  if (!mdp->oled_display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); // Don't proceed, loop forever
  }
  mdp->oled_display.setTextColor(WHITE);
  mdp->oled_display.clearDisplay();
  mdp->oled_display.drawBitmap(0, 0, astrokep_bmp, 128, 64, 1);
  mdp->oled_display.display();
}

void initWifi(MiniDisplayProperties* mdp, char* ssid, char* pass){
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
}

void initNTPTimeClient(MiniDisplayProperties* mdp, int utcOffset){
  int utcOffsetInSeconds = utcOffset * 3600;
  mdp->timeClient = new NTPClient(mdp->ntpUDP, "pool.ntp.org", utcOffsetInSeconds);
}

void registerIO(MiniDisplayProperties* mdp, int button_pin, int* encoder_pin_assignments){
  mdp->button_pin = button_pin;
  pinMode(button_pin, INPUT_PULLUP);

  for(int i = 0; i < 2; ++i){
    encoder_pins[i] = encoder_pin_assignments[i];
    pinMode(encoder_pins[i], INPUT_PULLUP);
    digitalPinToInterrupt(encoder_pins[i]);
    attachInterrupt(encoder_pins[i], readEncoder, CHANGE);
    encoder_pin_values[i] = digitalRead(encoder_pins[i]);
  }
}

ICACHE_RAM_ATTR static void readEncoder(){
  int current_encoder_values[2];
  for(int i = 0; i < 2; ++i){
    current_encoder_values[i] = digitalRead(encoder_pins[i]);
  }

  if (current_encoder_values[0] != encoder_pin_values[0] || 
      current_encoder_values[1] != encoder_pin_values[1] ){
    if(current_encoder_values[0] == 1 && current_encoder_values[1] == 1){
      if(encoder_pin_values[0] == 0 && encoder_pin_values[1] == 1){
        ++scroll_position;
      }
      else if(encoder_pin_values[0] == 1 && encoder_pin_values[1] == 0){
        --scroll_position;
      }
    }
    for(int i = 0; i < 2; ++i){
      encoder_pin_values[i] = current_encoder_values[i];
    }
  }
}

void pollIO(MiniDisplayProperties* mdp){
  //Handle encoder push button, prevent multi-sampling via button_ready_flag:
  if(mdp->button_ready_flag && !digitalRead(mdp->button_pin)){
    mdp->button_ready_flag = false;
    serviceButtonPress(mdp);
  }
  else if(!mdp->button_ready_flag && digitalRead(mdp->button_pin)){
    mdp->button_ready_flag = true;
  }

}

void serviceButtonPress(MiniDisplayProperties* mdp){
  mdp->disp_update_req = true;
  mdp->current_screen = mdp->current_screen->next_screen;
  delayMicroseconds(50);
}

void updateTime(MiniDisplayProperties* mdp){
  mdp->timeClient->update();
  mdp->disp_update_req = true;
}

void displayTime(MiniDisplayProperties* mdp, int x, int y, int textSize) {
  char t[6];
  mdp->oled_display.setCursor(x, y);
  mdp->oled_display.setTextSize(textSize);

  snprintf(t, sizeof(t), "%02d:%02d", mdp->timeClient->getHours(), mdp->timeClient->getMinutes());

  mdp->oled_display.print(t);
}

void displayDay(MiniDisplayProperties* mdp, int x, int y, int textSize) {
  char d[12];
  char *dayDecode[7] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

  mdp->oled_display.setCursor(x, y);
  mdp->oled_display.setTextSize(textSize);

  snprintf(d, sizeof(d), "%s", dayDecode[mdp->timeClient->getDay()]);

  mdp->oled_display.print(d);
}

void clearDisplay(MiniDisplayProperties* mdp){
  mdp->oled_display.clearDisplay();
}

void drawTimeScreen(MiniDisplayProperties* mdp){
  displayTime(mdp, 6, 20, 4);
  displayDay(mdp, 0, 0, 1);
}

void drawMenuScreen(MiniDisplayProperties* mdp){
  char d[24];
  mdp->oled_display.setCursor(5, 10);
  mdp->oled_display.setTextSize(2);
  snprintf(d, sizeof(d), "Menu - %d, %d - %d", encoder_pin_values[0], encoder_pin_values[1], scroll_position);
  mdp->oled_display.print(d);
}

void updateDisplay(MiniDisplayProperties* mdp){
  pollIO(mdp);
  if(mdp->disp_update_req){
    mdp->oled_display.clearDisplay();
    mdp->current_screen->draw(mdp);

    mdp->oled_display.display();
    mdp->disp_update_req = false;
  }
}
