#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlCHandler(int sig_num)
{
    std::cout << "smash: got ctrl-C\n";
    SmallShell &smash = SmallShell::getInstance();
    std::cout << "debug = " << smash.m_current_process<<"\n";
    if (smash.m_current_process != -1)
    {
        if (kill(smash.m_current_process, SIGINT) == -1)
        {
            perror("smash error: kill failed");
            return;
        }
        cout << "smash: process " << smash.m_current_process << " was killed\n";
        smash.setCurrFGPID(-1);
    }
}

