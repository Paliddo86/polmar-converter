#include "polmar_core.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <locale>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

std::string pathLabel(const std::filesystem::path& p) {
    auto u8 = p.u8string();
    return std::string(u8.begin(), u8.end());
}

std::vector<uint8_t> readFile(const std::filesystem::path& path) {
#ifdef _WIN32
    FILE* f = _wfopen(path.wstring().c_str(), L"rb");
    if (!f) {
        throw std::runtime_error("impossibile leggere " + pathLabel(path));
    }
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        throw std::runtime_error("impossibile leggere " + pathLabel(path));
    }
    long size = ftell(f);
    if (size < 0) {
        fclose(f);
        throw std::runtime_error("impossibile leggere " + pathLabel(path));
    }
    rewind(f);
    std::vector<uint8_t> data(static_cast<std::size_t>(size));
    if (size > 0 && fread(data.data(), 1, data.size(), f) != data.size()) {
        fclose(f);
        throw std::runtime_error("lettura incompleta di " + pathLabel(path));
    }
    fclose(f);
    return data;
#else
    std::ifstream f(path, std::ios::binary);
    if (!f) {
        throw std::runtime_error("impossibile leggere " + pathLabel(path));
    }
    f.seekg(0, std::ios::end);
    std::streamoff size = f.tellg();
    if (size < 0) {
        throw std::runtime_error("impossibile leggere " + pathLabel(path));
    }
    f.seekg(0, std::ios::beg);
    std::vector<uint8_t> data(static_cast<std::size_t>(size));
    if (size > 0) {
        f.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(size));
    }
    if (!f && size > 0) {
        throw std::runtime_error("lettura incompleta di " + pathLabel(path));
    }
    return data;
#endif
}

void writeFile(const std::filesystem::path& path, const std::vector<uint8_t>& data) {
#ifdef _WIN32
    FILE* f = _wfopen(path.wstring().c_str(), L"wb");
    if (!f) {
        throw std::runtime_error("impossibile scrivere " + pathLabel(path));
    }
    if (!data.empty() && fwrite(data.data(), 1, data.size(), f) != data.size()) {
        fclose(f);
        throw std::runtime_error("scrittura incompleta di " + pathLabel(path));
    }
    fclose(f);
#else
    std::ofstream f(path, std::ios::binary);
    if (!f) {
        throw std::runtime_error("impossibile scrivere " + pathLabel(path));
    }
    if (!data.empty()) {
        f.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
    }
    if (!f) {
        throw std::runtime_error("scrittura incompleta di " + pathLabel(path));
    }
#endif
}

std::string formatFrequency(double mhz, const std::string& separator) {
    std::ostringstream os;
    os.imbue(std::locale::classic());
    os << std::fixed << std::setprecision(5) << mhz;
    std::string s = os.str();
    if (separator == ",") {
        std::replace(s.begin(), s.end(), '.', ',');
    }
    return s;
}

std::string formatTone(double value) {
    std::ostringstream os;
    os.imbue(std::locale::classic());
    os << std::fixed << std::setprecision(1) << value;
    return os.str();
}

std::optional<int> decodeBCD(const std::vector<uint8_t>& bytes) {
    if (bytes.empty()) {
        return std::nullopt;
    }
    std::string digits;
    digits.reserve(bytes.size() * 2);
    for (auto it = bytes.rbegin(); it != bytes.rend(); ++it) {
        int hi = (*it >> 4) & 0x0F;
        int lo = *it & 0x0F;
        if (hi > 9 || lo > 9) {
            return std::nullopt;
        }
        digits.push_back(static_cast<char>('0' + hi));
        digits.push_back(static_cast<char>('0' + lo));
    }
    try {
        return std::stoi(digits);
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<int> decodeBCD12(int value) {
    int v = value & 0x0FFF;
    int h = (v >> 8) & 0xF;
    int m = (v >> 4) & 0xF;
    int l = v & 0xF;
    if (h > 9 || m > 9 || l > 9) {
        return std::nullopt;
    }
    return h * 100 + m * 10 + l;
}

std::vector<uint8_t> encode7Bit(std::size_t value) {
    std::vector<uint8_t> out;
    std::size_t v = value;
    do {
        uint8_t b = static_cast<uint8_t>(v & 0x7F);
        v >>= 7;
        if (v != 0) {
            b = static_cast<uint8_t>(b | 0x80);
        }
        out.push_back(b);
    } while (v != 0);
    return out;
}

std::vector<std::string> split(const std::string& value, const std::string& delimiter) {
    std::vector<std::string> out;
    std::size_t start = 0;
    while (true) {
        std::size_t pos = value.find(delimiter, start);
        if (pos == std::string::npos) {
            out.push_back(value.substr(start));
            break;
        }
        out.push_back(value.substr(start, pos - start));
        start = pos + delimiter.size();
    }
    return out;
}

std::string modeName(Mode mode) {
    switch (mode) {
        case Mode::MergeFull: return "merge-full";
        case Mode::ReplaceAll: return "replace-all";
        case Mode::FreqOnly: return "freq-only";
    }
    return "merge-full";
}

std::vector<std::string> settingValues(const std::string& suffix) {
    if (suffix.empty() || suffix.front() != '~') {
        return {};
    }
    return split(suffix.substr(1), "$");
}

std::string overrideSetting(const std::string& suffix, int index, const std::string& value) {
    if (suffix.empty() || suffix.front() != '~') {
        return suffix;
    }
    std::vector<std::string> parts = split(suffix.substr(1), "$");
    if (index >= 0 && index < static_cast<int>(parts.size())) {
        parts[index] = value;
    }
    std::string out = "~";
    for (std::size_t i = 0; i < parts.size(); ++i) {
        if (i != 0) {
            out += "$";
        }
        out += parts[i];
    }
    return out;
}

std::string applyOldGlobals(const std::string& suffix, std::vector<std::string>& warnings) {
    std::string out = suffix;
    out = overrideSetting(out, 0, "0");   // fmsel = Manual
    out = overrideSetting(out, 2, "0");   // keybeep = Off
    out = overrideSetting(out, 3, "1");   // scanmode = CO (Carry)
    out = overrideSetting(out, 5, "1");   // tot = 15S (il 5S old non esiste)
    out = overrideSetting(out, 7, "1");   // powersave = On
    out = overrideSetting(out, 8, "0");   // voxlevel = Off
    out = overrideSetting(out, 11, "0");  // light = Off
    warnings.push_back(
        "Nota: impostazioni globali migrate dal profilo old (FM Sel Manual, Key Beep Off, "
        "Battery Save On, Scan Mode Carry→CO, TOT 5s→15s, VOX Off, LIGHT Off).");
    warnings.push_back(
        "Nota: Scan Incept Delay, Key Lock, Dual Bander, VOX Delay, Allow cancel e "
        "Power Grades non hanno equivalente e restano dal nuovo file.");
    return out;
}

struct OldChannel {
    int index = 0;
    bool empty = true;
    double rxMHz = 0.0;
    double txMHz = 0.0;
    std::string rxTone = "Off";
    std::string txTone = "Off";
    std::string power = "High";
    std::string band = "Wide";
    std::string busyLock = "Off";
};

struct NewFile {
    std::vector<uint8_t> header;
    std::vector<uint8_t> objectId;
    std::string tablePrefix;
    std::vector<std::vector<std::string>> rows;
    std::string settingsSuffix;
    std::string separator;
};

std::string decodeTone(const std::vector<uint8_t>& bytes,
                       int channel,
                       const std::string& label,
                       UnknownTone unknown,
                       std::vector<std::string>& warnings) {
    int code = static_cast<int>(bytes[0]) | (static_cast<int>(bytes[1]) << 8);
    if (code == 0xFFFF) {
        return "Off";
    }

    if ((code & 0xC000) == 0xC000) {
        auto n = decodeBCD12(code & 0x0FFF);
        if (n) {
            std::ostringstream os;
            os << "D" << std::setw(3) << std::setfill('0') << *n << "I";
            return os.str();
        }
    }
    if ((code & 0x8000) == 0x8000 && (code & 0x4000) == 0) {
        auto n = decodeBCD12(code & 0x0FFF);
        if (n) {
            std::ostringstream os;
            os << "D" << std::setw(3) << std::setfill('0') << *n << "N";
            return os.str();
        }
    }

    if (auto n = decodeBCD(bytes); n && *n >= 600 && *n <= 3000) {
        return formatTone(static_cast<double>(*n) / 10.0);
    }

    char hex[16];
    std::snprintf(hex, sizeof(hex), "%02X %02X", bytes[0], bytes[1]);
    if (unknown == UnknownTone::Off) {
        warnings.push_back("Attenzione: canale " + std::to_string(channel) + " " + label +
                           ": tono sconosciuto " + hex + ", impostato Off");
        return "Off";
    }
    throw std::runtime_error("canale " + std::to_string(channel) + " " + label +
                             ": tono sconosciuto " + hex);
}

std::vector<OldChannel> parseOldFile(const std::filesystem::path& path,
                                     UnknownTone unknown,
                                     bool strict,
                                     std::vector<std::string>& warnings) {
    std::vector<uint8_t> bytes = readFile(path);
    if (bytes.size() != 2096) {
        throw std::runtime_error(pathLabel(path) + ": dimensione " + std::to_string(bytes.size()) +
                                 " byte, attesa 2096");
    }

    std::string header(bytes.begin(), bytes.begin() + 6);
    if (header != "KT-680") {
        throw std::runtime_error(pathLabel(path) + ": header '" + header +
                                 "' non riconosciuto (atteso KT-680)");
    }

    std::vector<OldChannel> channels;
    channels.reserve(128);

    for (int ch = 1; ch <= 128; ++ch) {
        std::size_t off = 0x30 + static_cast<std::size_t>(ch - 1) * 13;
        std::vector<uint8_t> rec(bytes.begin() + off, bytes.begin() + off + 13);

        if (std::all_of(rec.begin(), rec.end(), [](uint8_t b) { return b == 0xFF; })) {
            OldChannel c;
            c.index = ch;
            c.empty = true;
            channels.push_back(c);
            continue;
        }

        auto rxInt = decodeBCD({rec[0], rec[1], rec[2], rec[3]});
        auto txInt = decodeBCD({rec[4], rec[5], rec[6], rec[7]});
        if (!rxInt || !txInt) {
            std::ostringstream raw;
            raw << std::hex << std::uppercase << std::setfill('0');
            for (int i = 0; i < 8; ++i) {
                if (i) raw << ' ';
                raw << std::setw(2) << static_cast<int>(rec[i]);
            }
            throw std::runtime_error("canale " + std::to_string(ch) +
                                     ": frequenza BCD non valida (" + raw.str() + ")");
        }

        double rx = static_cast<double>(*rxInt) / 100000.0;
        double tx = static_cast<double>(*txInt) / 100000.0;
        if (rx < 400.0 || rx > 470.0 || tx < 400.0 || tx > 470.0) {
            std::string msg = "canale " + std::to_string(ch) +
                              ": frequenza fuori range 400-470 MHz (" +
                              formatFrequency(rx, ".") + ", " + formatFrequency(tx, ".") + ")";
            if (strict) {
                throw std::runtime_error(msg);
            }
            warnings.push_back("Attenzione: " + msg);
        }

        OldChannel c;
        c.index = ch;
        c.empty = false;
        c.rxMHz = rx;
        c.txMHz = tx;
        c.rxTone = decodeTone({rec[8], rec[9]}, ch, "RX", unknown, warnings);
        c.txTone = decodeTone({rec[10], rec[11]}, ch, "TX", unknown, warnings);
        uint8_t flag = rec[12];
        c.band = (flag & 0x10) == 0 ? "Narrow" : "Wide";
        c.busyLock = (flag & 0x04) == 0 ? "On" : "Off";
        c.power = "High";
        channels.push_back(c);
    }

    return channels;
}

NewFile parseNewData(const std::vector<uint8_t>& bytes, DecimalMode decimal, const std::string& label) {
    if (bytes.size() < 25) {
        throw std::runtime_error(label + ": file troppo corto");
    }
    if (bytes[17] != 0x06) {
        char hex[8];
        std::snprintf(hex, sizeof(hex), "%02X", bytes[17]);
        throw std::runtime_error(label + ": record BinaryFormatter non riconosciuto (byte 17 = " +
                                 std::string(hex) + ")");
    }

    NewFile result;
    result.objectId.assign(bytes.begin() + 18, bytes.begin() + 22);

    std::size_t idx = 22;
    std::size_t length = 0;
    int shift = 0;
    while (true) {
        if (idx >= bytes.size()) {
            throw std::runtime_error(label + ": lunghezza stringa troncata");
        }
        uint8_t b = bytes[idx++];
        length |= static_cast<std::size_t>(b & 0x7F) << shift;
        if ((b & 0x80) == 0) {
            break;
        }
        shift += 7;
        if (shift > 28) {
            throw std::runtime_error(label + ": lunghezza 7-bit non valida");
        }
    }

    if (idx + length > bytes.size()) {
        throw std::runtime_error(label + ": stringa troncata");
    }
    std::string s(bytes.begin() + idx, bytes.begin() + idx + length);
    if (idx + length < bytes.size() && bytes[idx + length] != 0x0B) {
        throw std::runtime_error(label + ": terminatore BinaryFormatter mancante");
    }

    const std::string marker = "Walkieinte~";
    if (s.compare(0, marker.size(), marker) != 0) {
        throw std::runtime_error(label + ": marcatore Walkieinte~ mancante");
    }

    std::size_t at = s.find("@&@", marker.size());
    if (at == std::string::npos) {
        throw std::runtime_error(label + ": separatore @&@ mancante");
    }
    result.tablePrefix = s.substr(marker.size(), at + 3 - marker.size());

    std::size_t last = s.rfind('~');
    if (last == std::string::npos || last <= at + 3) {
        throw std::runtime_error(label + ": separatore ~ delle impostazioni mancante");
    }
    std::string rowsString = s.substr(at + 3, last - (at + 3));
    result.settingsSuffix = s.substr(last);

    std::vector<std::string> rawRows = split(rowsString, "#$%");
    if (rawRows.size() != 128) {
        throw std::runtime_error(label + ": " + std::to_string(rawRows.size()) + " righe, attese 128");
    }
    result.rows.reserve(128);
    for (const auto& raw : rawRows) {
        std::vector<std::string> fields = split(raw, "^&*");
        if (fields.size() != 8) {
            throw std::runtime_error(label + ": una o più righe non hanno 8 campi");
        }
        result.rows.push_back(std::move(fields));
    }

    const std::vector<std::string> expected = {
        "ch", "rxfreq", "rxctc", "txfreq", "txctc", "txpower", "band", "busylock"
    };
    for (const auto& name : expected) {
        if (result.tablePrefix.find("<xs:element name=\"" + name + "\"") == std::string::npos) {
            throw std::runtime_error(label + ": schema senza la colonna " + name);
        }
    }

    result.separator = ".";
    if (decimal == DecimalMode::Comma) {
        result.separator = ",";
    } else if (decimal == DecimalMode::Dot) {
        result.separator = ".";
    } else {
        for (const auto& row : result.rows) {
            if (!row[1].empty()) {
                result.separator = row[1].find(',') != std::string::npos ? "," : ".";
                break;
            }
        }
    }

    result.header.assign(bytes.begin(), bytes.begin() + 17);
    return result;
}

NewFile parseNewFile(const std::filesystem::path& path, DecimalMode decimal) {
    return parseNewData(readFile(path), decimal, pathLabel(path));
}

struct MergeResult {
    std::vector<std::vector<std::string>> rows;
    int replaced = 0;
    int preserved = 0;
};

std::vector<std::string> oldRow(const OldChannel& ch, const std::string& separator) {
    return {
        std::to_string(ch.index),
        formatFrequency(ch.rxMHz, separator),
        ch.rxTone,
        formatFrequency(ch.txMHz, separator),
        ch.txTone,
        ch.power,
        ch.band,
        ch.busyLock
    };
}

std::vector<std::string> emptyRow(int index) {
    return {std::to_string(index), "", "", "", "", "", "", ""};
}

MergeResult mergeChannels(const std::vector<OldChannel>& old,
                          const NewFile& newFile,
                          Mode mode) {
    MergeResult result;
    result.rows.reserve(128);

    for (std::size_t i = 0; i < 128; ++i) {
        const OldChannel& o = old[i];
        const std::vector<std::string>& n = newFile.rows[i];
        std::vector<std::string> row;

        switch (mode) {
            case Mode::MergeFull:
                if (o.empty) {
                    row = n;
                    ++result.preserved;
                } else {
                    row = oldRow(o, newFile.separator);
                    ++result.replaced;
                }
                break;

            case Mode::ReplaceAll:
                if (o.empty) {
                    row = emptyRow(static_cast<int>(i + 1));
                } else {
                    row = oldRow(o, newFile.separator);
                    ++result.replaced;
                }
                break;

            case Mode::FreqOnly:
                if (o.empty) {
                    row = n;
                    ++result.preserved;
                } else {
                    row = n;
                    row[0] = std::to_string(i + 1);
                    row[1] = formatFrequency(o.rxMHz, newFile.separator);
                    row[3] = formatFrequency(o.txMHz, newFile.separator);
                    if (row[2].empty() && row[4].empty() && row[5].empty() &&
                        row[6].empty() && row[7].empty()) {
                        row[2] = "Off";
                        row[4] = "Off";
                        row[5] = "Low";
                        row[6] = "Narrow";
                        row[7] = "Off";
                    }
                    ++result.replaced;
                }
                break;
        }

        result.rows.push_back(std::move(row));
    }

    return result;
}

std::vector<uint8_t> buildNewData(const NewFile& newFile,
                                  const std::vector<std::vector<std::string>>& rows,
                                  std::string& outString) {
    std::string rowsString;
    for (std::size_t i = 0; i < rows.size(); ++i) {
        if (i != 0) {
            rowsString += "#$%";
        }
        for (std::size_t j = 0; j < rows[i].size(); ++j) {
            if (j != 0) {
                rowsString += "^&*";
            }
            rowsString += rows[i][j];
        }
    }

    outString = "Walkieinte~" + newFile.tablePrefix + rowsString + newFile.settingsSuffix;

    std::vector<uint8_t> out = newFile.header;
    out.push_back(0x06);
    out.insert(out.end(), newFile.objectId.begin(), newFile.objectId.end());
    std::vector<uint8_t> length = encode7Bit(outString.size());
    out.insert(out.end(), length.begin(), length.end());
    out.insert(out.end(), outString.begin(), outString.end());
    out.push_back(0x0B);
    return out;
}

void validateOutput(const std::vector<uint8_t>& data,
                    const std::string& expectedSettings,
                    const std::string& expectedPrefix) {
    NewFile parsed = parseNewData(data, DecimalMode::Auto, "output");
    if (parsed.settingsSuffix != expectedSettings) {
        throw std::runtime_error("output: impostazioni globali non preservate");
    }
    if (parsed.tablePrefix != expectedPrefix) {
        throw std::runtime_error("output: schema XML non preservato");
    }
    if (parsed.rows.size() != 128) {
        throw std::runtime_error("output: numero righe non valido");
    }
}

} // namespace

ConvertReport convertFiles(const ConvertOptions& options) {
    ConvertReport report;

    std::vector<OldChannel> old = parseOldFile(options.oldPath, options.unknownTone,
                                               options.strict, report.warnings);
    NewFile newFile = parseNewFile(options.newPath, options.decimal);

    if (options.migrateOldGlobals) {
        newFile.settingsSuffix = applyOldGlobals(newFile.settingsSuffix, report.warnings);
    }

    if (options.sqLevel >= 0) {
        if (options.sqLevel > 9) {
            throw std::runtime_error("SQ Level non valido (0..9)");
        }
        newFile.settingsSuffix = overrideSetting(newFile.settingsSuffix, 1,
                                                 std::to_string(options.sqLevel));
    }

    MergeResult merged = mergeChannels(old, newFile, options.mode);

    std::string outString;
    std::vector<uint8_t> data = buildNewData(newFile, merged.rows, outString);
    validateOutput(data, newFile.settingsSuffix, newFile.tablePrefix);

    report.populated = 0;
    for (const auto& ch : old) {
        if (!ch.empty) {
            ++report.populated;
            report.toneCounts[ch.rxTone]++;
            if (ch.txTone != ch.rxTone) {
                report.toneCounts[ch.txTone]++;
            }
        }
    }
    report.empty = 128 - report.populated;
    report.replaced = merged.replaced;
    report.preserved = merged.preserved;
    report.separator = newFile.separator;
    report.settings = newFile.settingsSuffix;
    report.outputBytes = data.size();

    std::vector<std::string> settingVals = settingValues(newFile.settingsSuffix);
    std::string sqValue = settingVals.size() > 1 ? settingVals[1] : "?";
    bool hasToneOff = false;
    for (const auto& ch : old) {
        if (!ch.empty && ch.rxTone == "Off") {
            hasToneOff = true;
            break;
        }
    }
    if (sqValue == "0" && hasToneOff) {
        report.warnings.push_back(
            "Attenzione: SQ Level = Off e ci sono canali senza tono; la nuova radio "
            "può avere fruscio continuo. Imposta SQ Level a 1 o superiore.");
    }

    std::string settings = report.settings;
    if (!settings.empty() && settings.front() == '~') {
        settings.erase(settings.begin());
    }

    std::ostringstream tones;
    bool firstTone = true;
    for (const auto& entry : report.toneCounts) {
        if (!firstTone) {
            tones << ", ";
        }
        firstTone = false;
        tones << entry.first << "=" << entry.second;
    }

    std::ostringstream log;
    log << "Vecchio: " << pathLabel(options.oldPath) << "\n";
    log << "Nuovo/template: " << pathLabel(options.newPath) << "\n";
    log << "Canali old: " << report.populated << " popolati, " << report.empty << " vuoti\n";
    log << "Toni: " << tones.str() << "\n";
    log << "Modalità: " << modeName(options.mode) << "\n";
    log << "Separatore decimale: " << (report.separator == "," ? "virgola" : "punto") << "\n";
    log << "Righe sostituite: " << report.replaced
        << ", righe preservate: " << report.preserved << "\n";
    log << "Impostazioni globali: "
        << (options.migrateOldGlobals ? "migrate dal vecchio file" : "dal nuovo file")
        << "\n";
    log << "Trailer impostazioni: " << settings << "\n";
    log << "SQ Level: " << sqValue
        << (options.sqLevel >= 0 ? " (forzato)" : " (dal nuovo file)") << "\n";

    for (const auto& warning : report.warnings) {
        log << warning << "\n";
    }

    if (options.dryRun) {
        log << "Dry-run: nessun file scritto\n";
        report.log = log.str();
        return report;
    }

    if (std::filesystem::exists(options.outputPath) && !options.force) {
        throw std::runtime_error("output già esistente: " + pathLabel(options.outputPath) +
                                 " (usa Sovrascrivi)");
    }

    writeFile(options.outputPath, data);
    log << "Output: " << pathLabel(options.outputPath) << " (" << data.size() << " byte)\n";
    report.log = log.str();
    return report;
}




