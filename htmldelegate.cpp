#include "htmldelegate.h"

#include <QApplication>
#include <QTextDocument>
#include <QAbstractTextDocumentLayout>
#include <QPainter>

using namespace std;

void HtmlDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option_, const QModelIndex &index) const
{
    QStyleOptionViewItem option = option_;
    initStyleOption(&option, index);

    QStyle *style = option.widget ? option.widget->style() : QApplication::style();

    QTextDocument doc;
    doc.setHtml(option.text);

    // Painting item without text
    option.text = QString();
    style->drawControl(QStyle::CE_ItemViewItem, &option, painter);

    QAbstractTextDocumentLayout::PaintContext ctx;

    // Highlighting text if item is selected
    if (option.state & QStyle::State_Selected) {
        ctx.palette.setColor(QPalette::Text, option.palette.color(QPalette::Active, QPalette::HighlightedText));
    }

    QRect textRect = style->subElementRect(QStyle::SE_ItemViewItemText, &option);
    painter->save();
    painter->translate(textRect.topLeft());
    painter->setClipRect(textRect.translated(-textRect.topLeft()));
    doc.documentLayout()->draw(painter, ctx);
    painter->restore();
}

QSize HtmlDelegate::sizeHint(const QStyleOptionViewItem &option_, const QModelIndex &index) const
{
    QStyleOptionViewItem option = option_;
    initStyleOption(&option, index);

    QTextDocument doc;
    doc.setHtml(option.text);
    doc.setTextWidth(option.rect.width());
    return QSize(doc.idealWidth(), doc.size().height());
}
