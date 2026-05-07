//
// Created by jsz on 4/18/22.
//

#include "include/Camera.h"
#include <opencv2/opencv.hpp>

Camera::Camera() {
    this->status = IMV_OK;
    IMV_HANDLE temp_devHandle = NULL;
    try {
        IMV_DeviceList deviceInfoList;
        this->status = IMV_EnumDevices(&deviceInfoList, interfaceTypeAll);
        if (IMV_OK != this->status) {
            printf("Enumeration devices failed! ErrorCode[%d]\n", this->status);
            getchar();
            return;
        }

        if (deviceInfoList.nDevNum < 1) {
            printf("no camera\n");
            return;
        }
        displayDeviceInfo(deviceInfoList);

        unsigned int cameraIndex = deviceInfoList.nDevNum; // sentinel: not found
        for (unsigned int i = 0; i < deviceInfoList.nDevNum; i++) {
            if (strstr(deviceInfoList.pDevInfo[i].vendorName, "Huaray") != nullptr) {
                cameraIndex = i;
                break;
            }
        }
        if (cameraIndex == deviceInfoList.nDevNum) {
            printf("No HuaRay camera found in device list\n");
            return;
        }

        this->status = IMV_CreateHandle(&temp_devHandle, modeByIndex, (void *) &cameraIndex);
        if (IMV_OK != this->status) {
            throw "Create devHandle failed";
        }
        this->devHandle = temp_devHandle;
    } catch (const char *exp) {
        printf("Create devHandle failed! ErrorCode[%d]\n", this->status);
    }
}

Camera::Camera(std::string serialNumber) {
    this->status = IMV_OK;
    IMV_HANDLE temp_devHandle = NULL;
    try {
        IMV_DeviceList deviceInfoList;
        this->status = IMV_EnumDevices(&deviceInfoList, interfaceTypeAll);
        if (IMV_OK != this->status) {
            printf("Enumeration devices failed! ErrorCode[%d]\n", this->status);
            getchar();
            return;
        }

        if (deviceInfoList.nDevNum < 1) {
            printf("no camera\n");
            return;
        }
        displayDeviceInfo(deviceInfoList);

        unsigned int cameraIndex = deviceInfoList.nDevNum; // sentinel: not found
        for (unsigned int i = 0; i < deviceInfoList.nDevNum; i++) {
            if (strstr(deviceInfoList.pDevInfo[i].serialNumber, serialNumber.c_str()) != nullptr) {
                cameraIndex = i;
                break;
            }
        }
        if (cameraIndex == deviceInfoList.nDevNum) {
            printf("No HuaRay camera found in device list\n");
            return;
        }

        this->status = IMV_CreateHandle(&temp_devHandle, modeByIndex, (void *) &cameraIndex);
        if (IMV_OK != this->status) {
            throw "Create devHandle failed";
        }
        this->devHandle = temp_devHandle;
    } catch (const char *exp) {
        printf("Create devHandle failed! ErrorCode[%d]\n", this->status);
    }
}

int Camera::init(double exposure, bool trigger) {
    // open camera
    this->status = IMV_Open(this->devHandle);

//    IMV_SetEnumFeatureValue(devHandle, "WhiteBalanceAuto", true);
    if (IMV_OK != this->status) {
        printf("Open camera failed! ErrorCode[%d]\n", this->status);
        exit(-1); // if we can't open the camera, kill the program
        return this->status;
    }

    //Enable continuous White Balance

    this->status = IMV_SetEnumFeatureValue(devHandle, "BalanceWhiteAuto", 2);
    
    if (IMV_OK != this->status) {
        printf("White balance failed! ErrorCode[%d]\n", this->status);
        return this->status;
    }

    // Set feature value

    this->status = setProperty(exposure, 1280, 1024);

    // Set hardware trigger

    if (trigger) {
        this->status = setTrigger();
        if (IMV_OK != this->status) {
            printf("Hardware Trigger Failed! ErrorCode[%d]\n", this->status);
            return this->status;
        }
        else {
            printf("pogchamps\n");
        }
    }

    // start grabbing
    this->status = IMV_StartGrabbing(this->devHandle);
    if (IMV_OK != this->status) {
        printf("Open camera failed! ErrorCode[%d]\n", this->status);
        return this->status;
    }
    return this->status;
}

int Camera::getImage(cv::Mat &img) {
    IMV_Frame raw_frame;

    this->status = IMV_GetFrame(this->devHandle, &raw_frame, 30);

    if (IMV_OK != this->status) {
        printf("Get raw_frame failed! ErrorCode[%d]\n", this->status);
        return this->status;
    }

    // Wrap the Bayer data (no copy)
    cv::Mat bayer(raw_frame.frameInfo.height,
              raw_frame.frameInfo.width,
              CV_8U,
              raw_frame.pData); 

    cv::cvtColor(bayer, img, cv::COLOR_BayerRG2RGB_EA);

    this->status = IMV_ReleaseFrame(this->devHandle, &raw_frame);
    if (IMV_OK != this->status) {
        printf("release raw_frame failed! ErrorCode[%d]\n", this->status);
        return this->status;
    }
    return 0;
}

int Camera::setProperty(double exposureTime, int width, int height) {
    this->status = IMV_SetDoubleFeatureValue(this->devHandle, "ExposureTime", exposureTime);
    if (IMV_OK != this->status) {
        printf("Set ExposureTime value failed! ErrorCode[%d]\n", this->status);
        return this->status;
    }

    this->status = IMV_SetIntFeatureValue(devHandle, "Width", width);
    if (IMV_OK != this->status) {
        printf("Set Width value failed! ErrorCode[%d]\n", this->status);
        return this->status;
    }
    this->status = IMV_SetIntFeatureValue(devHandle, "Height", height);
    if (IMV_OK != this->status) {
        printf("Set Height value failed! ErrorCode[%d]\n", this->status);
        return this->status;
    }
    return IMV_OK;
}

int Camera::setExposure(double exposureTime){
    this->status = IMV_SetDoubleFeatureValue(this->devHandle, "ExposureTime", exposureTime);
    if (IMV_OK != this->status) {
        printf("Set ExposureTime value failed! ErrorCode[%d]\n", this->status);
        return this->status;
    }
    return IMV_OK;
}

int Camera::setTrigger() {
    this->status = IMV_SetEnumFeatureValue(this->devHandle, "TriggerSelector", 1); // FrameStart
    if (IMV_OK != this->status) {
        printf("Set Hardware Trigger failed! ErrorCode[%d]\n", this->status);
        return this->status;
    }
    this->status = IMV_SetEnumFeatureValue(this->devHandle, "TriggerMode", true); // On
    if (IMV_OK != this->status) {
        printf("Set Trigger Mode failed! ErrorCode[%d]\n", this->status);
        return this->status;
    }
    this->status = IMV_SetEnumFeatureValue(this->devHandle, "TriggerSource", 1); // Line 1
    if (IMV_OK != this->status) {
        printf("Set Trigger source failed! ErrorCode[%d]\n", this->status);
        return this->status;
    }
    // this->status = IMV_SetEnumFeatureValue(this->devHandle, "TriggerActivation", 0); // RisingEdge
    // if (IMV_OK != this->status) {
    //     printf("Set Trigger Activation failed! ErrorCode[%d]\n", this->status);
    //     return this->status;
    // }
    return IMV_OK;
}

Camera::~Camera() {
    if (this->devHandle != nullptr) {
        IMV_DestroyHandle(this->devHandle);
    }
}
