#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include "Commands.h"
#include "signals.h"

int main(int argc, char *argv[]) {
    if (signal(SIGINT, ctrlCHandler) == SIG_ERR) {
        perror("smash error: failed to set ctrl-C handler");
    }


    SmallShell &smash = SmallShell::getInstance();
    while (true) {
        std::cout << smash.getPrompt() << "> ";
        std::string cmd_line;
        std::getline(std::cin, cmd_line);
        std::cout<<"pid is "<<smash.m_current_process<<std::endl;
        smash.executeCommand(cmd_line.c_str());
    }
    return 0;
}


