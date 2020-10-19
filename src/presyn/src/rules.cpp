#include "rules.h"

using namespace llvm;

namespace presyn::rules {

void do_nothing::match(
    rule_filler& fill, CallInst* hole, std::vector<Value*> const& choices,
    std::vector<Value*>& generated) const
{
}

void all_of_type::match(
    rule_filler& fill, CallInst* hole, std::vector<Value*> const& choices,
    std::vector<Value*>& generated) const
{
  for (auto val : choices) {
    if (val->getType() == hole->getType()) {
      generated.push_back(fill.copy_value(val));
    }
  }
}

void all_if_opaque::match(
    rule_filler& fill, CallInst* hole, std::vector<Value*> const& choices,
    std::vector<Value*>& generated) const
{
  if (fill.has_unknown_type(hole)) {
    for (auto val : choices) {
      generated.push_back(fill.copy_value(val));
    }
  }
}

} // namespace presyn::rules
