/*
  A C program to test the matching function (for master-mind) as implemented in matches.s

$ as  -o mm-matches.o mm-matches.s
$ gcc -c -o testm.o testm.c
$ gcc -o testm testm.o mm-matches.o
$ ./testm
*/

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

#define LENGTH 3
#define COLORS 3

#define NAN1 8
#define NAN2 9

const int seqlen = LENGTH;
const int seqmax = COLORS;

static int *seq1, *seq2, *cpy1, *cpy2;

/* ********************************** */
/* take these fcts from master-mind.c */
/* ********************************** */

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

/* show the results from calling countMatches on seq1 and seq1 */
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

/* read a guess sequence fron stdin and store the values in arr */
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

// The ARM assembler version of the matching fct
int *matches(int *val1, int *val2)
{
  int exact, approx = 0;
  // int accurate, exactfound, found = 0;
  int *accuracy = (int *)malloc(2 * sizeof(int));
  asm(
      "start:\n"
      "\tMOV R0, #0\n"
      "\tMOV R1, #0\n"
      "\tMOV R2, #0\n"
      "\tMOV R4, #0\n"

      "for_loop:\n"
      "\tCMP R0, #3\n"
      "\tBGE terminate\n"

      "\tLDR R2, [%[secret],R1]\n"
      "\tLDR R3, [%[guess],R1]\n"

      "\tADD R1, R1, #4\n"
      "\tADD R0, R0, #1\n"

      "\tCMP R2, R3\n"
      "\tBNE for_loop\n"
      "\tADD R4,R4,#1\n"

      "\tB for_loop\n"

      "terminate:\n"
      "\tMOV %[exact], R4\n"

      : [exact] "=r"(exact)
      : [secret] "r"(val1), [guess] "r"(val2)
      : "r0", "r1", "r2", "r3", "r4", "r5", "r6", "cc");

  accuracy[0] = exact;
  accuracy[1] = approx;

  return accuracy;
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

int main(int argc, char **argv)
{
  int *res;
  int *res_c;
  int t, t_c, m, n;
  int *seq1, *seq2, *cpy1, *cpy2;
  struct timeval t1, t2;
  char str_in[20], str[20] = "some text";
  int verbose = 0, debug = 0, help = 0, opt_s = 0, opt_n = 0;

  // see: man 3 getopt for docu and an example of command line parsing
  { // see the CW spec for the intended meaning of these options
    int opt;
    while ((opt = getopt(argc, argv, "hvs:n:")) != -1)
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
      case 's':
        opt_s = atoi(optarg);
        break;
      case 'n':
        opt_n = atoi(optarg);
        break;
      default: /* '?' */
        fprintf(stderr, "Usage: %s [-h] [-v] [-s <seed>] [-n <no. of iterations>]  \n", argv[0]);
        exit(EXIT_FAILURE);
      }
    }
  }

  seq1 = (int *)malloc(seqlen * sizeof(int));
  seq2 = (int *)malloc(seqlen * sizeof(int));
  cpy1 = (int *)malloc(seqlen * sizeof(int));
  cpy2 = (int *)malloc(seqlen * sizeof(int));

  if (argc > optind + 1)
  {
    strcpy(str_in, argv[optind]);
    m = atoi(str_in);
    strcpy(str_in, argv[optind + 1]);
    n = atoi(str_in);
    fprintf(stderr, "Testing matches function with sequences %d and %d\n", m, n);
  }
  else
  {
    int i, j, n = 10, oks = 0, tot = 0; // number of test cases
    int *res;
    int *res_c;
    fprintf(stderr, "Running tests of matches function with %d pairs of random input sequences ...\n", n);
    if (opt_n != 0)
      n = opt_n;
    if (opt_s != 0)
      srand(opt_s);
    else
      srand(1701);
    for (i = 0; i < n; i++)
    {
      for (j = 0; j < seqlen; j++)
      {
        seq1[j] = (rand() % seqlen + 1);
        seq2[j] = (rand() % seqlen + 1);
      }
      memcpy(cpy1, seq1, seqlen * sizeof(int));
      memcpy(cpy2, seq2, seqlen * sizeof(int));
      if (verbose)
      {
        fprintf(stderr, "Random sequences are:\n");
        showSeq(seq1);
        showSeq(seq2);
      }
      res = matches(seq1, seq2); // extern; code in matches.s
      memcpy(seq1, cpy1, seqlen * sizeof(int));
      memcpy(seq2, cpy2, seqlen * sizeof(int));
      res_c = countMatches(seq1, seq2); // local C function
      if (debug)
      {
        fprintf(stdout, "DBG: sequences after matching:\n");
        showSeq(seq1);
        showSeq(seq2);
      }
      fprintf(stdout, "Matches (encoded) (in C):   %d %d\n", res_c[0], res_c[1]);
      fprintf(stdout, "Matches (encoded) (in Asm): %d %d\n", res[0], res[1]);
      memcpy(seq1, cpy1, seqlen * sizeof(int));
      memcpy(seq2, cpy2, seqlen * sizeof(int));
      showMatches(res_c, seq1, seq2, 0);
      showMatches(res, seq1, seq2, 0);
      tot++;
      if (res[0] == res_c[0] && res[1] == res_c[1])
      {
        fprintf(stdout, "__ result OK\n");
        oks++;
      }
      else
      {
        fprintf(stdout, "** result WRONG\n");
      }
    }
    fprintf(stderr, "%d out of %d tests OK\n", oks, tot);
    exit(oks == tot ? 0 : 1);
  }

  readSeq(seq1, m);
  readSeq(seq2, n);

  memcpy(cpy1, seq1, seqlen * sizeof(int));
  memcpy(cpy2, seq2, seqlen * sizeof(int));
  memcpy(seq1, cpy1, seqlen * sizeof(int));
  memcpy(seq2, cpy2, seqlen * sizeof(int));

  gettimeofday(&t1, NULL);
  res_c = countMatches(seq1, seq2); // local C function
  gettimeofday(&t2, NULL);
  // d = difftime(t1,t2);
  if (t2.tv_usec < t1.tv_usec) // Counter wrapped
    t_c = (1000000 + t2.tv_usec) - t1.tv_usec;
  else
    t_c = t2.tv_usec - t1.tv_usec;

  if (debug)
  {
    fprintf(stdout, "DBG: sequences after matching:\n");
    showSeq(seq1);
    showSeq(seq2);
  }
  memcpy(seq1, cpy1, seqlen * sizeof(int));
  memcpy(seq2, cpy2, seqlen * sizeof(int));

  gettimeofday(&t1, NULL);
  res = matches(seq1, seq2); // extern; code in hamming4.s
  gettimeofday(&t2, NULL);
  // d = difftime(t1,t2);
  if (t2.tv_usec < t1.tv_usec) // Counter wrapped
    t = (1000000 + t2.tv_usec) - t1.tv_usec;
  else
    t = t2.tv_usec - t1.tv_usec;

  if (debug)
  {
    fprintf(stdout, "DBG: sequences after matching:\n");
    showSeq(seq1);
    showSeq(seq2);
  }

  memcpy(seq1, cpy1, seqlen * sizeof(int));
  memcpy(seq2, cpy2, seqlen * sizeof(int));
  showMatches(res_c, seq1, seq2, 0);
  showMatches(res, seq1, seq2, 0);

  if (res == res_c)
  {
    fprintf(stdout, "__ result OK\n");
  }
  else
  {
    fprintf(stdout, "** result WRONG\n");
  }
  fprintf(stderr, "C   version:\t\tresult=%d %d (elapsed time: %dms)\n", res_c[0], res_c[1], t_c);
  fprintf(stderr, "Asm version:\t\tresult=%d %d (elapsed time: %dms)\n", res[0], res[1], t);

  return 0;
}
