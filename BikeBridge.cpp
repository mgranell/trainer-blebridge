#include <Arduino.h>
#include <Bounce2.h>
#include <SimplyAtomic.h>

#include "BikeBridge.h"

#define LED_BUILTIN 2

const int ledPin = LED_BUILTIN;

// RPM Tracking
const int revPin = 32;
Bounce revBounce = Bounce();

// Rev change instruction and motor signal tracking
const int downSwitchPin = 26; // When low, resistance is decreasing
const int upSwitchPin = 27; // When low, resistance is increasing
const int pwmPin = 25; // Every falling edge represents a resistance change

Bounce downSwitch = Bounce(); // De-bounce the revolution input
Bounce upSwitch = Bounce(); // De-bounce the revolution input
Bounce pwmBounce = Bounce();

#define ZERO_RPM_DURATION_MS 60000

int currentResistance = 0;
int currentChangeDirection = 0;


// Change in time, handling roll-over
inline unsigned long deltaTime(unsigned long newTime, unsigned long oldTime) {
    if (newTime < oldTime) {
      return newTime + (ULONG_MAX - oldTime);
    }
    else {
      return newTime - oldTime;
    }
}

/////////////////////////
// RPM Tracking
///////////////////////////
long lastDisplayedPeriod = 0;
long revPulseWidth = 0;
void BikeBridge::checkForRevolution() {
  revBounce.update();
  if (revBounce.fell())
  {
    // The period of time held high (the previous period) is the rotation period of the wheel.
    if (revBounce.previousDuration() != lastDisplayedPeriod) 
    {
      crankCount++;
      lastCrankEventTime = millis();
      lastDisplayedPeriod = revBounce.previousDuration();

      int rpm = 60000 / (lastDisplayedPeriod + revPulseWidth);
      power = calculatePower(currentResistance, rpm);

      pLog->publish(MQTT_TOPIC_RPM, 
        "%d,%d,%d,%d", 
        rpm,
        power,
        lastDisplayedPeriod,
        revPulseWidth);       
    }
  }
  // Check for zero RPM
  else if (revBounce.rose()) {
      revPulseWidth = revBounce.previousDuration();

      if (lastDisplayedPeriod > 0
      && revBounce.duration() > ZERO_RPM_DURATION_MS ) {
        lastDisplayedPeriod = 0;
        power = 0;
        
        pLog->publish(MQTT_TOPIC_RPM, "0,0,%d,%d", revBounce.duration(), revPulseWidth);
      }
  }
}


///////////////////////////
// Track resistance changes
///////////////////////////
bool previousPwmLevel = false;

//IRAM_ATTR
void pwmChange() {
  pwmBounce.update();
}

// Check the PwmLevel and return true if changed.
bool BikeBridge::checkPwmLevel() {
  ATOMIC() {
    pwmBounce.update();
  }

  if (!pwmBounce.changed()) {
    // pwm is unstable
    return false;
  }

  bool currentPwmLevel = pwmBounce.read();
  if (currentPwmLevel != previousPwmLevel) {
      previousPwmLevel = currentPwmLevel;
      pLog->publish("cycle/pwmraw","%d,%s,%d", millis(), currentPwmLevel ? "rise" : "fall", pwmBounce.previousDuration());      
      return true;
  }

  return false;
}


int oldDown = -1;
int oldUp = -1;
long lastChange = 0;


/// Calcuate power in Watts from the resistance level (l) and rpm
int BikeBridge::calculatePower(int l, int rpm) {
  const float coef_lr2 = 8.79297E-05;	
  const float coef_r2 = -0.00209295;
  const float coef_l2 =	0.000510261;
  const float coef_lr =	9.64488E-05;
  const float coef_r =	0.42830974;
  const float coef_l =	0.133448598;
  const float coef_c =	4.622736387;

  int r2 = rpm * rpm;
  float result = 
      coef_lr2 * l * r2
    + coef_r2 * r2 
    + coef_l2 * (l * l)
    + coef_lr * (l * rpm)
    + coef_r * rpm
    + coef_l * l
    + coef_c;

  return (int)result;
}

void BikeBridge::updateChangeDirection() {
    //upSwitch.update();
    //downSwitch.update();

    int oldDirection = currentChangeDirection;

    int newUp = digitalRead(upSwitchPin);
    int newDown = digitalRead(downSwitchPin);
    if (newUp != oldUp || newDown != oldDown) {
      long now = millis();
      pLog->publish("cycle/switch","%d - lastchangedelta=%d, down=%d, up=%d", now, now - lastChange, newDown, newUp);
      oldUp = newUp;
      oldDown = newDown;
      lastChange = now;
    }

    if (newUp != newDown) {
      // We are changing.
      if (!newDown) {
        // Going down (low on the down pin, high on up pin)
        currentChangeDirection = -1;
      }
      else {
        // Going up
        currentChangeDirection = 1;
      }
    } else {
      if (newUp == 1) {
        // Both high, we have stopped changing
        currentChangeDirection =0 ;
      }
      // Otherwise we are both low, which is the "stabilisation" period. Leave direction as is.
    }
    
    // Other options is one or the other pins are in an unstable state.
    if (currentChangeDirection != oldDirection) {
      pLog->publish("cycle/direction","%d - change in resistance dir to %d", millis(), currentChangeDirection);
    }
}


void BikeBridge::setup(NetworkService * pNeworkService) {
  pLog = pNeworkService;

  revBounce.attach(revPin, INPUT_PULLUP);

  upSwitch.attach(upSwitchPin,INPUT_PULLUP);
  downSwitch.attach(downSwitchPin, INPUT_PULLUP);
  pwmBounce.attach(pwmPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(pwmPin), pwmChange, CHANGE);

  pinMode(ledPin, OUTPUT);  
}

void BikeBridge::loop() {
  digitalWrite(ledPin, digitalRead(revPin));

  // Check for another rev
  checkForRevolution();

  // Check the signals to see whether we are meant to be increasing/decreasing resistance
  updateChangeDirection();
  bool changed = checkPwmLevel();

  if (changed && pwmBounce.fell()) {
    // If we have a falling edge from the motor, then we need to inc/dec the measure resistance
    currentResistance += currentChangeDirection;
    if (currentResistance < 0) {
        currentResistance = 0;
    }

    pLog->publish("cycle/resistance","rawresistance,%d,%d,%d", millis(), currentChangeDirection, currentResistance);
  }
}
