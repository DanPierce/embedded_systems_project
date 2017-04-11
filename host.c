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
   printf("\n USAGE: host <Block Size> <Test Duration (min)>\n");
   printf("\n\tExample: host 256 12\n");
   exit(1);
}

int main (int argc, char *argv[])
{
   /* Set a high priority to the current process */
   setpriority(PRIO_PROCESS, 0, -20);

   int data_rate_kHz,block_size,operation_time_minutes; // parameters

   /* Load Arguments   */
   if ( (argc < 3) )
      usage();

   // data_rate_kHz = atoi(argv[1]);            // rate at which data is transferred
   block_size = atoi(argv[1]);               // block size of data passing
   operation_time_minutes = atoi(argv[2]);   // test duration in minutes

   /* Print parameters to screen */
   printf("\nOperation Parameters:\n\tData Rate: %d (kHz)\n\tBlock Size: %d\n\tTest Duration: %d (min)\n",
                           data_rate_kHz,block_size,operation_time_minutes);

   int numSamples = 0xFFFF; //data_rate_kHz*1000*operation_time_minutes*60; // maximum sample count value

   printf("\tNumber of samples to read: %d\n\n",numSamples);

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
   const int nibbleLimit = 0xF;    // limit of the chunks used to store data between writing to file

   if (numSamples<chunkLimit)
      numSamples=chunkLimit;

   /* Initialize Loop Variables */
   int k;                  // block count
   int numBlocksRead = 0;    // for interrupt
   int i = 0;                // loop index of RAM

   printf("\nWaiting on first value...\n");
   while( *ptr_0 == 0);        // wait for first value to be written

   int n0 = (*(ptr_0+i));     // actual value of n (read memory for initial value)
   int n = n0;
   int nexp = n0;             // expected value of n
   
   int cnt = 0;               // total number of samples read

   int datChunk[chunkLimit];  // initialize array for temporary storage of data

   /* Open data file */
   FILE *fp;
   fp = fopen("data.txt","w");

   /* Timing */
   time_t start_seconds,end_seconds; // time at start of test, time at end of test
   time(&start_seconds);   // Note: only precise to the second

   printf("\nn0 = %d\n\n",n0);

   // while(1){
   //    int j=0;
   //    while (j<10){
   //       printf("\nptr_0 + %d = %d",j,*(ptr_0+j));
   //       j++;
   //    }
   //    printf("\n");
   //    sleep(1);
   // }


   /* Start Test */
   do{
      while(numBlocksRead<(*ptr_1)){ // INTERRUPT
         
         k=block_size;
         while(k>0){
            i=cnt&ramLimit; // impose RAM limit
            nexp = nexp&nibbleLimit;

            n=(*(ptr_0+i)); // read memory
            
            datChunk[nexp&chunkLimit] = n; // Save data chunk for later writing to file

            cnt++;
            k--;
         }

         // write chunk to data file
         if ((nexp&chunkLimit)==0)
            fwrite(datChunk,sizeof(int),chunkLimit, fp);

         printf("i = %d\n",i);
         printf("n = %d\n",n);
         printf("nexp = %d\n",nexp);
         printf("cnt = %d\n\n",cnt);

         nexp+=block_size; // increment expected n by block size

         numBlocksRead++;
      }
   }while(( (nexp-n0)<numSamples )&( n==(nexp-1) ));

   /* Timing */
   time(&end_seconds); // Note: only precise to the second
   double seconds = difftime(end_seconds,start_seconds); // duration of test

   /* Close data file */
   fclose(fp);


   printf("Actual number of samples: %d\n",n-n0);

   /* Disable PRU and close memory mappings */
   prussdrv_pru_disable(PRU_NUM);
   prussdrv_exit ();

   FILE *fpr;
   fpr = fopen("data.txt","r");
   int num;
   while(fread(&num, sizeof(int), 1, fpr)){
      printf("here: %d\n",num);
   }

   fclose(fpr);



   /* Print results */
   if (n==(nexp-1)){
      printf("\n[PASS]\n");
      return 1;
   }else{
      printf("\n[FAIL]\n");
      return 0;
   }
}
//read memory
