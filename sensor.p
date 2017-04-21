// PRU Program for embedded systems project

.origin 0	// start of program in PRU memory

// LOAD DELAY TIME FROM LINUX
MOV r5, 0; 		// Number of delays (value read from Linux)
MOV r2, 0x0; 	// Start of PRU0 RAM (where parameters from linux is read)
LBBO    r5, r2, 0, 4     // value at address r2 into r5 w/ offset 0 and length 4

// LOAD BLOCK SIZE FROM LINUX
MOV r7, 0; 		// Block size (value read from Linux)
MOV r2, 0x2000;    // Start of PRU0 RAM (where parameters from linux is read)
LBBO    r7, r2, 0, 4     // value at address r2 into r5 w/ offset=4bytes and length=4bytes

// Interrupt (number of blocks written)
MOV r11, 0; 		// number of blocks written (for interrupt)
MOV r12, 0x10000	// STATIC - address of interrupt
SBBO r11,r12,0,4 	// store r11 (initial block count of 0) at address r12 (0x10000)

// Data (count)
MOV r1, 0; 	// n is in register r1
MOV r2, 0x0;    // r2 contains the address A where to store n

// Time stamp
MOV r15, 0; 		 // timestamp value
MOV r16, 0x10004;    // timestamp address (shared memory)
LBBO    r15, r16, 0, 4     // value at address r16 into r15 w/ offset=4bytes and length=4bytes

// TEMP
MOV r18, 0x10008;    // remove me 

// RAM limit
MOV r4, 0x3FFF

MAINLOOP: //while(1)
	MOV r3, r7 	// Size of the Block (k)

FORLOOP: // while(k > 0)
	
	LBBO    r15, r16, 0, 4     // value at address r16 into r15 w/ offset=4bytes and length=4bytes
	SBBO 	r15,r18,0,4 		// store r15 at address r18 (A)


	SBBO r1,r2,0,4 		// store r1 (n) at address r2 (A)
	ADD r2,r2,4 		// A = A + 4
	AND r2,r2,r4 		// reset A based on RAM limit
	ADD r1,r1,1 		// n = n + 1
	SUB r3,r3,1			// loop counter
	QBNE FORLOOP, r3, 0	// for loop end

	// Send block count interrupt
	ADD r11,r11,1			// Increment block count
	SBBO r11,r12,0,4 		// store r11 (block count) at address r12 (0x0)
	
    // Here is the delay
	MOV r0, r5

DELAY:
	MOV r6, 100
	// each DELAYDELAY is a 1 microsecond (with r6=100)
	DELAYDELAY:
		SUB r6,r6, 1
		QBNE DELAYDELAY, r6,0 	// delay
	SUB r0,r0, 1
	QBNE DELAY, r0,0 	// delay
	JMP MAINLOOP 		// while(1)










