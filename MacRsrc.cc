//          Copyright Jean Pierre Cimalando 2018.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MacRsrc.h"
#include <algorithm>
#include <stdio.h>
#include <string.h>

MacRsrc::MacRsrc()
{
}

MacRsrc::~MacRsrc()
{
}

size_t MacRsrc::GetResourceCount() const
{
    return fResources.size();
}

const MacRsrc::Resource &MacRsrc::GetResource(size_t index)
{
    return fResources[index];
}

uint16_t MacRsrc::FileAttributes() const
{
    return fFileAttributes;
}

void MacRsrc::SetFileAttributes(uint16_t attr)
{
    fFileAttributes = attr;
}

int MacRsrc::AddResource(uint32_t id, uint8_t attr, const char *type, std::string name, const std::vector<uint8_t> &data)
{
    Resource res;
    res.Id = id;
    res.Attr = attr;

    size_t TypeLen = strlen(type);
    if (TypeLen > 4)
        return -1;

    for (size_t i = 0; i < TypeLen; ++i)
        res.Type[i] = type[i];
    for (size_t i = TypeLen; i < 4; ++i)
        res.Type[i] = ' ';

    res.Name = std::move(name);
    res.DataOffset = fData.size();
    res.DataSize = data.size();
    fData.insert(fData.end(), data.begin(), data.end());

    fResources.push_back(std::move(res));
    return 0;
}

static int WriteU16(uint32_t value, FILE *stream)
{
    for (unsigned i = 0; i < 2; ++i) {
        if (fputc((value >> (8 * (1 - i))) & 0xff, stream) == EOF)
            return -1;
    }
    return 0;
}

static int WriteU24(uint32_t value, FILE *stream)
{
    for (unsigned i = 0; i < 3; ++i) {
        if (fputc((value >> (8 * (2 - i))) & 0xff, stream) == EOF)
            return -1;
    }
    return 0;
}

static int WriteU32(uint32_t value, FILE *stream)
{
    for (unsigned i = 0; i < 4; ++i) {
        if (fputc((value >> (8 * (3 - i))) & 0xff, stream) == EOF)
            return -1;
    }
    return 0;
}

int MacRsrc::Write(FILE *stream) const
{
    const std::vector<Resource> &Resources = fResources;
    size_t NumResources = Resources.size();

    // Header (fill later)
    off_t HeaderPos = ftell(stream);
    for (unsigned i = 0; i < 256; ++i)
        fputc(0, stream);

    // Resource data
    off_t ResourceDataPos = ftell(stream);
    for (size_t i = 0; i < NumResources; ++i) {
        const Resource &Res = Resources[i];
        WriteU32(Res.DataSize, stream);
        fwrite(&fData[Res.DataOffset], 1, Res.DataSize, stream);
    }

    // Resource map (fill later)
    off_t ResourceMapPos = ftell(stream);
    for (unsigned i = 0; i < 22; ++i)
        fputc(0, stream);
    WriteU16(fFileAttributes, stream);
    off_t ResourceMapOffsetsPos = ftell(stream);
    WriteU16(0, stream); // fill later
    WriteU16(0, stream); // fill later

    // Type list (fill later)
    struct TypeInfo
    {
        std::array<char, 4> Type;
        std::vector<size_t> Resources;
        off_t ReferenceListOffsetPos = 0;
    };

    std::vector<TypeInfo> TypeList;
    for (size_t R_i = 0; R_i < NumResources; ++R_i) {
        std::array<char, 4> Type = Resources[R_i].Type;
        auto Position = std::find_if(
            TypeList.begin(), TypeList.end(),
            [Type](const TypeInfo &x) -> bool { return x.Type == Type; });
        if (Position == TypeList.end()) {
            TypeInfo Info;
            Info.Type = Type;
            TypeList.push_back(Info);
            Position = TypeList.end() - 1;
        }
        Position->Resources.push_back(R_i);
    }

    off_t TypeListPos = ftell(stream);
    size_t NumTypes = TypeList.size();

    WriteU16((uint16_t)NumTypes - 1, stream);
    for (size_t i = 0; i < NumTypes; ++i) {
        fwrite(TypeList[i].Type.data(), 1, 4, stream);
        WriteU16((uint16_t)TypeList[i].Resources.size() - 1, stream);
        TypeList[i].ReferenceListOffsetPos = ftell(stream);
        WriteU16(0, stream); // fill later
    }

    // Reference list (fill later)
    std::vector<off_t> ReferenceNamePos(NumResources, -1);
    std::vector<size_t> ReferenceListOffset(NumTypes);
    for (size_t T_i = 0; T_i < NumTypes; ++T_i) {
        const TypeInfo &Type = TypeList[T_i];
        ReferenceListOffset[T_i] = ftell(stream) - TypeListPos;
        for (size_t R_i : Type.Resources) {
            const Resource &Res = Resources[R_i];
            WriteU16(Res.Id, stream);
            ReferenceNamePos[R_i] = ftell(stream);
            WriteU16((uint16_t)-1, stream); // fill later
            fputc(Res.Attr, stream);
            WriteU24(Res.DataOffset + R_i * sizeof(uint32_t), stream);
            WriteU32(0, stream);
        }
    }

    // Resource name list
    off_t NameListPos = ftell(stream);
    std::vector<size_t> NameOffset(NumResources);
    for (size_t R_i = 0; R_i < NumResources; ++R_i) {
        const Resource &Res = Resources[R_i];
        if (Res.Name.empty())
            NameOffset[R_i] = (size_t)-1;
        else {
            NameOffset[R_i] = ftell(stream) - NameListPos;
            fputc(Res.Name.size(), stream);
            fwrite(Res.Name.data(), 1, Res.Name.size(), stream);
        }
    }

    off_t EndFilePos = ftell(stream);

    // Fill reference list offsets
    for (size_t R_i = 0; R_i < NumResources; ++R_i) {
        fseek(stream, ReferenceNamePos[R_i], SEEK_SET);
        WriteU16((uint16_t)NameOffset[R_i], stream);
    }

    // Fill type list offsets
    for (size_t T_i = 0; T_i < NumTypes; ++T_i) {
        fseek(stream, TypeList[T_i].ReferenceListOffsetPos, SEEK_SET);
        WriteU16(ReferenceListOffset[T_i], stream);
    }

    // Fill resource map offsets
    fseek(stream, ResourceMapOffsetsPos, SEEK_SET);
    WriteU16(TypeListPos - ResourceMapPos, stream);
    WriteU16(NameListPos - ResourceMapPos, stream);

    // Fill header
    fseek(stream, HeaderPos, SEEK_SET);
    WriteU32(ResourceDataPos - HeaderPos, stream);
    WriteU32(ResourceMapPos - HeaderPos, stream);
    WriteU32(ResourceMapPos - ResourceDataPos, stream);
    WriteU32(EndFilePos - ResourceMapPos, stream);

    if (fflush(stream) != 0)
        return -1;

    return 0;
}

static void HexDump(FILE *stream, const uint8_t *data, size_t size)
{
    size_t addr = 0;

    while (addr < size) {
        fprintf(stream, "%08lx|", addr);

        for (size_t a = addr; a < addr + 16; ++a) {
            if (a < size)
                fprintf(stream, " %02X", data[a]);
            else
                fprintf(stream, "   ");
        }

        fprintf(stream, " | ");

        for (size_t a = addr; a < addr + 16; ++a) {
            if (a < size) {
                uint8_t ch = data[a];
                bool printable = ch >= 0x20 && ch < 0x7f;
                fprintf(stream, "%c", printable ? ch : '.');
            }
            else
                fprintf(stream, " ");
        }

        fprintf(stream, "\n");
        addr += 16;
    }
}

void MacRsrc::DisplayAsText(FILE *stream) const
{
    const std::vector<Resource> &Resources = fResources;
    size_t NumResources = Resources.size();

    for (size_t i = 0; i < NumResources; ++i) {
        const Resource &Res = Resources[i];

        if (i > 0)
            fprintf(stream, "\n");

        fprintf(stream, "Resource %u `%.4s` Attributes:$%02x Name:`%s`\n",
                Res.Id, Res.Type.data(), Res.Attr, Res.Name.c_str());
        HexDump(stream, &fData[Res.DataOffset], Res.DataSize);
    }
}
