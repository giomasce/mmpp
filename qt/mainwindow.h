#pragma once

#include <QMainWindow>

#include "mm/library.h"
#include "mm/toolbox.h"
#include "mm/reader.h"
#include "htmldelegate.h"

struct Context {
    Reader *parser = NULL;
    const ExtendedLibrary *lib = NULL;
    LibraryToolbox *tb = NULL;

    ~Context();

    static Context *create_from_filename(const boost::filesystem::path &filename);
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

protected:
    void paintEvent(QPaintEvent *event);

private slots:
    void on_actionOpen_triggered();

private:
    Ui::MainWindow *ui;
    QSharedPointer<Context> ctx;
    HtmlDelegate *html_delegate;
    Sentence sentence;

    void load_proof(std::string label);
};
