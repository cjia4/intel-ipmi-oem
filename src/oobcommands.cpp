/*
// Copyright (c) 2018 Intel Corporation
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

#include "xyz/openbmc_project/Common/error.hpp"
#include <host-ipmid/ipmid-api.h>
#include <array>
#include <commandutils.hpp>
#include <iostream>
#include <oemcommands.hpp>
#include <phosphor-ipmi-host/utils.hpp>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>
#include <oobcommands.hpp>
#include <ImageUpdate.hpp>

namespace ipmi
{
static void initOOB() __attribute__((constructor));
sdbusplus::bus::bus dbus(ipmid_get_sd_bus_connection()); // from ipmid-api.h

static constexpr uint8_t imageTypes = 5; //0-XML type 0 / 1- xml type 1 / 2-bios image / 3- ME image / 4 -allinone (BIOS and ME) image

std::vector<DataTransfer> gPayload;

uint8_t ValidateBIOSPwd(uint8_t *userpwd);
ipmi_ret_t ipmiOOBSetPayload(ipmi_netfn_t netfn, ipmi_cmd_t cmd,
                             ipmi_request_t request,
                             ipmi_response_t response,
                             ipmi_data_len_t dataLen,
                             ipmi_context_t context)
{
    SetPayloadReq*  pPayloadReq  = reinterpret_cast<SetPayloadReq*>(request);
    SetPayloadResp* pPayloadResp = reinterpret_cast<SetPayloadResp*>(response);
    uint32_t u32Crc;
    size_t   returnSize=0;
    OOBImageType currentPayloadType = pPayloadReq->payloadType;
    ipmi_ret_t rc=IPMI_CC_OK;

    //This is a must step as currentPayloadType == INVALID_IMG_TYPE after module init or each transfer complete.
    if (currentPayloadType >= OOBImageType::invalidType) {
        phosphor::logging::log<phosphor::logging::level::ERR>("invalid image type");
        *dataLen = 0;
        return OOBCompleteCode::compcodePayloadTypeNotSupported;
    }

    phosphor::logging::log<phosphor::logging::level::DEBUG>("PARASEL", phosphor::logging::entry("PARASEL=%x", pPayloadReq->paraSel));

    switch(pPayloadReq->paraSel) {
        case TransferState::PasswdAuth:
           if(success != ValidateBIOSPwd(pPayloadReq->Input.Para0.BIOSPassword)){
                phosphor::logging::log<phosphor::logging::level::ERR>("Bios Password Verification FAILED");
                *dataLen = 0;
                return compcodePwdVerficationFailed;
                
            }
            std::copy((pPayloadReq->Input.Para0.BIOSPassword), (pPayloadReq->Input.Para0.BIOSPassword + maxPasswordLength),  
                      (gPayload[currentPayloadType].ExData.BIOSPassword));
            phosphor::logging::log<phosphor::logging::level::DEBUG>("Bios Password Verification PASSED");
            *dataLen = 1;
            break;
        case TransferState::StartTransfer:
            break;
        case TransferState::InProgress:
            break;
        case TransferState::EndOfTransfer:
            break;
        case TransferState::UserAbort:
            break;
        default:
            *dataLen = 0;
            rc = IPMI_CC_INVALID_FIELD_REQUEST;
    }
    gPayload[1].status = pPayloadReq->paraSel;

    return rc;
}




ipmi_ret_t ipmiOOBGetPayload(ipmi_netfn_t netfn, ipmi_cmd_t cmd,
                             ipmi_request_t request,
                             ipmi_response_t response,
                             ipmi_data_len_t dataLen,
                             ipmi_context_t context)
{
    GetPayloadReq*  pPayloadReq  = reinterpret_cast<GetPayloadReq*>(request);
    GetPayloadResp* pPayloadResp = reinterpret_cast<GetPayloadResp*>(response);
    uint32_t u32Crc;
    uint8_t  *Imgbuffer;
    size_t   returnSize=0;
    OOBImageType currentPayloadType =static_cast<OOBImageType>(pPayloadReq->Input.Para0.payloadType);
    ipmi_ret_t rc=IPMI_CC_OK;

    //This is a must step as CurrentPayloadType == INVALID_IMG_TYPE after module init or each transfer complete.
    if (currentPayloadType >= OOBImageType::invalidType) {
        phosphor::logging::log<phosphor::logging::level::ERR>("invalid image type");
        *dataLen = 0;
        return OOBCompleteCode::compcodePayloadTypeNotSupported;
    }

    phosphor::logging::log<phosphor::logging::level::DEBUG>("ipmiOOBGetPayload", phosphor::logging::entry("ipmiOOBGetPayload Param=%x", pPayloadReq->paraSel));

    switch(pPayloadReq->paraSel) {
        case GetPayloadParameter::PayloadInfo:
            returnSize = 0;
            pPayloadResp->Output.Para0.payloadType = pPayloadReq->Input.Para0.payloadType;
            pPayloadResp->Output.Para0.payloadVersion = gPayload[pPayloadReq->Input.Para0.payloadType].payloadVersion;
            pPayloadResp->Output.Para0.totalPayloadlength = gPayload[pPayloadReq->Input.Para0.payloadType].givenPayloadSize;
            pPayloadResp->Output.Para0.totalPayloadchecksum = gPayload[pPayloadReq->Input.Para0.payloadType].givenPayloadChecksum;
            pPayloadResp->Output.Para0.payloadCurrentStatus = gPayload[pPayloadReq->Input.Para0.payloadType].status;
            pPayloadResp->Output.Para0.timeStamp = gPayload[pPayloadReq->Input.Para0.payloadType].uploadTimeStamp;
            pPayloadResp->Output.Para0.payloadFlag = gPayload[pPayloadReq->Input.Para0.payloadType].payloadFlag;
            rc = IPMI_CC_OK;
            rc = sizeof(pPayloadResp->Output.Para0);

            break;
        case GetPayloadParameter::PayloadData:
           if(pPayloadReq->Input.Para1.payloadLength == 0) {
               rc = IPMI_CC_INVALID_FIELD_REQUEST;
               *dataLen = 0;
               break;
           }
           Imgbuffer = new uint8_t(pPayloadReq->Input.Para1.payloadLength);
           if(Imgbuffer != NULL) {
               pPayloadResp->Output.Para1.actualPayloadlength = imgReadFromFile(pPayloadReq->Input.Para1.payloadType ,Imgbuffer, pPayloadReq->Input.Para1.payloadOffset,pPayloadReq->Input.Para1.payloadLength, &pPayloadResp->Output.Para1.actualPayloadChecksum );
               rc=IPMI_CC_OK;
               memcpy(pPayloadResp->Output.Para1.payloadData,Imgbuffer, pPayloadResp->Output.Para1.actualPayloadlength);
               *dataLen =  pPayloadResp->Output.Para1.actualPayloadlength+ sizeof(pPayloadResp->Output.Para1.actualPayloadlength)+sizeof(pPayloadResp->Output.Para1.actualPayloadChecksum)+ sizeof(pPayloadResp->Output.Para1.payloadType) ;
               delete Imgbuffer;
           } else {
               rc=OOBCompleteCode::compcodePayloadTypeNotSupported;
               *dataLen = 0;
           }

          break;
        case GetPayloadParameter::PayloadStatus:
            pPayloadResp->Output.Para2.status= gPayload[pPayloadReq->Input.Para0.payloadType].status;
            rc = IPMI_CC_OK;
             *dataLen = sizeof(pPayloadResp->Output.Para2.status);
            break;

        default:
            *dataLen = 0;
            rc = IPMI_CC_INVALID_FIELD_REQUEST;
    }

    return rc;
}


static void initOOB(void)
{
    phosphor::logging::log<phosphor::logging::level::INFO>(
        "OOB module initialization");

    gPayload.reserve(imageTypes);

    ipmiPrintAndRegister(
        netfnIntcOEMGeneral,
        static_cast<ipmi_cmd_t>(
            IPMINetfnIntelOEMGeneralCmd::cmdSetPayload),
        NULL, ipmiOOBSetPayload, PRIVILEGE_ADMIN);
    ipmiPrintAndRegister(
        netfnIntcOEMGeneral,
        static_cast<ipmi_cmd_t>(
            IPMINetfnIntelOEMGeneralCmd::cmdGetPayload),
        NULL, ipmiOOBGetPayload, PRIVILEGE_USER);

    return;
}

}

