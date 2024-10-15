#include "xubasetransfer.h"
XuBaseTransfer::XuBaseTransfer(QThread *parent)
    : QThread{parent}
{

}

XuBaseTransfer::~XuBaseTransfer()
{
    mCallBackMutex.lock();
    mCallBackHash.clear();
    mCallBackMutex.unlock();
}

bool XuBaseTransfer::attch(void *object, dataCallBackfun callback)
{
    //infof("XuBaseTransfer attch enter");
    mCallBackMutex.lock();
    if (mCallBackHash.contains(object))
    {
        mCallBackMutex.unlock();
        //infof("XuBaseTransfer attch leave");
        return false;
        /* code */
    }
    mCallBackHash.insert(object,callback);
    mCallBackMutex.unlock();
    //infof("XuBaseTransfer attch leave");
    return true;
}

bool XuBaseTransfer::dettch(void *object, dataCallBackfun callback)
{
    //infof("XuBaseTransfer dettch enter");
    mCallBackMutex.lock();
    if (!mCallBackHash.contains(object))
    {
        mCallBackMutex.unlock();
        //infof("XuBaseTransfer dettch leave");
        return false;
        /* code */
    }
    mCallBackHash.remove(object);
    mCallBackMutex.unlock();
    //infof("XuBaseTransfer dettch leave");
    return true;
}

bool XuBaseTransfer::clearCallBack()
{
    mCallBackMutex.lock();
    mCallBackHash.clear();
    mCallBackMutex.unlock();

    return true;
}

bool XuBaseTransfer::callBackData(unsigned char *data, int len)
{
    //debugf("XuBaseTransfer  callBackData enter");
    mCallBackMutex.lock();
    QHash<void *,dataCallBackfun>::const_iterator i;
    for (i = mCallBackHash.constBegin(); i != mCallBackHash.constEnd(); ++i)
    {
        i.value()(data,len);
        //qDebug() << "Key:" << i.key() << "Value:" << i.value();
    }
    mCallBackMutex.unlock();
    //debugf("XuBaseTransfer  callBackData leave");
    return true;
}
