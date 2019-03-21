//          Copyright Jean Pierre Cimalando 2018.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "AuRsrc.h"
#include <string.h>
#include <arpa/inet.h>

std::vector<uint8_t> MakeSTR(const std::string &string)
{
    size_t size = string.size();
    if (size > 255)
        size = 255;

    std::vector<uint8_t> pstr;
    pstr.reserve(size + 1);
    pstr.push_back((uint8_t)size);
    pstr.insert(pstr.end(), &string[0], &string[size]);

    return pstr;
}

std::vector<uint8_t> MakeCSTR(const std::string &string)
{
    const char *cstr = string.c_str();
    return std::vector<uint8_t>(cstr, cstr + string.size() + 1);
}

std::vector<uint8_t> MakeThng(
    unsigned archs, uint16_t id_plugin, uint16_t id_info, uint32_t version,
    const char type[4], const char subtype[4], const char manuf[4])
{
    unsigned num_archs = 0;
    for (unsigned a = ArchsBegin; a < ArchsEnd; ++a)
        num_archs += (archs & (1 << a)) != 0;

    std::vector<uint8_t> thng_buf(sizeof(CarbonThng) + num_archs * sizeof(CarbonThng::PlatformInfo));
    CarbonThng *thng = (CarbonThng *)thng_buf.data();
    memcpy(thng->Type, type, 4);
    memcpy(thng->Subtype, subtype, 4);
    memcpy(thng->Manufacturer, manuf, 4);
    memcpy(thng->NameType, "STR ", 4);
    thng->NameId = htons(id_plugin);
    memcpy(thng->InfoType, "STR ", 4);
    thng->InfoId = htons(id_info);
    thng->ComponentVersion = htonl(version);
    thng->RegistrationFlags = htonl(9);  // componentHasMultiplePlatforms | componentDoAutoVersion
    thng->PlatformInfoCount = htonl(num_archs);

    for (unsigned a = ArchsBegin, i = 0; a < ArchsEnd; ++a) {
        if ((archs & (1 << a)) == 0) continue;
        CarbonThng::PlatformInfo &pl = thng->PlatformInfos[i];
        pl.ComponentFlags = htonl(0x10000000);  // cmpThreadSafeOnMac
        memcpy(pl.CodeType, "dlle", 4);
        pl.CodeId = htons(id_plugin);
        pl.PlatformType = htons(a);
        ++i;
    }

    return thng_buf;
}
