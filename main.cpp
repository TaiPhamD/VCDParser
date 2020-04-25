#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <time.h>

int main( int argc, char *argv[] ) {
printf( "Opening file:  %s\n", argv[1]);

time_t startTs = clock();

//Input stream
FILE *fp = fopen(argv[1], "r");
if (fp == NULL)
    exit(EXIT_FAILURE);

//Ouput stream 
std::ofstream outFile( "output.txt" , std::ofstream::out);
char* line = NULL;
size_t len = 0;
int i=0;
int j=0;
char header;
long int currFrame=0;
long int prevFrame=0;
char dataMap[256];
for(i=0;i<256;i++)
    dataMap[i]=0;

i=0;
int count=0;
char data=0;
int index=0;
while ((getline(&line, &len, fp)) != -1) {
    // using printf() in all tests for consistency
    //printf("%s", line);
    header = line[0];
    switch(header){
        case '$':
          //printf("We got dollar\n");
          break;
        case '#':
          if(currFrame> 0){
                //print data from last frame to this curret frame
           // do{
                  outFile << currFrame << ","; 
                  for(j=0;j<256;j++){
                    //print out all data map if we have data for it
                    if(dataMap[j] != 0){
                        outFile << dataMap[j] << ",";
                    }        
                  }  
                  outFile << "\n";
                  count++;            
           //   } while(count < currFrame);
              prevFrame = currFrame;
          }
          sscanf(line+1, "%ld", &currFrame);         
          if(currFrame ==0){
            //printf("Initial frame: %ld\n",currFrame);                  
            prevFrame = currFrame;
          }


          break;
        default:
          //We got data so just store it in our dataMap buffer
          //we will print out the dataMap buffer once we get to the next #
          count = prevFrame ;  
          data = line[0];
          index= (int)line[1];
          dataMap[index] = data; 

    }
}

fclose(fp);
outFile.flush();
outFile.close();  

time_t endTs = clock();

double elapsed = (double)(endTs - startTs)/CLOCKS_PER_SEC;

printf("Total time to process is %f seconds\n",elapsed);

return 0;
}