
#include <set>
#include <istream>
#include <iostream>
#include <algorithm>
#include <iterator>
#include <memory>

#include "parser.h"
#include "statics.h"
#include "proof.h"
#include "statics.h"

using namespace std;

vector< string > tokenize(string in) {

  vector< string > toks;
  bool white = true;
  int begin = -1;
  for (unsigned int i = 0; i <= in.size(); i++) {
    char c = i < in.size() ? in[i] : ' ';
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
      throw MMPPException("Wrong input character");
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
                throw MMPPException("Dollars cannot appear in the middle of a token");
            }
            /* This can be a regular token or the beginning of a comment;
             * file inclusion is not supported so far and is treated like a comment. */
            this->in.get(c);
            if (this->in.eof()) {
                throw MMPPException("Interrupted dollar sequence");
            }
            if (c == '[' || c == ']') {
                throw MMPPException("File inclusion not supported");
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
                        throw MMPPException("File ended in a comment");
                    }
                    if (found_dollar) {
                        if (c == '(') {
                            throw MMPPException("Comment opening forbidden in comment");
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
                throw MMPPException("Comment closed while not in comment");
            } else if (c == '$' || is_valid(c)) {
                this->buf.push_back('$');
                this->buf.push_back(c);
                this->white = false;
            } else if (is_whitespace(c)) {
                throw MMPPException("Interrupted dollar sequence");
            } else {
                throw MMPPException("Forbidden input character");
            }
        } else if (is_valid(c)) {
            this->buf.push_back(c);
            this->white = false;
        } else if (is_whitespace(c)) {
            if (!this->white) {
                return this->finalize_token();
            }
        } else {
            throw MMPPException("Forbidden input character");
        }
    }
}

Parser::Parser(FileTokenizer &ft, bool execute_proofs) :
    ft(ft), execute_proofs(execute_proofs)
{
}

void Parser::run () {
    string token;
    this->label = 0;
    assert(this->stack.empty());
    this->stack.emplace_back();
    while ((token = ft.next()) != "") {
        if (token[0] == '$') {
            assert_or_throw(token.size() == 2);
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
                    throw MMPPException("File ended in a statement");
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
                throw MMPPException("Wrong statement type");
                break;
            }
            this->label = 0;
            this->toks.clear();
        } else {
            this->label = this->lib.create_label(token);
            assert_or_throw(this->label != 0);
            //cout << "Found label " << token << endl;
        }
    }
    this->final_frame = this->stack.back();
    this->lib.set_types(this->final_frame.types);
    this->stack.pop_back();
    assert_or_throw(this->stack.empty());
}

const Library &Parser::get_library() const {
    return this->lib;
}

const ParserStackFrame &Parser::get_final_frame() const
{
    return this->final_frame;
}

void Parser::parse_c()
{
    assert_or_throw(this->label == 0);
    assert_or_throw(this->stack.size() == 1);
    for (auto stok : this->toks) {
        SymTok tok = this->lib.create_symbol(stok);
        assert_or_throw(!this->check_const(tok));
        assert_or_throw(!this->check_var(tok));
        this->lib.add_constant(tok);
    }
}

void Parser::parse_v()
{
    assert_or_throw(this->label == 0);
    for (auto stok : this->toks) {
        SymTok tok = this->lib.create_symbol(stok);
        assert_or_throw(!this->check_const(tok));
        assert_or_throw(!this->check_var(tok));
        this->stack.back().vars.insert(tok);
    }
}

void Parser::parse_f()
{
    assert_or_throw(this->label != 0);
    assert_or_throw(this->toks.size() == 2);
    SymTok const_tok = this->lib.get_symbol(this->toks[0]);
    SymTok var_tok = this->lib.get_symbol(this->toks[1]);
    assert_or_throw(const_tok != 0);
    assert_or_throw(var_tok != 0);
    assert_or_throw(this->check_const(const_tok));
    assert_or_throw(this->check_var(var_tok));
    this->lib.add_sentence(this->label, { const_tok, var_tok });
    this->stack.back().types.push_back(this->label);
    this->stack.back().types_set.insert(this->label);

    // FIXME This does not appear to be mentioned in the specs, but it seems necessary anyway
    //Assertion ass(false, 0, {}, {}, this->label);
    //this->lib.add_assertion(this->label, ass);
}

void Parser::parse_e()
{
    assert_or_throw(this->label != 0);
    assert_or_throw(this->toks.size() >= 1);
    vector< SymTok > tmp;
    for (auto &stok : this->toks) {
        SymTok tok = this->lib.get_symbol(stok);
        assert_or_throw(tok != 0);
        assert_or_throw(this->check_const(tok) || this->check_var(tok));
        tmp.push_back(tok);
    }
    assert_or_throw(this->check_const(tmp[0]));
    this->lib.add_sentence(this->label, tmp);
    this->stack.back().hyps.push_back(this->label);
}

void Parser::parse_d()
{
    assert_or_throw(this->label == 0);
    for (auto it = this->toks.begin(); it != this->toks.end(); it++) {
        SymTok tok1 = this->lib.get_symbol(*it);
        assert_or_throw(this->check_var(tok1));
        for (auto it2 = it+1; it2 != this->toks.end(); it2++) {
            SymTok tok2 = this->lib.get_symbol(*it2);
            assert_or_throw(this->check_var(tok2));
            assert_or_throw(tok1 != tok2);
            this->stack.back().dists.insert(minmax(tok1, tok2));
        }
    }
}

void Parser::collect_vars_from_sentence(std::set<SymTok> &vars, const std::vector<SymTok> &sent) const {
    for (auto &tok : sent) {
        if (this->check_var(tok)) {
            vars.insert(tok);
        }
    }
}

void Parser::collect_vars_from_proof(std::set<SymTok> &vars, const std::vector<LabTok> &proof) const
{
    for (auto &tok : proof) {
        if (this->check_type(tok)) {
            assert_or_throw(this->lib.get_sentence(tok).size() == 2);
            vars.insert(this->lib.get_sentence(tok).at(1));
        }
    }
}

set< SymTok > Parser::collect_mand_vars(const std::vector<SymTok> &sent) const {
    set< SymTok > vars;
    this->collect_vars_from_sentence(vars, sent);
    for (auto &frame : this->stack) {
        for (auto &hyp : frame.hyps) {
            this->collect_vars_from_sentence(vars, this->lib.get_sentence(hyp));
        }
    }
    return vars;
}

std::set<SymTok> Parser::collect_opt_vars(const std::vector<LabTok> &proof, const std::set<SymTok> &mand_vars) const
{
    set< SymTok > vars;
    this->collect_vars_from_proof(vars, proof);
    set< SymTok > opt_vars;
    set_difference(vars.begin(), vars.end(), mand_vars.begin(), mand_vars.end(), inserter(opt_vars, opt_vars.begin()));
    return opt_vars;
}

// Here order matters! Be careful!
pair< int, vector< LabTok > > Parser::collect_mand_hyps(set< SymTok > vars) const {
    vector< LabTok > hyps;

    // Floating hypotheses
    int num_floating = 0;
    for (auto &frame : this->stack) {
        for (auto &type : frame.types) {
            const auto &sent = this->lib.get_sentence(type);
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

std::set<LabTok> Parser::collect_opt_hyps(std::set<SymTok> opt_vars) const
{
    set< LabTok > ret;
    for (auto &frame : this->stack) {
        for (auto &type : frame.types) {
            const auto &sent = this->lib.get_sentence(type);
            if (opt_vars.find(sent[1]) != opt_vars.end()) {
                ret.insert(type);
            }
        }
    }
    return ret;
}

set< pair< SymTok, SymTok > > Parser::collect_mand_dists(set< SymTok > vars) const {
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
    assert_or_throw(this->label != 0);
    assert_or_throw(this->toks.size() >= 1);
    vector< SymTok > tmp;
    for (auto &stok : this->toks) {
        SymTok tok = this->lib.get_symbol(stok);
        assert_or_throw(tok != 0);
        assert_or_throw(this->check_const(tok) || this->check_var(tok));
        tmp.push_back(tok);
    }
    assert_or_throw(this->check_const(tmp[0]));
    this->lib.add_sentence(this->label, tmp);

    // Collect things
    set< SymTok > mand_vars = this->collect_mand_vars(tmp);
    int num_floating;
    vector< LabTok > mand_hyps;
    tie(num_floating, mand_hyps) = this->collect_mand_hyps(mand_vars);
    set< pair< SymTok, SymTok > > mand_dists = this->collect_mand_dists(mand_vars);

    // Finally build assertion
    Assertion ass(false, num_floating, mand_dists, mand_hyps, {}, this->label);
    this->lib.add_assertion(this->label, ass);
}

void Parser::parse_p()
{
    // Usual sanity checks and symbol conversion
    assert_or_throw(this->label != 0);
    assert_or_throw(this->toks.size() >= 1);
    vector< SymTok > tmp;
    vector< LabTok > proof_labels;
    vector< LabTok > proof_refs;
    vector< CodeTok > proof_codes;
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
            assert_or_throw(tok != 0);
            assert_or_throw(this->check_const(tok) || this->check_var(tok));
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
                    assert_or_throw(tok != 0);
                    proof_refs.push_back(tok);
                }
            }
            if (compressed_proof == 2) {
                for (auto c : stok) {
                    CodeTok res = cd.push_char(c);
                    if (res != INVALID_CODE) {
                        proof_codes.push_back(res);
                    }
                }
            }
            if (compressed_proof == -1) {
                LabTok tok = this->lib.get_label(stok);
                assert_or_throw(tok != 0);
                proof_labels.push_back(tok);
            }
        }
    }
    assert_or_throw(this->check_const(tmp[0]));
    assert(compressed_proof == -1 || compressed_proof == 2);
    this->lib.add_sentence(this->label, tmp);

    // Collect things
    set< SymTok > mand_vars = this->collect_mand_vars(tmp);
    int num_floating;
    vector< LabTok > mand_hyps;
    tie(num_floating, mand_hyps) = this->collect_mand_hyps(mand_vars);
    set< pair< SymTok, SymTok > > mand_dists = this->collect_mand_dists(mand_vars);
    set< SymTok > opt_vars;
    if (compressed_proof < 0) {
        opt_vars = this->collect_opt_vars(proof_labels, mand_vars);
    } else {
        opt_vars = this->collect_opt_vars(proof_refs, mand_vars);
    }
    set< LabTok > opt_hyps = this->collect_opt_hyps(opt_vars);

    // Finally build assertion and attach proof
    Assertion ass(true, num_floating, mand_dists, mand_hyps, opt_hyps, this->label);
    shared_ptr< Proof > proof;
    if (compressed_proof < 0) {
        proof = shared_ptr< Proof > (new UncompressedProof(proof_labels));
    } else {
        proof = shared_ptr< Proof > (new CompressedProof(proof_refs, proof_codes));
    }
    ass.add_proof(proof);
    auto pe = ass.get_proof_executor(this->lib);
    assert_or_throw(pe->check_syntax());
    if (this->execute_proofs) {
        pe->execute();
    }
    this->lib.add_assertion(this->label, ass);
}

bool Parser::check_var(SymTok tok) const
{
    for (auto &frame : this->stack) {
        if (frame.vars.find(tok) != frame.vars.end()) {
            return true;
        }
    }
    return false;
}

bool Parser::check_const(SymTok tok) const
{
    return this->lib.is_constant(tok);
}

bool Parser::check_type(LabTok tok) const
{
    for (auto &frame : this->stack) {
        if (frame.types_set.find(tok) != frame.types_set.end()) {
            return true;
        }
    }
    return false;
}

CodeTok CompressedDecoder::push_char(char c)
{
    if (is_whitespace(c)) {
        return -1;
    }
    assert_or_throw('A' <= c && c <= 'Z');
    if (c == 'Z') {
        assert_or_throw(this->current == 0);
        return 0;
    } else if ('A' <= c && c <= 'T') {
        int res = this->current * 20 + (c - 'A' + 1);
        this->current = 0;
        assert(res != INVALID_CODE);
        return res;
    } else {
        this->current = this->current * 5 + (c - 'U' + 1);
        return INVALID_CODE;
    }
}

string CompressedEncoder::push_code(CodeTok x)
{
    assert_or_throw(x != INVALID_CODE);
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

MMPPException::MMPPException(string reason)
{
    this->reason = reason;
}
