#pragma once

#include <support/terminal.h>

#include <fmt/format.h>

#include <filesystem>

namespace support {

namespace detail {

#ifdef NDEBUG
static constexpr bool debug = false;
#else
static constexpr bool debug = true;
#endif

template <typename Cond, typename... Args>
void assert_impl(Cond&& cond, char const* check, char const* type,
    char const* file, int line, char const* func, Args... args)
{
  using namespace fmt::literals;
  namespace term = support::terminal;

  if constexpr (detail::debug) {
    auto result = std::forward<Cond>(cond)();

    if (!result) {
      auto path = std::filesystem::path(file);
      fmt::print("{type}{bad}{check}{reset} at:\n"
                 "  {bold}{file}:{line} ({func}){reset}\n",
          "bad"_a = term::f_red + term::bold, "reset"_a = term::reset,
          "bold"_a = term::bold, "check"_a = check,
          "file"_a = path.filename().string(), "line"_a = line, "type"_a = type,
          "func"_a = func);

      if constexpr (sizeof...(args) > 0) {
        fmt::print("\n{}\n", fmt::format(args...));
      }

      std::exit(1);
    }
  }
}

} // namespace detail

template <typename Func>
void in_debug(Func&& f)
{
  if constexpr (detail::debug) {
    std::forward<Func>(f)();
  }
}

} // namespace support

#define LAZY(x) [&] { return static_cast<bool>(x); }

#define assertion(c, ...)                                                      \
  do {                                                                         \
    support::detail::assert_impl(LAZY(c), #c, "Assertion failure: ", __FILE__, \
        __LINE__, __func__, __VA_ARGS__);                                      \
  } while (false)

#define assumes(c, ...)                                                        \
  do {                                                                         \
    support::detail::assert_impl(LAZY(c), #c,                                  \
        "Precondition violated: ", __FILE__, __LINE__, __func__, __VA_ARGS__); \
  } while (false)

#define ensures(c, ...)                                                        \
  do {                                                                         \
    support::detail::assert_impl(LAZY(c), #c,                                  \
        "Postcondition violated: ", __FILE__, __LINE__, __func__,              \
        __VA_ARGS__);                                                          \
  } while (false)

#define unimplemented()                                                        \
  do {                                                                         \
    support::detail::assert_impl(LAZY(false), "",                              \
        "Unimplemented code reached", __FILE__, __LINE__, __func__);           \
    __builtin_unreachable();                                                   \
  } while (false)

#define invalid_state()                                                        \
  do {                                                                         \
    support::detail::assert_impl(LAZY(false), "", "Invalid state reached",     \
        __FILE__, __LINE__, __func__);                                         \
    __builtin_unreachable();                                                   \
  } while (false)
