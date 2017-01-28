#ifndef PROOFTREEMODEL_H
#define PROOFTREEMODEL_H

#include <QAbstractItemModel>

#include "proof.h"

class ProofTreeModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit ProofTreeModel(ProofTree &proof_tree, QObject *parent = NULL);
    ~ProofTreeModel();

    QVariant data(const QModelIndex &index, int role) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;

private:
    ProofTree &proof_tree;
};

#endif // PROOFTREEMODEL_H
