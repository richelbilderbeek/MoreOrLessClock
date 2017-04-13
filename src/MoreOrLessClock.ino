/*
  MoreOrLessClock
  (C) 2015 Richel Bilderbeek

2015-04-04: v.1.0: Initial version

Description 7-segment-display, has common anode

 
18 17  16 15  14 13 12  11 10
+-------------+-------------+
|             |             |
|    /---\    |    /---\    |
|    | A |    |    | A |    |
|  / \---/ \  |  / \---/ \  |
|  | |   | |  |  | |   | |  |
|  |F|   |B|  |  |F|   |B|  |
|  | |   | |  |  | |   | |  |
|  \ /---\ /  |  \ /---\ /  |
|   X  G  X   |   X  G  X   |
|  / \---/ \  |  / \---/ \  |
|  | |   | |  |  | |   | |  |
|  |E|   |C|  |  |E|   |C|  |
|  | |   | |  |  | |   | |  |
|  \ /---\ /  |  \ /---\ /  |
|    | D |    |    | D |    |
|    \---/    |    \---/    |
|             |             |
+-------------+-------------+
1  2  3   4   5   6  7   8  9

1: E left
2: D left
3: C left
4: dot left
5: E right
6: D right
7: G right
8: C right
9: dot right
10: B right
11: A right
12: F right
13: VCC right: connect to Arduino 5V
14: VCC right: connect to Arduino 5V
15: B left
16: A left
17: G left
18: F left

Description shift register chip SN74HC595:

  16 15 14 13 12 11 10 9
  |  |  |  |  |  |  |  |
  +--+--+--+--+--+--+--+
  |>                   |
  +--+--+--+--+--+--+--+
  |  |  |  |  |  |  |  |
  1  2  3  4  5  6  7  8

SN74HC595 pin names:

   1: Q2
   2: Q3
   3: Q4
   3: Q5
   5: Q6
   6: Q7
   7: Q8 
   8: GND
   9: QH*: for daisy chaining, unused otherwise
  10: SRCLR* (to 5V)
  11: SRCLK (to D4): clock pin
  12: RCLK (to D3): latch pin
  13: OE* (to GND)
  14: SER (to D2): data pin
  15: Q1
  16: VCC (to 5V)

Left capacitive sensor:

       +--------+     +--------+  
   2 --+   RS   +--+--+   RH   |-- 4
       +--------+  |  +--------+
                   |
                   X

  2: sensor pin
  4: helper pin
  RH: 'resistance helper', resistance of at least 1 Mega-Ohm (brown-black-green-gold)
  RS: 'resistance sensor', resistance of 1 kOhm (brown-black-red-gold)
  X: place to touch wire

Right capacitive sensor:

       +--------+     +--------+  
   7 --+   RS   +--+--+   RH   |-- 4
       +--------+  |  +--------+
                   |
                   X

  7: sensor pin
  4: helper pin
  RH: 'resistance helper', resistance of at least 1 Mega-Ohm (brown-black-green-gold)
  RS: 'resistance sensor', resistance of 1 kOhm (brown-black-red-gold)
  X: place to touch wire

*/

//if NDEBUG is #defined, this is a release version
#define NDEBUG

#include <Time.h>

const int data_pin  =  2; 
const int latch_pin =  3;
const int clock_pin =  4;
const int error_pin = 13;

enum ClockMode { hours_and_minutes, minutes_and_seconds};

//const ClockMode clock_mode = minutes_and_seconds;
const ClockMode clock_mode = hours_and_minutes;

//The maximum number of minutes the MoreOrLessClock may deviate
const int max_delta_mins = 2;

// Phase, start in 0 or 1 chosen by chance: 
//   0: from correct time to be max_delta_mins lagging -> 2
//   1: from correct time to be max_delta_mins ahead -> 3
//   2: from lagging max_delta_mins to being max_delta_mins ahead -> 3
//   3: from being max_delta_mins ahead to lagging max_delta_mins -> 2
enum Phase { correct_to_lagging, correct_to_ahead, lagging_to_ahead, ahead_to_lagging };
//Chose a random initial starting phase
Phase phase = (rand() >> 5) % 0 ? correct_to_lagging : correct_to_ahead;
// The number of ticks the next phase starts
int next_phase_ticks 
  = max_delta_mins * 5 //The number of real minutes it takes to lag max_delta_mins
    * 60 //To seconds
    * 10 //The program uses a delay(100), thus takes ten ticks per loop
  ;
int current_phase_ticks = 0;

//Date of release
const int release_day   =    6;
const int release_month =    4;
const int release_year  = 2015;
 
void OnError(const String& error_message)
{
  Serial.print("ERROR: ");  
  Serial.println(error_message);  
  while (1)
  {
    //Blink LED
    digitalWrite(error_pin,!digitalRead(error_pin));
    //Write to serial
    delay(1000);
  }
}

void DisplayPhase()
{
  switch (phase)
  {
    case correct_to_lagging: Serial.println("Phase: correct to lagging"); return;
    case correct_to_ahead: Serial.println("Phase: correct to ahead"); return;
    case lagging_to_ahead: Serial.println("Phase: lagging to ahead"); return;
    case ahead_to_lagging: Serial.println("Phase: ahead to lagging"); return;
  }
  OnError("Unknown phase");
}

void NextPhase()
{
  current_phase_ticks = 0;

  next_phase_ticks 
    = max_delta_mins 
      *  5 //The number of real minutes it takes to lag max_delta_mins
      * 60 //To seconds
      * 10 //The program uses a delay(100), thus takes ten ticks per loop
      *  2 //Because the deviation has to change two amplitudes
  ;

  switch (phase)
  {
    case correct_to_lagging: 
    case ahead_to_lagging:
    {
      phase = lagging_to_ahead; 
      DisplayPhase();
      return;
    }
    case correct_to_ahead:
    case lagging_to_ahead:
    {
      phase = ahead_to_lagging;
      DisplayPhase();
      return;
    }
  }
  OnError("Unknown phase");
}

void SetTimeFromSerial()
{
  //Serial.println("Start of SetTimeFromSerial");
  const int  h  = Serial.available() ? Serial.parseInt() : -1;
  delay(10);
  const char c1 = Serial.available() ? Serial.read() : '0';
  delay(10);
  const int  m  = Serial.available() ? Serial.parseInt() : -1;
  delay(10);
  const char c2 = Serial.available() ? Serial.read() : '0';
  delay(10);
  const int  s  = Serial.available() ? Serial.parseInt() : -1;
  delay(10);
  const String used = String(h) + String(c1) + String(m) + String(c2) + String(s);
  if (h == -1) 
  {
    Serial.println(String("No hours, use e.g. '12:34:56' (used '") + used + String("')"));
    return;
  }
  if (c1 == '0') 
  {
    Serial.println("No first seperator, use e.g. '12:34:56' (used '" + used + "')");
    return;
  }
  if (m == -1) 
  {
    Serial.println("No minutes, use e.g. '12:34:56' (used '" + used + "')");
    return;
  }
  if (c2 == '0') 
  {
    Serial.println("No second seperator, use e.g. '12:34:56'");
    return;
  }
  if (s == -1) 
  {
    Serial.println("No seconds, use e.g. '12:34:56'");
    return;
  }
  if (h < 0 || h > 23)
  {
    Serial.println("Hours must be in range [0,23]");
    return;
  }
  if (m < 0 || h > 59)
  {
    Serial.println("Minutes must be in range [0,59]");
    return;
  }
  if (s < 0 || s > 59)
  {
    Serial.println("Seconds must be in range [0,59]");
    return;
  }
  setTime(h,m,s,release_day,release_month,release_year); 

  phase = (rand() >> 5) % 0 ? correct_to_lagging : correct_to_ahead;
  // The number of ticks the next phase starts
  next_phase_ticks 
    = max_delta_mins * 5 //The number of real minutes it takes to lag max_delta_mins
      * 60 //To seconds
      * 10 //The program uses a delay(100), thus takes ten ticks per loop
    ;
  current_phase_ticks = 0;
}


void setup()
{
  pinMode(data_pin,  OUTPUT);
  pinMode(latch_pin, OUTPUT);
  pinMode(clock_pin, OUTPUT);  
  pinMode(error_pin, OUTPUT);  
  Serial.begin(9600); //Cannot be used: chip is used stand-alone
  #ifndef NDEBUG
  Serial.println("MoreOrLessClock v. 1.0 (debug version)");
  #else //NDEBUG
  Serial.println("MoreOrLessClock v. 1.0 (release version)");
  #endif //NDEBUG
  //Set the date to the release data, otherwise the clock may lag to underflow values
  setTime(0,0,0,release_day,release_month,release_year); 
  DisplayPhase();
}

void loop()
{
  while (1)
  {

    //Respond to touches
    //const int sensors_state = GetSensors();
    //if (sensors_state == state_left_sensor_pressed) { SetTime(); delay(100); }

    if (Serial.available())
    {
      delay(100);
      SetTimeFromSerial();  
    }
    
    //Change the phase
    {
      ++current_phase_ticks;
      if (current_phase_ticks >= next_phase_ticks)
      {
        NextPhase();  
      }
    }
    //Change the time
    {
      //This would be a standard implementation:
      //      
      //  switch(rand() % 2) //Use bias in the lower digits
      //  {
      //    case 0: adjustTime( 1); break;
      //    case 1: adjustTime(-1); break;
      //    default: break;
      //  }
      //
      //This implementation results in a clock progression of 377 minutes in 506 real minutes,
      //that is, it lags 129 minutes per 506 real minutes,
      //or, it lags 1 minute per 5 real minutes.
      //
      //The user can set a maximum deviation, max_delta_mins. The get at this deviation from the correct time,
      //5 * max_delta_mins must pass.
      // 
      // Phase, start in 0 or 1 chosen by chance: 
      //   0: from correct time to be max_delta_mins lagging -> 2
      //   1: from correct time to be max_delta_mins ahead -> 3
      //   2: from lagging max_delta_mins to being max_delta_mins ahead -> 3
      //   3: from being max_delta_mins ahead to lagging max_delta_mins -> 2
      //
      switch (phase)
      { 
        case correct_to_lagging:
        case ahead_to_lagging:
        {
          switch(rand() % 2) //Use bias in the lower digits
          {
            case 0: adjustTime( 1); break;
            case 1: adjustTime(-1); break; //Chosen more often
          }
        }
        case correct_to_ahead:
        case lagging_to_ahead:
        {
          switch(rand() % 2) //Use bias in the lower digits
          {
            case 0: adjustTime(-1); break;
            case 1: adjustTime( 1); break; //Chosen more often
          }
        }
      }
    }
    
    //Show the time
    const int s = second();
    const int m = minute();
    const int h = hour();
    
    ShowTime(s,m,h);

    //Show tme in serial monitor
    { 
      const String time_now = String(h) + ":" + String(m) + ":" + String(s);
      Serial.println(time_now);
      delay(100);
    }
  }
}

void ShowTime(const int secs, const int mins, const int hours)
{
  #ifndef NDEBUG
  if (hours <  0) { OnError("ShowTimeRainbow: hours <  0, hours = " + String(hours)); }
  if (hours > 23) { OnError("ShowTimeRainbow: hours > 23, hours = " + String(hours)); }
  if (mins <  0) { OnError("ShowTimeRainbow: mins <  0, mins = " + String(mins)); }
  if (mins > 59) { OnError("ShowTimeRainbow: mins > 59, mins = " + String(mins)); }
  if (secs <  0) { OnError("ShowTimeRainbow: secs <  0, secs = " + String(secs)); }
  if (secs > 59) { OnError("ShowTimeRainbow: secs > 59, secs = " + String(secs)); }
  #endif // NDEBUG
  switch(clock_mode)
  { 
    case hours_and_minutes:
      ShowBinary(
        ~DigitToBinary(hours / 10),
        ~DigitToBinary(hours % 10),
        ~DigitToBinary(mins / 10),
        ~DigitToBinary(mins % 10)
      );
    break;
    case minutes_and_seconds:
      ShowBinary(
        ~DigitToBinary(mins / 10),
        ~DigitToBinary(mins % 10),
        ~DigitToBinary(secs / 10),
        ~DigitToBinary(secs % 10)
      );
    break;
  }
}

int DigitToBinary(const int digit)
{
  switch (digit)
  {
    case 0: return B11111100;
    case 1: return B01100000;
    case 2: return B11011010;
    case 3: return B11110010;
    case 4: return B01100110;
    case 5: return B10110110;
    case 6: return B10111110;
    case 7: return B11100000;
    case 8: return B11111110;
    case 9: return B11110110;
  } 

}

void ShowBinary(const int a, const int b, const int c, const int d)
{
  // Stuur de data naar het shift register
  shiftOut(data_pin, clock_pin, LSBFIRST, d);
  shiftOut(data_pin, clock_pin, LSBFIRST, c);
  shiftOut(data_pin, clock_pin, LSBFIRST, b);
  shiftOut(data_pin, clock_pin, LSBFIRST, a);

  // Zet de latch aan en uit, zodat de outputs aan gaan
  digitalWrite(latch_pin, HIGH);
  digitalWrite(latch_pin, LOW);

}

