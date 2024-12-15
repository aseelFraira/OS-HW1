#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlCHandler(int sig_num)
{
    std::cout << "smash: got ctrl-C\n";
    SmallShell &smash = SmallShell::getInstance();
    std::cout << "debug = " << smash.m_current_process << "\n";

    if (smash.m_current_process != -1)
    {
        if (kill(smash.m_current_process, 0) == -1)
        {
            perror("smash error: process does not exist");
            smash.setCurrFGPID(-1);
            return;
        }

        if (kill(smash.m_current_process, SIGKILL) == -1)
        {
            perror("smash error: kill failed");
            return;
        }

        std::cout << "smash: process " << smash.m_current_process << " was killed\n";
        smash.setCurrFGPID(-1);
    }
}

