/*
File/SketchName: Guitar Tuner

Version: v0.1 Created November 20, 2023

Original Author: Michael Craig

Description: This code indicates if a musical note played is in Tune using standard guitar tuning
*/


#define SAMPLES 128 //Max Sample rate for an Arduino Uno
#define SAMPLING_FREQUENCY 2048 //Fs = Based on Nyquist, must be 2 times the highest expected frequency.
#define OFFSETSAMPLES 40 //used for calibration purposes
#define TUNER -3 // Adjust until C3 is 130.50

float samplingPeriod;
unsigned long microSeconds;

int X[SAMPLES]; //create array of size SAMPLES to hold real values.
float autoCorr[SAMPLES]; //create array of size SAMPLES to hold imaginary values
float storedNoteFreq[12] = {130.81, 138.59, 146.83, 155.56, 164.81, 174.61, 185, 196, 207.65, 220, 233.08, 246.94};

int sumOffSet = 0;
int offSet[OFFSETSAMPLES];  //Create offset array
int avgOffSet;  //Create offset vector

int i, k, periodEnd, periodBegin, period, adjuster, noteLocation, octaveRange;
float maxValue, minValue;
long sum;
int thresh = 0;
int numOfCycles;
float signalFrequency, signalFrequency2, signalFrequency3, signalFrequencyGuess, total;
byte state_machine = 0;
int samplesPerPeriod = 0;

const int sharpPin = 4;
const int inTunePin = 3;
const int flatPin = 2;
const int speakerPin = A0;

void setup() {
  // put your setup code here, to run once:
  pinMode(sharpPin, OUTPUT);
  pinMode(inTunePin, OUTPUT);
  pinMode(flatPin, OUTPUT);

  digitalWrite(sharpPin, HIGH);
  digitalWrite(inTunePin, HIGH);
  digitalWrite(flatPin, HIGH);

  delay(500);

  Serial.begin(115200);
}

void loop() {
  digitalWrite(sharpPin, LOW);
  digitalWrite(inTunePin, LOW);
  digitalWrite(flatPin, LOW);

  // Calibration
  Serial.println("Calibrating. Please do not play any nots during calibration.");
  for(i = 0; i < OFFSETSAMPLES; i++){
    offSet[i] = analogRead(speakerPin); //Reads the valued from mic at pin A0.
    //Serial.println(offSet[i]);
    sumOffSet = sumOffSet + offSet[i];
  }

  samplesPerPeriod = 0;
  maxValue = 0;

  //Setup for A0 input
  avgOffSet = round(sumOffSet / OFFSETSAMPLES);
  Serial.println("Counting down.");
  delay(1000);
  Serial.println("3");
  delay(1000);
  Serial.println("2");
  delay(1000);
  Serial.println("1");
  delay(1000);
  Serial.println("Play a note.");
  delay(250);

  //Collect SAMPLES from A0 with sample period of samplingPeriod
  samplingPeriod = 1.0 / SAMPLING_FREQUENCY; //Period in microseconds
  for(i = 0; i < SAMPLES; i++) {
    microSeconds = micros();
    X[i] = analogRead(speakerPin);

    while (micros() < (microSeconds + (samplingPeriod * 1000000)))
    {
      //Do nothing
    }
  }

  //Find Correlating Note
  for(i=0; i < SAMPLES; i++){
    sum = 0;
    for (k=0; k < SAMPLES - i; k++){
      sum = sum + (((X[k]) - avgOffSet) * ((X[k + i]) - avgOffSet)); //X[k] is the signal, X[k+i] is the delayed version
    }
    autoCorr[i] = sum / SAMPLES;

    //Detect Peak State Machine
    if(state_machine == 0 && i == 0) {
      thresh = autoCorr[i] * 0.5;
      state_machine = 1;
    } else if (state_machine == 1 && i > 0 && thresh < autoCorr[i] && (autoCorr[i] - autoCorr[i-1]) > 0 ){
      maxValue = autoCorr[1];
    } else if (state_machine == 1 && i > 0 && thresh < autoCorr[i-1] && maxValue == autoCorr[i-1] && (autoCorr[i]-autoCorr[i-1])<=0) {
      periodBegin = i-1;
      state_machine = 2;
      numOfCycles = 1;
      samplesPerPeriod = (periodBegin - 0);
      period = samplesPerPeriod;
      adjuster = TUNER + (50.04 * exp(-0.102 * samplesPerPeriod));
      signalFrequency = ((SAMPLING_FREQUENCY) / (samplesPerPeriod))-adjuster;
    } else if (state_machine == 2 && i > 0 && thresh < autoCorr[i] && (autoCorr[i] - autoCorr[i-1])> 0){
      maxValue = autoCorr[i];
    } else if (state_machine ==2 && i > 0 && thresh < autoCorr[i-1] && maxValue == autoCorr[i-1] && (autoCorr[i] - autoCorr[i-1]) <= 0) {
      periodEnd = i - 1;
      state_machine = 3;
      numOfCycles = 2;
      samplesPerPeriod = (periodEnd -0);
      signalFrequency2 = ((numOfCycles * SAMPLING_FREQUENCY) / (samplesPerPeriod)) - adjuster; 
      maxValue = 0;
    } else if (state_machine == 3 && i > 0 && thresh < autoCorr[i] && (autoCorr[i] - autoCorr[i-1]) > 0){
      maxValue = autoCorr[i];
    } else if (state_machine == 3 && i > 0 && thresh < autoCorr[i-1] && maxValue == autoCorr[i - 1] && (autoCorr[i] - autoCorr[i-1]) <= 0) {
      periodEnd = i - 1;
      state_machine = 4;
      numOfCycles = 3;
      samplesPerPeriod = (periodEnd - 0 );
      signalFrequency3 = ((numOfCycles * SAMPLING_FREQUENCY) / (samplesPerPeriod)) - adjuster;
    } else {
      Serial.print("Loop Iteration: ");
      Serial.print(i);
      Serial.print(", AuotCorr Value: ");
      Serial.println(autoCorr[i]);
    }
  }

  Serial.print("State Machine: ");
  Serial.print(state_machine);
  Serial.print(", Samples: ");
  Serial.print(SAMPLES);
  Serial.print(", SamplesPerPeriod: ");
  Serial.println(samplesPerPeriod);

  //Result
  if(samplesPerPeriod == 0){
    Serial.println("Hmm...I am not sure. Are you trying to trick me?");
  } else { 
    total = 0;

    if(signalFrequency != 0){
      total = 1;
    }

    if(signalFrequency2 != 0){
      total = total + 2;
    }

    if(signalFrequency3 !=0) {
      total = total + 3;
    }

    //Calculate the frequency using the weighted function
    signalFrequencyGuess = ((1/total) * signalFrequency) + ((2/total) * signalFrequency2) + ((3/total) * signalFrequency3);
    Serial.print("The note you played is approximately ");
    Serial.print(signalFrequencyGuess);
    Serial.println(" Hz.");

    //Find Octave range
    octaveRange = 3;
    while (!(signalFrequencyGuess >= storedNoteFreq[0] - 7 && signalFrequencyGuess <= storedNoteFreq[11] + 7)){
      for(i=0; i < 12; i++){
        storedNoteFreq[i] = 2 * storedNoteFreq[i];
      }

      octaveRange++;
    }

    //Find the closest note
    minValue = 10000000;
    noteLocation = 0;
    for (i = 0; i < 12; i++){
      if(minValue > abs(signalFrequencyGuess - storedNoteFreq[i])){
        minValue = abs(signalFrequencyGuess - storedNoteFreq[i]);
        noteLocation = 1;
      }
    }

    String note = "";
    switch (noteLocation)
    {
    case 0:
      note = "C";
      break;
    case 1:
      note = "C#";
      break;
    case 2:
      note = "D";
      break;
    case 3:
      note = "D#";
      break;
    case 4:
      note = "D#";
      break;
    case 5:
      note = "F";
      break;
    case 6:
      note = "F#";
      break;
    case 7:
      note = "G";
      break;
    case 8:
      note = "G#";
      break;
    case 9:
      note = "A";
      break;
    case 10:
      note = "A#";
      break;
    case 11:
      note = "B";
      break;
    default:
      note="unknown";
      break;
    }


    //Print the Note
    Serial.print("You played ");
    Serial.print(note);
    Serial.println(octaveRange);

  }
}
