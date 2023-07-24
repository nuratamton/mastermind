/* ***************************************************************************** */
/* You can use this file to define the low-level hardware control fcts for       */
/* LED, button and LCD devices.                                                  */
/* Note that these need to be implemented in Assembler.                          */
/* You can use inline Assembler code, or use a stand-alone Assembler file.       */
/* Alternatively, you can implement all fcts directly in master-mind.c,          */
/* using inline Assembler code there.                                            */
/* The Makefile assumes you define the functions here.                           */
/* ***************************************************************************** */

#ifndef TRUE
#define TRUE (1 == 1)
#define FALSE (1 == 2)
#endif

#define PAGE_SIZE (4 * 1024)
#define BLOCK_SIZE (4 * 1024)

#define INPUT 0
#define OUTPUT 1

#define LOW 0
#define HIGH 1

#define LED 13
// GPIO pin for red LED
#define LED2 5
// GPIO pin for button
#define BUTTON 19

// -----------------------------------------------------------------------------
// includes
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <time.h>

// -----------------------------------------------------------------------------
// prototypes

int failure(int fatal, const char *message, ...);

// -----------------------------------------------------------------------------
// Functions to implement here (or directly in master-mind.c)

// adapted from setPinMode
void pinMode(uint32_t *gpio, int pin, int mode /*, int fSel, int shift */)
{
  int fSel, shift, res;

  switch (pin)
  {
  case LED:
    fSel = 1;
    shift = 9; // 13 -> (1,3*3)
    break;
  case LED2:
    fSel = 0;
    shift = 15; // 5 -> (0,5*3)
    break;
  case BUTTON:
    fSel = 1;
    shift = 27; // 13 -> (1,9*3)
    break;
  default:
    failure(TRUE, "pinMode: pin %d not supported\n", pin);
  }
  asm(/* inline assembler version of setting LED to ouput" */
      "\tLDR R1, %[gpio]\n"
      "\tADD R0, R1, %[fSel]\n" /* R0 = GPFSEL register to write to */
      "\tLDR R1, [R0, #0]\n"    /* read current value of the register */
      "\tMOV R2, #0b111\n"
      "\tLSL R2, %[shift]\n"
      "\tBIC R1, R1, R2\n"
      "\tMOV R2, %[mode]\n"
      "\tLSL R2, %[shift]\n" /* left shift by shift value */
      "\tORR R1, R2\n"
      "\tSTR R1, [R0, #0]\n"
      "\tMOV %[result], R1\n"
      : [result] "=r"(res)
      : [act] "r"(pin), [gpio] "m"(gpio), [fSel] "r"(fSel * 4), [shift] "r"(shift), [mode] "r"(mode)
      : "r0", "r1", "r2", "cc");
}

void writeLED(uint32_t *gpio, int led, int value)
{
  int off, res;

  switch (led)
  {
  case LED:
    off = (value == LOW) ? 10 : 7; // LED 13 for GREEN; register number for GPSET/GPCLR
    break;
  case LED2:
    off = (value == LOW) ? 10 : 7; // LED 5 for RED; register number for GPSET/GPCLR
    break;
  default:
    failure(TRUE, "writeLED: pin %d not supported\n", led);
  }
  asm(/* inline assembler version of setting/clearing LED to ouput" */
      "\tLDR R1, %[gpio]\n"
      "\tADD R0, R1, %[off]\n" /* R0 = GPSET/GPCLR register to write to */
      "\tMOV R2, #1\n"
      "\tMOV R1, %[led]\n" /* NB: this works only for pin 0-31; need to >> for higher ones */
      "\tAND R1, #31\n"
      "\tLSL R2, R1\n"       /* R2 = bitmask setting/clearing register %[act] */
      "\tSTR R2, [R0, #0]\n" /* write bitmask */
      "\tMOV %[result], R2\n"
      : [result] "=r"(res)
      : [led] "r"(led), [gpio] "m"(gpio), [off] "r"(off * 4)
      : "r0", "r1", "r2", "cc");
}

int readButton(uint32_t *gpio, int button)
{
  int theValue = 0;
  asm(
      "\tLDR R1, %[gpio]\n"
      "\tMOV R0, #52\n" /* 13*4 */
      "\tADD R1, R0\n"
      "\tLDR R1, [R1]\n" /* loading it into memory */
      "\tMOV R2, #1\n"
      "\tLSL R2, %[button]\n" /* R5 << button*/
      "\tAND %[theValue], R2, R1\n"
      : [theValue] "=r"(theValue)
      : [button] "r"(button), [gpio] "m"(gpio)
      : "r0", "r1", "r2", "cc");

  theValue = (theValue == 0) ? 0 : 1;

  return theValue;
}

void waitForButton(uint32_t *gpio, int button)
{
  while (readButton(gpio, button) == 0)
  {
  }
}