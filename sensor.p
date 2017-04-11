// PRU Program for embedded systems project

.origin 0	// start of program in PRU memory

#define GPIO0 0x44e07000         // GPIO Bank 0, See the AM335x TRM
#define GPIO_DATAIN       0x138  // to read the register data read from GPIO pins
#define GPIO_MASK 		  0x3C   // [11100] for GPIO 2, 3, 4 and 5

// Enable the OCP master port
LBCO    r0, C4, 4, 4     // load SYSCFG reg into r0 (use c4 const addr)
CLR     r0, r0, 4        // clear bit 4 (STANDBY_INIT)
SBCO    r0, C4, 4, 4     // store the modified r0 back at the load addr

// LOAD DELAY TIME FROM LINUX
// MOV r5, 0; 		// Number of delays (value read from Linux)
// MOV r2, 0x0; 	// Start of PRU0 RAM (where parameters from linux is read)
// LBBO    r5, r2, 0, 4     // value at address r2 into r5 w/ offset 0 and length 4

// LOAD BLOCK SIZE FROM LINUX
MOV r7, 0; 		// Block size (value read from Linux)
MOV r2, 0x2000;    // Start of PRU0 RAM (where parameters from linux is read)
LBBO    r7, r2, 0, 4     // value at address r2 into r5 w/ offset=4bytes and length=4bytes

// Interrupt
MOV r11, 0; 		// number of blocks written (for interrupt)
MOV r12, 0x10000	// STATIC - address of interrupt
SBBO r11,r12,0,4 	// store r11 (initial block count of 0) at address r12 (0x10000)

// Data
MOV r1, 0; 		// n is in register r1
MOV r2, 0x0;    // r2 contains the address A where to store n

// RAM limit
MOV r4, 0x3FFF

MAINLOOP: //while(1)
	MOV r3, r7 	// Size of the Block (k)

FORLOOP: // while(k > 0)

	LOOKFORNEWDATA:

		MOV r10, r9; 	// set old count reading (for checking changed value)

		MOV		r8, GPIO0 | GPIO_DATAIN      // load read addr for DATAIN
		LBBO	r9, r8, 0, 4     		// Load the value at r8 into r9

		AND     r9, r9, GPIO_MASK 	// r9 = r9 & 
		LSR		r9, r9, 2			// r9 = r9 >> 2

		// check if changed
		QBEQ	MAINLOOP,	r9, r10 	// if count = old_count, keep checking


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













