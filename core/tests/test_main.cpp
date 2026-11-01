#include <cstdio>
#include <cstring>

#include "test.h"

namespace fgtest {

std::vector<TestCase>& registry() {
  static std::vector<TestCase> r;
  return r;
}

static int g_failures = 0;
static bool g_current_failed = false;

void report_failure(const char* file, int line, const std::string& msg) {
  std::printf("    FALHA %s:%d: %s\n", file, line, msg.c_str());
  ++g_failures;
  g_current_failed = true;
}

}  // namespace fgtest

int main(int argc, char** argv) {
  const char* filter = argc > 1 ? argv[1] : nullptr;
  int run = 0, failed = 0;
  for (const auto& t : fgtest::registry()) {
    if (filter && std::strstr(t.name, filter) == nullptr) continue;
    fgtest::g_current_failed = false;
    t.fn();
    ++run;
    if (fgtest::g_current_failed) ++failed;
    std::printf("[%s] %s\n", fgtest::g_current_failed ? "FALHOU" : "  ok  ", t.name);
  }
  std::printf("\n%d testes, %d falharam, %d verificações com falha\n", run, failed, fgtest::g_failures);
  return failed == 0 ? 0 : 1;
}
