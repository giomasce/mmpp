
#include <QPainter>
#include <QTextDocument>
#include <QAbstractTextDocumentLayout>

#include "qt/mainwindow.h"
#include "ui_mainwindow.h"
#include "qt/prooftreemodel.h"

#include "platform.h"
#include "test/test_env.h"

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
    this->ctx = QSharedPointer<Context>(Context::create_from_filename(platform_get_resources_base() / "set.mm", platform_get_resources_base() / "set.mm.cache"));
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
    (void) event;

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
    delete this->te;
}

Context *Context::create_from_filename(const boost::filesystem::path &filename, const boost::filesystem::path &cache_filename)
{
    Context *ctx = new Context();
    ctx->te = new TestEnvironment(filename, cache_filename);
    ctx->lib = &ctx->te->lib;
    ctx->tb = &ctx->te->tb;
    return ctx;
}
