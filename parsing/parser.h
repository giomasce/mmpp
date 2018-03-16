#pragma once

#include <vector>
#include <unordered_map>
#include <cassert>

#include <boost/functional/hash.hpp>

template< typename SymType, typename LabType >
struct ParsingTree {
    LabType label;
    SymType type;
    std::vector< ParsingTree< SymType, LabType > > children;

    bool operator==(const ParsingTree< SymType, LabType > &other) const {
        // The type does not participate in equality check, since it is assumed that it is a function of the label
        return this->label == other.label && this->children == other.children;
    }

    bool operator!=(const ParsingTree< SymType, LabType > &other) const {
        return !this->operator==(other);
    }

    bool validate(const std::function< std::pair< SymType, std::vector< SymType > >(LabType) > &get_rule) const {
        if (this->label == LabType{}) {
            return false;
        }
        if (this->type == SymType{}) {
            return false;
        }
        auto model = get_rule(this->label);
        if (this->type != model.first) {
            return false;
        }
        if (this->children.size() != model.second.size()) {
            return false;
        }
        for (size_t i = 0; i < model.second.size(); i++) {
            if (this->children[i].type != model.second[i]) {
                return false;
            }
            if (!this->children[i].validate(get_rule)) {
                return false;
            }
        }
        return true;
    }
};

template< typename SymType, typename LabType >
struct ParsingTree2;

/*template< typename SymType, typename LabType >
struct ParsingTreeView;*/

template< typename SymType, typename LabType >
struct ParsingTreeNode {
    LabType label;
    SymType type;
    //size_t children_num;
    size_t descendants_num;

    bool operator==(const ParsingTreeNode< SymType, LabType > &other) const {
        return this->label == other.label && this->descendants_num == other.descendants_num;
    }

    bool operator!=(const ParsingTreeNode< SymType, LabType > &other) const {
        return !this->operator==(other);
    }

    bool operator<(const ParsingTreeNode< SymType, LabType > &other) const {
        return this->label < other.label || (this->label == other.label && this->descendants_num < other.descendants_num);
    }
};

namespace boost {
template< typename SymType, typename LabType >
struct hash< ParsingTreeNode< SymType, LabType > > {
    typedef ParsingTreeNode< SymType, LabType > argument_type;
    typedef std::size_t result_type;
    result_type operator()(const argument_type &x) const noexcept {
        result_type res = 0;
        boost::hash_combine(res, x.label);
        boost::hash_combine(res, x.descendants_num);
        return res;
    }
};
}

template< typename SymType, typename LabType >
struct ParsingTreeIterator {
    const ParsingTree2< SymType, LabType > &tree;
    size_t item_num;

    const ParsingTreeNode< SymType, LabType > &get_node () const {
        return this->tree.get_nodes()[this->item_num];
    }

    ParsingTreeIterator< SymType, LabType > get_first_child() const {
        return { this->tree, this->item_num + 1 };
    }

    ParsingTreeIterator< SymType, LabType > get_next_siebling() const {
        return { this->tree, this->item_num + 1 + this->get_node().descendants_num };
    }

    bool has_children() const {
        return this->get_node().descendants_num != 0;
    }

    ParsingTree2< SymType, LabType > get_view() const {
        return ParsingTree2< SymType, LabType >(this->tree.get_nodes() + this->item_num, this->get_node().descendants_num + 1);
    }

    // Iterator interface for accessing children

    ParsingTreeIterator< SymType, LabType > begin() const {
        return this->get_first_child();
    }

    ParsingTreeIterator< SymType, LabType > end() const {
        return this->get_next_siebling();
    }

    const ParsingTreeIterator< SymType, LabType > &operator*() const {
        return *this;
    }

    ParsingTreeIterator< SymType, LabType > &operator++() {
        this->item_num += 1 + this->get_node().descendants_num;
        return *this;
    }

    void advance(unsigned int count = 1) {
        this->item_num += count;
    }

    /*ParsingTreeIterator< SymType, LabType > operator++(int) {
        auto tmp = *this;
        this->operator++();
        return tmp;
    }*/

    bool operator==(const ParsingTreeIterator< SymType, LabType > &node) const {
        return this->item_num == node.item_num;
    }

    bool operator!=(const ParsingTreeIterator< SymType, LabType > &node) const {
        return !this->operator==(node);
    }
};

template< typename SymType, typename LabType >
struct ParsingTreeMultiIterator {
    enum Status {
        Open,
        Close,
        Finished,
    };

    std::vector< std::pair< ParsingTreeIterator< SymType, LabType >, ParsingTreeIterator< SymType, LabType > > > stack;

    ParsingTreeMultiIterator(const ParsingTreeIterator<SymType, LabType> &node) : ParsingTreeMultiIterator(node, node.get_next_siebling()) {
    }

    ParsingTreeMultiIterator(const ParsingTreeIterator< SymType, LabType > &begin, const ParsingTreeIterator< SymType, LabType > &end) : stack({ { begin, end } }) {
    }

    std::pair< Status, ParsingTreeNode< SymType, LabType > > next() {
        assert(!this->stack.empty());
        auto &stack_top = this->stack.back();
        if (stack_top.first == stack_top.second) {
            if (this->stack.size() > 1) {
                this->stack.pop_back();
                return std::make_pair(Close, ParsingTreeNode< SymType, LabType >{});
            } else {
                return std::make_pair(Finished, ParsingTreeNode< SymType, LabType >{});
            }
        } else {
            auto it = stack_top.first;
            ++stack_top.first;
            this->stack.push_back(std::make_pair(it.begin(), it.end()));
            return std::make_pair(Open, it.get_node());
        }
    }
};

/*template< typename SymType, typename LabType >
struct ParsingTreeView {
    const ParsingTreeNodeImpl< SymType, LabType > *nodes;

    //ParsingTreeView(const ParsingTree2< SymType, LabType > &pt) : ParsingTreeView(pt.get_view()) {
    //}

    ParsingTreeNode< SymType, LabType > get_root() const {
        return { *this, 0 };
    }
};*/

template< typename SymType, typename LabType >
struct ParsingTree2 {
    std::vector< ParsingTreeNode< SymType, LabType > > nodes_storage;
    const ParsingTreeNode< SymType, LabType > *nodes;
    size_t nodes_len;

    /*ParsingTreeView< SymType, LabType > get_view() const {
        return { this->tree.nodes.data() };
    }*/

    ParsingTree2() : nodes(NULL), nodes_len(0) {
    }

    ParsingTree2(const ParsingTreeNode< SymType, LabType > *nodes, size_t nodes_len) : nodes(nodes), nodes_len(nodes_len) {
    }

    ParsingTree2(std::vector< ParsingTreeNode< SymType, LabType > > &&nodes_storage, const ParsingTreeNode< SymType, LabType > *nodes, size_t nodes_len) : nodes_storage(std::move(nodes_storage)), nodes(nodes), nodes_len(nodes_len) {
    }

    /*ParsingTree2(const ParsingTree2< SymType, LabType > &pt, size_t item_num) : nodes(pt.get_nodes()), nodes_len(pt.get_nodes_len()) {
    }*/

    const ParsingTreeNode< SymType, LabType > *get_nodes() const {
        if (this->nodes != NULL) {
            return this->nodes;
        } else {
            return this->nodes_storage.data();
        }
    }

    size_t get_nodes_len() const {
        if (this->nodes != NULL) {
            return this->nodes_len;
        } else {
            return this->nodes_storage.size();
        }
    }

    void refresh() {
        if (this->nodes != NULL) {
            this->nodes_storage = std::vector< ParsingTreeNode< SymType, LabType > >(this->nodes, this->nodes + this->nodes_len);
            this->nodes = NULL;
            this->nodes_len = 0;
        }
    }

    ParsingTreeIterator< SymType, LabType > get_root() const {
        return { *this, 0 };
    }

    ParsingTreeMultiIterator< SymType, LabType > get_multi_iterator() const {
        return ParsingTreeMultiIterator< SymType, LabType >(this->get_root());
    }

    bool operator==(const ParsingTree2< SymType, LabType > &other) const {
        return this->get_nodes_len() == other.get_nodes_len() && std::equal(this->get_nodes(), this->get_nodes() + this->get_nodes_len(), other.get_nodes());
    }

    bool operator!=(const ParsingTree2< SymType, LabType > &other) const {
        return !this->operator==(other);
    }

    bool operator<(const ParsingTree2< SymType, LabType > &other) const {
        return std::lexicographical_compare(this->get_nodes(), this->get_nodes() + this->get_nodes_len(),
                                            other.get_nodes(), other.get_nodes() + other.get_nodes_len());
    }

    ParsingTree2< SymType, LabType > &operator=(const ParsingTree2< SymType, LabType > &x) {
        this->nodes_storage = x.nodes_storage;
        this->nodes = x.nodes;
        this->nodes_len = x.nodes_len;
        return *this;
    }
};

namespace boost {
template< typename SymType, typename LabType >
struct hash< ParsingTree2< SymType, LabType > > {
    typedef ParsingTree2< SymType, LabType > argument_type;
    typedef std::size_t result_type;
    result_type operator()(const argument_type &x) const noexcept {
        result_type res = 0;
        if (x.nodes == NULL) {
            boost::hash_combine(res, boost::hash_range(x.nodes_storage.begin(), x.nodes_storage.end()));
        } else {
            boost::hash_combine(res, boost::hash_range(x.nodes, x.nodes + x.nodes_len));
        }
        return res;
    }
};
}

template< typename SymType, typename LabType >
class ParsingTree2Generator {
public:
    void open_node(LabType label, SymType type) {
        this->stack.push_back(pt.nodes_storage.size());
        this->pt.nodes_storage.push_back({ label, type, 0 });
    }

    void close_node() {
        this->pt.nodes_storage[this->stack.back()].descendants_num = this->pt.nodes_storage.size() - this->stack.back() - 1;
        this->stack.pop_back();
    }

    void copy_tree(const ParsingTree2< SymType, LabType > &pt) {
        this->pt.nodes_storage.insert(this->pt.nodes_storage.end(), pt.get_nodes(), pt.get_nodes() + pt.get_nodes_len());
    }

    void reserve(size_t x) {
        this->pt.nodes_storage.reserve(x);
    }

    ParsingTree2< SymType, LabType > &&get_parsing_tree() {
        this->pt.nodes_storage.shrink_to_fit();
        assert(this->stack.empty());
        return std::move(this->pt);
    }

private:
    ParsingTree2< SymType, LabType > pt;
    std::vector< size_t > stack;
};

/*template< typename SymType, typename LabType >
ParsingTree2< SymType, LabType > var_parsing_tree(LabType label, SymType type) {
    ParsingTree2Generator< SymType, LabType > gen;
    gen.open_node(label, type);
    gen.close_node();
    return gen.get_parsing_tree();
}*/

template< typename SymType, typename LabType >
ParsingTree2< SymType, LabType > var_parsing_tree(LabType label, SymType type) {
    //ParsingTree2< SymType, LabType > pt;
    //pt.nodes_storage.push_back({ label, type, 0 });
    //return pt;
    return ParsingTree2< SymType, LabType >{ { { label, type, 0 } }, NULL, 0 };
}

template< typename SymType, typename LabType >
ParsingTree< SymType, LabType > pt2_to_pt(const ParsingTree2< SymType, LabType > &pt2) {
    ParsingTree< SymType, LabType > pt;
    auto &node = pt2.get_root().get_node();
    pt.label = node.label;
    pt.type = node.type;
    for (const auto &child : pt2.get_root()) {
        pt.children.push_back(pt2_to_pt(child.get_view()));
    }
    return pt;
}

/*template< typename SymType, typename LabType >
size_t pt_to_pt2_impl(const ParsingTree< SymType, LabType > &pt, std::vector< ParsingTreeNode< SymType, LabType > > &nodes_storage) {
    size_t idx = nodes_storage.size();
    nodes_storage.push_back({ pt.label, pt.type, 0 });
    size_t descendants_num = 0;
    for (const auto &child : pt.children) {
        descendants_num += pt_to_pt2_impl(child, nodes_storage) + 1;
    }
    nodes_storage[idx].descendants_num = descendants_num;
    return descendants_num;
}

template< typename SymType, typename LabType >
ParsingTree2< SymType, LabType > pt_to_pt2(const ParsingTree< SymType, LabType > &pt) {
    ParsingTree2< SymType, LabType > pt2;
    size_t descendants_num = pt_to_pt2_impl(pt, pt2.nodes_storage);
    pt2.nodes_storage.shrink_to_fit();
    assert(pt2.nodes_storage.size() == descendants_num + 1);
    return pt2;
}*/

template< typename SymType, typename LabType >
void pt_to_pt2_impl(const ParsingTree< SymType, LabType > &pt, ParsingTree2Generator< SymType, LabType > &gen) {
    gen.open_node(pt.label, pt.type);
    for (const auto &child : pt.children) {
        pt_to_pt2_impl(child, gen);
    }
    gen.close_node();
}

template< typename SymType, typename LabType >
ParsingTree2< SymType, LabType > pt_to_pt2(const ParsingTree< SymType, LabType > &pt) {
    ParsingTree2Generator< SymType, LabType > gen;
    pt_to_pt2_impl(pt, gen);
    return gen.get_parsing_tree();
}

template< typename SymType, typename LabType >
class Parser {
public:
    virtual ParsingTree< SymType, LabType > parse(const std::vector<SymType> &sent, SymType type) const {
        return this->parse(sent.begin(), sent.end(), type);
    }
    virtual ParsingTree< SymType, LabType > parse(typename std::vector<SymType>::const_iterator sent_begin, typename std::vector<SymType>::const_iterator sent_end, SymType type) const = 0;
    virtual ~Parser() {}
};

template< typename SymType, typename LabType >
void reconstruct_sentence_internal(const ParsingTree< SymType, LabType > &parsing_tree,
                                                     const std::unordered_map<SymType, std::vector<std::pair<LabType, std::vector<SymType> > > > &derivations,
                                                     const std::unordered_map< LabType, std::pair< SymType, std::vector< SymType > > > &ders_by_lab,
                                                     std::back_insert_iterator< std::vector< SymType > > it) {
    const auto &rule = ders_by_lab.at(parsing_tree.label);
    auto pt_it = parsing_tree.children.begin();
    for (const auto &sym : rule.second) {
        if (derivations.find(sym) == derivations.end()) {
            it = sym;
        } else {
            assert(pt_it != parsing_tree.children.end());
            reconstruct_sentence_internal(*pt_it, derivations, ders_by_lab, it);
            pt_it++;
        }
    }
    assert(pt_it == parsing_tree.children.end());
}

template< typename SymType, typename LabType >
std::vector< SymType > reconstruct_sentence(const ParsingTree< SymType, LabType > &parsing_tree,
                                            const std::unordered_map<SymType, std::vector<std::pair<LabType, std::vector<SymType> > > > &derivations,
                                            const std::unordered_map< LabType, std::pair< SymType, std::vector< SymType > > > &ders_by_lab,
                                            SymType first_sym = {}) {
    std::vector< SymType > res;
    if (first_sym != SymType{}) {
        res.push_back(first_sym);
    }
    reconstruct_sentence_internal(parsing_tree, derivations, ders_by_lab, std::back_inserter(res));
    return res;
}

template< typename SymType, typename LabType >
std::unordered_map< LabType, std::pair< SymType, std::vector< SymType > > > compute_derivations_by_label(const std::unordered_map<SymType, std::vector<std::pair<LabType, std::vector<SymType> > > > &derivations) {
    std::unordered_map< LabType, std::pair< SymType, std::vector< SymType > > > ders;
    for (const auto &der_sym : derivations) {
        for (const auto &der : der_sym.second) {
            ders.insert(make_pair(der.first, make_pair(der_sym.first, der.second)));
        }
    }
    return ders;
}
