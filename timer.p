// PRU Program for embedded systems project

.origin 0	// start of program in PRU memory


#define INS_PER_US   200         // 5ns per instruction
#define INS_PER_DELAY_LOOP 2     // two instructions per delay loop

// Timer slot
MOV r11, 0; 		// initial timestamp
MOV r12, 0x10004	// address of timestamp
SBBO r11,r12,0,4 	// store r11 (initial ts) at address r12 (0x10004)


MAINLOOP: //while(1)

	SBBO r11,r12,0,4 		// store r11 (ts) at address r12 (A)
	ADD r11,r11,1 		// ts = ts + 1
	
    // Here is the delay
	MOV r6, 198
	// each DELAY is 2 microseconds (with r6=198 and 4 additional instructions)
	DELAY:
		SUB r6,r6, 1
		QBNE DELAY, r6,0 	// delay

	JMP MAINLOOP 		// while(1)