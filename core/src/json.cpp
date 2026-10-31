#include "fg/json.h"

#include <cctype>
#include <limits>

namespace fg::json {

Value Value::make_bool(bool b) {
  Value v;
  v.type_ = Type::Bool;
  v.bool_ = b;
  return v;
}
Value Value::make_int(i64 i) {
  Value v;
  v.type_ = Type::Int;
  v.int_ = i;
  return v;
}
Value Value::make_string(std::string s) {
  Value v;
  v.type_ = Type::String;
  v.str_ = std::move(s);
  return v;
}
Value Value::make_array() {
  Value v;
  v.type_ = Type::Array;
  return v;
}
Value Value::make_object() {
  Value v;
  v.type_ = Type::Object;
  return v;
}

const Value* Value::find(const std::string& key) const {
  if (type_ != Type::Object) return nullptr;
  for (const auto& [k, val] : obj_) {
    if (k == key) return &val;
  }
  return nullptr;
}

namespace {

class Parser {
 public:
  explicit Parser(const std::string& t) : text_(t) {}

  ParseResult run() {
    ParseResult r;
    skip_ws();
    if (!parse_value(r.value, 0)) {
      r.error = error_;
      return r;
    }
    skip_ws();
    if (pos_ != text_.size()) {
      fail("conteúdo extra após o valor JSON");
      r.error = error_;
      return r;
    }
    r.ok = true;
    return r;
  }

 private:
  static constexpr int kMaxDepth = 64;
  const std::string& text_;
  size_t pos_ = 0;
  std::string error_;

  bool fail(const std::string& msg) {
    if (!error_.empty()) return false;
    size_t line = 1, col = 1;
    for (size_t i = 0; i < pos_ && i < text_.size(); ++i) {
      if (text_[i] == '\n') {
        ++line;
        col = 1;
      } else {
        ++col;
      }
    }
    error_ = std::to_string(line) + ":" + std::to_string(col) + ": " + msg;
    return false;
  }

  void skip_ws() {
    while (pos_ < text_.size()) {
      const char c = text_[pos_];
      if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
        ++pos_;
      } else if (c == '/' && pos_ + 1 < text_.size() && text_[pos_ + 1] == '/') {
        // Extensão: comentários de linha são aceitos para documentar dados.
        while (pos_ < text_.size() && text_[pos_] != '\n') ++pos_;
      } else {
        break;
      }
    }
  }

  bool parse_value(Value& out, int depth) {
    if (depth > kMaxDepth) return fail("aninhamento profundo demais");
    if (pos_ >= text_.size()) return fail("fim inesperado");
    const char c = text_[pos_];
    if (c == '{') return parse_object(out, depth);
    if (c == '[') return parse_array(out, depth);
    if (c == '"') {
      std::string s;
      if (!parse_string(s)) return false;
      out = Value::make_string(std::move(s));
      return true;
    }
    if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) return parse_number(out);
    if (text_.compare(pos_, 4, "true") == 0) {
      pos_ += 4;
      out = Value::make_bool(true);
      return true;
    }
    if (text_.compare(pos_, 5, "false") == 0) {
      pos_ += 5;
      out = Value::make_bool(false);
      return true;
    }
    if (text_.compare(pos_, 4, "null") == 0) {
      pos_ += 4;
      out = Value();
      return true;
    }
    return fail(std::string("caractere inesperado '") + c + "'");
  }

  bool parse_number(Value& out) {
    const size_t start = pos_;
    bool neg = false;
    if (text_[pos_] == '-') {
      neg = true;
      ++pos_;
    }
    if (pos_ >= text_.size() || !std::isdigit(static_cast<unsigned char>(text_[pos_]))) {
      return fail("número inválido");
    }
    i64 v = 0;
    while (pos_ < text_.size() && std::isdigit(static_cast<unsigned char>(text_[pos_]))) {
      const i64 d = text_[pos_] - '0';
      if (v > (std::numeric_limits<i64>::max() - d) / 10) return fail("número grande demais");
      v = v * 10 + d;
      ++pos_;
    }
    if (pos_ < text_.size() && (text_[pos_] == '.' || text_[pos_] == 'e' || text_[pos_] == 'E')) {
      pos_ = start;
      return fail("números fracionários não são permitidos nos dados (use inteiros)");
    }
    out = Value::make_int(neg ? -v : v);
    return true;
  }

  bool parse_string(std::string& out) {
    ++pos_;  // aspas iniciais
    while (pos_ < text_.size()) {
      const char c = text_[pos_++];
      if (c == '"') return true;
      if (c == '\\') {
        if (pos_ >= text_.size()) return fail("escape incompleto");
        const char e = text_[pos_++];
        switch (e) {
          case '"': out.push_back('"'); break;
          case '\\': out.push_back('\\'); break;
          case '/': out.push_back('/'); break;
          case 'n': out.push_back('\n'); break;
          case 't': out.push_back('\t'); break;
          case 'r': out.push_back('\r'); break;
          case 'b': out.push_back('\b'); break;
          case 'f': out.push_back('\f'); break;
          case 'u': {
            if (pos_ + 4 > text_.size()) return fail("escape \\u incompleto");
            u32 cp = 0;
            for (int i = 0; i < 4; ++i) {
              const char h = text_[pos_++];
              cp <<= 4;
              if (h >= '0' && h <= '9') cp |= static_cast<u32>(h - '0');
              else if (h >= 'a' && h <= 'f') cp |= static_cast<u32>(h - 'a' + 10);
              else if (h >= 'A' && h <= 'F') cp |= static_cast<u32>(h - 'A' + 10);
              else return fail("hex inválido em \\u");
            }
            // Codifica em UTF-8 (sem pares substitutos: suficiente para dados de jogo).
            if (cp < 0x80) {
              out.push_back(static_cast<char>(cp));
            } else if (cp < 0x800) {
              out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
              out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
            } else {
              out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
              out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
              out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
            }
            break;
          }
          default: return fail("escape desconhecido");
        }
      } else if (static_cast<unsigned char>(c) < 0x20) {
        return fail("caractere de controle dentro de string");
      } else {
        out.push_back(c);
      }
    }
    return fail("string não terminada");
  }

  bool parse_array(Value& out, int depth) {
    ++pos_;
    out = Value::make_array();
    skip_ws();
    if (pos_ < text_.size() && text_[pos_] == ']') {
      ++pos_;
      return true;
    }
    while (true) {
      skip_ws();
      Value item;
      if (!parse_value(item, depth + 1)) return false;
      out.mutable_items().push_back(std::move(item));
      skip_ws();
      if (pos_ >= text_.size()) return fail("array não terminado");
      if (text_[pos_] == ',') {
        ++pos_;
        continue;
      }
      if (text_[pos_] == ']') {
        ++pos_;
        return true;
      }
      return fail("esperado ',' ou ']'");
    }
  }

  bool parse_object(Value& out, int depth) {
    ++pos_;
    out = Value::make_object();
    skip_ws();
    if (pos_ < text_.size() && text_[pos_] == '}') {
      ++pos_;
      return true;
    }
    while (true) {
      skip_ws();
      if (pos_ >= text_.size() || text_[pos_] != '"') return fail("esperado nome de campo");
      std::string key;
      if (!parse_string(key)) return false;
      if (out.find(key) != nullptr) return fail("campo duplicado '" + key + "'");
      skip_ws();
      if (pos_ >= text_.size() || text_[pos_] != ':') return fail("esperado ':'");
      ++pos_;
      skip_ws();
      Value v;
      if (!parse_value(v, depth + 1)) return false;
      out.mutable_members().emplace_back(std::move(key), std::move(v));
      skip_ws();
      if (pos_ >= text_.size()) return fail("objeto não terminado");
      if (text_[pos_] == ',') {
        ++pos_;
        continue;
      }
      if (text_[pos_] == '}') {
        ++pos_;
        return true;
      }
      return fail("esperado ',' ou '}'");
    }
  }
};

}  // namespace

ParseResult parse(const std::string& text) { return Parser(text).run(); }

}  // namespace fg::json
