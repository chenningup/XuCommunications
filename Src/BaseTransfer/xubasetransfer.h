#ifndef XUBASETRANSFER_H
#define XUBASETRANSFER_H

#include <QObject>
#include <QThread>
#include <QHash>
#include <functional>
#include <QMutex>
//和下位机的通信基类
typedef std::function<bool(unsigned char *, int)>dataCallBackfun;
class XuBaseTransfer : public QThread
{
    Q_OBJECT
public:
    explicit XuBaseTransfer(QThread *parent = nullptr);

    virtual ~XuBaseTransfer();

    virtual bool init()=0;

    virtual bool open()=0;

    virtual bool close()=0;

    virtual bool send(unsigned char * data , int len) = 0;

    virtual bool isOpen()=0;

    bool attch(void *object,dataCallBackfun callback);

    bool dettch(void *object,dataCallBackfun callback);

    bool clearCallBack();

    bool callBackData(unsigned char *data, int len);
signals:
    void disConnect();
    void connected();
private:
    QMutex mCallBackMutex;
    QHash<void *,dataCallBackfun>mCallBackHash;
};

#endif // XUBASETRANSFER_H
