
#include <set>
#include <istream>
#include <iostream>
#include <algorithm>
#include <iterator>
#include <memory>
#include <fstream>

#include "reader.h"
#include "utils/utils.h"
#include "proof.h"

using namespace std;
using namespace boost::filesystem;

vector< string > tokenize(const string &in) {

  vector< string > toks;
  bool white = true;
  size_t begin = 0;
  for (size_t i = 0; i <= in.size(); i++) {
    char c = (i < in.size()) ? in[i] : ' ';
    if (is_mm_valid(c)) {
      if (white) {
        begin = i;
        white = false;
      }
    } else if (is_mm_whitespace(c)) {
      if (!white) {
        toks.push_back(in.substr(begin, i-begin));
        white = true;
      }
    } else {
      throw MMPPParsingError("Wrong input character");
    }
  }
  return toks;

}

/*FileTokenizer::FileTokenizer(const string &filename) :
    fin(filename), base_path(path(filename).parent_path()), cascade(NULL), white(true)
{
}*/

FileTokenizer::FileTokenizer(const path &filename) :
    fin(filename), base_path(filename.parent_path()), cascade(NULL), white(true)
{
}

FileTokenizer::FileTokenizer(string filename, path base_path) :
    fin(filename), base_path(base_path), cascade(NULL), white(true)
{
}

std::pair<bool, string> FileTokenizer::finalize_token(bool comment) {
    if (this->white) {
        return make_pair(comment, "");
    } else {
        string res = string(this->buf.begin(), this->buf.end());
        this->buf.clear();
        this->white = true;
        return make_pair(comment, res);
    }
}

std::pair<bool, string> FileTokenizer::next()
{
    while (true) {
        if (this->cascade != NULL) {
            auto next_pair = this->cascade->next();
            if (next_pair.second != "") {
                return next_pair;
            } else {
                delete this->cascade;
                this->cascade = NULL;
            }
        }
        char c = this->get_char();
        if (this->fin.eof()) {
            return this->finalize_token(false);
        }
        if (c == '$') {
            if (!this->white) {
                throw MMPPParsingError("Dollars cannot appear in the middle of a token");
            }
            // This can be a regular token or the beginning of a comment
            c = this->get_char();
            if (this->fin.eof()) {
                throw MMPPParsingError("Interrupted dollar sequence");
            }
            if (c == '(' || c == '[') {
                // Here the comment begin
                bool found_dollar = false;
                bool comment = c == '(';
                vector< char > content;
                while (true) {
                    c = this->get_char();
                    if (this->fin.eof()) {
                        throw MMPPParsingError("File ended in comment or in file inclusion");
                    }
                    if (found_dollar) {
                        if ((comment && c == '(') || (!comment && c == '[')) {
                            throw MMPPParsingError("Comment and file inclusion opening forbidden in comments and file inclusions");
                        } else if ((comment && c == ')') || (!comment && c == ']')) {
                            this->white = true;
                            if (comment) {
                                if (content.empty()) {
                                    break;
                                } else {
                                    return make_pair(true, string(content.begin(), content.end()));
                                }
                            } else {
                                string filename = trimmed(string(content.begin(), content.end()));
                                string actual_filename = (this->base_path / filename).native();
                                this->cascade = new FileTokenizer(actual_filename, this->base_path);
                                break;
                            }
                        } else if (c == '$') {
                            content.push_back('$');
                        } else {
                            content.push_back('$');
                            content.push_back(c);
                            found_dollar = false;
                        }
                    } else {
                        if (c == '$') {
                            found_dollar = true;
                        } else {
                            content.push_back(c);
                        }
                    }
                }
            } else if (c == ')') {
                throw MMPPParsingError("Comment closed while not in comment");
            } else if (c == ']') {
                throw MMPPParsingError("File inclusion closed while not in comment");
            } else if (c == '$' || is_mm_valid(c)) {
                this->buf.push_back('$');
                this->buf.push_back(c);
                this->white = false;
            } else if (is_mm_whitespace(c)) {
                throw MMPPParsingError("Interrupted dollar sequence");
            } else {
                throw MMPPParsingError("Forbidden input character");
            }
        } else if (is_mm_valid(c)) {
            this->buf.push_back(c);
            this->white = false;
        } else if (is_mm_whitespace(c)) {
            if (!this->white) {
                return this->finalize_token(false);
            }
        } else {
            throw MMPPParsingError("Forbidden input character");
        }
    }
}

FileTokenizer::~FileTokenizer()
{
    delete this->cascade;
}

Reader::Reader(TokenGenerator &tg, bool execute_proofs, bool store_comments) :
    tg(&tg), execute_proofs(execute_proofs), store_comments(store_comments),
    number(1)
{
}

void Reader::run () {
    //cout << "Running the reader" << endl;
    //auto t = tic();
    pair< bool, string > token_pair;
    this->label = 0;
    assert(this->stack.empty());
    this->stack.emplace_back();
    while ((token_pair = this->next_token()).second != "") {
        bool &comment = token_pair.first;
        string &token = token_pair.second;
        if (comment) {
            this->process_comment(token);
            continue;
        }
        if (token[0] == '$') {
            assert_or_throw< MMPPParsingError >(token.size() == 2, "Dollar sequence with wrong length");
            char c = token[1];

            // Parse scoping blocks
            if (c == '{') {
                this->stack.emplace_back();
                continue;
            } else if (c == '}') {
                assert_or_throw< MMPPParsingError >(!this->stack.empty(), "Unmatched closed scoping block");
                this->stack.pop_back();
                continue;
            }

            // Collect tokens in statement
            while ((token_pair = this->next_token()).second != "$.") {
                bool &comment = token_pair.first;
                string &token = token_pair.second;
                if (comment) {
                    this->process_comment(token);
                    continue;
                }
                if (token == "") {
                    throw MMPPParsingError("File ended in a statement");
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
                throw MMPPParsingError("Wrong statement type");
                break;
            }
            this->label = 0;
            this->toks.clear();
        } else {
            this->label = this->lib.create_label(token);
            assert_or_throw< MMPPParsingError >(this->label != 0, "Repeated label detected");
            //cout << "Found label " << token << endl;
        }
    }
    this->final_frame = this->stack.back();
    this->lib.set_final_stack_frame(this->final_frame);
    this->lib.set_max_number(this->number-1);
    this->stack.pop_back();
    assert_or_throw< MMPPParsingError >(this->stack.empty(), "Unmatched open scoping block");

    // Some final operations
    this->parse_t_comment(this->t_comment);
    this->t_comment = "";
    this->parse_j_comment(this->j_comment);
    this->j_comment = "";

    //toc(t, 1);
}

const LibraryImpl &Reader::get_library() const {
    return this->lib;
}

std::pair<bool, string> Reader::next_token()
{
    return this->tg->next();
}

const StackFrame &Reader::get_final_frame() const
{
    return this->final_frame;
}

void Reader::parse_c()
{
    assert_or_throw< MMPPParsingError >(this->label == 0, "Undue label in $c statement");
    assert_or_throw< MMPPParsingError >(this->stack.size() == 1, "Found $c statement when not in top-level scope");
    for (auto stok : this->toks) {
        SymTok tok = this->lib.create_symbol(stok);
        assert_or_throw< MMPPParsingError >(!this->check_const(tok), "Symbol already bound in $c statement");
        assert_or_throw< MMPPParsingError >(!this->check_var(tok), "Symbol already bound in $c statement");
        this->consts.insert(tok);
        this->lib.set_constant(tok, true);
    }
}

void Reader::parse_v()
{
    assert_or_throw< MMPPParsingError >(this->label == 0, "Undue label in $v statement");
    for (auto stok : this->toks) {
        SymTok tok = this->lib.create_or_get_symbol(stok);
        assert_or_throw< MMPPParsingError >(!this->check_const(tok), "Symbol already bound in $v statement");
        assert_or_throw< MMPPParsingError >(!this->check_var(tok), "Symbol already bound in $v statement");
        this->lib.set_constant(tok, false);
        this->stack.back().vars.insert(tok);
    }
}

void Reader::parse_f()
{
    assert_or_throw< MMPPParsingError >(this->label != 0, "Missing label in $f statement");
    assert_or_throw< MMPPParsingError >(this->toks.size() == 2, "Found $f statement with wrong length");
    SymTok const_tok = this->lib.get_symbol(this->toks[0]);
    SymTok var_tok = this->lib.get_symbol(this->toks[1]);
    assert_or_throw< MMPPParsingError >(const_tok != 0, "First member of a $f statement is not defined");
    assert_or_throw< MMPPParsingError >(var_tok != 0, "Second member of a $f statement is not defined");
    assert_or_throw< MMPPParsingError >(this->check_const(const_tok), "First member of a $f statement is not a constant");
    assert_or_throw< MMPPParsingError >(this->check_var(var_tok), "Second member of a $f statement is not a variable");
    this->lib.add_sentence(this->label, { const_tok, var_tok });
    this->stack.back().types.push_back(this->label);
    this->stack.back().types_set.insert(this->label);
}

void Reader::parse_e()
{
    assert_or_throw< MMPPParsingError >(this->label != 0, "Missing label in $e statement");
    assert_or_throw< MMPPParsingError >(this->toks.size() >= 1, "Empty $e statement");
    vector< SymTok > tmp;
    for (auto &stok : this->toks) {
        SymTok tok = this->lib.get_symbol(stok);
        assert_or_throw< MMPPParsingError >(tok != 0, "Symbol in $e statement is not defined");
        assert(this->check_const(tok) || this->check_var(tok));
        tmp.push_back(tok);
    }
    assert_or_throw< MMPPParsingError >(this->check_const(tmp[0]), "First symbol of $e statement is not a constant");
    this->lib.add_sentence(this->label, tmp);
    this->stack.back().hyps.push_back(this->label);
}

void Reader::parse_d()
{
    assert_or_throw< MMPPParsingError >(this->label == 0, "Undue label in $d statement");
    for (auto it = this->toks.begin(); it != this->toks.end(); it++) {
        SymTok tok1 = this->lib.get_symbol(*it);
        assert_or_throw< MMPPParsingError >(this->check_var(tok1), "Symbol in $d statement is not a variable");
        for (auto it2 = it+1; it2 != this->toks.end(); it2++) {
            SymTok tok2 = this->lib.get_symbol(*it2);
            assert_or_throw< MMPPParsingError >(this->check_var(tok2), "Symbol in $d statement is not a variable");
            assert_or_throw< MMPPParsingError >(tok1 != tok2, "Repeated symbol in $d statement");
            this->stack.back().dists.insert(minmax(tok1, tok2));
        }
    }
}

void Reader::collect_vars_from_sentence(std::set<SymTok> &vars, const std::vector<SymTok> &sent) const {
    for (auto &tok : sent) {
        if (this->check_var(tok)) {
            vars.insert(tok);
        }
    }
}

void Reader::collect_vars_from_proof(std::set<SymTok> &vars, const std::vector<LabTok> &proof) const
{
    for (auto &tok : proof) {
        if (this->check_type(tok)) {
            assert_or_throw< MMPPParsingError >(this->lib.get_sentence(tok).size() == 2);
            vars.insert(this->lib.get_sentence(tok).at(1));
        }
    }
}

set< SymTok > Reader::collect_mand_vars(const std::vector<SymTok> &sent) const {
    set< SymTok > vars;
    this->collect_vars_from_sentence(vars, sent);
    for (auto &frame : this->stack) {
        for (auto &hyp : frame.hyps) {
            this->collect_vars_from_sentence(vars, this->lib.get_sentence(hyp));
        }
    }
    return vars;
}

std::set<SymTok> Reader::collect_opt_vars(const std::vector<LabTok> &proof, const std::set<SymTok> &mand_vars) const
{
    set< SymTok > vars;
    this->collect_vars_from_proof(vars, proof);
    set< SymTok > opt_vars;
    set_difference(vars.begin(), vars.end(), mand_vars.begin(), mand_vars.end(), inserter(opt_vars, opt_vars.begin()));
    return opt_vars;
}

// Here order matters! Be careful!
pair< vector< LabTok >, vector< LabTok > > Reader::collect_mand_hyps(set< SymTok > vars) const {
    vector< LabTok > float_hyps, ess_hyps;

    // Floating hypotheses
    for (auto &frame : this->stack) {
        for (auto &type : frame.types) {
            const auto &sent = this->lib.get_sentence(type);
            if (vars.find(sent[1]) != vars.end()) {
                float_hyps.push_back(type);
            }
        }
    }

    // Essential hypotheses
    for (auto &frame : this->stack) {
        for (auto &hyp : frame.hyps) {
            ess_hyps.push_back(hyp);
        }
    }

    return make_pair(float_hyps, ess_hyps);
}

std::set<LabTok> Reader::collect_opt_hyps(std::set<SymTok> opt_vars) const
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

set< pair< SymTok, SymTok > > Reader::collect_mand_dists(set< SymTok > vars) const {
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

set< pair< SymTok, SymTok > > Reader::collect_opt_dists(set< SymTok > opt_vars, set< SymTok > mand_vars) const {
    set< SymTok > vars;
    set_union(opt_vars.begin(), opt_vars.end(), mand_vars.begin(), mand_vars.end(), inserter(vars, vars.begin()));
    set< pair< SymTok, SymTok > > dists;
    for (auto &frame : this->stack) {
        for (auto &dist : frame.dists) {
            if (vars.find(dist.first) != vars.end() && vars.find(dist.second) != vars.end()) {
                if (mand_vars.find(dist.first) == mand_vars.end() || mand_vars.find(dist.second) == mand_vars.end()) {
                    dists.insert(dist);
                }
            }
        }
    }
    return dists;
}

void Reader::parse_a()
{
    // Usual sanity checks and symbol conversion
    assert_or_throw< MMPPParsingError >(this->label != 0, "Missing label in $a statement");
    assert_or_throw< MMPPParsingError >(this->toks.size() >= 1, "Empty $a statement");
    vector< SymTok > tmp;
    for (auto &stok : this->toks) {
        SymTok tok = this->lib.get_symbol(stok);
        assert_or_throw< MMPPParsingError >(tok != 0, "Symbol in $a statement is not defined");
        assert(this->check_const(tok) || this->check_var(tok));
        tmp.push_back(tok);
    }
    assert_or_throw< MMPPParsingError >(this->check_const(tmp[0]), "First symbol of $a statement is not a constant");
    this->lib.add_sentence(this->label, tmp);

    // Collect things
    set< SymTok > mand_vars = this->collect_mand_vars(tmp);
    vector< LabTok > float_hyps, ess_hyps;
    tie(float_hyps, ess_hyps) = this->collect_mand_hyps(mand_vars);
    set< pair< SymTok, SymTok > > mand_dists = this->collect_mand_dists(mand_vars);

    // Finally build assertion
    Assertion ass(false, mand_dists, {}, float_hyps, ess_hyps, {}, this->label, this->number, this->last_comment);
    this->number++;
    this->last_comment = "";
    this->lib.add_assertion(this->label, ass);
}

void Reader::parse_p()
{
    // Usual sanity checks and symbol conversion
    assert_or_throw< MMPPParsingError >(this->label != 0, "Missing label in $p statement");
    assert_or_throw< MMPPParsingError >(this->toks.size() >= 1, "Empty $p statement");
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
            assert_or_throw< MMPPParsingError >(tok != 0, "Symbol in $p statement is not defined");
            assert(this->check_const(tok) || this->check_var(tok));
            tmp.push_back(tok);
        } else {
            assert_or_throw< MMPPParsingError >(compressed_proof != 3, "Additional tokens in an incomplete proof");
            if (compressed_proof == 0) {
                if (stok == "(") {
                    compressed_proof = 1;
                    continue;
                } else if (stok == "?") {
                    // The proof is marked incomplete, we record a dummy one
                    compressed_proof = 3;
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
                    assert_or_throw< MMPPParsingError >(tok != 0, "Label in compressed proof in $p statement is not defined");
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
                assert_or_throw< MMPPParsingError >(tok != 0, "Symbol in uncompressed proof in $p statement is not defined");
                proof_labels.push_back(tok);
            }
        }
    }
    assert_or_throw< MMPPParsingError >(this->check_const(tmp[0]), "First symbol of $p statement is not a constant");
    assert(compressed_proof == -1 || compressed_proof == 2 || compressed_proof == 3);
    this->lib.add_sentence(this->label, tmp);

    // Collect things
    set< SymTok > mand_vars = this->collect_mand_vars(tmp);
    vector< LabTok > float_hyps, ess_hyps;
    tie(float_hyps, ess_hyps) = this->collect_mand_hyps(mand_vars);
    set< pair< SymTok, SymTok > > mand_dists = this->collect_mand_dists(mand_vars);
    set< SymTok > opt_vars;
    if (compressed_proof == -1) {
        opt_vars = this->collect_opt_vars(proof_labels, mand_vars);
    } else if (compressed_proof == 2) {
        opt_vars = this->collect_opt_vars(proof_refs, mand_vars);
    }
    set< LabTok > opt_hyps = this->collect_opt_hyps(opt_vars);
    set< pair< SymTok, SymTok > > opt_dists = this->collect_opt_dists(opt_vars, mand_vars);

    // Finally build assertion and attach proof
    Assertion ass(true, mand_dists, opt_dists, float_hyps, ess_hyps, opt_hyps, this->label, this->number, this->last_comment);
    this->number++;
    this->last_comment = "";
    if (compressed_proof != 3) {
        shared_ptr< Proof > proof;
        if (compressed_proof < 0) {
            proof = shared_ptr< Proof > (new UncompressedProof(proof_labels));
        } else {
            proof = shared_ptr< Proof > (new CompressedProof(proof_refs, proof_codes));
        }
        ass.set_proof(proof);
        auto pe = ass.get_proof_executor(this->lib);
        assert_or_throw< MMPPParsingError >(pe->check_syntax(), "Syntax check failed for proof of $p statement");
        if (this->execute_proofs) {
            pe = ass.get_proof_executor(this->lib);
            pe->set_debug_output("executing " + lib.resolve_label(this->label));
            pe->execute();
        }
    }
    this->lib.add_assertion(this->label, ass);
}

void Reader::process_comment(const string &comment)
{
    if (this->store_comments) {
        this->last_comment = comment;
    }
    bool found_dollar = false;
    size_t i = 0;
    for (auto &c : comment) {
        if (!is_mm_whitespace(c)) {
            if (c == '$') {
                found_dollar = true;
            } else {
                if (found_dollar) {
                    if (c == 't') {
                        this->t_comment = string(comment.begin()+i+1, comment.end());
                    } else if (c == 'j') {
                        this->j_comment += string(comment.begin()+i+1, comment.end());
                    }
                    return;
                } else {
                    // Either the $ token is at the beginning or the comment is discarded
                    return;
                }
            }
        }
        i++;
    }
}

static string escape_string_literal(string s) {
    bool escape = false;
    vector< char > ret;
    for (char c : s) {
        if (escape) {
            if (c == 'n') {
                ret.push_back('\n');
            } else if (c == '\\') {
                ret.push_back('\\');
            } else {
                throw MMPPParsingError("Unknown escape sequence");
            }
            escape = false;
        } else {
            if (c == '\\') {
                escape = true;
            } else {
                ret.push_back(c);
            }
        }
    }
    if (escape) {
        throw MMPPParsingError("Broken escape sequence");
    }
    return string(ret.begin(), ret.end());
}

static string decode_string(vector< pair< bool, string > >::const_iterator begin, vector< pair< bool, string > >::const_iterator end, bool escape=false) {
    assert_or_throw< MMPPParsingError >(distance(begin, end) >= 1);
    assert_or_throw< MMPPParsingError >(distance(begin, end) % 2 == 1);
    for (size_t i = 1; i < (size_t) distance(begin, end); i += 2) {
        assert_or_throw< MMPPParsingError >(!(begin+i)->first, "Malformed string in $t comment");
        assert_or_throw< MMPPParsingError >((begin+i)->second == "+", "Malformed string in $t comment");
    }
    string value;
    for (size_t i = 0; i < (size_t) distance(begin, end); i += 2) {
        assert_or_throw< MMPPParsingError >((begin+i)->first, "Malformed string in $t comment");
        string tmp = (begin+i)->second;
        if (escape) {
            tmp = escape_string_literal(tmp);
        }
        value += tmp;
    }
    return value;
}

void Reader::parse_j_code(const std::vector<std::vector<std::pair<bool, string> > > &code)
{
    ParsingAddendumImpl add;
    this->lib.set_parsing_addendum(add);
    for (auto &tokens : code) {
        assert_or_throw< MMPPParsingError >(tokens.size() > 0, "empty instruction in $j comment");
        assert_or_throw< MMPPParsingError >(!tokens.at(0).first, "instruction in $j comment begins with a string");
        const string &command = tokens.at(0).second;
        if (command == "syntax") {
            assert_or_throw< MMPPParsingError >(tokens.size() >= 2, "syntax instruction in $j comment with wrong length");
            assert_or_throw< MMPPParsingError >(tokens.at(1).first, "malformed syntax instruction in $j comment");
            SymTok tok1 = this->lib.get_symbol(tokens.at(1).second);
            assert_or_throw< MMPPParsingError >(tok1 != 0, "unknown symbol in syntax instruction in $j comment");
            SymTok tok2 = tok1;
            if (tokens.size() > 2) {
                assert_or_throw< MMPPParsingError >(tokens.size() == 4, "syntax instruction in $j comment with wrong length");
                assert_or_throw< MMPPParsingError >(!tokens.at(2).first, "malformed syntax instruction in $j comment");
                assert_or_throw< MMPPParsingError >(tokens.at(3).first, "malformed syntax instruction in $j comment");
                assert_or_throw< MMPPParsingError >(tokens.at(2).second == "as", "malformed syntax instruction in $j comment");
                tok2 = this->lib.get_symbol(tokens.at(3).second);
                assert_or_throw< MMPPParsingError >(tok2 != 0, "unknown symbol in syntax instruction in $j comment");
            }
            add.syntax[tok1] = tok2;
        } else if (command == "unambiguous") {
            add.unambiguous = decode_string(tokens.begin() + 1, tokens.end(), true);
        } else if (command == "definition") {
            // FIXME Implement missing commands
        } else if (command == "condcongruence") {
        } else if (command == "congruence") {
        } else if (command == "notfree") {
        } else if (command == "primitive") {
        } else if (command == "justification") {
        } else if (command == "condequality") {
        } else if (command == "equality") {
        } else {
            //cerr << "Uknown command " << tokens.at(0).second << endl;
            throw MMPPParsingError("unknown command in $j comment");
        }
    }
    this->lib.set_parsing_addendum(add);
}

void Reader::parse_t_code(const vector< vector< pair< bool, string > > > &code) {
    LibraryAddendumImpl add;
    for (auto &tokens : code) {
        assert_or_throw< MMPPParsingError >(tokens.size() > 0, "empty instruction in $t comment");
        assert_or_throw< MMPPParsingError >(!tokens.at(0).first, "instruction in $t comment begins with a string");
        int deftype = 0;
        if (tokens.at(0).second == "htmldef") {
            deftype = 1;
        } else if (tokens.at(0).second == "althtmldef") {
            deftype = 2;
        } else if (tokens.at(0).second == "latexdef") {
            deftype = 3;
        } else if (tokens.at(0).second == "htmlcss") {
            add.htmlcss = decode_string(tokens.begin() + 1, tokens.end(), true);
        } else if (tokens.at(0).second == "htmlfont") {
            add.htmlfont = decode_string(tokens.begin() + 1, tokens.end(), true);
        } else if (tokens.at(0).second == "htmltitle") {
            add.htmltitle = decode_string(tokens.begin() + 1, tokens.end(), true);
        } else if (tokens.at(0).second == "htmlhome") {
            add.htmlhome = decode_string(tokens.begin() + 1, tokens.end(), true);
        } else if (tokens.at(0).second == "htmlbibliography") {
            add.htmlbibliography = decode_string(tokens.begin() + 1, tokens.end(), true);
        } else if (tokens.at(0).second == "exthtmltitle") {
            add.exthtmltitle = decode_string(tokens.begin() + 1, tokens.end(), true);
        } else if (tokens.at(0).second == "exthtmlhome") {
            add.exthtmlhome = decode_string(tokens.begin() + 1, tokens.end(), true);
        } else if (tokens.at(0).second == "exthtmllabel") {
            add.exthtmllabel = decode_string(tokens.begin() + 1, tokens.end(), true);
        } else if (tokens.at(0).second == "exthtmlbibliography") {
            add.exthtmlbibliography = decode_string(tokens.begin() + 1, tokens.end(), true);
        } else if (tokens.at(0).second == "htmlvarcolor") {
            add.htmlvarcolor = decode_string(tokens.begin() + 1, tokens.end(), true);
        } else if (tokens.at(0).second == "htmldir") {
            add.htmldir = decode_string(tokens.begin() + 1, tokens.end(), true);
        } else if (tokens.at(0).second == "althtmldir") {
            add.althtmldir = decode_string(tokens.begin() + 1, tokens.end(), true);
        } else {
            throw new MMPPParsingError("unknown command in $t comment");
        }
        if (deftype != 0) {
            assert_or_throw< MMPPParsingError >(tokens.size() >= 4, "*def instruction in $t comment with wrong length");
            assert_or_throw< MMPPParsingError >(tokens.size() % 2 == 0, "*def instruction in $t comment with wrong length");
            assert_or_throw< MMPPParsingError >(tokens.at(1).first, "malformed *def instruction in $t comment");
            assert_or_throw< MMPPParsingError >(!tokens.at(2).first, "malformed *def instruction in $t comment");
            assert_or_throw< MMPPParsingError >(tokens.at(3).first, "malformed *def instruction in $t comment");
            assert_or_throw< MMPPParsingError >(tokens.at(2).second == "as", "malformed *def instruction in $t comment");
            string value = decode_string(tokens.begin() + 3, tokens.end());
            SymTok tok = this->lib.get_symbol(tokens.at(1).second);
            assert_or_throw< MMPPParsingError >(tok != 0, "unknown symbol in *def instruction in $t comment");
            if (deftype == 1) {
                add.htmldefs.resize(max(add.htmldefs.size(), (size_t) tok+1));
                add.htmldefs[tok] = value;
            } else if (deftype == 2) {
                add.althtmldefs.resize(max(add.althtmldefs.size(), (size_t) tok+1));
                add.althtmldefs[tok] = value;
            } else if (deftype == 3) {
                add.latexdefs.resize(max(add.latexdefs.size(), (size_t) tok+1));
                add.latexdefs[tok] = value;
            }
        }
    }
    this->lib.set_addendum(add);
}

std::vector<std::vector<std::pair<bool, std::string> > > Reader::parse_comment(const string &comment)
{
    // 0 -> normal
    // 1 -> comment
    // 2 -> string with "
    // 3 -> string with '
    // 4 -> normal, found /
    // 5 -> comment, found *
    // 6 -> normal, expect whitespace (or ;)
    // 7 -> wait $
    // 8 -> skip one char
    uint8_t state = 0;
    vector< vector< pair< bool, string > > > code;
    vector< pair< bool, string > > tokens;
    vector< char > token;
    for (char c : comment) {
        if (state == 0) {
            state0:
            if (is_mm_whitespace(c)) {
                if (token.size() > 0) {
                    tokens.push_back(make_pair(false, string(token.begin(), token.end())));
                    token.clear();
                }
            } else if (c == ';') {
                if (token.size() > 0) {
                    tokens.push_back(make_pair(false, string(token.begin(), token.end())));
                    token.clear();
                }
                code.push_back(tokens);
                tokens.clear();
            } else if (c == '/') {
                state = 4;
            } else if (c == '"') {
                assert_or_throw< MMPPParsingError >(token.empty(), "$t string began during token");
                state = 2;
            } else if (c == '\'') {
                assert_or_throw< MMPPParsingError >(token.empty(), "$t string began during token");
                state = 3;
            } else {
                token.push_back(c);
            }
        } else if (state == 1) {
            if (c == '*') {
                state = 5;
            }
        } else if (state == 2) {
            if (c == '"') {
                tokens.push_back(make_pair(true, string(token.begin(), token.end())));
                token.clear();
                state = 6;
            } else {
                token.push_back(c);
            }
        } else if (state == 3) {
            if (c == '\'') {
                tokens.push_back(make_pair(true, string(token.begin(), token.end())));
                token.clear();
                state = 6;
            } else {
                token.push_back(c);
            }
        } else if (state == 4) {
            if (c == '*') {
                state = 1;
            } else {
                token.push_back('*');
                state = 0;
                goto state0;
            }
        } else if (state == 5) {
            if (c == '/') {
                state = 0;
            }
        } else if (state == 6) {
            if (c == ';') {
                assert(token.empty());
                code.push_back(tokens);
                tokens.clear();
            } else {
                assert_or_throw< MMPPParsingError >(is_mm_whitespace(c), "no whitespace after $t string");
            }
            state = 0;
        } else if (state == 7) {
            if (c == '$') {
                state = 8;
            } else {
                assert_or_throw< MMPPParsingError >(is_mm_whitespace(c), "where is $t gone?");
            }
        } else if (state == 8) {
            state = 0;
        } else {
            assert("Should not arrive here" == NULL);
        }
    }
    return code;
}

void Reader::parse_t_comment(const string &comment)
{
    auto code = this->parse_comment(comment);
    this->parse_t_code(code);
}

void Reader::parse_j_comment(const string &comment)
{
    auto code = this->parse_comment(comment);
    this->parse_j_code(code);
}

bool Reader::check_var(SymTok tok) const
{
    for (auto &frame : this->stack) {
        if (frame.vars.find(tok) != frame.vars.end()) {
            return true;
        }
    }
    return false;
}

bool Reader::check_const(SymTok tok) const
{
    return this->consts.find(tok) != this->consts.end();
}

bool Reader::check_type(LabTok tok) const
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
    if (is_mm_whitespace(c)) {
        return -1;
    }
    assert_or_throw< MMPPParsingError >('A' <= c && c <= 'Z', "Invalid character in compressed proof");
    if (c == 'Z') {
        assert_or_throw< MMPPParsingError >(this->current == 0, "Invalid character Z in compressed proof");
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
    assert_or_throw< MMPPParsingError >(x != INVALID_CODE);
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

TokenGenerator::~TokenGenerator()
{
}
