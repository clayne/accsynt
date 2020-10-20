#pragma once

#include <support/call_builder.h>
#include <support/traits.h>
#include <support/utility.h>

#include <props/props.h>

#include <limits>
#include <memory>
#include <random>
#include <type_traits>

namespace support {

namespace detail {

#define VAL(t) std::declval<t>()

// The formatter doesn't like SFINAE
// clang-format off

/**
 * SFINAE base case - types are not generators unless they meet the requirements
 * of the template specialisation below.
 */
template <typename, typename = std::void_t<>>
struct is_generator : std::false_type {
};

/**
 * SFINAE specialisation to detect valid generators. This needs both arms
 * (void_t and is_same) so that a valid type is still formed even when members
 * are missing.
 *
 * This could be a bit neater by using is_detected, but it's OK for now.
 */
template <typename T>
struct is_generator<T, 
    std::void_t<
      decltype(VAL(T).gen_args(VAL(call_builder&)))
    >>
  : std::conjunction<
      std::is_same<decltype(VAL(T).gen_args(VAL(call_builder&))), void>,
      std::is_copy_constructible<T>, 
      std::is_move_constructible<T>
    > {};

/**
 * Helper value for convenience.
 */
template <typename T> 
constexpr bool is_generator_v = is_generator<T>::value;

// clang-format on
} // namespace detail

/**
 * Type-erased wrapper class that allows any type satisfying is_generator to be
 * used generically by code that needs to generate arguments.
 *
 * Defines internal model and concept classes. The provided strategy from
 * construction is stored in a unique_ptr to the concept class, with
 * implementation provided by a templated model object that inherits from the
 * concept class.
 *
 * Method calls to the outer generator type are forwarded to the internal
 * implementations.
 *
 * When an object of this class is constructed, the type passed in is checked
 * using a type trait against a definition of valid generators.
 */
class argument_generator {
public:
  template <typename T>
  argument_generator(T&& strat)
      : strategy_(std::make_unique<model<T>>(FWD(strat)))
  {
    static_assert(detail::is_generator_v<std::decay_t<T>>, "Not a generator");
  }

  // Rule of 5 to make thus class copyable
  argument_generator(argument_generator& other);
  argument_generator& operator=(argument_generator other);

  argument_generator(argument_generator&&) = default;
  argument_generator& operator=(argument_generator&&) = default;

  ~argument_generator() = default;

  friend void swap(argument_generator& a, argument_generator& b);

  // Interface
  /**
   * Generate arguments using the wrapped strategy, filling them into the call
   * builder.
   */
  void gen_args(call_builder&);

private:
  // Type erasure
  struct concept
  {
    virtual ~concept() { }
    virtual concept* clone() = 0;
    virtual void gen_args(call_builder&) = 0;
  };

  template <typename T>
  struct model : concept {
    model(T obj)
        : object_(obj)
    {
    }

    model<T>* clone() override { return new model<T>(object_); }

    void gen_args(call_builder& build) override { object_.gen_args(build); }

  private:
    T object_;
  };

protected:
  std::unique_ptr<concept> strategy_;
};

/**
 * Generate arguments uniformly - use this if there's absolutely no restriction
 * on how data is structured other than a physical size limit for arrays, which
 * can be passed at construction.
 *
 * This does generate integers as values in the range [0, max_size), which means
 * that they can be safely used to index into arrays generated by this
 * generator.
 */
class uniform_generator {
public:
  static constexpr size_t max_size = 32;

  uniform_generator();
  uniform_generator(size_t);

  void seed(std::random_device::result_type);
  void gen_args(call_builder&);

private:
  // Specialised only for int and float - doing it as a template makes the code
  // a bit nicer for the array case, which can just forward through to this
  // template rather than doing is_same checks.
  template <typename T>
  T gen_single();

  template <typename T>
  std::vector<T> gen_array();

  std::default_random_engine engine_;
  size_t size_;
};

/**
 * Generator specifically used for CSR SPMV arguments - will only work if the
 * arguments are exactly right (i.e. are named correctly and have the right
 * types). Otherwise it'll throw an exception.
 */
class csr_generator {
public:
  csr_generator();

  void gen_args(call_builder&);

private:
  int64_t gen_rows();
  std::vector<int64_t> gen_rowstr(int64_t rows);
  std::vector<int64_t> gen_colidx(std::vector<int64_t> const& rowstr);
  std::vector<float> gen_data(std::vector<int64_t> const& rowstr);
  std::vector<float> gen_input(std::vector<int64_t> const& colidx);
  std::vector<float> gen_output(int64_t rows);

  bool is_csr_spmv(props::signature const&);

  int max_size_;
  std::default_random_engine engine_;
};

/*
 * Template definitions
 */
template <>
int uniform_generator::gen_single<int>();

template <>
float uniform_generator::gen_single<float>();

template <typename T>
std::vector<T> uniform_generator::gen_array()
{
  // Cubing here to ensure that sizes are respected even if we're in the
  // presence of (say) an O(n^3) algorithm.
  auto ret = std::vector<T>(size_ * size_ * size_);
  std::generate(ret.begin(), ret.end(), [this] { return gen_single<T>(); });
  return ret;
}
} // namespace support

#undef VAL
