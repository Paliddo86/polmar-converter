#pragma once

#include <cstddef>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

enum class Mode {
    MergeFull,
    ReplaceAll,
    FreqOnly
};

enum class DecimalMode {
    Auto,
    Dot,
    Comma
};

enum class UnknownTone {
    Error,
    Off
};

struct ConvertOptions {
    std::filesystem::path oldPath;
    std::filesystem::path newPath;
    std::filesystem::path outputPath;
    Mode mode = Mode::MergeFull;
    DecimalMode decimal = DecimalMode::Auto;
    UnknownTone unknownTone = UnknownTone::Error;
    // -1 = mantieni il valore del nuovo file; 0 = Off; 1..9 = livello.
    int sqLevel = 1;
    // Migra le impostazioni globali old con equivalente nella nuova scheda.
    bool migrateOldGlobals = true;
    bool force = false;
    bool strict = false;
    bool dryRun = false;
};

struct ConvertReport {
    int populated = 0;
    int empty = 0;
    int replaced = 0;
    int preserved = 0;
    std::string separator = ".";
    std::string settings;
    std::map<std::string, int> toneCounts;
    std::vector<std::string> warnings;
    std::size_t outputBytes = 0;
    std::string log;
};

ConvertReport convertFiles(const ConvertOptions& options);
