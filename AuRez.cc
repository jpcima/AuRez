//          Copyright Jean Pierre Cimalando 2018.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MacRsrc.h"
#include "AuRsrc.h"
#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

void show_help()
{
    fprintf(stderr,
            "Usage: AuRez <type-id> <subtype-id> <manufacturer-id>\n"
            "             [-o|--output <file.rsrc>]\n"
            "             [-A|--arch <arch>]* [-M|--manufacturer <name>] [-P|--product <name>] [-V|--version <x.y.z>]\n"
            "             [-e|--entry <function>] [-v|--view-entry <function>]\n"
            "             [-n|--no-view]"
            "\n"
            "Acceptable values for <type-id>:\n"
            "    `auou` Output          `aumu` MusicDevice     `aumf` MusicEffect\n"
            "    `aufc` FormatConverter `aufx` Effect          `aumx` Mixer\n"
            "    `aupn` Panner          `auol` OfflineEffect   `augn` Generator\n");
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        show_help();
        return 0;
    }

    unsigned archs = 0;
    const char *manufacturer = nullptr;
    const char *product = nullptr;
    const char *version_string = nullptr;
    const char *plugin_entry = nullptr;
    const char *view_entry = nullptr;
    const char *output_file = nullptr;
    bool view_enabled = true;

    static const option cmd_opts[] = {
        {"output", 1, nullptr, 'o'},
        {"arch", 1, nullptr, 'A'},
        {"manufacturer", 1, nullptr, 'M'},
        {"product", 1, nullptr, 'P'},
        {"version", 1, nullptr, 'V'},
        {"entry", 1, nullptr, 'e'},
        {"view-entry", 1, nullptr, 'v'},
        {"no-view", 0, nullptr, 'n'},
        {}
    };

    for (int c; (c = getopt_long(argc, argv, "o:A:M:P:V:e:v:n", cmd_opts, nullptr)) != -1;) {
        switch (c) {
        case 'o':
            output_file = optarg;
            break;
        case 'A': {
            const char *arch = optarg;
            if (!strcmp(arch, "i386"))
                archs |= (1 << ArchIA32NativeEntryPoint);
            else if (!strcmp(arch, "x86_64"))
                archs |= (1 << ArchX86_64NativeEntryPoint);
            else if (!strcmp(arch, "ppc"))
                archs |= (1 << ArchPowerPCNativeEntryPoint);
            else if (!strcmp(arch, "ppc64"))
                archs |= (1 << ArchPowerPC64NativeEntryPoint);
            else {
                fprintf(stderr, "Unrecognized architecture `%s`\n", arch);
                return 1;
            }
            break;
        }
        case 'M': {
            manufacturer = optarg;
            break;
        }
        case 'P': {
            product = optarg;
            break;
        }
        case 'V': {
            version_string = optarg;
            break;
        }
        case 'e': {
            plugin_entry = optarg;
            break;
        }
        case 'v': {
            view_entry = optarg;
            break;
        }
        case 'n': {
            view_enabled = false;
            break;
        }
        default:
            show_help();
            return 1;
        }
    }

    if (optind != argc - 3) {
        fprintf(stderr, "AuRez requires 3 positional arguments.\n");
        show_help();
        return 1;
    }

    const char *arg_type = argv[optind];
    const char *arg_subtype = argv[optind + 1];
    const char *arg_manuf = argv[optind + 2];

    if (strlen(arg_type) > 4) {
        fprintf(stderr, "The type-id cannot be longer than 4 characters.\n");
        return 1;
    }
    if (strlen(arg_subtype) > 4) {
        fprintf(stderr, "The subtype-id cannot be longer than 4 characters.\n");
        return 1;
    }
    if (strlen(arg_manuf) > 4) {
        fprintf(stderr, "The manufacturer-id cannot be longer than 4 characters.\n");
        return 1;
    }

    char type[4] = {' ', ' ', ' ', ' '};
    char subtype[4] = {' ', ' ', ' ', ' '};
    char manuf[4] = {' ', ' ', ' ', ' '};
    memcpy(type, arg_type, strlen(arg_type));
    memcpy(subtype, arg_subtype, strlen(arg_subtype));
    memcpy(manuf, arg_manuf, strlen(arg_manuf));

    if (archs == 0) {
        archs = (1 << ArchIA32NativeEntryPoint) | (1 << ArchX86_64NativeEntryPoint);
        fprintf(stderr, "Architectures not set, defaulting to i386 and x86_64\n");
    }
    if (!manufacturer) {
        manufacturer = "ACME";
        fprintf(stderr, "Manufacturer not set, defaulting to `%s`\n", manufacturer);
    }
    if (!product) {
        product = "Biniou";
        fprintf(stderr, "Product not set, defaulting to `%s`\n", product);
    }
    if (!version_string) {
        version_string = "1.0.0";
        fprintf(stderr, "Version not set, defaulting to `%s`\n", version_string);
    }
    if (!plugin_entry) {
        plugin_entry = "AUEntry";
        fprintf(stderr, "Plugin entry not set, defaulting to `%s`\n", plugin_entry);
    }
    if (view_enabled && !view_entry) {
        view_entry = "AUViewEntry";
        fprintf(stderr, "View entry not set, defaulting to `%s`\n", view_entry);
    }

    uint16_t id_plugin = 1000;
    uint16_t id_plugin_info = id_plugin + 1;
    uint16_t id_view = 2000;
    uint16_t id_view_info = id_view + 1;

    unsigned versionX, versionY, versionZ;
    if (sscanf(version_string, "%u.%u.%u", &versionX, &versionY, &versionZ) != 3) {
        fprintf(stderr, "The version string is not in `x.y.z` format.\n");
        return 1;
    }

    if (versionX > 255 || versionY > 255 || versionZ > 255) {
        fprintf(stderr, "The version components cannot be greater than 255.\n");
        return 1;
    }

    uint32_t version = (versionX << 16) | (versionY << 8) | versionZ;

    MacRsrc rsrc;

    rsrc.AddResource(
        id_plugin, ResPurgeable, "STR", "", MakeSTR(std::string(manufacturer) + ": " + product));
    rsrc.AddResource(
        id_plugin_info, ResPurgeable, "STR", "", MakeSTR(product));
    rsrc.AddResource(
        id_plugin, 0, "dlle", "", MakeCSTR(plugin_entry));
    rsrc.AddResource(
        id_plugin, 0, "thng", std::string(manufacturer) + ": " + product,
        MakeThng(archs, id_plugin, id_plugin_info, version, type, subtype, manuf));

    if (view_enabled) {
        rsrc.AddResource(
            id_view, ResPurgeable, "STR", "", MakeSTR(std::string(manufacturer) + ": " + product + " View"));
        rsrc.AddResource(
            id_view_info, ResPurgeable, "STR", "", MakeSTR(std::string(manufacturer) + ": " + product + " View"));
        rsrc.AddResource(
            id_view, 0, "dlle", "", MakeCSTR(view_entry));
        rsrc.AddResource(
            id_view, 0, "thng", std::string(manufacturer) + ": " + product + " View",
            MakeThng(archs, id_view, id_view_info, version, "auvw", subtype, manuf));
    }

    FILE *fh = output_file ? fopen(output_file, "wb") : stdout;
    if (!fh) {
        fprintf(stderr, "Cannot open output file.");
        return 1;
    }

    if (isatty(fileno(fh)))
        rsrc.DisplayAsText(fh);
    else {
        if (rsrc.Write(fh) == -1) {
            fprintf(stderr, "Cannot write resource output.");
            return 1;
        }
    }

    if (fh != stdout)
        fclose(fh);

    return 0;
}
