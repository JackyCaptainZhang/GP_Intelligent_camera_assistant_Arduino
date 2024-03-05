## Project Introduction

The project invovles in **Arduino-based gimbal develop** and **Android App develop**. This Project is an accessitive technology for those who has disabilities that can not hold the phone to take the photo. So the final aim is to enable them to just use their voice to finish photography composition and photo taking. In the final product, user will only need to say command like "Find birds". And the gimble will automatically move to search for the birds in the view. The gimbal will keep the bird in the center of the screen if the bird is detected. And user can say "take photo" to caputer the photo.

Also, we have the basic precise movement control. User can say like "Move Left" and the gimbal will rotate to the left until user say "Stop". User can adjust the gimbal to any direction very easily and smoothly. Unlike the advanced search function mentioned above , these basic movement control can work without the phone, which means any other devices (like USB cameras) who does not have bluetooth function can also be attached to this device and user can control its movement very easily.

# Software
See https://github.com/JackyCaptainZhang/GP_Intelligent_camera_assistant_Android

# Hardware

Hardware part mainly focus on the system logic speech control and the tracking function.

## 1. The logic and commands

2024-01-15

To use the voice module, we need to train the different commands to the modules (Max 256 commands and each command must be shorter than 1.5s). But if we want to use the command, we must load the index of the trained command to the registor. Because the registor can only load maximum 7 CMD at the same time, CMD need to be catgorized into some groups. Level 0 is the highest priority and is ready for all time. Level 1 is some basic function CMD, and is only available when "Alice" is called. Level 2 gives details to the Level 1 CMD, and is only available when Level 1 CMD is called. In brief, Level 1 gives the actions (move), Level 2 gives the action details (How to move/ When to stop).

PS: [] denotes the trained index for each command. Can be changed to personalised commands in training.

### Level 0 CMD (1)

* Alice [0]

### Level 1 CMD (6)

* Take photo [2]: call the phone function
* Move [3]
* Find [4]: call the phone function
* Rotate [5]
* Alblum [10]: call the phone function
* Tap center [11]
* () 

### Level 2 CMD

* When Move (5)
  * Up [6]
  * Down [7]
  * Left [8]
  * Right [9]
  * Stop [1]



Now we have set up the complete logic structure for our whole system, next go to the servo motor control part.



## 2. Smoother motor controls

2024-01-20

We want to make the motor runs more smoothly. (Ex: When you say "Move left", it will turn left slowly and you can say "Stop" to make it stop at any point you want). To do this, we can use the while loop with break statement.

```c++
bool stop = false;
int angle;

while (angle < 180) {
        MonitorStop();
    if (stop == true) {
      stop = false;
    break;
    }
    myServo.write(angle);
    angle++;
    Serial.println(angle);
    delay(20);  // control the speed
  }

void MonitorStop(){ // Function that use the speech moudule to monitor users' "Stop" command
int ret;
    ret = myVR.recognize(buf, 50);
        if(ret > 0){
          if(buf[1] == 1){
      stop = true;
      Serial.println("Stop!");         
    }
        } 
}
```



## 3. Extract the needed coordinate

What we received from the Android is in the format of :

```kotlin
BluetoothHelper.sendBluetoothCommand("X $$centerX !")
BluetoothHelper.sendBluetoothCommand("Y $$centerY !")
```

So we need to do two things:

1. Only extract the value `centerX` and `centerY` from the data packages
2. Label it is X or Y coordinate

So we use the customised data structor `Coordinate` which contains both type and value.

```c++
struct Coordinate {
  String type;
  String value;
};
```

And based on this data structor, we can have two methods to write data into it.

```c++
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


Coordinate extractCoordinate(String data) { // Function for extract the right X/Y coordinate value from the data package received from Bluetooth
  Coordinate coord;
  int dollarIndex = data.indexOf('$') + 1;
  int exclamationIndex = data.indexOf('!', dollarIndex);

  if (dollarIndex > 1 && exclamationIndex > dollarIndex) {
    coord.type = data.substring(0, 1); // Extract the type: X or Y
    coord.value = data.substring(dollarIndex, exclamationIndex); // Extract the coordinate value
  } else {
    coord.type = "Error";
    coord.value = "Invalid format";
  }
  return coord;
}

```



Next to the tracking function.

## 4. Closed-loop controlled Tacking

We now have the continuous coordinate from Android, and we can use Closed-loop to make motors move the item to the centre of the screen.

The basic idea is that the algorithm will calculate the differences between X and Y to the centre of the screen. When the difference is smaller than a threshold, the motor will stop moving.

```c++
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
```



## 5. Improvement in motor control

We have already had the control method for motor movements. However, due to the interferences of the serial communication (Bluetooth module, speech recognition module), motors will jerk and the control is not that reliable. To improve it, we can use the external PWM motor control chip, which is PCA9685. So we need to change the control methods to this chip. What is more, this chip can also resolve the power supply for each motor so that the whole system will be more compact.

It is not difficult to modify the code. Take rotate motor as example. Just change from the older version:

```c++
Servo Rotate_Servo;
Rotate_Servo.attach(6);
Rotate_Servo.write(rotate_angle);
```

to:

```c++
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

Adafruit_PWMServoDriver Servos = Adafruit_PWMServoDriver(0x40);

Servos.begin();
Servos.setPWMFreq(50);  // This is the maximum PWM frequency

pulseLength_Rotate = map(rotate_angle, 0, 180, servoMIN, servoMAX); // Map the angle to the according pulse length
Servos.setPWM(Rotate_Pin, 0, pulseLength_Rotate);
```



So now the complete system has finished. The final circuit is:



And the flow chart is:

![Flow Chart](/Users/jackyzhang/Desktop/Private/Schools/UCL/S1/SURG094 Group Project/Project Material/Flow Chart.jpg)

# Development Notes

1. Important Flags in Arduino:

* Moving

* Searching
* horizontal

2. Up and down servo:

* Initial angle: 120
* Up: 120++ to 150
* Down: 120-- to 80

3. Left and right servo:

* Initial angle: 140
* Left: 140++ to 180
* Right: 140-- to 100

4. Rotate: 90 (vertical) to 0 (horizontal)

5. Centre of Screen (360,753)

* move down if > 753
* move up if < 753
* move left if < 360
* move right if > 360

6. LED: 

```c++
pinMode(ledPin, OUTPUT);
ledState = LOW/HIGH;
digitalWrite(ledPin, ledState);
```

7. Rotate motor: Yellow (Signal), Green (Power), Grey (GND)

