@ This ARM Assembler code should implement a matching function, for use in the MasterMind program, as
@ described in the CW2 specification. It should produce as output 2 numbers, the first for the
@ exact matches (peg of right colour and in right position) and approximate matches (peg of right
@ color but not in right position). Make sure to count each peg just once!
	
@ Example (first sequence is secret, second sequence is guess):
@ 1 2 1
@ 3 1 3 ==> 0 1
@ You can return the result as a pointer to two numbers, or two values
@ encoded within one number
@
@ -----------------------------------------------------------------------------

.text
@ this is the matching fct that should be called from the C part of the CW	
.global         matches
@ use the name `main` here, for standalone testing of the assembler code
@ when integrating this code into `master-mind.c`, choose a different name
@ otw there will be a clash with the main function in the C code
.global         main1
main1: 
	LDR  R2, =secret	@ pointer to secret sequence
	LDR  R3, =guess		@ pointer to guess sequence

	@ you probably need to initialise more values here
	MOV R0, #0 /* i = 0 */
	MOV R1, #0 /* store exact matches */
	MOV R4, #0 /* store accurate matches */
	MOV R6, #0 /* acts as variable found */
	MOV R7, #0 /* j = 0 */

@ ... COMPLETE THE CODE BY ADDING YOUR CODE HERE, you should use sub-routines to structure your code

exit:	@MOV	 R0, R4		@ load result to output register
	MOV 	 R7, #1		@ load system call code
	SWI 	 0		@ return this value

@ -----------------------------------------------------------------------------
@ sub-routines

@ this is the matching fct that should be callable from C	
@ matches:
	@ for_loop

@ for_loop:
@ CMP R0, #3 /* i < seqlen */
@ 	@ BEQ if_loop
@ 	B exit

@ if_loop:	
@ 	CMP R2, R3 /* comparing seq1[i] and seq2[i] */
@ 	BNE else
@ 	B exit

	@ ADD R1, #1 /* incrementing number of exact matches */
	@ MOV R5, R2 /* store val of seq1[i] in R5, R5 => exact*/
	//B nested_if

	@ ADD R0, #1 /*incrementing i */
	@ ADD R2, #4 /* incrementing seq1[i] */
	@ ADD R3, #4 /* incrementing seq2[i] */
	@ B exit
	
@ nested_if:
@ 	CMP R6, R3
@ 	BEQ decrement_approx
@ 	B matches

@ decrement_approx:
@ 	SUB R4, #1
@ 	B matches


@ else:
@ 	B exit

@ for_loop_2:
@ 	CMP R7, #3
@ 	B exit

@ if_loop2:
@ 	SUB R7, R2, R3
@ 	CMP R7, #0
@ 	BEQ case1
@ 	B matches

@ case1:
@ 	CMP R6, R3
@ 	BNE case2
@ 	B matches

@ case2:
@ 	CMP R5, R3
@ 	BNE case3
@ 	B matches

@ case3:
@ 	MOV R6, R3
@ 	ADD R4, #1
@ 	B matches
@ 	B exit

	@ Input: R0, R1 ... ptr to int arrays to match ; Output: R0 ... exact matches (10s) and approx matches (1s) of base COLORS
	@ COMPLETE THE CODE HERE

@ show the sequence in R0, use a call to printf in libc to do the printing, a useful function when debugging 
@ showseq: 			@ Input: R0 = pointer to a sequence of 3 int values to show
	@ COMPLETE THE CODE HERE (OPTIONAL)
	
	
@ @ =============================================================================

.data

@ constants about the basic setup of the game: length of sequence and number of colors	
.equ LEN, 3
.equ COL, 3
.equ NAN1, 8
.equ NAN2, 9

@ a format string for printf that can be used in showseq
f4str: .asciz "Seq:    %d %d %d\n"

@ a memory location, initialised as 0, you may need this in the matching fct
n: .word 0x00
	
@ INPUT DATA for the matching function
.align 4
secret: .word 1 
	.word 2 
	.word 1 

.align 4
guess:	.word 3 
	.word 1 
	.word 3 

@ Not strictly necessary, but can be used to test the result	
@ Expect Answer: 0 1
.align 4
expect: .byte 0
	.byte 1

.align 4
secret1: .word 1 
	 .word 2 
	 .word 3 

.align 4
guess1:	.word 1 
	.word 1 
	.word 2 

@ Not strictly necessary, but can be used to test the result	
@ Expect Answer: 1 1
.align 4
expect1: .byte 1
	 .byte 1

.align 4
secret2: .word 2 
	 .word 3
	 .word 2 

.align 4
guess2:	.word 3 
	.word 3 
	.word 1 

@ Not strictly necessary, but can be used to test the result	
@ Expect Answer: 1 0
.align 4
expect2: .byte 1
	 .byte 0

