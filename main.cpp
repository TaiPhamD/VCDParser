#include <fstream>
#include <iostream>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define sampling_rate 500000000;
#define minSymbol 33 // maps to !
#define maxSymbol 47 // maps to /

int main(int argc, char *argv[]) {
  printf("Opening file:  %s\n", argv[1]);
  time_t startTs = clock();

  // Input stream
  FILE *fp = fopen(argv[1], "r");
  if (fp == NULL)
    exit(EXIT_FAILURE);

  char *line = NULL; // use to read in text input line
  size_t len = 0;
  int i = 0;
  int j = 0;
  int count = 0;
  char data = 0;
  int index = 0;
  bool doneVarRead = false; 
  bool foundVar = false;
  int endTime = 0;
  double endTimeDouble = 0.0;
  char header;
  char inputSymbol[2];
  int symbolIdx;
  long int currFrame; //most recent timestamp
  long int symbolPrevFrame[256]; //previous timestamp for each symbol
  // symbol can have different previous array
  std::ofstream *filePtr[256];
  for (i = 0; i < 256; i++) {
    filePtr[i] = NULL;
    symbolPrevFrame[i] = 0;
  }
  while ((getline(&line, &len, fp)) != -1) {
    // using printf() in all tests for consistency
    // printf("%s", line);
    header = line[0];
    switch (header) {
    case '$':
      if (memcmp(line, "$var wire", 8) == 0) {
        // We found a "var wire ..." line
        // advance read line by 12 characters to get rid of "var wire X "
        line = line + 12;
        count = strlen(line);
        // get rid of the "$end" text string so we only get the variable name
        line[count - 6] = 0;
        // grab the input symbol to be used as part of the output file name
        inputSymbol[0] = line[0];
        inputSymbol[1] = 0;
        symbolIdx = (int)inputSymbol[0];
        // create new output stream for each found symbol
        std::string fileName = "output_" + std::string(inputSymbol) + ".txt";
        std::cout << "index for: " << inputSymbol << " is " << symbolIdx;
        std::cout << "creating output file: " << fileName << "\n";
        filePtr[symbolIdx] = new std::ofstream(fileName, std::ofstream::out);
        *filePtr[symbolIdx] << "TIMESTAMP," << line
                            << "\n"; // print out variable name
        foundVar = true;

      } else {
        // we did not match a $varwire but previously did
        // so now we must be done scanning for var name
        if (foundVar == true) {
          doneVarRead = true;
          // outFile << "\n";
        }
        foundVar = false;
      }
      break;
    case '#':
      sscanf(line + 1, "%ld", &currFrame);
      break;
    default:

      data = line[0];
      index = (int)line[1];
      if (symbolPrevFrame[index] == 0) {
        // printf("Initial frame: %ld\n",currFrame);
        symbolPrevFrame[index] = currFrame;
      }

      // Compute endTime
      endTimeDouble = double(currFrame - symbolPrevFrame[index]) /
                      (double)1000000000 * sampling_rate;
      endTime = round(endTimeDouble);
      if (endTime > 0)
        *filePtr[index] << endTime << "," << data << "\n";
      // store the timestamp for the scanned timeframe
      symbolPrevFrame[index] = currFrame;
    }
  }

  // close input file
  fclose(fp);

  // close output files
  for (j = minSymbol; j < maxSymbol; j++) {
    if (filePtr[j] != NULL) {
      filePtr[j]->flush();
      filePtr[j]->close();
    }
  }

  time_t endTs = clock();
  double elapsed = (double)(endTs - startTs) / CLOCKS_PER_SEC;
  printf("Total time to process is %f seconds\n", elapsed);

  return 0;
}