#include "PowerMeter.h"
#include "ModbusPowerMeter.h"
#include "CC1101.h"

DeviceData deviceData;
void setup() {
  powerMeterSetup(&deviceData);
  cc1101Setup(&deviceData);
}

void loop() {
  powerMeterLoop(&deviceData);
  cc1101Loop(&deviceData);
}
