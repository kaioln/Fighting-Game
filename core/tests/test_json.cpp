#include "fg/json.h"
#include "test.h"

using namespace fg;

TEST("json: objeto, lista, inteiros, texto, comentários") {
  auto r = json::parse("{ // comentário\n \"a\": 1, \"b\": [true, false, null, -42], \"c\": \"x\\u00e9\" }");
  REQUIRE(r.ok);
  CHECK(r.value.find("a")->as_int() == 1);
  CHECK(r.value.find("b")->items().size() == 4);
  CHECK(r.value.find("b")->items()[3].as_int() == -42);
  CHECK(r.value.find("c")->as_string() == "x\xc3\xa9");
}

TEST("json: rejeita números fracionários nos dados") {
  auto r = json::parse("{\"a\": 1.5}");
  CHECK(!r.ok);
  CHECK(r.error.find("fracion") != std::string::npos);
}

TEST("json: rejeita campos duplicados e lixo após o valor") {
  CHECK(!json::parse("{\"a\": 1, \"a\": 2}").ok);
  CHECK(!json::parse("{\"a\": 1} x").ok);
  CHECK(!json::parse("[1, 2").ok);
  CHECK(!json::parse("\"sem fim").ok);
}

TEST("json: erro informa linha e coluna") {
  auto r = json::parse("{\n  \"a\": ?\n}");
  CHECK(!r.ok);
  CHECK(r.error.rfind("2:", 0) == 0);
}
