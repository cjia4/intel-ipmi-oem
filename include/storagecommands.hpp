/*
// Copyright (c) 2017 2018 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/

#pragma once
#include <cstdint>
#include <phosphor-ipmi-host/sensorhandler.hpp>

static constexpr uint8_t ipmiSdrVersion = 0x51;

#pragma pack(push, 1)
struct GetSDRInfoResp
{
    uint8_t sdrVersion;
    uint8_t recordCountLS;
    uint8_t recordCountMS;
    uint8_t freeSpace[2];
    uint32_t mostRecentAddition;
    uint32_t mostRecentErase;
    uint8_t operationSupport;
};

struct GetSDRReq
{
    uint16_t reservationID;
    uint16_t recordID;
    uint8_t offset;
    uint8_t bytesToRead;
};
#pragma pack(pop)

enum class SdrRepositoryInfoOps : uint8_t
{
    allocCommandSupported = 0x1,
    reserveSDRRepositoryCommandSupported = 0x2,
    partialAddSDRSupported = 0x4,
    deleteSDRSupported = 0x8,
    reserved = 0x10,
    modalLSB = 0x20,
    modalMSB = 0x40,
    overflow = 0x80
};

#pragma pack(push, 1)
struct GetAllocInfoResp
{
    uint8_t allocUnitsLSB;
    uint8_t allocUnitsMSB;
    uint8_t allocUnitSizeLSB;
    uint8_t allocUnitSizeMSB;
    uint8_t allocUnitFreeLSB;
    uint8_t allocUnitFreeMSB;
    uint8_t allocUnitLargestFreeLSB;
    uint8_t allocUnitLargestFreeMSB;
    uint8_t maxRecordSize;
};

struct GetFRUAreaReq
{
    uint8_t fruDeviceID;
    uint16_t fruInventoryOffset;
    uint8_t countToRead;
};

struct GetFRUAreaResp
{
    uint8_t inventorySizeLSB;
    uint8_t inventorySizeMSB;
    uint8_t accessType;
};

struct WriteFRUDataReq
{
    uint8_t fruDeviceID;
    uint16_t fruInventoryOffset;
    uint8_t data[];
};
#pragma pack(pop)

enum class GetFRUAreaAccessType : uint8_t
{
    byte = 0x0,
    words = 0x1
};

enum class SensorTypeCodes : uint8_t
{
    reserved = 0x0,
    temperature = 0x1,
    voltage = 0x2,
    current = 0x3,
    fan = 0x4,
    other = 0xB,
};

enum class SensorUnits : uint8_t
{
    unspecified = 0x0,
    degreesC = 0x1,
    volts = 0x4,
    amps = 0x5,
    watts = 0x6,
    rpm = 0x12,
};

enum class IPMINetfnStorageCmds : ipmi_cmd_t
{
    ipmiCmdGetFRUInvAreaInfo = 0x10,
    ipmiCmdReadFRUData = 0x11,
    ipmiCmdWriteFRUData = 0x12,
    ipmiCmdGetRepositoryInfo = 0x20,
    ipmiCmdGetSDRAllocationInfo = 0x21,
    ipmiCmdReserveSDR = 0x22,
    ipmiCmdGetSDR = 0x23,
    ipmiCmdGetSELInfo = 0x40,
    ipmiCmdReserveSEL = 0x42,
    ipmiCmdGetSELEntry = 0x43,
    ipmiCmdAddSEL = 0x44,
    ipmiCmdDeleteSEL = 0x46,
    ipmiCmdClearSEL = 0x47,
    ipmiCmdGetSELTime = 0x48,
    ipmiCmdSetSELTime = 0x49,
};

#pragma pack(push, 1)
struct FRUHeader
{
    uint8_t commonHeaderFormat;
    uint8_t internalOffset;
    uint8_t chassisOffset;
    uint8_t boardOffset;
    uint8_t productOffset;
    uint8_t multiRecordOffset;
    uint8_t pad;
    uint8_t checksum;
};
#pragma pack(pop)

namespace ipmi
{
namespace storage
{
ipmi_ret_t getFruSdrs(size_t index, get_sdr::SensorDataFruRecord& resp);

ipmi_ret_t getFruSdrCount(size_t& count);
} // namespace storage
} // namespace ipmi
