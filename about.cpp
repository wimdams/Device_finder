#include "about.h"
#include "ui_about.h"

About::About(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::About)
{
    ui->setupUi(this);
    ui->lblVersion->setText(ui->lblVersion->text() + " " + __DATE__ + " " + __TIME__);
}

About::~About()
{
    delete ui;
}
