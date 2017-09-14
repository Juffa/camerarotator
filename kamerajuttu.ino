/*
 * Camera rotator by Juha kaikko.
 * I guess it'll be GPL V2 then.
 * 
 * This project uses AccelStepper.h library which can be downloaded from http://www.airspayce.com/mikem/arduino/AccelStepper/index.html.
 *  
 */
 
 //testi

#include "Arduino.h"
#include "AccelStepper.h"
#include "LiquidCrystal.h"

//Setting up pins
const byte upButtonPin=10;
const byte downButtonPin=8;
const byte selectButtonPin=9;
const byte cameraPin=3;
const byte turnMotorPin=4;
const byte turnDirPin=5;
const byte tiltMotorPin=6;
const byte tiltDirPin=7;

//Setting up stepper pins
AccelStepper Xaxis(1,turnMotorPin,turnDirPin);
AccelStepper Yaxis(1,tiltMotorPin,tiltDirPin);

//Setting up LCD pins
LiquidCrystal lcd(19, 18, 17, 16, 15, 14);


const byte baseGearRatio=8;
const byte tiltGearRatio=8;
const int turnSteps=200*16*baseGearRatio;//using 16th step =25600 steps/360 degrees
const int tiltSteps=(200*16*tiltGearRatio); //=25600
unsigned int xSpeed=2000;
unsigned int ySpeed=2000;
byte menuScreen=0; //Current selected menu item
const byte menuSize=5; //(0-5), Easier to give the size of an array manually than trying to read it
const String menu[6]={"Settings","360x180","360","180","Time-lapse","testi"};

byte settingsMenuScreen=0;
const byte settingsMenuSize=5;
const String settingsMenu[6]={"Back","Sensor","Lens mm","ExpoTime","X-speed","Y-speed"};

byte sensor=0; //0=36x24mm, 1=23.6x15.7mm
const byte sensorMenuSize=1;
const float sensors[2][2]={{36,24},{23.6,15.7}}; //Sets up 2x2 array with sensor size information

byte lens=2;
const byte lensMenuSize=6;
const int lenses[7]={8,12,14,28,50,70,105}; //Lens millimeters for panorama calculation

byte timeLapseMenuScreen=0;
const byte timeLapseMenuSize=6;
const String timeLapseMenu[7]={"Back","exposure time","shots","x-shift","y-start angle","y-shift","Start"};

byte overlap=20; // Percentage of image overlap in panorama capture. More is better for stitching, but produces more images.

int xShift=16; //Amount of steps between shots in time-lapse. With microstepping 16 steps makes one full step. Positive = clockwise.
int yStartAngle=6400; // Measured in steps. 6400 = 90 degrees = horizontal . 0 is up, which is the starting position.
int yShift=0; //Every time x axis moves, this is checked if we want to move y axis also. Positive = downwards.
unsigned int shots=100; //Default number of shots for time-lapse. Tuneable in time-lapse setup menu
unsigned long expoTime=100; //In Milliseconds, so 1000=1sec. Can be a lot shorter when shooting in daylight. Camera needs to be in single shot mode or it can take multiple shots.
unsigned int intervalTime=0; //Extra added time between shots
unsigned int stabilizingTime=300; //Time between movement and shooting, to stabilize shakes. Matters more in longer exposures.
unsigned int expoAdjustmentDefault=10;
unsigned int expoAdjustment=expoAdjustmentDefault;

unsigned long buttonStartTime=0;
unsigned long buttonEndTime=0;
const int debounceDefault=200; 
int debounceTime=debounceDefault; //Button presses that occur within this amount of time from previous button press are ignored
unsigned long lastButtonTime=0;
int buttonPressCounter=0;
int repeatTime=50;
int repeatDelay=500;

byte upArrow[8]={
    B00100,
    B01110,
    B10101,
    B00100,
    B00100,
    B00100,
};
byte downArrow[8]={
    B00100,
    B00100,
    B00100,
    B10101,
    B01110,
    B00100,
};

void setup()
{
  Serial.begin(9600);
  Xaxis.setMaxSpeed(xSpeed);
  Yaxis.setMaxSpeed(ySpeed);
  Xaxis.setAcceleration(2000.0);
  Yaxis.setAcceleration(2000.0);

  pinMode(cameraPin, OUTPUT);
  pinMode(upButtonPin, INPUT_PULLUP);
  pinMode(downButtonPin, INPUT_PULLUP);
  pinMode(selectButtonPin, INPUT_PULLUP);

  lcd.createChar(0,upArrow);
  lcd.createChar(1,downArrow);
  lcd.begin(16,2);
  lcd.clear();
}

void loop()
{
   //Main menu
  lcd.clear();

    if(menuScreen!=0){ //prints the up arrow if not at limit
      lcd.setCursor(0,0);
      lcd.write(byte(0));
    }

    lcd.setCursor(2,0);
    lcd.print("Menu");

    if(menuScreen!=menuSize){ //prints the down arrow if not at limit
      lcd.setCursor(0,1);
      lcd.write(byte(1));
    }

    lcd.setCursor(2,1);
    lcd.print(menu[menuScreen]);//tulostaa näytölle missä menussa ollaan


  switch(buttonPress()){
    case 2:
      if(menuScreen>0){
        menuScreen-=1;
      }
      lcd.clear();
      break;

    case 3:

      if(menuScreen<menuSize){
        menuScreen+=1;
      }
      lcd.clear();
      break;

    case 1:
      switch(menuScreen){
        //after pressing select the loop breaks and this kicks in, and starts whatever was selected.
        //Variables menu and menuSize should match these items.
        case 0:
            settings();
            break;
        case 1:
            scan360x180();
            break;
        case 2:
            scan360();
            break;
        case 3:
            scan180();
            break;
        case 4:
            timelapseMenu();
            break;
        case 5:
            testi();
            break;
      }
      break;
  }
}



void settings(){
  lcd.clear();
  bool exit=false;
  
  while(exit==false){
    if(settingsMenuScreen!=0){ //prints the up arrow if not at limit
      lcd.setCursor(0,0);
      lcd.write(byte(0));
    }
    lcd.setCursor(2,0);
    lcd.print("Settings");

    if(settingsMenuScreen!=settingsMenuSize){//prints the down arrow if not at limit
      lcd.setCursor(0,1);
      lcd.write(byte(1));
    }
    lcd.setCursor(2,1);
    lcd.print(settingsMenu[settingsMenuScreen]); //tulostaa näytölle missä menussa ollaan

    switch(buttonPress()){
    case 2:
        if(settingsMenuScreen>0){
          settingsMenuScreen-=1;
        }
        lcd.clear();
        break;

    case 3:
        if(settingsMenuScreen<settingsMenuSize){
          settingsMenuScreen+=1;
        }
        lcd.clear();
        break;

    case 1:
      switch(settingsMenuScreen){
        //after pressing select the loop breaks and this kicks in and starts whatever was selected.
        //Variables SettingsMenuScreen and settingsMenuSize should match these items.
        case 0:
            //Back to previous menu
            exit=true;
            break;
        case 1:
            sensorSettings();
            lcd.clear();
            break;
        case 2:
            lensSettings();
            lcd.clear();
            break;
        case 3:
            shutterSettings();
            lcd.clear();
            break;
        case 4:
            xSpeedSettings();
            lcd.clear();
            break;
        case 5:
            ySpeedSettings();
            lcd.clear();
            break;
      }
      break;
    }
  }
}

void sensorSettings(){
  lcd.clear();
  bool exit=false;
  
  while(exit==false){
    if(sensor!=0){ //prints the up arrow if not at limit
      lcd.setCursor(0,0);
      lcd.write(byte(0));
    }
    lcd.setCursor(2,0);
    lcd.print("Sensor");

    if(sensor!=sensorMenuSize){//prints the down arrow if not at limit
      lcd.setCursor(0,1);
      lcd.write(byte(1));
    }
    lcd.setCursor(2,1);
    lcd.print((String)""+sensors[sensor][0]+" x "+sensors[sensor][1]+"");

  switch(buttonPress()){
    case 2:
      if(sensor>0){
        sensor-=1;
      }
      lcd.clear();
      break;

    case 3:
      if(sensor<sensorMenuSize){
        sensor+=1;
      }
      lcd.clear();
      break;

    case 1:
      exit=true;
      break;
    }
  }
}

void lensSettings(){
  lcd.clear();
  bool exit=false;
  while(exit==false){

    if(lens!=0){ //prints the up arrow if not at limit
      lcd.setCursor(0,0);
      lcd.write(byte(0));
    }

    lcd.setCursor(2,0);
    lcd.print("Lens");

    if(lens!=lensMenuSize){//prints the down arrow if not at limit
      lcd.setCursor(0,1);
      lcd.write(byte(1));
    }

    lcd.setCursor(2,1);
    lcd.print(lenses[lens],DEC);



    switch(buttonPress()){
    case 2:
      if(lens>0){
        lens-=1;
      }
      lcd.clear();
      break;

    case 3:
      if(lens<lensMenuSize){
        lens+=1;
      }
      lcd.clear();
      break;

    case 1:
      exit=true;
      break;
    }
  }
}


void shutterSettings(){
  lcd.clear();
  bool exit=false;
  while(exit==false){
    if(expoTime!=0){ //prints the up arrow if not at limit
      lcd.setCursor(0,0);
      lcd.write(byte(0));
    }
    lcd.setCursor(2,0);
    lcd.print("Shutter (ms)");
    lcd.setCursor(0,1);
    lcd.write(byte(1));
    lcd.setCursor(2,1);
    lcd.print(expoTime,DEC);



  if(millis()-buttonStartTime>=1000 && millis()-buttonStartTime<=2999){
    expoAdjustment=10;
  }else if(millis()-buttonStartTime>=3000 && millis()-buttonStartTime<=4999){
    expoAdjustment=50;
  }else if(millis()-buttonStartTime>=5000){
    expoAdjustment=200;
  }
    
    switch(buttonPress()){
    case 2:
      expoTime+=expoAdjustment;
      lcd.clear();
      break;

    case 3:
      if(expoAdjustment==10 && expoTime>=20){
        expoTime-=expoAdjustment;
      }else if(expoAdjustment==50 && expoTime>=60){
        expoTime-=expoAdjustment;
      }else if(expoAdjustment==200 && expoTime>=210){
        expoTime-=expoAdjustment;  
      }
      lcd.clear();
      break;

    case 1:
      exit=true;
      break;
    }
  }

}

void xSpeedSettings(){
  lcd.clear();
  bool exit=false;
  while(exit==false){
    if(xSpeed!=0){
    lcd.setCursor(0,0);
    lcd.write(byte(0));
    }
    lcd.setCursor(2,0);
    lcd.print("X speed");
    lcd.setCursor(0,1);
    lcd.write(byte(1));
    lcd.setCursor(2,1);
    lcd.print(xSpeed,DEC);
    
    switch(buttonPress()){
      case 2:
        xSpeed+=100;
        break;

      case 3:
        if(xSpeed>=100){
          xSpeed-=100;
        }
        break;

      case 1:
        Xaxis.setMaxSpeed(xSpeed);
        exit=true;
        break;
    }
  }
}

void ySpeedSettings(){
  lcd.clear();
  bool exit=false;
  while(exit==false){
    if(ySpeed!=0){
    lcd.setCursor(0,0);
    lcd.write(byte(0));
    }
    lcd.setCursor(2,0);
    lcd.print("Y speed");
    lcd.setCursor(0,1);
    lcd.write(byte(1));
    lcd.setCursor(2,1);
    lcd.print(ySpeed,DEC);
    
    switch(buttonPress()){
      case 2:
        ySpeed+=100;
        break;

      case 3:
        if(ySpeed>=100){
          ySpeed-=100;
        }
        break;

      case 1:
        Yaxis.setMaxSpeed(ySpeed);
        exit=true;
        break;
    }
  }
}


void timelapseMenu(){
  lcd.clear();
  bool exit=false;
  while(exit==false){

    if(timeLapseMenuScreen!=0){ //prints the up arrow if not at limit
          lcd.setCursor(0,0);
          lcd.write(byte(0));
    }

    lcd.setCursor(2,0);
    lcd.print("Time-lapse");


    if(timeLapseMenuScreen!=timeLapseMenuSize){//prints the down arrow if not at limit
          lcd.setCursor(0,1);
          lcd.write(byte(1));
        }

    lcd.setCursor(2,1);
    lcd.print(timeLapseMenu[timeLapseMenuScreen]);



    switch(buttonPress()){
    case 2:

        if(timeLapseMenuScreen>0){
          timeLapseMenuScreen-=1;
        }
        lcd.clear();
        break;

    case 3:

        if(timeLapseMenuScreen<timeLapseMenuSize){
          timeLapseMenuScreen+=1;
        }
        lcd.clear();
        break;

    case 1:
      switch(timeLapseMenuScreen){//after pressing select the loop breaks and this kicks in and starts whatever was selected
      //"exposure time","shots","x-shift","y-start angle","y-shift","Start"
        case 0:
            lcd.clear();
            exit=true;
            break;

        case 1:
            //expotime
            shutterSettings();
            lcd.clear();
            break;
        case 2:
            //shots
            timelapseShots();
            lcd.clear();
            break;

        case 3:
            //x-shift between shots
            timelapseXshift();
            lcd.clear();
            break;
        case 4:
            //y-start-angle, 90 vaakataso
            timelapseYStart();
            lcd.clear();
            break;

        case 5:
            //y-shift between shots
            timelapseYshift();
            lcd.clear();
            break;

        case 6:
            //start
            lcd.clear();
            timelapse();
            exit=true;
            break;

      }
      break;
    }
  }
}

void timelapseShots(){
  lcd.clear();
  bool exit=false;
  while(exit==false){

    if(shots>=10){ //prints the down arrow if not at limit
          lcd.setCursor(0,1);
          lcd.write(byte(1));
    }
    lcd.setCursor(2,0);
    lcd.print("nr of shots");
    lcd.setCursor(0,0);
    lcd.write(byte(0));
    lcd.setCursor(2,1);
    lcd.print(shots,DEC);
    lcd.print(" (");
    lcd.print((360.0/turnSteps)*xShift*shots,1);
    lcd.print(" deg)");


    switch(buttonPress()){
    case 2:
      shots+=10;
      lcd.clear();
      break;

    case 3:
      if(shots>10){
      shots-=10;
      }
      lcd.clear();
      break;

    case 1:
      exit=true;
      break;
    }
  }

}

void timelapseXshift(){
  lcd.clear();
  bool exit=false;
  while(exit==false){
  lcd.setCursor(0,0);
    lcd.write(byte(0));
    lcd.setCursor(2,0);
    lcd.print("Xsft ");
    lcd.print(xShift,DEC);
    lcd.print(" step");
    lcd.setCursor(0,1);
    lcd.write(byte(1));
    lcd.setCursor(2,1);
    lcd.print(360.0/turnSteps*xShift);
    lcd.print("/");
    lcd.print((360.0/turnSteps)*xShift*shots,1);
    lcd.print(" deg");



    switch(buttonPress()){
    case 2:
      xShift+=1;
      lcd.clear();
      break;

    case 3:
      xShift-=1;
      lcd.clear();
      break;

    case 1:
      exit=true;
      break;
    }
  }

}

void timelapseYshift(){
  lcd.clear();
  bool exit=false;
  while(exit==false){

    lcd.setCursor(0,0);
    lcd.write(byte(0));
    lcd.setCursor(2,0);
    lcd.print("Y shift");
    lcd.setCursor(0,1);
    lcd.write(byte(1));
    lcd.setCursor(2,1);
    lcd.print(yShift,DEC);


    switch(buttonPress()){
    case 2:
      yShift+=1;
      lcd.clear();
      break;

    case 3:
      yShift-=1;
      lcd.clear();
      break;

    case 1:
      exit=true;
      break;
    }
  }

}

void timelapseYStart(){
  lcd.clear();
  bool exit=false;
  while(exit==false){

  if(yStartAngle>=(-tiltSteps/2)){ //prints the up arrow if not at limit
      lcd.setCursor(0,0);
      lcd.write(byte(0));
    }
    lcd.setCursor(2,0);
    lcd.print("Y start pos");
    if(yStartAngle<=tiltSteps/2){ //prints the up arrow if not at limit
      lcd.setCursor(0,0);
      lcd.write(byte(0));
    }
    lcd.setCursor(0,1);
    lcd.write(byte(1));
    lcd.setCursor(2,1);
    lcd.print(yStartAngle);
    lcd.print(", ");
    lcd.print(yStartAngle/(tiltSteps/360),DEC);
    lcd.print("deg");



    switch(buttonPress()){
    case 2:
      if(yStartAngle<=tiltSteps/2){
      yStartAngle+=tiltSteps/360*5;
      }
      lcd.clear();
      break;

    case 3:
      if(yStartAngle>=(-tiltSteps/2)){
      yStartAngle-=tiltSteps/360*5;
      }
      lcd.clear();
      break;

    case 1:
      Serial.println(yStartAngle,DEC);
      exit=true;
      break;
    }
  }

}


//-------------------------------------------//
//-- Helper functions --//

int buttonPress(){
  //joku systeemi mikä tunnistaa pitkän painalluksen ja vähentää debouncetimeä
  //already pressing?


  if((digitalRead(selectButtonPin)==LOW || digitalRead(upButtonPin)==LOW || digitalRead(downButtonPin)==LOW) && millis()>buttonStartTime+repeatDelay){
    if (debounceTime>=10){
      debounceTime-=10;
      Serial.println("vähennetään debouncetimee");
    }//else if(debounceTime<10 && buttonStartTime>1000 && expoAdjustment<100 && millis()+20>lastButtonTime){

    //  expoAdjustment+=expoAdjustment;

    //}
  }



  while(millis()<lastButtonTime+debounceTime){
    //wait
  }

  if(digitalRead(selectButtonPin)==HIGH && digitalRead(upButtonPin)==HIGH && digitalRead(downButtonPin)==HIGH && buttonStartTime!=0){
    debounceTime=debounceDefault;
    buttonEndTime=millis(); //onko tarpeellinen?
    buttonStartTime=0;
    expoAdjustment=expoAdjustmentDefault;
    Serial.println("nappula nostettu");

  }


  while((digitalRead(selectButtonPin)==HIGH && digitalRead(upButtonPin)==HIGH && digitalRead(downButtonPin)==HIGH)){
    //wait
    }
  if(buttonStartTime==0){
    buttonStartTime=millis();
  }
  lastButtonTime=millis();


  if(digitalRead(selectButtonPin)==LOW){
    Serial.println("pressed select");
    return 1;

  }else if(digitalRead(upButtonPin)==LOW){
    Serial.println("pressed up");
    return 2;

  }else if(digitalRead(downButtonPin)==LOW){
    Serial.println("pressed down");
    return 3;

  }else{

  return 0;
  }
}


void shoot(int shootTime){
  Serial.println((String)"Shooting at:"+Xaxis.currentPosition()+","+Yaxis.currentPosition());
  digitalWrite(cameraPin, HIGH);
  delay(shootTime);
  digitalWrite(cameraPin, LOW);
}


float sensorX(){
  //1=36x24mm, 2=23.6x15.7mm
return(sensors[sensor][0]);
}
float sensorY(){
  //1=36x24mm, 2=23.6x15.7mm
return(sensors[sensor][1]);
}


int lensToSteps(int sensormm,int steps){
  return ((100.0-overlap)/100.0)*((steps/360.0)*((180/M_PI)*(2*atan(sensormm/(2.0*lenses[lens])))));
}




//-------------------------------------------//
//-- Actions --//

void scan360(){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Starting 360...");
  lcd.setCursor(0,1);
  lcd.print("dont touch!");
  delay(3000);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("dont touch!");
  int xTurns=round(0.5+(float)(turnSteps/lensToSteps(sensorX(),turnSteps)));
  int xTurnLength=turnSteps/xTurns;

  Serial.println((String)"turns on X: "+xTurns);
  Yaxis.runToNewPosition(tiltSteps/4);
  for(int x=0;x<xTurns;x++){

      //turn clockwise
      Xaxis.runToNewPosition(x*xTurnLength);
      delay(500);
      shoot(100);
      delay(100);

  }

  Yaxis.runToNewPosition(0);
  Xaxis.runToNewPosition(0);

}

void testi(){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("testi");
  for(int y=0;y<100;y++){
  Xaxis.runToNewPosition(turnSteps);
  Xaxis.runToNewPosition(0);}

}

void scan360x180(){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Starting 360x180");
  lcd.setCursor(0,1);
  lcd.print("dont touch!");
  delay(3000);

  int shotnumber=1;
  int xTurns=round(0.5+(float)(turnSteps/lensToSteps(sensorX(),turnSteps)));
  int yTurns=round(0.5+(float)((tiltSteps/2)/lensToSteps(sensorY(),tiltSteps)));
  int xTurnLength=turnSteps/xTurns;
  int yTurnLength=(tiltSteps/2)/yTurns;

  Serial.println((String)"turns on X: "+xTurns);
  Serial.println((String)"turns on Y: "+yTurns+"+up and down");
  Serial.println((String)"xTurnLength is: "+xTurnLength+" and needs "+xTurns+" to cover the distance "+xTurnLength*xTurns);
  Serial.println((String)"yTurnLength is: "+yTurnLength+" and needs "+yTurns+" to cover the distance "+yTurnLength*yTurns);

  for(int y=0;y<yTurns+1;y++){
    lcd.clear();
    lcd.setCursor(0,0);

    lcd.print("shooting ");
    lcd.print(shotnumber,DEC);
    lcd.print("/");

    lcd.print((yTurns*xTurns)-3,DEC);

    Yaxis.runToNewPosition(y*yTurnLength);
    if (y==0||y==yTurns){
      Serial.println((String)"shooting up or down");
      delay(stabilizingTime);
      shoot(expoTime);//shoot(100);
      shotnumber++;


    }else{
      for(int x=0;x<xTurns;x++){

        lcd.setCursor(9,0);
        lcd.print(shotnumber,DEC);
        lcd.print("/");
        lcd.print((yTurns*xTurns)-3,DEC);


        if(y%2==0){
          //turn counterclockwise
          Xaxis.runToNewPosition((xTurns-1)*xTurnLength-x*xTurnLength);
          delay(stabilizingTime);
          shoot(expoTime);//shoot(100);
          shotnumber++;

        }else{
          //turn clockwise
          Xaxis.runToNewPosition(x*xTurnLength);
          delay(stabilizingTime);
          shoot(expoTime);//shoot(100);
          shotnumber++;
        }
      }
    }
  }
  Yaxis.runToNewPosition(0);
  Xaxis.runToNewPosition(0);
}

void timelapse(){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Starting timelapse");
  lcd.setCursor(0,1);
  lcd.print("dont touch!");
  delay(3000);

  Yaxis.runToNewPosition(yStartAngle);
   
  for(int i=1;i<shots+1;i++){
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(i);
    lcd.print("/");
    lcd.print(shots);
    lcd.setCursor(0,1);
    lcd.print("(X:");
    lcd.print((360.0/turnSteps)*xShift*i,1);
    lcd.print(" Y:");
    lcd.print(360.0/tiltSteps*yStartAngle,1);
    lcd.print(")");

    if(i==1){
      delay(stabilizingTime);
      shoot(expoTime);
      delay(stabilizingTime);
      delay(intervalTime);
    }else{
      Xaxis.runToNewPosition(Xaxis.currentPosition()+xShift);
      Yaxis.runToNewPosition(Yaxis.currentPosition()+yShift);
      delay(stabilizingTime);
      shoot(expoTime);
      delay(stabilizingTime);
      delay(intervalTime);
    }
  }
  Yaxis.runToNewPosition(0);
  Xaxis.runToNewPosition(0);
}

void scan180(){

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("sorry, not");
  lcd.setCursor(0,1);
  lcd.print("implemented yet");
  delay(3000);



}


