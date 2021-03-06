#include <Wire.h>
#include <RTClib.h>
#include "CMMC_Modem.h"


IPAddress aisip = IPAddress(103, 20, 205, 85);
RTC_DATA_ATTR int rebootCount = -1;
extern HardwareSerial SERIAL0;

CMMC_Modem::CMMC_Modem(Stream* s, HardwareSerial* hwSerial, MODEM_TYPE modem_type)   {
  this->_modemSerial = s;
  this->hwSerial = hwSerial;
  this->_modemType = modem_type;
}

void CMMC_Modem::configLoop() {
  yield();
}

void CMMC_Modem::configSetup() {
  yield();
}

void CMMC_Modem::updateStatus(String s) {
  this->hwSerial->println(s);
  strcpy(this->status, s.c_str());
}

void CMMC_Modem::setup() {
  SERIAL0.println("setup modem..");
  // this->status = "Initializing Modem.";
  strcpy(this->status, "Initializing Modem.");
  // // this->_modemType = TYPE_TRUE_NB_IOT;
  //
  // if (this->_modemType == TYPE_AIS_NB_IOT) {
  //   pinMode(13, OUTPUT);
  //   digitalWrite(13, HIGH);
  //   delay(1);
  //   digitalWrite(13, LOW);
  // }
  // else if (this->_modemType == TYPE_TRUE_NB_IOT) {
  // }
  // else {
  //   this->hwSerial->printf("[type=%d] INVALID MODEM TYPE CONFIG.", this->_modemType);
  // }

    //reset Modem
      pinMode(13, OUTPUT); // LED
      pinMode(17, OUTPUT); // RESET NB
      digitalWrite(13, HIGH);
      digitalWrite(17, HIGH);
      delay(10);

      digitalWrite(13, LOW);
      digitalWrite(17, LOW);

  SERIAL0.println("Initializing CMMC NB-IoT");
  nb = new CMMC_NB_IoT(this->_modemSerial);
  static CMMC_Modem *that;
  that = this;
  nb->setDebugStream(&SERIAL0);
  nb->onDeviceReboot([]() {
    that->updateStatus(F("[user] Device rebooted."));
    delay(100);
  });

  nb->onDeviceReady([]() {
    that->hwSerial->println("[user] Device Ready!");
  });

  nb->onDeviceInfo([](CMMC_NB_IoT::DeviceInfo device) {
    SERIAL0.print(F("# Module IMEI-->  "));
    SERIAL0.println(device.imei);
    that->hwSerial->print(F("# Firmware ver-->  "));
    that->hwSerial->println(device.firmware);
    that->hwSerial->print(F("# IMSI SIM-->  "));
    that->hwSerial->println(device.imsi);
    that->IMEI = (String(device.imei));
    that->IMSI = (String(device.imsi));
    that->csq = device.csq;
    int n = that->csq;
    int8_t r;
    if (n == 0) r = -113;
    if (n == 1) r = -111;
    if (n == 31) r = -52;
    if ((n >= 2) && (n <= 30)) {
      r = map(n, 2, 30, -109, -53);
    }
    if (n > 30) {
      r = -115;
    }

    int x = map(r, -115, -53, 0, 100);
    that->rssi= r;
    that->signal = x;
  });

  nb->onMessageArrived([](char *text, size_t len, uint8_t socketId, char* ip, uint16_t port) {
    char buffer[100];
    sprintf(buffer, "++ [recv:] socketId=%u, ip=%s, port=%u, len=%d bytes (%lums)", socketId, ip, port, len, millis());
    that->updateStatus(buffer);
  });

  static int counter;
  static uint32_t prev;
  counter = 0;
  prev = millis();
  nb->onConnecting([]() {
    counter = (counter + 1);
    // String t = "Attching";

    String t = "";
    for (size_t i = 0; i <= counter%3; i++) {
      t += String(".");
    }
    that->updateStatus(t);

    vTaskDelay(500 / portTICK_PERIOD_MS);
    if (millis() - prev > (180 * 1000)) {
      ESP.deepSleep(1e6);
      vTaskDelay(500 / portTICK_PERIOD_MS);
      ESP.restart();
    }
  });

  nb->onConnected([](void * parameter ) {
    that->updateStatus("Connected.");
    that->hwSerial->print("[user] NB-IoT Network connected at (");
    that->hwSerial->print(millis());
    that->hwSerial->println("ms)");
    that->hwSerial->println("[1] createUdpSocket");
    that->nb->createUdpSocket("103.20.205.85", 5683, UDPConfig::DISABLE_RECV);
    delay(100);
    that->nb->createUdpSocket("128.199.205.93", 5683, UDPConfig::DISABLE_RECV);
    that->hwSerial->println("[2] createUdpSocket");
    delay(100);
    // that->csq = that->nb->getSignal();
    // int n = that->csq;
    // int8_t r;
    // if (n == 0) r = -113;
    // if (n == 1) r = -111;
    // if (n == 31) r = -52;
    // if ((n >= 2) && (n <= 30)) {
    //   r = map(n, 2, 30, -109, -53);
    // }
    // if (n > 30) {
    //   r = -115;
    // }
    //
    // int x = map(r, -115, -53, 0, 100);
    // that->rssi= r;
    // that->signal = x;


    that->isNbConnected = 1;
    that->_locked = false;
  });

  nb->hello();
  nb->rebootModule();
}

void CMMC_Modem::loop() {
  nb->loop();
  static CMMC_Modem *that;
  that = this;
}

// int CMMC_Modem::getSignal() {
//
// }

int CMMC_Modem::sendOverSocket(uint8_t *buffer, int buflen, int socketId) {
    if (nb->sendMessageHex(buffer, buflen, socketId)) {
      lastSentOkMillis = millis();
      updateStatus("sent.");
      delay(500);
    }
}

void CMMC_Modem::sendPacket(uint8_t *text, int buflen) {
  if (!isNbConnected) {
    this->hwSerial->println("NB IoT is not connected! skipped.");
    return;
  }

  this->_locked = true;
  this->csq = nb->getSignal();
  int n = this->csq;
  int8_t r;
  if (n == 0) r = -113;
  if (n == 1) r = -111;
  if (n == 31) r = -52;
  if ((n >= 2) && (n <= 30)) {
    r = map(n, 2, 30, -109, -53);
  }
  if (n > 30) {
    r = -115;
  }

  int x = map(r, -115, -53, 0, 100);
  this->rssi= r;
  this->signal = x;

  if (this->signal <= 0) {
    // ESP.deepSleep(10);
    digitalWrite(13, HIGH);
    digitalWrite(17, HIGH);
    delay(10);

    digitalWrite(13, LOW);
    digitalWrite(17, LOW);
  }
  // this->hwSerial->print("Signal = ");
  // this->hwSerial->println(nb->getSignal());
  // updateStatus(String("Signal = ") +  nb->getSignal());
  int rt = 0;
  uint8_t buffer[buflen];
  memcpy(buffer, text, buflen);
  updateStatus("dispatching queue...");
  sendOverSocket(buffer, buflen,  0);
  // vTaskDelay(200 / portTICK_PERIOD_MS);
  delay(500);
  sendOverSocket(buffer, buflen, 1);
  delay(500);
  this->_locked = false;
}

String CMMC_Modem::getStatus() {
  return String(this->status);
}

bool CMMC_Modem::isLocked() {
  return this->_locked;
}
