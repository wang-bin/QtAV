#include <QtAV/AVInput.h>
#include <QtDebug>
#include <QtTest/QTest>
using namespace QtAV;
class tst_AVInput : public QObject
{
    Q_OBJECT
private slots:
    void create();
    void createForProtocol();
    void read();
};

void tst_AVInput::create() {
    AVInput *in = AVInput::create("QFile");
    QVERIFY(in);
    delete in;
    in = AVInput::create("Other");
    QVERIFY(!in);
    delete in;
}
void tst_AVInput::createForProtocol() {
    AVInput *in = AVInput::createForProtocol("qrc");
    QVERIFY(in);
    delete in;
    in = AVInput::createForProtocol("");
    QVERIFY(in);
    QCOMPARE(in->name(), QString("QFile"));
    QVERIFY(in->protocols().contains("qrc"));
    delete in;
    in = AVInput::createForProtocol("xyz");
    QVERIFY(!in);
    delete in;
}
void tst_AVInput::read() {
    const QString path(":/QtAV.svg");
    AVInput *in = AVInput::createForProtocol(path.left(path.indexOf(QChar(':'))));
    QVERIFY(in);
    QVERIFY(in->isSeekable());
    in->setUrl(path);
    QByteArray data(1024, 0);
    in->read(data.data(), data.size());
    QFile f(path);
    f.open(QIODevice::ReadOnly);
    QByteArray data2(1024, 0);
    f.read(data2.data(), data2.size());
    QCOMPARE(data, data2);
    QCOMPARE(in->size(), f.size());
    delete in;
}

QTEST_MAIN(tst_AVInput)
#include "tst_avinput.moc"
