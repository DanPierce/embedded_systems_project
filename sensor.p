// PRU Program for embedded systems project

.origin 0	// start of program in PRU memory

#define GPIO0 0x44e07000         // GPIO Bank 0, See the AM335x TRM
#define GPIO_DATAIN       0x138  // to read the register data read from GPIO pins
#define GPIO_MASK 		  0x3C   // [11100] for GPIO 2, 3, 4 and 5

// Enable the OCP master port
LBCO    r0, C4, 4, 4     // load SYSCFG reg into r0 (use c4 const addr)
CLR     r0, r0, 4        // clear bit 4 (STANDBY_INIT)
SBCO    r0, C4, 4, 4     // store the modified r0 back at the load addr

// LOAD BLOCK SIZE FROM LINUX
MOV r7, 0; 		// Block size (value read from Linux)
MOV r2, 0x2000;    // Start of PRU0 RAM (where parameters from linux is read)
LBBO    r7, r2, 0, 4     // value at address r2 into r7 w/ offset=4bytes and length=4bytes

// Interrupt
MOV r11, 0; 		// number of blocks written (for interrupt)
MOV r12, 0x10000	// STATIC - address of interrupt
SBBO r11,r12,0,4 	// store r11 (initial block count of 0) at address r12 (0x10000)

// Address for passing data
SBBO r11,r2,0,4 		// store initial data value 0 at address r2 (A)
MOV r2, 0x0;    // r2 contains the address A where to store n

// Time stamp
MOV r15, 0; 		 // timestamp value
MOV r16, 0x10004;    // timestamp address (shared memory)
LBBO    r15, r16, 0, 4     // value at address r16 into r15 w/ offset=4bytes and length=4bytes

// Data + timestamp
MOV 	r17, 0		// where timestamp is first nibble (4 least significant bits)


// RAM limit
MOV r4, 0x3FFF

// 4 bit limit
MOV r8, 0xF

MAINLOOP: //while(1)
	MOV r3, r7 	// Size of the Block (k)

	FORLOOP: // while(k > 0)

		WAITFORNEWDATA:
			MOV r10, r9; 	// set old count reading (for checking changed value)

			MOV		r8, GPIO0 | GPIO_DATAIN      // load read addr for DATAIN
			LBBO	r9, r8, 0, 4     		// Load the value at r8 into r9

			AND     r9, r9, GPIO_MASK 	// r9 = r9 & 
			LSR		r9, r9, 2			// r9 = r9 >> 2

			// check if changed
			QBEQ	WAITFORNEWDATA,	r9, r10 	// if count = old_count, keep checking

		LBBO    r15, r16, 0, 4     // value at address r16 into r15 w/ offset=4bytes and length=4bytes
		LSL		r15, r15, 4			// r15 = r15 << 4
		OR 		r17, r15, r9		// r17 = r15 | r9

		SBBO r17,r2,0,4 		// store r17 (stamped data) at address r2 (A)
		ADD r2,r2,4 		// A = A + 4
		AND r2,r2,r4 		// reset A based on RAM limit
		SUB r3,r3,1			// loop counter
		QBNE FORLOOP, r3, 0	// for loop end

		// Send block count interrupt
		ADD r11,r11,1			// Increment block count
		SBBO r11,r12,0,4 		// store r11 (block count) at address r12 (0x0)

	JMP MAINLOOP 		// while(1)








