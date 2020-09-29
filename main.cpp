#include <iostream>
#include <string>
#include <utility>

struct token {
    enum {
        number, single, word
    } tag;
};

struct token_digit : token {
    int value;

    explicit token_digit(int v) : token({.tag=number}), value(v) {};
};

struct token_single : token {
    int value;

    explicit token_single(int v) : token({.tag=single}), value(v) {};
};

struct token_word : token {
    std::string &value;

    explicit token_word(std::string &v) : token({.tag=word}), value(v) {};
};

token *lookahead = nullptr;

token *scan() {
    // skip blank
    while (isblank(std::cin.peek()))
        std::cin.get();
    // handle digit
    if (isdigit(std::cin.peek())) {
        int v = 0;
        do v = v * 10 + std::cin.get() - '0';
        while (isdigit(std::cin.peek()));
        return new token_digit(v);
    }
    // handle word
    if (int c = std::cin.peek();
            islower(c) || isupper(c) || c == '_') {
        std::string word;
        do word += (char) std::cin.get();
        while (c = std::cin.peek(), (islower(c) || isupper(c) || isdigit(c) || c == '_'));
        return new token_word(word);
    }
    return new token_single(std::cin.get());
}

/* term     ->  term * factor
 *          |   term / factor
 *          |   factor
 * expr     ->  expr + term
 *          |   expr - term
 *          |   term
 * factor   -> digit | (expr)
 * * * * * * * * * * * * * * * * *
 * term     ->  factor rest1
 * rest1    ->  * factor rest1
 *          |   / factor rest1
 *          |   ε
 * expr     ->  term rest2
 * rest2    ->  + term rest2
 *          |   - term rest2
 *          |   ε
 * factor   ->  digit | (expr)
 * */

void parse_match(token *t) {
    if (lookahead == t) lookahead = scan();
    else throw std::exception("syntax error");
}

void parse_expr();

void parse_term();

void parse_factor() {
    if (lookahead->tag == token::number) {
        std::cout << ((token_digit *) lookahead)->value;
        parse_match(lookahead);
    } else parse_expr();
}

void parse_rest1() {
    if (lookahead->tag == token::single) {
        auto lookahead_single = (token_single *) lookahead;
        switch (lookahead_single->value) {
            case '*':
                parse_match(lookahead);
                parse_term();
                std::cout.put('*');
                parse_rest1();
                break;
            case '/':
                parse_match(lookahead);
                parse_term();
                std::cout.put('/');
                parse_rest1();
                break;
            default:
                /* do nothing */;
        }
    } else {/* do nothing */}
}

void parse_rest2() {
    if (lookahead->tag == token::single) {
        auto lookahead_single = (token_single *) lookahead;
        switch (lookahead_single->value) {
            case '+':
                parse_match(lookahead);
                parse_term();
                std::cout.put('+');
                parse_rest2();
                break;
            case '-':
                parse_match(lookahead);
                parse_term();
                std::cout.put('-');
                parse_rest2();
                break;
            default:
                /* do nothing */;
        }
    } else {/* do nothing */}
}

void parse_term() {
    parse_factor();
    parse_rest1();
}

void parse_expr() {
    parse_term();
    parse_rest2();
}

int main() {
    std::cout << "Hello, World!" << std::endl;
    try {
        delete lookahead;
        lookahead = scan();
        parse_expr();
    } catch (std::exception &e) {
        std::cout << e.what();
    }
    std::cout.put('\n');
    return 0;
}
