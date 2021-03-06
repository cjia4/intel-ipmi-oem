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

#pragma once

static constexpr size_t success = 0x0;
static constexpr size_t failure = 0x1;
#pragma pack(push, 1)

enum  OOBImageType 
{
    BIOSXMLType0 = 0,
    BIOSXMLType1 = 1,
    BIOSImage    = 2,
    MEImage      = 3,
    FDImage      = 4,
    invalidType,  //this item should always be the last one
};

struct SetPayloadReq
{
    uint8_t paraSel;      //Parameter Selector  0-Init 1-in Progress 2 -End of  transfer 3 - user abort
    OOBImageType payloadType;  // 1 byte	 0-XML type 0 ; 1-XML type 1; 2-BIOS; 3 - ME image; 4- FD image
    union {
        struct Parameter0
        {
            uint16_t payloadVersion;  // 2 bytes
            uint32_t payloadSize;     // 4 bytes
            uint32_t payloadChecksum; // 4 bytes
            uint8_t  payloadFlag;      // 1 byte
            uint8_t  BIOSPassword[maxPasswordLength]; // 32 bytes  (only available when PayloadType == 1)
        } Para0;      //when ParaSel == 0

        struct Parameter1
        {
            uint32_t reservationToken;
            uint32_t writingPayloadOffset;   // 4 bytes Writing payload data length
            uint32_t writingPayloadLength;   // 4 bytes Writing payload data length
            uint32_t writingPayloadChecksum; // 4 bytes Writing payload data checksum
            uint8_t  data[maxPayloadLengthPerTransfer];                  // data pointer
        } Para1;      //when ParaSel == 1
        struct Parameter2
        {
            uint32_t reservationToken;
        } Para2;
        struct Parameter3
        {
            uint32_t reservationToken;
        } Para3;

    } Input;
} ;


union SetPayloadResp
{
    uint32_t reservationToken;           //paramter == 0
    uint32_t actualPayloadWritten;       //paramter == 1           in progress
    uint32_t actualTotalPayloadWritten;  //paramter == 2  or 3     end of transfer or user abort
} ;



struct GetPayloadReq
{
    uint8_t paraSel;    //Parameter Selector  0-Init 1-in Progress 2 -End of  transfer 3 - user abort
    union Input{
        struct Parameter0
        {
            uint8_t  payloadType;     // 1 byte    0-XML type 0 ; 1-XML type 1; 2-BIOS; 3 - ME image; 4- FD image
        } Para0;      //when ParaSel == 0

        struct Parameter1
        {

            uint8_t payloadType;
            uint32_t payloadOffset; // 4 bytes Writing payload data offset to read
            uint32_t payloadLength;   // 4 bytes  payload data length
        } Para1;      //when ParaSel == 1
        struct Parameter2
        {
            uint8_t  payloadType;     // 1 byte    0-XML type 0 ; 1-XML type 1; 2-BIOS; 3 - ME image; 4- FD image
        } Para2;      //when ParaSel == 2

    }Input;
} ;


struct GetPayloadResp
{
     union Output{
        struct Parameter0
        {
            uint16_t  payloadVersion;     // 1 byte    0-XML type 0 ; 1-XML type 1; 2-BIOS; 3 - ME image; 4- FD image
            uint8_t   payloadType;
            uint32_t  totalPayloadlength;
            uint32_t  totalPayloadchecksum;
            uint8_t   payloadCurrentStatus;
            uint8_t   payloadFlag;
            uint32_t  timeStamp;
        } Para0;      //when ParaSel == 0

        struct Parameter1
        {
            uint8_t payloadType;
            uint32_t actualPayloadlength;   // 4 bytes  payload data length
            uint32_t actualPayloadChecksum; // 4 bytes Writing payload data checksum
            uint8_t  payloadData[maxPayloadLengthPerTransfer];
        } Para1;      //when ParaSel == 1
        struct Parameter2
        {
            uint8_t  status;
        } Para2;      //when ParaSel == 2

    }Output ;
} ;


struct DataTransfer
{
    uint8_t  imageType;
    uint8_t  status;
    uint8_t  payloadFlag;
    uint16_t payloadVersion;        //used by XML payload to indicate change order
    uint32_t givenPayloadSize;
    uint32_t givenPayloadChecksum;
    uint32_t actualPayloadSize;
    uint32_t actualPayloadChecksum;
    uint32_t uploadTimeStamp;
    int    fp;                   // fd for image data
    uint32_t reservationToken;
    union  {
        uint8_t  BIOSPassword[maxPasswordLength];
    } ExData;
};



enum OOBCompleteCode {
    compcodePayloadTypeNotSupported = 0x80,
    compcodePayloadChecksumFailed   = 0x81,
    compcodeCmdNotSupportedInState  = 0x82,
    compcodeTransferIncomplete      = 0x83,
    compcodePayloadPktMissing       = 0x84,
    compcodePayloadLengthIllegal    = 0x85,
    compcodePwdVerficationFailed    = 0x86,
    compcodeOSError                 = 0x87,
    compcodePayloadExeceedMaximum   = 0x88,
    compcodeNoSpaceForPayload       = 0xC4,
};

enum TransferState{
    StartTransfer = 0,
    InProgress    = 1,   
    EndOfTransfer = 2,
    UserAbort     = 3,
    NoImagePending= 4,
    PasswdAuth    = 5,
};

enum GetPayloadParameter{
    PayloadInfo = 0,
    PayloadData= 1,   //data transfer
    PayloadStatus = 2
};

#pragma pack(pop)
