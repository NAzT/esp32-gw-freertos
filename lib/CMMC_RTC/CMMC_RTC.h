#ifndef CMMC_RTC_H
#define CMMC_RTC_H

#include <Arduino.h>
#include <CMMC_Module.h>

#include <Wire.h>
#include "RTClib.h"

class CMMC_RTC: public CMMC_Module{
  protected:
    RTC_DS1307 *rtc;
    char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
    char dateTimeCharArrray[60];
  public:
    CMMC_RTC();
    void setup();
    void loop();
    void configLoop();
    void configSetup();
    String getDateTimeString();
};

#endif