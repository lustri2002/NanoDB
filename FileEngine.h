//
// Created by Alessio Lustri on 29/11/25.
//

#pragma once
#include <fstream>
#include <ios>
#include "DBEngine.h"
#include <string>
#include <optional>
#include  <cstdio>
#include <filesystem>

inline static const std::string TOMBSTONE = "__DELETED_TOMBSTONE__";

class FileEngine : public DBEngine<std::string, std::string> {
private:
    std::string filename;
    std::unordered_map<std::string, std::streampos> indexMap;
    std::ofstream outfile;

public:
    FileEngine(const std::string& file) :filename(file) {
        std::ifstream Fin(filename);
        if(Fin.is_open()) {
            std::string line;
            std::streampos currentPos = Fin.tellg();
            while (std::getline(Fin, line)) {
                size_t delimiterPos = line.find("=");
                if (delimiterPos != std::string::npos) {
                    std::string key = line.substr(0, delimiterPos);
                    if (line.substr(delimiterPos+1) != TOMBSTONE) {
                        indexMap[key] = currentPos;
                    }
                    else {
                        indexMap.erase(key);
                    }
                }
                currentPos = Fin.tellg();
            }
            Fin.close();
        }
        outfile.open(filename, std::ios::app);
        //std::cout << "[IndexMap] Loaded " << indexMap.size() << " keys in RAM.\n";
        if (!outfile.is_open()) {
            std::cerr << "[CRITICAL ERROR] Costruttore: Impossibile creare/aprire il file: " << filename << "\n";
        }

    }

    void compact(){
        outfile.close();
        std::string tempFileName = filename = ".tmp";
        std::ofstream tmpDB(tempFileName);
        std::unordered_map<std::string, std::streampos> tmpMap;

        auto iterator = indexMap.begin();
        while (iterator != indexMap.end()) {
            std::optional<std::string> val = get(iterator->first);
            if (val.has_value()) {
                std::streampos newPos = tmpDB.tellp();
                tmpDB << iterator->first << "=" << val.value() << "\n";
                tmpMap[iterator->first] = newPos;
            }
            ++iterator;
        }
        tmpDB.close();

        namespace fs = std::filesystem;

        try {
            // fs::rename su POSIX (Mac/Linux) è atomico e sovrascrive il file esistente!
            // Non serve nemmeno fare remove prima.
            fs::rename(tempFileName, filename);
        } catch (const fs::filesystem_error & e) {
            std::cerr << "[Critical Error] Filesystem rename failed: " << e.what() << "\n";
            // Tentativo di fallback: Copia e Cancella
            try {
                fs::copy_file(tempFileName, filename, fs::copy_options::overwrite_existing);
                fs::remove(tempFileName);
            } catch (...) {
                return; // Niente da fare, usciamo senza aggiornare indexMap
            }
        }

        indexMap = tmpMap;
        outfile.open(filename, std::ios::app);
    }

    void put(const std::string& key, const std::string& value) override {
        if (indexMap.find(key) != indexMap.end()) {
            //std::cout << "[Warning] Key Violation for key " << key << "\n";
            return;
        }

        if (outfile.is_open()) {
            std::streampos newPos = outfile.tellp();
            outfile << key << "=" << value << "\n";
            //outfile.flush();
            indexMap[key] = newPos;
            //std::cout << "[FileEngine] Written key " << key << "with offset " << newPos;
        } else {
            std::cerr << "[Error] Put fallita: lo stream di scrittura e' chiuso! Chiave persa: " << key << "\n";
        }
    }

    std::optional<std::string> get(const std::string& k) const override {
        auto it = indexMap.find(k);
        if (it == indexMap.end()) return std::nullopt;

        std::ifstream file(filename, std::ios::in);
        if (file.is_open()) {
            file.seekg(it->second);
            std::string line;
            if (std::getline(file, line)) {
                size_t delimiterPos = line.find('=');
                return line.substr(delimiterPos+1);
            }
            file.close();
        }
        return std::nullopt;
    }

    bool remove(const std::string &key) override {
        if (outfile.is_open() && indexMap.find(key) != indexMap.end()) {
            outfile << key << "=" << TOMBSTONE << "\n";
            if (indexMap.erase(key) > 0) {
                std::cout << "[FileEngine] Key '" << key << "' removed. \n";
                return true;
            }
        }
        return false; // return false if the file stream is closed or the key does not exists in the hash map
    }

    ~FileEngine() {
        outfile.close();
    }
};