#include <ModbusRTU.h>
#include <EEPROM.h>
#include <iarduino_VCC.h>
#include <TimerMs.h>

#define ADDRESS_BAUD_RATE              0
#define ADDRESS_PARITY                 2
#define ADDRESS_STOP_BITS              4
#define ADDRESS_MODBUS_ADDRESS         6
#define ADDRESS_POWERMETER_ADDRESS     8
#define ADDRESS_POLLING_PERIOD        10
#define ADDRESS_ONCE_A_DAY            12
#define ADDRESS_HOUR_POLLING_A_DAY    14
#define ADDRESS_MINUTES_POLLING_A_DAY 16

#define RXTX_PIN 8

ModbusRTU mb;
TimerMs tmrpw(1000, 0, 1); //инициализируем таймер
bool needCalcTemp;
bool needStartCalcTemp;
bool CalcTempStarted;

HoldingData holdingData;

void readBaudRate(DeviceData *deviceData) {
  EEPROM.get(ADDRESS_BAUD_RATE, holdingData.baudRate);
  if (holdingData.baudRate == 0 || holdingData.baudRate > 1152) {
    holdingData.baudRate = 96;
  }
  deviceData->holdingData.baudRate = holdingData.baudRate;
}

void storeBaudRate(DeviceData *deviceData) {
  holdingData.baudRate = deviceData->holdingData.baudRate;
  EEPROM.put(ADDRESS_BAUD_RATE, holdingData.baudRate);
}

void readParity(DeviceData *deviceData) {
  EEPROM.get(ADDRESS_PARITY, holdingData.parity);
  if (holdingData.parity > 2) {
    holdingData.parity = 0;
  }
  deviceData->holdingData.parity = holdingData.parity;
}

void storeParity(DeviceData *deviceData) {
  holdingData.parity = deviceData->holdingData.parity;
  EEPROM.put(ADDRESS_PARITY, holdingData.parity);
}

void readStopBits(DeviceData *deviceData) {
  EEPROM.get(ADDRESS_STOP_BITS, holdingData.stopBits);
  if (holdingData.stopBits == 0 || holdingData.stopBits > 2) {
    holdingData.stopBits = 2;
  }
  deviceData->holdingData.stopBits = holdingData.stopBits;
}

void storeStopBits(DeviceData *deviceData) {
  holdingData.stopBits = deviceData->holdingData.stopBits;
  EEPROM.put(ADDRESS_STOP_BITS, holdingData.stopBits);
}

void readModbusAddr(DeviceData *deviceData) {
  EEPROM.get(ADDRESS_MODBUS_ADDRESS, holdingData.modbusAddr);
  if (holdingData.modbusAddr == 0 || holdingData.modbusAddr > 255) {
    holdingData.modbusAddr = 0x77;
  }
  deviceData->holdingData.modbusAddr = holdingData.modbusAddr;
}

void storeModbusAddr(DeviceData *deviceData) {
  holdingData.modbusAddr = deviceData->holdingData.modbusAddr;
  EEPROM.put(ADDRESS_MODBUS_ADDRESS, holdingData.modbusAddr);
}

void readPowerMeterAddr(DeviceData *deviceData) {
  EEPROM.get(ADDRESS_POWERMETER_ADDRESS, holdingData.powerMeterAddress);
  deviceData->holdingData.powerMeterAddress = holdingData.powerMeterAddress;
}

void storePowerMeterAddr(DeviceData *deviceData) {
  holdingData.powerMeterAddress = deviceData->holdingData.powerMeterAddress;
  EEPROM.put(ADDRESS_POWERMETER_ADDRESS, holdingData.powerMeterAddress);
}

void readPollingPeriod(DeviceData *deviceData) {
  EEPROM.get(ADDRESS_POLLING_PERIOD, holdingData.pollingSettings.pollingPeriod);
  if (holdingData.pollingSettings.pollingPeriod == 0 || holdingData.pollingSettings.pollingPeriod == 65535) {
    holdingData.pollingSettings.pollingPeriod = 600;
  }
  deviceData->holdingData.pollingSettings.pollingPeriod = holdingData.pollingSettings.pollingPeriod;
}

void storePollingPeriod(DeviceData *deviceData) {
  holdingData.pollingSettings.pollingPeriod = deviceData->holdingData.pollingSettings.pollingPeriod;
  EEPROM.put(ADDRESS_POLLING_PERIOD, holdingData.pollingSettings.pollingPeriod);
}

void readOnceADay(DeviceData *deviceData) {
  EEPROM.get(ADDRESS_ONCE_A_DAY, holdingData.pollingSettings.onceADay);
  if (holdingData.pollingSettings.onceADay == 65535) {
    holdingData.pollingSettings.onceADay = 0;
  }
  deviceData->holdingData.pollingSettings.onceADay = holdingData.pollingSettings.onceADay;
}

void storeOnceADay(DeviceData *deviceData) {
  holdingData.pollingSettings.onceADay = deviceData->holdingData.pollingSettings.onceADay;
  EEPROM.put(ADDRESS_ONCE_A_DAY, holdingData.pollingSettings.onceADay);
}

void readHourPollingADay(DeviceData *deviceData) {
  EEPROM.get(ADDRESS_HOUR_POLLING_A_DAY, holdingData.pollingSettings.hourPollingADay);
  if (holdingData.pollingSettings.hourPollingADay == 0 || holdingData.pollingSettings.hourPollingADay > 23) {
    holdingData.pollingSettings.hourPollingADay = 6;
  }
  deviceData->holdingData.pollingSettings.hourPollingADay = holdingData.pollingSettings.hourPollingADay;
}

void storeHourPollingADay(DeviceData *deviceData) {
  holdingData.pollingSettings.hourPollingADay = deviceData->holdingData.pollingSettings.hourPollingADay;
  EEPROM.put(ADDRESS_HOUR_POLLING_A_DAY, holdingData.pollingSettings.hourPollingADay);
}

void readMinutesPollingADay(DeviceData *deviceData) {
  EEPROM.get(ADDRESS_MINUTES_POLLING_A_DAY, holdingData.pollingSettings.minutesPollingADay);
  if (holdingData.pollingSettings.minutesPollingADay > 59) {
    holdingData.pollingSettings.minutesPollingADay = 0;
  }
  deviceData->holdingData.pollingSettings.minutesPollingADay = holdingData.pollingSettings.minutesPollingADay;
}

void storeMinutesPollingADay(DeviceData *deviceData) {
  holdingData.pollingSettings.minutesPollingADay = deviceData->holdingData.pollingSettings.minutesPollingADay;
  EEPROM.put(ADDRESS_MINUTES_POLLING_A_DAY, holdingData.pollingSettings.minutesPollingADay);
}

void beginModbusSlave() {
  switch (holdingData.parity) {
    case 1:
      if (holdingData.stopBits == 1) {
        Serial.begin(long(holdingData.baudRate) * 100, SERIAL_8O1);
      } else {
        Serial.begin(long(holdingData.baudRate) * 100, SERIAL_8O2);
      }
      break;
    case 2:
      if (holdingData.stopBits == 1) {
        Serial.begin(long(holdingData.baudRate) * 100, SERIAL_8E1);
      } else {
        Serial.begin(long(holdingData.baudRate) * 100, SERIAL_8E2);
      }
      break;
    default:
      if (holdingData.stopBits == 1) {
        Serial.begin(long(holdingData.baudRate) * 100, SERIAL_8N1);
      } else {
        Serial.begin(long(holdingData.baudRate) * 100, SERIAL_8N2);
      }
  }
  mb.begin(&Serial, RXTX_PIN, true);
  mb.setBaudrate(long(holdingData.baudRate) * 100);
  mb.slave(holdingData.modbusAddr);
}

void checkPowerMeterAddress(DeviceData *deviceData)
{
  if(deviceData->holdingData.powerMeterAddress) {
    deviceData->Error = 1;
  }
  else {
    deviceData->Error = 0;
  }
}

void powerMeterSetup(DeviceData *deviceData)
{
  readBaudRate(deviceData);
  readParity(deviceData);
  readStopBits(deviceData);
  readModbusAddr(deviceData);
  readPowerMeterAddr(deviceData);
  readPollingPeriod(deviceData);
  readOnceADay(deviceData);
  readHourPollingADay(deviceData);
  readMinutesPollingADay(deviceData);

  beginModbusSlave();

  Serial.print(F("baudRate:"));
  Serial.println(deviceData->holdingData.baudRate);
  Serial.print(F("parity:"));
  Serial.println(deviceData->holdingData.parity);
  Serial.print(F("stopBits:"));
  Serial.println(deviceData->holdingData.stopBits);
  Serial.print(F("modbusAddr:"));
  Serial.println(deviceData->holdingData.modbusAddr);
  Serial.print(F("powerMeterAddress:"));
  Serial.println(deviceData->holdingData.powerMeterAddress);
  Serial.print(F("pollingPeriod:"));
  Serial.println(deviceData->holdingData.pollingSettings.pollingPeriod);
  Serial.print(F("onceADay:"));
  Serial.println(deviceData->holdingData.pollingSettings.onceADay);
  Serial.print(F("hourPollingADay:"));
  Serial.println(deviceData->holdingData.pollingSettings.hourPollingADay);
  Serial.print(F("minutesPollingADay:"));
  Serial.println(deviceData->holdingData.pollingSettings.minutesPollingADay);

// input регистры

  mb.addIreg(0x0020, deviceData->u1);
  mb.addIreg(0x0021, deviceData->i1h);
  mb.addIreg(0x0022, deviceData->i1l);
  mb.addIreg(0x0023, deviceData->u2);
  mb.addIreg(0x0024, deviceData->i2h);
  mb.addIreg(0x0025, deviceData->i2l);
  mb.addIreg(0x0026, deviceData->u3);
  mb.addIreg(0x0027, deviceData->i3h);
  mb.addIreg(0x0028, deviceData->i3l);
  mb.addIreg(0x0029, deviceData->f);

  mb.addIreg(0x002a, deviceData->serialNumber0);
  mb.addIreg(0x002b, deviceData->serialNumber1);
  mb.addIreg(0x002c, deviceData->serialNumber2);
  mb.addIreg(0x002d, deviceData->serialNumber3);
  mb.addIreg(0x002e, deviceData->serialNumber4);
  mb.addIreg(0x002f, deviceData->serialNumber5);
  mb.addIreg(0x0030, deviceData->serialNumber6);
  mb.addIreg(0x0031, deviceData->serialNumber7);
  mb.addIreg(0x0032, deviceData->serialNumber8);
  mb.addIreg(0x0033, deviceData->serialNumber9);
  mb.addIreg(0x0034, deviceData->serialNumber10);
  mb.addIreg(0x0035, deviceData->serialNumber11);
  mb.addIreg(0x0036, deviceData->serialNumber12);
  mb.addIreg(0x0037, 0);

  mb.addIreg(0x0038, deviceData->year);
  mb.addIreg(0x0039, deviceData->month);
  mb.addIreg(0x003a, deviceData->day);
  mb.addIreg(0x003b, deviceData->hours);
  mb.addIreg(0x003c, deviceData->minutes);
  mb.addIreg(0x003d, deviceData->seconds);

  mb.addIreg(0x003e, deviceData->energyh);
  mb.addIreg(0x003f, deviceData->energyl);
  mb.addIreg(0x0040, deviceData->energyT1h);
  mb.addIreg(0x0041, deviceData->energyT1l);
  mb.addIreg(0x0042, deviceData->energyT2h);
  mb.addIreg(0x0043, deviceData->energyT2l);
  mb.addIreg(0x0044, deviceData->energyT3h);
  mb.addIreg(0x0045, deviceData->energyT3l);

  mb.addIreg(0x0046, deviceData->c);
  mb.addIreg(0x0047, deviceData->Error);
  
  mb.addIreg(0x0048, 0);
  mb.addIreg(0x0049, 0);
  mb.addIreg(0x004a, 0);
  mb.addIreg(0x004b, 0);

  mb.addIreg(0x004c, deviceData->dateTime0);
  mb.addIreg(0x004d, deviceData->dateTime1);
  mb.addIreg(0x004e, deviceData->dateTime2);
  mb.addIreg(0x004f, deviceData->dateTime3);
  mb.addIreg(0x0050, deviceData->dateTime4);
  mb.addIreg(0x0051, deviceData->dateTime5);
  mb.addIreg(0x0052, deviceData->dateTime6);
  mb.addIreg(0x0053, deviceData->dateTime7);
  mb.addIreg(0x0054, deviceData->dateTime8);
  mb.addIreg(0x0055, deviceData->dateTime9);
  mb.addIreg(0x0056, deviceData->dateTime10);
  mb.addIreg(0x0057, deviceData->dateTime11);
  mb.addIreg(0x0058, deviceData->dateTime12);
  mb.addIreg(0x0059, deviceData->dateTime13);
  mb.addIreg(0x005a, deviceData->dateTime14);
  mb.addIreg(0x005b, deviceData->dateTime15);
  mb.addIreg(0x005c, deviceData->dateTime16);
  mb.addIreg(0x005d, deviceData->dateTime17);
  mb.addIreg(0x005e, deviceData->dateTime18);
  mb.addIreg(0x005f, 0);
  
  mb.addIreg(0x0060, deviceData->secondsUntilIterationh);
  mb.addIreg(0x0061, deviceData->secondsUntilIterationl);
  
  mb.addIreg(0x0062, 0);
  mb.addIreg(0x0063, 0);
  mb.addIreg(0x0064, 0);
  mb.addIreg(0x0065, 0);
  mb.addIreg(0x0066, 0);
  mb.addIreg(0x0067, 0);
  
  // WirenBoard регистры

  mb.addIreg(0x0068, deviceData->secondsWorkh);
  mb.addIreg(0x0069, deviceData->secondsWorkl);
  mb.addIreg(0x006a, 0);
  mb.addIreg(0x006b, 0);
  mb.addIreg(0x006c, 0);
  mb.addIreg(0x006d, 0);
  mb.addIreg(0x006e, 0);
  mb.addIreg(0x006f, 0);
  mb.addIreg(0x0070, 0);
  mb.addIreg(0x0071, 0);
  mb.addIreg(0x0072, 0);
  mb.addIreg(0x0073, 0);
  mb.addIreg(0x0074, 0);
  mb.addIreg(0x0075, 0);
  mb.addIreg(0x0076, 0);
  mb.addIreg(0x0077, 0);
  mb.addIreg(0x0078, 0);
  mb.addIreg(0x0079, deviceData->vcc);
  mb.addIreg(0x007c, deviceData->uptime);
  mb.addIreg(0x007d, 0);
  mb.addIreg(0x007e, 0);
  mb.addIreg(0x007f, 0);
  mb.addIreg(0x0080, 0);

// holding регистры

  mb.addHreg(0x0047, deviceData->holdingData.powerMeterAddress);
  mb.addHreg(0x0048, deviceData->holdingData.pollingSettings.pollingPeriod);
  mb.addHreg(0x0049, deviceData->holdingData.pollingSettings.onceADay);
  mb.addHreg(0x004a, deviceData->holdingData.pollingSettings.hourPollingADay);
  mb.addHreg(0x004b, deviceData->holdingData.pollingSettings.minutesPollingADay);

  // WirenBoard регистры
  
  mb.addHreg(0x006e, deviceData->holdingData.baudRate);
  mb.addHreg(0x006f, deviceData->holdingData.parity);
  mb.addHreg(0x0070, holdingData.stopBits);
  mb.addHreg(0x0080, deviceData->holdingData.modbusAddr);

  checkPowerMeterAddress(deviceData);
}

// dd.MM.yyyy hh:mm:ss
// - 19 символов
void powerMeterLoop(DeviceData *deviceData)
{
  // Расчет периода работы в секундах
  uint32_t secondsWork = uint32_t(millis() / 1000ul);
  deviceData->secondsWorkh = uint16_t((secondsWork & 0xffff0000) >> 16);
  deviceData->secondsWorkl = uint16_t(secondsWork & 0x0000ffff);

  mb.Ireg(0x0020, deviceData->u1);
  mb.Ireg(0x0021, deviceData->i1h);
  mb.Ireg(0x0022, deviceData->i1l);
  mb.Ireg(0x0023, deviceData->u2);
  mb.Ireg(0x0024, deviceData->i2h);
  mb.Ireg(0x0025, deviceData->i2l);
  mb.Ireg(0x0026, deviceData->u3);
  mb.Ireg(0x0027, deviceData->i3h);
  mb.Ireg(0x0028, deviceData->i3l);
  mb.Ireg(0x0029, deviceData->f);

  mb.Ireg(0x002a, deviceData->serialNumber0);
  mb.Ireg(0x002b, deviceData->serialNumber1);
  mb.Ireg(0x002c, deviceData->serialNumber2);
  mb.Ireg(0x002d, deviceData->serialNumber3);
  mb.Ireg(0x002e, deviceData->serialNumber4);
  mb.Ireg(0x002f, deviceData->serialNumber5);
  mb.Ireg(0x0030, deviceData->serialNumber6);
  mb.Ireg(0x0031, deviceData->serialNumber7);
  mb.Ireg(0x0032, deviceData->serialNumber8);
  mb.Ireg(0x0033, deviceData->serialNumber9);
  mb.Ireg(0x0034, deviceData->serialNumber10);
  mb.Ireg(0x0035, deviceData->serialNumber11);
  mb.Ireg(0x0036, deviceData->serialNumber12);

  mb.Ireg(0x004c, deviceData->dateTime0);
  mb.Ireg(0x004d, deviceData->dateTime1);
  mb.Ireg(0x004e, deviceData->dateTime2);
  mb.Ireg(0x004f, deviceData->dateTime3);
  mb.Ireg(0x0050, deviceData->dateTime4);
  mb.Ireg(0x0051, deviceData->dateTime5);
  mb.Ireg(0x0052, deviceData->dateTime6);
  mb.Ireg(0x0053, deviceData->dateTime7);
  mb.Ireg(0x0054, deviceData->dateTime8);
  mb.Ireg(0x0055, deviceData->dateTime9);
  mb.Ireg(0x0056, deviceData->dateTime10);
  mb.Ireg(0x0057, deviceData->dateTime11);
  mb.Ireg(0x0058, deviceData->dateTime12);
  mb.Ireg(0x0059, deviceData->dateTime13);
  mb.Ireg(0x005a, deviceData->dateTime14);
  mb.Ireg(0x005b, deviceData->dateTime15);
  mb.Ireg(0x005c, deviceData->dateTime16);
  mb.Ireg(0x005d, deviceData->dateTime17);
  mb.Ireg(0x005e, deviceData->dateTime18);

  mb.Ireg(0x0060, deviceData->secondsUntilIterationh);
  mb.Ireg(0x0061, deviceData->secondsUntilIterationl);

  mb.Ireg(0x0038, deviceData->year);
  mb.Ireg(0x0039, deviceData->month);
  mb.Ireg(0x003a, deviceData->day);
  mb.Ireg(0x003b, deviceData->hours);
  mb.Ireg(0x003c, deviceData->minutes);
  mb.Ireg(0x003d, deviceData->seconds);

  mb.Ireg(0x003e, deviceData->energyh);
  mb.Ireg(0x003f, deviceData->energyl);
  mb.Ireg(0x0040, deviceData->energyT1h);
  mb.Ireg(0x0041, deviceData->energyT1l);
  mb.Ireg(0x0042, deviceData->energyT2h);
  mb.Ireg(0x0043, deviceData->energyT2l);
  mb.Ireg(0x0044, deviceData->energyT3h);
  mb.Ireg(0x0045, deviceData->energyT3l);

  mb.Ireg(0x0046, deviceData->c);
  mb.Ireg(0x0047, deviceData->Error);

  deviceData->holdingData.powerMeterAddress                   = mb.Hreg(0x0047);
  deviceData->holdingData.pollingSettings.pollingPeriod       = mb.Hreg(0x0048);
  deviceData->holdingData.pollingSettings.onceADay            = mb.Hreg(0x0049);
  deviceData->holdingData.pollingSettings.hourPollingADay     = mb.Hreg(0x004a);
  deviceData->holdingData.pollingSettings.minutesPollingADay  = mb.Hreg(0x004b);

  // WirenBoard регистры

  mb.Ireg(0x0068, deviceData->secondsWorkh);
  mb.Ireg(0x0069, deviceData->secondsWorkl);

  deviceData->holdingData.baudRate  = mb.Hreg(0x006e);
  deviceData->holdingData.parity    = mb.Hreg(0x006f);
  deviceData->holdingData.stopBits  = mb.Hreg(0x0070);

  // Измерение температуры и напряжения
  if (!tmrpw.active() || tmrpw.tick())
  {
    if (needCalcTemp)
    {
      if (!CalcTempStarted)
      {
        ADMUX = (_BV(REFS1) | _BV(REFS0) | _BV(MUX3));
        ADCSRA |= _BV(ADEN); // enable the ADC
        tmrpw.setTime(20);
        CalcTempStarted = true;
        needStartCalcTemp = true;
      }
      else
      {
        if (needStartCalcTemp)
        {
          ADCSRA |= _BV(ADSC);
          needStartCalcTemp = false;
        }
        else if (!bit_is_set(ADCSRA, ADSC))
        {
          mb.Ireg(0x007c, (uint16_t) ((ADCW - 324.31 ) / 1.22 * 10));
          CalcTempStarted = false;
          needCalcTemp = false;
          tmrpw.setTime(500);
        }
      }
    }
    else
    {
      deviceData->vcc = analogRead_VCC() * 1000;
      mb.Ireg(0x0079, deviceData->vcc);
      needCalcTemp = true;
      tmrpw.setTime(500);
    }
    tmrpw.start();
  }

  deviceData->holdingData.modbusAddr = mb.Hreg(0x0080);

  //  deviceData->pollingPeriod = 300;
  //  deviceData->onceADay = 0;
  //  deviceData->baudRate = 1152;
  //  deviceData->holdingData.stopBits = 1;

  bool needRestart = false;
  if (deviceData->holdingData.baudRate != holdingData.baudRate) {
    storeBaudRate(deviceData);
    needRestart = true;
  }
  if (deviceData->holdingData.parity != holdingData.parity) {
    storeParity(deviceData);
    needRestart = true;
  }
  if (deviceData->holdingData.stopBits != holdingData.stopBits) {
    storeStopBits(deviceData);
    needRestart = true;
  }
  if (deviceData->holdingData.modbusAddr != holdingData.modbusAddr) {
    storeModbusAddr(deviceData);
    needRestart = true;
  }
  if (deviceData->holdingData.powerMeterAddress != holdingData.powerMeterAddress) {
    storePowerMeterAddr(deviceData);
    checkPowerMeterAddress(deviceData);
  }
  if (deviceData->holdingData.pollingSettings.pollingPeriod != holdingData.pollingSettings.pollingPeriod) {
    storePollingPeriod(deviceData);
  }
  if (deviceData->holdingData.pollingSettings.onceADay != holdingData.pollingSettings.onceADay) {
    storeOnceADay(deviceData);
  }
  if (deviceData->holdingData.pollingSettings.hourPollingADay != holdingData.pollingSettings.hourPollingADay) {
    storeHourPollingADay(deviceData);
  }
  if (deviceData->holdingData.pollingSettings.minutesPollingADay != holdingData.pollingSettings.minutesPollingADay) {
    storeMinutesPollingADay(deviceData);
  }
  if (needRestart) {
    beginModbusSlave();
  }

  mb.task();
  yield();
}
