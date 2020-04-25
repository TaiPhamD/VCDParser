# Converts VCD encoded data files into alligned time series text

### CLI syntax

./CVDConvert input.vcd

Then the app will output an output.txt CSV format that contains all your input data aligned with the timestamp in the first column.
The number of rows for the output file is the max row of all state transition between all your inputs.


### compile & prereg

- Requires for you to have cmake installed and C++ compiler

```
git clone https://github.com/TaiPhamD/VCDParser
cd VCDParser
mkdir build
cmake ..
make
./CVDConvert input.vcd
```