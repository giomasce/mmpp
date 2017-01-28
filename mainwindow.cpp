#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "parser.h"
#include "prooftreemodel.h"

using namespace std;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    ctx(NULL)
{
    ui->setupUi(this);
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
    LabTok tok = this->ctx->lib.get_label(label);
    const Assertion &ass = this->ctx->lib.get_assertion(tok);
    auto executor = ass.get_proof_executor(this->ctx->lib, true);
    executor->execute();
    ProofTree *tree = executor->get_proof_tree();
    ProofTreeModel *model = new ProofTreeModel(*tree, this->ui->proofTreeView);
    this->ui->proofTreeView->setModel(model);
}

Context *Context::create_from_filename(string filename)
{
    FileTokenizer ft(filename);
    Parser p(ft, false, true);
    p.run();
    LibraryImpl lib = p.get_library();
    LibraryToolbox tb(lib, true);
    return new Context({lib, tb});
}
