#pragma once

#include <dist/utils.h>

#include <llvm/IR/Value.h>
#include <llvm/Support/raw_ostream.h>

#include <random>
#include <set>

namespace accsynt {

template <typename Builder>
class IndexSynth {
public:
  IndexSynth(Builder &b) :
    B_(b)
  {}

  void add_index(llvm::Value *idx)
  {
    indexes_.insert(idx);
  }

  void add_const(llvm::Value *cst)
  {
    consts_.insert(cst);
  }

  llvm::Value *generate()
  {
    auto summands = std::vector<llvm::Value *>{};

    for(auto idx : indexes_) {
      auto use_type = index_use_type();
      if(use_type == 0) {
        // Use this index directly
        summands.push_back(idx);
      } else if(use_type == 1 && !consts_.empty()) {
        // Multiply by a constant
        auto factor = uniform_sample(consts_);
        auto mult = B_.CreateMul(idx, *factor);
        summands.push_back(mult);
      } // else ignore this one
    }

    if(summands.empty()) {
      return *uniform_sample(indexes_);
    } else {
      auto begin = summands.begin();
      auto end = summands.end();

      auto sum = *begin;
      for(auto it = std::next(begin); it != end; ++it) {
        sum = B_.CreateAdd(sum, *it);
      }
      return sum;
    }
  }

private:
  Builder &B_;
  std::set<llvm::Value *> indexes_{};
  std::set<llvm::Value *> consts_{};

  std::random_device rd_{};

  int index_use_type()
  {
    std::uniform_int_distribution<int> dist(0,2);
    return dist(rd_);
  }
};

}