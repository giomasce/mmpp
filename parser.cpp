
#include <set>
#include <cassert>
#include <istream>
#include <iostream>
#include <algorithm>
#include <iterator>

#include "parser.h"
#include "statics.h"
#include "proof.h"

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
    in(in), white(true)
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
                // real_comment is just a trick to work around problems arising from
                // not implementing corretly file inclusion
                bool real_comment = c == '(';
                bool found_dollar = false;
                while (true) {
                    this->in.get(c);
                    if (this->in.eof()) {
                        assert("File ended in a comment" == NULL);
                    }
                    if (found_dollar) {
                        if (c == '(') {
                            assert("Comment opening forbidden in comment" == NULL);
                        } else if ((real_comment && c == ')') || (!real_comment && c == ']')) {
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
            this->toks.clear();
        } else {
            this->label = this->lib.create_label(token);
            assert(this->label != 0);
            //cout << "Found label " << token << endl;
        }
    }
    this->stack.pop_back();
    assert(this->stack.empty());
}

void Parser::parse_c()
{
    assert(this->label == 0);
    assert(this->stack.size() == 1);
    for (auto stok : this->toks) {
        SymTok tok = this->lib.create_symbol(stok);
        assert(!this->check_const(tok));
        assert(!this->check_var(tok));
        this->lib.add_constant(tok);
    }
}

void Parser::parse_v()
{
    assert(this->label == 0);
    for (auto stok : this->toks) {
        SymTok tok = this->lib.create_symbol(stok);
        assert(!this->check_const(tok));
        assert(!this->check_var(tok));
        this->stack.back().vars.insert(tok);
    }
}

void Parser::parse_f()
{
    assert(this->label != 0);
    assert(this->toks.size() == 2);
    SymTok const_tok = this->lib.get_symbol(this->toks[0]);
    SymTok var_tok = this->lib.get_symbol(this->toks[1]);
    assert(const_tok != 0);
    assert(var_tok != 0);
    assert(this->check_const(const_tok));
    assert(this->check_var(var_tok));
    this->lib.add_sentence(this->label, { const_tok, var_tok });
    this->stack.back().types.push_back(this->label);
}

void Parser::parse_e()
{
    assert(this->label != 0);
    assert(this->toks.size() >= 1);
    vector< SymTok > tmp;
    for (auto &stok : this->toks) {
        SymTok tok = this->lib.get_symbol(stok);
        assert(tok != 0);
        assert(this->check_const(tok) || this->check_var(tok));
        tmp.push_back(tok);
    }
    assert(this->check_const(tmp[0]));
    this->lib.add_sentence(this->label, tmp);
    this->stack.back().hyps.push_back(this->label);
}

void Parser::parse_d()
{
    assert(this->label == 0);
    for (auto it = this->toks.begin(); it != this->toks.end(); it++) {
        SymTok tok1 = this->lib.get_symbol(*it);
        assert(this->check_var(tok1));
        for (auto it2 = it+1; it2 != this->toks.end(); it2++) {
            SymTok tok2 = this->lib.get_symbol(*it2);
            assert(this->check_var(tok2));
            assert(tok1 != tok2);
            this->stack.back().dists.insert(minmax(tok1, tok2));
        }
    }
}

void Parser::collect_vars(set< SymTok > &vars, vector< SymTok > sent) {
    for (auto &tok : sent) {
        if (this->check_var(tok)) {
            vars.insert(tok);
        }
    }
}

set< SymTok > Parser::collect_mand_vars(vector< SymTok > sent) {
    set< SymTok > vars;
    this->collect_vars(vars, sent);
    for (auto &frame : this->stack) {
        for (auto &hyp : frame.hyps) {
            this->collect_vars(vars, this->lib.get_sentence(hyp));
        }
    }
    return vars;
}

// Here order matters! Be careful!
pair< int, vector< LabTok > > Parser::collect_mand_hyps(set< SymTok > vars) {
    vector< LabTok > hyps;

    // Floating hypotheses
    int num_floating = 0;
    for (auto &frame : this->stack) {
        for (auto &type : frame.types) {
            auto sent = this->lib.get_sentence(type);
            if (vars.find(sent[1]) != vars.end()) {
                hyps.push_back(type);
                num_floating++;
            }
        }
    }

    // Essential hypotheses
    for (auto &frame : this->stack) {
        for (auto &hyp : frame.hyps) {
            hyps.push_back(hyp);
        }
    }

    return make_pair(num_floating, hyps);
}

set< pair< SymTok, SymTok > > Parser::collect_mand_dists(set< SymTok > vars) {
    set< pair< SymTok, SymTok > > dists;
    for (auto &frame : this->stack) {
        for (auto &dist : frame.dists) {
            if (vars.find(dist.first) != vars.end() && vars.find(dist.second) != vars.end()) {
                dists.insert(dist);
            }
        }
    }
    return dists;
}

void Parser::parse_a()
{
    // Usual sanity checks and symbol conversion
    assert(this->label != 0);
    assert(this->toks.size() >= 1);
    vector< SymTok > tmp;
    for (auto &stok : this->toks) {
        SymTok tok = this->lib.get_symbol(stok);
        assert(tok != 0);
        assert(this->check_const(tok) || this->check_var(tok));
        tmp.push_back(tok);
    }
    assert(this->check_const(tmp[0]));
    this->lib.add_sentence(this->label, tmp);

    // Collect mandatory things
    set< SymTok > mand_vars = this->collect_mand_vars(tmp);
    int num_floating;
    vector< LabTok > mand_hyps;
    tie(num_floating, mand_hyps) = this->collect_mand_hyps(mand_vars);
    set< pair< SymTok, SymTok > > mand_dists = this->collect_mand_dists(mand_vars);

    // Finally build assertion
    Assertion ass(false, num_floating, mand_dists, mand_hyps, this->label);
    this->lib.add_assertion(this->label, ass);
}

void Parser::parse_p()
{
    // Usual sanity checks and symbol conversion
    assert(this->label != 0);
    assert(this->toks.size() >= 1);
    vector< SymTok > tmp;
    vector< LabTok > proof_labels;
    vector< LabTok > proof_ref;
    vector< int > proof_codes;
    CompressedDecoder cd;
    bool in_proof = false;
    int8_t compressed_proof = 0;
    for (auto &stok : this->toks) {
        if (!in_proof) {
            if (stok == "$=") {
                in_proof = true;
                continue;
            }
            SymTok tok = this->lib.get_symbol(stok);
            assert(tok != 0);
            assert(this->check_const(tok) || this->check_var(tok));
            tmp.push_back(tok);
        } else {
            if (compressed_proof == 0) {
                if (stok == "(") {
                    compressed_proof = 1;
                    continue;
                } else {
                    // The proof is not in compressed form, processing continues below
                    compressed_proof = -1;
                }
            }
            if (compressed_proof == 1) {
                if (stok == ")") {
                    compressed_proof = 2;
                    continue;
                } else {
                    LabTok tok = this->lib.get_label(stok);
                    assert(tok != 0);
                    proof_ref.push_back(tok);
                }
            }
            if (compressed_proof == 2) {
                for (auto c : stok) {
                    int res = cd.push_char(c);
                    if (res > 0) {
                        proof_codes.push_back(res);
                    }
                }
            }
            if (compressed_proof == -1) {
                LabTok tok = this->lib.get_label(stok);
                assert(tok != 0);
                proof_labels.push_back(tok);
            }
        }
    }
    assert(this->check_const(tmp[0]));
    assert(compressed_proof == -1 || compressed_proof == 2);
    this->lib.add_sentence(this->label, tmp);

    // Collect mandatory things
    set< SymTok > mand_vars = this->collect_mand_vars(tmp);
    int num_floating;
    vector< LabTok > mand_hyps;
    tie(num_floating, mand_hyps) = this->collect_mand_hyps(mand_vars);
    set< pair< SymTok, SymTok > > mand_dists = this->collect_mand_dists(mand_vars);

    // Finally build assertion and attach proof
    Assertion ass(true, num_floating, mand_dists, mand_hyps, this->label);
    Proof *proof;
    if (compressed_proof < 0) {
        proof = new UncompressedProof(this->lib, ass, proof_labels);
    } else {
        proof = new CompressedProof(this->lib, ass, proof_ref, proof_codes);
    }
    ass.add_proof(proof);
    proof->execute();
    this->lib.add_assertion(this->label, ass);
}

bool Parser::check_var(SymTok tok)
{
    for (auto &frame : this->stack) {
        if (frame.vars.find(tok) != frame.vars.end()) {
            return true;
        }
    }
    return false;
}

bool Parser::check_const(SymTok tok)
{
    return this->lib.is_constant(tok);
}

int CompressedDecoder::push_char(char c)
{
    if (is_whitespace(c)) {
        return -1;
    }
    assert('A' <= c && c <= 'Z');
    if (c == 'Z') {
        assert(this->current == 0);
        return 0;
    } else if ('A' <= c && c <= 'T') {
        int res = this->current * 20 + (c - 'A' + 1);
        this->current = 0;
        return res;
    } else {
        this->current = this->current * 5 + (c - 'U' + 1);
        return -1;
    }
}

string CompressedEncoder::push_int(int x)
{
    assert(x >= 0);
    if (x == 0) {
        return "Z";
    }
    vector< char > buf;
    int div = (x-1) / 20;
    int rem = (x-1) % 20 + 1;
    buf.push_back('A' + rem - 1);
    x = div;
    while (x > 0) {
        div = (x-1) / 5;
        rem = (x-1) % 5 + 1;
        buf.push_back('U' + rem - 1);
        x = div;
    }
    return string(buf.rbegin(), buf.rend());
}
