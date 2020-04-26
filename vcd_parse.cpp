#include "matrix.h"
#include "mex.h"
#include <fstream>
#include <iostream>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>


//#include <vector> MEX or windows sdk does not let me use vector class

#define sampling_rate 500000000;
#define minSymbol 33 // maps to !
#define maxSymbol 47 // maps to /

using namespace std;
void _main();

// stupid microsoft windows math.h doesn't include round function
// so we have to define it here
int round(double a) {
  int result;
  if (a - floor(a) >= 0.5)
    result = floor(a) + 1;
  else
    result = floor(a);
  return result;
};

/* VCDParse class and parsing code */
class VCDParse {
public:
  std::ifstream *MyFile;
  // constructor
  VCDParse(char *fileName) {
    std::string filestring(fileName);
    mexPrintf("opening file: %s\n", filestring);
    MyFile = new std::ifstream(filestring);
    if (MyFile->is_open() == false) {
      mexPrintf("cannot open %s\n", filestring);
    }
  }
  // destructor
  ~VCDParse() {
    // mexPrintf("calling destructor\n");
    MyFile->close();
    delete MyFile;
  }
  void Parse(int nlhs, mxArray *plhs[]) {

    std::string line;
    char *c_line;
    int i = 0;
    int maxElement = 0;
    int count = 0;
    char header;
    int symbolOffset = 0;
    while (std::getline(*MyFile, line)) {
      c_line = (char *)line.c_str();
      header = c_line[0];
      if (memcmp(line.c_str(), "$var wire", 8) == 0) {
        count++;
        if (symbolOffset == 0) {
          // Compute offset on the first run so that we can use this offset
          // to make the first symbol ascii int index to 0.
          c_line = c_line + 12;
          int varLength = strlen(c_line);
          // get rid of the "$end" text string so we only get the variable name
          c_line[varLength - 6] = 0;
          // grab the input symbol and figure out its ascii int representation
          // for offset
          symbolOffset = (int)c_line[0];
          // mexPrintf("Input symbol %c offset:%d\n", c_line[0], symbolOffset);
        }
      }
      if (memcmp(line.c_str(), "#", 1) == 0) {
        maxElement++;
      }
    }
    mexPrintf("We found %d symbols total\n", count);
    mexPrintf("total timestamp line count %d\n", maxElement);

    // start allocating memory since we assume there cant be more elements
    // then max number of timestamp counts;
    //*symbolLength = malloc(count*sizeof(int));
    int32_t *lengthBuffer = (int32_t *)mxCalloc(count, sizeof(int32_t));
    // mexMakeMemoryPersistent((void*)lengthBuffer);
    if (lengthBuffer == 0)
      mexPrintf("Error allocating lengthBuffer\n");
    int32_t *timeBuffer =
        (int32_t *)mxMalloc(count * maxElement * sizeof(int32_t));
    // mexMakeMemoryPersistent((void*)timeBuffer);
    if (timeBuffer == 0)
      mexPrintf("Error allocating timeBuffer\n");
    int32_t *dataBuffer =
        (int32_t *)mxMalloc(count * maxElement * sizeof(int32_t));
    // mexMakeMemoryPersistent((void*)dataBuffer);
    if (dataBuffer == 0)
      mexPrintf("Error allocating dataBuffer\n");

    // mexPrintf("Finish allocating memory\n");

    // reset file pointer back to beginning so we can start from beginning of
    // file; and start proccessing the VCD file!
    MyFile->clear();
    MyFile->seekg(0, ios::beg);

    int endTime = 0;
    double endTimeDouble = 0.0;
    long int currFrame;                  // most recent timestamp
    long int symbolPrevFrame[maxSymbol]; // previous timestamp for each symbol
    for (int i = 0; i < maxSymbol; i++)
      symbolPrevFrame[i] = 0;
    int value;
    int symbolIdx;
    int idx;
    int test = 0;

    while (std::getline(*MyFile, line)) {
      test++;
      c_line = (char *)line.c_str();
      header = c_line[0];
      switch (header) {
      case '$':
        // don't care about metadata since we already parsed earlier
        break;
      case '#':
        // scan the timestamp skipping the '#' symbol
        sscanf(c_line + 1, "%ld", &currFrame);
        break;
      case '\n':
        break;
      default:
        value = c_line[0];
        symbolIdx = (int)c_line[1];
        if (symbolPrevFrame[symbolIdx] == 0) {
          symbolPrevFrame[symbolIdx] = currFrame;
        }
        // Compute endTime
        endTimeDouble = double(currFrame - symbolPrevFrame[symbolIdx]) /
                        (double)1000000000 * sampling_rate;
        endTime = round(endTimeDouble);
        if (endTime > 0) {
          // what i-th element are we at for the current symbolIdx
          idx = lengthBuffer[symbolIdx - symbolOffset];
          // subtract 48 as hack to quickly convert ascii numerical
          // representation to int
          *(dataBuffer + (symbolIdx - symbolOffset) * maxElement + idx) =
              (int32_t)value - 48;
          *(timeBuffer + (symbolIdx - symbolOffset) * maxElement + idx) =
              endTime;

          lengthBuffer[symbolIdx - symbolOffset]++;
          // mexPrintf("symbol: %d, %d, %d\n",symbolIdx, endTime, (int32_t)value
          // - 48);
        }

        symbolPrevFrame[symbolIdx] = currFrame;
        break;
      }
    }

    // mexPrintf("elements %d %d \n",lengthBuffer[0],lengthBuffer[1]);

    if (nlhs != count) {
      mexPrintf(
          "WARNING:parsed %d symbols but there are only %d output buffer\n",
          count, nlhs);
      // mexErrMsgIdAndTxt("MATLAB:mexcpp:nargout",
      //        "vcd_parse needs more array output to hold all found symbols");
    }
    int maxOutput = min(nlhs, count);

    int elementCount = 0;
    int *outPtr;

    /*
    for (int i = 0; i < maxOutput; i++) {
        elementCount = lengthBuffer[i];
        for(int j=0;j<elementCount;j++){
            int offset = i*maxElement+j;
            mexPrintf("element count: %d, offset:%d\n",elementCount,offset);
            mexPrintf("element time:%d,value:%d\n",*(timeBuffer+offset),
    *(dataBuffer+offset));
        }
    }
    */

    for (int i = 0; i < maxOutput; i++) {
      elementCount = lengthBuffer[i];
      // mexPrintf("symbol index:%d element:%d\n", i, elementCount);
      // create N x 2 matrix to hold timestamp and data per symbol
      plhs[i] = mxCreateNumericMatrix(elementCount, 2, mxINT32_CLASS, mxREAL);

      outPtr = (int *)mxGetPr(plhs[i]);
      // Copy time buffer back to matlab output
      memcpy((void *)outPtr, timeBuffer, elementCount * sizeof(int));
      // We already copied time data so advance output pointer to get to the 2nd
      // column to store data
      outPtr = outPtr + elementCount;
      // Copy data buffer back to matlab output
      memcpy((void *)outPtr, dataBuffer, elementCount * sizeof(int));
      // advance data buffer for the next symbol
      timeBuffer = timeBuffer + maxElement;
      dataBuffer = dataBuffer + maxElement;
    }

    // mexPrintf("symbol:%x, time:%x, data:%x\n",*symbolLength,*time,*data);
  }
};

//****Matlab to C interface****

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {
  char *str;
  /* Check for proper number of arguments */

  if (nrhs != 1) {
    mexErrMsgIdAndTxt("MATLAB:mexcpp:nargin",
                      "MEXCPP requires one input arguments.");
  }
  /* Check to be sure input is of type char */
  if (!(mxIsChar(prhs[0]))) {
    mexErrMsgIdAndTxt("MATLAB:mexatexit:inputNotString",
                      "Input must be of type string.\n.");
  }

  str = mxArrayToString(prhs[0]);
  // create fileresource class based on input string
  VCDParse myparse(str);
  mxFree(str);
  myparse.Parse(nlhs, plhs);
  //***Release memory to prevent memory leak****
  mxFree(str);
  return;
}
