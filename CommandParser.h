//
// Created by Alessio Lustri on 11/12/25.
//

#ifndef NANODB_COMMANDPARSER_H
#define NANODB_COMMANDPARSER_H
#include <string>

#include "BinaryEngine.h"

static void printHelp();
bool ParseString(std::string line, std::unique_ptr<BinaryEngine>& db, std::string& currentDBName);
#endif //NANODB_COMMANDPARSER_H