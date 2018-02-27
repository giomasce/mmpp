#pragma once

#include <QAbstractItemModel>

#include "mm/proof.h"
#include "mm/toolbox.h"

class ProofTreeModelItem {
    friend class ProofTreeModel;
public:
    ProofTreeModelItem(const ProofTree< Sentence > &pt, ProofTreeModelItem *parent = NULL, size_t num = 0);
    ~ProofTreeModelItem();
private:
    Sentence sentence;
    LabTok label;
    std::vector< ProofTreeModelItem* > children;
    size_t num;  // the index among the parent's children
    ProofTreeModelItem *parent;
};

class ProofTreeModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit ProofTreeModel(const ProofTree< Sentence > &proof_tree, const LibraryToolbox &tb, QObject *parent = NULL);
    ~ProofTreeModel();

    ProofTreeModelItem *get_ptmi(const QModelIndex &index) const;
    QVariant data(const QModelIndex &index, int role) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;

private:
    const LibraryToolbox &tb;
    ProofTreeModelItem *ptmi;
};
