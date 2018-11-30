#include "fragment.h"

#include <llvm/IR/Constant.h>
#include <llvm/IR/Function.h>

using namespace llvm;
using namespace props;

namespace synth {

fragment::fragment(std::vector<value> args) :
  args_(args)
{
}

void fragment::print_indent(std::ostream& os, size_t indent)
{
  for(auto i = 0u; i < indent; ++i) {
    os << "  ";
  }
}

void fragment::print(std::ostream& os)
{
  print(os, 0);
}

compile_metadata fragment::compile(compile_context& ctx)
{
  splice(ctx, ctx.entry_, ctx.exit_);
  return ctx.metadata_;
}

/**
 * From what I've thought about, I think the best thing to do is to have a
 * context "own" / manage a single function which is created when we create a
 * context object - then we can also keep track of the appropriate basic blocks
 * with the context object as well.
 */
compile_context::compile_context(Module& mod, signature sig) :
  sig_(sig), mod_(mod), 
  func_(sig_.create_function(mod_)),
  entry_(BasicBlock::Create(mod_.getContext(), "entry", func_)),
  exit_(BasicBlock::Create(mod_.getContext(), "exit", func_)),
  metadata_(func_)
{
  auto& ctx = mod_.getContext();

  auto rt = func_->getFunctionType()->getReturnType();
  if(rt->isVoidTy()) {
    return_ = ReturnInst::Create(ctx, exit_);
  } else {
    auto zero = Constant::getNullValue(rt);
    return_ = ReturnInst::Create(ctx, zero, exit_);
  }
}

llvm::Argument *compile_context::argument(std::string const& name)
{
  return std::next(func_->arg_begin(), sig_.param_index(name));
}

compile_metadata::compile_metadata(llvm::Function *fn)
  : function(fn)
{
}

}