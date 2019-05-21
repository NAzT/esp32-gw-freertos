#include "xCMMC_LED.h"

xCMMC_LED xCMMC_LED::init(blink_t type)
{
  if (type == BLINK_TYPE_TICKER)
  {
    this->_ticker = new Ticker;
    this->_ticker2 = new Ticker;
  }
  _initialized = true;
  return *this;
}


xCMMC_LED::xCMMC_LED(blink_t type)
{
  _type = type;
};

xCMMC_LED::xCMMC_LED(Ticker *ticker)
{
  _initialized = true;
  this->_ticker = ticker;
};

void xCMMC_LED::setPin(uint8_t pin)
{
  _ledPin = pin;
  pinMode(_ledPin, OUTPUT);
  digitalWrite(_ledPin, LOW);
}

void xCMMC_LED::toggle()
{
  this->state = !this->state;
  digitalWrite(this->_ledPin, this->state);
}

void xCMMC_LED::blink(uint32_t ms, uint8_t pin)
{
  this->setPin(pin);
  this->blink(ms);
}

void xCMMC_LED::detach()
{
  this->_ticker->detach();
  this->_ticker2->detach();
}

void xCMMC_LED::blink(uint32_t ms)
{
  if (_initialized == false)
    return;
  if (_ledPin == 254)
    return;
  static int _pin = this->_ledPin;
  this->detach();
  delete this->_ticker;
  delete this->_ticker2;
  this->_ticker = new Ticker;
  this->_ticker2 = new Ticker;
  static xCMMC_LED *_that = this;
  static auto lambda = []() {
    _that->state = !_that->state;
    if (_that->state == LOW)
    {
      _that->prev_active = millis();
    }
    digitalWrite(_pin, _that->state);
  };
  static auto wtf = []() {
    uint32_t diff = (millis() - _that->prev_active);
    if (diff > 60L)
    {
      _that->prev_active = millis();
      _that->state = HIGH;
      digitalWrite(_pin, _that->state);
    }
  };
  // auto function  = static_cast<void (*)(int)>(lambda);
  this->_ticker->attach_ms(ms, lambda);
  this->_ticker2->attach_ms(30, wtf);
}