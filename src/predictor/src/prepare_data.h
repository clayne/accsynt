#pragma once

#include <props/props.h>

#include <support/utility.h>

#include <fmt/format.h>

#include <iterator>
#include <map>
#include <set>
#include <string>
#include <type_traits>
#include <vector>

namespace predict {

using feature_map = std::map<std::string, int>;

/**
 * A single instance of example data for a learner to later consume. Belongs to
 * a dataset, which is responsible for inserting missing values etc. when
 * encoding.
 */
class example {
public:
  template <typename Func>
  example(Func&&, props::property_set const&);

  feature_map const& input() const { return input_; }
  feature_map const& output() const { return output_; }

private:
  feature_map input_ = {};
  feature_map output_ = {};
};

/**
 * A collection of examples, along with logic to make sure that missing values
 * etc. are encoded properly.
 *
 * In this model, the only summarisation that needs to be done is mapping
 * property names to classes - can simplify the summarisation code.
 *
 * So this summarisation can be removed and pushed into the dataset class
 * instead - it will just need to make two passes through the data in order to
 * summarise, then encode each individual property set.
 */
class dataset {
public:
  template <typename Iterator>
  dataset(Iterator begin, Iterator end);

  template <typename Container>
  explicit dataset(Container&& c);

  int encode(props::base_type) const;

private:
  constexpr auto prop_encoder() {
    return [this] (auto const& pn) {
      auto found = prop_names_.find(pn);
      return std::distance(prop_names_.begin(), found);
    };
  }

  std::set<std::string> prop_names_ = {};
  std::vector<example> examples_ = {};
};

/**
 * Implementations
 */

template <typename Func>
example::example(Func&& prop_enc, props::property_set const& ps)
{
  if(auto rt = ps.type_signature.return_type) {
  }
}

template <typename Iterator>
dataset::dataset(Iterator begin, Iterator end)
{
  // Summarise the data so that we're able to map property names to unique
  // indices later - this requires a first pass through the data.
  std::for_each(begin, end, [this] (auto const& ps) {
    for(auto const& prop : ps.properties) {
      prop_names_.insert(prop.name);
    }
  });

  // Then construct the set of examples from each property set.
  std::for_each(begin, end, [this] (auto const& ps) {
    examples_.emplace_back(prop_encoder(), ps);
  });
}

template <typename Container>
dataset::dataset(Container&& c) :
  dataset(support::adl_begin(FWD(c)), support::adl_end(FWD(c)))
{
}

}

namespace fmt {
  
template <>
struct formatter<::predict::example> {
  template <typename ParseContext>
  constexpr auto parse(ParseContext &ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(::predict::example const &e, FormatContext &ctx) {
    auto in_entries = std::vector<std::string>{};
    auto out_entries = std::vector<std::string>{};

    for(auto const& [k, v] : e.input()) {
    }

    for(auto const& [k, v] : e.output()) {
    }

    return format_to(ctx.out(), "Example()");
  }
};

template <>
struct formatter<::predict::dataset> {
  template <typename ParseContext>
  constexpr auto parse(ParseContext &ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(::predict::dataset const &d, FormatContext &ctx) {
    return format_to(ctx.out(), "Dataset()");
  }
};

}
