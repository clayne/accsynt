#include <props/props.h>

#define TAO_PEGTL_NAMESPACE props_pegtl
#include <tao/pegtl.hpp>

#include <iostream>

namespace props {

namespace pegtl = tao::props_pegtl;
using namespace pegtl;

std::optional<data_type> data_type_from_string(std::string const& str)
{
  if(str == "int") { return data_type::integer; }
  else if(str == "float") { return data_type::floating; }
  else { return std::nullopt; }
}

template <typename Rule>
struct signature_action : nothing<Rule>
{};

template <typename Rule>
struct param_action : nothing<Rule>
{};

template <typename Rule>
struct property_action : nothing<Rule>
{};

struct type_name
  : sor<
      TAO_PEGTL_STRING("void"),
      TAO_PEGTL_STRING("int"),
      TAO_PEGTL_STRING("float")
    >
{};

struct interface_name
  : identifier
{};

struct pointers
  : star<
      string<'*'>
    >
{};

struct param_spec
  : seq<
      type_name,
      plus<space>,
      pointers,
      interface_name
    >
{};

struct params
  : list<
      param_spec,
      seq<
        star<space>,
        string<','>,
        star<space>
      >
    >
{};

struct signature_grammar
  : seq<
      type_name,
      plus<space>,
      interface_name,
      string<'('>,
      action<
        param_action,
        opt<params>
      >,
      string<')'>
    >
{};

struct any_in_line
  : seq<
      not_at<eol>,
      any
    >
{};

struct comment_grammar
  : seq<
      bol,
      string<';'>,
      star<any_in_line>
    >
{};

struct ignore_line
  : sor<
      comment_grammar,
      bol
    >
{};

struct property_name
  : identifier
{};

struct value_string
  : seq<
      TAO_PEGTL_STRING(":"),
      identifier
    >
{};

struct value_param
  : identifier
{};

struct value_int
  : TAO_PEGTL_STRING("0")
{};

struct value_float
  : TAO_PEGTL_STRING("0.0")
{};

struct property_value
  : sor<
      value_string,
      value_param,
      value_int,
      value_float
    >
{};

struct property_list
  : list<
      property_value,
      seq<
        star<space>,
        string<','>,
        star<space>
      >
    >
{};

struct property_grammar
  : seq<
      property_name,
      opt<
        star<space>, ///// NEEDS TO BE LINE SPACES ONLY
        property_list
      >
    >
{};

struct file_grammar
  : seq<
      star<
        ignore_line,
        eol
      >,
      action<
        signature_action,
        state<
          signature,
          signature_grammar
        >
      >, eol,
      action<
        property_action,
        star<
          state<
            property,
            property_grammar
          >,
          eolf
        >
      >
    >
{};

template <>
struct property_action<property_name> {
  template <typename Input>
  static void apply(Input const& in, property& prop) {
    std::cout << "woo: " << in.string() << '\n';
  }
};

template <>
struct signature_action<interface_name> {
  template <typename Input>
  static void apply(Input const& in, signature& sig) {
    sig.name = in.string();
  }
};

template <>
struct signature_action<type_name> {
  template <typename Input>
  static void apply(Input const& in, signature& sig) {
    sig.return_type = data_type_from_string(in.string());
  }
};

template <>
struct param_action<interface_name> {
  template <typename Input>
  static void apply(Input const& in, signature& sig) {
    sig.parameters.back().name = in.string();
  }
};

template <>
struct param_action<pointers> {
  template <typename Input>
  static void apply(Input const& in, signature& sig) {
    sig.parameters.back().pointer_depth = in.string().length();
  }
};

template <>
struct param_action<type_name> {
  template <typename Input>
  static void apply(Input const& in, signature& sig) {
    sig.parameters.emplace_back();
    auto type = data_type_from_string(in.string());
    if(type) {
      sig.parameters.back().type = type.value();
    }
  }
};

signature signature::parse(std::string_view str)
{
  signature sig;
  pegtl::parse<must<signature_grammar, eof>, 
               signature_action>
  (
    string_input(str, ""), sig
  );
  return sig;
}

property property::parse(std::string_view str)
{
  property prop;
  pegtl::parse<must<property_grammar, eolf>
              >
  (
    string_input(str, ""), prop
  );
  return prop;
}

property_set property_set::parse(std::string_view str)
{
  property_set pset;
  pegtl::parse<must<file_grammar>
              >
  (
    string_input(str, ""), pset
  );
  return pset;
}

}
