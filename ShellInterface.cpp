//
// Created by Alessio Lustri on 11/12/25.
//

#include "BinaryEngine.h"
#include <sstream>
#include <iostream>
#include <string>
#include "ShellInterface.h"

inline static const std::string EXIT = "exit";

static void printHelp() {
    std::cout << "Comandi disponibili:\n";
    std::cout << "  put <chiave> <valore>  -> Salva un dato\n";
    std::cout << "  get <chiave>           -> Legge un dato\n";
    std::cout << "  del <chiave>           -> Cancella un dato\n";
    std::cout << "  compact                -> Ottimizza il database\n";
    std::cout << "  exit                   -> Chiude il programma\n";
    std::cout << "  use                    -> Cambia database\n";
    std::cout << "--------------------------------------\n";
}

static void printPrompt(const std::string& dbName) {
    std::cout << "NanoDB[" << dbName << "]>";
}

void shellInterface() {
    std::string currentDBname = "default";
    std::unique_ptr<BinaryEngine> db = std::make_unique<BinaryEngine>(currentDBname);
    std::string line;
    while (true) {
        printPrompt(currentDBname);

        std::getline(std::cin, line);
        std::stringstream ss(line);
        std::string command;
        std::string key;
        std::string val;
        ss >> command;

        //std::cout << "hai digitato: " << command << std::endl;
        if (command == EXIT) {
            break;
        }
        else if (command == "use"){
            std::string newName;
            ss >> newName;
            if (newName.empty()) {
                std::cout << "Error: missing required parameter: name";
            }else {
                db = std::make_unique<BinaryEngine>(newName);
                currentDBname = newName;
                std::cout << "Switched to database: " << currentDBname << std::endl;
            }
        }
        else if (command == "put") {
            ss >> key;
            std::getline(ss, val);
            if (!val.empty() && val[0] == ' ') val.erase(0, 1);
            db->put(key, val);
        }
        else if(command == "get") {
            ss >> key;
            std::optional<std::string> val = db->get(key);
            if (val.has_value()) std::cout << val.value() << std::endl;
            else std::cout << key << " not found" << std::endl;
        }
        else if(command == "del") {
            ss >> key;
            if (db->remove(key)) std::cout << "removed " << key << std::endl;
            else std::cout << key << " not found" << std::endl;
        }
        else if(command == "compact") {
            db->compact();
        }
        else if (command == "help") {
            printHelp();
        }
        else {
            std::cout << "unkown command. \n";
        }
    }
}