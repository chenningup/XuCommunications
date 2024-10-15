#ifndef XUPERUSBTRANSFER_H
#define XUPERUSBTRANSFER_H

#include <QObject>
#include "../BaseTransfer/xubasetransfer.h"
#include "libusb/libusb.h"
#include "../xudefine.h"

class DataCallBackFunClass:public QObject
{
    Q_OBJECT
public:
    DataCallBackFunClass(const dataCallBackfun &Fun):QObject(nullptr){fun = Fun;};
    DataCallBackFunClass(const DataCallBackFunClass&funclass){fun = funclass.fun;};
    DataCallBackFunClass & operator=(const DataCallBackFunClass&funclass){fun = funclass.fun;return *this;};
    bool operator==(const DataCallBackFunClass&funclass) const
    {
        return *(fun.target<bool(*)(unsigned char *, int)>()) == *(funclass.fun.target<bool(*)(unsigned char *, int)>());
    };
    ~DataCallBackFunClass(){};
    dataCallBackfun fun;
};
class XuUsbDeviceInfo:public QObject
{
    Q_OBJECT
public:
    XuUsbDeviceInfo();
    ~XuUsbDeviceInfo();
    XuUsbDeviceInfo(const XuUsbDeviceInfo&info);
    XuUsbDeviceInfo &operator=(const XuUsbDeviceInfo& other);
    bool operator==(const XuUsbDeviceInfo& other) const;
    friend class XuUsbTransferManager;
private:
    //XuUsbTransfer *mUsbTransfer;
    InperQtPropertyDefine(unsigned int,idVendor,private)
    InperQtPropertyDefine(unsigned int,idProduct,private)
    InperQtPropertyDefine(unsigned int,mBus,private)
    InperQtPropertyDefine(unsigned int,mAddress,private)
    InperQtPropertyDefine(unsigned char,mPath,private)
    InperQtPropertyDefine(unsigned int,bInterfaceNumber,private)

    QList<unsigned int>mEndPointList;
    QList<unsigned int>mOutPointList;
};
class XuUsbTransferManager;
class XuUsbTransfer : public XuBaseTransfer
{
    Q_OBJECT
public:
    explicit XuUsbTransfer();

    enum TransType
    {
        Control,
        Bulk,
        interrupt,
    };
    ~XuUsbTransfer();

    virtual bool init() override;

    virtual bool open() override;

    virtual bool close() override;

    virtual bool send(unsigned char * data , int len) override;

    virtual bool isOpen()override {return IsOpen;};

    void setFindTransferParam(int vendor , int product ,int bus = -1,int address=-1,int path=-1);

    void attachExtra(int inpoint,void *ptr,dataCallBackfun fun);
    void dettchExtra(int inpoint,void *ptr,dataCallBackfun fun);
    void extraCallBackData(int point ,unsigned char *data, int len);

    void run();

    void setContext(libusb_context *text){mContext = text;}
    void setDevice(libusb_device *device){mDev = device;};
    void setTransType(XuUsbTransfer::TransType type){mTransType = type;};

    //bool setInOutEndPoint(const unsigned int &inpoint,const unsigned int &outpoint);

    bool pushBackInEndPoint(const unsigned int &inpoint);
    bool pushBackOutEndPoint(const unsigned int &outpoint);

    void resetUsb();
    bool isExteraInpoint(int point);
signals:

public slots:
    void hotplugSlots(libusb_hotplug_event event,int vendor, int product, int bus,int address,int path);
private:
    bool setTransfer();
    void clearRecBuf();
private:
    libusb_context *mContext;
	libusb_device *mDev;
	libusb_device_handle *mHandle;
    QMutex mTransferMutex;
    QList<unsigned char*> mbufList;
    XuUsbTransfer::TransType mTransType;//传输类型 控制 批量  终端
    QList<unsigned int>mUseInEndPointAddressList;//需要通信的in端点列表
    QList<unsigned int>mUseOutEndPointAddressList;//需要通信的out端点列表
    QMutex mExtraCallBackMutex;
    QHash<int,QList<DataCallBackFunClass>>mExtraCallBackHash; //有些端口的消息是不走通用协议的,可以通过设置这个来过滤处理(通用的走基类的回调)
    bool IsOpen;
    InperQtPropertyDefine(XuUsbDeviceInfo,mUsbDeviceInfo,private)//找到了的usb的一些信息
    InperQtPropertyDefine(QString,Belong,private)

    //以下是寻找libusb device的参数 和mUsbDeviceInfo是不同的
    InperQtPropertyDefine(int,findVendor,private)
    InperQtPropertyDefine(int,findProduct,private)
    InperQtPropertyDefine(int,findBus,private)
    InperQtPropertyDefine(int,findAddress,private)
    InperQtPropertyDefine(int,findPath,private)
};

#endif // XUPERUSBTRANSFER_H
