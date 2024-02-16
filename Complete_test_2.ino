#include <SoftwareSerial.h>
#include "VoiceRecognitionV3.h"
#include <math.h>

#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

Adafruit_PWMServoDriver Servos = Adafruit_PWMServoDriver(0x40);

#define servoMIN 102
#define servoMAX 512
//Servo Pins
#define Left_Right_Pin  (12) 
#define Up_Down_Pin     (13) 
#define Rotate_Pin      (14) 

// object defination
VR myVR(2,3);    // TX to pin 2; RX to pin 3; Bluetooth use the hardware TX and RX pins
const int Bluetooth_statePin = 9; // Check the bluetooth connection status


// data storage
uint8_t record[6]; // save record
uint8_t buf[64];
String content = "";
char datarcv;
int centerX = 360;
int centerY = 753;
int difference_X = 0;
int difference_Y = 0;
int tracking_Sensitivity = 35;
int pulseLength_Leftright = 0;
int pulseLength_Updown = 0;
int pulseLength_Rotate = 0;

// Flags
int level = 0;
bool moveUp = false;
bool moveDown = false;
bool moveLeft = false;
bool moveRight = false;
bool horizental = false;
bool searching = false; // Test
bool X_Central = false;
bool Y_Central = false;

// initial angle
int up_down_angle = 44;
int left_right_angle = 63;
int rotate_angle = 90;

// speed control: the larger the slower
int up_down_Speed = 30;
int left_right_Speed = 30;
int rotate_Speed = 20;
int search_Speed = 100;

// Macro definations
//Level 0
#define level0Alice      (0) 
// Level 1
#define level1CMD1       (2) // Take photo
#define level1CMD2       (3) // Move
#define level1CMD3       (4) // Find
#define level1CMD4       (5) // Rotate
#define level1CMD5       (10) // Alblum
#define level1CMD6       (11) // Tap center
// Level 2
#define level2CMD1       (1) // Stop
#define level2CMD2       (6) // Up
#define level2CMD3       (7) // Down
#define level2CMD4       (8) // Left
#define level2CMD5       (9) // Right



struct Coordinate {
  String type;
  String value;
};


void setup()
{
  /** initialize */
  myVR.begin(9600);
  Servos.begin();
  Servos.setPWMFreq(50);  // This is the maximum PWM frequency
  pinMode(Bluetooth_statePin, INPUT);
  Serial.begin(9600);
  pulseLength_Leftright = map(left_right_angle, 0, 180, servoMIN, servoMAX); // Map the angle to the according pulse length
  Servos.setPWM(Left_Right_Pin, 0, pulseLength_Leftright);
  pulseLength_Updown = map(up_down_angle, 0, 180, servoMIN, servoMAX); // Map the angle to the according pulse length
  Servos.setPWM(Up_Down_Pin, 0, pulseLength_Updown);
  pulseLength_Rotate = map(rotate_angle, 0, 180, servoMIN, servoMAX); // Map the angle to the according pulse length
  Servos.setPWM(Rotate_Pin, 0, pulseLength_Rotate);

  if(myVR.clear() == 0){
    Serial.println("System Ready!");
  }else{
    Serial.println("Not find VoiceRecognitionModule.");
    while(1);
  }
  myVR.load(uint8_t (0));  // load the Alice
  level = 0;
}

void loop()
{ 
  int ret;
  ret = myVR.recognize(buf, 50);
  if(ret>0){
    switch(buf[1]){

      case level0Alice: // hello Alice
        if(level == 0){
          level = 1;
          myVR.clear();
          record[0] = level1CMD1;  // Take photo
          record[1] = level1CMD2;  // Move
          record[2] = level1CMD3;  // Find
          record[3] = level1CMD4;  // Rotate
          record[4] = level1CMD5;  // Alblum
          record[5] = level1CMD6;  // Tap center
          if(myVR.load(record, 6) >= 0){
            Serial.println("Alice");
          }
        }
        break;

      case level1CMD1: // Take photo 
        if(digitalRead(Bluetooth_statePin) == HIGH){ // Check the bluetooth connect
          Serial.println("Take photo!"); // Cannot be deleted!
        }else{
          Serial.println("Take photo Function unavaiable. Please connect to the phone.");
        }
        level = 0;
        myVR.clear();
        myVR.load(uint8_t (0));  // load the Alice
        break;
      
      case level1CMD2: // Move
        Serial.print("Move: ");
        if (level == 1) {
          level = 2;
          myVR.clear();
          record[0] = level2CMD1;  // Stop
          record[1] = level2CMD2;  // Up
          record[2] = level2CMD3;  // Down
          record[3] = level2CMD4;  // Left
          record[4] = level2CMD5;  // Right
          myVR.load(record, 5);
        }
        break;
      
      case level1CMD3: // Find
        if(digitalRead(Bluetooth_statePin) == HIGH){ // Check the bluetooth connect
          Serial.println("Find!"); // Cannot be deleted!
          searching = true;
        }else{
          Serial.println("Search Function unavaiable. Please connect to the phone.");
        }
        while (searching) {
          if(digitalRead(Bluetooth_statePin) == LOW){
            Serial.println("Disconnected Error!");
            searching = false;
            break;
          }
       ReceiveBluetoothMSG();
          }
        level = 0;
        myVR.clear();
        myVR.load(uint8_t (0));  // load the Alice
        break;
      
      case level1CMD4: // Rotate
        Serial.println("Rotate!");
        level = 0;
        myVR.clear();
        rotate();
        myVR.load(uint8_t (0));  // load the Alice
        break;

      case level1CMD5: // Alblum
        if(digitalRead(Bluetooth_statePin) == HIGH){ // Check the bluetooth connection
          Serial.println("Album!"); // Cannot be deleted!
        }else{
          Serial.println("Album Function unavaiable. Please connect to the phone.");
        }
        level = 0;
        myVR.clear();
        myVR.load(uint8_t (0)); // load the Alice
        break;

      case level1CMD6: // Tap center
        Serial.println("Tap center!");
        level = 0;
        up_down_angle = 34;
        left_right_angle = 63;
        rotate_angle = 90;
        pulseLength_Leftright = map(left_right_angle, 0, 180, servoMIN, servoMAX); // Map the angle to the according pulse length
        Servos.setPWM(Left_Right_Pin, 0, pulseLength_Leftright);
        pulseLength_Updown = map(up_down_angle, 0, 180, servoMIN, servoMAX); // Map the angle to the according pulse length
        Servos.setPWM(Up_Down_Pin, 0, pulseLength_Updown);
        pulseLength_Rotate = map(rotate_angle, 0, 180, servoMIN, servoMAX); // Map the angle to the according pulse length
        Servos.setPWM(Rotate_Pin, 0, pulseLength_Rotate);
        
        horizental = false;
        myVR.clear();
        myVR.load(uint8_t (0));  // load the Alice
        break;

      case level2CMD1: // Stop
        Serial.println("Stop!");
        moveDown = false;
        moveUp = false;
        moveLeft = false;
        moveRight = false;
        level = 0;
        myVR.clear();
        myVR.load(uint8_t (0));  // load the Alice
        break;

      case level2CMD2: // Move up
        Serial.println("up!");
        moveUp = true;
        moveDown = false;
        break;

      case level2CMD3: // Move Down
        Serial.println("down!");
        moveDown = true;
        moveUp = false;
        break;

      case level2CMD4: // Move left
        Serial.println("left!");
        moveLeft = true;
        moveRight = false;
        break;
      

      case level2CMD5: // Move right
        Serial.println("right!");
        moveRight = true;
        moveLeft = false;
        break;

      default:
        break;
    }
  }
  if(moveUp){
      continueMovingUp(up_down_Speed);
    }
  if(moveDown){
    continueMovingDown(up_down_Speed);
  }
  if(moveLeft){
    continueMovingLeft(left_right_Speed);
  }
  if(moveRight){
    continueMovingRight(left_right_Speed);
  }
}


void continueMovingUp(int Speed){
  if(up_down_angle < 80){
  up_down_angle ++;
  pulseLength_Updown = map(up_down_angle, 0, 180, servoMIN, servoMAX); // Map the angle to the according pulse length
  Servos.setPWM(Up_Down_Pin, 0, pulseLength_Updown);
  delay(Speed);
  }else{
    level = 0;
    moveUp = false;
    myVR.clear();
    myVR.load(uint8_t (0));  // load the Alice
  }
  
}
void continueMovingDown(int Speed){
  if(up_down_angle > 4){
  up_down_angle --;
  pulseLength_Updown = map(up_down_angle, 0, 180, servoMIN, servoMAX); // Map the angle to the according pulse length
  Servos.setPWM(Up_Down_Pin, 0, pulseLength_Updown);
  delay(Speed);
  }else{
    level = 0;
    moveDown = false;
    myVR.clear();
    myVR.load(uint8_t (0));  // load the Alice
  }
}

void continueMovingLeft(int Speed){
  if(left_right_angle < 125){
  left_right_angle ++;
  pulseLength_Leftright = map(left_right_angle, 0, 180, servoMIN, servoMAX); // Map the angle to the according pulse length
    Servos.setPWM(Left_Right_Pin, 0, pulseLength_Leftright);
  delay(Speed);
  }else{
    level = 0;
    moveLeft = false;
    myVR.clear();
    myVR.load(uint8_t (0));  // load the Alice
  }
}

void continueMovingRight(int Speed){
  if(left_right_angle > 0){
  left_right_angle --;
  pulseLength_Leftright = map(left_right_angle, 0, 180, servoMIN, servoMAX); // Map the angle to the according pulse length
  Servos.setPWM(Left_Right_Pin, 0, pulseLength_Leftright);
  delay(Speed);
  }else{
    level = 0;
    moveRight = false;
    myVR.clear();
    myVR.load(uint8_t (0));  // load the Alice
  }
}

void rotate(){
  if(!horizental){
          rotate_angle = 90;
          while(rotate_angle > 0){
          pulseLength_Rotate = map(rotate_angle, 0, 180, servoMIN, servoMAX); // Map the angle to the according pulse length
          Servos.setPWM(Rotate_Pin, 0, pulseLength_Rotate);
          rotate_angle--;
          delay(rotate_Speed);
        }
        horizental = true;
        }else{
          rotate_angle = 0;
          while(rotate_angle < 90){
          pulseLength_Rotate = map(rotate_angle, 0, 180, servoMIN, servoMAX); // Map the angle to the according pulse length
          Servos.setPWM(Rotate_Pin, 0, pulseLength_Rotate);
          rotate_angle++;
          delay(rotate_Speed);
        }
        horizental = false;
        }
}

void ReceiveBluetoothMSG(){ // Function for monitoring the bluetooth inputstream from the phone
    if(moveLeft){
      continueMovingLeft(search_Speed);
      }
    if(moveRight){
      continueMovingRight(search_Speed);
      }
    if(moveUp){
      continueMovingUp(search_Speed);
    }
    if(moveDown){
      continueMovingDown(search_Speed);
    }
    if(X_Central && Y_Central){
        searching = false;
        X_Central = false;
        Y_Central = false;
        moveUp = false;
        moveDown = false;
        moveLeft = false;
        moveRight = false;
        Serial.println("Search finish!"); // Cannot be deleted!
    }
  if (Serial.available()) {
    while (Serial.available() > 0) {
      datarcv = Serial.read();
      content.concat(datarcv);
      delay(10);  
    }
    if (datarcv == '\n') {
      processContent(content);
      content = "";
    }
  }
}

void processContent(String data) { 
// This function is used to process a string containing multiple coordinate packets.
// It splits the string into separate packets and processes each packet. 
// The loop exits when no more newline characters are found,
// indicating that all packets in the string have been processed
  int lastIndex = 0;
  int nextIndex;

  while ((nextIndex = data.indexOf('\n', lastIndex)) != -1) {
    // Extract a single packet from the string using the indices
    String packet = data.substring(lastIndex, nextIndex);
    lastIndex = nextIndex + 1;
    // Process the extracted packet
    Coordinate coord = extractCoordinate(packet);
    if (coord.type == "X") {
      int x = coord.value.toInt();
      difference_X = x - centerX;
      if(abs(difference_X) < tracking_Sensitivity){
        moveLeft = false;
        moveRight = false;
        X_Central = true;
      }

      if (difference_X < -tracking_Sensitivity) {
        moveRight = false;
        moveLeft = true;
      }

      if (difference_X > tracking_Sensitivity) {
        moveLeft = false;
        moveRight = true;
      }
      
    } else if (coord.type == "Y") {
      int y = coord.value.toInt();
      difference_Y = y - centerY;
      if(abs(difference_Y) < tracking_Sensitivity){
        moveUp = false;
        moveDown = false;
        Y_Central = true;
      }
      if(difference_Y > tracking_Sensitivity){
        moveDown = true;
        moveUp = false;
      }
      if(difference_Y < -tracking_Sensitivity){
        moveDown = false;
        moveUp = true;
      }
    }
  }
}


Coordinate extractCoordinate(String data) { // Function for extract the right X/Y coordinate value from the "X/Y $XXX !" data package received from Bluetooth
  Coordinate coord;
  int dollarIndex = data.indexOf('$') + 1;
  int exclamationIndex = data.indexOf('!', dollarIndex);

  if (dollarIndex > 1 && exclamationIndex > dollarIndex) {
    coord.type = data.substring(0, 1); // Extract the coordinate type: X or Y
    coord.value = data.substring(dollarIndex, exclamationIndex); // Extract the coordinate value
  } else {
    coord.type = "Error";
    coord.value = "Invalid format";
  }
  return coord;
}
