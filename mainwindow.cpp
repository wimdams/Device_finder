#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "QUdpSocket"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    m_udpSocket = new QUdpSocket(this);
    m_udpSocket->bind(QHostAddress::Any, UDP_PORT, QUdpSocket::ShareAddress);

    connect(m_udpSocket, &QUdpSocket::readyRead,
            this, &MainWindow::processPendingDatagrams);

    //Timer just to ask twice (udp)
    m_timer = new QTimer(this);
    m_timer->setSingleShot(true);
    m_timer->setInterval(500);
    connect(m_timer,&QTimer::timeout, this, &MainWindow::SendRequest);

    //Send Request on startup
    on_pbRefresh_clicked();

    ui->devices->setIconSize(QSize(48,48));
    ui->devices->setContextMenuPolicy(Qt::CustomContextMenu);

//    //Example items for screenshot:
//    QListWidgetItem * item = new QListWidgetItem(QIcon(":/res/stm32.png"), "00:80:E1:05:44:21 @ 192.168.0.11 (John Doe)");
//    ui->devices->addItem(item);
//    QListWidgetItem * item2 = new QListWidgetItem(QIcon(":/res/rpi.png"), "E4:5F:01:05:44:21 @ 192.168.0.11 (Jane Doe)");
//    ui->devices->addItem(item2);
//    QListWidgetItem * item3 = new QListWidgetItem(QIcon(":/res/unknown.png"), "44:44:44:05:44:21 @ 192.168.0.11 (Juul Doe)");
//    ui->devices->addItem(item3);
    connect(ui->actionAbout_Qt, &QAction::triggered, QApplication::aboutQt);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pbRefresh_clicked()
{
    SendRequest();
    m_timer->start();
}

void MainWindow::SendRequest()
{
    int broadcasts_send = 0;
    QUdpSocket udpSocket;
    QByteArray datagram = UDP_REQUEST"v1.0";

    //Get all the Network Interface Cards of this Computer
    QList<QNetworkInterface> NICs = QNetworkInterface::allInterfaces();
    foreach(QNetworkInterface NIC, NICs){
        //Get all the adresses on this NIC
        QList<QNetworkAddressEntry> adresses = NIC.addressEntries();
        foreach(QNetworkAddressEntry adress, adresses){
            QD << NIC.name() << adress.ip();
            //If broadcast adress = null => IPv6 (no support for broadcasts)
            QHostAddress broadc_addr = adress.broadcast();
            if(!broadc_addr.isNull()){
                //Send UDP packet
                udpSocket.writeDatagram(datagram, adress.broadcast(), UDP_PORT);
                broadcasts_send++;
            }
        }
    }
    if(broadcasts_send == 0){
        ui->pteLog->appendPlainText("Warning: no Network Interface Cards or valid IP adresses found (no broadcasts send)");
    }else{
        ui->pteLog->appendPlainText(tr("Info: %1 broadcasts send").arg(broadcasts_send));
    }
}

void MainWindow::processPendingDatagrams()
{
    QByteArray datagram;
    QHostAddress host;
    while (m_udpSocket->hasPendingDatagrams()) {
        datagram.resize(int(m_udpSocket->pendingDatagramSize()));
        m_udpSocket->readDatagram(datagram.data(), datagram.size(), &host);
        QString msg = QString(datagram).remove(QRegularExpression("\\r|\\n"));
        processDatagram(msg, host.toString());
    }
}

void MainWindow::processDatagram(QString msg,QString host)
{
    //Append received data to log
    QString logLine = tr("Van %1: \"%2\"")
            .arg(host)
            .arg(msg);
    ui->pteLog->appendPlainText(logLine);

    //We receive our own request. So a check to see if it is the request:
    if(msg.indexOf(UDP_REQUEST) != -1){
        return; //we are done here...
    }

    //RegEx for MAC adress
    QRegularExpression macRegExp("([0-9A-F]{2})[:-]([0-9A-F]{2})[:-]([0-9A-F]{2})[:-]([0-9A-F]{2})[:-]([0-9A-F]{2})[:-]([0-9A-F]{2})");
    QRegularExpressionMatch match = macRegExp.match(msg.toUpper());
    if(match.hasMatch()){
        QString foundOUI = match.captured(1) % match.captured(2) % match.captured(3);
        QString foundMAC = match.captured(0);
        QString imgName = ":/res/unknown.png";
        /*grep STMicro oui.txt | grep base
        0080E1     (base 16)            STMicroelectronics SRL
        10E77A     (base 16)            STMicrolectronics International NV*/
        if(foundOUI == "0080E1" || foundOUI == "10E77A") {
            imgName = ":/res/stm32.png";
        }
        /*grep Raspberry oui.txt | grep base
        DCA632     (base 16)            Raspberry Pi Trading Ltd
        E45F01     (base 16)            Raspberry Pi Trading Ltd
        B827EB     (base 16)            Raspberry Pi Foundation*/
        if(foundOUI == "DCA632" || foundOUI == "E45F01" || foundOUI == "B827EB") {
            imgName = ":/res/rpi.png";
        }
        QString needle = UDP_ANS_OWNER;
        QString owner = "???";
        int index = msg.indexOf(needle);
        if(index != -1){
            owner = msg.mid(index + needle.length());
        }else{
            ui->pteLog->appendPlainText("Warning: no OWNER found in message");
        }

        QListWidgetItem * item = searchItem(foundMAC);
        if(item == nullptr){
            item = new QListWidgetItem();
            ui->devices->addItem(item);
        }
        item->setIcon(QIcon(imgName));
        item->setText(tr("%1 @ %2 (%3)").arg(foundMAC).arg(host).arg(owner));
        item->setToolTip(tr("Last seen: %1").arg(QTime::currentTime().toString("H:mm:ss.zzz")));
    }else{
        ui->pteLog->appendPlainText("Warning: no MAC found in message");
    }
}

QListWidgetItem *MainWindow::searchItem(QString MAC)
{
    QListWidgetItem * item = nullptr;
    for(int i = 0; i < ui->devices->count(); i++){
        item = ui->devices->item(i);
        if(item->text().contains(MAC)){
            break;
        }
        item = nullptr;
    }
    return item;
}

void MainWindow::on_devices_itemDoubleClicked(QListWidgetItem *item)
{
    //Copy the IP to the clipboard
    QString itemText = item->text();
    int start = itemText.indexOf('@')+2;
    int end = itemText.indexOf('(')-1;
    QString ip = itemText.mid(start,end - start);
    QClipboard *clipboard = QGuiApplication::clipboard();
    clipboard->setText(ip);
}

void MainWindow::on_devices_customContextMenuRequested(const QPoint &pos)
{
    //create menu
    QAction copyAction(QIcon(":/res/copy-icon.png"),"Copy IP");
    QMenu menu;
    menu.addAction(&copyAction);

    //execute menu
    QAction * action = menu.exec(ui->devices->mapToGlobal(pos));
    if(action == &copyAction){
        QListWidgetItem *item = ui->devices->itemAt(pos);
        if(item){
            on_devices_itemDoubleClicked(item);
        }
    }
}

void MainWindow::on_actionAbout_triggered()
{
    About ab(this);
    ab.exec();
}
