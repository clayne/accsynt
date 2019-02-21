#include <interfind/finder.h>
#include <interfind/region.h>

#include <passes/passes.h>

#include <support/call_wrapper.h>

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/Transforms/Utils/Cloning.h>

#include <nlohmann/json.hpp>

using namespace support;
using namespace llvm;
using namespace props;
using json = nlohmann::json;
namespace fs = std::filesystem;

namespace interfind {

finder::finder(llvm::Module& mod, nlohmann::json conf) :
  module_(mod),
  signature_(signature::parse(conf["signature"].get<std::string>())),
  library_path_(conf["library_path"].get<std::string>()),
  dynamic_library_(library_path_.string())
{
  if(!fs::exists(library_path_)) {
    throw std::runtime_error("No such shared library");
  }
}

analysis_result finder::run(Module& mod, json config)
{
  auto find = finder(mod, config);

  auto reference = call_wrapper(
    find.signature_, mod, find.signature_.name,
    find.dynamic_library_);

  auto sig_t = find.signature_.function_type();

  auto fns = std::vector<Function *>{};

  for(auto& fn : mod) {
    if(!fn.isDeclaration()) {
      fns.push_back(&fn);
    }
  }

  for(auto fn : fns) {
    createDeduplicatePass()->runOnFunction(*fn);

    auto rf = region_finder(*fn, sig_t);

    for(auto cand : rf.all_candidates()) {
      auto f = cand.extract();
      auto wrapped = call_wrapper(find.signature_, mod, f->getName());

      auto ref_b = reference.get_builder();
      auto wrap_b = wrapped.get_builder();

      ref_b.add(3.0f);
      ref_b.add(7.0f);
      wrap_b.add(3.0f);
      wrap_b.add(7.0f);

      auto rr = reference.call(ref_b);
      auto wr = wrapped.call(wrap_b);

      f->removeFromParent();
    }
  }

  return {};
}

}