#include "rules.h"

namespace synth {

std::vector<rule> rule_registry::all()
{
  using match = match_expression;

  return {
    {
      "regularLoop", {"sz", "ptr"},
      { match("size", "ptr", "sz") },
      {}
    },
    {
      "regularLoop", {"sz", "ptrA", "ptrB"},
      { match("size", "ptrA", "sz"),
        match("size", "ptrB", "sz") },
      { distinct("ptrA", "ptrB") }
    },
    {
      "regularLoop", {"sz", "ptrA", "ptrB", "ptrC"},
      { match("size", "ptrA", "sz"),
        match("size", "ptrB", "sz"),
        match("size", "ptrC", "sz") },
      { distinct("ptrA", "ptrB", "ptrC") }
    }
  };
}

}
