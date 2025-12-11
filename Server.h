//
// Created by Alessio Lustri on 11/12/25.
//

#ifndef NANODB_SERVER_H
#define NANODB_SERVER_H
static void printHelp();
void sendResponse(int socket, const std::string& msg);
void Server();
#endif //NANODB_SERVER_H