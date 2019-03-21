//          Copyright Jean Pierre Cimalando 2018.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <string>
#include <vector>
#include <stdint.h>

std::vector<uint8_t> MakeSTR(const std::string &string);
std::vector<uint8_t> MakeCSTR(const std::string &string);

enum CarbonArchitecture
{
    ArchsBegin = 1,
    Arch68k = ArchsBegin,
    ArchPowerPC,
    ArchInterpreted,
    ArchWin32,
    ArchPowerPCNativeEntryPoint,
    ArchIA32NativeEntryPoint,
    ArchPowerPC64NativeEntryPoint,
    ArchX86_64NativeEntryPoint,
    ArchsEnd
};

#pragma pack(push, 1)
struct CarbonThng
{
    char Type[4];
    char Subtype[4];
    char Manufacturer[4];
    uint32_t ComponentFlags;
    uint32_t ComponentFlagsMask;
    char CodeType[4];
    uint16_t CodeId;
    char NameType[4];
    uint16_t NameId;
    char InfoType[4];
    uint16_t InfoId;
    char IconType[4];
    uint16_t IconId;
    uint32_t ComponentVersion;
    uint32_t RegistrationFlags;
    uint16_t IconFamilyId;
    struct PlatformInfo {
        uint32_t ComponentFlags;
        char CodeType[4];
        uint16_t CodeId;
        uint16_t PlatformType;
    };
    uint32_t PlatformInfoCount;
    PlatformInfo PlatformInfos[];
};
#pragma pack(pop)

std::vector<uint8_t> MakeThng(
    unsigned archs, uint16_t id_plugin, uint16_t id_info, uint32_t version,
    const char type[4], const char subtype[4], const char manuf[4]);
