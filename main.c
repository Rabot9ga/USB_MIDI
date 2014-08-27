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
uint8_t adcInd = 1;
uint8_t adcBuf[2];
uint8_t isrTimer2 = 0, isrAdc = 0;


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

uint8_t ButtonsPoll(void)
{
  return ~PINB & BUTTONS_MASK;
}

void StartTimer2(void)
{
  TCNT2 = 0;
  TCCR2 |= (1 << CS22) | (1 << CS21) | (1 << CS20);
}

void StopTimer2(void)
{
  TCCR2 &= ~((1 << CS22) | (1 << CS21) | (1 << CS20));
}

uint8_t IsTimer2Run(void)
{
  return ((TCCR2 & ((1 << CS22) | (1 << CS21) | (1 << CS20))) == ((1 << CS22) | (1 << CS21) | (1 << CS20)));
}

uint8_t IsTimer2Flagged(void)
{
  return (TIFR & _BV(OCF2));
}

ISR(TIMER2_COMP_vect)
{
  isrTimer2 = 1;
  StopTimer2();
}

ISR(ADC_vect)
{
  if (adcInd)
  {
    adcBuf[0] = ADCH;
  }
  else
  {
    adcBuf[1] = ADCH;
  }
  isrAdc = 1;
}

int main(void)
{
  uint8_t keys = 0, lastKeys = 0;
  uint8_t adcLast[2] = {0}, adcTemp[2] = {0};

  SFIOR &= ~_BV(PUD);     // Enable pull-up resistors

  DDRB = _BV(LED_PIN);    // PORTB only output is LED_PIN
  PORTB = BUTTONS_MASK;   // Turn on pull-ups on button lines

  DDRD = 0;               // PORTD has all input pins. usbInit() will reconfigure USB pins as needed
  PORTD = ~(_BV(USB_CFG_DMINUS_BIT) | _BV(USB_CFG_DPLUS_BIT));   // Turn on pull-ups on PORTD except two USB lines

  DDRC = 0;               // PORTC now completely free
  PORTC = 0xFF;           // We don't need any outputs here

  TCCR2 = (0 << FOC2) | (1 << WGM21) | (0 << WGM20) | (0 << COM21) | (0 << COM20);
  OCR2 = 234;             // Presc. 1:1024, CTC-mode, compare time 20 ms
  TIMSK |= _BV(OCIE2);    // Enable compare interrupt on Timer2

  //выравнивание влево, нулевой канал
  ADMUX = (0<<REFS1)|(0<<REFS0)|(1<<ADLAR)|(0<<MUX3)|(0<<MUX2)|(0<<MUX1)|(0<<MUX0);
  //вкл. ацп, режим непрерывного преобр., разрешение прерывания,частота преобр. = FCPU/128
  ADCSRA = (1<<ADEN)|(1<<ADSC)|(1<<ADFR)|(1<<ADIF)|(1<<ADIE)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0);
    
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
    do
      usbPoll();
    while (!usbInterruptIsReady());

    msgIndex = 0;
    msgCount = 0;

    if (!IsTimer2Run())
    {
      keys = ButtonsPoll();
      if (keys != lastKeys)
      {
        StartTimer2();
      }
    }

    if (isrTimer2)
    {
      uint8_t tempKeys = ButtonsPoll();
      if (tempKeys == keys)
      {
        keys = lastKeys ^ tempKeys;
        for (i = 0; i < 2; i++)
        {
          if (keys & _BV(i))
          {
            if (tempKeys & _BV(i))
            {
              SendNoteOn(0x60 + i);
            }
            else
            {
              SendNoteOff(0x60 + i);
            }
          }
        }
        lastKeys = tempKeys;
      }
      isrTimer2 = 0;
    }
    
    if (msgCount && usbInterruptIsReady())
      usbSetInterrupt(midiMsg, msgIndex);
  }
}
