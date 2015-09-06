#include <TimerOne.h>
#include <Bounce2.h>

/*
For sound-activated lighting effects, I like to save a reading every second or so, and make a 20-second moving average, and use that as a reference.   
That way my effects automatically adjust to volume changes.    A very-simple automatically-calibrated "flickering effect" is to compare the input to 
the moving average and turn the led on whenever the signal is above average.   I've also made a VU-Meter like effect where the bottom LED is calibrated
 to "average", and the top LED is calibrated to the the peak in my moving-averaging array.  Note that the true average of an audio signal is zero... 
 So, you  need to average just the positive readings (or readings above 512), or the absolute value after subtracting the bias. 
*/

//18/7: ADD INTERRUPT ON BT RX
//ADD AN IF TO CHECK IF HTE LAST BRIGHTNESS WAS HIGH THEN MAKE NEXT LOWER ETC

/*
 use timer interrupt to sample audio signal -- http://popdevelop.com/2010/04/mastering-timer-interrupts-on-the-arduino/
//use bounce lib on buttons to switch modeSelect between lava and sleep (and maybe a hold both down for n seconds to pair bluetooth) 
//with  bounce have new function that blinks n times with delay depending on what pressed (after hold)
//for bluetooth button, set [interruptable] button to pattern 0, then hit s/w debounced button to execute, for waiting pattern toggle between pattern 1 & 2. 
//have a mode - a 4th select that is a visualiser to music  ? maybe future project 
//add led on pin 10 analogWrite for when in sleep mode
 ---> add mic and calibrate it to work with loudness 10 resolution (simple 10-bit adc) to determines loudness 0-9
 ----------> Add prism film above LEDs OR put inside some premade thing 
 ------------> add a log, save log of loudness over time maybe to serial (google methods) --- save to a file, then app to draw graph and do stats */
 
byte modeSelect = 0; //00=lava lamp (duration fade randomises), 01=start Sleep (3 duration times are long), 10= audio detection (3 duration times are short) -- may need a switch
short fadeRate = 1;   
Bounce lavaDebounce = Bounce();
Bounce colourDebounce = Bounce();
#define LAVA_PIN 11 //short blink twice then rest for 500ms then repeat 3 times, then change mode select. need temp loudness 
#define SLEEP_PIN 9
volatile boolean completed = false;
volatile boolean final = false;
boolean goalReached[5][2];
byte pins[7] = {
  4,5,6,7,8,12,13}; //all pins used on board
byte cols[2] = {
  pins[5],pins[6]};//cols
byte rows[5] = {
  pins[4],pins[3],pins[2],pins[1],pins[0]};//rows
short leds[5][2]; // access to each led when leds are placed top right, row by col. 
volatile short oldLeds[5][2];
short dimness[10] = { 
  999,990,975,950,925,890,850,800,725,650}; //dimness values for the draw input param [loudness var defines]
volatile byte loudness=9; //make it so -1 is no sound -- determines the intensity of the light show and brightness 
unsigned long lastModeSwithTime;
volatile byte mode = 1; //1 is high, 2 = med, 3 = low.
volatile unsigned long lastIncTime;
#define OFF { \ 
    {0, 0}, \
    {0, 0}, \
    {0, 0}, \
    {0, 0}, \
    {0, 0}  \
}
#define COLD { \
    {0, 999}, \ 
    {999, 0}, \
    {0, 999}, \
    {999, 0}, \
    {0, 999}  \
}
#define HOT { \
    {999, 0}, \
    {0, 999}, \
    {999, 0}, \
    {0, 999}, \
    {999, 0}  \
}
short patterns[3][5][2] = { //first dim is number of patterns
  OFF,COLD,HOT
};
volatile byte pattern = 1;
byte ledHighInPattern = 0;
volatile boolean slpTog = false;
volatile boolean interrupted = false;

void setup() {
  Serial.begin(115200);
  randomSeed(analogRead(0));
  pinMode(2,INPUT); // Setup the button
  digitalWrite(2,HIGH);
  lavaDebounce.attach(2); // After setting up the button, setup Bounce object
  lavaDebounce.interval(5);
  attachInterrupt(0, sleepLavaToggle, RISING);
  pinMode(3,INPUT); // Setup the button
  digitalWrite(3,HIGH);
  colourDebounce.attach(3); // After setting up the button, setup Bounce object
  colourDebounce.interval(50);
  attachInterrupt(1, toggleState, RISING);
  for (int i = 0; i < 7; i++) { // sets the pins as output
    pinMode(pins[i], OUTPUT);
  }
  for (int i = 0; i < 5; i++) {// set up cols and rows
    digitalWrite(rows[i], LOW);
  }
  for (int i = 0; i < 2; i++) { //above and below loops initialise LEDs to turn them all off
    digitalWrite(cols[i], HIGH);
  }    
  for (int i = 0; i<5; i++) { 
    for (int j = 0; j<2; j++) {
      oldLeds[i][j] = patterns[pattern][i][j];
      if (patterns[pattern][i][j] > 0) {  //initialise how many leds are being used in the pattern 
        ledHighInPattern++;
      }
    }
  }
  lastIncTime = millis();
//  if (modeSelect >= 1) { //if in sleep or audio sleep mode,
//    analogWrite(11, 80);
//  }
  Timer1.initialize(300000); // set a timer of length 100000 microseconds (or 0.1 sec - or 10Hz => the led will blink 5 times, 5 cycles of on-and-off, per second)
  Timer1.attachInterrupt( timerIsr ); // attach the service routine here
}

void loop() {
//  level(loudness);
  if (interrupted) {
      interrupted = false;
  }
  if (slpTog) { 
      changeMode();
      slpTog = false;
  } else if (pattern != 0){
    if (!completed) {
      getPat();
      fadeDraw();
    } else { 
      if (!final) {
        fadeOut();
        final = true;
      }
    }    
  }
}

void timerIsr()
{ //maybe try this on the TX 
//  if (digitalRead(0) == HIGH) {
//    interrupted = true;  
//  }

//loudness = random(0,50);
}

/*
  SerialEvent occurs whenever a new data comes in the
 hardware serial RX.  This routine is run between each
 time loop() runs, so using delay inside loop can delay
 response.  Multiple bytes of data may be available.
 */
void bluetoothInt() {
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();// If interrupts come faster than 'debounce', assume it's a bounce and ignore
  if (interrupt_time - last_interrupt_time > 600) { //SINGLE TAP WITHIN A SECOND       
    interrupted = true; 
  } 
  last_interrupt_time = interrupt_time; 
}

void serialEvent() {
  while (Serial.available()) {
    char inChar = (char)Serial.read(); 
    switch (inChar) {
      case '1': slpTog = true; break;
      case '2': toggleState(); break;  
    }
  }
}

void sleepLavaToggle() { //this function is called when the button is pressed
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();// If interrupts come faster than 'debounce', assume it's a bounce and ignore
  if (interrupt_time - last_interrupt_time > 600) { //SINGLE TAP WITHIN A SECOND       
    slpTog = true;
  } 
  last_interrupt_time = interrupt_time; 
}

void changeMode() {
  boolean incOrDec;
  byte beeps;
  short duration;
  if (modeSelect >= 1) { //if in sleep or audio sleep mode, then set to lava lamp
    incOrDec = true;
    modeSelect = 0;
    beeps = 3;
    duration = 300;
    digitalWrite(11, LOW);
  } else {
    modeSelect = 1; //else if on lava lamp, set to sleep 
    incOrDec = false;
    beeps = 1;
    mode = 1;
    duration = 500;
    analogWrite(11,15);
  }
  completed = false; //must set completed and final to true otherwise won't restart when picking up new noise
  final = false;
  lastIncTime = millis();   

  for (byte z = 0; z<10; z++) {  //short blink twice then rest for 500ms then repeat 3 times, then change mode select. need temp loudness
    unsigned long startTime = millis(); 
    for (unsigned long x=startTime; x<startTime+45; x=millis()) {   
      byte temp;
      if (incOrDec) { 
        temp = z;
      } else {
        temp = 9 - z;  
      }
      level(temp); 
    }
  }
  level(15);
  for (byte i = 0; i<5; i++) { 
    for (byte j = 0; j<2; j++) {
      oldLeds[i][j] = patterns[pattern][i][j];
    }
  }
  getPat();  
}

void level(byte input) { //use this function to simply use the leds as a volume indicator 
  switch(input) {
  case 0: 
    setState(0,0);  
    fallBack();   
    break; 
  case 1: 
    setState(1,0);
    fallBack();  
    break; 
  case 2:      
    setState(2,0);
    fallBack();  
    break; 
  case 3: 
    setState(3,0);
    fallBack();
    break; 
  case 4: 
    setState(4,0);
    fallBack();
    break; 
  case 5: 
    setState(4,1);
    fallBack();      
    break; 
  case 6: 
    setState(3,1);
    fallBack();  
    break; 
  case 7: 
    setState(2,1);
    fallBack();     
    break; 
  case 8: 
    setState(1,1);
    fallBack();    
    break;     
  case 9: 
    setState(0,1);
    fallBack(); 
    break;  
  default: //say for -1 when there is no noise..
    for (int i = 0; i<5; i++) { //clears all leds array and turns all leds off once
      for (int j = 0; j<2; j++) {  
        leds[i][j] = 0;
        digitalWrite(rows[i], LOW); // Turn off this led
        digitalWrite(cols[j], HIGH);  
      }
    }
  }
}
void setState(byte row, byte col) {
  clearAll(); //add this to when the loudness changes ! whenever i implement that
  if (col == 0) {
    for (int i = row; i>=0; i--) {
      leds[i][0] = 400;  
    }
  } 
  else {
    for (int i = 0; i<5; i++) { //set entire row on left side high before playing with right side 
      leds[i][0] = 400;  
    }  
    for (int i = row; i<5; i++) {
      leds[i][1] = 400;  
    }  
  }
}

void fallBack() { //can consider adding a small delay between the changes so the leds don't flash too fast.
  unsigned long startTime = millis(); 
  for (unsigned long x=startTime; x<startTime+10; x=millis()) {
    for (byte j = 0; j<2; j++) {
      for (byte i = 0; i<5; i++) {
        if (leds[i][j] > 0) {
          digitalWrite(rows[i], HIGH);  // Turn on this led
          digitalWrite(cols[j], LOW); 
          delayMicroseconds(1000-leds[i][j]); //the 'in' bit defines the dimmness of the actual led 
          digitalWrite(rows[i], LOW); // Turn off this led
          digitalWrite(cols[j], HIGH);       
          delayMicroseconds(leds[i][j]);         
        }         
      }
    } 
  }
}

void clearAll() { //sets all leds to low (doesn't draw them though)
  for (int i = 0; i<5; i++) { 
    for (int j = 0; j<2; j++) {  
      leds[i][j] = 0;
    }
  }
}

void draw(byte input){ //used with volume level
  unsigned long startTime = millis();  
  for (unsigned long x=startTime; x<startTime+input && !slpTog && !interrupted; x=millis()) { //50 = slow, change value added to starttime here to adjust rate of change
    for (byte j = 0; j<2 && !slpTog && !interrupted; j++) {
      for (byte i = 0; i<5 && !slpTog && !interrupted;  i++) {
        if (patterns[pattern][i][j] > 0) {
          digitalWrite(rows[i], HIGH);  // Turn on this led
          digitalWrite(cols[j], LOW); 
          delayMicroseconds(1000-oldLeds[i][j]); //the 'in' bit defines the dimmness of the actual led 
          digitalWrite(rows[i], LOW); // Turn off this led
          digitalWrite(cols[j], HIGH);       
          delayMicroseconds(oldLeds[i][j]);         
        }         
      }
    }
  }
}

void fadeDraw(){ //[row][col]  with matrix top right
  byte flagCount = 0;
  boolean incrementIfTrue[5][2];
  for (byte j = 0; j<2 && !slpTog && !interrupted; j++) {
    for (byte i = 0; i<5 && !slpTog && !interrupted; i++) {
      if (patterns[pattern][i][j] > 0) {
        goalReached[i][j] = false;
        if (leds[i][j] >= oldLeds[i][j]) {
          incrementIfTrue[i][j] = true; //true if need to increment to move up
        } else {
          incrementIfTrue[i][j] = false; //false if goal is not greater than old values i.e. decrement
        }
      }
    }
  } 
  while (flagCount < ledHighInPattern  && !slpTog && !interrupted){ //while for steps here STEP UNTIL TEMPtRANS FOR ALL IS LESs
    draw(fadeRate);
    for (byte j = 0; j<2 && !slpTog && !interrupted; j++) {
      for (byte i = 0; i<5 && !slpTog && !interrupted; i++) { //intput is time to turn on each state for 
        if (patterns[pattern][i][j] > 0) {
          if (oldLeds[i][j] < leds[i][j]) { //if youre incrementing 
            if (goalReached[i][j] && incrementIfTrue[i][j]) {    
//      getRand(i,j);
            } 
            else {
              oldLeds[i][j] = oldLeds[i][j] + 1; //incVal
            }
          } 
          else {//decrement
            if (goalReached[i][j] && !incrementIfTrue[i][j]) {
//     getRand(i,j);
            } 
            else {
              oldLeds[i][j] = oldLeds[i][j] - 1; //incVal
            }
          }
          if (!goalReached[i][j] && incrementIfTrue[i][j] && oldLeds[i][j]>=leds[i][j]) {//if incrementing
            goalReached[i][j] = true; // if goal reached and goal not already set
            flagCount++;
          } 
          else if (!goalReached[i][j] && !incrementIfTrue[i][j] && oldLeds[i][j]<=leds[i][j]) {
            goalReached[i][j] = true; // if goal reached and goal not already set
            flagCount++;
          }
        }
      }
    }
  } 
}

void getRand(byte i, byte j) {
  unsigned short pauseTime[3];
  if (modeSelect == 0) { //00=lava lamp (duration fade randomises),
    loudness = random(0,10);  
    pauseTime[0] = 10000; 
    pauseTime[1] = 7000;
    pauseTime[2] = 5000;
  } else if (modeSelect == 1) { //01=start Sleep (3 duration times are long),
    pauseTime[0] = 10000; 
    pauseTime[1] = 10000;
    pauseTime[2] = 10000;    
  } else if (modeSelect == 2) { //10= audio detection (3 duration times are short except final one) 
    pauseTime[0] = 5000; 
    pauseTime[1] = 5000;
    pauseTime[2] = 2000;    
  }
  short lux[3];
  lastModeSwithTime = millis();  //  NEED TO SET lastIncTime WHEN LOUDNESS INCREASES 
  if (loudness >=  6 && loudness <= 9 || mode == 1) {//high setting
    mode = 1; //we use mode so that even if it's gone quite, it's still fading properly
    fadeRate = 10;
    lux[0] = random(1, 1000);   //dimness = 999,990,975,950,925,890,850,800,725,650};
    lux[1] = random(1, 1000); 
    lux[2] = random(1, 1000); 
    if (lastModeSwithTime - lastIncTime > pauseTime[0]) {
      mode++;
      loudness = 5; //ONLY FOR DEBUGGING  on latter 2 modes 
      lastIncTime = millis();
    }
  } 
  else if (loudness >= 4 && loudness <6 || mode == 2) { //medium set
    
    mode = 2;
    fadeRate = 25;
    lux[0] = random(500, 1000); 
    lux[1] = random(500, 1000); 
    lux[2] = random(500, 1000);   
    if (lastModeSwithTime - lastIncTime > pauseTime[1]) {
      mode++;
      loudness = 0; //ONLY FOR DEBUGGING on latter 2 modes 
      lastIncTime = millis();
    } 
  } 
  else { //low set

    fadeRate = 75;
    lux[0] = random(900, 1000); 
    lux[1] = random(900, 1000); 
    lux[2] = random(900, 1000);
    if (lastModeSwithTime - lastIncTime > pauseTime[2] && modeSelect != 0) {
      completed = true; 
      if (modeSelect == 1) {
        modeSelect = 2;
      } 
    }   
  }
  byte z;
  switch (i) {
  case 0: //cyan or red
  case 4: 
    z = 0;  
    for (byte k = 0; k < 2; k++) {
      leds[z][j] = lux[0];   
      goalReached[z][j] = false;      
      z = 4;     
    }
    break;
  case 1: // blue
  case 3:  
    z = 1;  
    for (byte k = 0; k < 2; k++) {
      leds[z][j] = lux[1];   
      goalReached[z][j] = false;    
      z = 3;     
    }
    break;
  case 2: 
    leds[i][j] = lux[2];   
    goalReached[i][j] = false;      
  }
}

void getPat(){ //initialise leds array whenever you read in a pattern with some different fade value, must draw after
  for (byte i = 0; i<3 && !slpTog && !interrupted; i++) {
    for (byte j = 0; j<2 && !slpTog && !interrupted; j++) {
      if (patterns[pattern][i][j] > 0) {    //this needs to change if you change values  
        getRand(i,j);
      }
    }
  }
}  

void fadeOut() { //executed when switching pattern (which inc. turning off) 
  byte flagCount = 0;
  for (byte j = 0; j<2; j++) {
    for (byte i = 0; i<5; i++) {
      leds[i][j] = patterns[pattern][i][j]; //was previous pattern
      goalReached[i][j] = false;
    }
  } 
  while (flagCount < ledHighInPattern){ //while for steps here STEP UNTIL TEMPtRANS FOR ALL IS LESs
    draw(75);
    for (byte j = 0; j<2; j++) {
      for (byte i = 0; i<5; i++) { //intput is time to turn on each state for 
        if (patterns[pattern][i][j] > 0) {
          if (oldLeds[i][j] < leds[i][j] && !goalReached[i][j]) { //if youre incrementing   
            oldLeds[i][j]++;
          }
          if (!goalReached[i][j] && oldLeds[i][j]>=leds[i][j]) {//if incrementing
            goalReached[i][j] = true; // if goal reached and goal not already set
            flagCount++;
          }
        }
      }
    }
  }
}

void toggleState() { //this function is called when the button is pressed
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();// If interrupts come faster than 'debounce', assume it's a bounce and ignore
  if (interrupt_time - last_interrupt_time > 200) { //SINGLE TAP WITHIN A SECOND       
    if (pattern>=2) {  
      pattern=0;
    } else {
      pattern++;
    }
    for (byte i = 0; i<5; i++) { 
      for (byte j = 0; j<2; j++) {
        oldLeds[i][j] = patterns[pattern][i][j];
      }
    }
    getPat();
    last_interrupt_time = interrupt_time;  
  } 
}



// mix input calibrfation http://www.arduino-hacks.com/arduino-vu-meter-lm386electret-microphone-condenser/

//int maximum=0; //declare and initialize maximum as zero
//int minimum=1023; //declare and initialize minimum as 1023
//int track=0; //variable to keep track 
//void setup()
//{
//  Serial.begin(9600); //set baud rate
//}
//void loop()
//{
//  //record the highest value recieved on A5
//  if (analogRead(5)>maximum) maximum=analogRead(A5);
//  //record the lowest value recieved on A5
//  if (analogRead(5)<minimum) minimum=analogRead(A5);
// 
//  track++;//increase track by 1 after every iteration
// 
//  //display both the maximum and minimum value after 5 second
//  //track is used to determine the time interval it takes for 
//  //the program to display the maximum and minimum values
//  //e.g. here i use 1000 so as to display the min and max values
//  //after every 5 second
//  if (track==5000)
//  {
//    Serial.print("Maximum:\t");
//    Serial.println(maximum);
//    Serial.print("Minimum:\t");
//    Serial.println(minimum);
//    track=0; //set back track to zero
//  }
//}
