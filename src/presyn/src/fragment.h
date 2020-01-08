#pragma once

#include "parameter.h"

#include <support/traits.h>

#include <memory>
#include <string>
#include <string_view>
#include <type_traits>

namespace presyn {

class fragment {
public:
  /**
   * Parse a fragment from a format string describing it. The logic for this is
   * essentially a factory pattern that dispatches on a parsed fragment name in
   * order to construct the correct type at runtime.
   */
  static std::unique_ptr<fragment> parse(std::string_view);

  /**
   * Compose this fragment with another one. This is an abstract operation, and
   * so it needs to be able to take ownership of the other fragment (e.g. to
   * save it until compile time when its compositional behaviour can be used).
   *
   * The consequence of this design is that fragments don't need to know the
   * internals of what they're being composed with (but can possibly in the
   * future make some kind of determinance about the behaviour of the other
   * fragment...).
   *
   * Because this gets ownership of the parameter, it means that (for example)
   * an implementation could just return the original if it needs.
   *
   * Generally, the pattern that will be followed by implementations of this is
   * that they store up fragments until compilation, when they'll use the
   * exposed behaviour of their compositions to perform a compilation.
   */
  [[nodiscard]] virtual std::unique_ptr<fragment> compose(
      std::unique_ptr<fragment>&&)
      = 0;

  /**
   * Because the core composition logic is defined virtually, we can use a
   * templated base class method to define composition in cases where we don't
   * have a UP already constructed.
   */
  template <typename Fragment>
  [[nodiscard]] std::enable_if_t<
      !support::is_unique_ptr_v<std::decay_t<Fragment>>,
      std::unique_ptr<fragment>>
  compose(Fragment&&);

  /**
   * Any two fragments can be composed together, but the result may not actually
   * use the second fragment. For example:
   *
   *  empty * F = empty, for all F
   *
   * Additionally, some fragments may have multiple child fragments. For
   * example:
   *
   *  seq(F, G)
   *
   * In this example, we need to be able to define the semantics of composition
   * properly. That is, which of these is the correct result?
   *
   *  seq(F, G) * H = seq(F * H, G)
   *  seq(F, G) * H = seq(F, G * H)
   *
   * The way to define this is by allowing fragments to communicate when they
   * will *use* the result of a composition:
   *
   *  accepts(empty) = false
   *
   * So then:
   *
   *  seq(F, G) * H = seq(F * H, G) if accepts(F)
   *                = seq(F, G * H) if accepts(G)
   *                = seq(F, G)     else
   *
   * The semantics of accepts can then be defined recursively:
   *
   *  accepts(seq(F, G)) = accepts(F) || accepts(G)
   *
   * We can see that this will respect the definition of composition given
   * above: seq(F, G) only uses H in its compositions if it accepts. However,
   * the composition is still well-defined in this case.
   *
   * This relationship between acceptance and composition should be respected by
   * new fragment implementations.
   */
  virtual bool accepts() const = 0;

  /**
   * Compilation logic not yet implemented until the core of the actual
   * behaviour is built up.
   */
  /* virtual program compile() = 0; */

  /**
   * Get a representation of this type of fragment as a string for
   * pretty-printing.
   */
  virtual std::string to_string() const = 0;

  /**
   * Because this is an abstract base class, it needs a virtual destructor to
   * make sure that derived classes are deleted correctly.
   */
  virtual ~fragment() = default;
};

/**
 * An empty fragment will generate no behaviour, and acts as an identity under
 * composition.
 */
class empty final : public fragment {
public:
  empty() = default;

  [[nodiscard]] std::unique_ptr<fragment> compose(
      std::unique_ptr<fragment>&&) override;

  bool accepts() const override;

  std::string to_string() const override;
};

/**
 * A linear fragment will produce a basic block of instructions (the number of
 * which is specified by a parameter - i.e. linear(2) produces 2 instructions,
 * and linear(0) would be equivalent to empty()). It acts as the identity for
 * composition.
 */
class linear final : public fragment {
public:
  linear(std::unique_ptr<parameter>&&);

  linear(int);

  /**
   * There's room for some semantics-based optimisations and algebraic
   * properties here - by "crossing the 4th wall", if this is composed with
   * another linear fragment you can merge them. It violates the strict
   * compositionality idea but would reduce the size / complexity /
   * compilation time.
   */
  [[nodiscard]] std::unique_ptr<fragment> compose(
      std::unique_ptr<fragment>&&) override;

  bool accepts() const override;

  std::string to_string() const override;

private:
  std::unique_ptr<parameter> instructions_;
};

/**
 * A fragment that represents two fragments being executed in sequence one after
 * the other.
 *
 * Composition is defined using acceptance - if the *first* element in the
 * sequence accepts, then it receives the composition. If it does not and the
 * second one does, similarly. If neither does, this fragment does not accept
 * either.
 *
 * The acceptance-based composition is only used if this fragment is partially
 * empty: i.e. it does not yet have an F or G as used above.
 */
class seq final : public fragment {
public:
  seq();

  [[nodiscard]] std::unique_ptr<fragment> compose(
      std::unique_ptr<fragment>&&) override;

  bool accepts() const override;

  std::string to_string() const override;

private:
  seq(std::unique_ptr<fragment>&&, std::unique_ptr<fragment>&&);

  std::unique_ptr<fragment> first_;
  std::unique_ptr<fragment> second_;
};

// Implementations

template <typename Fragment>
[[nodiscard]] std::enable_if_t<
    !support::is_unique_ptr_v<std::decay_t<Fragment>>,
    std::unique_ptr<fragment>>
fragment::compose(Fragment&& other)
{
  // This perfectly forwards the supplied fragment and constructs a unique_ptr
  // of the relevant derived type.
  return compose(std::make_unique<Fragment>(std::forward<Fragment>(other)));
}

namespace literals {

std::unique_ptr<fragment> operator""_frag(const char* str, size_t len);

} // namespace literals

} // namespace presyn