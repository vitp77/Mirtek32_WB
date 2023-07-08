struct PollingSettings { // 4
  uint16_t pollingPeriod      = 0x0000;
  uint16_t onceADay           = 0x0000;
  uint16_t hourPollingADay    = 0x0000;
  uint16_t minutesPollingADay = 0x0000;
};

struct HoldingData { // 5
  uint16_t powerMeterAddress = 0x0000;
  uint16_t baudRate   = 0x0000;
  uint16_t parity     = 0x0000;
  uint16_t stopBits   = 0x0000;
  uint16_t modbusAddr = 0x0000;
  PollingSettings pollingSettings;
};

struct DeviceData { // 42

  HoldingData holdingData;

  uint16_t year    = 0x0000;
  uint16_t month   = 0x0000;
  uint16_t day     = 0x0000;
  uint16_t hours   = 0x0000;
  uint16_t minutes = 0x0000;
  uint16_t seconds = 0x0000;

  uint16_t energyh = 0x0000;
  uint16_t energyl = 0x0000;

  uint16_t energyT1h = 0x0000;
  uint16_t energyT1l = 0x0000;

  uint16_t energyT2h = 0x0000;
  uint16_t energyT2l = 0x0000;

  uint16_t energyT3h = 0x0000;
  uint16_t energyT3l = 0x0000;

  uint16_t f  = 0x0000;
  uint16_t c  = 0x0000;
  uint16_t u1 = 0x0000;
  uint16_t u2 = 0x0000;
  uint16_t u3 = 0x0000;

  uint16_t i1h = 0x0000;
  uint16_t i1l = 0x0000;

  uint16_t i2h = 0x0000;
  uint16_t i2l = 0x0000;

  uint16_t i3h = 0x0000;
  uint16_t i3l = 0x0000;

  uint16_t serialNumber0 = 0x0000;
  uint16_t serialNumber1 = 0x0000;
  uint16_t serialNumber2 = 0x0000;
  uint16_t serialNumber3 = 0x0000;
  uint16_t serialNumber4 = 0x0000;
  uint16_t serialNumber5 = 0x0000;
  uint16_t serialNumber6 = 0x0000;
  uint16_t serialNumber7 = 0x0000;
  uint16_t serialNumber8 = 0x0000;
  uint16_t serialNumber9 = 0x0000;
  uint16_t serialNumber10 = 0x0000;
  uint16_t serialNumber11 = 0x0000;
  uint16_t serialNumber12 = 0x0000;

  uint16_t lastError = 0x0000;

  uint16_t secondsWorkh = 0x0000;
  uint16_t secondsWorkl = 0x0000;

  uint16_t vcc = 0x0000;

};
