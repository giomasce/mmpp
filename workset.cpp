#include "workset.h"

#include "json.h"

#include "reader.h"
#include "platform.h"
#include "jsonize.h"

using namespace std;
using namespace nlohmann;

Workset::Workset()
{
}

template< typename TokType >
std::vector< std::string > map_to_vect(const std::unordered_map< TokType, std::string > &m) {
    std::vector< std::string > ret;
    ret.resize(m.size()+1);
    for (auto &i : m) {
        ret[i.first] = i.second;
    }
    return ret;
}

json Workset::answer_api1(HTTPCallback &cb, std::vector< std::string >::const_iterator path_begin, std::vector< std::string >::const_iterator path_end, std::string method)
{
    (void) cb;
    (void) method;
    unique_lock< mutex > lock(this->global_mutex);
    if (path_begin == path_end) {
        json ret = json::object();
        return ret;
    }
    if (*path_begin == "load") {
        FileTokenizer ft(platform_get_resources_base() / "library.mm");
        Reader p(ft, false, true);
        p.run();
        this->library = make_unique< LibraryImpl >(p.get_library());
        json ret = { { "status", "ok" } };
        return ret;
    } else if (*path_begin == "get_context") {
        json ret;
        if (this->library == NULL) {
            ret["status"] = "unloaded";
            return ret;
        }
        ret["status"] = "loaded";
        ret["symbols"] = map_to_vect(this->library->get_symbols());
        ret["labels"] = map_to_vect(this->library->get_labels());
        const auto &addendum = this->library->get_addendum();
        ret["addendum"] = jsonize(addendum);
        ret["max_number"] = this->library->get_max_number();
        return ret;
    } else if (*path_begin == "get_sentence") {
        path_begin++;
        if (path_begin == path_end) {
            throw SendError(404);
        }
        int tok = safe_stoi(*path_begin);
        try {
            const Sentence &sent = this->library->get_sentence(tok);
            json ret;
            ret["sentence"] = sent;
            return ret;
        } catch (out_of_range e) {
            (void) e;
            throw SendError(404);
        }
    } else if (*path_begin == "get_assertion") {
        path_begin++;
        if (path_begin == path_end) {
            throw SendError(404);
        }
        int tok = safe_stoi(*path_begin);
        try {
            const Assertion &ass = this->library->get_assertion(tok);
            json ret;
            ret["assertion"] = jsonize(ass);
            return ret;
        } catch (out_of_range e) {
            (void) e;
            throw SendError(404);
        }
    } else if (*path_begin == "get_proof_tree") {
        path_begin++;
        if (path_begin == path_end) {
            throw SendError(404);
        }
        int tok = safe_stoi(*path_begin);
        try {
            const Assertion &ass = this->library->get_assertion(tok);
            const auto &executor = ass.get_proof_executor(*this->library, true);
            executor->execute();
            const auto &proof_tree = executor->get_proof_tree();
            json ret;
            ret["proof_tree"] = jsonize(proof_tree);
            return ret;
        } catch (out_of_range e) {
            (void) e;
            throw SendError(404);
        }
    }
    throw SendError(404);
}
