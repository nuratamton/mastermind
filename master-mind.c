/*
 * MasterMind implementation: template; see comments below on which parts need to be completed
 * CW spec: https://www.macs.hw.ac.uk/~hwloidl/Courses/F28HS/F28HS_CW2_2022.pdf
 * This repo: https://gitlab-student.macs.hw.ac.uk/f28hs-2021-22/f28hs-2021-22-staff/f28hs-2021-22-cwk2-sys

 * Compile:
 gcc -c -o lcdBinary.o lcdBinary.c
 gcc -c -o master-mind.o master-mind.c
 gcc -o master-mind master-mind.o lcdBinary.o
 * Run:
 sudo ./master-mind

 OR use the Makefile to build
 > make all
 and run
 > make run
 and test
 > make test

 ***********************************************************************
 * The Low-level interface to LED, button, and LCD is based on:
 * wiringPi libraries by
 * Copyright (c) 2012-2013 Gordon Henderson.
 ***********************************************************************
 * See:
 *	https://projects.drogon.net/raspberry-pi/wiringpi/
 *
 *    wiringPi is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU Lesser General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    wiringPi is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Lesser General Public License for more details.
 *
 *    You should have received a copy of the GNU Lesser General Public License
 *    along with wiringPi.  If not, see <http://www.gnu.org/licenses/>.
 ***********************************************************************
*/

/* ======================================================= */
/* SECTION: includes                                       */
/* ------------------------------------------------------- */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>

#include <unistd.h>
#include <string.h>
#include <time.h>

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ioctl.h>

/* --------------------------------------------------------------------------- */
/* Config settings */
/* you can use CPP flags to e.g. print extra debugging messages */
/* or switch between different versions of the code e.g. digitalWrite() in Assembler */
#define DEBUG
#undef ASM_CODE

// =======================================================
// Tunables
// PINs (based on BCM numbering)
// For wiring see CW spec: https://www.macs.hw.ac.uk/~hwloidl/Courses/F28HS/F28HS_CW2_2022.pdf
// GPIO pin for green LED
#define LED 13
// GPIO pin for red LED
#define LED2 5
// GPIO pin for button
#define BUTTON 19
// =======================================================
// delay for loop iterations (mainly), in ms
// in mili-seconds: 0.2s
#define DELAY 200
// in micro-seconds: 3s
#define TIMEOUT 3000000
// =======================================================
// APP constants   ---------------------------------
// number of colours and length of the sequence
#define COLS 3
#define SEQL 3
// =======================================================

// generic constants

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

// =======================================================
// Wiring (see inlined initialisation routine)

#define STRB_PIN 24
#define RS_PIN 25
#define DATA0_PIN 23
#define DATA1_PIN 10
#define DATA2_PIN 27
#define DATA3_PIN 22

/* ======================================================= */
/* SECTION: constants and prototypes                       */
/* ------------------------------------------------------- */

// =======================================================
// char data for the CGRAM, i.e. defining new characters for the display

static unsigned char newChar[8] =
    {
        0b11111,
        0b10001,
        0b10001,
        0b10101,
        0b11111,
        0b10001,
        0b10001,
        0b11111,
};

/* Constants */

static const int colors = COLS; // 3
static const int seqlen = SEQL; // 3

static char *color_names[] = {"red", "green", "blue"};

static int *theSeq = NULL;

static int *seq1, *seq2, *cpy1, *cpy2;

int finished;

/* --------------------------------------------------------------------------- */

// Mask for the bottom 64 pins which belong to the Raspberry Pi
//	The others are available for the other devices

#define PI_GPIO_MASK (0xFFFFFFC0)

static unsigned int gpiobase;
static uint32_t *gpio;

static int timed_out = 0;
static uint32_t *timer;

/* ------------------------------------------------------- */
// misc prototypes

int failure(int fatal, const char *message, ...);
void waitForEnter(void);
void waitForButton(uint32_t *gpio, int button);

/* ======================================================= */
/* SECTION: hardware interface (LED, button, LCD display)  */
/* ------------------------------------------------------- */
/* low-level interface to the hardware */

/* ********************************************************** */
/* COMPLETE the code for all of the functions in this SECTION */
/* Either put them in a separate file, lcdBinary.c, and use   */
/* inline Assembler there, or use a standalone Assembler file */
/* You can also directly implement them here (inline Asm).    */
/* ********************************************************** */

/* These are just prototypes; you need to complete the code for each function */

/* send a @value@ (LOW or HIGH) on pin number @pin@; @gpio@ is the mmaped GPIO base address */
void digitalWrite(uint32_t *gpio, int pin, int value);

/* set the @mode@ of a GPIO @pin@ to INPUT or OUTPUT; @gpio@ is the mmaped GPIO base address */
void pinMode(uint32_t *gpio, int pin, int mode);

/* send a @value@ (LOW or HIGH) on pin number @pin@; @gpio@ is the mmaped GPIO base address */
/* can use digitalWrite(), depending on your implementation */
void writeLED(uint32_t *gpio, int led, int value);

/* read a @value@ (LOW or HIGH) from pin number @pin@ (a button device); @gpio@ is the mmaped GPIO base address */
int readButton(uint32_t *gpio, int button);

/* wait for a button input on pin number @button@; @gpio@ is the mmaped GPIO base address */
/* can use readButton(), depending on your implementation */
void waitForButton(uint32_t *gpio, int button);

/* ======================================================= */
/* SECTION: game logic                                     */
/* ------------------------------------------------------- */
/* AUX fcts of the game logic */

/* ********************************************************** */
/* COMPLETE the code for all of the functions in this SECTION */
/* Implement these as C functions in this file                */
/* ********************************************************** */

/* initialise the secret sequence; by default it should be a random sequence */
void initSeq()
{
  /* memory allocation for theSeq */
  theSeq = malloc(seqlen * sizeof(int));
  /* to randomize each time */
  srand(time(NULL));
  for (int i = 0; i < seqlen; i++)
  {
    /* for each value of i, assign a random value between 0 and 4 */
    theSeq[i] = (rand() % seqlen) + 1;
  }
}

/* display the sequence on the terminal window, using the format from the sample run in the spec */
void showSeq(int *seq)
{
  printf("Secret: ");
  /* looping through seq */
  for (int i = 0; i < seqlen; i++)
  {
    /* printing each value of seq */
    printf("%d ", seq[i]);
  }
  printf("\n");
}

#define NAN1 8
#define NAN2 9

/* counts how many entries in seq2 match entries in seq1 */
/* returns exact and approximate matches, either both encoded in one value, */
/* or as a pointer to a pair of values */

int *countMatches(int *seq1, int *seq2)
{
  int exact, found;
  int *accuracy;
  accuracy = (int *)malloc(2 * sizeof(int));

  /* looping though the size of seq1 and checking for exact matches */
  for (int i = 0; i < seqlen; i++)
  {
    if (seq1[i] == seq2[i])
    {
      if (found == seq2[i])
      {
        accuracy[1]--;
      }
      /* incrementing the number of exact matches */
      accuracy[0]++;
      exact = seq2[i];
    }
    else
    {
      /* looping though the size of seq2 for each loop of seq1 */
      for (int j = 0; j < seqlen; j++)
      {
        /* if any element of first seq matches with any element of the second sequence */
        /* and wasn't accounted for in exact, increment approx */
        if (seq2[i] == seq1[j] && found != seq2[j] && exact != seq2[i])
        {
          found = seq2[j];
          /* incrementing the number of approx matches */
          accuracy[1]++;
        }
      }
    }
  }
  /* returns exact and approximate matches */
  /* as a pointer to a pair of values */
  return accuracy;
}

/* show the results from calling countMatches on seq1 and seq2 */
void showMatches(int *code, int *seq1, int *seq2, int lcd_format)
{
  int *accuracy;
  accuracy = (int *)malloc(2 * sizeof(int));
  /* accuracy from count matches is returned to accuracy */
  accuracy = countMatches(seq1, seq2);
  /* the first and second values of the array are printed */
  /* exact matches */
  printf("%d exact\n", accuracy[0]);
  /* approx matches */
  printf("%d approximate\n", accuracy[1]);
}

/* parse an integer value as a list of digits, and put them into @seq@ */
/* needed for processing command-line with options -s or -u            */
void readSeq(int *seq, int val)
{
  int digit = 2;
  while (digit > -1)
  {
    /* stores last digit at index 2 first and then decrements */
    seq[digit] = val % 10;
    val = val / 10;
    digit--;
  }
}

/* read a guess sequence from stdin and store the values in arr */
/* only needed for testing the game logic, without button input */
int *readNum(int max)
{
  int guess[max];
  printf("Enter a guess sequence: ");
  for (int i = 0; i < max; i++)
  {
    seq2[i] = scanf("%d", guess[i]);
  }
  return seq2;
}

/* ======================================================= */
/* SECTION: TIMER code                                     */
/* ------------------------------------------------------- */
/* TIMER code */

/* timestamps needed to implement a time-out mechanism */
static uint64_t startT, stopT;

/* ********************************************************** */
/* COMPLETE the code for all of the functions in this SECTION */
/* Implement these as C functions in this file                */
/* ********************************************************** */

/* you may need this function in timer_handler() below  */
/* use the libc fct gettimeofday() to implement it      */

uint64_t timeInMicroseconds()
{

  /* ***  COMPLETE the code here  ***  */

  struct timeval currentTime;
  gettimeofday(&currentTime, NULL);
  return currentTime.tv_sec * 1000LL + currentTime.tv_usec / 1000;
}

/* this should be the callback, triggered via an interval timer, */
/* that is set-up through a call to sigaction() in the main fct. */
void timer_handler(int signum)
{
  static int count = 0;
  stopT = timeInMicroseconds();
  count++;
  startT = timeInMicroseconds();
}

/* initialise time-stamps, setup an interval timer, and install the timer_handler callback */
void initITimer(uint64_t timeout)
{
  startT = timeInMicroseconds();
  stopT = startT + timeout;
}

/* ======================================================= */
/* SECTION: Aux function                                   */
/* ------------------------------------------------------- */
/* misc aux functions */

int failure(int fatal, const char *message, ...)
{
  va_list argp;
  char buffer[1024];

  if (!fatal) //  && wiringPiReturnCodes)
    return -1;

  va_start(argp, message);
  vsnprintf(buffer, 1023, message, argp);
  va_end(argp);

  fprintf(stderr, "%s", buffer);
  exit(EXIT_FAILURE);

  return 0;
}

/*
 * waitForEnter:
 *********************************************************************************
 */

void waitForEnter(void)
{
  printf("Press ENTER to continue: ");
  (void)fgetc(stdin);
}

/*
 * delay:
 *	Wait for some number of milliseconds
 *********************************************************************************
 */

void delay(unsigned int howLong)
{
  struct timespec sleeper, dummy;

  sleeper.tv_sec = (time_t)(howLong / 1000);
  sleeper.tv_nsec = (long)(howLong % 1000) * 1000000;

  nanosleep(&sleeper, &dummy);
}

/* From wiringPi code; comment by Gordon Henderson
 * delayMicroseconds:
 *	This is somewhat intersting. It seems that on the Pi, a single call
 *	to nanosleep takes some 80 to 130 microseconds anyway, so while
 *	obeying the standards (may take longer), it's not always what we
 *	want!
 *
 *	So what I'll do now is if the delay is less than 100uS we'll do it
 *	in a hard loop, watching a built-in counter on the ARM chip. This is
 *	somewhat sub-optimal in that it uses 100% CPU, something not an issue
 *	in a microcontroller, but under a multi-tasking, multi-user OS, it's
 *	wastefull, however we've no real choice )-:
 *
 *      Plan B: It seems all might not be well with that plan, so changing it
 *      to use gettimeofday () and poll on that instead...
 *********************************************************************************
 */

void delayMicroseconds(unsigned int howLong)
{
  struct timespec sleeper;
  unsigned int uSecs = howLong % 1000000;
  unsigned int wSecs = howLong / 1000000;

  /**/ if (howLong == 0)
    return;
#if 0
  else if (howLong  < 100)
    delayMicrosecondsHard (howLong) ;
#endif
  else
  {
    sleeper.tv_sec = wSecs;
    sleeper.tv_nsec = (long)(uSecs * 1000L);
    nanosleep(&sleeper, NULL);
  }
}

/* ======================================================= */
/* SECTION: aux functions for game logic                   */
/* ------------------------------------------------------- */

/* ********************************************************** */
/* COMPLETE the code for all of the functions in this SECTION */
/* Implement these as C functions in this file                */
/* ********************************************************** */

/* blink the led on pin @led@, @c@ times */
void blinkN(uint32_t *gpio, int led, int c)
{
  for (int i = 0; i < c; i++)
  {
    /* turns led on */
    writeLED(gpio, led, 1);
    /* delay*/
    delay(DELAY);
    /* turns led off */
    writeLED(gpio, led, 0);
    delay(DELAY);
  }
}

/* get the number of times the button is pressed */
int getInput()
{
  int buttonPressed, count;
  buttonPressed = 0;

  waitForButton(gpio, BUTTON);
  initITimer(3000);

  /* while the stop time is greater than the current time */
  while (stopT > timeInMicroseconds())
  {
    count = 0;
    if (readButton(gpio, BUTTON) == HIGH) // 0000000111111*10*0000008
    {
      /* the value of count is updated to one */
      /* to ensure that all the 1s aren't counted when the button is pressed */
      /* count is updated to 1 during transformation from 1 to 0 */
      count = 1;
    }
    /* buttonPressed is assigned the value of count before count gets reset */
    buttonPressed += count;
    delay(500);
  }
  return buttonPressed;
}

/* ======================================================= */
/* SECTION: main fct                                       */
/* ------------------------------------------------------- */

int main(int argc, char *argv[])
{ // this is just a suggestion of some variable that you may want to use

  int found = 0, attempts = 0, i, j, code;
  int c, d, buttonPressed, rel, foo;
  int *attSeq;

  int pinLED = LED, pin2LED2 = LED2, pinButton = BUTTON;
  int fSel, shift, pin, clrOff, setOff, off, res;
  int fd;

  int exact, contained;
  char str1[32];
  char str2[32];

  struct timeval t1, t2;
  int t;

  char buf[32];

  // variables for command-line processing
  char str_in[20], str[20] = "some text";
  int verbose = 0, debug = 0, help = 0, opt_m = 0, opt_n = 0, opt_s = 0, unit_test = 0;
  int *res_matches;

  // -------------------------------------------------------
  // process command-line arguments

  // see: man 3 getopt for docu and an example of command line parsing
  { // see the CW spec for the intended meaning of these options
    int opt;
    while ((opt = getopt(argc, argv, "hvdus:")) != -1)
    {
      switch (opt)
      {
      case 'v':
        verbose = 1;
        break;
      case 'h':
        help = 1;
        break;
      case 'd':
        debug = 1;
        break;
      case 'u':
        unit_test = 1;
        break;
      case 's':
        opt_s = atoi(optarg);
        break;
      default: /* '?' */
        fprintf(stderr, "Usage: %s [-h] [-v] [-d] [-u <seq1> <seq2>] [-s <secret seq>]  \n", argv[0]);
        exit(EXIT_FAILURE);
      }
    }
  }

  if (help)
  {
    fprintf(stderr, "MasterMind program, running on a Raspberry Pi, with connected LED, button and LCD display\n");
    fprintf(stderr, "Use the button for input of numbers. The LCD display will show the matches with the secret sequence.\n");
    fprintf(stderr, "For full specification of the program see: https://www.macs.hw.ac.uk/~hwloidl/Courses/F28HS/F28HS_CW2_2022.pdf\n");
    fprintf(stderr, "Usage: %s [-h] [-v] [-d] [-u <seq1> <seq2>] [-s <secret seq>]  \n", argv[0]);
    exit(EXIT_SUCCESS);
  }

  if (unit_test && optind >= argc - 1)
  {
    fprintf(stderr, "Expected 2 arguments after option -u\n");
    exit(EXIT_FAILURE);
  }

  if (verbose && unit_test)
  {
    printf("1st argument = %s\n", argv[optind]);
    printf("2nd argument = %s\n", argv[optind + 1]);
  }

  if (verbose)
  {
    fprintf(stdout, "Settings for running the program\n");
    fprintf(stdout, "Verbose is %s\n", (verbose ? "ON" : "OFF"));
    fprintf(stdout, "Debug is %s\n", (debug ? "ON" : "OFF"));
    fprintf(stdout, "Unittest is %s\n", (unit_test ? "ON" : "OFF"));
    if (opt_s)
      fprintf(stdout, "Secret sequence set to %d\n", opt_s);
  }

  seq1 = (int *)malloc(seqlen * sizeof(int));
  seq2 = (int *)malloc(seqlen * sizeof(int));
  cpy1 = (int *)malloc(seqlen * sizeof(int));
  cpy2 = (int *)malloc(seqlen * sizeof(int));

  // check for -u option, and if so run a unit test on the matching function
  if (unit_test && argc > optind + 1)
  { // more arguments to process; only needed with -u
    strcpy(str_in, argv[optind]);
    opt_m = atoi(str_in);
    strcpy(str_in, argv[optind + 1]);
    opt_n = atoi(str_in);
    // CALL a test-matches function; see testm.c for an example implementation
    readSeq(seq1, opt_m); // turn the integer number into a sequence of numbers
    readSeq(seq2, opt_n); // turn the integer number into a sequence of numbers
    if (verbose)
      fprintf(stdout, "Testing matches function with sequences %d and %d\n", opt_m, opt_n);
    res_matches = countMatches(seq1, seq2);
    showMatches(res_matches, seq1, seq2, 1);
    exit(EXIT_SUCCESS);
  }
  else
  {
    /* nothing to do here; just continue with the rest of the main fct */
  }

  if (opt_s)
  { // if -s option is given, use the sequence as secret sequence
    if (theSeq == NULL)
      theSeq = (int *)malloc(seqlen * sizeof(int));
    readSeq(theSeq, opt_s);
    if (verbose)
    {
      fprintf(stderr, "Running program with secret sequence:\n");
      showSeq(theSeq);
    }
  }

  // -------------------------------------------------------
  // LCD constants, hard-coded: 16x2 display, using a 4-bit connection

  if (geteuid() != 0)
    fprintf(stderr, "setup: Must be root. (Did you forget sudo?)\n");

  // init of guess sequence, and copies (for use in countMatches)
  attSeq = (int *)malloc(seqlen * sizeof(int));
  cpy1 = (int *)malloc(seqlen * sizeof(int));
  cpy2 = (int *)malloc(seqlen * sizeof(int));

  // -----------------------------------------------------------------------------
  // constants for RPi2
  gpiobase = 0x3F200000;

  // -----------------------------------------------------------------------------
  // memory mapping
  // Open the master /dev/memory device

  if ((fd = open("/dev/mem", O_RDWR | O_SYNC | O_CLOEXEC)) < 0)
    return failure(FALSE, "setup: Unable to open /dev/mem: %s\n", strerror(errno));

  // GPIO:
  gpio = (uint32_t *)mmap(0, BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, gpiobase);
  if ((int32_t)gpio == -1)
    return failure(FALSE, "setup: mmap (GPIO) failed: %s\n", strerror(errno));

  // -------------------------------------------------------
  // Configuration of LED and BUTTON

  /* ***  COMPLETE the code here  ***  */

  // // controlling LED pin 5
  // fSel = 0;   // 0   // GPIO 5 lives in register 0 (GPFSEL2)
  // shift = 15; // 15  // GPIO 5 sits in slot 5 of register 0, thus shift by 3*5 (3 bits per pin)
  // // C version of setting LED
  // *(gpio + fSel) = (*(gpio + fSel) & ~(7 << shift)) | (1 << shift); // Sets bits to one = output

  pinMode(gpio, pin2LED2, OUTPUT);

  // // controlling LED pin 13
  // fSel = 0;  // 0   // GPIO 13 lives in register 1 (GPFSEL2)
  // shift = 9; // 9  // GPIO 13 sits in slot 3 of register 1, thus shift by 3*3 (3 bits per pin)
  // // C version of setting LED
  // *(gpio + fSel) = (*(gpio + fSel) & ~(7 << shift)) | (1 << shift); // Sets bits to one = output

  pinMode(gpio, pinLED, OUTPUT);

  // // controlling button pin 19
  // fSel = 1;   // 1 // GPIO 19 lives in register 2 (GPFSEL2)
  // shift = 27; // 27 // GPIO 19 sits in slot 9 of register 1, thus shift by 9*3 (3 bits per pin)
  // // C version of mode input for button
  // *(gpio + fSel) = *(gpio + fSel) & ~(7 << shift); // Sets bits to one = output

  pinMode(gpio, pinButton, INPUT);

  // Gordon Henderson's explanation of this part of the init code (from wiringPi):
  // 4-bit mode?
  //	OK. This is a PIG and it's not at all obvious from the documentation I had,
  //	so I guess some others have worked through either with better documentation
  //	or more trial and error... Anyway here goes:
  //
  //	It seems that the controller needs to see the FUNC command at least 3 times
  //	consecutively - in 8-bit mode. If you're only using 8-bit mode, then it appears
  //	that you can get away with one func-set, however I'd not rely on it...
  //
  //	So to set 4-bit mode, you need to send the commands one nibble at a time,
  //	the same three times, but send the command to set it into 8-bit mode those
  //	three times, then send a final 4th command to set it into 4-bit mode, and only
  //	then can you flip the switch for the rest of the library to work in 4-bit
  //	mode which sends the commands as 2 x 4-bit values.

  // -----------------------------------------------------------------------------
  // Start of game
  fprintf(stderr, "Printing welcome message on the LCD display ...\n");
  fprintf(stderr, "Printing welcome message on the LCD display ...\n");

  /* initialise the secret sequence */
  if (!opt_s)
    initSeq();
  if (debug)
    showSeq(theSeq);

  // optionally one of these 2 calls:
  // waitForEnter () ;
  // waitForButton (gpio, pinButton) ;

  // -----------------------------------------------------------------------------
  // +++++ main loop

  while (!found)
  {
    int *guess = (int *)malloc(3 * sizeof(int));
    for (int i = 0; i < 3; i++)
    {

      /* the number of times button is pressed is stored here */
      buttonPressed = getInput();

      if (buttonPressed == 1)
      {
        blinkN(gpio, LED2, 1);
        blinkN(gpio, LED, 1);
      }

      else if (buttonPressed == 2)
      {
        blinkN(gpio, LED2, 1);
        blinkN(gpio, LED, 2);
      }

      else
      {
        buttonPressed = 3;
        blinkN(gpio, LED2, 1);
        blinkN(gpio, LED, 3);
      }
      guess[i] = buttonPressed;
    }

    int *accuracy = countMatches(theSeq, guess);
    showMatches(accuracy, theSeq, guess, 1);

    /* marking the end of a round */
    blinkN(gpio, LED2, 2);
    delay(DELAY);
    /* green led blinks exact matches */
    blinkN(gpio, LED, accuracy[0]);
    delay(DELAY);
    /* red led - separator */
    blinkN(gpio, LED2, 1);
    delay(DELAY);
    /* green led blinks approx matches */
    blinkN(gpio, LED, accuracy[1]);

    if (accuracy[0] == 3)
    {
      found = 1;
    }
    else
    {
      blinkN(gpio, LED2, 3);
    }

    attempts++;

    /* ******************************************************* */
    /* ***  COMPLETE the code here  ***                        */
    /* this needs to implement the main loop of the game:      */
    /* check for button presses and count them                 */
    /* store the input numbers in the sequence @attSeq@        */
    /* compute the match with the secret sequence, and         */
    /* show the result                                         */
    /* see CW spec for details                                 */
    /* ******************************************************* */
  }
  if (found)
  {
    /* success output m*/
    printf("SUCCESS\nNumber of attempts: %d\n", attempts);
    writeLED(gpio, LED2, 1);
    blinkN(gpio, LED, 3);
    writeLED(gpio, LED2, 0);
  }
  else
  {
    fprintf(stdout, "Sequence not found\n");
  }
  return 0;
}
