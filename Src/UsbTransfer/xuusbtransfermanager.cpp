#include "xuusbtransfermanager.h"
#include "libusb/libusb.h"
#ifndef Q_OS_WIN
#include <sys/ioctl.h>
#include <linux/usbdevice_fs.h>
#include <fcntl.h>
#endif
libusb_context *usbContext = nullptr;
libusb_hotplug_callback_handle callback_handle;
static int LIBUSB_CALL hotplug_callback(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data)
{
    struct libusb_device_descriptor desc;
    libusb_get_device_descriptor(dev, &desc);

    if (event == LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED)
    {
        //infof("Device inserted: Vendor ID = 0X{:X}, Product ID = 0X{:X} ", desc.idVendor, desc.idProduct);
    }
    else if (event == LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT)
    {
       // infof("Device removed: Vendor ID = 0X{:X}, Product ID = 0X{:X}", desc.idVendor, desc.idProduct);
    }
    // int vendor, int product, int bus,int address,int path
    //
    XuUsbDeviceInfo info;
    if(XuUsbTransferManager::Instance().getInfoFromUsbdevice(dev,info))
    {
        emit XuUsbTransferManager::Instance().hotplugSignal(event,info.getidVendor(),info.getidProduct(),info.getmBus(),info.getmAddress(),info.getmPath());
    }
    return 0;
}



XuUsbTransferManager::XuUsbTransferManager()
    : QThread(nullptr),isInit(false),mScanCycle(5),isRelease(false)
{
    connect(&mScanUsbTimer,&QTimer::timeout,this,&XuUsbTransferManager::scanUsbSlot);
}

XuUsbTransferManager::~XuUsbTransferManager()
{
    if(isInit)
    {
        //infof("libusb_exit");
        libusb_exit(usbContext);
    }
    isRelease = true;
}

XuUsbTransferManager &XuUsbTransferManager::Instance()
{
    static XuUsbTransferManager mInperUsbTransferManager;
    return mInperUsbTransferManager;
}
void XuUsbTransferManager::init()
{
    if(isInit)
    {
        return;
    }
    // mUsbDeviceList.clear();
    //InperLog::Instance()->init();
    int ret = libusb_init(&usbContext);
    if (ret != 0)
    {
        //errorf("libusb init error");
        return ;
    }
    libusb_set_debug(usbContext,LIBUSB_LOG_LEVEL_DEBUG);
    isInit = true;
    start();
    if(!libusb_has_capability(LIBUSB_CAP_HAS_HOTPLUG))//如果不支持hotplugSignal
    {
        libusb_device **devices;
        ssize_t count = libusb_get_device_list(usbContext, &devices);
        if (count > 0)
        {
            for (ssize_t a = 0; a < count; ++a)
            {
                XuUsbDeviceInfo usbinfo;
                libusb_device *device = devices[a];
                if(getInfoFromUsbdevice(device,usbinfo))
                {
                    mUsbDeviceList.push_back(usbinfo);
                }
            }
            libusb_free_device_list(devices, 1);
        }
        mScanUsbTimer.start(mScanCycle * 1000);
    }
    else
    {
        libusb_hotplug_register_callback(usbContext,
                                         LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT,
                                         0, // filter
                                         LIBUSB_HOTPLUG_MATCH_ANY,
                                         LIBUSB_HOTPLUG_MATCH_ANY,
                                         LIBUSB_HOTPLUG_MATCH_ANY,
                                         hotplug_callback,
                                         NULL, // user data
                                         &callback_handle);
    }

}

void XuUsbTransferManager::resetUsb(QString &path)
{
#ifndef Q_OS_WIN
    //QString usbPath = QString("/dev/bus/usb/00%1/00%2").arg(mUsbDeviceInfo.getmBus()).arg(mUsbDeviceInfo.getmAddress());
    int fd = open(path.toStdString().c_str(), O_RDWR);
    if (fd < 0)
    {
        //errorf("Failed to open USB device");
        return;
    }

    // 执行重置操作
    int ret = ioctl(fd, USBDEVFS_RESET, 0);
    if (ret < 0)
    {
        //errorf("Failed to reset USB device");
    }
    else
    {
        //infof("USB device {:s} reset successfully",path.toStdString());
    }
#endif
}

void XuUsbTransferManager::run()
{
    if(isInit)
    {
        if(usbContext)
        {
            while (!isRelease)
            {
                libusb_handle_events(usbContext);
            }
        }
    }
}

void XuUsbTransferManager::scanUsbDevice()
{
    libusb_device **devices;
    ssize_t count = libusb_get_device_list(usbContext, &devices);
    if (count < 0)
    {
       // errorf("get device list error");
        return;
    }
    for (ssize_t a = 0; a < count; ++a)
    {
        XuUsbDeviceInfo usbinfo;
        libusb_device *device = devices[a];
        QList<XuUsbDeviceInfo>tmpDeviceList = mUsbDeviceList;
        bool findsuccess = true;
        if(getInfoFromUsbdevice(device,usbinfo))
        {
            if(tmpDeviceList.contains(usbinfo))
            {
                tmpDeviceList.removeOne(usbinfo);
            }
            else
            {
                emit XuUsbTransferManager::Instance().hotplugSignal(LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED,usbinfo.getidVendor(),usbinfo.getidProduct(),usbinfo.getmBus(),usbinfo.getmBus(),usbinfo.getmPath());
                mUsbDeviceList.push_back(usbinfo);
            }
        }
        else
        {
            findsuccess = false;
        }
        if(findsuccess)
        {
            for (auto var : tmpDeviceList)
            {
                emit XuUsbTransferManager::Instance().hotplugSignal(LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT,var.getidVendor(),var.getidProduct(),var.getmBus(),var.getmBus(),var.getmPath());
                mUsbDeviceList.removeOne(var);
            }
        }
    }
    libusb_free_device_list(devices, 1);
}

libusb_device *XuUsbTransferManager::getUsbDevice(int vendor, int product, int bus,int address,int path)
{
    libusb_device * finddevice = nullptr;
    libusb_device **devices;
    // 获取已连接的设备列表
    ssize_t count = libusb_get_device_list(usbContext, &devices);
    bool isfound = false;
    for (ssize_t a = 0; a < count; ++a)
    {
        libusb_device *device = devices[a];
        if(isfound)
        {
            libusb_unref_device(device);
            continue;
        }
        struct libusb_device_descriptor desc;
        int result = libusb_get_device_descriptor(device, &desc);
        if (result != LIBUSB_SUCCESS)
        {
            // 错误处理
            //errorf("libusb_get_device_descriptor error");
            libusb_unref_device(device);
            continue;
        }
        int tmpbus = libusb_get_bus_number(device);
        uint8_t _address=libusb_get_device_address(device);
        uint8_t pathlist[8] = { 0 };
        int ret = libusb_get_port_numbers(device, pathlist, sizeof(pathlist));
        if(bus == -1)
        {
            if(vendor == desc.idVendor && desc.idProduct)
            {
                isfound = true;
                finddevice = device;
            }
        }
        else
        {
            if(vendor == desc.idVendor && desc.idProduct && tmpbus == bus)
            {
                if(address!=-1 && path!=-1){
                    bool isPath=false;
                    for(int i=1;i<ret;i++){
                        if(pathlist[i]==path){
                            isPath=true;
                        }
                    }
                    if(_address==address && isPath){
                        isfound = true;
                        finddevice = device;
                    }
                }else {
                    isfound = true;
                    finddevice = device;
                }
            }
        }
    }
    libusb_free_device_list(devices, 0);

    if(!finddevice)
    {
        //errorf("get deivce error vendor {} product {} bus {}",vendor , product , bus);
        return nullptr;
    }
    return finddevice;
    // XuUsbTransfer * usb = new XuUsbTransfer();
    // usb->setmUsbDeviceInfo(info);
    // usb->setDevice(finddevice);
    // return usb;
}

bool XuUsbTransferManager::getInfoFromUsbdevice(libusb_device *device,XuUsbDeviceInfo &info)
{
    if(!device)
    {
        return false;
    }
    struct libusb_device_descriptor desc;
    int result = libusb_get_device_descriptor(device, &desc);
    if (result != LIBUSB_SUCCESS)
    {
        //errorf("libusb_get_device_descriptor error");
        return false;
    }
    //memcpy(&info.mDescriptor,&desc,sizeof(XuUsbDeviceInfo));
    info.idVendor = desc.idVendor;
    info.idProduct = desc.idProduct;
    //infof("{}:{} (bus: {}, device: {}) ", desc.idVendor, desc.idProduct, libusb_get_bus_number(device), libusb_get_device_address(device));
    info.mBus = libusb_get_bus_number(device);
    info.mAddress = libusb_get_device_address(device);

    //idlist.append(UsbDeviceId(desc.idProduct,desc.idVendor));
    //debugf("vid:{:X},PID:{:X}.Bus:{},Address:{}", desc.idVendor, desc.idProduct,info.mBus,info.mAddress);
    // finfof(stdout, "%04x:%04x (bus: %d, device: %d) ",
    //     desc.idVendor, desc.idProduct, libusb_get_bus_number(device), libusb_get_device_address(device));
    uint8_t path[8] = { 0 };
    int ret = libusb_get_port_numbers(device, path, sizeof(path));
    if (ret > 0)
    {
        //infof("path: {}", path[0]);
        info.mPath = path[ret-1];
        //for (int j = 1; j < ret; ++j)
        //infof(".{}", path[j]);
    }
    int interface_number = 0;
    struct libusb_config_descriptor *conf_desc;
    ret = libusb_get_config_descriptor(device, 0, &conf_desc);
    if (ret != LIBUSB_SUCCESS)
    {
        //errorf("libusb_get_config_descriptor error {}",ret);
        return false;
    }
    int iface, nb_ifaces= -1;
    int i ,j, k, r;
    nb_ifaces = conf_desc->bNumInterfaces;
    uint8_t endpoint_in = 0, endpoint_out = 0;
    const struct libusb_endpoint_descriptor *endpoint;

    //infof("             nb interfaces:{}", nb_ifaces);
    for (i = 0; i < nb_ifaces; i++)
    {
        //infof("interface{}: id = {}\n", i, conf_desc->interface[i].altsetting[0].bInterfaceNumber);
        for (j = 0; j < conf_desc->interface[i].num_altsetting; j++)
        {
            //infof("interface{}.altsetting{}: num endpoints = {}",
            //  i, j, conf_desc->interface[i].altsetting[bInterfaceNumberj].bNumEndpoints);
            info.bInterfaceNumber = conf_desc->interface[i].altsetting[0].bInterfaceNumber;
            int res = 0;
            for (k = 0; k < conf_desc->interface[i].altsetting[j].bNumEndpoints; k++)
            {
                struct libusb_ss_endpoint_companion_descriptor *ep_comp = NULL;
                endpoint = &conf_desc->interface[i].altsetting[j].endpoint[k];
                //debugf("endpointaddress:{:X}", endpoint->bEndpointAddress);
                if ((endpoint->bmAttributes & LIBUSB_TRANSFER_TYPE_MASK) & (LIBUSB_TRANSFER_TYPE_BULK | LIBUSB_TRANSFER_TYPE_INTERRUPT))
                {
                    if (endpoint->bEndpointAddress & LIBUSB_ENDPOINT_IN)
                    {
                        if (!endpoint_in)
                        {
                            endpoint_in = endpoint->bEndpointAddress;
                            info.mEndPointList.push_back(endpoint_in);
                            res |= 1;
                        }

                    }
                    else
                    {
                        if (!endpoint_out)
                        {
                            endpoint_out = endpoint->bEndpointAddress;
                            info.mOutPointList.push_back(endpoint->bEndpointAddress);
                            res |= 2;
                        }
                    }
                }
            }
            if (res == 3)
            {

            }
        }
    }
    libusb_free_config_descriptor(conf_desc);
    return true;
}

// XuUsbTransfer *XuUsbTransferManager::getUsbTransferByAddress(int vendor, int product, int address)
// {
//     XuUsbDeviceInfo info;
//     if(!findUsbDeviceInfoByAddress(vendor,product,address,info))
//     {
//         errorf("cant find device info vendor {} product {} bus {}",vendor , product , address);
//         return nullptr;
//     }
//     libusb_device** devs;
//     ssize_t cnt;
//     libusb_device_handle * handle = nullptr;
//     cnt=libusb_get_device_list(usbContext,&devs);
//     if(cnt<0){
//         return nullptr;
//     }
//     for(ssize_t i=0;i<cnt;++i){
//         libusb_device* dev=devs[i];
//         libusb_device_descriptor desc;
//         int r=libusb_get_device_descriptor(dev,&desc);
//         if(r<0){
//             continue;
//         }

//         if(desc.idVendor==vendor && desc.idProduct==product){
//             uint8_t _address=libusb_get_device_address(dev);
//             if(_address==address){
//                 int ret = libusb_open(dev, &handle);
//                 if (ret != LIBUSB_SUCCESS)
//                 {
//                     errorf("libusb_opesetbInterfaceNumbern error {}",ret);
//                 }
//                 break;
//             }
//         }
//     }
//     libusb_free_device_list(devs,1);
//     XuUsbTransfer * usb = new XuUsbTransfer();
//     usb->setmUsbDeviceInfo(info);
//     usb->setContext(usbContext);
//     usb->setDeviceHandle(handle);
//     return usb;
// }

void XuUsbTransferManager::scanUsbSlot()
{
    scanUsbDevice();
}
