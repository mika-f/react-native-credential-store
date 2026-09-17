#include "Namespace.h"
#include "CredentialStore.h"
#include <windows.h>
#include <cstdio>
#include <set>
#include <vector>
#include <tuple>
#include <atomic>
#include <thread>

using namespace rnssecurestore;
static void check(bool value, const char *label) {
  if (!value)
    throw std::runtime_error(label);
}
static void units() {
  check(hashNamespaceComponent("token") ==
            "3c469e9d6c5875d37a43f353d4f88e61fcf812c66eee3457465a40b0da4153e0",
        "SHA-256 known vector");
  check(hashNamespaceComponent("") ==
            "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
        "empty hash");
  check(hashNamespaceComponent("認証") == hashNamespaceComponent("\xe8\xaa\x8d\xe8\xa8\xbc"),
        "exact UTF-8 hash input");
  check(hashNamespaceComponent("/") != hashNamespaceComponent("%2F"), "not percent-encoded input");
  check(hashNamespaceComponent("é") != hashNamespaceComponent("e\xcc\x81"), "no normalization");
  std::set<std::wstring> targets;
  for (const auto &tuple : std::vector<std::tuple<std::string, std::string, std::string>>{
           {"app", "service", "token"},
           {"app", "service", "Token"},
           {"app", "Service", "token"},
           {"App", "service", "token"}}) {
    auto target =
        buildWindowsTargetName(std::get<0>(tuple), std::get<1>(tuple), std::get<2>(tuple));
    check(target.size() == 214, "fixed target length");
    targets.insert(target);
  }
  check(targets.size() == 4, "logical case sensitivity");
  try {
    validateUtf8("\xff");
    check(false, "invalid UTF-8 accepted");
  } catch (const Error &e) {
    check(e.code == "E_ENCODING", "encoding error");
  }
  try {
    buildWindowsTargetName("", "s", "k");
    check(false, "empty identity accepted");
  } catch (const Error &e) {
    check(e.code == "E_INVALID_APPLICATION_ID", "identity error");
  }
  check(defaultApplicationId() == defaultApplicationId() && !defaultApplicationId().empty(),
        "stable default identity");
  puts("Windows native namespace and validation tests passed");
}

static void integration() {
  std::string app = "rnssecurestore.tests." + std::to_string(GetCurrentProcessId()) + "." +
                    std::to_string(GetTickCount64());
  struct Cleanup {
    std::vector<std::tuple<std::string, std::string, std::string>> items;
    ~Cleanup() {
      for (const auto &[key, service, id] : items) {
        try {
          removeItem(key, service, id);
        } catch (...) {
          fputs("Test credential cleanup failed\n", stderr);
        }
      }
    }
  } cleanup;
  auto set = [&](const std::string &key, const std::string &value, const std::string &service,
                 const std::string &id) {
    cleanup.items.emplace_back(key, service, id);
    setItem(key, value, service, id);
  };
  check(!getItem("missing", "default", app), "missing get");
  check(!hasItem("missing", "default", app), "missing has");
  removeItem("missing", "default", app);
  set("token", "one", "default", app);
  check(getItem("token", "default", app) == "one", "set/get");
  check(hasItem("token", "default", app), "existing has");
  set("token", "two", "default", app);
  check(getItem("token", "default", app) == "two", "overwrite");
  for (const auto &value : std::vector<std::string>{"", "認証🔑", std::string("a\0b", 3)}) {
    set("value", value, "default", app);
    check(getItem("value", "default", app) == value && hasItem("value", "default", app),
          "value round trip");
  }
  std::string boundary;
  for (int i = 0; i < 853; i++)
    boundary += "あ";
  for (int i = 0; i < 2; i++) {
    set("boundary", boundary, "default", app);
    check(getItem("boundary", "default", app) == boundary, "byte boundary");
    boundary += "x";
  }
  try {
    setItem("boundary", boundary, "default", app);
    check(false, "oversized accepted");
  } catch (const Error &e) {
    check(e.code == "E_VALUE_TOO_LARGE", "size error");
  }
  auto tuples = std::vector<std::tuple<std::string, std::string, std::string>>{
      {"token", "default", app}, {"Token", "default", app},
      {"token", "Default", app}, {"token", "default", "R" + app.substr(1)},
      {"é", "認証", app},        {"e\xcc\x81", "認証", app}};
  for (size_t i = 0; i < tuples.size(); i++) {
    const auto &[k, s, a] = tuples[i];
    set(k, std::to_string(i), s, a);
  }
  for (size_t i = 0; i < tuples.size(); i++) {
    const auto &[k, s, a] = tuples[i];
    check(getItem(k, s, a) == std::to_string(i), "namespace isolation");
  }
  set("concurrent", "initial", "default", app);
  std::atomic<bool> failed{false};
  std::vector<std::thread> threads;
  for (int i = 0; i < 16; i++)
    threads.emplace_back([&, i] {
      try {
        setItem("concurrent", std::to_string(i), "default", app);
      } catch (...) {
        failed = true;
      }
    });
  for (auto &thread : threads)
    thread.join();
  check(!failed && hasItem("concurrent", "default", app), "concurrent writes");
  removeItem("token", "default", app);
  removeItem("token", "default", app);
  check(!getItem("token", "default", app) && !hasItem("token", "default", app), "removal");
  puts("Windows Credential Manager integration tests passed");
}
int main(int argc, char **argv) {
  try {
    units();
    if (argc > 1 && std::string(argv[1]) == "--integration")
      integration();
  } catch (const Error &error) {
    fprintf(stderr, "FAIL: %s (%lu)\n", error.code.c_str(), error.nativeCode);
    return 1;
  } catch (const std::exception &error) {
    fprintf(stderr, "FAIL: %s\n", error.what());
    return 1;
  }
}
