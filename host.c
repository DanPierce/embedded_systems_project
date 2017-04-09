/** Dan Pierce
*   Embedded Systems Development Project - Phase I
*   2017.03.09
*/

#include <stdio.h>
#include <stdlib.h>
#include <prussdrv.h>
#include <pruss_intc_mapping.h>
#include <pthread.h>
#include <time.h>

#define PRU_NUM	0   // using PRU0 for these examples

void usage(void)
{
   printf("\nUSAGE: host <PRU Delay (ms)> <Block Size> <Operation Time (min)> <Write To File (0/1)> \n");
   printf("\n\tExample: host 10 256 12 1 \n");
   exit(1);
}

int main (int argc, char *argv[])
{

   int delay_ms,block_size,operation_time_minutes,write_to_file; //delay in ms
   /* Load Arguments   */
   if ( (argc < 5) )
        usage();

   delay_ms = atoi(argv[1]);
   block_size = atoi(argv[2]);
   operation_time_minutes = atoi(argv[3]);
   write_to_file = atoi(argv[4]);

   printf("\nOperation Parameters:\n\tPRU Delay (ms): %d\n\tBlock Size: %d\n\tOperation Time (min): %d\n\tWrite To File: %d\n",
                           delay_ms,block_size,operation_time_minutes,write_to_file);

   double data_rate_kHz = ((double)block_size)/((double)delay_ms);
   printf("\t-----------------\n\tData rate = %f [kHz]\n\n",data_rate_kHz);

   /* Initialize structure used by prussdrv_pruintc_intc   */
   /* PRUSS_INTC_INITDATA is found in pruss_intc_mapping.h */
   tpruss_intc_initdata pruss_intc_initdata = PRUSS_INTC_INITDATA;

   /* Allocate and initialize memory */
   prussdrv_init ();
   if(prussdrv_open (PRU_EVTOUT_0)){
      printf("Cannot open PRU");
      exit(1);
   }

   /* Write parameters to PRU   */
   prussdrv_pru_write_memory(PRUSS0_PRU0_DATARAM, 0, &delay_ms, 4);
   prussdrv_pru_write_memory(PRUSS0_PRU1_DATARAM, 0, &block_size, 4);

   /* Map PRU's INTC */
   prussdrv_pruintc_init(&pruss_intc_initdata);

   /* Load and execute binary on PRU */
   prussdrv_exec_program (PRU_NUM, "./sensor.bin");

   /* Memory mapping */
   unsigned int * ptr_0; // points to global memory that maps PRU memory address 0x0 (first address of PRU0)
   prussdrv_map_prumem(PRUSS0_PRU0_DATARAM, (void **) &ptr_0);

   unsigned int * ptr_1; // points to global memory that maps first address of PRU1
   ptr_1 = ptr_0 + (0x10000>>2);

   const unsigned int ramLimit = (0x3FFF>>2);

   /* Timing */
   time_t start_seconds;
   time(&start_seconds);
   unsigned int num_iterations = (unsigned int) ( (data_rate_kHz*1000.0)*operation_time_minutes*60.0 );

   /* Initialize Loop Variables */
   unsigned int numBlocksRead=0;  // For interrupt
   unsigned int k;        // Block count
   unsigned int nexp=0;   // expected value of n
   unsigned int n=0;      // actual value of n
   unsigned int i=0;		  // Loop index of RAM

   /* Start Test */
   do{
      if(numBlocksRead<(*ptr_1)){ // INTERRUPT
         k=block_size;
         while(k>0){
            i=nexp&ramLimit; // impose RAM limit
            n=(*(ptr_0+i)); // read memory

            nexp++; // increment expected n by block size
            k--;
            printf("i = %d\tn =  %d\tnexp=%d\n",i,n,nexp-1);
         }
         numBlocksRead++;
         printf("Number of blocks read: %d\n",numBlocksRead);
      }
   // }while(n==(nexp-1));
   }while((nexp<num_iterations)&(n==(nexp-1)));


   time_t end_seconds;
   time(&end_seconds);


   double seconds = difftime(end_seconds,start_seconds);

   printf("diff time =  %f [sec]\n",seconds);

   printf("actual rate =  %f [kHz]\n",(1.0/1000.0)*((double)n)/seconds);
   

   printf("n =  %d\tnexp=%d\n\n",n,nexp-1);

   /* Disable PRU and close memory mappings */
   prussdrv_pru_disable(PRU_NUM);
   prussdrv_exit ();
}
//read memory
