////////////////////////////////////
////////////////////////////////////
// John O'Leary
// Embedded Systems Final Lab
// University California Riverside
// May 31st, 2023
////////////////////////////////////
////////////////////////////////////

#include "Timer.h"
#include "LiquidCrystal.h"
#include "pitches.h"

// Sound Variables
int buzzer = 3;

int melody[] = {
 NOTE_G4, NOTE_A4, NOTE_B4, NOTE_C5
};

// == Song 1 ==
int song1[] = {
  NOTE_A1, NOTE_G1, NOTE_E1, NOTE_G1, NOTE_A1,
  NOTE_B1, NOTE_B1, NOTE_B1, NOTE_A1, NOTE_G1,
  NOTE_A1, NOTE_A1, NOTE_G1, NOTE_E1, NOTE_G1,
  NOTE_A1, NOTE_B1, NOTE_A1, NOTE_G1, NOTE_E1
};
int song1_time[] = {
4, 8, 8, 4, 8, 8, 8, 4, 8, 8, 4, 8, 8, 8, 4, 4, 4, 8, 8, 8, 8
};

// == Song 2 ==
int song2[] {
  NOTE_E2, NOTE_G2, NOTE_A2, NOTE_E2, NOTE_G2,
  NOTE_E2, NOTE_G2, NOTE_A2, NOTE_E2, NOTE_G2,
  NOTE_C3, NOTE_D3, NOTE_C3, NOTE_E2, NOTE_G2,
  NOTE_C3, NOTE_D3, NOTE_C3, NOTE_E2, NOTE_G2
};
int song2_time[] = {
8, 4, 8, 4, 8, 8, 4, 8, 4, 8, 8, 4, 8, 4, 8, 8, 4, 8, 4, 8
};

// == Song 3 ==
int song3[] {
  NOTE_E3, NOTE_A3, NOTE_D4, NOTE_G3, NOTE_C4,
  NOTE_E3, NOTE_A3, NOTE_D4, NOTE_G3, NOTE_C4,
  NOTE_E3, NOTE_A3, NOTE_D4, NOTE_G3, NOTE_C4,
  NOTE_E3, NOTE_A3, NOTE_D4, NOTE_G3, NOTE_C4
};
int song3_time[] = {
3, 1, 2, 5, 4, 2, 4, 1, 3, 5, 5, 3, 2, 1, 4, 4, 5, 3, 1, 2
}; 

// LCD variables
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);





// Task Structure Definition
typedef struct task {
   int state;                  // Tasks current state
   unsigned long period;       // Task period
   unsigned long elapsedTime;  // Time elapsed since last task tick
   int (*TickFct)(int);        // Task tick function
} task;


const unsigned char tasksNum = 3;
task tasks[tasksNum]; // We have 2 tasks

// Task Periods
const unsigned long periodLCDOutput = 100;
const unsigned long periodJoystickInput = 100;
const unsigned long periodSoundOutput = 100;
const unsigned long periodController = 500;


// GCD 
const unsigned long tasksPeriodGCD = 100;

// Task Function Definitions
int TickFct_LCDOutput(int state);
int TickFct_JoystickInput(int state);
int TickFct_SoundOutput(int state);

// Task Enumeration Definitions
enum LO_States {LO_init, LO_MenuSong1, LO_MenuSong2, LO_MenuSong3};
enum JI_States {JI_init, JI_Switch, JI_MoveLeft, JI_MoveRight, JI_Clicked};
enum SO_States {SO_init, SO_Song1, SO_Song2, SO_Song3, SO_SoundOff};

void TimerISR() { // TimerISR actually defined here
  unsigned char i;
  for (i = 0; i < tasksNum; ++i) { // Heart of the scheduler code
     if ( tasks[i].elapsedTime >= tasks[i].period ) { // Ready
        tasks[i].state = tasks[i].TickFct(tasks[i].state);
        tasks[i].elapsedTime = 0;
     }
     tasks[i].elapsedTime += tasksPeriodGCD;
  }
}


void LCDWriteLines(String line1, String line2) {
  lcd.clear();          
  lcd.setCursor(0, 0);
  lcd.print(line1);
  lcd.setCursor(0,1);
  lcd.print(line2);
}

// Task Function Definitions
int song_number = 0;  // The song selected to play 
int song_paused = 0;  // Song paused? 1 yes, 0 no
int resume_song = 0;  // The song to return to after a pause
int menuOption = 0;   // The menu to be displayed
int new_select = 0;   // A new song has been selected

// Helper function used to determine part of 
//    the message printed on the display
String GetSongMessage(){
  String message = ""; 
  switch(resume_song){
    case 0:
      message = "Select Song";
      break;
    case 1:
      if( song_paused ){ message = "Paused: Song 1"; }
      else { message = "Playing: Song 1"; }
      break;
    case 2:
      if( song_paused ){ message = "Paused: Song 2"; }
      else { message = "Playing: Song 2"; }
      break;
    case 3:
      if( song_paused ){ message = "Paused: Song 1"; }
      else { message = "Playing: Song 3"; }
      break;
    default:
      break;
  }
  return message;
}

// Menu display SM:
// Displays the menu associated with the menuOption
// 1: Select song 1
// 2: Select song 2
// 3: Select song 3
int TickFct_LCDOutput(int state) {
  switch (state) { // State Transitions
    case LO_init:
      state = LO_MenuSong1;
    break;
    case LO_MenuSong1:
      //if(menuOption < 1) { state = LO_MenuPR; }
      if(menuOption > 1) { state = LO_MenuSong2; }
      break;
    case LO_MenuSong2:
      if(menuOption < 2) { state = LO_MenuSong1; }
      if(menuOption > 2) { state = LO_MenuSong3; }
      break;
    case LO_MenuSong3:
      if(menuOption < 3) { state = LO_MenuSong2; }
      break;
    default:
      state = LO_MenuSong1;
      break;
  }
  switch (state) { // State Actions
    case LO_MenuSong1:
      LCDWriteLines(GetSongMessage(), "Select Song1");
      break;
    case LO_MenuSong2:
      LCDWriteLines(GetSongMessage(), "Select Song2");
      break;
    case LO_MenuSong3:
      LCDWriteLines(GetSongMessage(), "Select Song3");
      break;
  }
  return state;
}

// Joystick input SM:
// The stick moving left or right moves the menu
// Clicking the stick selects the option in the current menu
int TickFct_JoystickInput(int state) {
  switch (state) { // State Transitions
    case JI_init:
      state = JI_Switch;
      break;
    case JI_Switch:
      // if stick has moved...
      if( analogRead(A1) > 800 ){ state = JI_MoveLeft; } // LEFT, move menu to the left
      if( analogRead(A1) < 300 ){ state = JI_MoveRight; } // RIGHT, move menu to the right
      // If stick has clicked...
      if( digitalRead(10) == LOW ){ state = JI_Clicked; } // Select the menu option
      break;
    case JI_MoveLeft:
      state = JI_Switch;
      break;
    case JI_MoveRight:
      state = JI_Switch;
      break;
    case JI_Clicked:
      state = JI_Switch;
      break;
    default:
      state = JI_Switch;
      break;
  }

   switch (state) { // State Actions
    case JI_Switch:
      // Do Nothing
      break;
    case JI_MoveLeft:
      // Move the menu to the left by one number, until 1
      if( menuOption > 1 ){ menuOption--; }
      break;
    case JI_MoveRight:
      // MOve the menu to the right by one number, until 3
      if( menuOption < 3 ){ menuOption++; } 
      break;
    case JI_Clicked:
        // If a song has been paused, resume the song. Toggle paused to false.
        if( song_paused ) { song_number = resume_song; song_paused = 0; }
        // If a song is not playing, select teh current song.
        else if( song_number == 0 ) {
          song_number = menuOption;
          resume_song = song_number;
          song_paused = 0;
          new_select = 1;
        }
        // If a song is playing, pause the song. Remember the song number and set paused to true.
        else{ resume_song = song_number; song_number = 0; song_paused = 1; }
      break;
  }
  return state;
}

// Sound Output SM:
// Play the song selected in the joystick SM
// Stop playing after the song has ended ( 20 notes )
int counter = 0;
int note = 0;
int TickFct_SoundOutput(int state) {
  switch (state) { // State Transitions
    case SO_init:
      state = SO_SoundOff;
    break;
    case SO_Song1:
      if(note == 20) {    // End the song after the 20th note
        state = SO_SoundOff;
        song_number = resume_song = 0;
        counter = 0;
        note = 0;
      }
      else{
        if( new_select ){ note = 0; new_select = 0; } // Restart if the song is newly selected
        if(counter >= song1_time[note]) {
         state = SO_SoundOff;
         note++;
         counter = 0;
        }
      }
      break;
    case SO_Song2:
      if(note == 20) {    // End the song after the 20th note
        state = SO_SoundOff;
        song_number = resume_song = 0;
        counter = 0;
        note = 0;
      }
      else{
        if( new_select ){ note = 0; new_select = 0; } 
        if(counter >= song2_time[note]) {
         state = SO_SoundOff;
         note++;
         counter = 0;
        }
      }
      break;
    case SO_Song3:
      if(note == 20) {    // End the song after the 20th note
        state = SO_SoundOff;
        song_number = resume_song = 0;
        counter = 0;
        note = 0;
      }
      else{
        if( new_select ){ note = 0; new_select = 0; }
        if(counter >= song3_time[note]) {
         state = SO_SoundOff;
         note++;
         counter = 0;
        }
      }
      break;
    case SO_SoundOff:
      // Switch to the song selected in the joystick SM
      switch(song_number){
        case 0:
          // Paused state, Stay in song off
          break;
        case 1:
          state = SO_Song1;
          break;
        case 2:
          state = SO_Song2;
          break;
        case 3:
          state = SO_Song3;
          break;
        default:
          state = SO_Song1;
          break;
      }
      if( song_number != 0 ){
        counter++;
        counter = counter % 4;
      }
    break;
    
  }
   switch (state) { // State Actions
    case SO_Song1:
      tone(8, song1[note], periodSoundOutput* song1_time[note]);
      counter++;
      break;
    case SO_Song2:
      tone(8, song2[note], periodSoundOutput* song2_time[note]);
      counter++;
      break;
    case SO_Song3:
      tone(8, song3[note], periodSoundOutput* song3_time[note]);
      counter++;
      break;
    case SO_SoundOff:
      noTone(8);
    break;
  }
  return state;
}



void InitializeTasks() {
  unsigned char i=0;
  tasks[i].state = LO_init;
  tasks[i].period = periodLCDOutput;
  tasks[i].elapsedTime = tasks[i].period;
  tasks[i].TickFct = &TickFct_LCDOutput;
  ++i;
  tasks[i].state = JI_init;
  tasks[i].period = periodJoystickInput;
  tasks[i].elapsedTime = tasks[i].period;
  tasks[i].TickFct = &TickFct_JoystickInput;
  ++i;
  tasks[i].state = SO_init;
  tasks[i].period = periodSoundOutput;
  tasks[i].elapsedTime = tasks[i].period;
  tasks[i].TickFct = &TickFct_SoundOutput;
}

void setup() {
  // put your setup code here, to run once:
  InitializeTasks();

  TimerSet(tasksPeriodGCD);
  TimerOn();
  Serial.begin(9600);
  // Initialize Outputs
  lcd.begin(16, 2);

  // Initialize Inputs
  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  pinMode(10, INPUT_PULLUP);

}

void loop() {
  // put your main code here, to run repeatedly:
  // Task Scheduler with Timer.h
  while(!TimerFlag) {}
  TimerFlag = 0;
}
