//
// Created by Alessio Lustri on 11/12/25.
//

#include <iostream>
#include <sstream>
#include <unistd.h>
#include <sys/socket.h> // Contiene la funzione socket()
#include <netinet/in.h> // Contiene le costanti come AF_INET

#include "BinaryEngine.h"

inline static const std::string EXIT = "exit";

void sendResponse(int socket, const std::string& msg) {
    std::string packet = msg + "\n";
    send(socket, packet.c_str(), packet.size(), 0);
}

void sendPrompt(int cln, std::string& currDBname) {
    std::string prompt = "NanoDB [" + currDBname + "]> ";
    send(cln, prompt.c_str(), prompt.size(), 0);
};
static void printHelp(int cln) {
    std::string str = "";
    str += "Comandi disponibili:\n";
    str += "  put <chiave> <valore>  -> Salva un dato\n";
    str += "  get <chiave>           -> Legge un dato\n";
    str += "  del <chiave>           -> Cancella un dato\n";
    str += "  compact                -> Ottimizza il database\n";
    str += "  exit                   -> Chiude il programma\n";
    str += "  use                    -> Cambia database\n";
    str += "-----------------------------------------------------\n";
    sendResponse(cln, str);
}



void Server() {
    std::string currentDBName = "default";
    std::unique_ptr<BinaryEngine> db = std::make_unique<BinaryEngine>(currentDBName);
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd < 0) {
        std::cerr << "Errore creazione socket\n";
        return;
    }
    std::cout << "[Server] Socket creata con successo! ID: " << server_fd << "\n";

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Errore bind\n";
        return;
    }

    if (listen(server_fd, 3) < 0) {
        std::cerr << "Error listen\n";
        return;
    }
    std::cout << "[Server] Server in ascolto...\n";
    while (true) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);

        if (client_fd < 0) {
            std::cerr << "Error connection\n";
            continue;
        };
        std::cout << "[Server] Nuova connessione creata con successo! ID: " << client_fd << "\n";
        sendResponse(client_fd, "Welcome in NanoDB");
        char buff[4096];
        while (true) {
            sendPrompt(client_fd, currentDBName);
            memset(buff, 0, sizeof(buff));
            int bytes_read = read(client_fd, buff, sizeof(buff));
            if (bytes_read <= 0) {
                goto closure;
            }

            std::string line(buff);
            if (!line.empty() && line.back() == '\n') line.pop_back();
            if (!line.empty() && line.back() == '\r') line.pop_back();

            if (line.empty()) continue;
            std::stringstream ss(line);
            std::string command;
            std::string key;
            std::string val;

            ss >> command;

            if (command == EXIT) {
                goto closure;
            }
            else if (command == "use"){
                std::string newName;
                ss >> newName;
                if (newName.empty()) {
                    sendResponse(client_fd,"Error: missing required parameter: name");
                } else {
                    db = std::make_unique<BinaryEngine>(newName);
                    currentDBName = newName;
                    sendResponse(client_fd, "Switched to database: " + currentDBName);
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
                if (val.has_value()) sendResponse(client_fd,val.value());
                else sendResponse(client_fd, key + " not found");
            }
            else if(command == "del") {
                ss >> key;
                if (db->remove(key)) sendResponse(client_fd, "removed " + key);
                else sendResponse(client_fd,key + " not found");
            }
            else if(command == "compact") {
                db->compact();
            }
            else if (command == "help") {
                printHelp(client_fd);
            }
            else {
                sendResponse(client_fd,"Unknown command.");
            }
        }

        // Chiudiamo la chiamata con questo client e torniamo ad aspettarne un altro
        closure:
        std::cout << "[Server] Client disconnesso.\n";
        close(client_fd);
    }

    close(server_fd);
}