//
// The MIT License (MIT)
//
// Copyright (c) 2019 Livox. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

/** Livox LiDAR data source, data from dependent lidar */

#ifndef LDS_LIDAR_H_
#define LDS_LIDAR_H_

#include <memory>
#include <vector>

#include "livox_def.h"
#include "livox_sdk.h"

//=======================================================================================

typedef enum
{
    kConnectStateOff = 0,
    kConnectStateOn = 1,
    kConnectStateConfig = 2,
    kConnectStateSampling = 3,
} LidarConnectState;

typedef enum
{
    kConfigFan = 1,
    kConfigReturnMode = 2,
    kConfigCoordinate = 4,
    kConfigImuRate = 8
} LidarConfigCodeBit;

typedef enum
{
    kCoordinateCartesian = 0,
    kCoordinateSpherical
} CoordinateType;

typedef struct
{
    bool enable_fan;
    uint32_t return_mode;
    uint32_t coordinate;
    uint32_t imu_rate;
    volatile uint32_t set_bits;
    volatile uint32_t get_bits;
} UserConfig;

typedef struct
{
    uint8_t handle;
    LidarConnectState connect_state;
    DeviceInfo info;
    UserConfig config;
} LidarDevice;

//=======================================================================================

/**
 * LiDAR data source, data from dependent lidar.
 */
class LdsLidar
{
public:

    static LdsLidar& GetInstance()
    {
      static LdsLidar lds_lidar;
      return lds_lidar;
    }

    //-----------------------------------------------------------------------------------

    int broadcast( std::vector<std::string>& broadcast_code_strs );
    int DeInitLdsLidar();

    //-----------------------------------------------------------------------------------

private:

    bool _auto_connect_mode;
    uint32_t _whitelist_count;
    volatile bool _is_initialized;
    char _broadcast_code_whitelist[kMaxLidarCount][kBroadcastCodeSize];

    uint32_t _lidar_count;
    LidarDevice _lidars[kMaxLidarCount];

    uint32_t _data_recveive_count[kMaxLidarCount];

    //-----------------------------------------------------------------------------------

    LdsLidar();
    ~LdsLidar();

    LdsLidar( const LdsLidar& ) = delete;
    LdsLidar& operator=( const LdsLidar& ) = delete;

    //-----------------------------------------------------------------------------------

    static void GetLidarDataCb( const uint8_t handle,
                                LivoxEthPacket* data,
                                const uint32_t data_num,
                                void* client_data );

    static void OnDeviceBroadcast( const BroadcastDeviceInfo* info );

    static void OnDeviceChange( const DeviceInfo* info, const DeviceEvent type );

    static void StartSampleCb( const livox_status status,
                               const uint8_t handle,
                               const uint8_t response,
                               void* clent_data );

    static void StopSampleCb( const livox_status status,
                              const uint8_t handle,
                              const uint8_t response,
                              void* clent_data );

    static void DeviceInformationCb( const livox_status status,
                                     const uint8_t handle,
                                     DeviceInformationResponse* ack,
                                     void* clent_data );

    static void LidarErrorStatusCb( const livox_status status,
                                    const uint8_t handle,
                                    ErrorMessage* message );

    static void ControlFanCb( const livox_status status,
                              const uint8_t handle,
                              const uint8_t response,
                              void* clent_data );

    static void SetPointCloudReturnModeCb( const livox_status status,
                                           const uint8_t handle,
                                           const uint8_t response,
                                           void* clent_data );

    static void SetCoordinateCb( const livox_status status,
                                 const uint8_t handle,
                                 const uint8_t response,
                                 void* clent_data );

    static void SetImuRatePushFrequencyCb( const livox_status status,
                                           const uint8_t handle,
                                           const uint8_t response,
                                           void* clent_data );

    //-----------------------------------------------------------------------------------

    int AddBroadcastCodeToWhitelist( const char* broadcast_code );

    void AddLocalBroadcastCode();

    bool FindInWhitelist( const char* broadcast_code );

    void EnableAutoConnectMode();

    void DisableAutoConnectMode();

    bool IsAutoConnectMode();
};

#endif
