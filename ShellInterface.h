//
// Created by Alessio Lustri on 11/12/25.
//
#pragma once
#include <string>

#ifndef NANODB_SHELLINTERFACE_H
#define NANODB_SHELLINTERFACE_H
static void printPrompt(const std::string& dbName);
static void printHelp();
void shellInterface();
#endif //NANODB_SHELLINTERFACE_H