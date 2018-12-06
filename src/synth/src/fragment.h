#pragma once

#include <props/props.h>

#include <support/indent.h>

#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>

#include <functional>
#include <set>
#include <vector>

namespace synth {

class fragment;

/**
 * The metadata we collect during compilation is:
 *  * The function itself will be returned as part of this object.
 *  * A set of seed values.
 *  * A set of blocks to use as data blocks.
 *  * A set of output locations to be stored to.
 */
struct compile_metadata {
  llvm::Function *function;
  std::set<llvm::Value *> seeds = {};
  std::set<llvm::BasicBlock *> data_blocks = {};
  std::set<llvm::Value *> outputs = {};

  explicit compile_metadata(llvm::Function *fn);
};

/**
 * Information and helper methods for compiling fragments. Responsible for
 * interfacing with an LLVM function, keeping track of a signature etc.
 */
class compile_context {
  friend class fragment;

public:
  compile_context(llvm::Module& mod,
                  props::signature sig);
 
  /**
   * Don't want these to be copyable - once used to compile they are done as we
   * create the function and fill it up.
   */
  compile_context(compile_context const&) = delete;
  compile_context& operator=(compile_context const&) = delete;

  // TODO: define these and add a flag to the object that checks for
  // use-after-move?
  compile_context(compile_context&&) = default;
  compile_context& operator=(compile_context&&) = default;

  /**
   * Get the LLVM arg for the parameter name passed in. This lives in the
   * context because it depends on the signature.
   */
  llvm::Argument *argument(std::string const& name);

// TODO: work out encapsulation for context - need to make information available
// to derived fragment classes?
/* protected: */
  props::signature sig_;

  llvm::Module& mod_;

  llvm::Function *func_;
  llvm::BasicBlock *entry_;
  llvm::BasicBlock *exit_;
  llvm::ReturnInst *return_;

  compile_metadata metadata_;
};

class fragment {
public:
  using frag_ptr = std::unique_ptr<fragment>;

  static std::vector<frag_ptr> enumerate_all(std::vector<frag_ptr>&& fragments);

  /**
   * Instantiate a fragment based on matched arguments from an inference rule.
   * This doesn't receive a signature because the compilation context is
   * responsible for that - means that fragments are portable across functions
   * with differently ordered parameters etc.
   *
   * TODO: Do we want to check arguments somehow? Virtual validation method or
   * just let subclasses do their own thing?
   */
  fragment(std::vector<props::value> args);

  /**
   * Default virtual destructor to allow for polymorphic usage.
   */
  virtual ~fragment() = default;

  /**
   * Virtual clone to allow for polymorphic copying of fragment objects.
   */
  virtual frag_ptr clone() = 0;

  /**
   * Get a pretty-printed representation of this fragment.
   */
  virtual std::string to_str(size_t indent = 0) = 0;

  /**
   * Compile this fragment to LLVM using ctx, which contains all the information
   * needed to do so (attachment blocks, signature for parameter index mapping,
   * etc). This probably doesn't need to be virtual in that case - public
   * interface knows how to compile in terms of managing a context and splicing,
   * given knowledge of how to splice (virtual).
   */
  compile_metadata compile(compile_context& ctx);

  /**
   * Recursive primitive that makes up compilation - insert this fragment
   * between two basic blocks. Will expect the entry block not to be terminated?
   */
  virtual void splice(compile_context& ctx, llvm::BasicBlock *entry, llvm::BasicBlock *exit) = 0;

  /**
   * Adds a new child fragment to this one - will recurse into existing children
   * if necessary in order to achieve the "first empty hole" part of the
   * semantics.
   *
   * Returns true if the child was added, and false if not. Subclasses are
   * responsible for managing their own insertion logic (i.e. keeping track of
   * how many children they have).
   *
   * The child pointer passed into this one is moved from even if insertion
   * fails.
   */
  virtual bool add_child(frag_ptr&& f, size_t idx) = 0;

  template <typename T>
  bool add_child(T frag, size_t idx);

  /**
   * Counts the number of holes left in this fragment that can be instantiated
   * with something else. Default implementation makes sure to recurse properly,
   * but needs to make a virtual call to get the immediate number.
   */
  virtual size_t count_holes() const = 0;

protected:
  static std::vector<frag_ptr> enumerate_permutation(
    std::vector<frag_ptr> const& perm);

  /**
   * Helper method to clone and copy with the right type - simplifies the
   * virtual clone method by having this handle the construction of a
   * unique_ptr.
   */
  template <typename T>
  frag_ptr clone_as(T const& obj) const;

  template <typename... Children>
  std::array<std::reference_wrapper<frag_ptr>, sizeof...(Children)> 
    children_ref(Children&...) const;

  /**
   * If the fragment pointed to is empty / nullptr, then return 1 - it
   * represents a hole. Otherwise return the number of empty holes in that
   * fragment.
   */
  static size_t count_or_empty(frag_ptr const& frag);

  static std::string string_or_empty(frag_ptr const& frag, size_t ind);

  std::vector<props::value> args_;
};

template <typename T>
bool fragment::add_child(T frag, size_t idx)
{
  return add_child(frag_ptr{frag.clone()}, idx);
}

template <typename T>
fragment::frag_ptr fragment::clone_as(T const& obj) const
{
  return fragment::frag_ptr(new T{obj});
}

template <typename... Children>
std::array<std::reference_wrapper<fragment::frag_ptr>, sizeof...(Children)> 
fragment::children_ref(Children&... chs) const
{
  return { std::ref(chs)... };
}

}
