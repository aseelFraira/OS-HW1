#include <unistd.h>
#include <string>
#include <cstring>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <algorithm>




using namespace std;

const std::string WHITESPACE = " \n\r\t\f\v";
const std::string DEFAULT_PROMPT = "smash";
#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

string _ltrim(const std::string &s) {
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string &s) {
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string &s) {
    return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char *cmd_line, char **args) {
    FUNC_ENTRY()
    int i = 0;
    std::istringstream iss(_trim(string(cmd_line)).c_str());
    for (std::string s; iss >> s;) {
        args[i] = (char *) malloc(s.length() + 1);
        memset(args[i], 0, s.length() + 1);
        strcpy(args[i], s.c_str());
        args[++i] = NULL;
    }
    return i;

    FUNC_EXIT()
}

bool _isBackgroundComamnd(const char *cmd_line) {
    const string str(cmd_line);
    return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char *cmd_line) {
    const string str(cmd_line);
    // find last character other than spaces
    unsigned int idx = str.find_last_not_of(WHITESPACE);
    // if all characters are spaces then return
    if (idx == string::npos) {
        return;
    }
    // if the command line does not end with & then return
    if (cmd_line[idx] != '&') {
        return;
    }
    // replace the & (background sign) with space and then remove all tailing spaces.
    cmd_line[idx] = ' ';
    // truncate the command line string up to the last non-space character
    cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}
//////////////////////////MY HELPER FUNCTIONS//////////////////////////////////
std::vector<std::string> getArgs(const char *cmd_line) {
    std::vector<std::string> argsVec;
    char **args = (char **) malloc(COMMAND_MAX_ARGS * sizeof(char *));
    for (int i = 0; i < COMMAND_MAX_ARGS; i++)
        args[i] = nullptr;

    int num = _parseCommandLine(cmd_line, args);
    for (int i = 0; i < num; i++) {
        argsVec.push_back(args[i]);
        free(args[i]);
    }
    free(args);
    return argsVec;
}

bool checkFormatNumber(const std::string& str) {
    int i  = 0;
    while (str[i]) {
        if (!isdigit(str[i])) {
            return false;
        }
        i++;
    }
    return true;
}

////////////////////////////////////////////////////////////////////////////////
Command::Command(const char *cmd_line):m_cmd_line(std::string(cmd_line)){
    std::string cpy = std::string(cmd_line);
    size_t firstSpacePos = cpy.find(' ');
    if (firstSpacePos == std::string::npos) { //Handle the case only one string!
        m_command = cpy;
    }else{
        m_command =cpy.substr(0, firstSpacePos);
    }
}

std::string Command::getCommand() {
    return m_command;
}

BuiltInCommand::BuiltInCommand(const char *cmd_line): Command(cmd_line){
    int len = strlen(cmd_line);
    char* cpy = (char*) malloc(sizeof(char) * (len + 1));
    strcpy(cpy, cmd_line);
    _removeBackgroundSign(cpy);
    m_args = getArgs(cpy);
}

// should check this
std::string Command::m_remove_background_char(const char *cmd_line) const {

    char *cmd_line_copy = new char[strlen(cmd_line) + 1];
    strcpy(cmd_line_copy, cmd_line);
    _removeBackgroundSign(cmd_line_copy);

    std::string modified_cmd_line(cmd_line_copy);
    delete[] cmd_line_copy;

    return modified_cmd_line;
}

////////////////////////////////External Commands///////////////////////////////

ExternalCommand::ExternalCommand(const char *cmd_line):Command(cmd_line), m_is_complex(complexity(cmd_line)) {}

bool ExternalCommand::complexity(const char *cmd_line)
{
    for (const char *ptr = cmd_line; *ptr != '\0'; ++ptr) {
        if (*ptr == '*' || *ptr == '?') {
            return true;
        }
    }
    return false;
}

void ExternalCommand::execute() {
    pid_t pid = fork();

    if (pid == -1) {
        perror("smash error: fork failed");
        return;
    }
    if (pid == 0) {
        if (setpgrp() == -1) {
            perror("smash error: setpgrp failed");
            _exit(1);
        }
        if (m_is_complex) { // Complex command = True
            //send to bash to excute
            std::string trimmed_cmd = _trim(Command::m_remove_background_char(getCommandLINE().c_str()));
            const char *command_line = trimmed_cmd.c_str();
            if (execlp("/bin/bash", "/bin/bash", "-c", command_line, nullptr) == -1) {
                perror("smash error: execlp failed");
                _exit(1);
            }
        }else { // excuting directly
            char *args[COMMAND_MAX_ARGS + 1] = {0};
            char trimmed_cmd_line[COMMAND_MAX_LENGTH + 1];

            strncpy(trimmed_cmd_line, getCommandLINE().c_str(), COMMAND_MAX_LENGTH);
            _removeBackgroundSign(trimmed_cmd_line);
            _parseCommandLine(_trim(trimmed_cmd_line).c_str(), args);

            if (execvp(args[0], args) == -1) {
                perror("smash error: execvp failed");
            }

            for (int i = 0; args[i] != nullptr; ++i) {
                free(args[i]);
            }
            _exit(1);
        }
    }
    else {
        m_pid = pid;
        if (m_isBackGround) {
            SmallShell::getInstance().getList()->addJob(this, m_pid);
        } else {
            SmallShell::getInstance().setPid(m_pid);

            if (waitpid(m_pid, nullptr, WUNTRACED) == -1) {
                perror("smash error: waitpid failed");
            }
            SmallShell::getInstance().setPid(-1);
        }
    }
}
////////////////////////Special Commands///////////////////////////////////////

///////////////////////**COMMAND NUMBER 1 ---- REDIRECTION**///////////////////

// should check cpy
bool is_redirectional(const char *cmd_line) {
    if (cmd_line != nullptr) {
        std::string cpy = std::string(cmd_line);
        size_t firstArrowPos = cpy.find_first_of(">");
        if (firstArrowPos == std::string::npos) {
            return false;
        }
        return true;
    }
    return false;
}

RedirectionCommand::RedirectionType RedirectionCommand::getRedirectionType(const char *cmd_line) {
    std::string  cmd_line_copy(cmd_line);
    unsigned int arrow_index = cmd_line_copy.find_first_of(">");
    if (cmd_line_copy[arrow_index + 1] == '>') {
        return RedirectionType::two_arrows;
    } //command is for sure redirectional
    return RedirectionType::one_arrow;
}

RedirectionCommand::RedirectionCommand(const char *cmd_line):Command(cmd_line) ,
m_command(),m_file_path() {
    if (!is_redirectional(cmd_line))
    {
        throw std::logic_error("wrong name");
    }
    m_redirection_1_2 = getRedirectionType(cmd_line);
    std::string cpy_cmd_line = std::string(cmd_line);
    m_command = _trim(cpy_cmd_line.substr(0, cpy_cmd_line.find_first_of(">")));
    m_file_path = _trim(cpy_cmd_line.substr(cpy_cmd_line.find_last_of(">") + 1));

}

void RedirectionCommand::execute() {
    SmallShell &small_shell = SmallShell::getInstance();
    int FDcpy = dup(1);
    int newFD = -1;
    if (FDcpy == -1) {
        perror("smash error: dup failed");
        return;
    }
    if (close(1) == -1) {
        perror("smash error: close failed");
        return;
    }
    if (m_redirection_1_2 == RedirectionType::one_arrow) {
        newFD = open(m_file_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644); //erease content
        if (newFD == -1){
            perror("smash error: open failed");
            if (dup2(FDcpy, 1) == -1) // restore
            {
                perror("smash error: dup2 failed");
                return;
            }
            if (close(FDcpy) == -1) // close duplicate
            {
                perror("smash error: close failed");
                return;
            }
             return;
        }
    }
    else if (m_redirection_1_2 == RedirectionType::two_arrows) {
        newFD = open(m_file_path.c_str(), O_RDWR | O_CREAT | O_APPEND, 0655); // append to content
        if (newFD == -1) {
            perror("smash error: open failed");
            if (dup2(FDcpy, 1) == -1) {
                perror("smash error: dup2 failed");
                return;
            }
            if (close(FDcpy) == -1) {
                perror("smash error: close failed");
                return;
            }
            return;
        }
    }
    small_shell.executeCommand(m_command.c_str());

    if (close(newFD) == -1) {
        perror("smash error: close failed");
        return;
    }
    if (dup2(FDcpy, 1) == -1) {
        perror("smash error: dup2 failed");
        return;
    }
    if (close(FDcpy) == -1) {
        perror("smash error: close failed");
        return;
    }
}

///////////////////////**COMMAND NUMBER 3 ---- listdir**///////////////////
ListDirCommand::ListDirCommand(const char *cmd_line, int indent) : Command(cmd_line) {
    int len = strlen(cmd_line);
    char* cpy = (char*) malloc(sizeof(char) * (len + 1));
    strcpy(cpy, cmd_line);
    _removeBackgroundSign(cpy);
    m_args = getArgs(cpy);
    m_indent_level = indent;

    char* buffer = getcwd(nullptr, 0); // Allocate buffer dynamically
    if (buffer != nullptr) {
        m_dir_path = std::string(buffer);
        free(buffer); // Free dynamically allocated memory
    }

    if (m_args.size() > 2) {
        std::cerr << "smash error: listdir: too many arguments\n";
    } else if (m_args.size() == 1) {
        m_current_dir = "";
    } else {
        m_current_dir = m_args[1];
    }
}

void ListDirCommand::execute() {
    m_files.clear();
    m_directories.clear();

    DIR* dir = opendir(m_current_dir.c_str());
    if (!dir) {
        perror("opendir");
        return;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string name(entry->d_name);
        if (name == "." || name == "..") {
            continue;
        }

        std::string path = m_current_dir + '/' + name;
        struct stat path_stat;
        if (stat(path.c_str(), &path_stat) == -1) {
            perror("stat");
            continue;
        }

        if (S_ISDIR(path_stat.st_mode)) {
            m_directories.push_back(name);
        } else {
            m_files.push_back(name);
        }
    }
    closedir(dir);

    std::sort(m_directories.begin(), m_directories.end());
    std::sort(m_files.begin(), m_files.end());

    for (const auto& dir : m_directories) {
        std::cout << std::string(m_indent_level, '\t') << dir << std::endl;

        ListDirCommand recursive((m_dir_path + '/' + dir).c_str(), m_indent_level + 1);
        recursive.execute();
    }

    for (const auto& file : m_files) {
        std::cout << std::string(m_indent_level, '\t') << file << std::endl;
    }
}


//////////////////////////////////////////////////////////////////////////////

///////////////////////**COMMAND NUMBER 1 ---- CHPRMOPT**//////////////////////
ChangePromptCommand::ChangePromptCommand(const char *cmd_line): BuiltInCommand(cmd_line) {} //ctor
void ChangePromptCommand::execute() {
    if(m_args.size() == 1){ //In case of zero arguments
        SmallShell::getInstance().setPrompt(DEFAULT_PROMPT); //TODO
    }
    if(m_args.size() > 1){ //More than one argument we take the first and ignore the others
        SmallShell::getInstance().setPrompt(m_args[1]);
    }
}
///////////////////////**COMMAND NUMBER 2 ---- SHOWPID**//////////////////////
ShowPidCommand::ShowPidCommand(const char *cmd_line): BuiltInCommand(cmd_line){}

void ShowPidCommand::execute() {
    std::cout << "smash pid is " << getpid() << std::endl;
}


///////////////////////**COMMAND NUMBER 3 ---- PWD**//////////////////////

GetCurrDirCommand::GetCurrDirCommand(const char *cmd_line): BuiltInCommand(cmd_line) {}
void GetCurrDirCommand::execute() {
    char* buffer = getcwd(nullptr, 0); // Allocate buffer dynamically
    if (buffer != nullptr) {
        std::cout << std::string(buffer) << '\n';
        free(buffer); // Free dynamically allocated memory
    } else {
        perror("smash error: getcwd failed"); //TODO: shall we print error like this?
    }
}

///////////////////////**COMMAND NUMBER 4 ---- CD**//////////////////////

std::string ChangeDirCommand::m_prev_dir;
ChangeDirCommand::ChangeDirCommand(const char *cmd_line):
BuiltInCommand(cmd_line){
    if(m_args.size() > 2){
        std::cerr<<"smash error: cd: too many arguments\n";
    }else if(m_args[1] == "-" && m_prev_dir.size() == 0){
        std::cerr<<"smash error: cd: OLDPWD not set\n";
    }

}

void ChangeDirCommand::execute() {
    char buffer[COMMAND_MAX_LENGTH + 1];
    if(getcwd(buffer,COMMAND_MAX_LENGTH + 1) == nullptr){
        perror("smash error: getcwd failed");
    }
    else if(m_args[1] == "-"){ // in case of '-': returning to prev dir
        if(chdir(m_prev_dir.c_str()) == 0) { // if chdir sucesssed
            m_prev_dir = buffer;
        }else{ //if chdir fails
            perror("smash error: chdir failed");
        }
    }
    else if(m_args[1] == ".."){ // in case returning to father ...
        if(chdir(getFatherDir(buffer).c_str()) == -1){ //in case of failiure
            perror("smash error: chdir failed");
        }
    }
    else{
        if(chdir(buffer) == -1){
            perror("smash error: chdir failed");
        }
    }
}

std::string ChangeDirCommand::getFatherDir(const std::string &path) {
    size_t lastSlash = path.find_last_of("/");

    if (lastSlash == std::string::npos || (lastSlash == 0 && path.length() == 1))
    {
        return path; // No parent directory, or it's the root
    }
    else if (lastSlash == 0)
    {
        return "/"; // The parent directory is the root
    }
    else
    {
        return path.substr(0, lastSlash); // Return the substring up to the last slash
    }
}

///////////////////////**COMMAND NUMBER 5 ---- JOBS**//////////////////////
JobsCommand::JobsCommand(const char *cmd_line): BuiltInCommand(cmd_line) {}

void JobsCommand::execute() {
    SmallShell::getInstance().m_job_list.printJobsList(); //TODO::BETTER BE SETTER!
}

JobsList::JobsList() {}

JobsList::JobEntry::JobEntry(int jobID, pid_t pid, const std::string& command,
                             bool isStopped):
        m_jobID(jobID),m_job_pid(pid),m_command(command),m_is_stopped(isStopped) {}

pid_t JobsList::JobEntry::getJobPid() const {
    return m_job_pid;
}

int JobsList::JobEntry::getJobID() const {
    return m_jobID;
}

void JobsList::addJob(Command *cmd,pid_t pid ,bool isStopped) { //NOTE:HERE WE SHOULD SEND THE PID!!!
    if(cmd) { //AVOID SEG FAULT
        removeFinishedJobs();
        m_jobs.push_back(JobEntry(m_maxID + 1, pid,
                                  cmd->getCommandLINE(),isStopped));
    }
}

JobsList::JobEntry *JobsList::getLastJob() { //DONE
    if(m_jobs.size() > 0){
        return &(m_jobs.back());
    }else{
        return nullptr;
    }
}


void JobsList::removeFinishedJobs() {
    std::vector<int> ids;
    for(JobsList::JobEntry &job : m_jobs){
        int result = waitpid(job.getJobPid(), nullptr, WNOHANG);
        if(result == job.getJobPid() || result == -1){ //TODO: if error occured what should we do
            ids.push_back(job.getJobID());
        }
    }
    for(int id : ids){
        removeJobById(id);
    }

}

JobsList::JobEntry *JobsList::getJobById(int jobId){
    for(JobsList::JobEntry &j : m_jobs){
        if(j.getJobID() == jobId ){
            return &j;
        }
    }
    return nullptr;
}

void JobsList::removeJobById(int jobId){

    for (std::vector<JobEntry>::iterator it = m_jobs.begin(); it != m_jobs.end(); ){
        if (it->getJobID() == jobId){
            m_jobs.erase(it);
            return;
        } else
        {
            ++it;
        }
    }

}

std::string JobsList::JobEntry::getCMD() const {
    return m_command;
}

void JobsList::killAllJobs() {

    for (JobsList::JobEntry &job : m_jobs)
    {
        std::cout << job.getJobPid() << ": " << job.getCMD() << "\n";

        if (kill(job.getJobPid(), SIGKILL) != 0) // failure
        {
            perror("smash error: kill failed");
        }
    }
    m_jobs.clear();
}

void JobsList::printJobsList() {
    removeFinishedJobs();
    for (JobsList::JobEntry &job : m_jobs)
    {
        std::cout << "[" << job.getJobID() << "] " << job.getCMD() << "\n";
    }
}
int JobsList::getSize() const {

    return m_jobs.size();
}

///////////////////////**COMMAND NUMBER 6 ---- FG**//////////////////////
ForegroundCommand::ForegroundCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {
    JobsList &lst = SmallShell::m_job_list;
    if(!checkFormatNumber(m_args[1])){
        std::cerr << "smash error: fg: invalid arguments\n";
    }else if (m_args.size() > 2) { //if we have more than one argument
        std::cerr << "smash error: fg: invalid arguments\n";
    }else if (lst.getSize() == 0 && m_args.size() == 1) {
        std::cerr << "smash error: fg: jobs list is empty\n";
    }else if (lst.getJobById(atoi(m_args[1].c_str())) == nullptr) { //TODO: WE NEED TO CHECK IF VALID FORMAT
        std::cerr << "smash error: fg: jobs list is empty\n";
    }
}


void ForegroundCommand::execute() {
    JobsList* lst = SmallShell::getInstance().getList();
    JobsList::JobEntry* j =lst->getJobById(atoi(m_args[1].c_str()));
    SmallShell::getInstance().setPid(j->getJobPid());
    std::cout<< j->getCMD() << " " << SmallShell::getInstance().getPid();
    SmallShell::getInstance().m_job_list.removeJobById(j->getJobID());
    if(waitpid(j->getJobPid(), nullptr,WUNTRACED) == -1){
        perror("smash error: waitpid failed");
    }
    SmallShell::getInstance().setPid(-1);
}

///////////////////////**COMMAND NUMBER 7 ---- QUIT**//////////////////////
QuitCommand::QuitCommand(const char *cmd_line): BuiltInCommand(cmd_line) {
}

void QuitCommand::execute() {
    if(m_args.size() > 2 && m_args[1] == "kill"){
        JobsList &j = SmallShell::getInstance().m_job_list;
        std::cout << "sending SIGKILL signal to" << j.getSize() << "jobs" <<"\n";
        if(j.getSize() > 0 ) {
            j.killAllJobs();
        }
    }
    exit(0); //TODO: SHOULD WE USE EXIST?!
}

///////////////////////**COMMAND NUMBER 8 ---- KILL**//////////////////////

KillCommand::KillCommand(const char *cmd_line): BuiltInCommand(cmd_line){
    if (!checkFormatNumber(m_args[1]) || !checkFormatNumber(m_args[2])) {
        std::cerr << "smash error: kill: invalid arguments\n" ;
    }
    if(m_args.size() > 3 || m_args.size() < 2){
        std::cerr << "smash error: kill: invalid arguments\n" ;

    }
    m_jobID = std::atoi(m_args[2].c_str());
    m_signal_num = std::stoi(m_args[1].substr(1));
    if(!SmallShell::getInstance().getList()->getJobById(m_jobID)){
        std::cerr << "smash error: kill: job-id "<<m_jobID<<" does not exist\n" ;
    }
    if(m_signal_num < 0 || m_signal_num > 31){
        perror("smash error: kill failed");
    }
}

void KillCommand::execute() {
    JobsList* list = SmallShell::getInstance().getList();
    JobsList::JobEntry *job = list->getJobById(m_jobID);
    if (job != nullptr)
    {
        if (kill(job->getJobPid(),m_signal_num ) != 0) // failure
        {
            perror("smash error: kill failed");
            return;
        }
        std::cout << "signal number " << m_signal_num << " was sent to pid " << job->getJobPid() << "\n";
    }
}
///////////////////////**COMMAND NUMBER 9 ---- ALIAS**//////////////////////


aliasCommand::aliasCommand(const char *cmd_line): BuiltInCommand(cmd_line) {
    std::string name, command;
    std::string input = std::string(cmd_line);
    // Find the position of '='
    size_t equal_pos = input.find('=');
    if(equal_pos == -1){
        std::cerr << "alias: invalid alias format \n";
    }
    if (equal_pos != std::string::npos) {
        // Extract the name
        name = input.substr(0, equal_pos);

        // Extract the command (excluding quotes)
        size_t start_quote = input.find('\'', equal_pos);
        size_t end_quote = input.rfind('\'');
        if (start_quote != std::string::npos &&
            end_quote != std::string::npos && start_quote < end_quote) {
            command = input.substr(start_quote + 1,
                                   end_quote - start_quote - 1);
            }
    }
    for (size_t i = 0; i < input.length(); ++i) {
        if (!(input[i] >= 'a' && input[i] < 'z') ||
            !(input[i] >= 'A' && input[i] < 'Z')
            || !(input[i] >= '0' && input[i] <= '9') || !(input[i] != '_')) {
            std::cerr << "smash error: alias: invalid alias format\n";

            }
    }
    for (const auto &p : SmallShell::getInstance().m_aliases){
        if (p.first == name) {
            std::cerr << "smash error: alias: invalid alias format\n";
        }
    }
    m_A_command = command;
    m_name = name;
    // TODO :

}

void aliasCommand::execute() {
    if(m_args.size() == 1){
        for(const auto& p : SmallShell::getInstance().m_aliases){
            std::cout<<p.first<<"='"<< p.second << "'\n" ;
        }
    }else{
        SmallShell::getInstance().m_aliases.push_back(pair<std::string,
                                                      std::string>(m_name,m_A_command));
    }

}

///////////////////////**COMMAND NUMBER 10 ---- UNALIAS**//////////////////////

unaliasCommand::unaliasCommand(const char *cmd_line) : BuiltInCommand(cmd_line){
    if(m_args.size() == 1){
        std::cerr << "smash error: unalias: not enough arguments\n";
    }
}
void unaliasCommand::execute() {
    int size = m_args.size();
    for(int i = 1; i < size;i++){
        if(!SmallShell::getInstance().removeAlias(m_args[i])){
            std::cerr << "smash error: unalias: not enough arguments\n";
            //TODO:
        }
    }
}


//////////////////////////////////////////////////////////////////////////////












SmallShell::SmallShell() {
// TODO: add your implementation
}

void SmallShell::setPrompt(const std::string &newPrompt) {
    m_smash_prompt = newPrompt;
}

SmallShell::~SmallShell() {
// TODO: add your implementation
}













   /////////////////////////////////////////////////////////////////////////
/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command *SmallShell::CreateCommand(const char *cmd_line) {

  string cmd_s = _trim(string(cmd_line));
  string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

  if (firstWord == "chprompt") {
    return new ChangePromptCommand(cmd_line);
  }
  else if (firstWord == "showpid") {
    return new ShowPidCommand(cmd_line);
  }
  else if (firstWord.compare("pwd") == 0) {
      return new GetCurrDirCommand(cmd_line);
  }
  else if (firstWord.compare("cd") == 0) {
      return new ChangeDirCommand(cmd_line);
  }
  else if (firstWord.compare("jobs") == 0) {
      return new JobsCommand(cmd_line);
  }
  else if (firstWord.compare("fg") == 0) {
      return new ShowPidCommand(cmd_line);
  }
  else if (firstWord.compare("quit") == 0) {
      return new ShowPidCommand(cmd_line);
  }
  else if (firstWord.compare("kill") == 0) {
      return new ShowPidCommand(cmd_line);
  }
  else if (firstWord.compare("alias") == 0) {
      return new ShowPidCommand(cmd_line);
  }
  else if (firstWord.compare("unalias") == 0) {
      return new ShowPidCommand(cmd_line);
  }

  else {
    return new ExternalCommand(cmd_line);
  }

    return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line) {
    // TODO: Add your implementation here
    // for example:
    // Command* cmd = CreateCommand(cmd_line);
    // cmd->execute();
    // Please note that you must fork smash process for some commands (e.g., external commands....)
}

pid_t SmallShell::getPid() const {
    return m_pid;
}

void SmallShell::setPid(pid_t pid) {
    m_pid = pid;
}

JobsList *SmallShell::getList() const {
    return &(m_job_list);
}

bool SmallShell::removeAlias(const std::string& toRemove) {
    for (auto it = m_aliases.begin(); it != m_aliases.end(); ) {
        if (it->first == toRemove) {
            it = m_aliases.erase(it); // Erase and update iterator
            return true;
        }
        ++it; // Move to the next element
    }
    return false;
}

std::string SmallShell::getPrompt() const {
    return m_smash_prompt;
}
