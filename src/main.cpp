#include <Adafruit_NeoPixel.h>


#define PIXELS_PER_STRIP 17
#define PIXELS_LED_RING 16
#define SPEEDUP 40 //90
#define LED_STRIP_1_PIN 2
#define LED_STRIP_2_PIN 4
#define LED_STRIP_3_PIN 6
#define LED_STRIP_4_PIN 8
#define LED_RING_PIN 10

#define BUZZER_PIN 11

#define BUTTON_1_PIN 3
#define BUTTON_2_PIN 5
#define BUTTON_3_PIN 7
#define BUTTON_4_PIN 9

#define DEBUG
#define INITIALSPEED 10 //13
#define INITIAL_LIFES 3
#define NUMBER_OF_PLAYERS 4

#define PLAYERZONE 5
//------------------------
// Structures
//------------------------

typedef enum {LEFT, UP, RIGHT, DOWN, MIDDLE} side_type;

side_type sides[5] = {
  LEFT, UP, RIGHT, DOWN, MIDDLE
};

struct Button {
  int pin;
  bool down;    // button has been released just now
  bool up;      // button has been pressed just now
  bool isPressed; // button is being pressed
};

struct Color {
  byte red;
  byte green;
  byte blue;
  uint32_t RGBcolor;
};

struct Player {
  Color color;
  uint16_t lifes;
  side_type side;
  Button button;
};

struct Ball {
  Color color;
  uint8_t currentLEDObject;
  float position;
  side_type direction_from;
  side_type direction_to;
  float speed; // pixels/s
};


//------------------------
// Initializations
//------------------------

bool quit = true;

side_type nextAlivePlayer;
byte alivePlayers = 0;

Adafruit_NeoPixel led_ring = Adafruit_NeoPixel(PIXELS_LED_RING, LED_RING_PIN, NEO_GRB + NEO_KHZ800);

Adafruit_NeoPixel ledObjects[5] = {
  Adafruit_NeoPixel(PIXELS_PER_STRIP, LED_STRIP_1_PIN, NEO_GRB + NEO_KHZ800),
  Adafruit_NeoPixel(PIXELS_PER_STRIP, LED_STRIP_2_PIN, NEO_GRB + NEO_KHZ800),
  Adafruit_NeoPixel(PIXELS_PER_STRIP, LED_STRIP_3_PIN, NEO_GRB + NEO_KHZ800),
  Adafruit_NeoPixel(PIXELS_PER_STRIP, LED_STRIP_4_PIN, NEO_GRB + NEO_KHZ800),
  led_ring
};

Player players[4] = {
  { .color = Color{0,0,255,led_ring.Color(0,0,255)}, 
    .lifes = 0, 
    .side = LEFT, 
    .button = (Button) {.pin = BUTTON_1_PIN}
  },
  { .color = Color{255,0,0,led_ring.Color(255,0,0)}, 
    .lifes = 0, 
    .side = UP, 
    .button = (Button) {.pin = BUTTON_2_PIN}
  },
  { .color = Color{0,255,0,led_ring.Color(0,255,0)}, 
    .lifes = 0, 
    .side = RIGHT, 
    .button = (Button) {.pin = BUTTON_3_PIN}
  },
  { .color = Color{255,255,0,led_ring.Color(255,255,0)}, 
    .lifes = 0, 
    .side = DOWN, 
    .button = (Button) {.pin = BUTTON_4_PIN}
  },
};

Ball ball = {
  .color = Color{255,255,255,led_ring.Color(255,255,255)}, 
  .currentLEDObject = MIDDLE,
  .position = 0,
  .direction_from = MIDDLE,
  .direction_to = MIDDLE,
  .speed = INITIALSPEED
};


// Refresh interval at which to set our game loop
// To avoid having the game run at different speeds depending on hardware
const int refreshInterval = 16; // 60 FPS

// Used to calculate the delta between loops for a steady frame-rate
unsigned long lastRefreshTime = 0;

// is the game in standby mode?
byte currentStandby = 0;

//------------------------
// Supporting functions for LEDs
//------------------------

// Fill the dots one after the other with a color
void setAllTo(int ledNumber, uint32_t color) {
  for (uint16_t i = 0; i < ledObjects[ledNumber].numPixels(); i++) {
    ledObjects[ledNumber].setPixelColor(i, color);
  }
  ledObjects[ledNumber].show();
}

void colorWipe(int ledNumber, uint32_t c, uint8_t wait) {
  for (uint16_t i = 0; i < ledObjects[ledNumber].numPixels(); i++) {
    tone(BUZZER_PIN, 200 + (i*100));
    ledObjects[ledNumber].setPixelColor(i, c);
    ledObjects[ledNumber].show();
    delay(wait);
    noTone(BUZZER_PIN);
  }
}

// Fade color in and out
// Source (FadeInOut): https://www.tweaking4all.com/hardware/arduino/adruino-led-strip-effects/#LEDStripEffectFadeInandFadeOutYourownColors
void colorFadeInOut(int ledNumber, byte red, byte green, byte blue){
  float r, g, b;
     
  for(int k = 0; k < 256; k=k+1) {
    r = (k/256.0)*red;
    g = (k/256.0)*green;
    b = (k/256.0)*blue;
    setAllTo(ledNumber, ledObjects[ledNumber].Color(r, g, b));
  }
     
  for(int k = 255; k >= 0; k=k-2) {
    r = (k/256.0)*red;
    g = (k/256.0)*green;
    b = (k/256.0)*blue;
    setAllTo(ledNumber, ledObjects[ledNumber].Color(r, g, b));
  }
}

void drawPointLossAnimation(Player player)
{
  tone(BUZZER_PIN, 250);
  setAllTo(player.side, led_ring.Color(255, 255, 255));
  delay(200);
  setAllTo(player.side, led_ring.Color(0, 0, 0));
  noTone(BUZZER_PIN);
}

//------------------------
// Supporting functions
//------------------------

side_type getNextAlivePlayer(side_type last) {
  //byte next = last==(sizeof(players) / sizeof(players[0])-1) ? last+1: 0;
  byte next = sides[last+1] == MIDDLE ? LEFT: sides[last+1];
  Serial.print("LoopCheck nextAlivePlayer: ");
  Serial.println(next);
  do {
    if(players[next].lifes > 0) {
      Serial.print("nextAlivePlayer: ");
      Serial.println(next);
      return sides[next];
    } else {
      // next = last==(sizeof(players) / sizeof(players[0])) ? last+1: 0;
      next = sides[next+1] == MIDDLE ? LEFT: sides[next+1];
      Serial.print("LoopCheck nextAlivePlayer: ");
      Serial.println(next);
    }
  } while (next != last);
  return sides[4];
}

void renderPlayer(Player *p) {
    for (uint16_t i = 0; i < p->lifes; i++) {
      ledObjects[p->side].setPixelColor((i), p->color.RGBcolor); //led connection at button side
      ledObjects[p->side].show();
    }

}

void renderBall(Ball ball) {
  ledObjects[ball.currentLEDObject].setPixelColor((ball.position), ball.color.RGBcolor);
  ledObjects[ball.currentLEDObject].show();
}

void processButtonInput(Button *button) {
  bool prevPressed = button->isPressed;
  int state = digitalRead(button->pin);
  bool newPressed = state == LOW;
  // Serial.print("Button Pin ");
  // Serial.print(button->pin);
  if (prevPressed && !newPressed) { // just released
    button->up = true;
    button->down = false;
    Serial.print("justReleased: ");
    Serial.println(button->pin);
  } else if (!prevPressed && newPressed) { // just pressed
    button->up = false;
    Serial.print("justPressed: ");
    Serial.println(button->pin);
    button->down = true;
  } else {
    button->up = false;
    button->down = false;
  }

  button->isPressed = newPressed;
  // Serial.print(", isPressed: ");
  // Serial.print(button->isPressed);
  // Serial.print(", up: ");
  // Serial.print(button->up);
  // Serial.print(", down: ");
  // Serial.println(button->down);
}

void processInput() {
  for (byte i = 0; i < NUMBER_OF_PLAYERS; i = i + 1) {
    if(quit ||  // the button has only to be checked when the game is not running or
        (players[i].lifes > 0 && ball.currentLEDObject == players[i].side) // if the ball is on the players strip and he has lifes left
      ) {
      processButtonInput(&players[i].button);
    }
  }
}

// player lost a life
void loseLife() {
  players[ball.direction_to].lifes = players[ball.direction_to].lifes - 1; // reduce lifes of player
  if (players[ball.direction_to].lifes == 0) { // if the current player lost his last life
    alivePlayers = alivePlayers-1; // reduce number of players
    if(alivePlayers == 1) { // is only one player left? then reset all players lifes
      quit = true;
      alivePlayers = 0;
      players[ball.direction_from].lifes = 0;
      colorWipe(players[ball.direction_from].side, players[ball.direction_from].color.RGBcolor , 200);  
      // reset all strips to black after each game
      for (byte i = 0; i < NUMBER_OF_PLAYERS; i = i + 1) {
        setAllTo(players[i].side, led_ring.Color(0,0,0));
      }

    } else {
      colorWipe(players[ball.direction_to].side, players[ball.direction_from].color.RGBcolor , 100);
      setAllTo(players[ball.direction_to].side, led_ring.Color(0,0,0));
    }
  } else {
    drawPointLossAnimation(players[ball.direction_to]);
    ball.direction_from = ball.direction_to; // if the player has lifes left, than he starts the ball, otherwise the player that shot him out starts the ball
  }
  nextAlivePlayer = getNextAlivePlayer(ball.direction_from);
  ball.direction_to = players[nextAlivePlayer].side;
  ball.currentLEDObject = players[ball.direction_from].side;
  ball.position = 0; //starting position is last pixel
  ball.speed = INITIALSPEED;
}

void drawGame()
{
  for (byte i = 0; i < NUMBER_OF_PLAYERS; i = i + 1) {
    setAllTo(players[i].side, led_ring.Color(0, 0, 0));
    renderPlayer(&players[i]);
    ledObjects[players[i].side].setPixelColor(PLAYERZONE, players[i].color.red/10, players[i].color.green/10, players[i].color.blue/10);
    ledObjects[players[i].side].show();
  }
  
  renderBall(ball);

}

void drawStandby()
{
  if (players[LEFT].button.isPressed || players[UP].button.isPressed || players[RIGHT].button.isPressed || players[DOWN].button.isPressed) {
    for(byte i = 0; i < NUMBER_OF_PLAYERS; i = i + 1) {
      if(players[i].button.isPressed) {
        colorFadeInOut(players[i].side, players[i].color.red, players[i].color.green, players[i].color.blue);
      }
    }
  } else {
    colorFadeInOut(MIDDLE, random(0, 10), random(0, 10), random(0, 10)); 
  }
}

void updateBall(unsigned int td) {
  float moveBy = ball.speed * (td / (float) 1000);
  
  for (byte i = 0; i < NUMBER_OF_PLAYERS; i = i + 1) {
      // The ball can only be returned by pushing the button down in the players zone 
    if (ball.currentLEDObject == players[i].side && ball.currentLEDObject == ball.direction_to && ball.position<=PLAYERZONE+0.5 && (players[i].button.down || players[i].button.up)) {
      tone(BUZZER_PIN, 1000);
      ball.speed = SPEEDUP / ball.position ;
      nextAlivePlayer = getNextAlivePlayer(players[i].side);
      ball.direction_from = players[i].side;
      ball.direction_to = players[nextAlivePlayer].side;
    }
  }
  Serial.print("Ball Position: ");
  Serial.print(sides[ball.currentLEDObject]);
  Serial.print(" - ");
  Serial.println(ball.position);
  
  if(ball.currentLEDObject==ball.direction_from) { //ball is on the way back to the middle
    ball.position = ball.position + moveBy;
    if(ball.position > PIXELS_PER_STRIP) {
      ledObjects[ball.currentLEDObject].setPixelColor((ball.position), led_ring.Color(0,0,0));
      ball.currentLEDObject = ball.direction_to;
      ball.position = ball.position - moveBy;
    }
  } else { //ball is already on new LED object, move towards playerzone
    ball.position = ball.position - moveBy;
    if (ball.position <= 0 ) //ball got over the last pixel -->lose life
    {
      loseLife();
    }
  }

}

//------------------------
// setup function
//------------------------

void setup() {

  #ifdef DEBUG
    Serial.begin(9600);
  #endif

//Initialize players buttons and strips
  for (byte i = 0; i < NUMBER_OF_PLAYERS; i = i + 1) {
    pinMode(players[i].button.pin, INPUT_PULLUP);
    ledObjects[players[i].side].begin();
    ledObjects[players[i].side].setBrightness(15);  // Range from 0-255, so 100 is a bit less than 50% brightness
    ledObjects[players[i].side].show();
    
    setAllTo(players[i].side, led_ring.Color(0, 0, 0)); //necessary?
  }
//initialize LED ring
  led_ring.begin();
  led_ring.setBrightness(1);
  led_ring.show();
  setAllTo(MIDDLE, led_ring.Color(0, 0, 0)); //necessary?
}
//------------------------
// loop
//------------------------

void loop() {
  unsigned long now = millis();

  if (lastRefreshTime == 0) {
    lastRefreshTime = now;
    return;
  }

  unsigned int td = now - lastRefreshTime;

  if (td > refreshInterval)
  {
    lastRefreshTime = now;
    processInput();
    if (!quit)
    {
      noTone(BUZZER_PIN);
      updateBall(td);
      if (!quit) {
        drawGame();
      }
      
    }
    else
    { // the game starts if 2 or more players hold down the button and at least one releases it
      if (alivePlayers > 1 && (players[LEFT].button.up || players[UP].button.up || players[RIGHT].button.up || players[DOWN].button.up))
      {
        quit = false;
        if(players[LEFT].lifes > 0 && players[LEFT].button.up) { //left player is alive and released the button first
          ball.direction_from = LEFT;
          ball.direction_to = getNextAlivePlayer(LEFT);
        } else if(players[UP].lifes > 0 && players[UP].button.up) { //upper player is alive and released the button first
          ball.direction_from = UP;
          ball.direction_to = getNextAlivePlayer(UP);
        } else if(players[RIGHT].lifes > 0 && players[RIGHT].button.up) { //right player is alive and released the button first
          ball.direction_from = RIGHT;
          ball.direction_to = getNextAlivePlayer(RIGHT);
        } else if(players[DOWN].lifes > 0 && players[DOWN].button.up) { //down player is alive and released the button first
          ball.direction_from = DOWN;
          ball.direction_to = getNextAlivePlayer(DOWN);
        }
      } else {
        //Check if a new player joined or the first player released the button again
        for (byte i = 0; i < NUMBER_OF_PLAYERS; i = i + 1) {
          if(alivePlayers == 1 && players[i].button.up && players[i].lifes > 0) { // if the only player that previously held the button down releases it, remove him
            alivePlayers = alivePlayers - 1;
            players[i].lifes = 0;
            Serial.println("All players left");
          }
          if(players[i].button.down && players[i].lifes == 0) { // if player pressed the button just now, get him in the game
            alivePlayers = alivePlayers + 1;
            players[i].lifes = INITIAL_LIFES;
            
            Serial.print("Added player: ");
            Serial.println(i);
          }
        }
        drawStandby();
      }
      

    }
  }
}
