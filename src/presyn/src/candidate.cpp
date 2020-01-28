#include "candidate.h"

#include <support/assert.h>
#include <support/narrow_cast.h>

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>

#include <algorithm>
#include <map>
#include <set>

using namespace llvm;

namespace {

// Generic stub visitor

template <typename Func>
class stub_visitor : public InstVisitor<stub_visitor<Func>> {
public:
  stub_visitor(Func&&);

  void visitCallInst(CallInst&) const;

private:
  Func action_;
};

template <typename Func>
stub_visitor<Func>::stub_visitor(Func&& f)
    : action_(std::forward<Func>(f))
{
}

template <typename Func>
void stub_visitor<Func>::visitCallInst(CallInst& inst) const
{
  auto fn = inst.getCalledFunction();
  auto name = fn->getName();
  if (name.startswith("stub")) {
    action_(inst);
  }
}

// Validation visitor

class is_valid_visitor : public InstVisitor<is_valid_visitor> {
public:
  is_valid_visitor() = default;

  bool valid() const;

  void visitCallInst(CallInst const&);

private:
  bool valid_ = true;
};

bool is_valid_visitor::valid() const { return valid_; }

void is_valid_visitor::visitCallInst(CallInst const& ci)
{
  auto fn = ci.getCalledFunction();
  if (fn->isDeclaration()) {
    valid_ = false;
  }
}

} // namespace

namespace presyn {

candidate::candidate(props::signature sig, std::unique_ptr<Module>&& mod)
    : signature_(sig)
    , module_(std::move(mod))
{
  resolve_names();
  choose_values();
  resolve_operators();
}

Function& candidate::function() const
{
  auto func = module_->getFunction(signature_.name);
  assertion(func != nullptr, "Must have correctly named candidate function");
  return *func;
}

Module& candidate::module() { return *module_; }
Module const& candidate::module() const { return *module_; }

void candidate::resolve_names()
{
  // The process for resolving stubbed-out names in the generated sketch is as
  // follows:
  //  - for all the call insts in the function, look at their name and argument
  //    list.
  //  - if they have a name like 'stub', and their only argument is a constant
  //    character array, then de-materialize it to a string and look up the
  //    param with that name in the sig.
  //  - delete the stub, and replace it with the named argument
  //
  //  For all these things we need an instvisitor really - will save writing all
  //  the loops over and over.

  auto replacements = std::map<CallInst*, Value*> {};

  stub_visitor([&, this](auto& ci) {
    if (ci.arg_size() != 1) {
      return;
    }

    if (auto name = arg_name(ci.getArgOperand(0))) {
      auto idx = signature_.param_index(*name);
      auto arg = function().arg_begin() + idx;

      replacements[&ci] = arg;
    }
  }).visit(function());

  for (auto [stub, val] : replacements) {
    auto conv = converter(val->getType(), stub->getType());

    auto build = IRBuilder(stub);
    auto call = build.CreateCall(conv, {val}, stub->getName());

    safe_rauw(stub, call);
  }
}

void candidate::choose_values()
{
  // After resolving the named stubs in the function, the next step in the
  // candidate construction process is to select values for all the stubs in the
  // program.
  //
  // Worth noting that this will involve some kind of non-determinism (as random
  // choices will have to be made), so it's probably worth considering from the
  // beginning how to get it to be controllable. For a given sketch, the set of
  // available decisions will always be the same, so we can try to record which
  // ones are made so that branches / near misses / introspection are possible.
}

void candidate::resolve_operators()
{
  // After values are chosen for the stubbed out values in the function, the
  // operators can be resolved - this step will involve some thought about the
  // types of the values being used (as by now we'll know the types).
}

bool candidate::is_valid() const
{
  auto vis = is_valid_visitor();
  vis.visit(function());
  return vis.valid();
}

std::optional<std::string> candidate::arg_name(llvm::Value* arg) const
{
  if (auto const_val = dyn_cast<ConstantDataArray>(arg)) {
    auto arr_t = const_val->getType();
    if (arr_t->getElementType()->isIntegerTy(8)) {
      auto str = std::vector<char> {};

      for (auto i = 0; i < arr_t->getNumElements(); ++i) {
        auto elt = const_val->getAggregateElement(i);
        auto chr = ::support::narrow_cast<char>(
            elt->getUniqueInteger().getSExtValue());
        str.push_back(chr);
      }

      return std::string(str.begin(), str.end());
    }
  }

  return std::nullopt;
}

llvm::Function* candidate::converter(llvm::Type* from, llvm::Type* to)
{
  // This allows us to get rid of the opaque struct type where usages of it
  // remain in the code - if any of these stubs try to produce an opaque struct
  // type, then we assume that the resulting type is precisely what it's being
  // converted from.
  if (auto ptr_to = dyn_cast<PointerType>(to)) {
    if (auto elt_st = dyn_cast<StructType>(ptr_to->getElementType())) {
      if (elt_st->isOpaque()) {
        to = from;
      }
    }
  }

  if (converters_.find({from, to}) == converters_.end()) {
    auto func_ty = FunctionType::get(to, {from}, false);
    auto func = Function::Create(
        func_ty, GlobalValue::ExternalLinkage, "id", *module_);

    auto bb = BasicBlock::Create(module_->getContext(), "entry", func);

    converters_[{from, to}] = func;
  }

  return converters_.at({from, to});
}

void candidate::safe_rauw(CallInst* stub, Value* call)
{
  auto replacements = std::map<CallInst*, Value*> {};

  if (call->getType() == stub->getType()) {
    stub->replaceAllUsesWith(call);
  } else {
    // Different types so we need to recreate every stub call that *uses* the
    // result of this one separately.

    for (auto user : stub->users()) {
      assertion(isa<CallInst>(user), "Users of stub calls must be calls");
      auto user_call = cast<CallInst>(user);

      auto new_args = std::vector<Value*> {};
      for (auto& arg : user_call->args()) {
        new_args.push_back(arg == stub ? call : arg);
      }

      auto new_call = IRBuilder(stub).CreateCall(
          user_call->getCalledFunction(), new_args, stub->getName());

      replacements[user_call] = new_call;
    }
  }

  for (auto [st, ca] : replacements) {
    safe_rauw(st, ca);
  }

  stub->eraseFromParent();
}

} // namespace presyn
