
#include <unordered_map>
#include <string>
#include <sstream>

#include <boost/format.hpp>
#include <boost/functional/hash.hpp>
#include <boost/algorithm/string.hpp>

#include <giolib/static_block.h>
#include <giolib/main.h>

#include "utils/utils.h"
#include "parsing/lr.h"
#include "parsing/earley.h"
#include "mm/setmm.h"
#include "mm/mmutils.h"
#include "parsing/unif.h"

bool recognize(const ParsingTree< SymTok, LabTok > &pt, const std::string &model, const LibraryToolbox &tb, SubstMap< SymTok, LabTok > &subst) {
    UnilateralUnificator< SymTok, LabTok > unif(tb.get_standard_is_var());
    auto model_pt = tb.parse_sentence(tb.read_sentence(model));
    model_pt.validate(tb.get_validation_rule());
    assert(model_pt.label != LabTok{});
    unif.add_parsing_trees(model_pt, pt);
    std::set< LabTok > model_vars;
    collect_variables(model_pt, tb.get_standard_is_var(), model_vars);
    bool ret;
    std::tie(ret, subst) = unif.unify();
    if (ret) {
        for (const auto &var : model_vars) {
            ParsingTree< SymTok, LabTok > pt_var;
            pt_var.label = var;
            pt_var.type = tb.get_var_lab_to_type_sym(var);
            subst.insert(std::make_pair(var, pt_var));
        }
    }
    return ret;
}

void convert_to_tstp(const ParsingTree< SymTok, LabTok > &pt, std::ostream &st, const LibraryToolbox &tb, const std::set< LabTok > &set_vars) {
    assert(pt.label != LabTok{});
    SubstMap< SymTok, LabTok > subst;
    if (recognize(pt, "wff A = B", tb, subst)) {
        convert_to_tstp(subst.at(tb.get_var_sym_to_lab(tb.get_symbol("A"))), st, tb, set_vars);
        st << "=";
        convert_to_tstp(subst.at(tb.get_var_sym_to_lab(tb.get_symbol("B"))), st, tb, set_vars);
    } else if (recognize(pt, "wff -. ph", tb, subst)) {
        st << "~";
        convert_to_tstp(subst.at(tb.get_var_sym_to_lab(tb.get_symbol("ph"))), st, tb, set_vars);
    } else if (recognize(pt, "wff ( ph -> ps )", tb, subst)) {
        st << "(";
        convert_to_tstp(subst.at(tb.get_var_sym_to_lab(tb.get_symbol("ph"))), st, tb, set_vars);
        st << "=>";
        convert_to_tstp(subst.at(tb.get_var_sym_to_lab(tb.get_symbol("ps"))), st, tb, set_vars);
        st << ")";
    } else if (recognize(pt, "wff ( ph /\\ ps )", tb, subst)) {
        st << "(";
        convert_to_tstp(subst.at(tb.get_var_sym_to_lab(tb.get_symbol("ph"))), st, tb, set_vars);
        st << "&";
        convert_to_tstp(subst.at(tb.get_var_sym_to_lab(tb.get_symbol("ps"))), st, tb, set_vars);
        st << ")";
    } else if (recognize(pt, "wff ( ph \\/ ps )", tb, subst)) {
        st << "(";
        convert_to_tstp(subst.at(tb.get_var_sym_to_lab(tb.get_symbol("ph"))), st, tb, set_vars);
        st << "|";
        convert_to_tstp(subst.at(tb.get_var_sym_to_lab(tb.get_symbol("ps"))), st, tb, set_vars);
        st << ")";
    } else if (recognize(pt, "wff ( ph <-> ps )", tb, subst)) {
        st << "(";
        convert_to_tstp(subst.at(tb.get_var_sym_to_lab(tb.get_symbol("ph"))), st, tb, set_vars);
        st << "<=>";
        convert_to_tstp(subst.at(tb.get_var_sym_to_lab(tb.get_symbol("ps"))), st, tb, set_vars);
        st << ")";
    } else if (recognize(pt, "wff A. x ph", tb, subst)) {
        st << "![";
        convert_to_tstp(subst.at(tb.get_var_sym_to_lab(tb.get_symbol("x"))), st, tb, set_vars);
        st << "]:";
        convert_to_tstp(subst.at(tb.get_var_sym_to_lab(tb.get_symbol("ph"))), st, tb, set_vars);
    } else if (recognize(pt, "wff E. x ph", tb, subst)) {
        st << "?[";
        convert_to_tstp(subst.at(tb.get_var_sym_to_lab(tb.get_symbol("x"))), st, tb, set_vars);
        st << "]:";
        convert_to_tstp(subst.at(tb.get_var_sym_to_lab(tb.get_symbol("ph"))), st, tb, set_vars);
    } else if (recognize(pt, "class x", tb, subst)) {
        st << boost::to_upper_copy(tb.resolve_symbol(tb.get_var_lab_to_sym(subst.at(tb.get_var_sym_to_lab(tb.get_symbol("x"))).label)));
    } else if (recognize(pt, "set x", tb, subst)) {
        st << boost::to_upper_copy(tb.resolve_symbol(tb.get_var_lab_to_sym(subst.at(tb.get_var_sym_to_lab(tb.get_symbol("x"))).label)));
    } else if (recognize(pt, "wff ph", tb, subst)) {
        st << boost::to_lower_copy(tb.resolve_symbol(tb.get_var_lab_to_sym(subst.at(tb.get_var_sym_to_lab(tb.get_symbol("ph"))).label)));
        if (!set_vars.empty()) {
            st << "(";
            bool first = true;
            for (const auto &x : set_vars) {
                if (first) {
                    first = false;
                } else {
                    st << ",";
                }
                st << boost::to_upper_copy(tb.resolve_symbol(tb.get_var_lab_to_sym(x)));
            }
            st << ")";
        }
    } else {
        throw MMPPException("Unknown syntax construct");
    }
}

int convert_to_tstp_main(int argc, char *argv[]) {
    (void) argc;

    auto &data = get_set_mm();
    //auto &lib = data.lib;
    auto &tb = data.tb;

    //auto pt = tb.parse_sentence(tb.read_sentence("|- A. x ( -. -. a = a -> ( x = y /\\ E. y -. y = z ) )"));
    //auto pt = tb.parse_sentence(tb.read_sentence("|- A. x ( a = a -> a = a )"));
    //auto pt = tb.parse_sentence(tb.read_sentence("|- ( ( ( y = z -> ( ( x = y -> ph ) /\\ E. x ( x = y /\\ ph ) ) ) /\\ E. y ( y = z /\\ ( ( x = y -> ph ) /\\ E. x ( x = y /\\ ph ) ) ) ) <-> ( ( y = z -> ( ( x = z -> ph ) /\\ E. x ( x = z /\\ ph ) ) ) /\\ E. y ( y = z /\\ ( ( x = z -> ph ) /\\ E. x ( x = z /\\ ph ) ) ) ) )"));
    //auto pt = tb.parse_sentence(tb.read_sentence("|- ph"));
    //auto pt = tb.parse_sentence(tb.read_sentence(("|- ( A. x ( ph -> ps ) -> ( E. x ph <-> E. x ( ph /\\  ps ) ) )")));
    auto pt = tb.parse_sentence(tb.read_sentence(argv[1]));
    assert(pt.label != LabTok{});
    pt.validate(tb.get_validation_rule());

    std::set< LabTok > set_vars;
    collect_variables(pt, std::function< bool(LabTok) >([&tb](auto x) { return tb.get_standard_is_var()(x) && tb.get_var_lab_to_type_sym(x) == tb.get_symbol("set"); }), set_vars);

    std::cout << "fof(1,conjecture,";
    convert_to_tstp(pt, std::cout, tb, set_vars);
    std::cout << ").";
    std::cout << std::endl;

    return 0;
}
gio_static_block {
    gio::register_main_function("convert_to_tstp", convert_to_tstp_main);
}

struct ReconstructFOF {
    const LibraryToolbox &tb;
    const std::set<std::pair<LabTok, LabTok>> &dists;
    std::map<LabTok, LabTok> open_vars;
    std::map<LabTok, std::set<LabTok>> params;
    std::vector<std::pair<LabTok, bool>> quants;

    ParsingTree<SymTok, LabTok> find_params(const ParsingTree<SymTok, LabTok> &pt, bool neg_depth = false) {
        assert(pt.label != LabTok{});
        SubstMap< SymTok, LabTok > subst;
        bool is_quant = false;
        bool forall = false;
        if (recognize(pt, "wff A. x ph", this->tb, subst)) {
            is_quant = true;
            forall = true;
        } else if (recognize(pt, "wff E. x ph", this->tb, subst)) {
            is_quant = true;
            forall = false;
        }
        if (is_quant) {
            LabTok var = subst.at(tb.get_var_sym_to_lab(tb.get_symbol("x"))).label;
            assert(this->open_vars.find(var) == this->open_vars.end());
            LabTok new_var = tb.new_temp_var(tb.get_symbol("set")).first;
            this->open_vars[var] = new_var;
            this->quants.push_back(std::make_pair(new_var, forall ^ neg_depth));
            auto ret = this->find_params(subst.at(tb.get_var_sym_to_lab(tb.get_symbol("ph"))), neg_depth);
            this->open_vars.erase(var);
            return ret;
        } else if (recognize(pt, "wff -. ph", this->tb, subst)) {
            auto ph_pt = this->find_params(subst.at(tb.get_var_sym_to_lab(tb.get_symbol("ph"))), !neg_depth);
            return create_pt(this->tb, "wff -. ph", {{"ph", ph_pt}});
        } else if (recognize(pt, "wff ( ph -> ps )", this->tb, subst)) {
            auto ph_pt = this->find_params(subst.at(tb.get_var_sym_to_lab(tb.get_symbol("ph"))), !neg_depth);
            auto ps_pt = this->find_params(subst.at(tb.get_var_sym_to_lab(tb.get_symbol("ps"))), neg_depth);
            return create_pt(this->tb, "wff ( ph -> ps )", {{"ph", ph_pt}, {"ps", ps_pt}});
        } else if (recognize(pt, "wff ( ph /\\ ps )", this->tb, subst)) {
            auto ph_pt = this->find_params(subst.at(tb.get_var_sym_to_lab(tb.get_symbol("ph"))), neg_depth);
            auto ps_pt = this->find_params(subst.at(tb.get_var_sym_to_lab(tb.get_symbol("ps"))), neg_depth);
            return create_pt(this->tb, "wff ( ph /\\ ps )", {{"ph", ph_pt}, {"ps", ps_pt}});
        } else if (recognize(pt, "wff ( ph \\/ ps )", this->tb, subst)) {
            auto ph_pt = this->find_params(subst.at(tb.get_var_sym_to_lab(tb.get_symbol("ph"))), neg_depth);
            auto ps_pt = this->find_params(subst.at(tb.get_var_sym_to_lab(tb.get_symbol("ps"))), neg_depth);
            return create_pt(this->tb, "wff ( ph \\/ ps )", {{"ph", ph_pt}, {"ps", ps_pt}});
        } else if (recognize(pt, "wff ph", this->tb, subst)) {
            auto var = subst.at(tb.get_var_sym_to_lab(tb.get_symbol("ph"))).label;
            std::set<LabTok> vars;
            for (const auto &open_var : this->open_vars) {
                if (this->dists.find(std::minmax(var, open_var.first)) == this->dists.end()) {
                    vars.insert(open_var.first);
                }
            }
            auto res = this->params.insert(std::make_pair(var, vars));
            if (!res.second) {
                assert(res.first->second == vars);
            }
            return pt;
        } else {
            throw MMPPException("Unknown syntax construct");
        }
    }
};

int quant_cnf_main(int argc, char *argv[]) {
    (void) argc;

    auto &data = get_set_mm();
    //auto &lib = data.lib;
    auto &tb = data.tb;

    auto pt = tb.parse_sentence(tb.read_sentence(argv[1]));
    assert(pt.label != LabTok{});
    pt.validate(tb.get_validation_rule());

    std::set<std::pair<LabTok, LabTok>> dists;
    dists.insert(std::minmax(tb.get_var_sym_to_lab(tb.get_symbol("x")), tb.get_var_sym_to_lab(tb.get_symbol("ps"))));
    ReconstructFOF rf = { tb, dists, {}, {}, {} };
    std::cout << tb.print_sentence(pt) << std::endl;
    std::cout << tb.print_sentence(rf.find_params(pt)) << std::endl;

    for (const auto &var : rf.params) {
        std::cout << tb.print_sentence(std::vector<SymTok>{tb.get_var_lab_to_sym(var.first)}) << ":";
        for (const auto &var2 : var.second) {
            std::cout << " " << tb.print_sentence(std::vector<SymTok>{tb.get_var_lab_to_sym(var2)});
        }
        std::cout << std::endl;
    }

    return 0;
}
gio_static_block {
    gio::register_main_function("quant_cnf", quant_cnf_main);
}
