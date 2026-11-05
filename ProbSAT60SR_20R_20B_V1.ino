/*
 * ============================================================================
 * ProbSAT60SR_20R_20B_V1.ino
 * ============================================================================
 *
 * Probabilistic air-puff / water-reward task for an Arduino Yun.
 *
 * Each trial starts when the subject breaks an IR beam (SENSORPIN). One of
 * three trial types is drawn at random:
 *
 *   60 %  SR air puff + water reward
 *   20 %  R  sham puff + water      (no puff)
 *   20 %  B  sham puff + sham pump  (no puff, no water)
 *
 * Trial-type codes used in the sketch names:
 *   SR = air puff + water, S = air puff only (sham pump),
 *   R  = water only (sham puff), B = blank (sham puff + sham pump)
 *
 * Trial timeline (all times in ms, set in setup()):
 *
 *   beam break -> WAIT1 (random 200-800) -> PUFF (500) -> WAIT2 (500)
 *              -> PUMP (75) -> WAIT3 (>= 725 AND beam unblocked) -> next trial
 *
 * While a trial is running, the beam state, lick state and trial phase are
 * sampled roughly every 100 ms and logged as comma-separated rows to a text
 * file on the Yun's USB / SD storage. See arduino/README.md for the
 * full column description and how to
 * identify each trial type in the data.
 *
 * Hardware:     Arduino Yun (ATmega32U4 + Linux side, via the Bridge library)
 * Libraries:    Bridge (FileIO.h, Process), bundled with the Arduino IDE
 * Storage:      USB stick or micro-SD mounted at /mnt/sda1 on the Yun
 *
 * Pin map:
 *   D4  SENSORPIN        IR beam-break sensor input (internal pull-up;
 *                        LOW = beam blocked)
 *   D11 LICKPIN          Lick sensor input (HIGH = lick)
 *   D7  relayPinPuff     Air-puff valve relay
 *   D8  relayPinPump     Water pump relay
 *   D2  relayPinFake     Sham-puff relay (control for the puff)
 *   D10 relayPinFakeTwo  Sham-pump relay (control for the pump)
 *   A0  (unconnected)    Read once as a random seed
 *
 * Output files (on the Yun):
 *   /mnt/sda1/arduino/www/PrSAT60sr20r20b.txt<MM_DD_YY_T_hh_mm_ss>.txt  trial data
 *   /mnt/sda1/arduino/www/progressfile.txt                        status
 *
 * Authors:  [AUTHOR NAMES]
 * Lab:      [LAB NAME, INSTITUTION]
 * Paper:    [PAPER CITATION / "under review"]
 * License:  [LICENSE, e.g. MIT] -- see LICENSE in the repository root
 *
 * NOTE: Only comments (and leading whitespace) were added or edited when this file was curated for
 * publication. The executable code is unchanged from the version used to
 * collect the data reported in the paper.
 * ============================================================================
 */

#include <FileIO.h>   // Arduino Yun Bridge library: FileSystem, File, Process

// ---------------------------------------------------------------------------
// Global state
// ---------------------------------------------------------------------------

// Stage start times (ms since power-on, from millis()). 0 means "not active".
unsigned long time;            // unused
unsigned long wait3start;      // start of WAIT3 (post-pump interval)
unsigned long puffstart;       // start of PUFF stage
unsigned long blockedstart;    // set when a new trial is triggered by the beam
unsigned long wait2start;      // start of WAIT2 (puff -> pump interval)
unsigned long pufftime;        // PUFF duration (ms)
unsigned long wait3time;       // WAIT3 nominal duration (ms); 200 ms is subtracted in loop()
unsigned long pumptime;        // pump / sham-pump pulse duration (ms)
unsigned long wait2time;       // WAIT2 duration (ms)
unsigned long waterprob;       // unused (superseded by trialtype)
unsigned long touch;           // unused
unsigned long lastwrite;       // unused
unsigned long npump;           // running count of real water deliveries
unsigned long wait1start;      // start of WAIT1 (beam break -> puff)
unsigned long wait1time;       // WAIT1 duration for the current trial (random, ms)
unsigned long loopdelay;       // delay after each logged sample (ms) -> ~10 Hz sampling
unsigned long puffprob;        // unused (superseded by trialtype)
unsigned long trialtype;       // random draw 0-99 that sets the trial type

// Buffer of data rows held in RAM until they are written to the file.
// Each row is at most 22 characters + terminator.
char printdata[45][23];
unsigned long datacounter;     // number of rows currently in printdata

// Output pins (relays)
#define relayPinPump 8         // water pump
#define relayPinPuff 7         // air-puff valve
#define relayPinFake 2         // sham puff (control)
#define relayPinFakeTwo 10     // sham pump (control)

// Input pins (sensors)
#define SENSORPIN 4            // IR beam break, LOW when blocked
#define LICKPIN 11             // lick sensor, HIGH when licking

int temp;                      // return value of Printprogressfile() (unused)
//const int savePin = 5;
//String filename;
String filenameWeb;            // full path of this session's data file
String cleaneddatetime;        // session start date/time, made filename-safe
boolean recording;             // unused
String dataString;             // the data row being built in loop()
double runtime;                // time of the current sample (s since power-on)
boolean rewardreal;            // true = real water this trial
boolean puffreal;              // true = real air puff this trial
int PuffWrite;                 // 1/0 copy of puffreal for the data row

//-SETUP-SETUP-SETUP-SETUP-SETUP-SETUP-SETUP-SETUP-------------------------------------------------------------------------------------
// Runs once at power-on: sets task parameters, configures pins, starts the
// Bridge, waits for the lick sensor to settle and builds the data filename.
void setup() {


  // --- Task parameters -----------------------------------------------------
  npump = 0; //set number of pumps at 0
  wait3start = 0; //set WAIT3 start at 0
  puffstart = 0; //set puffstart at 0
  blockedstart = 0; //set blockedstart at 0
  wait2start = 0; //set WAIT2 start at 0
  wait1start = 0; //set wait1start at 0
  wait1time = 0; //set wait1time at 0
  loopdelay = 100; //delay after each logged sample (ms)
  pufftime = 500; //PUFF stage lasts for x milliseconds
  wait2time = 500; //WAIT2 (puff -> pump) lasts for x milliseconds
  wait3time = 925; //WAIT3 nominal duration in ms (effective minimum is wait3time - 200 = 725 ms)
  pumptime = 75; //pump pulse lasts for x milliseconds
  waterprob = 80; //unused: the trial probabilities are set by the trialtype thresholds in loop()
  puffprob = 60; //unused: the trial probabilities are set by the trialtype thresholds in loop()
  datacounter = 0;
  PuffWrite = 0;

  // --- Pins ----------------------------------------------------------------
  // initialize the sensor pin as an input:
  pinMode(SENSORPIN, INPUT);
  digitalWrite(SENSORPIN, HIGH); // turn on the pullup
  pinMode(relayPinPump, OUTPUT);
  pinMode(relayPinPuff, OUTPUT);
  pinMode(relayPinFake, OUTPUT);
  pinMode(relayPinFakeTwo, OUTPUT);
  pinMode(LICKPIN, INPUT);

  // --- Bridge / file system (Linux side of the Yun) ------------------------
  Bridge.begin();
  FileSystem.begin();
  FileSystem.remove("/mnt/sda1/arduino/www/progressfile.txt");
  File dataFile = FileSystem.open("/mnt/sda1/arduino/www/progressfile.txt", FILE_APPEND); //open the file
  if (dataFile) { //if the file is available, write to it
    dataFile.println("Starting");
    dataFile.close();
  }

  randomSeed(analogRead(A0)); //seed the random generator from the floating A0 pin

  while (digitalRead(LICKPIN)) { //if the lick pin is on when the arduino is on then delay so voltage can stabilize
    temp = Printprogressfile("initializing");
    delay(1000);
  }

  //pinMode(savePin, INPUT); //switch is an input
  //recording = false; //no recording

  // --- Data filename -------------------------------------------------------
  // `date +%D-%T` returns e.g. "11/03/26-12:00:00", which becomes
  // "11_03_26_T_12_00_00". The resulting path is
  // /mnt/sda1/arduino/www/PrSAT60sr20r20b.txt11_03_26_T_12_00_00.txt
  cleaneddatetime += getTimeStamp(); //the cleaneddatetime is the date and time
  cleaneddatetime.replace("/", "_"); //replace with an underscore
  cleaneddatetime.replace("-", "_T_"); //replace with _T_
  cleaneddatetime.replace(":", "_"); //replace with an underscore
  filenameWeb += "/mnt/sda1/arduino/www/PrSAT60sr20r20b.txt" + cleaneddatetime += ".txt";

}

//------------------------------------------------------------------------------
// Overwrites progressfile.txt (served by the Yun's web server) with a single
// status line, so the session can be monitored remotely.

int Printprogressfile(String progressfiledata) {
  FileSystem.remove("/mnt/sda1/arduino/www/progressfile.txt");
  File dataFile = FileSystem.open("/mnt/sda1/arduino/www/progressfile.txt", FILE_APPEND); //open the file
  if (dataFile) { //if the file is available, write to it
    /*    dataFile.println("Puff Time in ms");
        dataFile.println(String(pufftime));
        dataFile.println("First Pause Time in ms");
        dataFile.println(String(wait2time));
        dataFile.println("Pause Time in ms");
        dataFile.println(String(wait3time));
        dataFile.println("Pump Time in ms");
        dataFile.println(String(pumptime));
        dataFile.println("Water Probability in %");
        dataFile.println(String(waterprob));
    */    dataFile.println(progressfiledata);
    dataFile.close();
  }
  return 0;

}



//------------------------------------------------------------------------------
// Returns the Yun's Linux system date and time as "MM/DD/YY-hh:mm:ss".

String getTimeStamp() {
  String result;
  Process time;
  // date is a command line utility to get the date and the time
  // in different formats depending on the additional parameter
  time.begin("date");
  time.addParameter("+%D-%T");  // parameters: D for the complete date mm/dd/yy
  //             T for the time hh:mm:ss
  time.run();  // run the command

  // read the output of the command
  while (time.available() > 0) {
    char c = time.read();
    if (c != '\n') {
      result += c;
    }
  }
  return result;
}


//-LOOP-LOOP-LOOP-LOOP-LOOP-LOOP-LOOP-LOOP--------------------------------------------------------------------------------------
// The trial is a state machine. Each stage stores its start time in a
// *start variable; a value of 0 means the stage is not active. Every pass
// through loop() checks each stage in order, advances it when its time is
// up, and (while any stage is active) logs one data row.
void loop() {
  dataString = ""; //clear the data row for this pass

  // --- Trial trigger -------------------------------------------------------
  if  (!digitalRead(SENSORPIN)) { //if sensor is blocked
    if ((puffstart == 0) && (wait3start == 0) &&  (wait2start == 0) && (wait1start == 0)) { //if no trial is running
      blockedstart = millis(); //start a new trial
    }
    else {
      blockedstart = 0;
    }
  }

  //-BLOCKED: choose the trial type----------------------------------------------------------------

  if (blockedstart > 0) { //a new trial has been triggered
    blockedstart = 0;
    trialtype = random(100); //random integer 0-99 sets the trial type

    if (trialtype < 60) { //60% of trials: SR, air puff + water
      rewardreal = true; //real water
      puffreal = true; //real puff
    }
    else if (trialtype < 80) { //20% of trials: R, sham puff + water (no puff)
      rewardreal = true; //real water
      puffreal = false; //sham puff
    }
    else { //20% of trials: B, sham puff + sham pump (no puff, no water)
      rewardreal = false; //sham pump
      puffreal = false; //sham puff
    }
    wait1time = random(200, 801); //WAIT1 duration: random integer 200-800 ms
    wait1start = millis(); //start WAIT1
  }

  //-WAIT1: beam break -> puff-----------------------------------------------------------------------

  if (wait1start > 0) { //if WAIT1 is running
    if ((wait1time) < (millis() - wait1start)) { //if WAIT1 is over
      wait1start = 0; //end WAIT1
      wait1time = 0; //reset WAIT1 duration (logged as 0 from here on)
      puffstart = millis(); //start the PUFF stage
      if (puffreal) { //if actually puffing
        digitalWrite(relayPinPuff, HIGH); //turn puff on
      }
      else { //if not puffing
        digitalWrite(relayPinFake, HIGH); //turn sham puff on
      }
    }
  }

  //-PUFF: air puff (or sham) on for pufftime---------------------------------------------------------

  if (puffstart > 0) { //if the PUFF stage is running
    if ((pufftime) < (millis() - puffstart)) { //if the PUFF stage is over
      puffstart = 0; //end PUFF
      if (puffreal) {
        digitalWrite(relayPinPuff, LOW); //turn puff off
      }
      else {
        digitalWrite(relayPinFake, LOW); //turn sham puff off
      }
      wait2start = millis(); //start WAIT2
    }
  }

  //-WAIT2: puff -> pump, then pump (or sham) pulse---------------------------------------------------

  if (wait2start > 0) { //if WAIT2 is running
    if ((wait2time) < (millis() - wait2start)) { //if WAIT2 is over
      wait2start = 0; //end WAIT2
      if (rewardreal) { //if actually pumping
        npump = npump + 1; //adding one to the total number of times pumped
        digitalWrite(relayPinPump, HIGH); //turn pump on
        delay(pumptime); //length of a pump (blocking)
        digitalWrite(relayPinPump, LOW); //turn pump off
      }
      else { //if not actually pumping
        digitalWrite(relayPinFakeTwo, HIGH); //turn sham pump on
        delay(pumptime); //length of a pump (blocking)
        digitalWrite(relayPinFakeTwo, LOW); //turn sham pump off
      }
      wait3start = millis(); //start WAIT3
    }
  }

  //-WAIT3: post-pump interval; trial ends when time is up AND the beam is clear------------------
  if (wait3start > 0) { //if WAIT3 is running
    if (((wait3time - 200) < (millis() - wait3start)) && (digitalRead(SENSORPIN))) { //at least 725 ms elapsed and beam unblocked
      wait3start = 0; //end WAIT3 -> trial over
      // Flush the buffered rows, then write an end-of-trial marker row.
      File dataFileWeb = FileSystem.open(filenameWeb.c_str(), FILE_APPEND); //open the file
      if (dataFileWeb) { //if the file is available, write to it
        for (int x = 0; x < datacounter; x++) {
          String tempString = String(printdata[x]);
          dataFileWeb.println(tempString);
          for (int y = 0; y < 23; y++) {
            printdata[x][y] = (char)0;
          }
        }

        // End-of-trial marker: "<time>,0,0,7,0,<puff flag>".
        // <time> is the timestamp of the last logged sample of this trial
        // (runtime is captured before it is updated on the next line).
        String tempString2 = String(runtime);
        String PuffString1;
        if(puffreal){
          PuffString1 = String(1);
        }
        else{
          PuffString1 = String(0);
        }
        runtime = float(millis()) / 1000; //calculate seconds
        dataFileWeb.println(tempString2 + ",0,0,7,0," + PuffString1);

        dataFileWeb.close();
        datacounter = 0;
      }

    }
  }




  //}

  //-DATA ROW: built and buffered on every pass while a trial is running----------------------------
  // Row format: time_s, beam, lick, phase, wait1_ms, puff
  //   beam   1 = blocked, 0 = clear
  //   lick   2 = licking, 0 = not licking
  //   phase  trials with real water: 3 = WAIT1, 4 = PUFF (real or sham),
  //          5 = WAIT2, 7 = WAIT3; trials without water: always 9
  //   wait1_ms  this trial's WAIT1 duration (0 once WAIT1 has ended)
  //   puff   1 = real puff trial, 0 = sham puff trial

  if (!(blockedstart == 0) || !(puffstart == 0) || !(wait1start == 0) || !(wait2start == 0) || !(wait3start == 0)) { //if a trial is running
    // dataString += getTimeStamp(); //start the date
    // dataString += ","; //separate with a comma
    runtime = float(millis()) / 1000; //calculate seconds
    dataString += runtime; //time in seconds (2 decimal places)
    dataString += ","; //separate with a comma
    if (!digitalRead(SENSORPIN)) { //if the sensor is blocked
      dataString += "1"; //write a 1
    }
    else {
      dataString += "0"; //otherwise write a 0
    }
    dataString += ","; //separate with a comma
    if (digitalRead(LICKPIN) != 0) { //if licked
      dataString += "2"; //write a 2
    }
    else {
      dataString += "0"; //otherwise write a 0
    }
    dataString += ","; //separate with a comma


    if (rewardreal) {
      if (wait1start > 0) { //if in WAIT1
        dataString += "3"; //write a 3
      }
      if (puffstart > 0) { //if in PUFF
        dataString += "4"; //write a 4
      }
      if (wait2start > 0) { //if in WAIT2
        dataString += "5"; //write a 5
      }
      if (wait3start > 0) { //if in WAIT3
        dataString += "7"; //write a 7
      }
    }
    else {
      dataString += "9"; //9 = no-water trial (phase not coded)
    }

    dataString += ","; //separate with a comma
    dataString += wait1time;

    dataString += ",";
    if (puffreal) { //if actually puffing
      PuffWrite = 1;
    }
    else { //if not puffing
      PuffWrite = 0;
    }
    dataString += PuffWrite;


    // Store the row (truncated to 22 characters) in the RAM buffer and
    // write the buffer to the file once 40 rows have accumulated.
    char charBuf[23];
    dataString.toCharArray(charBuf, 23);
    strcpy(printdata[datacounter], charBuf);
    datacounter = datacounter + 1;
    if (datacounter == 40) {
      datadump();
    }

    delay(loopdelay);
  }


}

//- END LOOP-ENDLOOP-END LOOP-END LOOP-END LOOP-END LOOP-END LOOP-END LOOP------------------------------------------------------------
//------------------------------------------------------------------------------
// Appends all buffered rows to the data file and clears the buffer.

void datadump() {

  File dataFileWeb = FileSystem.open(filenameWeb.c_str(), FILE_APPEND); //open the file
  if (dataFileWeb) { //if the file is available, write to it
    for (int x = 0; x < datacounter; x++) {
      String tempString = String(printdata[x]);
      dataFileWeb.println(tempString);
      for (int y = 0; y < 23; y++) {
        printdata[x][y] = (char)0;
      }
    }
    dataFileWeb.close();
    datacounter = 0;
  }
}
