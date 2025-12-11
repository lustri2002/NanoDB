#include "ShellInterface.h"

#include <iostream>
#include "Server.h"

void printASCII() {
    std::cout << R"(
        ███   ███                          ██████   ██████
        ████  ███   ██████  ███  ███  ███  ██   ██  ██   ██
        ██ ██ ███  ██    ██ ████ ███ ██ ██ ██   ██  ██████
        ██  █████  ████████ ██  ████ ██ ██ ██   ██  ██   ██
        ██   ████  ██    ██ ██   ███  ███  ██████   ██████
            )" << std::endl;
}

int main() {
    printASCII();
    //shellInterface();
    Server();
    return 0;
}