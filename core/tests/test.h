// Framework de testes mínimo (sem dependências externas).
#pragma once
#include <functional>
#include <sstream>
#include <string>
#include <vector>

namespace fgtest {

struct TestCase {
  const char* name;
  std::function<void()> fn;
};

std::vector<TestCase>& registry();
void report_failure(const char* file, int line, const std::string& msg);

struct Registrar {
  Registrar(const char* name, std::function<void()> fn) { registry().push_back({name, std::move(fn)}); }
};

}  // namespace fgtest

#define FG_CONCAT2(a, b) a##b
#define FG_CONCAT(a, b) FG_CONCAT2(a, b)
#define TEST(name)                                                              \
  static void FG_CONCAT(test_fn_, __LINE__)();                                  \
  static fgtest::Registrar FG_CONCAT(test_reg_, __LINE__)(name, &FG_CONCAT(test_fn_, __LINE__)); \
  static void FG_CONCAT(test_fn_, __LINE__)()

#define CHECK(cond)                                                    \
  do {                                                                 \
    if (!(cond)) fgtest::report_failure(__FILE__, __LINE__, #cond);    \
  } while (0)

#define CHECK_EQ(a, b)                                                                     \
  do {                                                                                     \
    const auto fg_va = (a);                                                                \
    const auto fg_vb = (b);                                                                \
    if (!(fg_va == fg_vb)) {                                                               \
      std::ostringstream fg_ss;                                                            \
      fg_ss << #a << " == " << #b << "  (" << +fg_va << " vs " << +fg_vb << ")";           \
      fgtest::report_failure(__FILE__, __LINE__, fg_ss.str());                             \
    }                                                                                      \
  } while (0)

#define REQUIRE(cond)                                                  \
  do {                                                                 \
    if (!(cond)) {                                                     \
      fgtest::report_failure(__FILE__, __LINE__, #cond);               \
      return;                                                          \
    }                                                                  \
  } while (0)
