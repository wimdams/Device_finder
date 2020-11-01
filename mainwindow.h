#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtNetwork>
#include <QtWidgets>
#define QD qDebug() << __FILE__ << __LINE__

#include "about.h"

// UDP protocol:
//REQ= Where are you?v1.0
//ANS= XX:XX:XX:XX:XX:XX is present and my owner is YYYYYY ZZZZZZ
#define UDP_PORT 64000
#define UDP_REQUEST "Where are you?"
#define UDP_ANS_OWNER "my owner is "

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void SendRequest();
    void processPendingDatagrams();

    void on_pbRefresh_clicked();
    void on_devices_itemDoubleClicked(QListWidgetItem *item);
    void on_devices_customContextMenuRequested(const QPoint &pos);

    void on_actionAbout_triggered();

private:
    Ui::MainWindow *ui;
    QTimer * m_timer;
    QUdpSocket *m_udpSocket = nullptr;
    void processDatagram(QString msg, QString host);
    QListWidgetItem *searchItem(QString MAC);
};
#endif // MAINWINDOW_H
