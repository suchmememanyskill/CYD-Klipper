#ifdef TOUCH_CST820

#include "CST820.h"

CST820::CST820(int8_t sda_pin, int8_t scl_pin, int8_t rst_pin, int8_t int_pin) {
  _sda = sda_pin;
  _scl = scl_pin;
  _rst = rst_pin;
  _int = int_pin;
}

void CST820::begin(void) {
  if (_sda != -1 && _scl != -1) {
    Wire.begin(_sda, _scl);
  } else {
    Wire.begin();
  }

  if (_int != -1) {
    pinMode(_int, OUTPUT);
    digitalWrite(_int, HIGH);
    delay(1);
    digitalWrite(_int, LOW);
    delay(1);
  }

  if (_rst != -1) {
    pinMode(_rst, OUTPUT);
    digitalWrite(_rst, LOW);
    delay(10);
    digitalWrite(_rst, HIGH);
    delay(300);
  }

  // Final init
  i2c_write(0xFE, 0XFF);
}

bool CST820::getTouch(uint16_t *x, uint16_t *y, uint8_t *gesture) {
  bool finger = false;
  finger = (bool)i2c_read(0x02);

  *gesture = i2c_read(0x01);
  if (!(*gesture == SlideUp || *gesture == SlideDown)) {
    *gesture = None;
  }

  uint8_t data[4];
  i2c_read_continuous(0x03, data, 4);
  *x = ((data[0] & 0x0f) << 8) | data[1];
  *y = ((data[2] & 0x0f) << 8) | data[3];

  return finger;
}

uint8_t CST820::i2c_read(uint8_t addr) {
  uint8_t rdData;
  uint8_t rdDataCount;
  do {
    Wire.beginTransmission(I2C_ADDR_CST820);
    Wire.write(addr);
    Wire.endTransmission(false); // Restart
    rdDataCount = Wire.requestFrom(I2C_ADDR_CST820, 1);
  } while (rdDataCount == 0);
  while (Wire.available()) {
    rdData = Wire.read();
  }
  return rdData;
}

uint8_t CST820::i2c_read_continuous(uint8_t addr, uint8_t *data,
                                    uint32_t length) {
  Wire.beginTransmission(I2C_ADDR_CST820);
  Wire.write(addr);
  if (Wire.endTransmission(true))
    return -1;
  Wire.requestFrom(I2C_ADDR_CST820, length);
  for (int i = 0; i < length; i++) {
    *data++ = Wire.read();
  }
  return 0;
}

void CST820::i2c_write(uint8_t addr, uint8_t data) {
  Wire.beginTransmission(I2C_ADDR_CST820);
  Wire.write(addr);
  Wire.write(data);
  Wire.endTransmission();
}

uint8_t CST820::i2c_write_continuous(uint8_t addr, const uint8_t *data,
                                     uint32_t length) {
  Wire.beginTransmission(I2C_ADDR_CST820);
  Wire.write(addr);
  for (int i = 0; i < length; i++) {
    Wire.write(*data++);
  }
  if (Wire.endTransmission(true))
    return -1;
  return 0;
}

#endif