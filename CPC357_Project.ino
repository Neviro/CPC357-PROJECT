#include <lcd_i2c.h>
#include <ESP32Servo.h>

//LCD address
lcd_i2c lcd(0x3E, 16, 1); 

//control barrier
Servo barrierServo;
const int servoPin = 4; 
const int servoOpenAngle = 90; 
const int servoCloseAngle = 180;  

//to open barrier (ir sensor 1)
const int irBarrier1 = 5; 

//to close barrier (ir sensor 2)
const int irBarrier2 = 6; 

//to detect available parking slot on floor 1 (ir sensor 3)
const int irParking1 = 7; 

//to detect available parking slot on floor 2 (ir sensor 4)
const int irParking2 = 8; 

//to detect air quality of vehicle 
const int airQualityPin = 9; 

void scrollText(String message, int scrollDelay, int repeatCount = 2);

void setup() {
  lcd.begin();
  lcd.clear();
  Serial.begin(9600);

  pinMode(irBarrier1, INPUT);
  pinMode(irBarrier2, INPUT);
  pinMode(irParking1, INPUT);
  pinMode(irParking2, INPUT);
  pinMode(airQualityPin, INPUT);

  barrierServo.attach(servoPin);
  barrierServo.write(servoCloseAngle); 

  lcd.print("System Ready");
  Serial.println("System Ready");
  delay(2000);
  lcd.clear();
}

void loop() {

  //read sensor values
  int barrier1State = digitalRead(irBarrier1); 
  int barrier2State = digitalRead(irBarrier2); 
  int airQualityValue = analogRead(airQualityPin);
  bool parking1Full = digitalRead(irParking1); 
  bool parking2Full = digitalRead(irParking2); 

  //serial monitor
  Serial.print("Barrier1: ");
  Serial.print(barrier1State);
  Serial.print(" Barrier2: ");
  Serial.print(barrier2State);
  Serial.print(" AirQuality: ");
  Serial.print(airQualityValue);
  Serial.print(" P1 Full: ");
  Serial.print(parking1Full);
  Serial.print(" P2 Full: ");
  Serial.println(parking2Full);

  //detect car at entry(ir 1 = 0 when car is present)
  if (barrier1State == 0) { 
    lcd.clear();
    lcd.print("Car Detected!");
    Serial.println("Car Detected at Entry");
    barrierServo.write(servoOpenAngle); 
    Serial.println("Barrier Opened");
    delay(2000);

    //good air quality (EV)
    if (airQualityValue < 1000) { 
      //floor 1 available
      if (parking1Full == 1) { 
        lcd.clear();
        scrollText("EV->1st Floor", 300, 2);
        Serial.println("EV (Good Air Quality) -> Floor 1");
      } 

      //floor 1 full
      else { 
        lcd.clear();
        scrollText("1st Full->Redirect to Floor 2", 300, 2);
        Serial.println("Floor 1 Full -> Redirect to Floor 2");
      }
    } 

    //poor air quality (non EV)
    else { 
      //floor 2 available
      if (parking2Full == 1) { 
        lcd.clear();
        scrollText("Non-EV->2nd Floor", 300, 2);
        Serial.println("Non-EV (Poor Air Quality) -> Floor 2");
      } 

      //floor 2 full
      else { 
        lcd.clear();
        scrollText("2nd Full->Redirect to Floor 1", 300, 2);
        Serial.println("Floor 2 Full -> Redirect to Floor 1");
      }
    }

    //wait for the car to pass ir2
    while (digitalRead(irBarrier2) == 1) { 
      scrollText("Waiting car to exit", 300, 2);
      Serial.println("Waiting for car to exit...");
    }

    //add a delay after the car passes to avoid hitting
    delay(3000);

    barrierServo.write(servoCloseAngle); 
    lcd.clear();
    scrollText("Barrier Closed", 300, 2);
    Serial.println("Barrier Closed");
    delay(2000); 
  }

  delay(500); 
}

//long text on lcd
void scrollText(String message, int scrollDelay, int repeatCount) {
  int messageLength = message.length();
  if (messageLength <= 16) { 
    for (int j = 0; j < repeatCount; j++) { 
      lcd.clear();
      lcd.print(message.c_str());
      delay(scrollDelay * 5); 
    }
    return;
  }
  for (int r = 0; r < repeatCount; r++) { 
    for (int i = 0; i <= messageLength - 16; i++) { 
      lcd.clear();
      lcd.print(message.substring(i, i + 16).c_str()); 
      delay(scrollDelay);
    }
  }
}
