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
    this->ui->proofTreeView->setItemDelegate(this->html_delegate);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_actionOpen_triggered()
{
    this->ctx = QSharedPointer<Context>(Context::create_from_filename("../set.mm/set.mm"));
    this->load_proof("syl");
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
    this->ui->proofTreeView->setModel(model);
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
    ctx->parser = new Parser(ft, false, true);
    ctx->parser->run();
    ctx->lib = &ctx->parser->get_library();
    ctx->tb = new LibraryToolbox(*ctx->lib, true);
    return ctx;
}
