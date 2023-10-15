//#define ShowReceivedPacked
//#define ShowResultedPacked
//#define ShowGettingIformation

#include <ELECHOUSE_CC1101_SRC_DRV.h>
#include "CRC8.h"
#include <TimerMs.h>
#include <avr/pgmspace.h>

PollingSettings pollingSettings;

bool pollingStarted;
uint8_t indexPacket;
uint8_t iteration;
byte packagReceived;

CRC8 crc;
byte resultbuffer[61] = { 0 }; //буфер конечного, сшитого принятого пакета
byte bytecount = 0; //указатель байтов в результирующем буфере
TimerMs tmr(1000, 0, 1); //инициализируем таймер

// Идентификаторы пакетов
const byte numberPackets[] PROGMEM = {0x1c, 0x05, 0x2B, 0x0a, 0x1c};

//Настройки для CC1101 с форума (47 бит)
const byte rfSettings[] PROGMEM = {
  0x0D,  // IOCFG2              GDO2 Output Pin Configuration
  0x2E,  // IOCFG1              GDO1 Output Pin Configuration
  0x06,  // IOCFG0              GDO0 Output Pin Configuration
  0x4F,  // FIFOTHR             RX FIFO and TX FIFO Thresholds
  0xD3,  // SYNC1               Sync Word, High Byte
  0x91,  // SYNC0               Sync Word, Low Byte
  0x3C,  // PKTLEN              Packet Length
  0x00,  // PKTCTRL1            Packet Automation Control
  0x41,  // PKTCTRL0            Packet Automation Control
  0x00,  // ADDR                Device Address
  0x16,  // CHANNR              Channel Number (0x0B)
  0x0F,  // FSCTRL1             Frequency Synthesizer Control
  0x00,  // FSCTRL0             Frequency Synthesizer Control
  0x10,  // FREQ2               Frequency Control Word, High Byte
  0x8B,  // FREQ1               Frequency Control Word, Middle Byte
  0x54,  // FREQ0               Frequency Control Word, Low Byte
  0xD9,  // MDMCFG4             Modem Configuration
  0x83,  // MDMCFG3             Modem Configuration
  0x13,  // MDMCFG2             Modem Configuration
  0xD2,  // MDMCFG1             Modem Configuration
  0xAA,  // MDMCFG0             Modem Configuration
  0x31,  // DEVIATN             Modem Deviation Setting
  0x07,  // MCSM2               Main Radio Control State Machine Configuration
  0x0C,  // MCSM1               Main Radio Control State Machine Configuration
  0x08,  // MCSM0               Main Radio Control State Machine Configuration
  0x16,  // FOCCFG              Frequency Offset Compensation Configuration
  0x6C,  // BSCFG               Bit Synchronization Configuration
  0x03,  // AGCCTRL2            AGC Control
  0x40,  // AGCCTRL1            AGC Control
  0x91,  // AGCCTRL0            AGC Control
  0x87,  // WOREVT1             High Byte Event0 Timeout
  0x6B,  // WOREVT0             Low Byte Event0 Timeout
  0xF8,  // WORCTRL             Wake On Radio Control
  0x56,  // FREND1              Front End RX Configuration
  0x10,  // FREND0              Front End TX Configuration
  0xE9,  // FSCAL3              Frequency Synthesizer Calibration
  0x2A,  // FSCAL2              Frequency Synthesizer Calibration
  0x00,  // FSCAL1              Frequency Synthesizer Calibration
  0x1F,  // FSCAL0              Frequency Synthesizer Calibration
  0x41,  // RCCTRL1             RC Oscillator Configuration
  0x00,  // RCCTRL0             RC Oscillator Configuration
  0x59,  // FSTEST              Frequency Synthesizer Calibration Control
  0x59,  // PTEST               Production Test
  0x3F,  // AGCTEST             AGC Test
  0x81,  // TEST2               Various Test Settings
  0x35,  // TEST1               Various Test Settings
  0x09  // TEST0               Various Test Settings (0x0B)
};

byte transmitt_byte[] =  //массив отправляемого пакета
{
  0x10, //длина пакета 16 байт
  0x73,
  0x55, //начало payload
  0x00, //тип запроса
  0x00, //
  0x00, // (atoi(MeterAdressValue)) & 0xff; //младший байт адреса счётчика
  0x00, // ((atoi(MeterAdressValue)) >> 8) & 0xff; //старший байт адреса счётчика
  0x09, //
  0xff, //
  0x00, //
  0x00, //PIN
  0x00, //PIN
  0x00, //PIN
  0x00, //PIN
  0x00, // байт CRC
  0x55, //конец payload
  0x55, //конец payload
};

void SerialPrint2Dec(int Number) {
  if (Number < 10) {
    Serial.print(F("0"));
  }
  Serial.print(Number, DEC);
}

void printThePackage(byte *package, uint8_t lenght) {
  for (int index = 0; index < lenght - 1; index++) {
    if (package[index] < 0x10) {
      Serial.print(F("0"));
    }
    Serial.print(package[index], HEX);
    Serial.print(F(" "));
  }
}

bool Receive() {
  if (ELECHOUSE_cc1101.CheckReceiveFlag()) {
    //Get received Data and calculate length
    byte buffer[61] = { 0 }; //буффер пакетов, принятых из трансивера (или отправленных в трансивер)
    byte len = ELECHOUSE_cc1101.ReceiveData(buffer);
    //подшиваем 1/3 пакета в общий пакет
    if (buffer[1] == 0x73 && buffer[2] == 0x55 || bytecount + len - 1 > 61) {
      bytecount = 0;
    }
    for (int i = 1; i < len; i++) {
      resultbuffer[bytecount] = buffer[i];
      bytecount++;
    }
#ifdef ShowReceivedPacked
    printThePackage(buffer, len);
    Serial.println();
#endif
    ELECHOUSE_cc1101.SpiStrobe(0x36);  // Exit RX / TX, turn off frequency synthesizer and exit
    ELECHOUSE_cc1101.SpiStrobe(0x3A);  // Flush the RX FIFO buffer
    ELECHOUSE_cc1101.SpiStrobe(0x3B);  // Flush the TX FIFO buffer
    ELECHOUSE_cc1101.SpiStrobe(0x34);  // Enable RX
    return true;
  }
  return false;
}

bool CheckPacket(DeviceData *deviceData) {
  if (bytecount == 0) {
    return true;
  }
  crc.restart();
  crc.add(resultbuffer + 2, bytecount - 4);
  byte myCRC = crc.getCRC();
#ifdef ShowResultedPacked
  printThePackage(resultbuffer, bytecount);
  Serial.print(F("(L:"));
  Serial.print(bytecount, HEX);
  if (myCRC != resultbuffer[bytecount - 2]) {
    Serial.print(F("; !crc: "));
  }
  else {
    Serial.print(F("; CRC: "));
  }
  Serial.print(myCRC, HEX);
  Serial.print(F(")"));
#endif
  if ((resultbuffer[0] == 0x73)
      && (resultbuffer[1] == 0x55)
      && (resultbuffer[4] == (deviceData->holdingData.powerMeterAddress & 0xff))
      && (resultbuffer[5] == ((deviceData->holdingData.powerMeterAddress >> 8) & 0xff))) {
#ifdef ShowResultedPacked
    Serial.print(F(" ->"));
#endif
  } else if ((resultbuffer[0] == 0x73)
             && (resultbuffer[1] == 0x55)
             && (resultbuffer[6] == (deviceData->holdingData.powerMeterAddress & 0xff))
             && (resultbuffer[7] == ((deviceData->holdingData.powerMeterAddress >> 8) & 0xff))) {
#ifdef ShowResultedPacked
    Serial.print(F(" <-"));
#endif
  } else {
#ifdef ShowResultedPacked
    Serial.println(F(" Bad"));
#endif
    return false;
  }
#ifdef ShowResultedPacked
  Serial.println();
#endif
  if (myCRC != resultbuffer[bytecount - 2]) {
    return false;
  }
  return true;
}

void ParcePacket(DeviceData *deviceData) {
  uint64_t timeSt = 0;
  switch (resultbuffer[8]) {
    case 0x1c:
      deviceData->day     = resultbuffer[17];
      deviceData->month   = resultbuffer[18];
      deviceData->year    = resultbuffer[19];
      deviceData->hours   = resultbuffer[15];
      deviceData->minutes = resultbuffer[14];
      deviceData->seconds = resultbuffer[13];
      deviceData->dateTime0 = '2';
      deviceData->dateTime1 = '0';
      deviceData->dateTime2 = resultbuffer[19] / 10 + 48;
      deviceData->dateTime3 = resultbuffer[19] - (deviceData->dateTime2 - 48) * 10 + 48;
      deviceData->dateTime4 = '-';
      deviceData->dateTime5 = resultbuffer[18] / 10 + 48;
      deviceData->dateTime6 = resultbuffer[18] - (deviceData->dateTime5 - 48) * 10 + 48;
      deviceData->dateTime7 = '-';
      deviceData->dateTime8 = resultbuffer[17] / 10 + 48;
      deviceData->dateTime9 = resultbuffer[17] - (deviceData->dateTime8 - 48) * 10 + 48;
      deviceData->dateTime10 = ' ';
      deviceData->dateTime11 = resultbuffer[15] / 10 + 48;
      deviceData->dateTime12 = resultbuffer[15] - (deviceData->dateTime11 - 48) * 10 + 48;
      deviceData->dateTime13 = ':';
      deviceData->dateTime14 = resultbuffer[14] / 10 + 48;
      deviceData->dateTime15 = resultbuffer[14] - (deviceData->dateTime14 - 48) * 10 + 48;
      deviceData->dateTime16 = ':';
      deviceData->dateTime17 = resultbuffer[13] / 10 + 48;
      deviceData->dateTime18 = resultbuffer[13] - (deviceData->dateTime17 - 48) * 10 + 48;
#ifdef ShowGettingIformation
      SerialPrint2Dec(deviceData->day);
      Serial.print(F("."));
      SerialPrint2Dec(deviceData->month);
      Serial.print(F("."));
      Serial.print(deviceData->year + 2000);
      Serial.print(F(" "));
      SerialPrint2Dec(deviceData->hours);
      Serial.print(F(":"));
      SerialPrint2Dec(deviceData->minutes);
      Serial.print(F(":"));
      SerialPrint2Dec(deviceData->seconds);
      Serial.println();
#endif
      break;
    case 0x05:
      deviceData->energyl =   (uint16_t(resultbuffer[24]) << 8) | uint16_t(resultbuffer[23]);
      deviceData->energyh =   (uint16_t(resultbuffer[25]));
      deviceData->energyT1l = (uint16_t(resultbuffer[28]) << 8) | uint16_t(resultbuffer[27]);
      deviceData->energyT1h = (uint16_t(resultbuffer[29]));
      deviceData->energyT2l = (uint16_t(resultbuffer[32]) << 8) | uint16_t(resultbuffer[31]);
      deviceData->energyT2h = (uint16_t(resultbuffer[33]));
      deviceData->energyT3l = (uint16_t(resultbuffer[36]) << 8) | uint16_t(resultbuffer[35]);
      deviceData->energyT3h = (uint16_t(resultbuffer[37]));
#ifdef ShowGettingIformation
      Serial.print(F("SUM:"));
      Serial.println(float((uint32_t(deviceData->energyh) << 16) | deviceData->energyl) / 100);
      Serial.print(F("T1:"));
      Serial.println(float((uint32_t(deviceData->energyT1h) << 16) | deviceData->energyT1l) / 100);
      Serial.print(F("T2:"));
      Serial.println(float((uint32_t(deviceData->energyT2h) << 16) | deviceData->energyT2l) / 100);
#endif
      break;
    case 0x2b:
      deviceData->f =   (uint16_t(resultbuffer[25]) << 8) | uint16_t(resultbuffer[24]);
      deviceData->c =   (uint16_t(resultbuffer[27]) << 8) | uint16_t(resultbuffer[26]);
      deviceData->u1 =  (uint16_t(resultbuffer[29]) << 8) | uint16_t(resultbuffer[28]);
      deviceData->u2 =  (uint16_t(resultbuffer[31]) << 8) | uint16_t(resultbuffer[30]);
      deviceData->u3 =  (uint16_t(resultbuffer[33]) << 8) | uint16_t(resultbuffer[32]);
      deviceData->i1l = (uint16_t(resultbuffer[35]) << 8) | uint16_t(resultbuffer[34]);
      deviceData->i1h = uint16_t(resultbuffer[36]);
      deviceData->i2l = (uint16_t(resultbuffer[38]) << 8) | uint16_t(resultbuffer[37]);
      deviceData->i1h = uint16_t(resultbuffer[39]);
      deviceData->i3l = (uint16_t(resultbuffer[41]) << 8) | uint16_t(resultbuffer[40]);
      deviceData->i1h = uint16_t(resultbuffer[42]);
#ifdef ShowGettingIformation
      Serial.print(F("Freq:"));
      Serial.println(float(deviceData->f) / 100);
      Serial.print(F("Cos:"));
      Serial.println(float(deviceData->c) / 1000);
      Serial.print(F("U1:"));
      Serial.println(float(deviceData->u1) / 100);
      Serial.print(F("U2:"));
      Serial.println(float(deviceData->u2) / 100);
      Serial.print(F("U3:"));
      Serial.println(float(deviceData->u3) / 100);
      Serial.print(F("I1:"));
      Serial.println(float((uint32_t(deviceData->i1h) << 16) | deviceData->i1l) / 1000);
      Serial.print(F("I2:"));
      Serial.println(float((uint32_t(deviceData->i2h) << 16) | deviceData->i2l) / 1000);
      Serial.print(F("I3:"));
      Serial.println(float((uint32_t(deviceData->i3h) << 16) | deviceData->i3l) / 1000);
      Serial.print(F("Vcc:"));
      Serial.println(float(deviceData->vcc) / 1000);
      Serial.print(F("sec:"));
      Serial.println((uint32_t(deviceData->secondsWorkh) << 16) | deviceData->secondsWorkl);
#endif
      break;
    case 0x0a:
      deviceData->serialNumber0   = resultbuffer[14];
      deviceData->serialNumber1   = resultbuffer[15];
      deviceData->serialNumber2   = resultbuffer[16];
      deviceData->serialNumber3   = resultbuffer[17];
      deviceData->serialNumber4   = resultbuffer[18];
      deviceData->serialNumber5   = resultbuffer[19];
      deviceData->serialNumber6   = resultbuffer[20];
      deviceData->serialNumber7   = resultbuffer[21];
      deviceData->serialNumber8   = resultbuffer[22];
      deviceData->serialNumber9   = resultbuffer[23];
      deviceData->serialNumber10  = resultbuffer[24];
      deviceData->serialNumber11  = resultbuffer[25];
      deviceData->serialNumber12  = resultbuffer[26];
#ifdef ShowGettingIformation
      Serial.print(F("S/n:"));
      Serial.print(char(deviceData->serialNumber0));
      Serial.print(char(deviceData->serialNumber1));
      Serial.print(char(deviceData->serialNumber2));
      Serial.print(char(deviceData->serialNumber3));
      Serial.print(char(deviceData->serialNumber4));
      Serial.print(char(deviceData->serialNumber5));
      Serial.print(char(deviceData->serialNumber6));
      Serial.print(char(deviceData->serialNumber7));
      Serial.print(char(deviceData->serialNumber8));
      Serial.print(char(deviceData->serialNumber9));
      Serial.print(char(deviceData->serialNumber10));
      Serial.print(char(deviceData->serialNumber11));
      Serial.println(char(deviceData->serialNumber12));
#endif
      break;
  }
}

void rememberPollingSettings(DeviceData *deviceData) {
  pollingSettings.pollingPeriod      = deviceData->holdingData.pollingSettings.pollingPeriod;
  pollingSettings.onceADay           = deviceData->holdingData.pollingSettings.onceADay;
  pollingSettings.hourPollingADay    = deviceData->holdingData.pollingSettings.hourPollingADay;
  pollingSettings.minutesPollingADay = deviceData->holdingData.pollingSettings.minutesPollingADay;
}

bool isPowerMeterPacket(DeviceData *deviceData) {
  return (resultbuffer[0] == 0x73)
         && (resultbuffer[1] == 0x55)
         && (resultbuffer[6] == (deviceData->holdingData.powerMeterAddress & 0xff))
         && (resultbuffer[7] == ((deviceData->holdingData.powerMeterAddress >> 8) & 0xff));
}

void setTimerToNextPolling(DeviceData *deviceData) {
  if (deviceData->holdingData.pollingSettings.onceADay > 0) {
    uint32_t hours = 0;
    if (deviceData->holdingData.pollingSettings.hourPollingADay < deviceData->hours) {
      hours = deviceData->holdingData.pollingSettings.hourPollingADay + 24 - deviceData->hours;
    } else {
      hours = deviceData->holdingData.pollingSettings.hourPollingADay - deviceData->hours;
    }
    uint32_t minutes = 0;
    if (deviceData->holdingData.pollingSettings.hourPollingADay < deviceData->minutes) {
      minutes = deviceData->holdingData.pollingSettings.minutesPollingADay + 24 - deviceData->minutes;
    } else {
      minutes = deviceData->holdingData.pollingSettings.minutesPollingADay - deviceData->minutes;
    }
    tmr.setTime(uint32_t((uint32_t(hours * 3600) + uint32_t(minutes * 60))) * 1000); // 24 * 60 * 60 * 1000
  } else {
    tmr.setTime(uint32_t(deviceData->holdingData.pollingSettings.pollingPeriod) * 1000);
  }
  pollingStarted = false;
}

void sendPacket(uint8_t packetIndex, DeviceData *deviceData) {
  digitalWrite(LED_BUILTIN, HIGH);
  // Serial.println(packetIndex);
  // Подготовка авкета
  switch (packetIndex) {
    case 0x1c:
      // Запрос времени и даты
      transmitt_byte[0]  = 0x0f; //длина пакета 16 байт
      transmitt_byte[3]  = 0x20; //тип запроса
      transmitt_byte[9]  = 0x1c; //
      transmitt_byte[14] = 0x00;
      break;
    case 0x05:
      // Запрос показаний счетчика
      transmitt_byte[0]  = 0x10; //длина пакета 16 байт
      transmitt_byte[3]  = 0x21; //тип запроса
      transmitt_byte[9]  = 0x05; //
      transmitt_byte[14] = 0x00;
      break;
    case 0x2b:
      // Запрос значений напряжений и токов
      transmitt_byte[0]  = 0x10;
      transmitt_byte[3]  = 0x21;
      transmitt_byte[9]  = 0x2b;
      transmitt_byte[14] = 0x00;
      break;
    case 0x0a:
      // Запрос серийного номера
      transmitt_byte[0]  = 0x10;
      transmitt_byte[3]  = 0x21;
      transmitt_byte[9]  = 0x0a;
      transmitt_byte[14] = 0x01;
      break;
    default:
      // Маловероятно, но вдруг пакет не знакомый
      digitalWrite(LED_BUILTIN, LOW);
      return;
  }
  // Установка адреса
  transmitt_byte[5] = (deviceData->holdingData.powerMeterAddress) & 0xff; //младший байт адреса счётчика
  transmitt_byte[6] = ((deviceData->holdingData.powerMeterAddress) >> 8) & 0xff; //старший байт адреса счётчика
  // Расчет CRC
  crc.restart();
  crc.add(transmitt_byte + 3, transmitt_byte[0] - 4);
  transmitt_byte[transmitt_byte[0] - 1] = crc.getCRC(); //CRC
  // Установка завершающего байта
  transmitt_byte[transmitt_byte[0]] = 0x55;
  // Непосредственно отправка
  ELECHOUSE_cc1101.SpiStrobe(0x33);                                 //Calibrate frequency synthesizer and turn it off
  //delay(1);
  ELECHOUSE_cc1101.SpiStrobe(0x3B);                                 // Flush the TX FIFO buffer
  ELECHOUSE_cc1101.SpiStrobe(0x36);                                 // Exit RX / TX, turn off frequency synthesizer and exit
  ELECHOUSE_cc1101.SpiWriteReg(0x3e, 0xC4);                         //выставляем мощность 10dB
  ELECHOUSE_cc1101.SendData(transmitt_byte, transmitt_byte[0] + 1); //отправляем пакет
  ELECHOUSE_cc1101.SpiStrobe(0x3A);                                 // Flush the RX FIFO buffer
  ELECHOUSE_cc1101.SpiStrobe(0x34);                                 // Enable RX
#ifdef ShowReceivedPacked
  printThePackage(transmitt_byte, transmitt_byte[0] + 1);
  Serial.println();
#endif
  bytecount = 0;      //указатель байтов в результирующем буфере
  packagReceived = false;
  digitalWrite(LED_BUILTIN, LOW);
}

void poolingLoop(DeviceData *deviceData) {
  if (!tmr.active() || tmr.tick()) {
    if (!pollingStarted) {
      pollingStarted = true;
      indexPacket = 0;
      iteration = 0;
    } else if (packagReceived) {
      indexPacket ++;
      iteration = 0;
    } else {
      iteration++;
    }
    if (indexPacket > 3) {
      setTimerToNextPolling(deviceData);
      deviceData->Error = 0;
    } else {
      if (iteration < 10) {
        sendPacket(pgm_read_byte(numberPackets + indexPacket), deviceData);
        // Для приема пакета обычно хватает секунды, если не хватило увеличивается время
        tmr.setTime(2000 + (1000 * iteration));
      } else {
        // Если ответа получить не удается - делается минутная пауза
        tmr.setTime(180000);
        pollingStarted = false;
        deviceData->Error = 2;
      }
    }
    tmr.start();
  }
}

void receiveLoop(DeviceData *deviceData) {
  if (Receive()) {
    if (resultbuffer[bytecount - 1] == 0x55) {
      if (CheckPacket(deviceData)) {
        if (isPowerMeterPacket(deviceData)) {
          ParcePacket(deviceData);
          if (pollingStarted && resultbuffer[8] == transmitt_byte[9]) {
            packagReceived = true;
            tmr.force();
          }
        }
      }
    }
  }
}

void cc1101Setup(DeviceData * deviceData) {

  rememberPollingSettings(deviceData);
  pollingStarted = false;
  packagReceived = false;

  pinMode(LED_BUILTIN, OUTPUT);

  ELECHOUSE_cc1101.setGDO0(2);
  if (ELECHOUSE_cc1101.getCC1101()) {     // Check the CC1101 Spi connection.
    Serial.println(F("Connection OK"));
  }
  else {
    Serial.println(F("Connection Error"));
    deviceData->Error = 3;
  }

  //Инициализируем cc1101
  ELECHOUSE_cc1101.SpiStrobe(0x30);                           //reset
  for (uint8_t regNumber = 0; regNumber < 0x2F; regNumber++) {
    ELECHOUSE_cc1101.SpiWriteReg(regNumber, pgm_read_byte(rfSettings + regNumber));
  }
  ELECHOUSE_cc1101.SpiStrobe(0x33);                           //Calibrate frequency synthesizer and turn it off
  delay(1);
  ELECHOUSE_cc1101.SpiStrobe(0x3A);                           // Flush the RX FIFO buffer
  ELECHOUSE_cc1101.SpiStrobe(0x3B);                           // Flush the TX FIFO buffer
  ELECHOUSE_cc1101.SpiStrobe(0x34);                           // Enable RX

  crc.reset();
  crc.setPolynome(0xA9);
}

void cc1101Loop(DeviceData *deviceData) {
  if (deviceData->holdingData.powerMeterAddress == 0) {
    return;
  }
  
  uint32_t secondsUntilIteration = tmr.timeLeft() / 1000;
  
  deviceData->secondsUntilIterationh = uint16_t((secondsUntilIteration & 0xffff0000) >> 16);
  deviceData->secondsUntilIterationl = uint16_t(secondsUntilIteration & 0x0000ffff);
  
  poolingLoop(deviceData);
  receiveLoop(deviceData);
}
