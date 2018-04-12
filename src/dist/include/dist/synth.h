#pragma once

#include <dist/contexts.h>

#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Module.h>

#include <forward_list>
#include <thread>
#include <tuple>

namespace accsynt {

template <typename R, typename... Args>
class Synthesizer {
public:
  using ret_t = typename R::example_t;
  using args_t = std::tuple<typename Args::example_t...>;
  using io_pair_t = std::pair<ret_t, args_t>;

  Synthesizer(R r, Args... args) :
    return_type_(r), arg_types_(args...) {}

  virtual std::unique_ptr<llvm::Module> generate_candidate(bool&) 
  {
    return nullptr;
  };

  std::unique_ptr<llvm::Module> threaded_generate();

  args_t example() const {
    auto ret = args_t{};
    zip_for_each(ret, arg_types_, [&](auto& ex, auto a) {
      ex = a.generate();
    });
    return ret;
  };

  llvm::FunctionType *llvm_function_type() const;

protected:
  R return_type_;
  std::tuple<Args...> arg_types_;
};

template <typename R, typename... Args>
std::unique_ptr<llvm::Module> Synthesizer<R, Args...>::threaded_generate()
{
  auto ret = std::unique_ptr<llvm::Module>{};
  bool done = false;

  auto work = [&] {
    auto cand = this->generate_candidate(done);
    if(cand) {
      ret = std::move(cand);
    }
  };

  auto threads = std::forward_list<std::thread>{};
  auto max_threads = std::max(1u, std::thread::hardware_concurrency() - 1);
  for(auto i = 0u; i < max_threads; ++i) {
    threads.emplace_front(work);
  }

  for(auto& t : threads) {
    t.join();
  }

  return ret;
}


template <typename R, typename... Args>
llvm::FunctionType *Synthesizer<R, Args...>::llvm_function_type() const
{
  auto llvm_arg_tys = std::array<llvm::Type*, sizeof...(Args)>{};
  zip_for_each(arg_types_, llvm_arg_tys, [] (auto a, auto& ll) {
    ll = a.llvm_type();
  });

  auto i64_t = llvm::IntegerType::get(ThreadContext::get(), 64);
  auto ptr_t = llvm::PointerType::getUnqual(i64_t);
  auto arg_tys = std::array<llvm::Type*, sizeof...(Args) + 1>{};
  std::copy(std::begin(llvm_arg_tys), std::end(llvm_arg_tys), std::next(std::begin(arg_tys)));
  arg_tys[0] = ptr_t;

  return llvm::FunctionType::get(return_type_.llvm_type(), arg_tys, false);
}


}
