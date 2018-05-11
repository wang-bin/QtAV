namespace QtAV
{

class VideoPreviewWidget : public QWidget
{
%TypeHeaderCode
#include <QtAVWidgets/VideoPreviewWidget.h>
%End

public:
    explicit VideoPreviewWidget(QWidget *parent /TransferThis/ = 0);
    void setTimestamp(qint64 msec);
    qint64 timestamp() const;
    void preview();
    void setFile(const QString& file);
    QString file() const;
    void setKeepAspectRatio(bool value = true);
    bool isKeepAspectRatio() const;
    bool isAutoDisplayFrame() const;
    void setAutoDisplayFrame(bool b=true);
public slots:
    void displayFrame(const QtAV::VideoFrame& frame);
    void displayNoFrame();
signals:
    void timestampChanged();
    void fileChanged();
    void gotError(const QString &);
    void gotAbort(const QString &);
    void gotFrame(const QtAV::VideoFrame & frame);
protected:
    virtual void resizeEvent(QResizeEvent *);
};

};
