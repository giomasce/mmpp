
#include "mmutils.h"

ParsingTree< SymTok, LabTok > create_var_pt(const LibraryToolbox &tb, SymTok var) {
    ParsingTree< SymTok, LabTok > ret;
    assert(tb.get_standard_is_var_sym()(var));
    ret.type = tb.get_var_sym_to_type_sym(var);
    ret.label = tb.get_var_sym_to_lab(var);
    assert(ret.validate(tb.get_validation_rule()));
    return ret;
}

ParsingTree< SymTok, LabTok > create_temp_var_pt(const LibraryToolbox &tb, SymTok type) {
    auto var_data = tb.new_temp_var(type);
    return create_var_pt(tb, var_data.second);
}

ParsingTree< SymTok, LabTok > create_pt(const LibraryToolbox &tb, const std::string &templ_str, const std::map< std::string, ParsingTree< SymTok, LabTok > > &subst_str) {
    auto templ = tb.parse_sentence(tb.read_sentence(templ_str));
    SubstMap< SymTok, LabTok > subst;
    for (const auto &p : subst_str) {
        auto var_sym = tb.get_symbol(p.first);
        assert(var_sym != SymTok{});
        assert(tb.get_standard_is_var_sym()(var_sym));
        auto var_lab = tb.get_var_sym_to_lab(var_sym);
        subst[var_lab] = p.second;
    }
    auto ret = substitute(templ, tb.get_standard_is_var(), subst);
    ret.validate(tb.get_validation_rule());
    return ret;
}
