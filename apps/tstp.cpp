
#include <unordered_map>
#include <string>

#include <boost/format.hpp>
#include <boost/functional/hash.hpp>

#include "utils/utils.h"
#include "parsing/lr.h"
#include "parsing/earley.h"

using namespace std;

const string ID_LETTERS = "qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM1234567890_$'.";
const string SPACE_LETTERS = " \t\n\r";

enum class TSTPTokenType {
    CHAR,
    LETTER,
    ID,
    EXPR,
    ARGLIST,
    WHITESPACE,
    LINE,
};

ostream &operator<<(ostream &stream, const TSTPTokenType &tt) {
    return stream << static_cast< int >(tt);
}

enum TSTPRule {
    RULE_NONE = 0,
    RULE_CHAR_IS_LETTER = 0x100,
    RULE_CHAR_IS_WHITESPACE = 0x200,
    RULE_LINE = 0x300,
    RULE_LETTER_IS_ID,
    RULE_ID_AND_LETTER_IS_ID,
    RULE_ID_IS_EXPR,
    RULE_PARENS_EXPR_IS_EXPR,
    RULE_EXPR_IS_ARGLIST,
    RULE_ARGLIST_AND_EXPR_IS_ARGLIST,
    RULE_FUNC_APP_IS_EXPR,
    RULE_EMPTY_FUNC_APP_IS_EXPR,
    RULE_EQUALITY_IS_EXPR,
    RULE_INEQUALITY_IS_EXPR,
    RULE_LIST_IS_EXPR,
    RULE_EMPTY_LIST_IS_EXPR,
    RULE_FORALL_IS_EXPR,
    RULE_EXISTS_IS_EXPR,
    RULE_NEGATION_IS_EXPR,
};

ostream &operator<<(ostream &stream, const TSTPRule &rule) {
    return stream << static_cast< int >(rule);
}

struct TSTPToken {
    TSTPTokenType type;
    char content;

    TSTPToken() = default;
    TSTPToken(TSTPTokenType type, char content = 0) : type(type), content(content) {}

    bool operator==(const TSTPToken &other) const {
        return this->type == other.type && this->content == other.content;
    }

    bool operator<(const TSTPToken &other) const {
        return (this->type < other.type) || (this->type == other.type && this->content < other.content);
    }
};

ostream &operator<<(ostream &stream, const TSTPToken &tok) {
    return stream << "[" << tok.type << "," << tok.content << "]";
}

namespace std {
template< >
struct hash< TSTPToken > {
    typedef TSTPToken argument_type;
    typedef std::size_t result_type;
    result_type operator()(const argument_type &x) const noexcept {
        result_type res = 0;
        boost::hash_combine(res, x.type);
        boost::hash_combine(res, x.content);
        return res;
    }
};
}

vector< TSTPToken > trivial_lexer(string s) {
    vector < TSTPToken > ret;
    set< char > id_letters_set;
    for (char c : ID_LETTERS) {
        id_letters_set.insert(c);
    }
    bool last_was_id_letter = false;
    bool found_space = false;
    for (char c : s) {
        bool this_is_id_letter = id_letters_set.find(c) != id_letters_set.end();
        if (last_was_id_letter && this_is_id_letter && found_space) {
            throw "Whitespace cannot separate identifiers";
        }
        if (c == ' ' || c == '\n' || c == '\r' || c == '\t') {
            found_space = true;
            continue;
        } else {
            found_space = false;
        }
        ret.push_back({TSTPTokenType::CHAR, c});
        last_was_id_letter = this_is_id_letter;
    }
    return ret;
}

int parse_tstp_main(int argc, char *argv[]) {
    (void) argc;
    (void) argv;

    std::unordered_map<TSTPToken, std::vector<std::pair<TSTPRule, vector<TSTPToken> > > > ders;
    for (char c : ID_LETTERS) {
        ders[TSTPToken(TSTPTokenType::LETTER)].push_back(make_pair(static_cast< TSTPRule >(RULE_CHAR_IS_LETTER+c), vector< TSTPToken >{TSTPToken(TSTPTokenType::CHAR, c)}));
    }
    for (char c : SPACE_LETTERS) {
        ders[TSTPToken(TSTPTokenType::WHITESPACE)].push_back(make_pair(static_cast< TSTPRule >(RULE_CHAR_IS_WHITESPACE+c), vector< TSTPToken >{TSTPToken(TSTPTokenType::CHAR, c)}));
    }
    ders[TSTPToken(TSTPTokenType::LINE)].push_back(make_pair(RULE_LINE, vector< TSTPToken >{TSTPToken(TSTPTokenType::EXPR), TSTPToken(TSTPTokenType::CHAR, '.')}));
    ders[TSTPToken(TSTPTokenType::ID)].push_back(make_pair(RULE_LETTER_IS_ID, vector< TSTPToken >{TSTPToken(TSTPTokenType::LETTER)}));
    ders[TSTPToken(TSTPTokenType::ID)].push_back(make_pair(RULE_ID_AND_LETTER_IS_ID, vector< TSTPToken >{TSTPToken(TSTPTokenType::ID), TSTPToken(TSTPTokenType::LETTER)}));
    ders[TSTPToken(TSTPTokenType::EXPR)].push_back(make_pair(RULE_ID_IS_EXPR, vector< TSTPToken >{TSTPToken(TSTPTokenType::ID)}));
    ders[TSTPToken(TSTPTokenType::EXPR)].push_back(make_pair(RULE_PARENS_EXPR_IS_EXPR, vector< TSTPToken >{TSTPToken(TSTPTokenType::CHAR, '('), TSTPToken(TSTPTokenType::EXPR), TSTPToken(TSTPTokenType::CHAR, ')')}));
    ders[TSTPToken(TSTPTokenType::ARGLIST)].push_back(make_pair(RULE_EXPR_IS_ARGLIST, vector< TSTPToken >{TSTPToken(TSTPTokenType::EXPR)}));
    ders[TSTPToken(TSTPTokenType::ARGLIST)].push_back(make_pair(RULE_ARGLIST_AND_EXPR_IS_ARGLIST, vector< TSTPToken >{TSTPToken(TSTPTokenType::ARGLIST), TSTPToken(TSTPTokenType::CHAR, ','), TSTPToken(TSTPTokenType::EXPR)}));
    ders[TSTPToken(TSTPTokenType::EXPR)].push_back(make_pair(RULE_FUNC_APP_IS_EXPR, vector< TSTPToken >{TSTPToken(TSTPTokenType::ID), TSTPToken(TSTPTokenType::CHAR, '('), TSTPToken(TSTPTokenType::ARGLIST), TSTPToken(TSTPTokenType::CHAR, ')')}));
    ders[TSTPToken(TSTPTokenType::EXPR)].push_back(make_pair(RULE_EMPTY_FUNC_APP_IS_EXPR, vector< TSTPToken >{TSTPToken(TSTPTokenType::ID), TSTPToken(TSTPTokenType::CHAR, '('), TSTPToken(TSTPTokenType::CHAR, ')')}));
    ders[TSTPToken(TSTPTokenType::EXPR)].push_back(make_pair(RULE_EQUALITY_IS_EXPR, vector< TSTPToken >{TSTPToken(TSTPTokenType::EXPR), TSTPToken(TSTPTokenType::CHAR, '='), TSTPToken(TSTPTokenType::EXPR)}));
    ders[TSTPToken(TSTPTokenType::EXPR)].push_back(make_pair(RULE_INEQUALITY_IS_EXPR, vector< TSTPToken >{TSTPToken(TSTPTokenType::EXPR), TSTPToken(TSTPTokenType::CHAR, '!'), TSTPToken(TSTPTokenType::CHAR, '='), TSTPToken(TSTPTokenType::EXPR)}));
    ders[TSTPToken(TSTPTokenType::EXPR)].push_back(make_pair(RULE_LIST_IS_EXPR, vector< TSTPToken >{TSTPToken(TSTPTokenType::CHAR, '['), TSTPToken(TSTPTokenType::ARGLIST), TSTPToken(TSTPTokenType::CHAR, ']')}));
    ders[TSTPToken(TSTPTokenType::EXPR)].push_back(make_pair(RULE_EMPTY_LIST_IS_EXPR, vector< TSTPToken >{TSTPToken(TSTPTokenType::CHAR, '['), TSTPToken(TSTPTokenType::CHAR, ']')}));
    ders[TSTPToken(TSTPTokenType::EXPR)].push_back(make_pair(RULE_FORALL_IS_EXPR, vector< TSTPToken >{TSTPToken(TSTPTokenType::CHAR, '!'), TSTPToken(TSTPTokenType::CHAR, '['), TSTPToken(TSTPTokenType::ID), TSTPToken(TSTPTokenType::CHAR, ']'), TSTPToken(TSTPTokenType::CHAR, ':'), TSTPToken(TSTPTokenType::EXPR)}));
    ders[TSTPToken(TSTPTokenType::EXPR)].push_back(make_pair(RULE_EXISTS_IS_EXPR, vector< TSTPToken >{TSTPToken(TSTPTokenType::CHAR, '?'), TSTPToken(TSTPTokenType::CHAR, '['), TSTPToken(TSTPTokenType::ID), TSTPToken(TSTPTokenType::CHAR, ']'), TSTPToken(TSTPTokenType::CHAR, ':'), TSTPToken(TSTPTokenType::EXPR)}));
    ders[TSTPToken(TSTPTokenType::EXPR)].push_back(make_pair(RULE_NEGATION_IS_EXPR, vector< TSTPToken >{TSTPToken(TSTPTokenType::CHAR, '~'), TSTPToken(TSTPTokenType::EXPR)}));

    LRParser< TSTPToken, TSTPRule > parser(ders);
    //EarleyParser< TSTPToken, TSTPRule > parser(ders);
    parser.initialize();

    string line;
    while (getline(cin, line)) {
        auto lexes = trivial_lexer(line);
        auto pt = parser.parse(lexes.begin(), lexes.end(), TSTPToken(TSTPTokenType::LINE));
        cout << "Parsed as " << pt.label << endl;
    }

    return 0;
}
static_block {
    register_main_function("parse_tstp", parse_tstp_main);
}
