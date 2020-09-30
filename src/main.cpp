#include <Adafruit_NeoPixel.h>
#include <EEPROM.h>

#define PIXELS_PER_STRIP 17
#define PIXELS_LED_RING 16

#define LED_STRIP_1_PIN 2
#define LED_STRIP_2_PIN 5
#define LED_STRIP_3_PIN A4
#define LED_STRIP_4_PIN A1
#define LED_RING_PIN 8

#define BUZZER_PIN 9

#define BUTTON_1_PIN 3
#define BUTTON_2_PIN 6
#define BUTTON_3_PIN A5
#define BUTTON_4_PIN A2

#define DEBUG

#define EEPROM_ADRESS_BRIGHTNESS 0
#define EEPROM_ADRESS_LOW_SPEED 1
#define EEPROM_ADRESS_HIGH_SPEED 2
#define EEPROM_ADRESS_GAME_MODE 3
#define EEPROM_ADRESS_MAX_RING_ROUNDS 4
#define EEPROM_ADRESS_SELF_PROBABILITY 5
#define EEPROM_ADRESS_SOUND 5

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

byte ringDirection = 0; //0=counter clockwise, 1=clockwise
bool ringPassed = false;

uint16_t ledRingMenuWay[16] = {6, 5, 4, 3, 2, 1, 0,15,14,13,12,11,10, 9, 8, 7};
uint16_t ledRingWays[4][4][18] = { //1st byte is number of pixels, following the pixel addresses
  { //LEFT to
    {17,  6, 5, 4, 3, 2, 1, 0,15,14,13,12,11,10, 9, 8, 7, 6}, //LEFT to LEFT
    { 5,  6, 5, 4, 3, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, //LEFT to UP
    { 9,  6, 5, 4, 3, 2, 1, 0,15,14, 0, 0, 0, 0, 0, 0, 0, 0}, //LEFT to RIGHT
    {13,  6, 5, 4, 3, 2, 1, 0,15,14,13,12,11,10, 0, 0, 0, 0}, //LEFT to DOWN
  },
  { //UP to
    {13,  2, 1, 0,15,14,13,12,11,10, 9, 8, 7, 6, 0, 0, 0, 0}, //UP to LEFT
    {17,  2, 1, 0,15,14,13,12,11,10, 9, 8, 7, 6, 5, 4, 3, 2}, //UP to UP
    { 5,  2, 1, 0,15,14, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, //UP to RIGHT
    { 9,  2, 1, 0,15,14,13,12,11,10, 0, 0, 0, 0, 0, 0, 0, 0}, //UP to DOWN
  },
  { //RIGHT to
    { 9, 14,13,12,11,10, 9, 8, 7, 6, 0, 0, 0, 0, 0, 0, 0, 0}, //RIGHT to LEFT
    {13, 14,13,12,11,10, 9, 8, 7, 6, 5, 4, 3, 2, 0, 0, 0, 0}, //RIGHT to UP
    {17, 14,13,12,11,10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0,15,14}, //RIGHT to RIGHT
    { 5, 14,13,12,11,10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, //RIGHT to DOWN
  },
  { //DOWN to
    { 5, 10, 9, 8, 7, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, //DOWN to LEFT
    { 9, 10, 9, 8, 7, 6, 5, 4, 3, 2, 0, 0, 0, 0, 0, 0, 0, 0}, //DOWN to UP
    {13, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0,15,14, 0, 0, 0, 0}, //DOWN to RIGHT
    {17, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0,15,14,13,12,11,10}, //DOWN to DOWN
  }
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
  Color life_color;
  Color zone_color;
  Color ring_color;
  uint16_t lifes;
  side_type side;
  Button button;
  side_type clockwiseNextPlayer;
  side_type counterClockwiseNextPlayer;
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

byte brightness_control = 0; //0=low brightness - 5=very high brightness
//will be overwritten from EEPROM

const byte MAIN_BRIGHTNESS[8] = {
  1, 5, 10, 25, 50, 100, 200, 255
}; //[0-255]
const byte BALL_BRIGHTNESS[8] = {
  1, 3, 8, 20, 35, 75, 150, 220
};
const byte LEDRING_BRIGHTNESS[8] = {
  5, 5, 8, 20, 35, 75, 150, 220
};
const byte ZONE_BRIGHTNESS[8] = {
  1, 1, 3, 10, 20, 50, 100, 200
};

int low_speed = 30; //[1-255] will be overwritten from EEPROM
int high_speed = 90; //[1-255] will be overwritten from EEPROM

bool quit = true;
bool isRestartBall = true;
int8_t restartMove = -1;

Adafruit_NeoPixel led_ring = Adafruit_NeoPixel(PIXELS_LED_RING, LED_RING_PIN, NEO_GRB + NEO_KHZ800);

Adafruit_NeoPixel ledObjects[5] = {
  Adafruit_NeoPixel(PIXELS_PER_STRIP, LED_STRIP_1_PIN, NEO_GRB + NEO_KHZ800),
  Adafruit_NeoPixel(PIXELS_PER_STRIP, LED_STRIP_2_PIN, NEO_GRB + NEO_KHZ800),
  Adafruit_NeoPixel(PIXELS_PER_STRIP, LED_STRIP_3_PIN, NEO_GRB + NEO_KHZ800),
  Adafruit_NeoPixel(PIXELS_PER_STRIP, LED_STRIP_4_PIN, NEO_GRB + NEO_KHZ800),
  led_ring
};

Player players[4] = {
  { .life_color = Color{0,0,MAIN_BRIGHTNESS[brightness_control],led_ring.Color(0,0,MAIN_BRIGHTNESS[brightness_control])}, 
    .zone_color = Color{0,0,ZONE_BRIGHTNESS[brightness_control],led_ring.Color(0,0,ZONE_BRIGHTNESS[brightness_control])}, 
    .ring_color = Color{0,0,LEDRING_BRIGHTNESS[brightness_control],led_ring.Color(0,0,LEDRING_BRIGHTNESS[brightness_control])}, 
    .lifes = 0, 
    .side = LEFT, 
    .button = (Button) {.pin = BUTTON_1_PIN},
    .clockwiseNextPlayer = LEFT,
    .counterClockwiseNextPlayer = LEFT
  },
  { .life_color = Color{0,MAIN_BRIGHTNESS[brightness_control],0,led_ring.Color(0,MAIN_BRIGHTNESS[brightness_control],0)}, 
    .zone_color = Color{0,ZONE_BRIGHTNESS[brightness_control],0,led_ring.Color(0,ZONE_BRIGHTNESS[brightness_control],0)}, 
    .ring_color = Color{0,LEDRING_BRIGHTNESS[brightness_control],0,led_ring.Color(0,LEDRING_BRIGHTNESS[brightness_control],0)}, 
    .lifes = 0, 
    .side = UP, 
    .button = (Button) {.pin = BUTTON_2_PIN},
    .clockwiseNextPlayer = UP,
    .counterClockwiseNextPlayer = UP
  },
  { .life_color = Color{MAIN_BRIGHTNESS[brightness_control],0,0,led_ring.Color(MAIN_BRIGHTNESS[brightness_control],0,0)}, 
    .zone_color = Color{ZONE_BRIGHTNESS[brightness_control],0,0,led_ring.Color(ZONE_BRIGHTNESS[brightness_control],0,0)}, 
    .ring_color = Color{LEDRING_BRIGHTNESS[brightness_control],0,0,led_ring.Color(LEDRING_BRIGHTNESS[brightness_control],0,0)}, 
    .lifes = 0, 
    .side = RIGHT, 
    .button = (Button) {.pin = BUTTON_3_PIN},
    .clockwiseNextPlayer = RIGHT,
    .counterClockwiseNextPlayer = RIGHT
  },
  { .life_color = Color{MAIN_BRIGHTNESS[brightness_control],MAIN_BRIGHTNESS[brightness_control],0,led_ring.Color(MAIN_BRIGHTNESS[brightness_control],MAIN_BRIGHTNESS[brightness_control],0)}, 
    .zone_color = Color{ZONE_BRIGHTNESS[brightness_control],ZONE_BRIGHTNESS[brightness_control],0,led_ring.Color(ZONE_BRIGHTNESS[brightness_control],ZONE_BRIGHTNESS[brightness_control],0)}, 
    .ring_color = Color{LEDRING_BRIGHTNESS[brightness_control],LEDRING_BRIGHTNESS[brightness_control],0,led_ring.Color(LEDRING_BRIGHTNESS[brightness_control],LEDRING_BRIGHTNESS[brightness_control],0)}, 
    .lifes = 0, 
    .side = DOWN, 
    .button = (Button) {.pin = BUTTON_4_PIN},
    .clockwiseNextPlayer = DOWN,
    .counterClockwiseNextPlayer = DOWN
  },
};

side_type nextAlivePlayer;
byte alivePlayers = 0;
side_type alivePlayerList[4];

void appendAlivePlayer(side_type item) {
    if (alivePlayers < NUMBER_OF_PLAYERS) alivePlayerList[alivePlayers++] = item;
}
void removeAlivePlayer(side_type item) {
  alivePlayers--;
  byte addedAlivePlayers = 0;
  byte i = 0;
  while (addedAlivePlayers < alivePlayers) {
    if(players[i].lifes > 0) {
      alivePlayerList[addedAlivePlayers++] = sides[i];
    }
    i++;
  }

  players[players[item].clockwiseNextPlayer].counterClockwiseNextPlayer = players[item].counterClockwiseNextPlayer;
  players[players[item].counterClockwiseNextPlayer].clockwiseNextPlayer = players[item].clockwiseNextPlayer;
}

Ball ball = {
  .color = Color{BALL_BRIGHTNESS[brightness_control],BALL_BRIGHTNESS[brightness_control],BALL_BRIGHTNESS[brightness_control],led_ring.Color(BALL_BRIGHTNESS[brightness_control],BALL_BRIGHTNESS[brightness_control],BALL_BRIGHTNESS[brightness_control])}, 
  .currentLEDObject = MIDDLE,
  .position = 5,
  .direction_from = MIDDLE,
  .direction_to = MIDDLE,
  .speed = low_speed*1.0
};

// Refresh interval at which to set our game loop
// To avoid having the game run at different speeds depending on hardware
const int refreshInterval = 16; // 60 FPS

// Used to calculate the delta between loops for a steady frame-rate
unsigned long lastRefreshTime = 0;
unsigned long loseLifeTime = 0;

//menu
byte menuActivationCounter = 0;
byte menuMode = 0; //0=menu inactive, 1=set brightness, 2=set LOW_SPEED, 3=set HIGH_SPEED
byte gameMode = 1; //1=counter-clockwise, 2=clockwise, 3=random direction, 4=random target (excl. self), 5=random target (incl. self), 6=random mode on every new game
bool randomGameMode = false;
byte maxRingRounds = 0;
byte ringRoundRandomValue = 0;
byte ringRoundCounter = 0;
byte selfProbability = 16; // value of 1 means 1/2 chance that the ball returns to self, 5 means 1/6 chance
bool soundActivated = true;

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
    if(soundActivated) tone(BUZZER_PIN, 200 + (i*100));
    ledObjects[ledNumber].setPixelColor(i, c);
    ledObjects[ledNumber].show();
    delay(wait);
    if(soundActivated) noTone(BUZZER_PIN);
  }
}

void initGameSettings() 
{
  players[LEFT].life_color = Color{0,0,MAIN_BRIGHTNESS[brightness_control],led_ring.Color(0,0,MAIN_BRIGHTNESS[brightness_control])};
  players[LEFT].zone_color = Color{0,0,ZONE_BRIGHTNESS[brightness_control],led_ring.Color(0,0,ZONE_BRIGHTNESS[brightness_control])};
  players[LEFT].ring_color = Color{0,0,LEDRING_BRIGHTNESS[brightness_control],led_ring.Color(0,0,LEDRING_BRIGHTNESS[brightness_control])};
  players[UP].life_color = Color{0,MAIN_BRIGHTNESS[brightness_control],0,led_ring.Color(0,MAIN_BRIGHTNESS[brightness_control],0)}; 
  players[UP].zone_color = Color{0,ZONE_BRIGHTNESS[brightness_control],0,led_ring.Color(0,ZONE_BRIGHTNESS[brightness_control],0)};
  players[UP].ring_color = Color{0,LEDRING_BRIGHTNESS[brightness_control],0,led_ring.Color(0,LEDRING_BRIGHTNESS[brightness_control],0)};
  players[RIGHT].life_color = Color{MAIN_BRIGHTNESS[brightness_control],0,0,led_ring.Color(MAIN_BRIGHTNESS[brightness_control],0,0)}; 
  players[RIGHT].zone_color = Color{ZONE_BRIGHTNESS[brightness_control],0,0,led_ring.Color(ZONE_BRIGHTNESS[brightness_control],0,0)};
  players[RIGHT].ring_color = Color{LEDRING_BRIGHTNESS[brightness_control],0,0,led_ring.Color(LEDRING_BRIGHTNESS[brightness_control],0,0)};
  players[DOWN].life_color = Color{MAIN_BRIGHTNESS[brightness_control],MAIN_BRIGHTNESS[brightness_control],0,led_ring.Color(MAIN_BRIGHTNESS[brightness_control],MAIN_BRIGHTNESS[brightness_control],0)}; 
  players[DOWN].zone_color = Color{ZONE_BRIGHTNESS[brightness_control],ZONE_BRIGHTNESS[brightness_control],0,led_ring.Color(ZONE_BRIGHTNESS[brightness_control],ZONE_BRIGHTNESS[brightness_control],0)};
  players[DOWN].ring_color = Color{LEDRING_BRIGHTNESS[brightness_control],LEDRING_BRIGHTNESS[brightness_control],0,led_ring.Color(LEDRING_BRIGHTNESS[brightness_control],LEDRING_BRIGHTNESS[brightness_control],0)};
  ball.color = Color{BALL_BRIGHTNESS[brightness_control],BALL_BRIGHTNESS[brightness_control],BALL_BRIGHTNESS[brightness_control],led_ring.Color(BALL_BRIGHTNESS[brightness_control],BALL_BRIGHTNESS[brightness_control],BALL_BRIGHTNESS[brightness_control])};
}

void initPlayers()
{
  for (byte i = 0; i < NUMBER_OF_PLAYERS; i++) {
    if(players[i].lifes>0) {
      byte nextCounterClockwise = sides[i] == DOWN ? LEFT: sides[i+1];
      byte nextClockwise = sides[i] == LEFT ? DOWN: sides[i-1];
      for (byte j = 0; j < NUMBER_OF_PLAYERS-1; j++) {
        if(players[nextCounterClockwise].lifes > 0) {
          players[i].counterClockwiseNextPlayer = sides[nextCounterClockwise];
        } else {
          nextCounterClockwise = sides[nextCounterClockwise+1] == MIDDLE ? LEFT: sides[nextCounterClockwise+1];
        }
        if(players[nextClockwise].lifes > 0) {
          players[i].clockwiseNextPlayer = sides[nextClockwise];
        } else {
          nextClockwise = sides[nextClockwise] == LEFT ? DOWN: sides[nextClockwise-1];
        }
      }
    }
  }
  for (byte i = 0; i < NUMBER_OF_PLAYERS; i++) {
    Serial.print("Player: "); Serial.println(players[i].side);
    Serial.print("   Lifes: "); Serial.println(players[i].lifes);
    Serial.print("   NextClockwise: "); Serial.println(players[i].clockwiseNextPlayer);
    Serial.print("   NextCounterClockwise: "); Serial.println(players[i].counterClockwiseNextPlayer);
  }
}

void drawPointLossAnimation(Player player)
{
  if(soundActivated) tone(BUZZER_PIN, 250);
  setAllTo(player.side, led_ring.Color(BALL_BRIGHTNESS[brightness_control], BALL_BRIGHTNESS[brightness_control], BALL_BRIGHTNESS[brightness_control]));
  delay(100);
  setAllTo(player.side, led_ring.Color(0, 0, 0));
  if(soundActivated) noTone(BUZZER_PIN);
}

//------------------------
// Supporting functions
//------------------------

side_type getNextAlivePlayer(side_type last) {
  Serial.print("Number of Alive Players: ");Serial.println(alivePlayers);
  if(alivePlayers == 1) { //only one player alive
    return alivePlayerList[0];

  } else if(gameMode == 5 || gameMode == 4) { //random next player incl./excl. self
    side_type alivePlayersArray[alivePlayers-1];
    byte side_index = 0;
    for(byte i = 0; i < alivePlayers; i++) {
      if(alivePlayerList[i] != last) {
        alivePlayersArray[side_index++] = alivePlayerList[i];
      }
    }
    long next_index = random(0, alivePlayers-1);
    Serial.print("next_index=");  Serial.println(next_index);
    side_type next = alivePlayersArray[next_index];
    Serial.print("next_side=");  Serial.println(next);

    if(gameMode == 5) {
      if(random(1,selfProbability+2)==1){
        Serial.print("next_side=self");  Serial.println(next);
        return last;
      }
    }
    return next;

  } else if (ringDirection == 0) { //counter clockwise
    return players[last].counterClockwiseNextPlayer;
    // byte next = sides[last+1] == MIDDLE ? LEFT: sides[last+1];
    // do {
    //   if(players[next].lifes > 0) {
    //     return sides[next];
    //   } else {
    //     next = sides[next+1] == MIDDLE ? LEFT: sides[next+1];
    //   }
    // } while (next != last);

  } else if (ringDirection == 1) { //clockwise
    return players[last].clockwiseNextPlayer;
    // byte next = last == 0 ? DOWN: sides[last-1];
    // Serial.print("1 LoopCheck nextAlivePlayer: ");
    // Serial.println(next);
    // do {
    //   if(players[next].lifes > 0) {
    //     Serial.print("nextAlivePlayer: ");
    //     Serial.println(next);
    //     return sides[next];
    //   } else {
    //     next = next == 0 ? DOWN: sides[next-1];
    //     Serial.print("LoopCheck nextAlivePlayer: ");
    //     Serial.println(next);
    //   }
    // } while (next != last);
  }
return last;

}

void renderPlayer(Player *p) {
    for (uint16_t i = 0; i < p->lifes; i++) {
      ledObjects[p->side].setPixelColor((i), p->life_color.RGBcolor); //led connection at button side
      ledObjects[p->side].show();
    }

}

void renderBall(Ball ball) {
  if(ball.currentLEDObject == MIDDLE) {
    if(ringRoundCounter<ringRoundRandomValue){
      if(ringDirection == 1) {
        ledObjects[ball.currentLEDObject].setPixelColor(ledRingWays[ball.direction_from][ball.direction_from][(byte)(ball.position)+1]
          , ball.color.red
          , ball.color.green
          , ball.color.blue);
      } else {
        ledObjects[ball.currentLEDObject].setPixelColor(ledRingWays[ball.direction_from][ball.direction_from][(byte)(ball.position)]
          , ball.color.red
          , ball.color.green
          , ball.color.blue);
      }
    } else {
      if(ringDirection == 1) {
        ledObjects[ball.currentLEDObject].setPixelColor(ledRingWays[ball.direction_to][ball.direction_from][(byte)(ball.position)+1]
          , ball.color.red
          , ball.color.green
          , ball.color.blue);
      } else {
        ledObjects[ball.currentLEDObject].setPixelColor(ledRingWays[ball.direction_from][ball.direction_to][(byte)(ball.position)]
          , ball.color.red
          , ball.color.green
          , ball.color.blue);
      } 
    }
  } else {
    ledObjects[ball.currentLEDObject].setPixelColor(ball.position
      , ball.color.red
      , ball.color.green
      , ball.color.blue);
  }
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
  loseLifeTime = millis();
  players[ball.direction_to].lifes = players[ball.direction_to].lifes - 1; // reduce lifes of player
  Serial.print("Life lost: Player ");
  Serial.println(ball.direction_to);

  if (players[ball.direction_to].lifes == 0) { // if the current player lost his last life
    Serial.print("Last life lost: Player ");
    Serial.println(ball.direction_to);
    removeAlivePlayer(ball.direction_to); // reduce number of players
    if(alivePlayers <= 1) { // is only one player left? then reset all players lifes
      quit = true;
      Serial.print("Game won: Player ");
      side_type winner = getNextAlivePlayer(ball.direction_from);
      Serial.println(winner);
      alivePlayers = 0;
      players[winner].lifes = 0;
      colorWipe(players[winner].side, players[winner].life_color.RGBcolor , 100);  
      // reset all strips to black after each game
      for (byte i = 0; i < NUMBER_OF_PLAYERS; i = i + 1) {
        setAllTo(players[i].side, led_ring.Color(0,0,0));
      }

    } else { //one player is out but more than one is left
      colorWipe(players[ball.direction_to].side, players[ball.direction_from].life_color.RGBcolor , 20);
      setAllTo(players[ball.direction_to].side, led_ring.Color(0,0,0));
      nextAlivePlayer = getNextAlivePlayer(ball.direction_from);
      if(ball.direction_from == ball.direction_to) { // after self destruction
        ball.direction_from = nextAlivePlayer;
      }
    }
  } else { //player lost one life but is still in 
    drawPointLossAnimation(players[ball.direction_to]);
    ball.direction_from = ball.direction_to; // if the player has lifes left, than he starts the ball, otherwise the player that shot him out starts the ball
    nextAlivePlayer = getNextAlivePlayer(ball.direction_from);
    ball.direction_to = nextAlivePlayer;
  }
  if(gameMode == 3 || gameMode == 4 || gameMode == 5 ) {
    ringDirection = random(0,2);
  }
  Serial.print("Start side: Player ");
  Serial.println(ball.direction_from);
  ball.direction_to = nextAlivePlayer;
  Serial.print("Target side: Player ");
  Serial.println(ball.direction_to);
  ball.currentLEDObject = ball.direction_from;
  ball.position = PLAYERZONE; //starting position is playerzone
  ball.speed = low_speed;
  isRestartBall = true;
  restartMove = -1;
  lastRefreshTime = 0;
}

void drawGame()
{
  setAllTo(MIDDLE, led_ring.Color(0, 0, 0));
  for (byte i = 0; i < NUMBER_OF_PLAYERS; i = i + 1) {
    setAllTo(players[i].side, led_ring.Color(0, 0, 0));
    renderPlayer(&players[i]);
    if(players[i].lifes>0) {
      ledObjects[players[i].side].setPixelColor(PLAYERZONE
      , players[i].zone_color.red
      , players[i].zone_color.green
      , players[i].zone_color.blue);
    } else {
      ledObjects[players[i].side].setPixelColor(PLAYERZONE, 0, 0, 0);
    }
    
    ledObjects[players[i].side].show();
  }
  renderBall(ball);

}

void updateBallMenu(int speed)
{
  setAllTo(RIGHT, led_ring.Color(0, 0, 0));
  ball.direction_from = RIGHT;
  ball.currentLEDObject = RIGHT;
  ball.direction_to = RIGHT;
  
  float moveBy = speed/100.0;
  ball.position = ball.position + (moveBy * restartMove);
  if (ball.position <= 0 ) {//ball got out of player zone switch direction
    restartMove = 1;
    ball.position = 0;
  } else if (ball.position > PIXELS_LED_RING) {
    restartMove = -1;
    ball.position = PIXELS_LED_RING;
  }
  renderBall(ball);
}

void drawMenu()
{
  setAllTo(MIDDLE, led_ring.Color(0, 0, 0));
  setAllTo(RIGHT, led_ring.Color(0, 0, 0));
  
  for(byte i = 0; i < menuMode; i = i + 1) {
    ledObjects[LEFT].setPixelColor(i
      , players[LEFT].life_color.red
      , players[LEFT].life_color.green
      , players[LEFT].life_color.blue);
    ledObjects[LEFT].show();
  }

  if(players[LEFT].button.up){
    menuMode = menuMode + 1;
  }
  if(menuMode==1) { //brightness
    if(players[UP].button.up){
      brightness_control==7 ? brightness_control = 0 : brightness_control = brightness_control + 1;
    }
    if(players[DOWN].button.up){
      brightness_control==0 ? brightness_control = 7 : brightness_control = brightness_control - 1;
    }
    initGameSettings();
    for(byte i = 0; i <= ((brightness_control+1)*2)-1; i = i + 1) {
      ledObjects[MIDDLE].setPixelColor(ledRingMenuWay[i]
        , players[RIGHT].life_color.red
        , players[RIGHT].life_color.green
        , players[RIGHT].life_color.blue);
        ledObjects[MIDDLE].show();
    }
  } else if(menuMode==2) { //low_speed
    if(players[UP].button.up){
      low_speed>=170 ? low_speed = 20 : low_speed = low_speed + 10;
    }
    if(players[DOWN].button.up){
      low_speed<=20 ? low_speed = 170 : low_speed = low_speed - 10;
    }
    updateBallMenu(low_speed);
    for(byte i = 0; i < (low_speed-10)/10; i = i + 1) {
      ledObjects[MIDDLE].setPixelColor(ledRingMenuWay[i]
        , players[RIGHT].life_color.red
        , players[RIGHT].life_color.green
        , players[RIGHT].life_color.blue);
        ledObjects[MIDDLE].show();
    }
  } else if(menuMode==3) { //high_speed
    if(players[UP].button.up){
      high_speed>=190 ? high_speed = 40 : high_speed = high_speed + 10;
    }
    if(players[DOWN].button.up){
      high_speed<=40 ? high_speed = 190 : high_speed = high_speed - 10;
    }
    updateBallMenu(high_speed);
    for(byte i = 0; i < (high_speed-30)/10; i = i + 1) {
      ledObjects[MIDDLE].setPixelColor(ledRingMenuWay[i]
        , players[RIGHT].life_color.red
        , players[RIGHT].life_color.green
        , players[RIGHT].life_color.blue);
        ledObjects[MIDDLE].show();
    }
  } else if(menuMode==4) { //game mode
    if(players[UP].button.up){
      gameMode>=6 ? gameMode = 1 : gameMode = gameMode + 1;
    }
    if(players[DOWN].button.up){
      gameMode<=1 ? gameMode = 6 : gameMode = gameMode - 1;
    }
    if(gameMode==6) {
      randomGameMode = true;
    } else {
      randomGameMode = false;
    }
    for(byte i = 0; i < gameMode; i = i + 1) {
      ledObjects[MIDDLE].setPixelColor(ledRingMenuWay[i]
        , players[RIGHT].life_color.red
        , players[RIGHT].life_color.green
        , players[RIGHT].life_color.blue);
        ledObjects[MIDDLE].show();
    }
  } else if(menuMode==5) { // maximum rounds the middle ring will go
    if(players[UP].button.up){
      maxRingRounds>=16 ? maxRingRounds = 0 : maxRingRounds = maxRingRounds + 1;
    }
    if(players[DOWN].button.up){
      maxRingRounds<=0 ? maxRingRounds = 16 : maxRingRounds = maxRingRounds - 1;
    }
    for(byte i = 0; i < maxRingRounds; i = i + 1) {
      ledObjects[MIDDLE].setPixelColor(ledRingMenuWay[i]
        , players[RIGHT].life_color.red
        , players[RIGHT].life_color.green
        , players[RIGHT].life_color.blue);
        ledObjects[MIDDLE].show();
    }
  } else if(menuMode==6) { // self probability
    if(players[UP].button.up){
      selfProbability>=16 ? selfProbability = 1 : selfProbability = selfProbability + 1;
    }
    if(players[DOWN].button.up){
      selfProbability<=1 ? selfProbability = 16 : selfProbability = selfProbability - 1;
    }
    for(byte i = 0; i < selfProbability; i = i + 1) {
      ledObjects[MIDDLE].setPixelColor(ledRingMenuWay[i]
        , players[RIGHT].life_color.red
        , players[RIGHT].life_color.green
        , players[RIGHT].life_color.blue);
        ledObjects[MIDDLE].show();
    }
  } else if(menuMode==7) { // sound
    byte soundActivatedRead = soundActivated ? 1 : 0;
    if(players[UP].button.up){
      soundActivatedRead==1 ? soundActivatedRead = 0 : soundActivatedRead = 1;
    }
    if(players[DOWN].button.up){
      soundActivatedRead==1 ? soundActivatedRead = 0 : soundActivatedRead = 1;
    }
    for(byte i = 0; i < soundActivatedRead; i = i + 1) {
      ledObjects[MIDDLE].setPixelColor(ledRingMenuWay[i]
        , players[RIGHT].life_color.red
        , players[RIGHT].life_color.green
        , players[RIGHT].life_color.blue);
        ledObjects[MIDDLE].show();
    }
    soundActivated = soundActivatedRead == 1 ? true : false;
  }

   else if(menuMode==8) {
    EEPROM.update(EEPROM_ADRESS_BRIGHTNESS, brightness_control);
    EEPROM.update(EEPROM_ADRESS_LOW_SPEED, low_speed);
    EEPROM.update(EEPROM_ADRESS_HIGH_SPEED, high_speed);
    EEPROM.update(EEPROM_ADRESS_GAME_MODE, gameMode);
    EEPROM.update(EEPROM_ADRESS_MAX_RING_ROUNDS, maxRingRounds);
    EEPROM.update(EEPROM_ADRESS_SELF_PROBABILITY, selfProbability);
    EEPROM.update(EEPROM_ADRESS_SOUND, soundActivated ? 1 : 0);
    setAllTo(RIGHT, led_ring.Color(0, 0, 0));
    setAllTo(LEFT, led_ring.Color(0, 0, 0));
    setAllTo(MIDDLE, led_ring.Color(0, 0, 0));
    menuMode=0;
  }
}

void drawStandby()
{
  if(menuActivationCounter > 0) {
    ledObjects[MIDDLE].setPixelColor(ledRingWays[LEFT][LEFT][1], 0,0,0);
    ledObjects[MIDDLE].show();
    for(byte i = 0; i < menuActivationCounter; i = i + 1) {
      ledObjects[LEFT].setPixelColor(i
        , players[LEFT].life_color.red
        , players[LEFT].life_color.green
        , players[LEFT].life_color.blue);
      ledObjects[LEFT].show();
    }
  }  else if (players[LEFT].button.isPressed || players[UP].button.isPressed || players[RIGHT].button.isPressed || players[DOWN].button.isPressed) {
    for(byte i = 0; i < NUMBER_OF_PLAYERS; i = i + 1) {
      if(players[i].button.isPressed) {
        ledObjects[MIDDLE].setPixelColor(ledRingWays[players[i].side][players[i].side][1]
        , players[i].ring_color.red
        , players[i].ring_color.green
        , players[i].ring_color.blue);
        ledObjects[MIDDLE].show();
      } else {
        ledObjects[MIDDLE].setPixelColor(ledRingWays[players[i].side][players[i].side][1], 0,0,0);
        ledObjects[MIDDLE].show();
      }
    }
  } else { //no button pressed and menu not active
    for(byte i = 0; i < NUMBER_OF_PLAYERS; i = i + 1) {
      ledObjects[MIDDLE].setPixelColor(ledRingWays[players[i].side][players[i].side][1], 0,0,0);
      ledObjects[MIDDLE].show();
    }
  }
}

void updateBall(unsigned int td) {
  if(isRestartBall) { //on restarting the ball the ball moves in the player zone for initial speed
    if(millis() > loseLifeTime + 1000) { //delay after losing life to not accidentally start the ball with failed return hit
      loseLifeTime = 0;
      float moveBy = low_speed/100.0;
      ball.speed = low_speed;
      //for (byte i = 0; i < NUMBER_OF_PLAYERS; i = i + 1) {
          // The ball can only be started by pushing the button in the players zone 
        if (ball.currentLEDObject == players[ball.direction_from].side && ball.position<=PLAYERZONE+0.5 && players[ball.direction_from].button.up) {
          if(soundActivated) tone(BUZZER_PIN, 500);
          ringPassed = false;
          ringRoundRandomValue = random(0,maxRingRounds+1);
          ringRoundCounter = 0;
          ball.speed = low_speed + ((high_speed - low_speed) * ((PLAYERZONE-ball.position)/PLAYERZONE )) ;
          isRestartBall = false;
        }
      //}
      //move ball in players zone
      ball.position = ball.position + (moveBy * restartMove);
      if (ball.position <= 0 ) {//ball got out of player zone switch direction
        restartMove = 1;
        ball.position = 0;
      } else if (ball.position >= PLAYERZONE) {
        restartMove = -1;
        ball.position = PLAYERZONE;
      }
    }
  } else { //not restart ball
    float moveBy = ball.speed / 100.0;
  //for (byte i = 0; i < NUMBER_OF_PLAYERS; i = i + 1) {
      // The ball can only be returned by pushing the button in the players zone and if the ball passed the ring already
      if (ball.currentLEDObject == players[ball.direction_to].side && ball.position<=PLAYERZONE+0.5 && (players[ball.direction_to].button.down) && ringPassed) {
        if(soundActivated) tone(BUZZER_PIN, 500);
        ringPassed = false;
        ringRoundRandomValue = random(0,maxRingRounds+1);
        ringRoundCounter = 0;
        ball.speed = low_speed + ((high_speed - low_speed) * ((PLAYERZONE-ball.position)/PLAYERZONE )) ;
        if(gameMode == 3 || gameMode == 4 || gameMode == 5 ) {
          ringDirection = random(0,2);
        }
        nextAlivePlayer = getNextAlivePlayer(players[ball.direction_to].side);
        //Serial.print("nextAlivePlayer "+ nextAlivePlayer);  Serial.println(nextAlivePlayer); 
        ball.direction_from = ball.direction_to;
        ball.direction_to = players[nextAlivePlayer].side;
        //Serial.print("Returned from : ");
        //Serial.print(ball.direction_from);
        //Serial.print(" to: ");
        //Serial.println(ball.direction_to);
      }
    //}
     //Serial.print("Ball Position: ");
     //Serial.print(sides[ball.currentLEDObject]);
     //Serial.print(" - ");
     //Serial.println(ball.position);
    
    if(ball.currentLEDObject==ball.direction_from && !ringPassed) { //ball is on the way back to the middle
      //Serial.print("back to the middle "); Serial.println(ball.position);
      ball.position = ball.position + moveBy;
      if(ball.position > PIXELS_PER_STRIP) { // ball needs to go to middle ring
        ledObjects[ball.currentLEDObject].setPixelColor((ball.position-moveBy), led_ring.Color(0,0,0)); //set last pixel of old strip to black
        //set ball position on middle ring
        ball.currentLEDObject = MIDDLE;
        ringPassed = true;
        if(ringDirection==1) {
          if(ringRoundCounter<ringRoundRandomValue) {
            ball.position = ledRingWays[ball.direction_from][ball.direction_from][0] - (ball.position-PIXELS_PER_STRIP);
          } else {
            ball.position = ledRingWays[ball.direction_to][ball.direction_from][0] - (ball.position-PIXELS_PER_STRIP);
          }
          
        } else {
          ball.position = 1 - (PIXELS_PER_STRIP-ball.position);
          ball.position = ball.position + moveBy;
        }
      }
    } else if(ball.currentLEDObject == MIDDLE) {
      //Serial.print("ball on middle "); Serial.println(ball.position);
      if(ringDirection == 1) {
        ball.position = ball.position - moveBy;
        if(ringRoundCounter<ringRoundRandomValue){ //new round
          if(ball.position <= 0) {
            if(ringRoundCounter+1==ringRoundRandomValue){ //last round)
              ball.position = ledRingWays[ball.direction_to][ball.direction_from][0] + ball.position;
            } else {
              ball.position = ledRingWays[ball.direction_from][ball.direction_from][0] + ball.position;
            }
            ringRoundCounter++;
          }
        } else {
          if(ball.position <= 0) { //ball goes to strip of to-direction
            ledObjects[ball.currentLEDObject].setPixelColor((ball.position+moveBy), led_ring.Color(0,0,0)); //set last pixel of LED ring to black
            ball.position = PIXELS_PER_STRIP  + ball.position;
            ball.currentLEDObject = ball.direction_to;
          }
        }
      } else {
        ball.position = ball.position + moveBy;
        if(ringRoundCounter<ringRoundRandomValue){
          if(ball.position >= ledRingWays[ball.direction_from][ball.direction_from][0]) { //new round
            ball.position = 1 + moveBy;
            ringRoundCounter++;
          }
        } else {
          if(ball.position > ledRingWays[ball.direction_from][ball.direction_to][0]+1) { //ball goes to strip of to-direction
            ledObjects[ball.currentLEDObject].setPixelColor((ball.position-moveBy), led_ring.Color(0,0,0)); //set last pixel of LED ring to black
            ball.currentLEDObject = ball.direction_to;
            ball.position = PIXELS_PER_STRIP - (ball.position - ledRingWays[ball.direction_from][ball.direction_to][0]) - moveBy;
          }
        }
      }
    } else { //ball is already on new LED object, move towards playerzone
      //Serial.print("ball on target "); Serial.println(ball.position);
      ball.position = ball.position - moveBy;
      if (ball.position <= 0 ) //ball got over the last pixel -->lose life
      {
        loseLife();
      }
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

  randomSeed(analogRead(A0)*analogRead(A3));

  brightness_control = EEPROM.read(EEPROM_ADRESS_BRIGHTNESS);
  if(brightness_control > 7) { //initial setting
    brightness_control = 3;
    EEPROM.update(EEPROM_ADRESS_BRIGHTNESS, brightness_control);
  }
  low_speed = EEPROM.read(EEPROM_ADRESS_LOW_SPEED);
  if(low_speed == 255) { //initial setting
    low_speed = 30;
    EEPROM.update(EEPROM_ADRESS_LOW_SPEED, low_speed);
  }
  high_speed = EEPROM.read(EEPROM_ADRESS_HIGH_SPEED);
  if(high_speed == 255) { //initial setting
    high_speed = 90;
    EEPROM.update(EEPROM_ADRESS_HIGH_SPEED, high_speed);
  }
  gameMode = EEPROM.read(EEPROM_ADRESS_GAME_MODE);
  if(gameMode > 6) { //initial setting
    gameMode = 1;
    EEPROM.update(EEPROM_ADRESS_GAME_MODE, gameMode);
  }
  maxRingRounds = EEPROM.read(EEPROM_ADRESS_MAX_RING_ROUNDS);
  if(maxRingRounds > 16) { //initial setting
    maxRingRounds = 0;
    EEPROM.update(EEPROM_ADRESS_MAX_RING_ROUNDS, maxRingRounds);
  }
  selfProbability = EEPROM.read(EEPROM_ADRESS_SELF_PROBABILITY);
  if(selfProbability > 16) { //initial setting
    selfProbability = 16;
    EEPROM.update(EEPROM_ADRESS_SELF_PROBABILITY, selfProbability);
  }
  byte soundActivatedRead = EEPROM.read(EEPROM_ADRESS_SOUND);
  if(soundActivatedRead > 1) { //initial setting
    soundActivatedRead = 1;
    EEPROM.update(EEPROM_ADRESS_SOUND, soundActivatedRead);
  }
  soundActivated = soundActivatedRead == 1 ? true : false;

  initGameSettings();
//Initialize players buttons and strips
  for (byte i = 0; i < NUMBER_OF_PLAYERS; i = i + 1) {
    pinMode(players[i].button.pin, INPUT_PULLUP);
    ledObjects[players[i].side].begin();
    //ledObjects[players[i].side].setBrightness(255);
    ledObjects[players[i].side].show();
    
    setAllTo(players[i].side, led_ring.Color(0, 0, 0)); //necessary?
  }
//initialize LED ring
  led_ring.begin();
  //led_ring.setBrightness(255);
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
      if(soundActivated) noTone(BUZZER_PIN);
      updateBall(td);
      if (!quit) {
        drawGame();
      }
      
    }
    else if (menuMode) {
      drawMenu();
    } else 
    { // the game starts if 2 or more players hold down the button and at least one releases it
      if (alivePlayers > 1 && (players[LEFT].button.up || players[UP].button.up || players[RIGHT].button.up || players[DOWN].button.up))
      {
        quit = false;
        ringPassed = false;
        if(randomGameMode){
          gameMode = random(1,6);
        }
        switch (gameMode) {
          case 1:
            ringDirection = 0;
            break;
          case 2:
            ringDirection = 1;
            break;
          default:
            break;
        }
        if(gameMode == 3 || gameMode == 4 || gameMode == 5 ) {
          ringDirection = random(0,2);
        }
        if(players[LEFT].lifes > 0 && players[LEFT].button.up) { //left player is alive and released the button first
          ball.direction_from = LEFT;
          ball.currentLEDObject = LEFT;
          initPlayers();
          ball.direction_to = getNextAlivePlayer(LEFT);
          isRestartBall = true;
        } else if(players[UP].lifes > 0 && players[UP].button.up) { //upper player is alive and released the button first
          ball.direction_from = UP;
          ball.currentLEDObject = UP;
          ball.direction_to = getNextAlivePlayer(UP);
          isRestartBall = true;
        } else if(players[RIGHT].lifes > 0 && players[RIGHT].button.up) { //right player is alive and released the button first
          ball.direction_from = RIGHT;
          ball.currentLEDObject = RIGHT;
          ball.direction_to = getNextAlivePlayer(RIGHT);
          isRestartBall = true;
        } else if(players[DOWN].lifes > 0 && players[DOWN].button.up) { //down player is alive and released the button first
          ball.direction_from = DOWN;
          ball.currentLEDObject = DOWN;
          ball.direction_to = getNextAlivePlayer(DOWN);
          isRestartBall = true;
        }
      } else {
        //Check if a new player joined or the first player released the button again
        for (byte i = 0; i < NUMBER_OF_PLAYERS; i = i + 1) {
          if(alivePlayers == 1 && players[i].button.up && players[i].lifes > 0) { // if the only player that previously held the button down releases it, remove him
            removeAlivePlayer(sides[i]);
            players[i].lifes = 0;
            Serial.println("All players left");
            if(i==LEFT) {
              menuActivationCounter++;
              if(menuActivationCounter == 3){
                menuMode = 1;
                menuActivationCounter = 0;
                ledObjects[LEFT].setPixelColor(0,0,0,0);
                ledObjects[LEFT].setPixelColor(1,0,0,0);
                ledObjects[LEFT].show();
              }
            }
          }
          if(players[i].button.down && players[i].lifes == 0) { // if player pressed the button just now, get him in the game
            appendAlivePlayer(sides[i]);
            players[i].lifes = INITIAL_LIFES;
            Serial.print("Added player: ");
            Serial.println(i);
            if(i!=LEFT) {
              menuActivationCounter = 0;
              ledObjects[LEFT].setPixelColor(0,0,0,0);
              ledObjects[LEFT].setPixelColor(1,0,0,0);
              ledObjects[LEFT].show();
            }
          }
        }
        drawStandby();
      }
      

    }
  }
}
