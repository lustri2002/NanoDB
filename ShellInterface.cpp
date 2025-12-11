//
// Created by Alessio Lustri on 11/12/25.
//

#include "BinaryEngine.h"
#include <sstream>
#include <iostream>
#include <string>
#include <memory>
#include <vector>
#include "ShellInterface.h"
#include "CommandParser.h"

extern "C" {
#include "linenoise.h"
}
inline static const std::string EXIT = "exit";

void shellInterface() {
    std::string currentDBName = "default";
    std::unique_ptr<BinaryEngine> db = std::make_unique<BinaryEngine>(currentDBName);

    // Configurazione Cronologia
    linenoiseHistorySetMaxLen(100);

    char* line_raw;

    // LOOP PRINCIPALE CON LINENOISE
    while ((line_raw = linenoise(("NanoDB [" + currentDBName + "]> ").c_str())) != NULL) {

        std::string line = line_raw;

        // Aggiungi alla cronologia
        if (!line.empty()) {
            linenoiseHistoryAdd(line_raw);
        }
        linenoiseFree(line_raw); // Importante: liberare la memoria C

        if (line.empty()) continue;

        if (ParseString(line,db, currentDBName)) break;
    }
}