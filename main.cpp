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

struct token_word : token {
    std::string value;

    explicit token_word(std::string &v) : token({.tag=word}), value(v) {};
};

struct ast {
    enum {
        expr, stmt
    } tag;
};

struct ast_expr : ast {
    enum tag_t {
        num, opt
    } expr_tag;

    explicit ast_expr(enum tag_t t) :
            ast({.tag=expr}), expr_tag(t) {};
};

struct ast_num_expr : ast_expr {
    int value;

    explicit ast_num_expr(int v) :
            ast_expr(num),
            value(v) {};
};

struct ast_opt_expr : ast_expr {
    int op;
    ast_expr *left, *right;

    explicit ast_opt_expr(int op, ast_expr *l, ast_expr *r) :
            ast_expr(opt),
            op(op),
            left(l),
            right(r) {};
};

struct ast_stmt : ast {
    enum tag_t {
        assign, if_stmt, goto_stmt
    } stmt_tag;

    explicit ast_stmt(enum tag_t t) :
            ast({.tag=expr}), stmt_tag(t) {};
};

struct ast_assign_stmt : ast_stmt {
    std::string lhs;
    ast_expr *rhs;

    explicit ast_assign_stmt(std::string &lhs, ast_expr *rhs) :
            ast_stmt(assign), lhs(lhs), rhs(rhs) {};
};

struct ast_goto_stmt : ast_stmt {
    std::string label;

    explicit ast_goto_stmt(std::string label) :
            ast_stmt(goto_stmt), label(label) {};
};

struct ast_if_stmt : ast_stmt {
    ast_expr *expr;
    ast_stmt *stmt;

    explicit ast_if_stmt(ast_expr *expr, ast_stmt *stmt) :
            ast_stmt(if_stmt), expr(expr), stmt(stmt) {};
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
    int c = std::cin.peek();
    if (islower(c) || isupper(c) || c == '_') {
        std::string word;
        do word += (char) std::cin.get();
        while (c = std::cin.peek(), (islower(c) || isupper(c) || isdigit(c) || c == '_'));
        return new token_word(word);
    }
    return new token_single(std::cin.get());
}

/* * * * left recursive * * * * *
 * term     ->  term * factor
 *          |   term / factor
 *          |   factor
 * expr     ->  expr + term
 *          |   expr - term
 *          |   term
 * factor   -> digit | word | (expr)
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
 * factor   ->  digit | word | (expr)
 * */

void parse_match(token *t) {
    if (lookahead == t) {
        delete lookahead;
        lookahead = scan();
    } else throw std::logic_error("syntax error");
}

ast_expr *parse_expr();

ast_expr *parse_factor() {
    if (lookahead->tag == token::number) {
        int v = ((token_digit *) lookahead)->value;
        parse_match(lookahead);
        return new ast_num_expr(v);
    } else throw std::logic_error("syntax error");
}

ast_expr *parse_term() {
    auto l = parse_factor();
    while (lookahead->tag == token::single) {
        int c = ((token_single *) lookahead)->value;
        if (c == '*' || c == '/') {
            parse_match(lookahead);
            l = new ast_opt_expr(c, l, parse_factor());
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
            l = new ast_opt_expr(c, l, parse_term());
        } else break;
    }
    return l;
}

/* * * * * statement * * * * *
 * stmts    ->  stmt | stmt \n stmt
 * stmt     ->  word = expr
 *          |   if(expr) stmt
 *          |   goto word
 *          |   { opt_ret stmts opt_ret }
 * opt_ret  -> \n | ε
 * */

ast_stmt *parse_stmt() {
    if (lookahead->tag == token::word) {
        std::string word(((token_word *) lookahead)->value);
        if (word == "if") {             // if(expr) stmt goto
            parse_match(lookahead);         // if
            if (lookahead->tag != token::single || ((token_single *) lookahead)->value != '(')
                throw std::logic_error("syntax error: expected '(' after 'if'");
            else parse_match(lookahead);    // (
            auto expr = parse_expr();       // expr
            if (lookahead->tag != token::single || ((token_single *) lookahead)->value != ')')
                throw std::logic_error("syntax error: expected ')'");
            else parse_match(lookahead);    // )
            auto stmt = parse_stmt();       // stmt
            return new ast_if_stmt(expr, stmt);
        } else if (word == "goto") {
            parse_match(lookahead);
            if (lookahead->tag != token::word)
                throw std::logic_error("syntax error: expected a label after 'goto'");
            auto label = ((token_word *) lookahead)->value;
            parse_match(lookahead);
            return new ast_goto_stmt(label);
        } else {                        // word = expr
            parse_match(lookahead);                                 // word
            if (lookahead->tag == token::single &&
                ((token_single *) lookahead)->value == '=') {
                parse_match(lookahead);                             // =
                return new ast_assign_stmt(word, parse_expr());     // expr
            } else throw std::logic_error("syntax error");
        }
    }
    throw std::logic_error("unknown expr_tag");
}

int calc(ast_expr *expr) {
    if (expr->expr_tag == ast_expr::num)
        return ((ast_num_expr *) expr)->value;
    else if (expr->expr_tag == ast_expr::opt) {
        auto opt = (ast_opt_expr *) expr;
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


int tac_expr(ast_expr *expr) {
    static int tmp_counter = 0;
    if (expr->expr_tag == ast_expr::num) {
        std::cout << "tmp" << tmp_counter << " = "
                  << ((ast_num_expr *) expr)->value
                  << std::endl;
        return tmp_counter++;
    } else if (expr->expr_tag == ast_expr::opt) {
        auto opt = (ast_opt_expr *) expr;
        bool is_num_left = opt->left->expr_tag == ast_expr::num;
        bool is_num_right = opt->right->expr_tag == ast_expr::num;
        int v_left = is_num_left
                     ? ((ast_num_expr *) opt->left)->value
                     : tac_expr(((ast_opt_expr *) opt)->left);
        int v_right = is_num_right
                      ? ((ast_num_expr *) opt->right)->value
                      : tac_expr(((ast_opt_expr *) opt)->right);
        std::cout << "tmp" << tmp_counter << " = "
                  << (is_num_left ? "" : "tmp") << v_left
                  << " " << (char) opt->op << " "
                  << (is_num_right ? "" : "tmp") << v_right
                  << std::endl;
        return tmp_counter++;
    }
    throw std::logic_error("unsupported expr_tag");
}

void tac_stmt(ast_stmt *stmt) {
    if (stmt->stmt_tag == ast_stmt::assign) {
        auto assign_stmt = (ast_assign_stmt *) stmt;
        int tmp = tac_expr(assign_stmt->rhs);
        std::cout << assign_stmt->lhs << " = tmp" << tmp << std::endl;
    } else if (stmt->stmt_tag == ast_stmt::if_stmt) {
        static int if_label_count = 0;
        auto if_stmt = (ast_if_stmt *) stmt;
        int expr_tmp = tac_expr(if_stmt->expr);
        std::cout << "if tmp" << expr_tmp << " goto IfLabel" << if_label_count << std::endl;
        tac_stmt(if_stmt->stmt);
        std::cout << "IfLabel" << if_label_count++ << ":" << std::endl;
    } else throw std::logic_error("unsupported stmt_tag");
}

int main() {
    std::cout << "Hello, World!" << std::endl;
    try {
        lookahead = scan();
//        ast_expr *my_ast = parse_expr();
//        std::cout << calc(my_ast) << std::endl;
//        int result = tac_expr(my_ast);
//        std::cout << "output temp" << result << std::endl;
        ast_stmt *my_ast = parse_stmt();
        tac_stmt(my_ast);
    } catch (std::exception &e) {
        std::cout << e.what();
    }
    std::cout.put('\n');
    return 0;
}
