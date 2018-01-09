#pragma once

// Implementation from https://en.wikipedia.org/wiki/Disjoint-set_data_structure
template< typename LabType >
class DisjointSet {
public:
    void make_set(LabType lab) {
        // Only insert if the node does not exists yet
        this->parent.insert(std::make_pair(lab, lab));
        this->rank.insert(std::make_pair(lab, 0));
    }

    LabType find_set(LabType lab) {
        if (this->parent[lab] != lab) {
            this->parent[lab] = this->find_set(parent[lab]);
        }
        return parent[lab];
    }

    std::pair< bool, LabType > union_set(LabType l1, LabType l2) {
        l1 = this->find_set(l1);
        l2 = this->find_set(l2);
        if (l1 == l2) {
            return std::make_pair(false, l1);
        }
        if (this->rank[l1] < this->rank[l2]) {
            this->parent[l1] = l2;
            return std::make_pair(true, l2);
        } else if (this->rank[l1] > this->rank[l2]) {
            this->parent[l2] = l1;
            return std::make_pair(true, l1);
        } else {
            this->parent[l2] = l1;
            this->rank[l1]++;
            return std::make_pair(true, l1);
        }
    }

    void clear() {
        this->parent.clear();
        this->rank.clear();
    }

private:
    std::unordered_map< LabType, LabType > parent;
    std::unordered_map< LabType, size_t > rank;
};

// Incremental cycle detector
// State of the art at the moment of writing seems to be https://arxiv.org/pdf/1112.0784.pdf?fname=cm&font=TypeI
// However, for now I implement a naive algorithm
// For the record, most graphs from actual unifications seem to be very sparse, rarely with twice more edges than nodes
template< typename LabType >
class NaiveIncrementalCycleDetector {
public:
    NaiveIncrementalCycleDetector() : edge_num(0) {
    }

    void make_node(LabType node) {
        this->deps.insert(std::pair< LabType, std::set< LabType > >(node, {}));
    }

    void make_edge(LabType from, LabType to) {
        bool res;
        std::tie(std::ignore, res) = this->deps.at(from).insert(to);
        if (res) {
            this->edge_num++;
        }
    }

    size_t get_node_num() const {
        return this->deps.size();
    }

    size_t get_edge_num() const {
        return this->edge_num;
    }

    bool is_acyclic() const {
        return CycleDetectorInternal(this->deps).template run< false >().first;
    }

    std::pair< bool, std::vector< LabType > > find_topo_sort() const {
        return CycleDetectorInternal(this->deps).template run< true >();
    }

    void clear() {
        this->deps.clear();
    }

    enum Status { Unvisited = 0, Visiting, Visited };

    class CycleDetectorInternal {
    public:
        CycleDetectorInternal(const std::unordered_map< LabType, std::set< LabType > > &deps) : deps(deps) {
            topo_sort.reserve(this->deps.size());
        }

        template< bool keep_trace >
        std::pair< bool, std::vector< LabType > > run() {
            for (const auto &x : this->deps) {
                if (!this->run_on< keep_trace >(x.first)) {
                    this->topo_sort.clear();
                    return std::make_pair(false, this->topo_sort);
                }
            }
            return std::make_pair(true, this->topo_sort);
        }

    private:
        template< bool keep_trace >
        bool run_on(LabType lab) {
            auto it = this->deps.find(lab);
            if (it == this->deps.end()) {
                return true;
            }
            if (this->status[lab] == Visited) {
                return true;
            }
            if (this->status[lab] == Visiting) {
                return false;
            }
            this->status[lab] = Visiting;
            for (const auto &new_lab : it->second) {
                bool res = this->run_on< keep_trace >(new_lab);
                if (!res) {
                    return false;
                }
            }
            this->status[lab] = Visited;
            if (keep_trace) {
                this->topo_sort.push_back(lab);
            }
            return true;
        }

        const std::unordered_map< LabType, std::set< LabType > > &deps;
        std::unordered_map< LabType, Status > status;
        std::vector< LabType > topo_sort;
    };

private:
    std::unordered_map< LabType, std::set< LabType > > deps;
    size_t edge_num;
};
