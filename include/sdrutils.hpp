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

#include <boost/algorithm/string.hpp>
#include <boost/container/flat_map.hpp>
#include <cstring>
#include <phosphor-logging/log.hpp>
#include <storagecommands.hpp>

#pragma once

struct CmpStrVersion
{
    bool operator()(std::string a, std::string b) const
    {
        return strverscmp(a.c_str(), b.c_str()) < 0;
    }
};

using SensorSubTree = boost::container::flat_map<
    std::string,
    boost::container::flat_map<std::string, std::vector<std::string>>,
    CmpStrVersion>;

inline static bool getSensorSubtree(SensorSubTree& subtree)
{
    sd_bus* bus = NULL;
    int ret = sd_bus_default_system(&bus);
    if (ret < 0)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Failed to connect to system bus",
            phosphor::logging::entry("ERRNO=0x%X", -ret));
        sd_bus_unref(bus);
        return false;
    }
    sdbusplus::bus::bus dbus(bus);
    auto mapperCall =
        dbus.new_method_call("xyz.openbmc_project.ObjectMapper",
                             "/xyz/openbmc_project/object_mapper",
                             "xyz.openbmc_project.ObjectMapper", "GetSubTree");
    static const auto depth = 2;
    static constexpr std::array<const char*, 3> interfaces = {
        "xyz.openbmc_project.Sensor.Value",
        "xyz.openbmc_project.Sensor.Threshold.Warning",
        "xyz.openbmc_project.Sensor.Threshold.Critical"};
    mapperCall.append("/xyz/openbmc_project/sensors", depth, interfaces);

    try
    {
        auto mapperReply = dbus.call(mapperCall);
        subtree.clear();
        mapperReply.read(subtree);
    }
    catch (sdbusplus::exception_t&)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "getSensorSubtree: Error calling mapper");
        return false;
    }
    return true;
}

struct CmpStr
{
    bool operator()(const char* a, const char* b) const
    {
        return std::strcmp(a, b) < 0;
    }
};

const static boost::container::flat_map<const char*, SensorTypeCodes, CmpStr>
    sensorTypes{{{"temperature", SensorTypeCodes::temperature},
                 {"voltage", SensorTypeCodes::voltage},
                 {"current", SensorTypeCodes::current},
                 {"fan_tach", SensorTypeCodes::fan},
                 {"power", SensorTypeCodes::other}}};

inline static std::string getSensorTypeStringFromPath(const std::string& path)
{
    // get sensor type string from path, path is defined as
    // /xyz/openbmc_project/sensors/<type>/label
    size_t typeEnd = path.rfind("/");
    size_t typeStart = path.rfind("/", typeEnd - 1) + 1;
    if (typeEnd != std::string::npos && typeStart != std::string::npos)
    {
        return path.substr(typeStart, typeEnd - typeStart);
    }
    return path;
}

inline static uint8_t getSensorTypeFromPath(const std::string& path)
{
    uint8_t sensorType = 0;
    std::string type = getSensorTypeStringFromPath(path);
    auto findSensor = sensorTypes.find(type.c_str());
    if (findSensor != sensorTypes.end())
    {
        sensorType = static_cast<uint8_t>(findSensor->second);
    } // else default 0x0 RESERVED

    return sensorType;
}

inline static uint8_t getSensorNumberFromPath(const std::string& path)
{
    SensorSubTree sensorTree;
    if (!getSensorSubtree(sensorTree))
        return 0xFF;
    uint8_t sensorNum = 0xFF;

    for (const auto& sensor : sensorTree)
    {
        sensorNum++;
        if (sensor.first == path)
        {
            break;
        }
    }
    return sensorNum;
}

inline static uint8_t getSensorEventTypeFromPath(const std::string& path)
{
    // TODO: Add support for additional reading types as needed
    return 0x1; // reading type = threshold
}

inline static std::string getPathFromSensorNumber(uint8_t sensorNum)
{
    SensorSubTree sensorTree;
    std::string path;
    if (!getSensorSubtree(sensorTree))
        return path;

    if (sensorTree.size() < sensorNum)
    {
        return path;
    }

    uint8_t sensorIndex = sensorNum;
    for (const auto& sensor : sensorTree)
    {
        if (sensorIndex-- == 0)
        {
            path = sensor.first;
            break;
        }
    }

    return path;
}
