#include "options.h"

#include <coverage/coverage.h>

#include <support/argument_generator.h>
#include <support/load_module.h>

#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>

#include <fmt/format.h>

using namespace support;
using namespace llvm;

Function* get_single_function(Module& mod)
{
  Function* ret_func = nullptr;

  for (auto& func : mod) {
    if (!func.isDeclaration()) {
      if (ret_func == nullptr) {
        // If this is the only defined function we've seen so far, keep track
        ret_func = &func;
      } else {
        // If this and another function are both definitions, then there's no
        // unambiguous candidate.
        return nullptr;
      }
    }
  }

  return ret_func;
}

coverage::wrapper get_wrapper(Module& mod)
{
  if (FunctionName == "-") {
    if (auto func = get_single_function(mod)) {
      return coverage::wrapper(*func);
    } else {
      throw std::runtime_error("Function selection ambiguous");
    }
  }

  return coverage::wrapper(mod, FunctionName);
}

int main(int argc, char** argv)
try {
  InitializeNativeTarget();
  LLVMInitializeNativeAsmPrinter();
  LLVMInitializeNativeAsmParser();

  cl::ParseCommandLineOptions(argc, argv);

  auto mod = load_or_parse_module(InputFile);
  if (!mod) {
    errs() << "Couldn't load or parse module: " << InputFile << '\n';
    return 1;
  }

  auto wrapper = get_wrapper(*mod);
  auto gen = uniform_generator();

  fmt::print("{},{},{}\n", "inputs", "covered", "total");

  for (auto i = 0; i < NumInputs; ++i) {
    auto build = wrapper.get_builder();
    gen.gen_args(build);

    wrapper.call(build);

    fmt::print(
        "{},{},{}\n", i + 1, wrapper.covered_conditions(),
        wrapper.total_conditions());
  }
} catch (std::runtime_error& e) {
  llvm::errs() << "Error creating coverage JIT wrapper:  ";
  llvm::errs() << e.what() << '\n';
  std::exit(2);
}