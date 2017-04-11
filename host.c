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
   printf("\n USAGE: host <Block Size> <Test Duration (min)> \n");
   printf("\n\tExample: host 256 12\n");
   exit(1);
}

int main (int argc, char *argv[])
{
   /* Set a high priority to the current process */
   setpriority(PRIO_PROCESS, 0, -20);

   int blockSize,operationTimeMinutes; // parameters

   /* Load Arguments   */
   if ( (argc < 4) )
      usage();

   blockSize = atoi(argv[1]);               // block size of data passing
   operationTimeMinutes = atoi(argv[2]);   // test duration in minutes

   printf("\nOperation Parameters:\n\tBlock Size: %d\n\tTest Duration: %d (min)\n",blockSize,operationTimeMinutes);

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
   // prussdrv_pru_write_memory(PRUSS0_PRU0_DATARAM, 0, &number_us_delays, 4);
   prussdrv_pru_write_memory(PRUSS0_PRU1_DATARAM, 0, &blockSize, 4);

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
   // const int chunkLimit = ramLimit;    // limit of the chunks used to store data between writing to file
   const int chunkLimit = blockSize-1;    // limit of the chunks used to store data between writing to file
   const int chunkSize = chunkLimit+1;    // limit of the chunks used to store data between writing to file

   /* Initialize Loop Variables */
   int k;                  // block count
   int numBlocksRead = 0;    // for interrupt
   int i = 0;                // loop index of RAM
   int n = 0;
   
   int cnt = 0;               // total number of samples read

   int datChunk[chunkSize];  // initialize array for temporary storage of data

   /* Open data file */
   FILE *fp;
   fp = fopen("data.txt","w");

   /* Timing */
   time_t start_seconds,end_seconds; // time at start of test, time at end of test
   time(&start_seconds);   // Note: only precise to the second

   // printf("numBlocksRead = %d\n",numBlocksRead);
   // printf("(*ptr_1) = %d\n",(*ptr_1));

   /* Start Test */
   do{
      while( numBlocksRead<(*ptr_1) ){ // INTERRUPT
         
         k=blockSize;
         while(k>0){
            i=cnt&ramLimit; // impose RAM limit

            n=(*(ptr_0+i)); // read memory
            
            datChunk[cnt&chunkLimit] = n; // Save data chunk for later writing to file

            // printf("i = %d\n",i);
            // printf("n = %d\n",n);
            // printf("cnt = %d\n\n",cnt);

            cnt++;
            k--;
         }

         // write chunk to data file
         if ( (cnt&chunkLimit)==0 )
            fwrite(datChunk,sizeof(int),chunkSize, fp);

         numBlocksRead++;
      }
      time(&end_seconds); // Note: only precise to the second
   }while( difftime(end_seconds,start_seconds) < operationTimeMinutes );

   /* Timing */
   double seconds = difftime(end_seconds,start_seconds); // duration of test

   /* Close data file */ 
   fclose(fp);

   int num;
   FILE *fpr;
   fpr = fopen("data.txt","r");
   while(fread(&num, sizeof(int), 1, fpr))
      printf("%d\n",num);
   fclose(fpr);

   /* Disable PRU and close memory mappings */
   prussdrv_pru_disable(PRU_NUM);
   prussdrv_exit ();

   return 0;

}
//read memory
