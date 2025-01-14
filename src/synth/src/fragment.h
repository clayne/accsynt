#pragma once

#include "compile_context.h"
#include "compile_metadata.h"

#include <props/props.h>

#include <support/indent.h>
#include <value_ptr/value_ptr.h>

#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>

#include <functional>
#include <set>
#include <unordered_set>
#include <vector>

namespace synth {
class fragment;
}

namespace std {
template <>
struct hash<bsc::value_ptr<synth::fragment>> {
  size_t operator()(bsc::value_ptr<synth::fragment> const& frag) const noexcept;
};
} // namespace std

namespace synth {

struct fragment_equal {
  bool operator()(bsc::value_ptr<fragment> const& a,
      bsc::value_ptr<fragment> const& b) const;
};

class fragment {
public:
  using frag_ptr = bsc::value_ptr<fragment>;
  using frag_set
      = std::unordered_set<frag_ptr, std::hash<frag_ptr>, fragment_equal>;

  /**
   * Randomly sample without replacement from the supplied fragments to create a
   * partially complete fragment. The remaining holes will be filled with linear
   * blocks.
   */
  static frag_ptr sample(
      std::vector<frag_ptr> const& fragments, size_t num_frags);

  static frag_set enumerate(std::vector<frag_ptr> const& fragments,
      std::optional<size_t> max_size = std::nullopt,
      size_t data_blocks = std::numeric_limits<size_t>::max());

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
  virtual void splice(
      compile_context& ctx, llvm::BasicBlock* entry, llvm::BasicBlock* exit)
      = 0;

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
  virtual bool add_child(frag_ptr f, size_t idx) = 0;

  template <typename T>
  bool equal_as(T const& other) const;

  /**
   * Counts the number of holes left in this fragment that can be instantiated
   * with something else. Default implementation makes sure to recurse properly,
   * but needs to make a virtual call to get the immediate number.
   */
  virtual size_t count_holes() const = 0;

  virtual bool equal_to(frag_ptr const& other) const = 0;

  static bool equal_non_null(frag_ptr const& a, frag_ptr const& b);

  virtual int get_id() const = 0;
  virtual std::vector<int> id_sequence() const;

protected:
  template <typename Func>
  static void choose(
      size_t n, std::vector<frag_ptr> const& fragments, Func&& f);

  template <typename Func>
  static void choose(size_t n, std::vector<frag_ptr> const& fragments,
      std::vector<frag_ptr>& accum, Func&& f);

  static frag_set enumerate_all(
      std::vector<frag_ptr> const& fragments, std::optional<size_t> max_size);

  static frag_set enumerate_permutation(std::vector<frag_ptr> const& perm);

  template <typename Iterator>
  static void enumerate_recursive(
      frag_set& results, frag_ptr& accum, Iterator begin, Iterator end);

  template <typename... Children>
  std::array<std::reference_wrapper<frag_ptr>, sizeof...(Children)>
  children_ref(Children&...) const;

  template <typename... Children>
  std::array<std::reference_wrapper<const frag_ptr>, sizeof...(Children)>
  children_ref(Children const&...) const;

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
bool fragment::equal_as(T const& other) const
{
  if (auto ptr = dynamic_cast<T const*>(this)) {
    return *ptr == other;
  }

  return false;
}

template <typename... Children>
std::array<std::reference_wrapper<fragment::frag_ptr>, sizeof...(Children)>
fragment::children_ref(Children&... chs) const
{
  return { std::ref(chs)... };
}

template <typename... Children>
std::array<std::reference_wrapper<const fragment::frag_ptr>,
    sizeof...(Children)>
fragment::children_ref(Children const&... chs) const
{
  return { std::cref(chs)... };
}

template <typename Iterator>
void fragment::enumerate_recursive(
    fragment::frag_set& results, frag_ptr& accum, Iterator begin, Iterator end)
{
  if (begin == end) {
    results.insert(accum);
  } else {
    auto holes = accum->count_holes();
    for (auto i = 0u; i < holes; ++i) {
      auto cloned = accum;
      auto next_clone = *begin;

      cloned->add_child(next_clone, i);
      enumerate_recursive(results, cloned, std::next(begin), end);
    }
  }
}

template <typename Func>
void fragment::choose(
    size_t n, std::vector<fragment::frag_ptr> const& fragments, Func&& f)
{
}
} // namespace synth
