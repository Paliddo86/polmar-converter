#include "polmar_core.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

static Mode parseMode(const std::string& value) {
    if (value == "replace-all") return Mode::ReplaceAll;
    if (value == "freq-only") return Mode::FreqOnly;
    return Mode::MergeFull;
}

static DecimalMode parseDecimal(const std::string& value) {
    if (value == "dot") return DecimalMode::Dot;
    if (value == "comma") return DecimalMode::Comma;
    return DecimalMode::Auto;
}

int main(int argc, char** argv) {
    if (argc < 4) {
        std::cerr << "Uso: " << argv[0]
                  << " VECCHIO.dat NUOVO.dat OUTPUT.dat [mode] [decimal] [sqlevel] [globals]\n";
        return 2;
    }

    ConvertOptions options;
    options.oldPath = std::filesystem::path(argv[1]);
    options.newPath = std::filesystem::path(argv[2]);
    options.outputPath = std::filesystem::path(argv[3]);
    options.mode = argc > 4 ? parseMode(argv[4]) : Mode::MergeFull;
    options.decimal = argc > 5 ? parseDecimal(argv[5]) : DecimalMode::Auto;
    if (argc > 6) {
        std::string sq = argv[6];
        if (sq == "keep" || sq == "-1") {
            options.sqLevel = -1;
        } else if (sq == "off") {
            options.sqLevel = 0;
        } else {
            try {
                options.sqLevel = std::stoi(sq);
            } catch (...) {
                std::cerr << "sqlevel non valido: " << sq << "\n";
                return 2;
            }
        }
    }
    if (argc > 7) {
        options.migrateOldGlobals = std::string(argv[7]) != "new";
    }
    options.force = true;
    options.unknownTone = UnknownTone::Error;
    options.strict = false;
    options.dryRun = false;

    try {
        ConvertReport report = convertFiles(options);
        std::cout << report.log;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Errore: " << e.what() << "\n";
        return 1;
    }
}
