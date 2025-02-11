#include <Adafruit_NeoPixel.h>

#define TESTING_MODE false
//NeoPixel pin
#define PIN 13
//NeoPixel settings
#define NUMPIXELS 4
#define BRIGHTNES_LEVEL 20

//Motor pins
#define MOT_A1 10
#define MOT_A2 11
#define MOT_B1 6
#define MOT_B2 5
#define MOT_R1 2
#define MOT_R2 3

//Distance sersor pins
#define trigPin 8
#define echoPin 12

// Distance constants
#define CLOSE 20
#define NORMAL 30
#define FAR 100

//Gripper pin
#define GRIP 9

//Turns
#define TURN_90_LEFT 38
#define TURN_90_RIGHT 37

//Movement
#define MOTOR_TURN_SPEED 124
#define CHECK_STRAIGT_LINE_MOVEMENT 6

#define MOTOR_A_SPEED 225 //left
#define MOTOR_B_SPEED 219 //right

#define MOTOR_A_SLOW_SPEED 207
#define MOTOR_B_SLOW_SPEED 218

#define MOTOR_A_SLOW_TURN_SPEED 150
#define MOTOR_B_SLOW_TURN_SPEED 150

// Delay
#define DelayValue 200

int BLACK_LIMIT = 800;

Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

const int sensorPins[] = { A0, A1, A2, A3, A4, A5, A6, A7 };
int sensor_A0, sensor_A1, sensor_A2, sensor_A3, sensor_A4, sensor_A5, sensor_A6, sensor_A7;

//snoop dogg state
bool started = false;
bool solved = false;
bool ended = false;

//Wheel rotation state
volatile int countL = 0;
volatile int countR = 0;

void ISR_L() {
  countL++;
}

void ISR_R() {
  countR++;
}

void setup() {
  Serial.begin(9600);
  pixels.begin();

  pinMode(MOT_A1, OUTPUT);
  pinMode(MOT_A2, OUTPUT);
  pinMode(MOT_B1, OUTPUT);
  pinMode(MOT_B2, OUTPUT);
  pinMode(MOT_R1, INPUT_PULLUP);
  pinMode(MOT_R2, INPUT_PULLUP);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  pinMode(GRIP, OUTPUT);
  digitalWrite(GRIP, LOW);

  for (int i = 0; i < 8; i++) {
    pinMode(sensorPins[i], INPUT);
  }
//  Serial.println("hello"); //make when do the line sensors testing easier to read number
  attachInterrupt(digitalPinToInterrupt(MOT_R1), ISR_R, CHANGE);
  attachInterrupt(digitalPinToInterrupt(MOT_R2), ISR_L, CHANGE);
}


void loop() {
  if (TESTING_MODE) {
    started = true;
  }

 //checking the line sensors
// read();
//  Serial.print(sensor_A0);
//   Serial.print(" ");
//  Serial.print(sensor_A1);
//   Serial.print(" "); 
//  Serial.print(sensor_A2);
//   Serial.print(" ");
//  Serial.print(sensor_A3);
//   Serial.print(" ");
//  Serial.print(sensor_A4);
//   Serial.print(" ");
//  Serial.print(sensor_A5);
//   Serial.print(" ");
//  Serial.print(sensor_A6);
//    Serial.print(" ");
//  Serial.print(sensor_A7);
//    Serial.println(" ");

  //Game logic
  if (!started) {
    start();
  } else if (!solved) {
    maze();
  } else if (!ended) {
    end();
  }
}

//Running when obstacle apper and calibrate black limit
void start() {
  //Check is obstacle appear
  int distance = culculateDistance();
  Serial.println(distance);
  int countToStart = 0;
  while (distance >= 30 || countToStart < 5) {
    distance = culculateDistance();
    Serial.println(distance);
    if (distance > 30) {
      countToStart = 0;
    } else {
      countToStart++;
    }
  }
  activationWait();

  //Calibration for black limit
  int blackLimit[3];
  int currentIndex = 0;
  int color;

  for (int i = 0; i < 6; i++) {
    stop();
    delay(100);
    int curentColor = getAverageLightLevel();
    delay(100);
    goStraightSlow();

    color = curentColor;
    while (color > curentColor - 300 && color < curentColor + 300) {
      color = getAverageLightLevel();
    }
    if (i % 2 == 1) {
      Serial.println(curentColor);
      blackLimit[currentIndex] = curentColor;
      currentIndex++;
    }
  }
  stop();
  delay(1000);
//
//  Adjust movement
//   while (!isAllSensors()) {
//     read();
//     if (sensor_A2 >= BLACK_LIMIT) {
//       smallTurnRight();
//     } else if (sensor_A5 >= BLACK_LIMIT) {
//       smallTurnLeft();
//     } else {
//       goStraightSlow();
//     }
//   }
  startMovementAdjustment();

  grab();
  stop();
  delay(1000);

  goStraight(6);
  turnLeft(TURN_90_LEFT);
  delay(100);

  started = true;
}

//Solve the maze
void maze() {
  read();
  if (isLeftSensors()) {
    stop();
    delay(10);
    read();
  }

  if (isRightSensors()) {
    goStraight(CHECK_STRAIGT_LINE_MOVEMENT);
    read();
    if (isAllSensors()) {
      solved = true;
    } else {
      turnRight(TURN_90_RIGHT);
      delay(DelayValue);
    }

  } else if (isLeftSensors()) {
    goStraight(CHECK_STRAIGT_LINE_MOVEMENT);
    read();
    if (!isCenterAndOneRightSensors()) {
      turnLeft(TURN_90_LEFT);
      delay(DelayValue);
    }
  } else if (isNoSensors()) {
    goStraight(8);
    turnRightUltra();
    delay(DelayValue);
  } else if (sensor_A2 >= BLACK_LIMIT) {
    smallTurnRight();
  } else if (sensor_A5 >= BLACK_LIMIT) {
    smallTurnLeft();
  } else {
    goStraight();
  }
}

//Finish maze solving and ungrab obstacle
void end() {
  delay(500);
  goBack(5);
  ungrab();
  goBack(30);
  while (true) {
    setPixlsRed();
    delay(200);
    setPixlsYellow();
    delay(200);
    setPixlsGreen();
    delay(200);
  }
  ended = true;
}
// Read values from sensor
void read() {
  sensor_A0 = analogRead(A0);
  sensor_A1 = analogRead(A1);
  sensor_A2 = analogRead(A2);
  sensor_A3 = analogRead(A3);
  sensor_A4 = analogRead(A4);
  sensor_A5 = analogRead(A5);
  sensor_A6 = analogRead(A6);
  sensor_A7 = analogRead(A7);
}

// Get average value from all sensors
int getAverageLightLevel() {
  read();
  return (sensor_A0 + sensor_A1 + sensor_A2 + sensor_A3 + sensor_A4 + sensor_A5 + sensor_A6 + sensor_A7) / 8;
}

// Calculate black limit from array of light levels
int getAverageBlackLimit(int* array) {
  int res = 0;
  for (int i = 0; i < 3; i++) {
    res += array[i];
  }
  return res / 3;
}

// Check for sensors on line
bool isAllSensors() {
  return (aboveBlackLimit(sensor_A0) && aboveBlackLimit(sensor_A1) && aboveBlackLimit(sensor_A2) && aboveBlackLimit(sensor_A3) && aboveBlackLimit(sensor_A4) && aboveBlackLimit(sensor_A5) && aboveBlackLimit(sensor_A6) && aboveBlackLimit(sensor_A7));
}

bool isRightSensors() {
  return (aboveBlackLimit(sensor_A0) && aboveBlackLimit(sensor_A1) && aboveBlackLimit(sensor_A2)) || (aboveBlackLimit(sensor_A0) && aboveBlackLimit(sensor_A1)) || aboveBlackLimit(sensor_A0);
}

bool isLeftSensors() {
  return (aboveBlackLimit(sensor_A5) && aboveBlackLimit(sensor_A6) && aboveBlackLimit(sensor_A7)) || (aboveBlackLimit(sensor_A6) && aboveBlackLimit(sensor_A7));
}

bool isNoSensors() {
  return BelowBlackLimit(sensor_A0) && BelowBlackLimit(sensor_A1) && BelowBlackLimit(sensor_A2) && BelowBlackLimit(sensor_A3) && BelowBlackLimit(sensor_A4) && BelowBlackLimit(sensor_A5) && BelowBlackLimit(sensor_A6) && BelowBlackLimit(sensor_A7);
}

bool isCenterSensors() {
  return aboveBlackLimit(sensor_A3) || aboveBlackLimit(sensor_A4);
}

bool isCenterAndOneRightSensors() {
  return aboveBlackLimit(sensor_A3) || aboveBlackLimit(sensor_A4)|| aboveBlackLimit(sensor_A2);
}
bool aboveBlackLimit(int sensor) {
  return sensor >= BLACK_LIMIT;
}

bool BelowBlackLimit(int sensor) {
  return sensor <= BLACK_LIMIT;
}


void startMovementAdjustment() {
  while (!isAllSensors()) {
    read();
    if        (aboveBlackLimit(sensor_A1)) {
      smallTurnRight(210);
    } else if (aboveBlackLimit(sensor_A6)) {
      smallTurnLeft(210);
    } else if (aboveBlackLimit(sensor_A0)){
      smallTurnRight(160);
    } else if (aboveBlackLimit(sensor_A7)){
      smallTurnLeft(160);
    } else if (aboveBlackLimit(sensor_A2)) {
      smallTurnRight();
    } else if (aboveBlackLimit(sensor_A5)) {
      smallTurnLeft();
    }else {
      goStraightSlow();
    }
  }
}


//Calculate distance from distance sensor
int culculateDistance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH);
  return duration * 0.034 / 2;
}

// Grab with servo
void grab() {
  Serial.println("grab");
  for (int i = 0; i < 15; i++) {
    digitalWrite(GRIP, HIGH);
    delayMicroseconds(1000);
    digitalWrite(GRIP, LOW);
    delayMicroseconds(19000);
  }
}

// Ungrab with servo
void ungrab() {
  Serial.println("ungrab");
  for (int i = 0; i < 15; i++) {
    digitalWrite(GRIP, HIGH);
    delayMicroseconds(1500);
    digitalWrite(GRIP, LOW);
    delayMicroseconds(18500);
  }
}

//Movement
void goStraight() {
  setPixlsGreen();
  analogWrite(MOT_A2, MOTOR_A_SPEED);
  analogWrite(MOT_B2, MOTOR_B_SPEED);
  analogWrite(MOT_A1, LOW);
  analogWrite(MOT_B1, LOW);
}

void goStraightSlow() {
  setPixlsGreen();
  analogWrite(MOT_A2, MOTOR_A_SLOW_SPEED);
  analogWrite(MOT_B2, MOTOR_B_SLOW_SPEED);
  analogWrite(MOT_A1, LOW);
  analogWrite(MOT_B1, LOW);
}

void goStraight(int d) {
  setPixlsGreen();
  countL = 0;
  countR = 0;

  while (countR < d) {
    analogWrite(MOT_A2, MOTOR_A_SPEED);
    analogWrite(MOT_B2, MOTOR_B_SPEED);
    analogWrite(MOT_A1, LOW);
    analogWrite(MOT_B1, LOW);
  }
  stop();
}

void goBack(int d) {
  setPixlsGreen();
  countL = 0;
  countR = 0;

  while (countR < d) {
    analogWrite(MOT_A1, MOTOR_A_SPEED);
    analogWrite(MOT_B1, MOTOR_B_SPEED);
    analogWrite(MOT_A2, LOW);
    analogWrite(MOT_B2, LOW);
  }
  stop();
}

void stop() {
  analogWrite(MOT_A1, LOW);
  analogWrite(MOT_B1, LOW);
  analogWrite(MOT_A2, LOW);
  analogWrite(MOT_B2, LOW);
}

//Small Turning
void smallTurnLeft() {
  analogWrite(MOT_A2, MOTOR_TURN_SPEED);
  analogWrite(MOT_B2, MOTOR_B_SPEED);
}

void smallTurnRight() {
  analogWrite(MOT_B2, MOTOR_TURN_SPEED);
  analogWrite(MOT_A2, MOTOR_A_SPEED);
}

void smallTurnLeft(int speed) {
  analogWrite(MOT_A2, speed);
  analogWrite(MOT_B2, MOTOR_B_SPEED);
}

void smallTurnRight(int speed) {
  analogWrite(MOT_B2, speed);
  analogWrite(MOT_A2, MOTOR_A_SPEED);
}


void turnLeft(int d) {
  setPixlsYellow();
  countL = 0;
  countR = 0;

  while (countL < d) {
    analogWrite(MOT_B2, MOTOR_B_SPEED);
    analogWrite(MOT_A1, LOW);
    analogWrite(MOT_A2, LOW);
    analogWrite(MOT_B1, LOW);
  }
  stop();
}

void turnRight(int d) {
  setPixlsYellow();
  countL = 0;
  countR = 0;

  while (countR < d) {
    analogWrite(MOT_A2, MOTOR_A_SPEED);
    analogWrite(MOT_A1, LOW);
    analogWrite(MOT_B1, LOW);
    analogWrite(MOT_B2, LOW);
  }
  stop();
}

void turnRightUltra() {
  setPixlsRed();
  fullTurnRight();
  while (true) {
    read();
    if (isCenterSensors()) {
      stop();
      break;
    }
  }
  stop();
  fullTurnLeft();
  delay(120);
  stop();
}

void fullTurnRight() {
  analogWrite(MOT_B1, MOTOR_B_SPEED);
  analogWrite(MOT_A2, MOTOR_A_SPEED);
  analogWrite(MOT_A1, LOW);
  analogWrite(MOT_B2, LOW);
}


void fullTurnRightSlow() {
  analogWrite(MOT_B1, MOTOR_B_SLOW_TURN_SPEED);
  analogWrite(MOT_A2, MOTOR_A_SLOW_TURN_SPEED);
  analogWrite(MOT_A1, LOW);
  analogWrite(MOT_B2, LOW);
}



void fullTurnLeft() {
  analogWrite(MOT_B2, MOTOR_B_SPEED);
  analogWrite(MOT_A1, MOTOR_A_SPEED);
  analogWrite(MOT_B1, LOW);
  analogWrite(MOT_A2, LOW);
}

// Shows robot initialization visualy
void activationWait() {
  pixels.setPixelColor(0, pixels.Color(0, BRIGHTNES_LEVEL, 0));
  pixels.show();
  delay(1000);
  pixels.setPixelColor(1, pixels.Color(BRIGHTNES_LEVEL, BRIGHTNES_LEVEL, 0));
  pixels.show();
  delay(1000);
  pixels.setPixelColor(2, pixels.Color(BRIGHTNES_LEVEL, 0, 0));
  pixels.show();
  delay(1000);
  pixels.setPixelColor(3, pixels.Color(BRIGHTNES_LEVEL, 0, 0));
  pixels.show();
  delay(1000);
  setPixlsGreen();
}

//Lights
void setPixlsRed() {
  pixels.setPixelColor(0, pixels.Color(0, BRIGHTNES_LEVEL, 0));
  pixels.setPixelColor(1, pixels.Color(0, BRIGHTNES_LEVEL, 0));
  pixels.setPixelColor(2, pixels.Color(0, BRIGHTNES_LEVEL, 0));
  pixels.setPixelColor(3, pixels.Color(0, BRIGHTNES_LEVEL, 0));
  pixels.show();
}

void setPixlsGreen() {
  pixels.setPixelColor(0, pixels.Color(BRIGHTNES_LEVEL, 0, 0));
  pixels.setPixelColor(1, pixels.Color(BRIGHTNES_LEVEL, 0, 0));
  pixels.setPixelColor(2, pixels.Color(BRIGHTNES_LEVEL, 0, 0));
  pixels.setPixelColor(3, pixels.Color(BRIGHTNES_LEVEL, 0, 0));
  pixels.show();
}

void setPixlsYellow() {
  pixels.setPixelColor(0, pixels.Color(BRIGHTNES_LEVEL, BRIGHTNES_LEVEL, 0));
  pixels.setPixelColor(1, pixels.Color(BRIGHTNES_LEVEL, BRIGHTNES_LEVEL, 0));
  pixels.setPixelColor(2, pixels.Color(BRIGHTNES_LEVEL, BRIGHTNES_LEVEL, 0));
  pixels.setPixelColor(3, pixels.Color(BRIGHTNES_LEVEL, BRIGHTNES_LEVEL, 0));
  pixels.show();
}

void setPixlsBlue() {
  pixels.setPixelColor(0, pixels.Color(0, 0, BRIGHTNES_LEVEL));
  pixels.setPixelColor(1, pixels.Color(0, 0, BRIGHTNES_LEVEL));
  pixels.setPixelColor(2, pixels.Color(0, 0, BRIGHTNES_LEVEL));
  pixels.setPixelColor(3, pixels.Color(0, 0, BRIGHTNES_LEVEL));
  pixels.show();
}
