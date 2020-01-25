#include <Adafruit_NeoPixel.h>

#define PIXELS 42
#define SPEEDUP 100
#define LED_PIN 2
#define BUTTON1_PIN A3
#define BUTTON1_GND_PIN A2
#define BUTTON2_PIN A1
#define BUTTON2_GND_PIN A0

#define DEBUG
#define INITIALSPEED 13
#define INITIAL_LIVES 5


#define ZONEPLAYER1 8-1
#define ZONEPLAYER2 PIXELS-8
Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

typedef enum {LEFT, RIGHT} side_type;

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
};

struct Ball {
  uint32_t color;
  float position;
  side_type direction;
  float speed; // pixels/s
};

struct Button button1;
struct Button button2;
struct Player p1;
struct Player p2;
struct Ball ball;

// Fill the dots one after the other with a color
void setAllTo(uint32_t color) {
  for (uint16_t i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, color);
  }
}

void setup() {

#ifdef DEBUG
  Serial.begin(9600);
#endif

  // initialize virtual GND Pins
  pinMode(BUTTON1_GND_PIN, OUTPUT);
  pinMode(BUTTON2_GND_PIN, OUTPUT);

  digitalWrite(BUTTON1_GND_PIN, LOW);
  digitalWrite(BUTTON2_GND_PIN, LOW);

  button1 = (Button) {
    .pin = BUTTON1_PIN
  };

  button2 = (Button) {
    .pin = BUTTON2_PIN
  };

  pinMode(button1.pin, INPUT_PULLUP);
  pinMode(button2.pin, INPUT_PULLUP);

  strip.begin();
  // Range from 0-255, so 100 is a bit less than 50% brightness
  strip.setBrightness(40);
  strip.show();

  ball = (Ball) {
    .color = strip.Color(255, 0, 0),
     .position = PIXELS / 2,
      .direction = LEFT,
       .speed = INITIALSPEED
  };

  p1 = (Player) {
    .color = strip.Color(255, 80, 0),
     .lives = INITIAL_LIVES,
      .side = LEFT,
  };

  p2 = (Player) {
    .color = strip.Color(0, 0, 255),
     .lives = INITIAL_LIVES,
      .side = RIGHT
  };

  setAllTo(strip.Color(0, 0, 0));
}



void renderPlayer(Player *p) {
  int maxRight = PIXELS - 1;

  if (p->side == RIGHT) {
    for (int i = 0; i < p->lives; i++) {
      strip.setPixelColor((maxRight - i), p->color);
    }
    return;
  }

  if (p->side == LEFT) {
    for (int i = 0; i < p->lives; i++) {
      strip.setPixelColor(i, p->color);
    }
    return;
  }
  Serial.println("Player1:");
}

void renderBall(Ball *ball) {
  strip.setPixelColor((ball->position), ball->color);
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
  /*
    if (button->pin == 10) {
      if (state) {
        strip.setPixelColor(54, strip.Color(255, 255, 255));
      } else {
        strip.setPixelColor(54, strip.Color(0, 0, 0));
      }
    } else if (button->pin == 12) {
      if (state) {
        strip.setPixelColor(55, strip.Color(255, 255, 255));
      } else {
        strip.setPixelColor(55, strip.Color(0, 0, 0));
      }
    }
  */
}

void processInput() {
  processButtonInput(&button1);
  processButtonInput(&button2);
}

void colorWipe(uint32_t c, uint8_t wait) {
  for (uint16_t i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
    strip.show();
    delay(wait);
  }
}
void drawWinnerR() {

  if (p2.lives == 1) {
    colorWipe(p1.color , 100);
  }
}

void drawWinnerL() {


if (p1.lives == 1) {
  colorWipe(p2.color , 100);
}

}

void drawGame()
{
  setAllTo(strip.Color(0, 0, 0));
  renderPlayer(&p1);
  renderPlayer(&p2);
  renderBall(&ball);

  strip.setPixelColor(ZONEPLAYER1, 50, 50, 50);
  strip.setPixelColor(ZONEPLAYER2, 50, 50, 50);
  strip.show();
  Serial.println("Player1:");
  Serial.println(p1.lives);
  Serial.println("Player2:");
  Serial.println(p2.lives);
}

void drawLostAnimation()
{
  setAllTo(strip.Color(255, 255, 255));
  strip.show();
  delay(200);
  setAllTo(strip.Color(0, 0, 0));
  strip.show();
}

void drawStrobo()
{
  static uint8_t pause = 0;
  if (pause < 1)
  {
    setAllTo(strip.Color(255, 0, 0));
    pause++;
  }
  else if (pause < 2)
  {
    setAllTo(strip.Color(0, 255, 0));
    pause++;
  }
  else if (pause < 3)
  {
    setAllTo(strip.Color(0, 0, 255));
    pause = 0;
  }
  strip.show();
}

uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if (WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}





void drawStandby()
{
  colorWipe(strip.Color(random(0, 255), random(0, 255), random(0, 255)), 10);
}

void updateBall(Ball *ball, unsigned int td) {
  float moveBy = ball->speed * (td / (float) 1000);

  if ((button1.down || button1.up) && ball->direction == LEFT) {

    //----Man kann den Ball erst zurueckschicken, wenn er sich bis auf 1/4 des Stripes der eigenen Zone genaehert hat.
    //----Sonst wird es langweilig, weil man sich durch "Dauerfeuer" gegenseitig im Mittelfeld halten kann.
    if(ball->position<ZONEPLAYER1){
      ball->speed = SPEEDUP / ball->position ;
      ball->direction = RIGHT;
    }else{
      /*
      drawLostAnimation();
        drawWinnerL();
        p1.lives = p1.lives - 1;
        ball->direction = LEFT;
        ball->position = PIXELS / 2; //ball->position * -1;
        ball->speed = INITIALSPEED;
      */
    }
  }
  
  if ((button2.down || button2.up) && ball->direction == RIGHT) {
    if(ball->position>ZONEPLAYER2){
      ball->speed = SPEEDUP / (PIXELS - ball->position);
      ball->direction = LEFT;
    }else{
      /*
      drawLostAnimation();
        drawWinnerR();
        p2.lives = p2.lives - 1;
        ball->direction = RIGHT;
        ball->position = PIXELS / 2;//(float) (PIXELS - 1) - ((float) (PIXELS - 1) - ball->position);
        ball->speed = INITIALSPEED;
      */
    }
  }

  switch (ball->direction) {
    case LEFT:
      ball->position = ball->position - moveBy;
      if (ball->position < 0)
      {
        drawLostAnimation();
        drawWinnerL();
        p1.lives = p1.lives - 1;
        ball->direction = LEFT;
        ball->position = PIXELS / 2; //ball->position * -1;
        ball->speed = INITIALSPEED;
      }
      break;
    case RIGHT:
      ball->position = ball->position + moveBy;
      if (ball->position > PIXELS - 1)
      {
        drawLostAnimation();
        drawWinnerR();
        p2.lives = p2.lives - 1;
        ball->direction = RIGHT;
        ball->position = PIXELS / 2;//(float) (PIXELS - 1) - ((float) (PIXELS - 1) - ball->position);
        ball->speed = INITIALSPEED;
      }
      break;
  }
}

void update(unsigned int td)
{
  updateBall(&ball, td);
  if (0 == p1.lives)
  {
    p1.lives = INITIAL_LIVES;
    p2.lives = INITIAL_LIVES;
    quit = true;
  }
  if (0 == p2.lives)
  {
    p1.lives = INITIAL_LIVES;
    p2.lives = INITIAL_LIVES;
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
    {
      if ((button1.down || button1.up) || (button2.down || button2.up))
      {
        quit = false;
      }
      drawStandby();

    }
  }
}