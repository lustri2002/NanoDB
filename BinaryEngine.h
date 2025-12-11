//
// Created by Alessio Lustri on 03/12/25.
//
#pragma once

#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <optional>
#include <filesystem>
#include <vector>
#include <cstdint> // Per uint32_t
#include "DBEngine.h"

// Usiamo una stringa speciale come tombstone.
// In un sistema binario avanzato useremmo un "flag byte", ma per ora va bene così.
inline static const std::string TOMBSTONE_BIN = "__DELETED_TOMBSTONE__";

class BinaryEngine : public DBEngine<std::string, std::string> {
private:
    std::string filename;
    std::unordered_map<std::string, std::streampos> indexMap;
    std::ofstream outfile;

    // --- Helper per scrivere/leggere interi (4 byte) ---
    void writeInt(std::ostream& os, uint32_t val) const {
        os.write(reinterpret_cast<const char*>(&val), sizeof(val));
    }

    bool readInt(std::istream& is, uint32_t& val) const {
        is.read(reinterpret_cast<char*>(&val), sizeof(val));
        return is.good();
    }

    // --- Helper per scrivere stringhe in formato [LEN][DATA] ---
    void writeString(std::ostream& os, const std::string& str) const {
        uint32_t len = static_cast<uint32_t>(str.size());
        writeInt(os, len);
        os.write(str.data(), len);
    }

    // --- Helper per leggere stringhe ---
    bool readString(std::istream& is, std::string& str) const {
        uint32_t len = 0;
        if (!readInt(is, len)) return false;

        str.resize(len);
        if (len > 0) {
            is.read(&str[0], len);
        }
        return is.good();
    }

public:
    BinaryEngine(const std::string& file) : filename(file + ".bin") {
        // 1. CARICAMENTO (Recovery)
        std::ifstream Fin(filename, std::ios::binary);
        if(Fin.is_open()) {
            // Loop di lettura sequenziale
            while (Fin.peek() != EOF) {
                std::streampos currentPos = Fin.tellg();

                std::string key, val;
                // Leggiamo la struttura [Len][Key][Len][Val]
                if (!readString(Fin, key) || !readString(Fin, val)) {
                    break; // File finito o corrotto
                }

                if (val == TOMBSTONE_BIN) {
                    indexMap.erase(key);
                } else {
                    indexMap[key] = currentPos;
                }
            }
            Fin.close();
            std::cout << "[DB] Loaded " << indexMap.size() << " keys (Binary Mode).\n";
        }

        // 2. APERTURA SCRITTURA (Append + Binary)
        outfile.open(filename, std::ios::app | std::ios::binary);
        if (!outfile.is_open()) {
            std::cerr << "[CRITICAL] Cannot open file: " << filename << "\n";
        }
    }

    void put(const std::string& key, const std::string& value) override {
        if (!outfile.is_open()) return;

        outfile.seekp(0, std::ios::end);
        std::streampos pos = outfile.tellp();

        writeString(outfile, key);
        writeString(outfile, value);
        outfile.flush();

        indexMap[key] = pos;
    }

    std::optional<std::string> get(const std::string& key) const override {
        auto it = indexMap.find(key);
        if (it == indexMap.end()) return std::nullopt;

        std::ifstream inFile(filename, std::ios::binary);
        if (!inFile.is_open()) return std::nullopt;

        // Saltiamo all'offset del record
        inFile.seekg(it->second);

        std::string readKey, readVal;

        // Leggiamo Key e Value
        // (Dobbiamo leggere la Key anche se la sappiamo già, per spostare il cursore avanti)
        if (readString(inFile, readKey) && readString(inFile, readVal)) {
            if (readKey == key) {
                return readVal;
            }
        }

        return std::nullopt;
    }

    bool remove(const std::string &key) override {
        if (indexMap.find(key) == indexMap.end()) return false;

        if (outfile.is_open()) {
            outfile.seekp(0, std::ios::end);

            // Scriviamo [Len][Key][Len][TOMBSTONE_BIN]
            writeString(outfile, key);
            writeString(outfile, TOMBSTONE_BIN);
            outfile.flush();

            indexMap.erase(key);
            return true;
        }
        return false;
    }

    void compact() {
        std::cout << "[Compact] Starting binary compaction...\n";
        outfile.close();

        std::string tempFileName = filename + ".tmp";
        std::ofstream tmpDB(tempFileName, std::ios::binary);
        std::unordered_map<std::string, std::streampos> tmpMap;

        // Iteriamo le chiavi vive
        for (const auto& pair : indexMap) {
            std::optional<std::string> val = get(pair.first);
            if (val.has_value()) {
                std::streampos newPos = tmpDB.tellp();

                // Usiamo gli helper interni per scrivere nel temp
                writeString(tmpDB, pair.first);
                writeString(tmpDB, val.value());

                tmpMap[pair.first] = newPos;
            }
        }
        tmpDB.close();

        namespace fs = std::filesystem;
        try {
            fs::rename(tempFileName, filename);
            indexMap = tmpMap;
            std::cout << "[Compact] Done.\n";
        } catch (const fs::filesystem_error & e) {
            std::cerr << "[Error] Rename failed: " << e.what() << "\n";
        }

        outfile.open(filename, std::ios::app | std::ios::binary);
    }

    ~BinaryEngine() {
        if(outfile.is_open()) outfile.close();
    }
};