
#include <set>
#include <cassert>
#include <istream>
#include <iostream>

#include "parser.h"
#include "statics.h"

using namespace std;

vector< string > tokenize(string in) {

  vector< string > toks;
  bool white = true;
  int begin = -1;
  for (unsigned int i = 0; i < in.size(); i++) {
    char c = in[i];
    if (is_valid(c)) {
      if (white) {
        begin = i;
        white = false;
      }
    } else if (is_whitespace(c)) {
      if (!white) {
        toks.push_back(in.substr(begin, i-begin));
        white = true;
      }
    } else {
      assert("Wrong input character" == NULL);
    }
  }
  return toks;

}

FileTokenizer::FileTokenizer(istream &in) :
    in(in), white(true), comment(false)
{
}

string FileTokenizer::finalize_token() {
    if (this->white) {
        return "";
    } else {
        string res = string(this->buf.begin(), this->buf.end());
        this->buf.clear();
        this->white = true;
        return res;
    }
}

string FileTokenizer::next()
{
    while (true) {
        char c;
        this->in.get(c);
        if (this->in.eof()) {
            return this->finalize_token();
        }
        if (c == '$') {
            if (!this->white) {
                assert("Dollars cannot appear in the middle of a token" == NULL);
            }
            /* This can be a regular token or the beginning of a comment;
             * file inclusion is not supported so far and is treated like a comment. */
            this->in.get(c);
            if (this->in.eof()) {
                assert("Interrupted dollar sequence" == NULL);
            }
            if (c == '[' || c == '(') {
                // Here the comment begin
                bool found_dollar = false;
                while (true) {
                    this->in.get(c);
                    if (this->in.eof()) {
                        assert("File ended in a comment" == NULL);
                    }
                    if (found_dollar) {
                        if (c == '[' || c == '(') {
                            assert("Comment opening forbidden in comment" == NULL);
                        } else if (c == ']' || c == ')') {
                            break;
                        }
                        found_dollar = false;
                    }
                    if (c == '$') {
                        found_dollar = true;
                    }
                }
            } else if (c == ']' || c == ')') {
                assert("Comment closed while not in comment" == NULL);
            } else if (c == '$' || is_valid(c)) {
                this->buf.push_back('$');
                this->buf.push_back(c);
                this->white = false;
            } else if (is_whitespace(c)) {
                assert("Interrupted dollar sequence" == NULL);
            } else {
                assert("Forbidden input character" == NULL);
            }
        } else if (is_valid(c)) {
            this->buf.push_back(c);
            this->white = false;
        } else if (is_whitespace(c)) {
            if (!this->white) {
                return this->finalize_token();
            }
        } else {
            assert("Forbidden input character" == NULL);
        }
    }
}

Parser::Parser(FileTokenizer &ft) :
    ft(ft)
{
}

void Parser::run () {
    string token;
    this->label = 0;
    assert(this->stack.empty());
    this->stack.emplace_back();
    while ((token = ft.next()) != "") {
        if (token[0] == '$') {
            assert(token.size() == 2);
            char c = token[1];

            // Parse scoping blocks
            if (c == '{') {
                this->stack.emplace_back();
                continue;
            } else if (c == '}') {
                this->stack.pop_back();
                continue;
            }

            // Collect tokens in statement
            while ((token = ft.next()) != "$.") {
                if (token == "") {
                    assert("File ended in a statement" == NULL);
                }
                this->toks.push_back(token);
            }

            // Process statement
            switch (c) {
            case 'c':
                this->parse_c();
                break;
            case 'v':
                this->parse_v();
                break;
            case 'f':
                this->parse_f();
                break;
            case 'e':
                this->parse_e();
                break;
            case 'd':
                this->parse_d();
                break;
            case 'a':
                this->parse_a();
                break;
            case 'p':
                this->parse_p();
                break;
            default:
                assert("Wrong statement type" == NULL);
                break;
            }
            this->label = 0;
        } else {
            this->label = this->lib.create_label(token);
            assert(this->label != 0);
        }
    }
    this->stack.pop_back();
    assert(this->stack.empty());
}

void Parser::parse_c()
{
    assert(this->label == 0);
    for (auto tok : this->toks) {
        Tok res = this->lib.create_symbol(tok);
        assert(res != 0);
        this->consts.insert(res);
    }
}

void Parser::parse_v()
{
    assert(this->label == 0);
    for (auto tok : this->toks) {
        Tok res = this->lib.create_symbol(tok);
        assert(res != 0);
        this->stack.back().vars.insert(res);
    }
}

void Parser::parse_f()
{
    assert(this->label != 0);
}

void Parser::parse_e()
{
    assert(this->label != 0);
}

void Parser::parse_d()
{
    assert(this->label == 0);
}

void Parser::parse_a()
{
    assert(this->label != 0);
}

void Parser::parse_p()
{
    assert(this->label != 0);
}
