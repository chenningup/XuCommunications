#include "xuperusbtransfer.h"
#include "xuusbtransfermanager.h"


XuUsbDeviceInfo::XuUsbDeviceInfo():QObject(nullptr),
    mBus(0),
    mAddress(0),
    idVendor(0),
    idProduct(0)
{
}

XuUsbDeviceInfo::~XuUsbDeviceInfo()
{
}

XuUsbDeviceInfo::XuUsbDeviceInfo(const XuUsbDeviceInfo &info)
{
    idVendor = info.idVendor;
    idProduct = info.idProduct;
    mBus = info.mBus;
    mAddress = info.mAddress;
    mPath = info.mPath;
    bInterfaceNumber = info.bInterfaceNumber;
    mEndPointList = info.mEndPointList;
    mOutPointList = info.mOutPointList;
}

XuUsbDeviceInfo & XuUsbDeviceInfo::operator=(const XuUsbDeviceInfo& other)
{
    idVendor = other.idVendor;
    idProduct = other.idProduct;
    mBus = other.mBus;
    mAddress = other.mAddress;
    mPath = other.mPath;
    bInterfaceNumber = other.bInterfaceNumber;
    mEndPointList = other.mEndPointList;
    mOutPointList = other.mOutPointList;
    return *this;
}

bool XuUsbDeviceInfo::operator==(const XuUsbDeviceInfo& other) const
{
    if(other.idProduct == idProduct &&
        other.idVendor == idVendor &&
        other.mPath == mPath &&
        other.mAddress == mAddress &&
        other.mBus == mBus)
    {
        return true;
    }
    return false;
}

XuUsbTransfer::XuUsbTransfer()
    : XuBaseTransfer(nullptr),
    mContext(nullptr),
    mDev(nullptr),
    mHandle(nullptr),
    IsOpen(false),
    findVendor(-1),
    findProduct(-1),
    findBus(-1),
    findAddress(-1),
    findPath(-1)
{
    mTransType = XuUsbTransfer::TransType::Bulk;
}

XuUsbTransfer::~XuUsbTransfer()
{
    close();
    if(mDev)
    {
        libusb_unref_device(mDev);
    }
    clearRecBuf();
}

bool XuUsbTransfer::init()
{
    connect(&XuUsbTransferManager::Instance(),&XuUsbTransferManager::hotplugSignal,this,&XuUsbTransfer::hotplugSlots);
    return true;
}

bool XuUsbTransfer::open()
{
    if(IsOpen)
    {
        //infof("{:s}'s usb opened ",getBelong().toStdString());
        return true;
    }
    //infof("{:s}'s usb transfer try open,mHandle:{}",getBelong().toStdString(),fmt::ptr(mHandle));
	if (!mHandle)
	{
        if(!mDev)
        {
            mDev = XuUsbTransferManager::Instance().getUsbDevice(getfindVendor(),getfindProduct(),getfindBus(),getfindAddress(),getfindPath());
        }
        if(!mDev)
        {
            //errorf("{:s}'s usb getUsbDevice() failed",getBelong().toStdString());
            return false;
        }
        int ret = libusb_open(mDev, &mHandle);
        if (ret < 0)
        {
            //errorf("{:s}'s usb libusb_open() failed: {:s}\n",getBelong().toStdString() ,libusb_error_name(ret));
            return false;
        }
        XuUsbTransferManager::Instance().getInfoFromUsbdevice(mDev,mUsbDeviceInfo);
    }
    if(!setTransfer())
    {
        return false;
    }
    IsOpen = true;
    emit connected();
    //infof("{:s}'s usb transfer open success,mHandle:{}",getBelong().toStdString(),fmt::ptr(mHandle));
    return true;
}

bool XuUsbTransfer::close()
{
    if(mHandle)
    {
        libusb_release_interface(mHandle, mUsbDeviceInfo.getbInterfaceNumber());
        libusb_close(mHandle);
        mHandle = nullptr;
        IsOpen = false;
    }
    else
    {
        //errorf("{:s}'s usb mHandle is empty",getBelong().toStdString());
    }
    if(mDev)
    {
        libusb_unref_device(mDev);
        mDev = nullptr;
    }
    emit disConnect();
    //infof("{:s}'s usb transfer close success ",getBelong().toStdString());
    return true;
}
void callbackRevc(struct libusb_transfer *transfer)
{
    if (transfer->status != LIBUSB_TRANSFER_COMPLETED)
    {
        //errorf("transfer is not completed status {}",transfer->status);
        libusb_free_transfer(transfer);
        return;
    }
    if (transfer->actual_length > 0)
    {
        //for (size_t i = 0; i < transfer->actual_length; i++)
        //{
        //	infof("{:x}", transfer->buffer[i]usb);
        //}
        //std::string data((char *)transfer->buffer, transfer->actual_length);
        //infof("{:x}", data);
        if (transfer->user_data)
        {
            XuUsbTransfer *usb = (XuUsbTransfer *)(transfer->user_data);
            if(usb->isExteraInpoint(transfer->endpoint))
            {
                usb->extraCallBackData(transfer->endpoint,transfer->buffer,transfer->actual_length);
            }
            else
            {
                usb->callBackData(transfer->buffer,transfer->actual_length);
            }
        }
        //QByteArraylibusb_free_transfer s(reinterpret_cast<char*>(transfer->buffer), transfer->actual_length);
        //qObj = reinterpret_cast<QUniversalSerialBus*>(transfer->user_data);
        //emit qObj->revice(s);
    }
    //再次提交传输用于接收
    int rv = libusb_submit_transfer(transfer);
    if (rv < 0)
    {
        //errorf("error libusb_submit_transfer : {} ", libusb_strerror(libusb_error(rv)));
        libusb_cancel_transfer(transfer);
        libusb_free_transfer(transfer);
    }
}
void on_data_send_finish1(struct libusb_transfer *transfer)
{
    printf("send finish\n");
    libusb_cancel_transfer(transfer);
    libusb_free_transfer(transfer);
    //libusb_submit_transfer(transfer);

}
bool XuUsbTransfer::send(unsigned char *data, int len)
{
    if(!mHandle || !IsOpen)
    {
        //errorf("handle is empty or not open");
        return false;
    }
    if (mUseOutEndPointAddressList.isEmpty())
    {
        if(mUsbDeviceInfo.mOutPointList.isEmpty())
        {
            //errorf("mUsbDeviceInfo mOutPointList is empty");
            return false;
        }
        else
        {
            mUseOutEndPointAddressList.push_back(mUsbDeviceInfo.mOutPointList[0]);
        }
    }
    
    int rlen = 0;
    // for (size_t i = 0; i < len; ++i) {
    //     // 打印每个字节，并以十六进制格式显示
    //     std::cout << "0x" << std::hex << static_cast<int>(data[i]) << " ";
    //     if (i > 0 && i % (len-1) == 0) {
    //         std::cout << std::endl; // 每行打印16个字节
    //     }
    // }
    int len1 = LIBUSB_ERROR_OTHER;
    //unsigned char buf[20] = { 0X48,0X57,0X01,0X01,0X64,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00, 0X93,0X0A,0Xff,0Xff,0XCA,0XFE };
    //int len1 = libusb_bulk_transfer(mHandle, bOutEndpointAddress, buf, sizeof(buf), &rlen, 1000);

    switch (mTransType)
    {
    case XuUsbTransfer::TransType::Control:
    {
        uint8_t  bmRequestType,bRequest;
        uint16_t wValue,wIndex;
        libusb_control_transfer(mHandle,bmRequestType ,bRequest,wValue,wIndex,data, len, 500);
    }
    break;
    case XuUsbTransfer::TransType::Bulk:
    {
        len1 = libusb_bulk_transfer(mHandle, mUseOutEndPointAddressList[0], data, len, &rlen, 500);
    }
    break;
    case XuUsbTransfer::TransType::interrupt:
    {
        //libusb_fill_interrupt_transfer(mTransfer,mHandle,mUseOutEndPointAddressList, buf, 1024, &callbackRevc, this, 500);
    }
    break;
    default:
        break;
    }
    if (len1 == LIBUSB_SUCCESS)
    {
        return true;
    }
    else
    {
        //errorf("libusb send error code {}",len1);
    }
    return false;
}

void XuUsbTransfer::setFindTransferParam(int vendor, int product, int bus, int address, int path)
{
    setfindVendor(vendor);
    setfindProduct(product);
    setfindBus(bus);
    setfindAddress(address);
    setfindPath(path);
}

void XuUsbTransfer::attachExtra(int inpoint,void *ptr, dataCallBackfun fun)
{
    mExtraCallBackMutex.lock();
    if(mExtraCallBackHash.contains(inpoint))
    {
        QList<DataCallBackFunClass> &funlist  = mExtraCallBackHash[inpoint];
        if(funlist.contains(DataCallBackFunClass(fun)))
        {
            mExtraCallBackMutex.unlock();
            return;
        }
        else
        {
            funlist.push_back(DataCallBackFunClass(fun));
        }
    }
    else
    {
        QList<DataCallBackFunClass> tmplist;
        tmplist.push_back(fun);
        mExtraCallBackHash.insert(inpoint,tmplist);
    }
    mExtraCallBackMutex.unlock();
}

void XuUsbTransfer::dettchExtra(int inpoint,void *ptr, dataCallBackfun fun)
{
    mExtraCallBackMutex.lock();
    if(mExtraCallBackHash.contains(inpoint))
    {
        QList<DataCallBackFunClass> &funlist  = mExtraCallBackHash[inpoint];
        if(funlist.contains(DataCallBackFunClass(fun)))
        {
            funlist.removeOne(DataCallBackFunClass(fun));
        }
    }
    mExtraCallBackMutex.unlock();
}

void XuUsbTransfer::extraCallBackData(int point,unsigned char *data, int len)
{
    if(!IsOpen)
    {
        return;
    }
    mExtraCallBackMutex.lock();
    QList<DataCallBackFunClass> &funlist = mExtraCallBackHash[point];
    for (size_t i = 0; i < funlist.size(); i++)
    {
        funlist[i].fun(data,len);
    }
    mExtraCallBackMutex.unlock();
}

void XuUsbTransfer::run()
{

}

bool XuUsbTransfer::pushBackInEndPoint(const unsigned int &inpoint)
{
    if(mUseInEndPointAddressList.contains(inpoint))
    {
        return false;
    }{}
    mUseInEndPointAddressList.push_back(inpoint);
    return true;
}

bool XuUsbTransfer::pushBackOutEndPoint(const unsigned int &outpoint)
{
    if(mUseOutEndPointAddressList.contains(outpoint))
    {
        return false;
    }
    mUseOutEndPointAddressList.push_back(outpoint);
    return true;
}

void XuUsbTransfer::resetUsb()
{
    QString usbPath = QString("/dev/bus/usb/00%1/00%2").arg(mUsbDeviceInfo.getmBus()).arg(mUsbDeviceInfo.getmAddress());
    XuUsbTransferManager::Instance().resetUsb(usbPath);
    // if(mHandle)
    // {
    //     libusb_reset_device(mHandle);
    // }
}

bool XuUsbTransfer::isExteraInpoint(int point)
{
    return mExtraCallBackHash.contains(point);
}

void XuUsbTransfer::hotplugSlots(libusb_hotplug_event event, int vendor, int product, int bus, int address, int path)
{
    if (event == LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED)
    {
        if(vendor == getfindVendor() && product == getfindProduct())
        {
            if(getfindBus() != -1)
            {
                if(getfindBus() == bus && getfindAddress() == address && getfindPath() == path)
                {
                    open();
                }
            }
            else
            {
                open();
            }
        }

    }
    else if (event == LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT)
    {
        if(mUsbDeviceInfo.getidVendor() == vendor &&
            mUsbDeviceInfo.getidProduct() == product &&
            mUsbDeviceInfo.getmBus() == bus &&
            mUsbDeviceInfo.getmAddress() == address &&
            mUsbDeviceInfo.getmPath() == path)
        {
            close();
        }
    }
}
// bool XuUsbTransfer::setInOutEndPoint(const unsigned int &inpoint, const unsigned int &outpoint)
// {
//     // if(!mOutEndpointAddressList.contains(outpoint) || !mInEndpointAddressList.contains(inpoint))
//     // {
//     //     return false;
//     // }
//     mUseInEndPointAddress = inpoint;
//     mUseOutEndPointAddress = outpoint;
//     return true;
// }

bool XuUsbTransfer::setTransfer()
{
    //infof("{:s}'s usb setTransfer enter",getBelong().toStdString());
    if(mUseInEndPointAddressList.isEmpty())
    {
        if(mUsbDeviceInfo.mEndPointList.isEmpty())
        {
            //errorf(" mUseInEndPointAddressList and mInEndpointAddressList is empty ");
            return false;
        }
        else
        {
            mUseInEndPointAddressList.push_back(mUsbDeviceInfo.mEndPointList[0]);
        }     
    }
    clearRecBuf();
    int ret = libusb_claim_interface(mHandle, mUsbDeviceInfo.getbInterfaceNumber());
    if (ret < 0)
    {
        ret = libusb_detach_kernel_driver(mHandle, mUsbDeviceInfo.getbInterfaceNumber());
        if (ret < 0)
        {
            //errorf("libusb_detach_kernel_driver error {}",ret);
            return false ;
        }
        ret = libusb_claim_interface(mHandle, mUsbDeviceInfo.getbInterfaceNumber());
        if (ret < 0)
        {
            //errorf("libusb_claim_interface error {}" ,ret);
            return false;
        }
    }
    for (size_t i = 0; i < mUseInEndPointAddressList.size(); i++)
    {
        unsigned char * buf = new unsigned char[1024];
        memset(buf, 0, 1024);
        libusb_transfer *mTransfer = libusb_alloc_transfer(0);
        mTransfer->actual_length = 0;
        
        switch (mTransType)
        {
        case XuUsbTransfer::TransType::Control:
        {
            libusb_fill_control_transfer(mTransfer,mHandle,buf,callbackRevc,this,0);
        }
        break;
        case XuUsbTransfer::TransType::Bulk:
        {
            libusb_fill_bulk_transfer(mTransfer, mHandle, mUseInEndPointAddressList[i], buf, 1024, &callbackRevc, this, 0);
        }
        break;
        case XuUsbTransfer::TransType::interrupt:
        {
            libusb_fill_interrupt_transfer(mTransfer,mHandle, mUseInEndPointAddressList[i], buf, 1024, &callbackRevc, this, 0);
        }
        break;
        default:
            break;
        }
        //libusb_fill_interrupt_transfer(transfer, mInfo.handle, mInfo.bInEndpointAddress, buf, 1024, &callbackRevc, this, 0);
        ret = libusb_submit_transfer(mTransfer);
        if (ret < 0)
        {
            libusb_cancel_transfer(mTransfer);
            libusb_free_transfer(mTransfer);
            //errorf("setTransfer libusb_submit_transfer error");
            mTransfer = nullptr;
            delete[] buf;
        }
        else
        {
            //infof("submit mTransfer {}",fmt::ptr(mTransfer));
            mbufList.push_back(buf);
        }
    }
    return true;
}

void XuUsbTransfer::clearRecBuf()
{
    for (size_t j = 0; j < mbufList.size(); j++)
    {
        if(mbufList[j])
        {
            delete[] mbufList[j];
        }
    }
    mbufList.clear();
}
