#include <iostream>
#include <string>
#include <utility>

struct token {
    enum tag_t {
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

//struct token_word : token {
//    std::string &value;
//
//    explicit token_word(std::string &v) : token({.tag=word}), value(v) {};
//};

struct ast {
    enum {
        expr
    } tag;
};

struct ast_expr : ast {
    enum tag_t {
        num, opt
    } expr_tag;

    explicit ast_expr(enum tag_t t) :
            ast({.tag=expr}),
            expr_tag(t) {};
};

struct ast_number : ast_expr {
    int value;

    explicit ast_number(int v) :
            ast_expr(num),
            value(v) {};
};

struct ast_opt : ast_expr {
    int op;
    ast_expr *left, *right;

    explicit ast_opt(int op, ast_expr *l, ast_expr *r) :
            ast_expr(opt),
            op(op),
            left(l),
            right(r) {};
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
//    int c = std::cin.peek();
//    if (islower(c) || isupper(c) || c == '_') {
//        std::string word;
//        do word += (char) std::cin.get();
//        while (c = std::cin.peek(), (islower(c) || isupper(c) || isdigit(c) || c == '_'));
//        return new token_word(word);
//    }
    return new token_single(std::cin.get());
}

/*
 * * * * left recursive * * * * *
 * term     ->  term * factor
 *          |   term / factor
 *          |   factor
 * expr     ->  expr + term
 *          |   expr - term
 *          |   term
 * factor   -> digit | (expr)
 *
 * * * * right recursive * * * * *
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
    else throw std::logic_error("syntax error");
}

ast_expr *parse_expr();

ast_expr *parse_factor() {
    if (lookahead->tag == token::number) {
        int v = ((token_digit *) lookahead)->value;
        parse_match(lookahead);
        return new ast_number(v);
    } else return parse_expr();
}

ast_expr *parse_term() {
    auto l = parse_factor();
    while (lookahead->tag == token::single) {
        int c = (((token_single *) lookahead)->value);
        if (c == '*' || c == '/') {
            parse_match(lookahead);
            l = new ast_opt(c, l, parse_term());
        } else break;
    }
    return l;
}

ast_expr *parse_expr() {
    auto l = parse_term();
    while (lookahead->tag == token::single) {
        int c = ((token_single *) lookahead)->value;
        if (c == '+' || c == '-') {
            parse_match(lookahead);
            l = new ast_opt(c, l, parse_term());
        } else break;
    }
    return l;
}

int calc(ast_expr *expr) {
    if (expr->expr_tag == ast_expr::num)
        return ((ast_number *) expr)->value;
    else if (expr->expr_tag == ast_expr::opt) {
        auto opt = (ast_opt *) expr;
        switch (opt->op) {
            case '+':
                return calc(opt->left) + calc(opt->right);
            case '-':
                return calc(opt->left) - calc(opt->right);
            case '*':
                return calc(opt->left) * calc(opt->right);
            case '/':
                return calc(opt->left) / calc(opt->right);
        }
    }
    throw std::logic_error("unknown expr_tag");
}

int tmp_count = 0;

int tac(ast_expr *expr) {
    if (expr->expr_tag == ast_expr::opt) {
        auto opt = (ast_opt *) expr;
        int v_left, v_right;
        bool is_num_left = opt->left->expr_tag == ast_expr::num;
        bool is_num_right = opt->right->expr_tag == ast_expr::num;
        v_left = is_num_left ? ((ast_number *) opt->left)->value
                             : tac(((ast_opt *) opt)->left);
        v_right = is_num_right ? ((ast_number *) opt->right)->value
                               : tac(((ast_opt *) opt)->right);
        int my_t = tmp_count++;
        std::cout << "t" << my_t << " = "
                  << (is_num_left ? "" : "t") << v_left
                  << " " << (char) opt->op << " "
                  << (is_num_right ? "" : "t") << v_right
                  << std::endl;
        return my_t;
    }
    throw std::logic_error("unsupported expr_tag");
}

int main() {
    std::cout << "Hello, World!" << std::endl;
    try {
        delete lookahead;
        lookahead = scan();
        ast_expr *my_ast = parse_expr();
//        std::cout << calc(my_ast) << std::endl;
        tac(my_ast);
    } catch (std::exception &e) {
        std::cout << e.what();
    }
    std::cout.put('\n');
    return 0;
}
