#include "json.h"

#include <cctype>
#include <cstdlib>

namespace {

struct Parser {
    const std::string& s;
    size_t i = 0;
    std::string* err = nullptr;

    char Peek() const { return i < s.size() ? s[i] : '\0'; }
    char Get() { return i < s.size() ? s[i++] : '\0'; }

    void SkipWs(){
        while(i < s.size() && std::isspace(static_cast<unsigned char>(s[i]))){
            i++;
        }
    }

    bool Expect(char c){
        SkipWs();
        if(Peek() == c){
            i++;
            return true;
        }
        return false;
    }

    bool Fail(const char* msg){
        if(err){
            *err = msg;
        }
        return false;
    }

    bool ParseValue(JsonValue* out){
        SkipWs();
        char c = Peek();
        if(c == '{') return ParseObject(out);
        if(c == '[') return ParseArray(out);
        if(c == '"') return ParseString(out);
        if(c == 't' || c == 'f') return ParseBool(out);
        if(c == 'n') return ParseNull(out);
        if(c == '-' || std::isdigit(static_cast<unsigned char>(c))) return ParseNumber(out);
        return Fail("unexpected token");
    }

    bool ParseObject(JsonValue* out){
        if(!Expect('{')) return Fail("expected {");
        out->type = JsonValue::Type::Object;
        SkipWs();
        if(Expect('}')) return true;
        while(true){
            JsonValue key;
            if(!ParseString(&key)) return false;
            SkipWs();
            if(!Expect(':')) return Fail("expected :");
            JsonValue value;
            if(!ParseValue(&value)) return false;
            out->obj[key.str] = value;
            SkipWs();
            if(Expect('}')) break;
            if(!Expect(',')) return Fail("expected ,");
        }
        return true;
    }

    bool ParseArray(JsonValue* out){
        if(!Expect('[')) return Fail("expected [");
        out->type = JsonValue::Type::Array;
        SkipWs();
        if(Expect(']')) return true;
        while(true){
            JsonValue value;
            if(!ParseValue(&value)) return false;
            out->arr.push_back(value);
            SkipWs();
            if(Expect(']')) break;
            if(!Expect(',')) return Fail("expected ,");
        }
        return true;
    }

    bool ParseString(JsonValue* out){
        if(!Expect('"')) return Fail("expected string");
        out->type = JsonValue::Type::String;
        std::string result;
        while(i < s.size()){
            char c = Get();
            if(c == '"') break;
            if(c == '\\'){
                char n = Get();
                if(n == '"' || n == '\\' || n == '/') result.push_back(n);
                else if(n == 'b') result.push_back('\b');
                else if(n == 'f') result.push_back('\f');
                else if(n == 'n') result.push_back('\n');
                else if(n == 'r') result.push_back('\r');
                else if(n == 't') result.push_back('\t');
                else return Fail("unsupported escape");
            }else{
                result.push_back(c);
            }
        }
        out->str = result;
        return true;
    }

    bool ParseBool(JsonValue* out){
        SkipWs();
        if(s.compare(i, 4, "true") == 0){
            i += 4;
            out->type = JsonValue::Type::Bool;
            out->b = true;
            return true;
        }
        if(s.compare(i, 5, "false") == 0){
            i += 5;
            out->type = JsonValue::Type::Bool;
            out->b = false;
            return true;
        }
        return Fail("expected bool");
    }

    bool ParseNull(JsonValue* out){
        SkipWs();
        if(s.compare(i, 4, "null") == 0){
            i += 4;
            out->type = JsonValue::Type::Null;
            return true;
        }
        return Fail("expected null");
    }

    bool ParseNumber(JsonValue* out){
        SkipWs();
        size_t start = i;
        if(Peek() == '-') i++;
        while(std::isdigit(static_cast<unsigned char>(Peek()))) i++;
        if(Peek() == '.'){
            i++;
            while(std::isdigit(static_cast<unsigned char>(Peek()))) i++;
        }
        if(Peek() == 'e' || Peek() == 'E'){
            i++;
            if(Peek() == '+' || Peek() == '-') i++;
            while(std::isdigit(static_cast<unsigned char>(Peek()))) i++;
        }
        if(i == start) return Fail("expected number");
        out->type = JsonValue::Type::Number;
        out->num = std::strtod(s.c_str() + start, nullptr);
        return true;
    }
};

} // namespace

bool ParseJson(const std::string& text, JsonValue* out, std::string* err)
{
    Parser p{text};
    p.err = err;
    if(!p.ParseValue(out)){
        return false;
    }
    p.SkipWs();
    if(p.i != text.size()){
        if(err){
            *err = "trailing characters";
        }
        return false;
    }
    return true;
}
