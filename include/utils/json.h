#pragma once 

#include <string>
#include <sstream>
#include <vector>
#include <unordered_map>

namespace json {

    class Value;

    /** The undefined value placeholder. 
     
        Does not contain any useful information apart from the optinal comment, exists for unified creation of values via constructors. 
     */
    class Undefined {
    public:
        std::string const & comment() const { return comment_; }
        void setComment(std::string const & comment) { comment_ = comment; }

    private:
        std::string comment_;
    }; // json::Undefined

    /** The Null value placeholder. 

        Does not contain any useful information apart from the optinal comment, exists for unified creation of values via constructors. 
     */
    class Null {
    public:
        std::string const & comment() const { return comment_; }
        void setComment(std::string const & comment) { comment_ = comment; }
    private:
        std::string comment_;
    }; // json::Null

    /** Boolean JSON value. 
     */
    class Bool {
    public:
        explicit Bool(bool value):value_{value} {}

        std::string const & comment() const { return comment_; }
        void setComment(std::string const & comment) { comment_ = comment; }

        operator bool () const { return value_; } 
    private:
        bool value_;
        std::string comment_;

    }; // json::Boolean

    /** Integer JSON value. 
     
        Contrary to JSON specification numbers are stored as either double, or boolean values. 
     */
    class Int {
    public:
        explicit Int(int value):value_{value} {}

        std::string const & comment() const { return comment_; }
        void setComment(std::string const & comment) { comment_ = comment; }

        operator int () const { return value_; }
    private:
        int value_;
        std::string comment_;

    }; // json::Integer

    /** Double JSON value. 
     
        Contrary to JSON specification numbers are stored as either double, or boolean values. 
     */
    class Double {
    public:
        explicit Double(double value):value_{value} {}

        std::string const & comment() const { return comment_; }
        void setComment(std::string const & comment) { comment_ = comment; }

        operator double () const { return value_; }
    private:
        double value_;
        std::string comment_;

    }; // json::Double

    /** String JSON value. 
     */
    class String {
    public:
        explicit String(std::string const & value):value_{value} {}
        explicit String(char const * value):value_{value} {}
        explicit String(std::string && value):value_{std::move(value)} {}

        std::string const & comment() const { return comment_; }
        void setComment(std::string const & comment) { comment_ = comment; }

        size_t size() const { return value_.size(); }
        char const * c_str() const { return value_.c_str(); }
    private:
        std::string value_;
        std::string comment_;

    }; // json::String

    /** JSON Array. 
     */
    class Array {
    public:

        std::string const & comment() const { return comment_; }
        void setComment(std::string const & comment) { comment_ = comment; }

    private:
        std::vector<Value*> elements_;
        std::string comment_;

    }; // json::Array

    /** JSON Struct.
     */
    class Struct {
    public:

        std::string const & comment() const { return comment_; }
        void setComment(std::string const & comment) { comment_ = comment; }

    private:
        std::unordered_map<std::string, Value *> elements_;
        std::string comment_;
    }; // json::Struct

    /** A Generic JSON value class. 
     
        Provides a tagged union of all JSON values that can be easily manipulated providing some basic conversions as well. 
     */
    class Value {
    public:
        enum class Kind {
            Undefined, 
            Null, 
            Bool, 
            Int,
            Double,
            String,
            Array,
            Struct
        }; // json::Value::Kind

        Value(): kind_{Kind::Undefined}, valueUndefined_{} {}

        Value(Undefined value): kind_{Kind::Undefined}, valueUndefined_{} {}
        Value(Null value): kind_{Kind::Null}, valueNull_{} {}

        Value(bool value): kind_{Kind::Bool}, valueBool_{value} {}
        Value(Bool value): kind_{Kind::Bool}, valueBool_{value} {}

        Value(int value): kind_{Kind::Int}, valueInt_{value} {}
        Value(Int value): kind_{Kind::Int}, valueInt_{value} {}

        Value(double value): kind_{Kind::Double}, valueDouble_{value} {}
        Value(Double value): kind_{Kind::Double}, valueDouble_{value} {}

        Value(std::string const & value): kind_{Kind::String}, valueString_{value} {}
        Value(std::string && value): kind_{Kind::String}, valueString_{std::move(value)} {}
        Value(char const * value): kind_{Kind::String}, valueString_{value} {}
        Value(String const & value): kind_{Kind::String}, valueString_{value} {}
        Value(String && value): kind_{Kind::String}, valueString_{std::move(value)} {}

        Value(Array const & value): kind_{Kind::Array}, valueArray_{value} {}
        Value(Array && value): kind_{Kind::Array}, valueArray_{std::move(value)} {}

        Value(Struct const & value): kind_{Kind::Struct}, valueStruct_{value} {}
        Value(Struct && value): kind_{Kind::Struct}, valueStruct_{std::move(value)} {}

        Value(Value const & from):
            kind_{from.kind_},
            valueUndefined_{} {
            switch (kind_) {
                case Kind::Undefined:
                    break;
                case Kind::Null:
                    new (&valueNull_) Null{};
                    break;
                case Kind::Bool:
                    new (&valueBool_) Bool{from.valueBool_};
                    break;
                case Kind::Int:
                    new (&valueInt_) Int{from.valueInt_};
                    break;
                case Kind::Double:
                    new (&valueDouble_) Double{from.valueDouble_};
                    break;
                case Kind::String:
                    new (&valueString_) String{from.valueString_};
                    break;
                case Kind::Array:
                    new (&valueArray_) Array{from.valueArray_};
                    break;
                case Kind::Struct:
                    new (&valueStruct_) Struct{from.valueStruct_};
            }
        }

        Value(Value && from):
            kind_{from.kind_},
            valueUndefined_{} {
            switch (kind_) {
                case Kind::Undefined:
                    break;
                case Kind::Null:
                    new (&valueNull_) Null{};
                    break;
                case Kind::Bool:
                    new (&valueBool_) Bool{std::move(from.valueBool_)};
                    break;
                case Kind::Int:
                    new (&valueInt_) Int{std::move(from.valueInt_)};
                    break;
                case Kind::Double:
                    new (&valueDouble_) Double{std::move(from.valueDouble_)};
                    break;
                case Kind::String:
                    new (&valueString_) String{std::move(from.valueString_)};
                    break;
                case Kind::Array:
                    new (&valueArray_) Array{std::move(from.valueArray_)};
                    break;
                case Kind::Struct:
                    new (&valueStruct_) Struct{std::move(from.valueStruct_)};
            }
        }

        ~Value() {
            detach();
        }

        std::string const & comment() const {
            switch (kind_) {
                case Kind::Undefined:
                    return valueUndefined_.comment();
                case Kind::Null:
                    return valueNull_.comment();
                case Kind::Bool:
                    return valueBool_.comment();
                case Kind::Int:
                    return valueInt_.comment();
                case Kind::Double:
                    return valueDouble_.comment();
                case Kind::String:
                    return valueString_.comment();
                case Kind::Array:
                    return valueArray_.comment();
                case Kind::Struct:
                    return valueStruct_.comment();
            }
        }

        void setComment(std::string const & comment) {
            switch (kind_) {
                case Kind::Undefined:
                    return valueUndefined_.setComment(comment);
                case Kind::Null:
                    return valueNull_.setComment(comment);
                case Kind::Bool:
                    return valueBool_.setComment(comment);
                case Kind::Int:
                    return valueInt_.setComment(comment);
                case Kind::Double:
                    return valueDouble_.setComment(comment);
                case Kind::String:
                    return valueString_.setComment(comment);
                case Kind::Array:
                    return valueArray_.setComment(comment);
                case Kind::Struct:
                    return valueStruct_.setComment(comment);
            }
        }

        template<typename T> T const & as() const;

        Value & operator = (Value const & other) {
            detach();
            kind_ = other.kind_;
            switch (kind_) {
                case Kind::Undefined:
                    valueUndefined_ = other.valueUndefined_;
                    break;
                case Kind::Null:
                    valueNull_ = other.valueNull_;
                    break;
                case Kind::Bool:
                    valueBool_ = other.valueBool_;
                    break;
                case Kind::Int:
                    valueInt_ = other.valueInt_;
                    break;
                case Kind::Double:
                    valueDouble_ = other.valueDouble_;
                    break;
                case Kind::String:
                    valueString_ = other.valueString_;
                    break;
                case Kind::Array:
                    valueArray_ = other.valueArray_;
                    break;
                case Kind::Struct:
                    valueStruct_ = other.valueStruct_;
                    break;
            }
            return *this;
        }


    private:

        void detach() {
            switch (kind_) {
                case Kind::String:
                    valueString_.~String();
                    break;
                case Kind::Array:
                    valueArray_.~Array();
                    break;
                case Kind::Struct:
                    valueStruct_.~Struct();
                    break;
                default:
                    break; // trivial destructors
            }
        }

        Kind kind_;
        union {
            Undefined valueUndefined_;
            Null valueNull_;
            Bool valueBool_;
            Int valueInt_;
            Double valueDouble_;
            String valueString_;
            Array valueArray_;
            Struct valueStruct_;
        };

        friend Value parse(std::istream &);

        /** A rather simle and permissive JSON parser. 
         
            Aside from the proper JSON it also supports comments, trailing commas, literal names and so on. 
         */
        class Parser {
        public:
            
            class Token {
                friend class Parser;
            public:
                enum class Kind {
                    Undefined, 
                    Null,
                    Bool,
                    Int,
                    Double,
                    String, 
                    Comment, 
                    Identifier,
                    Colon,
                    Comma,
                    SquareOpen,
                    SquareClose,
                    CurlyOpen,
                    CurlyClose,
                }; // json::Value::Parser::Token::Kind

                Token(size_t l, size_t c, Kind kind):
                    line{l}, col{c}, kind{kind}, valueBool_{false} {
                }

                Token(size_t l, size_t c, Kind kind, bool value):
                    line{l}, col{c}, kind{kind}, valueBool_{value} {
                }

                Token(size_t l, size_t c, Kind kind, int value):
                    line{l}, col{c}, kind{kind}, valueInt_{value} {
                }

                Token(size_t l, size_t c, Kind kind, double value):
                    line{l}, col{c}, kind{kind}, valueDouble_{value} {
                }

                Token(size_t l, size_t c, Kind kind, std::string && value):
                    line{l}, col{c}, kind{kind}, valueString_{std::move(value)} {
                }


                ~Token() {
                    using namespace std;
                    switch (kind) {
                        case Kind::Identifier:
                        case Kind::String:
                        case Kind::Comment:
                            valueString_.~string();
                            break;
                    }
                }

                size_t line;
                size_t col;

                Kind kind;

            private:


                union {
                    bool valueBool_;
                    int valueInt_;
                    double valueDouble_;
                    std::string valueString_;
                }; 
                
            }; // json::Value::Parser::Token

            Parser(std::istream & s):
                s_{s} {
            }

            Value parse() {
                return parse(next());
            }

        private:

            Value parse(Token t) {
                switch (t.kind) {
                    case Token::Kind::Comment:
                        return parseWithComment(t.valueString_);
                    case Token::Kind::Undefined:
                        return Undefined{};
                    case Token::Kind::Null:
                        return Null{};
                    case Token::Kind::Bool:
                        return Value{t.valueBool_};
                    case Token::Kind::Int:
                        return Value{t.valueInt_};
                    case Token::Kind::Double:
                        return Value{t.valueDouble_};
                    case Token::Kind::String:
                        return Value{t.valueString_};
                    case Token::Kind::SquareOpen: {
                        Array i{};
                        // TODO

                        return i;
                    }
                    case Token::Kind::CurlyOpen: {
                        Struct i{};

                        // TODO
                        return i;
                    }


                }
            }

            Value parseWithComment(std::string comment) {
                Value result = parse();
                result.setComment(comment);
                return result;
            }

            /** Returns the next token in the input stream. 
             
                " => string
                0-9 => number 
                a-zA-Z_ => identifier
                / => comment

             */
            Token next() {
                while (true) {
                    auto line = l_;
                    auto col = c_;
                    auto c = nextChar();
                    switch (c) {
                        case ':':
                            return Token{l_, c_, Token::Kind::Colon};
                        case ',':
                            return Token{l_, c_, Token::Kind::Comma};
                        case '[':
                            return Token{l_, c_, Token::Kind::SquareOpen};
                        case ']':
                            return Token{l_, c_, Token::Kind::SquareClose};
                        case '{':
                            return Token{l_, c_, Token::Kind::CurlyOpen};
                        case '}':
                            return Token{l_, c_, Token::Kind::CurlyClose};
                        case '/':
                            return Token{l_, c_, Token::Kind::Comment, nextComment(l_, c_)};
                        case '"':
                        case '\'':
                            return Token{l_, c_, Token::Kind::String, nextString(l_, c_, c)};
                        case ' ':
                        case '\t':
                        case '\n':
                        case '\r':
                            // whitespace, just read next
                            break;
                        default:
                            if (isDigit(c))
                                return nextNumber(l_, c_, c);
                            else if (isIdentifierStart(c))
                                return nextIdentifier(l_, c_, c);
                            else
                                error("Valid JSON character");
                    }
                }
            }

            std::string nextComment(size_t l, size_t c) {
                std::string result{};
                switch (nextChar()) {
                    case '/': // single line comment
                        while (! eof()) {
                            char c = nextChar();
                            if (c == '\n')
                                break;
                            result += c;
                        }
                        break;
                    case '*': // multi-line comment 
                        while (true) {
                            if (eof())
                                error(l, c, "Unterminated multi-line comment");
                            char c = nextChar();
                            if (c == '*') {
                                char c2 = nextChar();
                                if (c2 == '/')
                                    break;
                                result += c;
                                result += c2;
                            } else {
                                result += c;
                            }
                        }
                        break;
                    default:
                        error("// or /* comment");
                }
                return result;
            }

            std::string nextString(size_t l, size_t c, char delimiter) {
                std::string result{};
                while (true) {
                    if (eof())
                        error(l, c, "unterminated string literal");
                    char c = nextChar();
                    if (c == '\\') { 
                        char c = nextChar();
                        switch (c) {
                            case '"':
                            case '\'':
                            case '\\':
                                result += c;
                                break;
                            case 't':
                                result += '\t';
                                break;
                            case 'n':
                                result += '\n';
                                break;
                            case 'r':
                                result += '\r';
                                break;
                            case '\n':
                                break;
                            default:
                                error("valid string escape sequence");
                        }
                    } else {
                        result += c;
                    }
                }
                return result;
            }

            /** Parses an identifier. 
             */
            Token nextIdentifier(size_t l, size_t c, char start) {
                std::string result{start};
                while (! eof() && isIdentifier(peekChar()))
                    result += nextChar();
                if (result == "null")
                    return Token{l, c, Token::Kind::Null};
                else if (result == "undefined")
                    return Token{l, c, Token::Kind::Undefined};
                else if (result == "true")
                    return Token{l, c, Token::Kind::Bool, true};
                else if (result == "false")
                    return Token{l, c, Token::Kind::Bool, false};
                else
                    return Token{l, c, Token::Kind::Identifier, std::move(result)};
            }

            Token nextNumber(size_t l, size_t c, char start) {
                int result = (start - '0');
                while (isDigit(peekChar()))
                    result = result * 10 + (nextChar() - '0');
                if (peekChar() == '.') {
                    nextChar();
                    throw "";
                } else {
                    return Token{l, c, Token::Kind::Int, result};
                }
            }

            char nextChar() {
                char result = s_.get();
                if (result == '\r') {
                    ++l_;
                    c_ = 1;
                } else {
                    ++c_;
                }
                return result;
            }

            char peekChar() {
                return s_.peek();
            }

            bool eof() {
                return s_.eof();
            }

            void error(char const * expected) {
                // TODO
            }
            void error(size_t l, size_t c, char const * msg) {
                // TODO
            }

            bool isDigit(char c) { return c > '0' && c <= '9'; }
            bool isIdentifierStart(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'; }
            bool isIdentifier(char c) { return isIdentifierStart(c) || isDigit(c); } 

            // location information for better errors
            size_t l_ = 1;
            size_t c_ = 1;

            std::istream & s_;

        }; // json::Value::Parser

    }; // json::Value

    template<> 
    inline Bool const & Value::as() const {
        if (kind_ != Kind::Bool)
            throw "Expected bool but found";
        return valueBool_;
    }

    template<> 
    inline Int const & Value::as() const {
        if (kind_ != Kind::Int)
            throw "Expected bool but found";
        return valueInt_;
    }

    /** Parses the given stream and returns the JSON object. 
     */
    inline Value parse(std::istream & s) {
        return Value{};
    }

    /** Parses the given string and returns the JSON object. 
     */
    inline Value parse(char const * str) {
        std::stringstream s{str};
        return parse(s);
    }

    // TODO serialize



} // namespace json

#ifdef TESTS




#endif
