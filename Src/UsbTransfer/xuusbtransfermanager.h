#ifndef XUUSBTRANSFERMANAGER_H
#define XUUSBTRANSFERMANAGER_H

#include <QObject>
#include <QThread>
#include <QTimer>
#include "xuperusbtransfer.h"
#include <QMutex>

//如果libusb 不支持 热拔插的平台(windows) 使用定时扫描usb的方式
//定时扫描不删除 XuUsbDeviceInfo 若扫描不到 设置为下线即可 防止其他地方拿到了mUsbTransfer，掉线后 Manager会删除mUsbTransfer 导致调用空指针报错
class XuUsbTransferManager : public QThread
{
    Q_OBJECT
public:
    explicit XuUsbTransferManager();
    ~XuUsbTransferManager();

    static XuUsbTransferManager& Instance();

    void init();

    void resetUsb(QString &path);

    void run();

    void scanUsbDevice();

    libusb_device *getUsbDevice(int vendor , int product ,int bus = -1,int address=-1,int path=-1);
    // XuUsbTransfer *getUsbTransferByAddress(int vendor , int product ,int address = -1);
    bool getInfoFromUsbdevice(libusb_device *device,XuUsbDeviceInfo &info);
public slots:
    void scanUsbSlot();
signals:
    void hotplugSignal(libusb_hotplug_event event,int vendor, int product, int bus,int address,int path);
private:
private:
    bool isInit;
    int mScanCycle;
    QTimer mScanUsbTimer;//定时用来启动扫描线程
    bool isRelease;
    QList<XuUsbDeviceInfo>mUsbDeviceList;
};







#endif // XUUSBTRANSFERMANAGER_H
