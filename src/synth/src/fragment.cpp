#include "fragment.h"
#include "linear_fragment.h"

#include <support/choose.h>
#include <support/value_ptr.h>

#include <llvm/IR/Constant.h>
#include <llvm/IR/Function.h>

#include <algorithm>
#include <numeric>

using namespace support;
using namespace llvm;
using namespace props;

namespace synth {

fragment::frag_set fragment::enumerate(
    std::vector<fragment::frag_ptr> const& fragments,
    std::optional<size_t> max_size, size_t data_blocks)
{
  if (max_size && max_size.value() == 0) {
    return {};
  }

  auto control = [&] {
    if (!max_size) {
      return enumerate_all(fragments, max_size);
    } else {
      auto all = fragment::frag_set{};
      for (auto i = 0u; i < max_size.value(); ++i) {
        auto deep = enumerate_all(fragments, i + 1);
        all.merge(std::move(deep));
      }
      return all;
    }
  }();

  auto results = fragment::frag_set{};

  for (auto const& cf : control) {
    auto holes = cf->count_holes();
    auto vec = std::vector<fragment::frag_ptr>{};

    for (auto i = 0u; i < holes; ++i) {
      if (i < data_blocks) {
        vec.emplace_back(new linear_fragment({}));
      } else {
        vec.emplace_back(new empty_fragment({}));
      }
    }

    std::sort(vec.begin(), vec.end());

    do {
      auto frag_copy = cf;

      for (auto i = 0u; i < holes; ++i) {
        frag_copy->add_child(vec.at(i), 0);
      }

      results.insert(frag_copy);
    } while (std::next_permutation(vec.begin(), vec.end()));
  }

  return results;
}

fragment::frag_set fragment::enumerate_all(
    std::vector<fragment::frag_ptr> const& fragments,
    std::optional<size_t> max_size)
{
  auto ret = fragment::frag_set{};
  auto real_max
      = std::min(max_size.value_or(fragments.size()), fragments.size());

  ::support::choose(fragments.size(), real_max).for_each([&](auto idxs) {
    auto perm = std::vector<fragment::frag_ptr>{};
    for (auto idx : idxs) {
      perm.push_back(fragments.at(idx));
    }

    auto all_for_perm = enumerate_permutation(perm);
    for (auto&& frag : all_for_perm) {
      ret.insert(frag);
    }
  });

  return ret;
}

fragment::frag_set fragment::enumerate_permutation(
    std::vector<frag_ptr> const& perm)
{
  if (perm.empty()) {
    return {};
  }

  auto ret = fragment::frag_set{};

  auto begin = std::next(perm.begin());
  auto end = perm.end();
  auto accum = perm.at(0);

  enumerate_recursive(ret, accum, begin, end);
  return ret;
}

fragment::fragment(std::vector<value> args)
    : args_(args)
{
}

compile_metadata fragment::compile(compile_context& ctx)
{
  splice(ctx, ctx.entry_, ctx.exit_);
  return ctx.metadata_;
}

size_t fragment::count_or_empty(fragment::frag_ptr const& frag)
{
  if (frag) {
    return frag->count_holes();
  } else {
    return 1;
  }
}

std::string fragment::string_or_empty(frag_ptr const& frag, size_t ind)
{
  if (frag) {
    return frag->to_str(ind);
  } else {
    return fmt::format("{}[?]", ::support::indent{ ind });
  }
}

bool fragment_equal::operator()(
    value_ptr<fragment> const& a, value_ptr<fragment> const& b) const
{
  return a->to_str() == b->to_str();
  /* return a->equal_to(b); */
}
}

size_t std::hash<value_ptr<synth::fragment>>::operator()(
    value_ptr<synth::fragment> const& frag) const noexcept
{
  return std::hash<std::string>{}(frag->to_str());
}
