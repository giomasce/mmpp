#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "library.h"
#include "toolbox.h"
#include "parser.h"
#include "htmldelegate.h"

struct Context {
    Parser *parser = NULL;
    const Library *lib = NULL;
    LibraryToolbox *tb = NULL;

    ~Context();

    static Context *create_from_filename(std::string filename);
};

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_actionOpen_triggered();

private:
    Ui::MainWindow *ui;
    QSharedPointer<Context> ctx;
    HtmlDelegate *html_delegate;

    void load_proof(std::string label);
};

#endif // MAINWINDOW_H
