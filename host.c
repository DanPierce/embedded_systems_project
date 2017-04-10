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
   setpriority(PRIO_PROCESS, 0, -20);

   int data_rate_kHz,block_size,operation_time_minutes;

   /* Load Arguments   */
   if ( (argc < 4) )
        usage();

   data_rate_kHz = atoi(argv[1]);
   block_size = atoi(argv[2]);
   operation_time_minutes = atoi(argv[3]);
   
   printf("\nOperation Parameters:\n\tData Rate: %d (kHz)\n\tBlock Size: %d\n\tTest Duration: %d (min)\n",
                           data_rate_kHz,block_size,operation_time_minutes);

   int number_100ms_delays = (int) (100.0 * ((double)block_size)/((double)data_rate_kHz));

   printf("\t---------------------------------------\n\tNumber of PRU 100ms Delays = %d (us) \n",number_100ms_delays);

   // int maxCountVal = 720e6;
   int maxCountVal = data_rate_kHz*1000*operation_time_minutes*60;

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
   prussdrv_pru_write_memory(PRUSS0_PRU0_DATARAM, 0, &number_100ms_delays, 4);
   prussdrv_pru_write_memory(PRUSS0_PRU1_DATARAM, 0, &block_size, 4);

   /* Map PRU's INTC */
   prussdrv_pruintc_init(&pruss_intc_initdata);

   /* Load and execute binary on PRU */
   prussdrv_exec_program (PRU_NUM, "./sensor.bin");

   /* Memory mapping */
   int * ptr_0; // points to global memory that maps PRU memory address 0x0 (first address of PRU0)
   prussdrv_map_prumem(PRUSS0_PRU0_DATARAM, (void **) &ptr_0);

   int * ptr_1; // points to global memory that maps first address of PRU1
   ptr_1 = ptr_0 + (0x10000>>2);

   const int ramLimit = (0x3FFF>>2);
   // const int chunkLimit = ramLimit;
   const int chunkLimit = block_size-1;

   /* Initialize Loop Variables */
   int numBlocksRead=0;  // For interrupt
   int k;        // Block count
   int nexp=0;   // expected value of n
   int n=0;      // actual value of n
   int i=0;		  // Loop index of RAM

   int datChunk[chunkLimit];

   // Write data to file
   FILE *fp;
   fp = fopen("data.txt","w");

   /* Timing */
   time_t start_seconds,end_seconds;
   time(&start_seconds);

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
         
         if ((nexp&chunkLimit)==0)
            fwrite(datChunk,sizeof(int),chunkLimit, fp);

         numBlocksRead++;
      }
   }while((nexp<maxCountVal)&(n==(nexp-1)));

   /* Timing */
   time(&end_seconds);
   double seconds = difftime(end_seconds,start_seconds);

   fclose(fp);


   // Print results
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

   // ------ Save all results in one file 
   FILE *f_r;
   f_r = fopen("results/results_full_4095.txt","a");

   fprintf(f_r,"%d,%d,%d,%d,%d,%d,%d\n",data_rate_kHz,block_size,operation_time_minutes,numBlocksRead,nexp,maxCountVal,n);

   fclose(f_r);

   /* Disable PRU and close memory mappings */
   prussdrv_pru_disable(PRU_NUM);
   prussdrv_exit ();
}
//read memory
