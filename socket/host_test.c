/** Dan Pierce
*   Embedded Systems Development Project - Phase I
*   2017.03.09 
*/

#include <stdio.h>
#include <stdlib.h>

int main (void)
{
   
   FILE *fp = fopen("data_file.txt", "w"); 

   int i=0;
   while(i<5){ 
      fprintf(fp, "%d\n",2*i);
      // sleep(1.0);
      i++;
   }

   fclose(fp);

   // FILE *fpr = fopen("data_file.txt", "r"); 

   // int current_data;
   
   // while (fscanf(fpr,"%d\n",&current_data) != EOF){
   //    printf("%d\n",current_data);
   // }

   // fclose(fpr);


   return 0;
}
