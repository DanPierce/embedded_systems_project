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
#include <sys/resource.h>

#define PRU_NUM	0   // using PRU0 for these examples

void usage(void)
{
   printf("\nUSAGE: host <Data Rate (kHz)> <Block Size> <Test Duration (min)>\n");
   printf("\n\tExample: host 10 256 12\n");
   exit(1);
}

int main (int argc, char *argv[])
{
   /* Set a high priority to the current process */
   setpriority(PRIO_PROCESS, 0, -20);

   int data_rate_kHz,block_size,operation_time_minutes; // parameters

   /* Load Arguments   */
   if ( (argc < 4) )
        usage();

   data_rate_kHz = atoi(argv[1]);            // rate at which data is transferred
   block_size = atoi(argv[2]);               // block size of data passing
   operation_time_minutes = atoi(argv[3]);   // test duration in minutes
   
   /* Print parameters to screen */
   printf("\nOperation Parameters:\n\tData Rate: %d (kHz)\n\tBlock Size: %d\n\tTest Duration: %d (min)\n",
                           data_rate_kHz,block_size,operation_time_minutes);

   int number_us_delays = (int) (1000.0 * ((double)block_size)/((double)data_rate_kHz)); // number of 1 microsecond delays of the PRU to acheive desired rate

   printf("\t---------------------------------------\n\tNumber of PRU microsecond Delays = %d (us) \n",number_us_delays);

   int maxCountVal = data_rate_kHz*1000*operation_time_minutes*60; // maximum sample count value

   printf("\tMax Count Value: %d\n\n",maxCountVal);

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
   prussdrv_pru_write_memory(PRUSS0_PRU0_DATARAM, 0, &number_us_delays, 4);
   prussdrv_pru_write_memory(PRUSS0_PRU1_DATARAM, 0, &block_size, 4);

   /* Map PRU's INTC */
   prussdrv_pruintc_init(&pruss_intc_initdata);

   /* Load and execute binary on PRU */
   prussdrv_exec_program (PRU_NUM, "./sensor.bin");

   /* Memory mapping */
   int * ptr_0; // points to global memory that maps PRU memory address 0x0 (first address of PRU0)
   prussdrv_map_prumem(PRUSS0_PRU0_DATARAM, (void **) &ptr_0);

   int * ptr_1; // points to global memory that maps shared memory
   ptr_1 = ptr_0 + (0x10000>>2);

   const int ramLimit = (0x3FFF>>2);   // limit of PRU0 and PRU1 RAM (16KB)
   const int chunkLimit = ramLimit;    // limit of the chunks used to store data between writing to file

   /* Initialize Loop Variables */
   int numBlocksRead=0;    // for interrupt
   int k;                  // block count
   int nexp=0;             // expected value of n
   int n=0;                // actual value of n
   int i=0;		            // loop index of RAM

   int datChunk[chunkLimit];  // initialize array for temporary storage of data

   /* Open data file */
   FILE *fp;
   fp = fopen("data.txt","w");

   /* Timing */
   time_t start_seconds,end_seconds; // time at start of test, time at end of test
   time(&start_seconds);   // Note: only precise to the second

   /* Start Test */
   do{
      while(numBlocksRead<(*ptr_1)){ // INTERRUPT
         k=block_size;
         while(k>0){
            i=nexp&ramLimit; // impose RAM limit
            n=(*(ptr_0+i)); // read memory

            datChunk[nexp&chunkLimit] = n; // Save data chunk for later writing to file

            nexp++; // increment expected n by block size
            k--;
         }
         
         // write chunk to data file
         if (((nexp-1)&chunkLimit)==0)
            fwrite(datChunk,sizeof(int),chunkLimit, fp);

         numBlocksRead++;
      }
   }while((nexp<maxCountVal)&(n==(nexp-1)));

   /* Timing */
   time(&end_seconds); // Note: only precise to the second
   double seconds = difftime(end_seconds,start_seconds); // duration of test

   /* Close data file */
   fclose(fp);


   /* Print results */
   if (n==(nexp-1)){
      printf("\n[PASS]\n");
   }else{
      printf("\n[FAIL]\n");
   }

   printf("\nTest lasted for %f seconds\n",seconds);
   printf("Expected rate = %d [kHz]\n",data_rate_kHz);
   printf("Actual rate = %f [kHz]\n",(1.0/1000.0)*((double)n)/seconds);
   printf("Data written: %d blocks (%d measurements)\n",numBlocksRead,nexp);
   printf("n = %d\n",n);

   /* Save results to appended file (for averaging) */
   FILE *fp2;
   fp2 = fopen("results/results_full.txt","a");
   fprintf(fp2,"%d,%d,%d,%d,%d,%d,%d\n",data_rate_kHz,block_size,operation_time_minutes,numBlocksRead,nexp,maxCountVal,n);
   fclose(fp2);

   /* Disable PRU and close memory mappings */
   prussdrv_pru_disable(PRU_NUM);
   prussdrv_exit ();
}
//read memory
