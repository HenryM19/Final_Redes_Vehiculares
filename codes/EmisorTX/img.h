#ifndef _IMG_H
#define _IMG_H

#include "arduino.h"

const unsigned char logo1[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x78, 0xF0, 0x00, 0x00, 0x00, 0x01, 0x00, 0x03, 0x00, 0x40, 0x00, 0x00, 
  0x0C, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x20, 0x08, 
  0x00, 0x00, 0x08, 0x20, 0x30, 0x00, 0x00, 0x06, 0x40, 0x60, 0x00, 0x80, 
  0x03, 0x40, 0xC0, 0x03, 0xE0, 0x01, 0x80, 0x80, 0xFF, 0xFF, 0x00, 0x80, 
  0x00, 0xFF, 0x7F, 0x02, 0x80, 0x00, 0xFE, 0x3F, 0x01, 0x80, 0x00, 0xFC, 
  0x9F, 0x01, 0x80, 0x00, 0xF8, 0xCF, 0x00, 0x80, 0x00, 0xF0, 0xE7, 0x00, 
  0x40, 0x00, 0xE0, 0xFB, 0x00, 0x40, 0x00, 0xC0, 0x7D, 0x00, 0x40, 0x00, 
  0x80, 0x7E, 0x00, 0x20, 0x00, 0x00, 0x7F, 0x00, 0x20, 0x00, 0x80, 0x7F, 
  0x00, 0x10, 0x00, 0x80, 0x7F, 0x00, 0x08, 0x00, 0x80, 0x7F, 0x00, 0x0C, 
  0x00, 0x00, 0x7F, 0x00, 0x06, 0x00, 0x00, 0x7E, 0x00, 0x03, 0x00, 0x00, 
  0x7C, 0x80, 0x00, 0x00, 0x00, 0x78, 0x60, 0x00, 0x00, 0x00, 0xF0, 0x30, 
  0x00, 0x00, 0x00, 0xE0, 0x0E, 0x00, 0x00, 0x00, 0xC0, 0x03, 0x00, 0x00, 
  0x00, 0xF8, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 
  0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, };

  const unsigned char* logos[3] = {
  logo1
};

  #endif // _IMG_H