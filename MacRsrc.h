//          Copyright Jean Pierre Cimalando 2018.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <string>
#include <vector>
#include <array>
#include <stdint.h>

class MacRsrc
{
public:
    MacRsrc();
    ~MacRsrc();

    struct Resource;

    size_t GetResourceCount() const;
    const Resource &GetResource(size_t index);

    uint16_t FileAttributes() const;
    void SetFileAttributes(uint16_t attr);

    int AddResource(uint32_t id, uint8_t attr, const char *type, std::string name, const std::vector<uint8_t> &data);

    int Write(FILE *stream) const;
    void DisplayAsText(FILE *stream) const;

    struct Resource
    {
        uint32_t Id = 0;
        uint8_t Attr = 0;
        std::array<char, 4> Type;
        std::string Name;
        size_t DataOffset = 0;
        size_t DataSize = 0;
    };

private:
    uint16_t fFileAttributes = 0;
    std::vector<Resource> fResources;
    std::vector<uint8_t> fData;
};

enum MacResourceAttribute
{
    ResSysHeap                    = 64,
    ResPurgeable                  = 32,
    ResLocked                     = 16,
    ResProtected                  = 8,
    ResPreload                    = 4,
    ResChanged                    = 2
};
