#include <SoftwareSerial.h>
#include "VoiceRecognitionV3.h"
#include <math.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

Adafruit_PWMServoDriver Servos = Adafruit_PWMServoDriver(0x40);

#define servoMIN 102
#define servoMAX 512
//Pins definations
//Servo Pins (Pins of the PWM chip)
#define Left_Right_Pin  (12) 
#define Up_Down_Pin     (13) 
#define Rotate_Pin      (14) 
// Check the bluetooth connection status
#define Bluetooth_statePin  (9) 
#define LED_Pin         (10) 
#define RST_Pin         (11) 
void(* resetFunc) (void) = 0; // For reset functions. Just call resetFunc();


// object defination
VR myVR(2,3);    // TX to pin 2; RX to pin 3; Bluetooth use the hardware TX and RX pins


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
int up_down_angle = 105;
int left_right_angle = 140;
int rotate_angle = 173;
int left_most_Angle = 180;
int right_most_Angle = 100;
int up_most_Angle = 135;
int down_most_Angle = 65;


// speed control: the larger the slower
int up_down_Speed = 30;
int left_right_Speed = 30;
int rotate_Speed = 20;
int search_Speed = 100;
int flash_Speed = 150;

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
#define level2CMD6       (12) // Pause



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
  pinMode(LED_Pin, OUTPUT);
  pinMode(RST_Pin, INPUT_PULLUP);
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
    //while(1);
  }
  myVR.load(uint8_t (0));  // load the Alice
  level = 0;
  LED_Flash(flash_Speed); 
  LED_Flash(flash_Speed); 
  LED_Flash(flash_Speed); 
}

void loop()
{ 
  checkRST();
  int ret;
  ret = myVR.recognize(buf, 50);
  if(ret>0){
    switch(buf[1]){

      case level0Alice: // hello Alice
        LED_Flash(flash_Speed);
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
            //Serial.println("Alice");
          }
        }
        break;

      case level1CMD1: // Take photo
        LED_Flash(flash_Speed); 
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
        LED_Flash(flash_Speed);
        //Serial.print("Move: ");
        if (level == 1) {
          level = 2;
          myVR.clear();
          record[0] = level2CMD1;  // Stop
          record[1] = level2CMD2;  // Up
          record[2] = level2CMD3;  // Down
          record[3] = level2CMD4;  // Left
          record[4] = level2CMD5;  // Right
          record[5] = level2CMD6;  // Pause
          myVR.load(record, 6);
        }
        break;
      
      case level1CMD3: // Find
        LED_Flash(flash_Speed);
        if(digitalRead(Bluetooth_statePin) == HIGH){ // Check the bluetooth connect
          Serial.println("Find!"); // Cannot be deleted!
          searching = true;
        }else{
          Serial.println("Search Function unavaiable. Please connect to the phone.");
        }
        while (searching) {
          checkRST();
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
        LED_Flash(flash_Speed);
        //Serial.println("Rotate!");
        level = 0;
        myVR.clear();
        rotate();
        myVR.load(uint8_t (0));  // load the Alice
        break;

      case level1CMD5: // Alblum
        LED_Flash(flash_Speed);
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
        LED_Flash(flash_Speed);
        //Serial.println("Tap center!");
        level = 0;
        up_down_angle = 105;
        left_right_angle = 140;
        rotate_angle = 173;
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
        LED_Flash(flash_Speed);
        //Serial.println("Stop!");
        moveDown = false;
        moveUp = false;
        moveLeft = false;
        moveRight = false;
        level = 0;
        myVR.clear();
        myVR.load(uint8_t (0));  // load the Alice
        break;

      case level2CMD6: // Pause
        LED_Flash(flash_Speed);
        //Serial.println("Pause!");
        moveDown = false;
        moveUp = false;
        moveLeft = false;
        moveRight = false;
        break;

      case level2CMD2: // Move up
        LED_Flash(flash_Speed);
        //Serial.println("up!");
        moveUp = true;
        moveDown = false;
        break;

      case level2CMD3: // Move Down
        LED_Flash(flash_Speed);
        //Serial.println("down!");
        moveDown = true;
        moveUp = false;
        break;

      case level2CMD4: // Move left
        LED_Flash(flash_Speed);
        //Serial.println("left!");
        moveLeft = true;
        moveRight = false;
        break;
      

      case level2CMD5: // Move right
        LED_Flash(flash_Speed);
        // Serial.println("right!");
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
  if(up_down_angle < up_most_Angle){
  up_down_angle ++;
  pulseLength_Updown = map(up_down_angle, 0, 180, servoMIN, servoMAX); // Map the angle to the according pulse length
  Servos.setPWM(Up_Down_Pin, 0, pulseLength_Updown);
  delay(Speed);
  }else{
    moveUp = false;
  }
  
}
void continueMovingDown(int Speed){
  if(up_down_angle > down_most_Angle){
  up_down_angle --;
  pulseLength_Updown = map(up_down_angle, 0, 180, servoMIN, servoMAX); // Map the angle to the according pulse length
  Servos.setPWM(Up_Down_Pin, 0, pulseLength_Updown);
  delay(Speed);
  }else{
    moveDown = false;
  }
}

void continueMovingLeft(int Speed){
  if(left_right_angle < left_most_Angle){
  left_right_angle ++;
  pulseLength_Leftright = map(left_right_angle, 0, 180, servoMIN, servoMAX); // Map the angle to the according pulse length
    Servos.setPWM(Left_Right_Pin, 0, pulseLength_Leftright);
  delay(Speed);
  }else{
    moveLeft = false;
  }
}

void continueMovingRight(int Speed){
  if(left_right_angle > right_most_Angle){
  left_right_angle --;
  pulseLength_Leftright = map(left_right_angle, 0, 180, servoMIN, servoMAX); // Map the angle to the according pulse length
  Servos.setPWM(Left_Right_Pin, 0, pulseLength_Leftright);
  delay(Speed);
  }else{
    moveRight = false;
  }
}

void rotate(){
  if(!horizental){
          rotate_angle = 173;
          while(rotate_angle > 88){
          pulseLength_Rotate = map(rotate_angle, 0, 180, servoMIN, servoMAX); // Map the angle to the according pulse length
          Servos.setPWM(Rotate_Pin, 0, pulseLength_Rotate);
          rotate_angle--;
          delay(rotate_Speed);
        }
        horizental = true;
        }else{
          rotate_angle = 88;
          while(rotate_angle < 173){
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
/* This function is used to process a string containing multiple coordinate packets.
   It splits the string into separate packets and processes each packet. 
   The loop exits when no more newline characters are found,
   indicating that all packets in the string have been processed.
   Also, the tracking function is also in this block.
 */
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
      difference_X = x - centerX; // Calclate the difference of X
      if(abs(difference_X) < tracking_Sensitivity){ // Stop moving when smaller than a threshold
        moveLeft = false;
        moveRight = false;
        X_Central = true;
      }

      if (difference_X < -tracking_Sensitivity) { // Move left when whenobject is on the left
        moveRight = false;
        moveLeft = true;
      }

      if (difference_X > tracking_Sensitivity) { // Move right when whenobject is on the right
        moveLeft = false;
        moveRight = true;
      }
      
    } else if (coord.type == "Y") {
      int y = coord.value.toInt();
      difference_Y = y - centerY; // Calclate the difference of Y
      if(abs(difference_Y) < tracking_Sensitivity){ // Stop moving when smaller than a threshold
        moveUp = false;
        moveDown = false;
        Y_Central = true;
      }
      if(difference_Y > tracking_Sensitivity){ // Move down when whenobject is below
        moveDown = true;
        moveUp = false;
      }
      if(difference_Y < -tracking_Sensitivity){ // Move up when whenobject is above
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

void LED_Flash(int flashSpeed){
  digitalWrite(LED_Pin, HIGH);
  delay(flashSpeed);
  digitalWrite(LED_Pin, LOW);
  delay(flashSpeed);
}

void checkRST(){
  if(digitalRead(RST_Pin) == LOW){
    delay(1100);
    resetFunc();
  }
}



