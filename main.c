/*  Name: main.c
    Project: USB-MIDI pedal with controls
    Authors: Dandi (github.com/Dandi91), Rabot9ga (github.com/Rabot9ga)
    Creation Date: 22-08-2014
    Modified: 25-08-2014
    License: GPL

    MIDI interface based on project V-USB MIDI device on Low-Speed USB
    Author: Martin Homuth-Rosemann
    and V-USB library (released 06-12-2012)
*/

#include <avr/io.h>
#include <avr/interrupt.h>

#include "usbdrv/usbconfig.h"
#include "usbdrv/usbdrv.h"

// Buttons
#define BUTTONS_MASK            0x03
#define BUTTON0_MASK            0x01
#define BUTTON1_MASK            0x02
//LED
#define LED_PORT                PORTB
#define LED_PIN                 0x02

// MIDI defines
#define MIDI_CHANNEL            0x0

// MIDI message codes
#define MIDI_MSG_NOTE_ON        0x9
#define MIDI_MSG_NOTE_OFF       0x8
#define MIDI_MSG_CTRL_CHANGE    0xB

// Defaults for not used settings
#define MIDI_DEF_NOTE_VELOCITY  0x7F

#define MAX_MSG_COUNT           2

// File with USB descriptiors
#include "descriptors.h"

// Some debug macros for LED
#define LED_ON  LED_PORT |= _BV(LED_PIN)
#define LED_OFF LED_PORT &= ~_BV(LED_PIN)

uint8_t midiMsg[8];
uint8_t msgIndex = 0, msgCount = 0;

uint8_t usbFunctionDescriptor(usbRequest_t* rq)
{
	if (rq->wValue.bytes[1] == USBDESCR_DEVICE)
  {
		usbMsgPtr = (uint8_t*)deviceDescrMIDI;
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

  SFIOR &= ~_BV(PUD);     // Enable pull-up resistors

  DDRB = _BV(LED_PIN);    // PORTB only output is LED_PIN
  PORTB |= BUTTONS_MASK;  // Turn on pull-ups on button lines

  DDRD = 0;               // PORTD has all input pins. usbInit() will reconfigure USB pins as needed
  PORTD = 0;              // Turn off pull-ups on PORTD

  DDRC = 0;               // PORTC now completely free
  PORTC = 0;              // We don't need any outputs or pull-ups here

  // TODO: contact debounce timer setup & use

  usbInit();
  usbDeviceDisconnect();  // Start reenumeration
  uint8_t i = 0;
  while (--i)             // Emulate USB disconnect for ~10ms
  {
    uint8_t j = 0;
    while (--j);
  }
  usbDeviceConnect();
  sei();
  // End of initialization

  LED_ON;

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
