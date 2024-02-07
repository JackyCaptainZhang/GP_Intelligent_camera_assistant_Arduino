#include <SoftwareSerial.h>
#include "VoiceRecognitionV3.h"
#include <Servo.h>

// object defination
VR myVR(2,3);    // TX to pin 2; RX to pin 3; Bluetooth use the hardware TX and RX pins
Servo Rotate_Servo; // Pin 6
Servo Left_Right_Servo; // Pin 7
Servo Up_Down_Servo; // Pin 8

// data storage
uint8_t record[6]; // save record
uint8_t buf[64];
String content = "";
char datarcv;
String centerX = "";
String centerY = "";

// flags
int level = 0;
bool moveUp = false;
bool moveDown = false;
bool moveLeft = false;
bool moveRight = false;
bool horizental = false;
bool searching = false; // Test

// initial angle
int up_down_angle = 44;
int left_right_angle = 63;
int rotate_angle = 90;

// speed control: the larger the slower
int up_down_Speed = 30;
int left_right_Speed = 30;
int rotate_Speed = 20;

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
  Serial.begin(9600);
  if(myVR.clear() == 0){
    Serial.println("System Ready!");
  }else{
    Serial.println("Not find VoiceRecognitionModule.");
    while(1);
  }
  myVR.load(uint8_t (0));  // load the Alice
  level = 0;
  Rotate_Servo.attach(6);
  Left_Right_Servo.attach(7);
  Up_Down_Servo.attach(8);
  Rotate_Servo.write(rotate_angle);
  Left_Right_Servo.write(left_right_angle);
  Up_Down_Servo.write(up_down_angle);
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
        Serial.println("Take photo!");
        level = 0;
        // todo
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
          if(myVR.load(record, 5) >= 0){
            //printRecord(record, 5);
            //Serial.println(F("loaded."));
          }
        }
        break;
      
      case level1CMD3: // Find
        Serial.println("Find!");
        searching = true;
        // todo
        while (searching) {
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
        Serial.println("Album!");
        level = 0;
        // todo
        myVR.clear();
        myVR.load(uint8_t (0));  // load the Alice
        break;

      case level1CMD6: // Tap center
        Serial.println("Tap center!");
        level = 0;
        Rotate_Servo.write(90);
        Left_Right_Servo.write(63);
        Up_Down_Servo.write(34);
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
        startMovingUp();
        break;

      case level2CMD3: // Move Down
        Serial.println("down!");
        moveDown = true;
        startMovingDown();
        break;

      case level2CMD4: // Move left
        Serial.println("left!");
        moveLeft = true;
        startMovingLeft();
        break;
      

      case level2CMD5: // Move right
        Serial.println("right!");
        moveRight = true;
        startMovingRight();
        break;

      default:
        break;
    }
  }
  if(moveUp){
      continueMovingUp();
    }
  if(moveDown){
    continueMovingDown();
  }
  if(moveLeft){
    continueMovingLeft();
  }
  if(moveRight){
    continueMovingRight();
  }
}

/**
  @brief   Print signature, if the character is invisible, 
           print hexible value instead.
  @param   buf     --> command length
           len     --> number of parameters
*/
void printSignature(uint8_t *buf, int len)
{
  int i;
  for(i=0; i<len; i++){
    if(buf[i]>0x19 && buf[i]<0x7F){
      Serial.write(buf[i]);
    }
    else{
      Serial.print("[");
      Serial.print(buf[i], HEX);
      Serial.print("]");
    }
  }
}

/**
  @brief   Print signature, if the character is invisible, 
           print hexible value instead.
  @param   buf  -->  VR module return value when voice is recognized.
             buf[0]  -->  Group mode(FF: None Group, 0x8n: User, 0x0n:System
             buf[1]  -->  number of record which is recognized. 
             buf[2]  -->  Recognizer index(position) value of the recognized record.
             buf[3]  -->  Signature length
             buf[4]~buf[n] --> Signature
*/

void startMovingUp(){
  if(up_down_angle < 74){
  up_down_angle ++;
  Up_Down_Servo.write(up_down_angle);
  delay(up_down_Speed);
  }else{
    level = 0;
    moveUp = false;
    myVR.clear();
    myVR.load(uint8_t (0));  // load the Alice
  }
}
void startMovingDown(){
  if(up_down_angle > 4){
  up_down_angle --;
  Up_Down_Servo.write(up_down_angle);
  delay(up_down_Speed);
  }else{
    level = 0;
    moveDown = false;
    myVR.clear();
    myVR.load(uint8_t (0));  // load the Alice
  }
}

void startMovingLeft(){
  if(left_right_angle < 125){
  left_right_angle ++;
  Left_Right_Servo.write(left_right_angle);
  delay(left_right_Speed);
  }else{
    level = 0;
    moveLeft = false;
    myVR.clear();
    myVR.load(uint8_t (0));  // load the Alice
  }
}

void startMovingRight(){
  if(left_right_angle > 0){
  left_right_angle --;
  Left_Right_Servo.write(left_right_angle);
  delay(left_right_Speed);
  }else{
    level = 0;
    moveRight = false;
    myVR.clear();
    myVR.load(uint8_t (0));  // load the Alice
  }
}

void continueMovingUp(){
  if(up_down_angle < 80){
  up_down_angle ++;
  Up_Down_Servo.write(up_down_angle);
  //Serial.println(up_down_angle);
  delay(up_down_Speed);
  }else{
    level = 0;
    moveUp = false;
    myVR.clear();
    myVR.load(uint8_t (0));  // load the Alice
  }
  
}
void continueMovingDown(){
  if(up_down_angle > 4){
  up_down_angle --;
  Up_Down_Servo.write(up_down_angle);
  //Serial.println(up_down_angle);
  delay(up_down_Speed);
  }else{
    level = 0;
    moveDown = false;
    myVR.clear();
    myVR.load(uint8_t (0));  // load the Alice
  }
}

void continueMovingLeft(){
  if(left_right_angle < 125){
  left_right_angle ++;
  Left_Right_Servo.write(left_right_angle);
  //Serial.println(left_right_angle);
  delay(left_right_Speed);
  }else{
    level = 0;
    moveLeft = false;
    myVR.clear();
    myVR.load(uint8_t (0));  // load the Alice
  }
}

void continueMovingRight(){
  if(left_right_angle > 0){
  left_right_angle --;
  Left_Right_Servo.write(left_right_angle);
  //Serial.println(left_right_angle);
  delay(left_right_Speed);
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
          Rotate_Servo.write(rotate_angle);
          rotate_angle--;
          delay(rotate_Speed);
        }
        horizental = true;
        }else{
          rotate_angle = 0;
          while(rotate_angle < 90){
          Rotate_Servo.write(rotate_angle);
          rotate_angle++;
          delay(rotate_Speed);
        }
        horizental = false;
        }
}

void ReceiveBluetoothMSG(){ // Function for monitoring the bluetooth inputstream from the phone
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
      Serial.println("X coordinate: " + coord.value);
    } else if (coord.type == "Y") {
      Serial.println("Y coordinate: " + coord.value);
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
