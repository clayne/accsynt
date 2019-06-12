#include <catch2/catch.hpp>

#include <props/props.h>

#include <sstream>

using namespace props;
using namespace props::literals;

// clang-format off
TEST_CASE("Can use the v2 visitor")
{
  SECTION("in a very basic case") 
  {
    auto s = "void f(float a, int *b, int c)"_sig;

    auto c = 0;
    sig_visitor{
      on(data_type::integer, [&] (auto const&) { ++c; })
    }.visit(s);

    REQUIRE(c == 1);
  }

  SECTION("on another basic case")
  {
    auto s = "char c(int argc, char **argv)"_sig;

    auto c = 0;
    sig_visitor{
      on(data_type::character, [&] (auto const&) { throw 0; }),
      on(data_type::character, 2, [&] (auto const&) { ++c; }),
      on(data_type::integer, [&] (auto const&) { c -= 2; })
    }.visit(s);

    REQUIRE(c == -1);
  }

  SECTION("visiting different depths")
  {
    auto s = "void f(int a, int *b, int **c, int *d)"_sig;

    auto ss = std::stringstream{};
    sig_visitor{
      on(data_type::integer, [&] (auto const&) { ss << "A"; }),
      on(data_type::integer, 1, [&] (auto const&) { ss << "B"; }),
      on(data_type::integer, 2, [&] (auto const&) { ss << "C"; }),
      on(data_type::integer, 3, [&] (auto const&) { ss << "D"; })
    }.visit(s);

    REQUIRE(ss.str() == "ABCB");
  }

  SECTION("visiting more than once")
  {
    auto s = "int g(int a, int b)"_sig;
    
    auto c = 0;
    sig_visitor{
      on(data_type::integer, [&] (auto const&) { ++c; })
    }.visit(s);

    REQUIRE(c == 2);
  }

  SECTION("complex visiting")
  {
    auto s = "float h(float ***g, float z, int b, int *t, char **n)"_sig;

    auto ss = std::stringstream{};
    sig_visitor {
      on(data_type::character, 2, [&] (auto const& p) { ss << "Cp2" << p.name; }),
      on(data_type::floating, 3, [&] (auto const& p) { ss << "Fp3" << p.name; }),
      on(data_type::floating, [&] (auto const& p) { ss << "Fs" << p.name; }),
      on(data_type::integer, 1, [&] (auto const& p) { ss << "Ip1" << p.name; }),
      on(data_type::integer, [&] (auto const& p) { ss << "Is" << p.name; })
    }.visit(s);

    REQUIRE(ss.str() == "Fp3gFszIsbIp1tCp2n");
  }
}
// clang-format on
