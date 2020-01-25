#include <Adafruit_NeoPixel.h>


#define PIXELS_PER_STRIP 21
#define PIXELS_LED_RING 8
#define SPEEDUP 100
#define LED_STRIP_1_PIN 2
#define LED_STRIP_2_PIN 4
#define LED_STRIP_3_PIN 6
#define LED_STRIP_4_PIN 8
#define LED_RING_PIN 10

#define MODE_SWITCH 13 //low = 2 players, high = 4 players
#define PIEZO_PIN = 11

#define BUTTON_1_PIN 3
#define BUTTON_2_PIN 5
#define BUTTON_3_PIN 7
#define BUTTON_4_PIN 9

#define DEBUG
#define INITIALSPEED 13
#define INITIAL_LIVES 5
#define NUMBER_OF_PLAYERS 4

#define PLAYERZONE PIXELS_PER_STRIP-8

typedef enum {LEFT, UP, RIGHT, DOWN, MIDDLE} side_type;

bool quit = false;

struct Button {
  int pin;
  bool down;    // button has been released just now
  bool up;      // button has been pressed just now
  bool isPressed; // button is being pressed
};

struct Player {
  uint32_t color;
  uint16_t lives;
  side_type side;
  Button button;
  Adafruit_NeoPixel strip;
};

struct Ball {
  uint32_t color;
  uint8_t currentLEDObject;
  float position;
  side_type direction_from;
  side_type direction_to;
  float speed; // pixels/s
};

struct Ball ball;
byte nextAlivePlayer;
byte alivePlayers = 4;

Adafruit_NeoPixel led_ring = Adafruit_NeoPixel(PIXELS_LED_RING, LED_RING_PIN, NEO_GRB + NEO_KHZ800);

Player players[4] = {
  { .color = led_ring.Color(0,0,255), 
    .lives = INITIAL_LIVES, 
    .side = LEFT, 
    .button = (Button) {.pin = BUTTON_1_PIN},
    .strip = Adafruit_NeoPixel(PIXELS_PER_STRIP, LED_STRIP_1_PIN, NEO_GRB + NEO_KHZ800)
  },
  { .color = led_ring.Color(255,0,0), 
    .lives = INITIAL_LIVES, 
    .side = UP, 
    .button = (Button) {.pin = BUTTON_2_PIN},
    .strip = Adafruit_NeoPixel(PIXELS_PER_STRIP, LED_STRIP_2_PIN, NEO_GRB + NEO_KHZ800)
  },
  { .color = led_ring.Color(0,255,0), 
    .lives = INITIAL_LIVES, 
    .side = RIGHT, 
    .button = (Button) {.pin = BUTTON_3_PIN},
    .strip = Adafruit_NeoPixel(PIXELS_PER_STRIP, LED_STRIP_3_PIN, NEO_GRB + NEO_KHZ800)
  },
  { .color = led_ring.Color(255,255,0), 
    .lives = INITIAL_LIVES, 
    .side = DOWN, 
    .button = (Button) {.pin = BUTTON_4_PIN},
    .strip = Adafruit_NeoPixel(PIXELS_PER_STRIP, LED_STRIP_4_PIN, NEO_GRB + NEO_KHZ800)
  },
};

Adafruit_NeoPixel ledObjects[5] = {
  players[0].strip,
  players[1].strip,
  players[2].strip,
  players[3].strip,
  led_ring
};


// Fill the dots one after the other with a color
void setAllTo(Adafruit_NeoPixel strip, uint32_t color) {
  for (uint16_t i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, color);
  }
}

void setup() {

  #ifdef DEBUG
    Serial.begin(9600);
  #endif

  pinMode(MODE_SWITCH, INPUT_PULLUP);
  for (byte i = 0; i < 4; i = i + 1) {
    pinMode(players[i].button.pin, INPUT_PULLUP);
    players[i].strip.begin();
    players[i].strip.setBrightness(40);  // Range from 0-255, so 100 is a bit less than 50% brightness
    players[i].strip.show();
    setAllTo(players[i].strip, players[i].strip.Color(0, 0, 0));
  }

  ball = (Ball) {
    .color = led_ring.Color(255, 0, 0),
    .currentLEDObject = MIDDLE,
    .position = 0,
    .direction_from = MIDDLE,
    .direction_to = MIDDLE,
    .speed = INITIALSPEED
  };

}

byte getNextAlivePlayer(byte last) {
  byte next = last=(sizeof(players) / sizeof(players[0])-1) ? last+1: 0;
  Serial.print("LoopCheck nextAlivePlayer: ");
  Serial.println(next);
  do {
    if(players[next].lives > 0) {
      Serial.print("nextAlivePlayer: ");
      Serial.println(next);
      return next;
    } else {
      next = last=(sizeof(players) / sizeof(players[0])) ? last+1: 0;
      Serial.print("LoopCheck nextAlivePlayer: ");
      Serial.println(next);
    }
  } while (next != last);
}


void renderPlayer(Player *p) {
    for (int i = 0; i < p->lives; i++) {
      p->strip.setPixelColor((PIXELS_PER_STRIP -1 - i), p->color);
    }

}

void renderBall(Ball *ball) {
  ledObjects[ball->currentLEDObject].setPixelColor((ball->position), ball->color);
}

// Refresh interval at which to set our game loop
// To avoid having the game run at different speeds depending on hardware
const int refreshInterval = 16; // 60 FPS

// Used to calculate the delta between loops for a steady frame-rate
unsigned long lastRefreshTime = 0;

void processButtonInput(Button *button) {
  bool prevPressed = button->isPressed;

  int state = digitalRead(button->pin);


  bool newPressed = state == HIGH;

  if (prevPressed && !newPressed) { // just released
    button->up = true;
    button->down = false;
  } else if (!prevPressed && newPressed) { // just pressed
    button->up = false;
    button->down = true;
  } else {
    button->up = false;
    button->down = false;
  }

  button->isPressed = newPressed;
}

void processInput() {
  for (byte i = 0; i < 4; i = i + 1) {
    processButtonInput(&players[i].button);
  }
}

void colorWipe(Adafruit_NeoPixel strip, uint32_t c, uint8_t wait) {
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
    strip.show();
    delay(wait);
  }
}
void drawWinner(Player p) {
  if (p.lives == 1) {
    if(alivePlayers == 2) {
      colorWipe(players[ball.direction_from].strip, players[ball.direction_from].color , 100);
    } else {
      alivePlayers = alivePlayers-1;
    }
  }
}

void drawGame()
{
  for (byte i = 0; i < 4; i = i + 1) {
    setAllTo(players[i].strip, players[i].strip.Color(0, 0, 0));
    renderPlayer(&players[i]);
    players[i].strip.setPixelColor(PLAYERZONE, 50, 50, 50);
    players[i].strip.show();
    Serial.print("Player ");
    Serial.print(i, DEC);
    Serial.print(": ");
    Serial.println(players[i].lives, DEC);
  }
  
  renderBall(&ball);

}

void drawPointLossAnimation(Player player)
{
  setAllTo(player.strip, player.strip.Color(255, 255, 255));
  player.strip.show();
  delay(200);
  setAllTo(player.strip, player.strip.Color(0, 0, 0));
  player.strip.show();
}

// void drawStrobo()
// {
//   static uint8_t pause = 0;
//   if (pause < 1)
//   {
//     setAllTo(strip.Color(255, 0, 0));
//     pause++;
//   }
//   else if (pause < 2)
//   {
//     setAllTo(strip.Color(0, 255, 0));
//     pause++;
//   }
//   else if (pause < 3)
//   {
//     setAllTo(strip.Color(0, 0, 255));
//     pause = 0;
//   }
//   strip.show();
// }

// uint32_t Wheel(byte WheelPos) {
//   WheelPos = 255 - WheelPos;
//   if (WheelPos < 85) {
//     return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
//   }
//   if (WheelPos < 170) {
//     WheelPos -= 85;
//     return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
//   }
//   WheelPos -= 170;
//   return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
// }




byte currentStandby = 0;
void drawStandby()
{
  colorWipe(players[currentStandby].strip, players[currentStandby].strip.Color(random(0, 255), random(0, 255), random(0, 255)), 10);
  currentStandby = currentStandby + 1;
}

void updateBall(Ball *ball, unsigned int td) {
  float moveBy = ball->speed * (td / (float) 1000);
  
  for (byte i = 0; i < 4; i = i + 1) {
      // The ball can only be returned by pushing the button down in the players zone 
    if (players[i].button.down && ball->direction_to == players[i].side && ball->position<PLAYERZONE) {

      if(ball->position<PLAYERZONE){
        ball->speed = SPEEDUP / ball->position ;
        
        nextAlivePlayer = getNextAlivePlayer(i);
        ball->direction_from = players[i].side;
        ball->direction_to = players[nextAlivePlayer].side;
        
      }
    }
  }
  


  if(ball->currentLEDObject==ball->direction_to) { //ball is on the way back to the middle
    ball->position = ball->position - moveBy;
    if(ball->position < 0) {
      ball->currentLEDObject = players[nextAlivePlayer].side;
      ball->position = ball->position - ball->position;
    }
  } else { //ball is already on new LED object, move towards playerzone
    ball->position = ball->position + moveBy;
    if (ball->position > PIXELS_PER_STRIP - 1) //ball got over the last pixel -->lose live
    {
      drawPointLossAnimation(players[ball->direction_to]);
      drawWinner(players[ball->direction_from]);
      players[ball->direction_to].lives = players[ball->direction_to].lives - 1;
      nextAlivePlayer = getNextAlivePlayer(ball->direction_to);
      if(players[ball->direction_to].lives > 0) { // if the player has lives left, than he starts the ball, otherwise the player that shot him out starts the ball
        ball->direction_from = ball->direction_to;
      }
      ball->direction_to = players[getNextAlivePlayer(ball->direction_from)].side;
      ball->position = PIXELS_PER_STRIP -1; //starting position is last pixel
      ball->speed = INITIALSPEED;
    }
  }

}

void update(unsigned int td)
{
  updateBall(&ball, td);

  if (1 == alivePlayers)
  {
    for (byte i = 0; i < 4; i = i + 1) {
      players[i].lives = INITIAL_LIVES;
    }
    quit = true;
  }
}

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
    static unsigned long lastTimeNotStandby;
    processInput();
    if (!quit)
    {

      update(td);
      drawGame();
      lastTimeNotStandby = millis();
    }
    else
    { //if all players hold down the button at the same time, the game starts
      if (players[LEFT].button.isPressed && players[UP].button.isPressed && players[RIGHT].button.isPressed && players[DOWN].button.isPressed)
      {
        quit = false;
      } else { //changed: only drawStandby if not changed
        drawStandby();
      }
      

    }
  }
}