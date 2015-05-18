#include <QtAV/MediaIO.h>
#include <QtDebug>
#include <QtTest/QTest>
using namespace QtAV;
class tst_MediaIO : public QObject
{
    Q_OBJECT
private slots:
    void create();
    void createForProtocol();
    void read();
};

void tst_MediaIO::create() {
    MediaIO *in = MediaIO::create("QFile");
    QVERIFY(in);
    delete in;
    in = MediaIO::create("Other");
    QVERIFY(!in);
    delete in;
}
void tst_MediaIO::createForProtocol() {
    MediaIO *in = MediaIO::createForProtocol("qrc");
    QVERIFY(in);
    delete in;
    in = MediaIO::createForProtocol("");
    QVERIFY(in);
    QCOMPARE(in->name(), QString("QFile"));
    QVERIFY(in->protocols().contains("qrc"));
    delete in;
    in = MediaIO::createForProtocol("xyz");
    QVERIFY(!in);
    delete in;
}
void tst_MediaIO::read() {
    const QString path(":/QtAV.svg");
    MediaIO *in = MediaIO::createForProtocol(path.left(path.indexOf(QChar(':'))));
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

QTEST_MAIN(tst_MediaIO)
#include "tst_avinput.moc"
