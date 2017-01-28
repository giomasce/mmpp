#include "prooftreemodel.h"

ProofTreeModel::ProofTreeModel(const ProofTree &proof_tree, const LibraryToolbox &tb, QObject *parent) :
    QAbstractItemModel(parent),
    tb(tb),
    ptmi(new ProofTreeModelItem(proof_tree))
{
}

ProofTreeModel::~ProofTreeModel()
{
    delete this->ptmi;
}

ProofTreeModelItem *ProofTreeModel::get_ptmi(const QModelIndex &index) const
{
    if (index.isValid()) {
        return static_cast< ProofTreeModelItem* >(index.internalPointer());
    } else {
        return this->ptmi;
    }
}

QVariant ProofTreeModel::data(const QModelIndex &index, int role) const
{
    if (role != Qt::DisplayRole) {
        return QVariant();
    }
    ProofTreeModelItem *ptmi = this->get_ptmi(index);
    //return QVariant::fromValue((void*) ptmi);
    //return QString(this->tb.print_sentence(ptmi->sentence).to_string().c_str());
    return QString(this->tb.print_sentence(ptmi->sentence, SentencePrinter::STYLE_ALTHTML).to_string().c_str());
}

QModelIndex ProofTreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!this->hasIndex(row, column, parent)) {
        return QModelIndex();
    }
    ProofTreeModelItem *ptmi = get_ptmi(parent);
    ProofTreeModelItem *child_ptmi = ptmi->children[row];
    return createIndex(row, column, child_ptmi);
}

QModelIndex ProofTreeModel::parent(const QModelIndex &child) const
{
    if (!child.isValid()) {
        return QModelIndex();
    }
    ProofTreeModelItem *child_ptmi = get_ptmi(child);
    ProofTreeModelItem *ptmi = child_ptmi->parent;
    if (ptmi == this->ptmi) {
        return QModelIndex();
    } else {
        return createIndex(ptmi->num, 0, ptmi);
    }
}

int ProofTreeModel::rowCount(const QModelIndex &parent) const
{
    if (parent.column() > 0) {
        return 0;
    }
    ProofTreeModelItem *ptmi = get_ptmi(parent);
    return ptmi->children.size();
}

int ProofTreeModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}

ProofTreeModelItem::ProofTreeModelItem(const ProofTree &pt, ProofTreeModelItem *parent, size_t num) :
    sentence(pt.sentence), label(pt.label), num(num), parent(parent)
{
    size_t child_num = 0;
    for (const auto &child : pt.children) {
        this->children.push_back(new ProofTreeModelItem(child, this, child_num++));
    }
}

ProofTreeModelItem::~ProofTreeModelItem()
{
    for (auto &ptr : this->children) {
        delete ptr;
    }
}
