// Parser JSON mínimo e estrito, usado apenas no carregamento de dados (fora da simulação).
// Números são inteiros (os dados de jogo não usam ponto flutuante).
#pragma once
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "fg/types.h"

namespace fg::json {

enum class Type { Null, Bool, Int, String, Array, Object };

class Value {
 public:
  Value() = default;
  static Value make_bool(bool b);
  static Value make_int(i64 v);
  static Value make_string(std::string s);
  static Value make_array();
  static Value make_object();

  Type type() const { return type_; }
  bool is_null() const { return type_ == Type::Null; }
  bool is_bool() const { return type_ == Type::Bool; }
  bool is_int() const { return type_ == Type::Int; }
  bool is_string() const { return type_ == Type::String; }
  bool is_array() const { return type_ == Type::Array; }
  bool is_object() const { return type_ == Type::Object; }

  bool as_bool() const { return bool_; }
  i64 as_int() const { return int_; }
  const std::string& as_string() const { return str_; }
  const std::vector<Value>& items() const { return arr_; }
  const std::vector<std::pair<std::string, Value>>& members() const { return obj_; }

  // Retorna nullptr se a chave não existir (ou se não for objeto).
  const Value* find(const std::string& key) const;

  std::vector<Value>& mutable_items() { return arr_; }
  std::vector<std::pair<std::string, Value>>& mutable_members() { return obj_; }

 private:
  Type type_ = Type::Null;
  bool bool_ = false;
  i64 int_ = 0;
  std::string str_;
  std::vector<Value> arr_;
  std::vector<std::pair<std::string, Value>> obj_;  // ordem preservada
};

struct ParseResult {
  bool ok = false;
  Value value;
  std::string error;  // "linha:coluna: mensagem"
};

ParseResult parse(const std::string& text);

}  // namespace fg::json
