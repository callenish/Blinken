/*
  Blinkenlights on a 120 Volt circuit.
  
  Repeat cycles a 120V circuit (possibly a lamp, hence the name) on for a period then off for a period, repeatedly.

  The hardware I used for this project is principally an Arduino, a PowerSwitch Tail 2 to handle the 120V circuit, 
  an optional DS1307 RTC kit (from AdaFruit) to do the scheduling with, and a spring-loaded toggle switch to change 
  between the three running modes: REPEAT, SCHEDULE, and RUN. See the README.md for more details.
  
  This code uses an internal pull-up resistor for the switch but none is required for the PST2. It assumes one 
  ground connection comes from a GND terminal and one is a GPIO pin set to LOW. Change setup() if you want 
  something different in either case.

  Constants you may want to change (time periods and pin assignments) are collected at the top of the sketch.

  This code is in the public domain. No warranty is expressed or implied.
*/

// Constants
const unsigned long onPeriod = 30000;             // Time to turn the circuit on in ms
const unsigned long offPeriod = 20000;            // Time to keep the circuit off in ms
const unsigned long debouncePeriod = 50;          // Increase if the switch acts up, we're mostly waiting around anyway
const unsigned long blinkOnPeriod = 1000;
const unsigned long blinkOffPeriod = 1000;
const unsigned long millisOffset = 0; // Useful for debugging the code executed when millis() overflows;

  // Define your pins
const int controlPin = 8;             // Controls whether 120V circuit turns on or not
const int groundPin = 7;              // This should change to 9 once I use headers, or may use standard GND pin
const int modePin = 11;               // Toggles between scheduled, constant, and hold mode
const int indicatorPin = 13;          // Reflect operating state with on-board led, on = schedule mode

// Enums
enum Mode {REPEAT, SCHEDULE, RUN};    // Modes the program can run in: always repeating, repeating on schedule, or always on 
enum IndicatorState {OFF, BLINKING, ON};          // States the indicator can be in, matches the modes in order

// Global Variables
Mode currentMode = SCHEDULE;          // Default mode is SCHEDULE
IndicatorState currentIndicatorState = BLINKING;  // Matching indicator default state is BLINKING
int currentPowerState = LOW;          // Note that we don't have a PowerState enum. Instead, we use HIGH and LOW

// the setup routine runs once when you press reset or boot the system
void setup() {                
  // initialize the digital pin as an output.
  pinMode(controlPin, OUTPUT);     
  pinMode(groundPin, OUTPUT);
  pinMode(indicatorPin, OUTPUT);  
  pinMode(modePin, INPUT_PULLUP);     // Use internal resistor on switch
  digitalWrite(groundPin, LOW);       // Take no chances, ground pin must have voltage LOW
  Serial.begin(57600);                // For logging
  delay(5000);                        // Give time to get the serial monitor up
  update_indicator(true);             // Reset the indicator state
}

// the loop routine is restarted from the top once you return or fall off the end
void loop() {
  // Begin ON half of cycle
  if (request_power_state(HIGH)) {
    Serial.print("Starting On Cycle for ");
    Serial.print(onPeriod / 1000UL);
    Serial.println(" seconds");
  }
  if (run_half_cycle(onPeriod) && currentMode != RUN) {   // false if a mode change ends the cycle prematurely
    if (request_power_state(LOW)) {
      Serial.print("Starting Off Cycle for ");
      Serial.print(offPeriod / 1000UL);
      Serial.println(" seconds");
    }
    run_half_cycle(offPeriod);
  }
}

// Decide whether to set the power to the state that has been requested, returning whether the request was carried out
// Eventually the RTC schedule stuff will live here
boolean request_power_state(int requestedState) {
  int newState;
  boolean requestHonored = false;

  Serial.print("DEBUG: Requested Power State ");
  Serial.println(requestedState);
  // Will need to check whether we are currently in a scheduled ON period before allowing an ON state with RTC
  switch (currentMode) {
    case REPEAT: newState = requestedState; requestHonored = true; break;
    case SCHEDULE: newState = requestedState; requestHonored = true; break;
    case RUN: newState = HIGH; requestHonored = (requestedState == HIGH); break;
  }
  if (newState != currentPowerState) {
    currentPowerState = newState;
    digitalWrite(controlPin, currentPowerState);
    Serial.print("DEBUG: Power State Changed, Time=");
    Serial.println(my_millis());
  }
  if (requestHonored) {
    Serial.print("DEBUG: Request Honored, Time=");
    Serial.println(my_millis());
  }
  return requestHonored;
}

// Run through on or off portion of cycle, returns whether the half cycle completed or was interrupted by mode change
boolean run_half_cycle(unsigned long period) {
  static Mode lastSeenMode = currentMode;
  unsigned long initialTime = my_millis();
  unsigned long nextPowerStateChange;
  
  Serial.print("DEBUG: Running Half Cycle for next ");
  Serial.print(period);
  Serial.println("ms");
  nextPowerStateChange = next_switch_time(initialTime, period);
  while (!time_surpassed(nextPowerStateChange, initialTime)) {
    update_indicator(currentMode != lastSeenMode);    // Update indicator for blinking or mode change
    lastSeenMode = currentMode;
    mode_change_check();              // Check for switch toggling through the modes (returns after debouncing time)
    if (currentMode != lastSeenMode) {
      lastSeenMode = currentMode;
      Serial.print("DEBUG: Bailing on cycle because mode has changed, Time=");
      Serial.println(my_millis());
      return false;                   // Premature cycle end due to mode change 
    }
  }
  return true;
}

// Give the time to next make a switch from on to off or vice versa
// Right now it is bog-simple, but once we get the RTC and schedules...
unsigned long next_switch_time(unsigned long currTime, unsigned long holdTime) {
  unsigned long nextSwitch = currTime + holdTime;

  Serial.print("Time when power state is next changed: ");
  Serial.println(nextSwitch);
  return nextSwitch;
}

// Indicates whether the current time has surpassed the target time, taking overflow into consideration
boolean time_surpassed(unsigned long targetTime, unsigned long originalTime) {
  unsigned long currTime = my_millis();
  
  if ((targetTime < originalTime) && (currTime > originalTime)) {
    return false;                     // Haven't overflowed yet, so we couldn't have reached target time
  } else {
    return (currTime > targetTime);
  }
}

// check for mode change caused by debounced switch being pushed and adjust indicator accordingly
void mode_change_check() {
  int lastVal;
  unsigned long initialTime;
  unsigned long fullyDebouncedTime;
  int val = digitalRead(modePin);     // read the input pin
  
  if (val == LOW) {                   // We have a mode change. Now wait out any debounce
    lastVal = val;
    increment_mode();
    match_indicator_to_mode();        // This will take care of resetting the indicator for the mode change
    Serial.print("Mode Change: ");
    Serial.println(mode_to_string());
    
    // First wait until pin goes HIGH again. This marks the beginning of the debounce period
    Serial.print("DEBUG: Switch LOW state detected=");
    Serial.println(my_millis());
    while (val == LOW) {
      val = digitalRead(modePin);
      lastVal = val;
      update_indicator(false);        // Need to keep blinking in SCHEDULE mode, we've already done the mode change
    }
    Serial.print("DEBUG: Switch back in HIGH state");
    Serial.println(my_millis());

    // Initialize debounce
    initialTime = my_millis();
    fullyDebouncedTime = initialTime + debouncePeriod;
    Serial.print("DEBUG: Initial Debounce Time=");
    Serial.print(initialTime);
    Serial.print("    Target Debounce Time=");
    Serial.println(fullyDebouncedTime);
    
    // Now wait out the debounce period while still updating a blinking indicator
    while (!time_surpassed(fullyDebouncedTime, initialTime)) {
      val = digitalRead(modePin);
      if (val != lastVal) {           // Switch changed state, must be noise since we are still debouncing
        initialTime = my_millis();    // Restart debounce period
        fullyDebouncedTime = initialTime + debouncePeriod;
      }
      update_indicator(false);        // Need to keep blinking in SCHEDULE mode, but no mode change allowed in debounce
    }
    Serial.print("DEBUG: Finished Debounce at ");
    Serial.println(my_millis());
  }
}

// Change the mode to its next state
// I'd return the new Mode but it would require an extra header file and I don't use the return value anyway
void increment_mode() {
  switch(currentMode) {
    case REPEAT: currentMode = SCHEDULE; break;
    case SCHEDULE: currentMode = RUN; break;
    case RUN: currentMode = REPEAT; break;
  }
}

// Match the LED indicator state to the mode we are running in
void match_indicator_to_mode() {
  IndicatorState lastSeenState = currentIndicatorState;
  switch(currentMode) {
    case REPEAT: currentIndicatorState = OFF; break;
    case SCHEDULE: currentIndicatorState = BLINKING; break;
    case RUN: currentIndicatorState = ON; break;
  }
  update_indicator(lastSeenState != currentIndicatorState);
}

// Return a string that represents the current mode we are running in
char* mode_to_string() {
  switch(currentMode) {
    case REPEAT: return "REPEAT";
    case SCHEDULE: return "SCHEDULE";
    case RUN: return "RUNNING";
  }
  return "UNKNOWN";                   // Should never get here
}

// Update the indicator to reflect the current mode, or to blink it
void update_indicator(boolean modeChange) {
  static int indicatorState = LOW;
  static boolean blinking = false;    // Could just use currentMode == SCHEDULE, but this is clearer
  static unsigned long blinkStart;
  static unsigned long blinkEnd;
  boolean changeIndicatorState = modeChange;
  
  if (modeChange) {
    switch(currentMode) {
      case REPEAT: indicatorState = LOW; blinking = false; break;
      case SCHEDULE:
        blinking = true;
        blinkStart = my_millis();
        blinkEnd = blinkStart + blinkOnPeriod;    // Begin blinking with the ON portion
        indicatorState = HIGH;
        break;
      case RUN: indicatorState = HIGH; blinking = false; break;
    }
  }
  if (blinking && time_surpassed(blinkEnd, blinkStart)) {
    changeIndicatorState = true;
    blinkStart = my_millis();
    if (indicatorState == HIGH) {     // Switch indicator from ON to OFF
      blinkEnd = blinkStart + blinkOffPeriod;
      indicatorState = LOW;
    } else {                          // Switch from OFF to ON
      blinkEnd = blinkStart + blinkOnPeriod;
      indicatorState = HIGH;
    }
  }
  if (changeIndicatorState) {
    digitalWrite(indicatorPin, indicatorState);
  }
}

// This exists solely so that I can debug code that deals with overflowing the millis() return value
unsigned long my_millis() {
  return millis() + millisOffset;
}

