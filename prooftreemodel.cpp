#include "prooftreemodel.h"

ProofTreeModel::ProofTreeModel(ProofTree &proof_tree, QObject *parent) :
    QAbstractItemModel(parent),
    proof_tree(proof_tree)
{
}

ProofTreeModel::~ProofTreeModel()
{
}

QVariant ProofTreeModel::data(const QModelIndex &index, int role) const
{

}

QModelIndex ProofTreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!this->hasIndex(row, column, parent)) {
        return QModelIndex();
    }
    ProofTree *pt;
    if (parent.isValid()) {
        pt = static_cast< ProofTree* >(parent.internalPointer());
    } else {
        pt = &this->proof_tree;
    }
    ProofTree *child_pt = pt->children[row];
    return createIndex(row, column, child_pt);
}

QModelIndex ProofTreeModel::parent(const QModelIndex &child) const
{
    if (!child.isValid()) {
        return QModelIndex();
    }
    ProofTree *child_pt = static_cast< ProofTree* >(child.internalPointer());
    // TODO
}

int ProofTreeModel::rowCount(const QModelIndex &parent) const
{

}

int ProofTreeModel::columnCount(const QModelIndex &parent) const
{

}
