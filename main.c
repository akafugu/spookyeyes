/*
 * Spooky Lights
 * (C) 2011-13 Akafugu Corporation
 *
 * This program is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 *
 */

/*
 * Pinout:
 * PB0 (pin 5) - NC
 * PB1 (pin 6) - LED (OC0B)
 * PB2 (pin 7) - LED (OC0A)
 * PB3 (pin 2) - NC
 * PB4 (pin 3) - NC
 * PB5 (pin 1) - Reset
 *
 */

#define LED_PORT PORTB
#define LED_DDR  DDRB
#define LED_BIT PB0

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdbool.h>

#define DELAY 100

// for counting seconds
volatile uint8_t int_counter;

// for tracking which pattern to show
volatile uint8_t pattern;
#define MAX_PATTERN 5

void (*function_list[MAX_PATTERN+1])(void);

// random number seed (will give same flicker sequence each time)
//volatile uint32_t lfsr = 0xbeefcace;
volatile uint32_t lfsr = 0xbeefdead;

uint32_t rand(void)
{
	// http://en.wikipedia.org/wiki/Linear_feedback_shift_register
	// Galois LFSR: taps: 32 31 29 1; characteristic polynomial: x^32 + x^31 + x^29 + x + 1 */
  	lfsr = (lfsr >> 1) ^ (-(lfsr & 1u) & 0xD0000001u);
	return lfsr;
}


///
/// HELPER FUNCTIONS
///

///
/// PATTERNS
///

// blink all with shrinking delay
void blink_all(void)
{
	static uint8_t direction = 0;
	static uint8_t delays = 255;
	
	if (delays % 2 == 0) {
		OCR0A = 0xff;
		OCR0B = 0xff;
	}
	else {
		OCR0A = 0x0;
		OCR0B = 0x0;
	}

	for (uint8_t i = 0; i < delays; i++)
		_delay_ms(2);
	
	if (direction) {
		delays+=5;
		if (delays >= 250) direction = 0;
	}
	else {
		delays-=5;
		if (delays <= 5) direction = 1;
	}
}

void blink_staggered(void)
{
	static uint8_t i = 0;
	
	if (i == 0) {
		OCR0A = 0;
		OCR0B = 0xff;
		i = 1;
	}
	else {
		OCR0A = 0xff;
		OCR0B = 0;
		i = 0;
	}
	
	_delay_ms(DELAY);
}

void breathe(bool even)
{
	for (uint16_t i = 15 ; i <= 255; i+=1) {
		OCR0B = i;
		if (even)
			OCR0A = i;
		else
			OCR0A = (255 - i) + 15;

		if (i > 150) {
			_delay_ms(4);
		}
		if ((i > 125) && (i < 151)) {
			_delay_ms(5);
		}
		if (( i > 100) && (i < 126)) {
			_delay_ms(7);
		}
		if (( i > 75) && (i < 101)) {
			_delay_ms(10);
		}
		if (( i > 50) && (i < 76)) {
			_delay_ms(14);
		}
		if (( i > 25) && (i < 51)) {
			_delay_ms(18);
		}
		if (( i > 1) && (i < 26)) {
			_delay_ms(19);
		}
	}
	for (uint16_t i = 255; i >=15; i-=1) {
		OCR0B = i;
		if (even)
			OCR0A = i;
		else
			OCR0A = (255 - i) + 15;

		if (i > 150) {
			_delay_ms(4);
		}
		if ((i > 125) && (i < 151)) {
			_delay_ms(5);
		}
		if (( i > 100) && (i < 126)) {
			_delay_ms(7);
		}
		if (( i > 75) && (i < 101)) {
			_delay_ms(10);
		}
		if (( i > 50) && (i < 76)) {
			_delay_ms(14);
		}
		if (( i > 25) && (i < 51)) {
			_delay_ms(18);
		}
		if (( i > 1) && (i < 26)) {
			_delay_ms(19);
		}
	}
	_delay_ms(300);
}

void breathe_even()
{
	breathe(true);
}

void breathe_odd()
{
	breathe(false);
}

ISR(WDT_vect)
{
	pattern = (rand()>>24) % MAX_PATTERN;
}

void main(void) __attribute__ ((noreturn));

void main(void)
{
	// Set up watchdog timer
	//WDTCR |= (1<<WDP2) | (1<<WDP0); // 0.5s prescaler
	WDTCR |= (1<<WDP3); // (1<<WDP2) | (1<<WDP0); // 4s prescaler
	WDTCR |= (1<<WDTIE); // enable watchdog interrupts

	// 10-bit fast PWM with prescaler = 1 (7.58kHz)
	TCCR0A = _BV(COM0A1) | _BV(COM0B1) | _BV(WGM00) | _BV(WGM01);
	TCCR0B = _BV(CS00);

	OCR0A = 0x0;
	OCR0B = 0x0;

	DDRB |= (1 << 0)|(1 << 1)|(1 << 2)|(1 << 3);

	PORTB = 0;

	function_list[0] = &blink_staggered;
	function_list[1] = &blink_all;
	function_list[2] = &breathe_even;
	function_list[3] = &breathe_even;
	function_list[4] = &breathe_odd;

	sei();


	while(1) {
		function_list[pattern]();
	}
}


