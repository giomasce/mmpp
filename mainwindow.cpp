
#include <QPainter>
#include <QTextDocument>
#include <QAbstractTextDocumentLayout>

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "prooftreemodel.h"

using namespace std;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    ctx(NULL)
{
    ui->setupUi(this);
    this->html_delegate = new HtmlDelegate();
    this->ui->proofThesis->setTextFormat(Qt::RichText);
    this->ui->proofThesis->setDisabled(false);
    this->ui->proofTreeView->setItemDelegate(this->html_delegate);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_actionOpen_triggered()
{
    this->ctx = QSharedPointer<Context>(Context::create_from_filename("../set.mm/set.mm"));
    this->load_proof("avril1");
    this->update();
}

void MainWindow::load_proof(string label)
{
    LabTok tok = this->ctx->lib->get_label(label);
    const Assertion &ass = this->ctx->lib->get_assertion(tok);
    auto executor = ass.get_proof_executor(*this->ctx->lib, true);
    executor->execute();
    const ProofTree &tree = executor->get_proof_tree();
    ProofTreeModel *model = new ProofTreeModel(tree, *this->ctx->tb, this->ui->proofTreeView);
    this->ui->proofThesis->setText(this->ctx->tb->print_sentence(tree.sentence, SentencePrinter::STYLE_ALTHTML).to_string().c_str());
    this->sentence = tree.sentence;
    this->ui->proofTreeView->setModel(model);
    this->update();
}

// Experiement
void MainWindow::paintEvent(QPaintEvent *event) {
    /*if (this->ctx == NULL) {
        return;
    }
    QPainter painter(this);
    QStyle *style = this->style();
    QTextDocument doc;
    doc.setHtml(this->ctx->tb->print_sentence(this->sentence, SentencePrinter::STYLE_ALTHTML).to_string().c_str());
    doc.setTextWidth(-1);
    QSize size = doc.size().toSize();
    QAbstractTextDocumentLayout::PaintContext ctx;
    painter.save();
    QRect rect(QPoint(20, 40), size);
    painter.translate(rect.topLeft());
    painter.setClipRect(rect.translated(-rect.topLeft()));
    doc.documentLayout()->draw(&painter, ctx);
    painter.restore();*/
}

Context::~Context()
{
    delete this->tb;
    delete this->lib;
    delete this->parser;
}

Context *Context::create_from_filename(string filename)
{
    Context *ctx = new Context();
    FileTokenizer ft(filename);
    ctx->parser = new Reader(ft, false, true);
    ctx->parser->run();
    ctx->lib = &ctx->parser->get_library();
    ctx->tb = new LibraryToolbox(*ctx->lib, true);
    return ctx;
}
