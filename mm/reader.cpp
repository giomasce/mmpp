
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

void Reader::run () {
    //cout << "Running the reader" << endl;
    //auto t = tic();
    std::pair< bool, std::string > token_pair;
    this->label = 0;
    assert(this->stack.empty());
    this->stack.emplace_back();
    while ((token_pair = this->next_token()).second != "") {
        bool &comment = token_pair.first;
        std::string &token = token_pair.second;
        if (comment) {
            this->process_comment(token);
            continue;
        }
        if (token[0] == '$') {
            gio::assert_or_throw< MMPPParsingError >(token.size() == 2, "Dollar sequence with wrong length");
            char c = token[1];

            // Parse scoping blocks
            if (c == '{') {
                this->stack.emplace_back();
                continue;
            } else if (c == '}') {
                gio::assert_or_throw< MMPPParsingError >(!this->stack.empty(), "Unmatched closed scoping block");
                this->stack.pop_back();
                continue;
            }

            // Collect tokens in statement
            while ((token_pair = this->next_token()).second != "$.") {
                bool &comment = token_pair.first;
                std::string &token = token_pair.second;
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
            gio::assert_or_throw< MMPPParsingError >(this->label != LabTok{}, "Repeated label detected");
            //cout << "Found label " << token << endl;
        }
    }
    this->final_frame = this->stack.back();
    this->lib.set_final_stack_frame(this->final_frame);
    this->lib.set_max_number(LabTok(this->number.val()-1));
    this->stack.pop_back();
    gio::assert_or_throw< MMPPParsingError >(this->stack.empty(), "Unmatched open scoping block");

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

std::pair<bool, std::string> Reader::next_token()
{
    return this->tg->next();
}

const StackFrame &Reader::get_final_frame() const
{
    return this->final_frame;
}

void Reader::parse_c()
{
    gio::assert_or_throw< MMPPParsingError >(this->label == LabTok{}, "Undue label in $c statement");
    gio::assert_or_throw< MMPPParsingError >(this->stack.size() == 1, "Found $c statement when not in top-level scope");
    for (auto stok : this->toks) {
        SymTok tok = this->lib.create_symbol(stok);
        gio::assert_or_throw< MMPPParsingError >(!this->check_const(tok), "Symbol already bound in $c statement");
        gio::assert_or_throw< MMPPParsingError >(!this->check_var(tok), "Symbol already bound in $c statement");
        this->consts.insert(tok);
        this->lib.set_constant(tok, true);
    }
}

void Reader::parse_v()
{
    gio::assert_or_throw< MMPPParsingError >(this->label == LabTok{}, "Undue label in $v statement");
    for (auto stok : this->toks) {
        SymTok tok = this->lib.create_or_get_symbol(stok);
        gio::assert_or_throw< MMPPParsingError >(!this->check_const(tok), "Symbol already bound in $v statement");
        gio::assert_or_throw< MMPPParsingError >(!this->check_var(tok), "Symbol already bound in $v statement");
        this->lib.set_constant(tok, false);
        this->stack.back().vars.insert(tok);
    }
}

void Reader::parse_f()
{
    gio::assert_or_throw< MMPPParsingError >(this->label != LabTok{}, "Missing label in $f statement");
    gio::assert_or_throw< MMPPParsingError >(this->toks.size() == 2, "Found $f statement with wrong length");
    SymTok const_tok = this->lib.get_symbol(this->toks[0]);
    SymTok var_tok = this->lib.get_symbol(this->toks[1]);
    gio::assert_or_throw< MMPPParsingError >(const_tok != SymTok{}, "First member of a $f statement is not defined");
    gio::assert_or_throw< MMPPParsingError >(var_tok != SymTok{}, "Second member of a $f statement is not defined");
    gio::assert_or_throw< MMPPParsingError >(this->check_const(const_tok), "First member of a $f statement is not a constant");
    gio::assert_or_throw< MMPPParsingError >(this->check_var(var_tok), "Second member of a $f statement is not a variable");
    this->lib.add_sentence(this->label, { const_tok, var_tok }, SentenceType::FLOATING_HYP);
    this->stack.back().types.push_back(this->label);
    this->stack.back().types_set.insert(this->label);
}

void Reader::parse_e()
{
    gio::assert_or_throw< MMPPParsingError >(this->label != LabTok{}, "Missing label in $e statement");
    gio::assert_or_throw< MMPPParsingError >(this->toks.size() >= 1, "Empty $e statement");
    std::vector< SymTok > tmp;
    for (auto &stok : this->toks) {
        SymTok tok = this->lib.get_symbol(stok);
        gio::assert_or_throw< MMPPParsingError >(tok != SymTok{}, "Symbol in $e statement is not defined");
        assert(this->check_const(tok) || this->check_var(tok));
        tmp.push_back(tok);
    }
    gio::assert_or_throw< MMPPParsingError >(this->check_const(tmp[0]), "First symbol of $e statement is not a constant");
    this->lib.add_sentence(this->label, tmp, SentenceType::ESSENTIAL_HYP);
    this->stack.back().hyps.push_back(this->label);
}

void Reader::parse_d()
{
    gio::assert_or_throw< MMPPParsingError >(this->label == LabTok{}, "Undue label in $d statement");
    for (auto it = this->toks.begin(); it != this->toks.end(); it++) {
        SymTok tok1 = this->lib.get_symbol(*it);
        gio::assert_or_throw< MMPPParsingError >(this->check_var(tok1), "Symbol in $d statement is not a variable");
        for (auto it2 = it+1; it2 != this->toks.end(); it2++) {
            SymTok tok2 = this->lib.get_symbol(*it2);
            gio::assert_or_throw< MMPPParsingError >(this->check_var(tok2), "Symbol in $d statement is not a variable");
            gio::assert_or_throw< MMPPParsingError >(tok1 != tok2, "Repeated symbol in $d statement");
            this->stack.back().dists.insert(std::minmax(tok1, tok2));
        }
    }
}

void Reader::collect_vars_from_sentence(std::set<SymTok> &vars, const std::vector<SymTok> &sent) const {
    return collect_variables(sent, [this](auto tok) { return this->check_var(tok); }, vars);
}

void Reader::collect_vars_from_proof(std::set<SymTok> &vars, const std::vector<LabTok> &proof) const
{
    for (auto &tok : proof) {
        if (this->check_type(tok)) {
            gio::assert_or_throw< MMPPParsingError >(this->lib.get_sentence(tok).size() == 2, "Type has wrong size");
            vars.insert(this->lib.get_sentence(tok).at(1));
        }
    }
}

std::set< SymTok > Reader::collect_mand_vars(const std::vector<SymTok> &sent) const {
    std::set< SymTok > vars;
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
    std::set< SymTok > vars;
    this->collect_vars_from_proof(vars, proof);
    std::set< SymTok > opt_vars;
    std::set_difference(vars.begin(), vars.end(), mand_vars.begin(), mand_vars.end(), std::inserter(opt_vars, opt_vars.begin()));
    return opt_vars;
}

// Here order matters! Be careful!
std::pair< std::vector< LabTok >, std::vector< LabTok > > Reader::collect_mand_hyps(std::set< SymTok > vars) const {
    std::vector< LabTok > float_hyps, ess_hyps;

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
    std::set< LabTok > ret;
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

std::set< std::pair< SymTok, SymTok > > Reader::collect_mand_dists(std::set< SymTok > vars) const {
    std::set< std::pair< SymTok, SymTok > > dists;
    for (auto &frame : this->stack) {
        for (auto &dist : frame.dists) {
            if (vars.find(dist.first) != vars.end() && vars.find(dist.second) != vars.end()) {
                dists.insert(dist);
            }
        }
    }
    return dists;
}

std::set< std::pair< SymTok, SymTok > > Reader::collect_opt_dists(std::set< SymTok > opt_vars, std::set< SymTok > mand_vars) const {
    std::set< SymTok > vars;
    std::set_union(opt_vars.begin(), opt_vars.end(), mand_vars.begin(), mand_vars.end(), std::inserter(vars, vars.begin()));
    std::set< std::pair< SymTok, SymTok > > dists;
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
    gio::assert_or_throw< MMPPParsingError >(this->label != LabTok{}, "Missing label in $a statement");
    gio::assert_or_throw< MMPPParsingError >(this->toks.size() >= 1, "Empty $a statement");
    std::vector< SymTok > tmp;
    for (auto &stok : this->toks) {
        SymTok tok = this->lib.get_symbol(stok);
        gio::assert_or_throw< MMPPParsingError >(tok != SymTok{}, "Symbol in $a statement is not defined");
        assert(this->check_const(tok) || this->check_var(tok));
        tmp.push_back(tok);
    }
    gio::assert_or_throw< MMPPParsingError >(this->check_const(tmp[0]), "First symbol of $a statement is not a constant");
    this->lib.add_sentence(this->label, tmp, SentenceType::AXIOM);

    // Collect things
    std::set< SymTok > mand_vars = this->collect_mand_vars(tmp);
    std::vector< LabTok > float_hyps, ess_hyps;
    std::tie(float_hyps, ess_hyps) = this->collect_mand_hyps(mand_vars);
    std::set< std::pair< SymTok, SymTok > > mand_dists = this->collect_mand_dists(mand_vars);

    // Finally build assertion
    Assertion ass(false, false, mand_dists, {}, float_hyps, ess_hyps, {}, this->label, this->number, this->last_comment);
    this->number = LabTok(this->number.val()+1);
    this->last_comment = "";
    this->lib.add_assertion(this->label, ass);
}

void Reader::parse_p()
{
    // Usual sanity checks and symbol conversion
    gio::assert_or_throw< MMPPParsingError >(this->label != LabTok{}, "Missing label in $p statement");
    gio::assert_or_throw< MMPPParsingError >(this->toks.size() >= 1, "Empty $p statement");
    std::vector< SymTok > tmp;
    std::vector< LabTok > proof_labels;
    std::vector< LabTok > proof_refs;
    std::vector< CodeTok > proof_codes;
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
            gio::assert_or_throw< MMPPParsingError >(tok != SymTok{}, "Symbol in $p statement is not defined");
            assert(this->check_const(tok) || this->check_var(tok));
            tmp.push_back(tok);
        } else {
            gio::assert_or_throw< MMPPParsingError >(compressed_proof != 3, "Additional tokens in an incomplete proof");
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
                    gio::assert_or_throw< MMPPParsingError >(tok != LabTok{}, "Label in compressed proof in $p statement is not defined");
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
                gio::assert_or_throw< MMPPParsingError >(tok != LabTok{}, "Symbol in uncompressed proof in $p statement is not defined");
                proof_labels.push_back(tok);
            }
        }
    }
    gio::assert_or_throw< MMPPParsingError >(this->check_const(tmp[0]), "First symbol of $p statement is not a constant");
    assert(compressed_proof == -1 || compressed_proof == 2 || compressed_proof == 3);
    this->lib.add_sentence(this->label, tmp, SentenceType::PROPOSITION);

    // Collect things
    std::set< SymTok > mand_vars = this->collect_mand_vars(tmp);
    std::vector< LabTok > float_hyps, ess_hyps;
    std::tie(float_hyps, ess_hyps) = this->collect_mand_hyps(mand_vars);
    std::set< std::pair< SymTok, SymTok > > mand_dists = this->collect_mand_dists(mand_vars);
    std::set< SymTok > opt_vars;
    if (compressed_proof == -1) {
        opt_vars = this->collect_opt_vars(proof_labels, mand_vars);
    } else if (compressed_proof == 2) {
        opt_vars = this->collect_opt_vars(proof_refs, mand_vars);
    }
    std::set< LabTok > opt_hyps = this->collect_opt_hyps(opt_vars);
    std::set< std::pair< SymTok, SymTok > > opt_dists = this->collect_opt_dists(opt_vars, mand_vars);

    // Finally build assertion and attach proof
    Assertion ass(true, compressed_proof != 3, mand_dists, opt_dists, float_hyps, ess_hyps, opt_hyps, this->label, this->number, this->last_comment);
    this->number = LabTok(this->number.val()+1);
    this->last_comment = "";
    if (compressed_proof != 3) {
        std::shared_ptr< Proof > proof;
        if (compressed_proof < 0) {
            proof = std::make_shared< UncompressedProof > (proof_labels);
        } else {
            proof = std::make_shared< CompressedProof >(proof_refs, proof_codes);
        }
        ass.set_proof(proof);
        auto po = ass.get_proof_operator(this->lib);
        gio::assert_or_throw< MMPPParsingError >(po->check_syntax(), "Syntax check failed for proof of $p statement");
        if (this->execute_proofs) {
            auto pe = ass.get_proof_executor< Sentence >(this->lib);
            pe->set_debug_output("executing " + lib.resolve_label(this->label));
            pe->execute();
        }
    }
    this->lib.add_assertion(this->label, ass);
}

void Reader::process_comment(const std::string &comment)
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
                        this->t_comment = std::string(comment.begin()+i+1, comment.end());
                    } else if (c == 'j') {
                        this->j_comment += std::string(comment.begin()+i+1, comment.end());
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

static std::string escape_string_literal(std::string s) {
    bool escape = false;
    std::vector< char > ret;
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
    return std::string(ret.begin(), ret.end());
}

static std::string decode_string(std::vector< std::pair< bool, std::string > >::const_iterator begin, std::vector< std::pair< bool, std::string > >::const_iterator end, bool escape=false) {
    gio::assert_or_throw< MMPPParsingError >(distance(begin, end) >= 1, "Malformed string in $t comment");
    gio::assert_or_throw< MMPPParsingError >(distance(begin, end) % 2 == 1, "Malformed string in $t comment");
    for (size_t i = 1; i < (size_t) distance(begin, end); i += 2) {
        gio::assert_or_throw< MMPPParsingError >(!(begin+i)->first, "Malformed string in $t comment");
        gio::assert_or_throw< MMPPParsingError >((begin+i)->second == "+", "Malformed string in $t comment");
    }
    std::string value;
    for (size_t i = 0; i < (size_t) std::distance(begin, end); i += 2) {
        gio::assert_or_throw< MMPPParsingError >((begin+i)->first, "Malformed string in $t comment");
        std::string tmp = (begin+i)->second;
        if (escape) {
            tmp = escape_string_literal(tmp);
        }
        value += tmp;
    }
    return value;
}

void Reader::parse_j_code(const std::vector<std::vector<std::pair<bool, std::string> > > &code)
{
    ParsingAddendumImpl add;
    this->lib.set_parsing_addendum(add);
    for (auto &tokens : code) {
        gio::assert_or_throw< MMPPParsingError >(tokens.size() > 0, "empty instruction in $j comment");
        gio::assert_or_throw< MMPPParsingError >(!tokens.at(0).first, "instruction in $j comment begins with a string");
        const std::string &command = tokens.at(0).second;
        if (command == "syntax") {
            gio::assert_or_throw< MMPPParsingError >(tokens.size() >= 2, "syntax instruction in $j comment with wrong length");
            gio::assert_or_throw< MMPPParsingError >(tokens.at(1).first, "malformed syntax instruction in $j comment");
            SymTok tok1 = this->lib.get_symbol(tokens.at(1).second);
            gio::assert_or_throw< MMPPParsingError >(tok1 != SymTok{}, "unknown symbol in syntax instruction in $j comment");
            SymTok tok2 = tok1;
            if (tokens.size() > 2) {
                gio::assert_or_throw< MMPPParsingError >(tokens.size() == 4, "syntax instruction in $j comment with wrong length");
                gio::assert_or_throw< MMPPParsingError >(!tokens.at(2).first, "malformed syntax instruction in $j comment");
                gio::assert_or_throw< MMPPParsingError >(tokens.at(3).first, "malformed syntax instruction in $j comment");
                gio::assert_or_throw< MMPPParsingError >(tokens.at(2).second == "as", "malformed syntax instruction in $j comment");
                tok2 = this->lib.get_symbol(tokens.at(3).second);
                gio::assert_or_throw< MMPPParsingError >(tok2 != SymTok{}, "unknown symbol in syntax instruction in $j comment");
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
        } else if (command == "bound") {
        } else if (command == "free_var") {
        } else if (command == "natded_init") {
        } else if (command == "natded_assume") {
        } else if (command == "natded_weak") {
        } else if (command == "natded_cut") {
        } else if (command == "natded_true") {
        } else if (command == "natded_imp") {
        } else if (command == "natded_and") {
        } else if (command == "natded_or") {
        } else if (command == "natded_not") {
        } else {
            //cerr << "Uknown command " << tokens.at(0).second << endl;
            throw MMPPParsingError("unknown command " + command + " in $j comment");
        }
    }
    this->lib.set_parsing_addendum(add);
}

void Reader::parse_t_code(const std::vector< std::vector< std::pair< bool, std::string > > > &code) {
    LibraryAddendumImpl add;
    for (auto &tokens : code) {
        gio::assert_or_throw< MMPPParsingError >(tokens.size() > 0, "empty instruction in $t comment");
        gio::assert_or_throw< MMPPParsingError >(!tokens.at(0).first, "instruction in $t comment begins with a string");
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
            gio::assert_or_throw< MMPPParsingError >(tokens.size() >= 4, "*def instruction in $t comment with wrong length");
            gio::assert_or_throw< MMPPParsingError >(tokens.size() % 2 == 0, "*def instruction in $t comment with wrong length");
            gio::assert_or_throw< MMPPParsingError >(tokens.at(1).first, "malformed *def instruction in $t comment");
            gio::assert_or_throw< MMPPParsingError >(!tokens.at(2).first, "malformed *def instruction in $t comment");
            gio::assert_or_throw< MMPPParsingError >(tokens.at(3).first, "malformed *def instruction in $t comment");
            gio::assert_or_throw< MMPPParsingError >(tokens.at(2).second == "as", "malformed *def instruction in $t comment");
            std::string value = decode_string(tokens.begin() + 3, tokens.end());
            SymTok tok = this->lib.get_symbol(tokens.at(1).second);
            gio::assert_or_throw< MMPPParsingError >(tok != SymTok{}, "unknown symbol in *def instruction in $t comment");
            if (deftype == 1) {
                add.htmldefs.resize(std::max(add.htmldefs.size(), (size_t) tok.val()+1));
                add.htmldefs[tok.val()] = value;
            } else if (deftype == 2) {
                add.althtmldefs.resize(std::max(add.althtmldefs.size(), (size_t) tok.val()+1));
                add.althtmldefs[tok.val()] = value;
            } else if (deftype == 3) {
                add.latexdefs.resize(std::max(add.latexdefs.size(), (size_t) tok.val()+1));
                add.latexdefs[tok.val()] = value;
            }
        }
    }
    this->lib.set_addendum(add);
}

std::vector<std::vector<std::pair<bool, std::string> > > Reader::parse_comment(const std::string &comment)
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
    std::vector< std::vector< std::pair< bool, std::string > > > code;
    std::vector< std::pair< bool, std::string > > tokens;
    std::vector< char > token;
    for (char c : comment) {
        if (state == 0) {
            state0:
            if (is_mm_whitespace(c)) {
                if (token.size() > 0) {
                    tokens.push_back(std::make_pair(false, std::string(token.begin(), token.end())));
                    token.clear();
                }
            } else if (c == ';') {
                if (token.size() > 0) {
                    tokens.push_back(std::make_pair(false, std::string(token.begin(), token.end())));
                    token.clear();
                }
                code.push_back(tokens);
                tokens.clear();
            } else if (c == '/') {
                state = 4;
            } else if (c == '"') {
                gio::assert_or_throw< MMPPParsingError >(token.empty(), "$t string began during token");
                state = 2;
            } else if (c == '\'') {
                gio::assert_or_throw< MMPPParsingError >(token.empty(), "$t string began during token");
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
                tokens.push_back(make_pair(true, std::string(token.begin(), token.end())));
                token.clear();
                state = 6;
            } else {
                token.push_back(c);
            }
        } else if (state == 3) {
            if (c == '\'') {
                tokens.push_back(make_pair(true, std::string(token.begin(), token.end())));
                token.clear();
                state = 6;
            } else {
                token.push_back(c);
            }
        } else if (state == 4) {
            if (c == '*') {
                state = 1;
            } else {
                /* If we have found a '/' which is not followed by '*', then first
                 * we push the '/', then we jump to state 0 and reprocess the current
                 * token. */
                token.push_back('/');
                state = 0;
                goto state0;
            }
        } else if (state == 5) {
            if (c == '/') {
                state = 0;
            } else if (c == '*') {
                state = 5;
            } else {
                state = 1;
            }
        } else if (state == 6) {
            if (c == ';') {
                assert(token.empty());
                code.push_back(tokens);
                tokens.clear();
            } else {
                gio::assert_or_throw< MMPPParsingError >(is_mm_whitespace(c), "no whitespace after $t string");
            }
            state = 0;
        } else if (state == 7) {
            if (c == '$') {
                state = 8;
            } else {
                gio::assert_or_throw< MMPPParsingError >(is_mm_whitespace(c), "where is $t gone?");
            }
        } else if (state == 8) {
            state = 0;
        } else {
            assert("Should not arrive here" == nullptr);
        }
    }
    return code;
}

void Reader::parse_t_comment(const std::string &comment)
{
    auto code = this->parse_comment(comment);
    this->parse_t_code(code);
}

void Reader::parse_j_comment(const std::string &comment)
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

Reader::Reader(TokenGenerator &tg, bool execute_proofs, bool store_comments) :
    tg(&tg), execute_proofs(execute_proofs), store_comments(store_comments),
    number(1)
{
}
