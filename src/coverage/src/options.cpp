#include "options.h"

using namespace llvm;

cl::opt<std::string>
    InputFile("input-file", cl::desc("Input bitcode file"), cl::init("-"));

cl::alias
    InputFileA("i", cl::desc("Alias for input-file"), cl::aliasopt(InputFile));

cl::opt<std::string>
    FunctionName(cl::Positional, cl::desc("Function to test"), cl::Required);

cl::opt<int> NumInputs(
    "num-inputs", cl::desc("The number of distinct inputs to generate"),
    cl::init(10));

cl::alias
    NumInputsA("n", cl::desc("Alias for num-inputs"), cl::aliasopt(NumInputs));
