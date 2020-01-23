#include "sketch_context.h"

#include "stub.h"

#include <support/assert.h>
#include <support/thread_context.h>

#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>

using namespace support;
using namespace llvm;

namespace presyn {

sketch_context::sketch_context(Module& mod, props::signature sig)
    : module_(mod)
    , sig_(sig)
    , opaque_type_(StructType::create(thread_context::get(), "opaque"))
{
}

CallInst* sketch_context::stub() { return stub(opaque_type_); }

CallInst* sketch_context::stub(llvm::Type* ty)
{
  return stub(ty, std::vector<llvm::Value*>{});
}

CallInst*
sketch_context::stub(llvm::Type* ty, std::vector<llvm::Value*> const& args)
{
  auto func_type = FunctionType::get(ty, true);

  if (stubs_.find(ty) == stubs_.end()) {
    auto func = Function::Create(
        func_type, GlobalValue::ExternalLinkage, "stub_func", module_);

    stubs_[ty] = func;
  }

  return CallInst::Create(func_type, stubs_[ty], args, "stub");
}

CallInst* sketch_context::stub(std::string const& name)
{
  return stub(opaque_type_, name);
}

CallInst* sketch_context::stub(llvm::Type* ty, std::string const& name)
{
  auto const_name = constant_name(name);
  return stub(ty, {const_name});
}

Constant* sketch_context::constant_name(std::string const& name)
{
  if (names_.find(name) == names_.end()) {
    auto char_ty = IntegerType::get(module_.getContext(), 8);
    auto name_chars = std::vector<llvm::Constant*>();

    for (auto c : name) {
      name_chars.push_back(ConstantInt::get(char_ty, c, false));
    }

    auto array_type = ArrayType::get(char_ty, name_chars.size());
    names_[name] = ConstantArray::get(array_type, name_chars);
  }

  return names_[name];
}

} // namespace presyn
