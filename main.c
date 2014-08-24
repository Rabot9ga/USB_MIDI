//#define F_CPU 12000000

#include <avr/interrupt.h>
#include <avr/io.h>
#include "usbdrv/usbconfig.h"
#include "usbdrv/usbdrv.h"



#define BUTTONS_MASK            0x03
#define BUTTON0_MASK            0x01
#define BUTTON1_MASK            0x02

// MIDI defines
#define MIDI_CHANNEL            0x0

// MIDI message codes
#define MIDI_MSG_NOTE_ON        0x9
#define MIDI_MSG_NOTE_OFF       0x8
#define MIDI_MSG_CTRL_CHANGE    0xB


// Defaults for not used settings
#define MIDI_DEF_NOTE_VELOCITY  0x7F

#define MAX_MSG_COUNT           2

#include "descriptors.h"

uint8_t midiMsg[8];
uint8_t msgIndex = 0, msgCount = 0;

uint8_t usbFunctionDescriptor(usbRequest_t* rq)
{
	if (rq->wValue.bytes[1] == USBDESCR_DEVICE)
  {
		usbMsgPtr = (uchar*)deviceDescrMIDI;
		return sizeof(deviceDescrMIDI);
	}
  else
  {
    /* must be config descriptor */
		usbMsgPtr = (uint8_t*)configDescrMIDI;
		return sizeof(configDescrMIDI);
	}
}

uint8_t usbFunctionSetup(uint8_t data[8])
{
	
	return 0xff;
}

void SendNoteOn(uint8_t note)
{
  if (msgCount++ < MAX_MSG_COUNT)
  {
    midiMsg[msgIndex++] = MIDI_MSG_NOTE_ON | (MIDI_CHANNEL << 4);    // USB protocol
    midiMsg[msgIndex++] = MIDI_CHANNEL | (MIDI_MSG_NOTE_ON << 4);    // MIDI protocol
    midiMsg[msgIndex++] = note;
    midiMsg[msgIndex++] = MIDI_DEF_NOTE_VELOCITY;
  }
}

void SendNoteOff(uint8_t note)
{
  if (msgCount++ < MAX_MSG_COUNT)
  {
    midiMsg[msgIndex++] = MIDI_MSG_NOTE_OFF | (MIDI_CHANNEL << 4);    // USB protocol
    midiMsg[msgIndex++] = MIDI_CHANNEL | (MIDI_MSG_NOTE_OFF << 4);    // MIDI protocol
    midiMsg[msgIndex++] = note;
    midiMsg[msgIndex++] = MIDI_DEF_NOTE_VELOCITY;
  }
}

int main(void)
{
  uint8_t lastKeys = 0;

  SFIOR &= ~(1 << PUD);   // Enable pull-up resistors

  // Set PORTB0..1 for buttons with pull-up
  DDRB &= ~(BUTTONS_MASK);    // 0b11111100
  DDRB |= (1 << 2);
  PORTB |= BUTTONS_MASK;
  
  
  usbInit();
  usbDeviceDisconnect(); // запускаем принудительно реэнумерацию
  uint8_t i = 0;
  while (--i)            // эмулируем USB дисконнект на время ~10 мс
  {
    uint8_t j = 0;
    while (--j);
  }
  usbDeviceConnect();
  PORTB |= (1 << 2);
  sei();

  for (;;)
  {
    usbPoll();
    if (usbInterruptIsReady())
    {
      msgIndex = 0;
      msgCount = 0;
    }
    uint8_t keys = PINB & BUTTONS_MASK;
    if (keys != lastKeys)
    {
      if (keys & BUTTON0_MASK)
        SendNoteOn(0x60);
      else
        SendNoteOff(0x60);
      if (keys & BUTTON1_MASK)
        SendNoteOn(0x61);
      else
        SendNoteOff(0x61);
      lastKeys = keys;
    }
    if (msgCount && usbInterruptIsReady())
      usbSetInterrupt(midiMsg, msgIndex);
  }
}
