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
#include <regex>
#include <sys/types.h>




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
Command::Command(const char *cmd_line,const std::string& aliasName):m_cmd_line(std::string(cmd_line)),
m_aliasName(std::string(cmd_line)) {
    if (aliasName != "") {
        m_aliasName = aliasName;
    }
    m_isBackGround = _isBackgroundComamnd(m_cmd_line.c_str());
    std::string cpy = std::string(cmd_line);
    size_t firstSpacePos = cpy.find(
            ' '); //TODO Should we check for all kind of whitspaces tabs,...
    if (firstSpacePos == std::string::npos) { //Handle the case only one string!
        m_command = cpy;
    } else {
        m_command = cpy.substr(0, firstSpacePos);
    }

}
Command::~Command() {

}


std::string Command::getCommand() {
    return m_command;
}
std::string Command::getCommandLINE() {
    return m_aliasName;
}



BuiltInCommand::BuiltInCommand(const char *cmd_line,const std::string& aliasName): Command(cmd_line,aliasName){
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

ExternalCommand::ExternalCommand(const char *cmd_line,const std::string& aliasName):Command(cmd_line,aliasName
), m_is_complex(complexity(cmd_line)) {
}

bool ExternalCommand::complexity(const char *cmd_line)
{
    if (std::string(cmd_line).find_first_of("*?") != std::string::npos)
    {
        return true;
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
            return;
        }

        if (m_is_complex) { // Complex command = True
            //send to bash to excute
            const char *command_line =_trim(Command::m_remove_background_char(m_cmd_line.c_str())).c_str();
            if (execlp("/bin/bash", "/bin/bash", "-c", command_line, nullptr) != 0) {
                perror("smash error: execlp failed");
                return;
            }
        }else { // excuting directly
            char *args[COMMAND_MAX_ARGS + 1] = {0};
            char trimmed_cmd_line[COMMAND_MAX_LENGTH + 1];

            strcpy(trimmed_cmd_line, m_cmd_line.c_str());
            _removeBackgroundSign(trimmed_cmd_line);
            _parseCommandLine(_trim(trimmed_cmd_line).c_str(), args);

            if (execvp(args[0], args) == -1) {
                perror("smash error: execvp failed");
            }

            for (int i = 0; i <= COMMAND_MAX_ARGS; ++i) {
                if (args[i] != nullptr) {
                    free(args[i]);
                }
            }
            exit(0);
        }
    }
    else {
        m_pid = pid;
        if (m_isBackGround) {
            SmallShell::getInstance().getList()->addJob(this, m_pid);
        }else {
            SmallShell::getInstance().setCurrFGPID(m_pid);
            if (waitpid(m_pid, nullptr, WUNTRACED) == -1) {
                perror("smash error: waitpid failed");
            }
            SmallShell::getInstance().setCurrFGPID(-1);
        }
    }
}
////////////////////////Special Commands///////////////////////////////////////



///////////////////////**COMMAND NUMBER 1 ---- REDIRECTION**///////////////////

// should check cpy
bool is_redirectional(const char *cmd_line) {
    if (cmd_line)
    {
        std::string s(cmd_line);
        int index_first = s.find_first_of(">");
        if (index_first == -1)
        {
            return false;
        }
        else
        {
            return true;
        }
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

RedirectionCommand::RedirectionCommand(const char *cmd_line,const std::string& aliasName):Command(cmd_line,aliasName), m_command(),m_file_path() {
    if (!is_redirectional(cmd_line))
    {
        return;
    }
    m_redirection_1_2 = getRedirectionType(cmd_line);
    std::string cpy_cmd_line(cmd_line);
    m_command = _trim(cpy_cmd_line.substr(0, cpy_cmd_line.find_first_of(">")));
    m_file_path = _trim(cpy_cmd_line.substr(cpy_cmd_line.find_last_of(">") + 1));

}

void RedirectionCommand::execute() {
    SmallShell &small_shell = SmallShell::getInstance();
    int newFD;
    int FDcpy = dup(1);
    if (FDcpy == -1) {
        perror("smash error: dup failed");
        return;
    }
    if (close(1)) {//-1
        perror("smash error: close failed");
        return;
    }
    if (m_redirection_1_2 == RedirectionType::one_arrow) {

        newFD = open(m_file_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666); //erease content
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
        newFD = open(m_file_path.c_str(), O_RDWR | O_CREAT | O_APPEND, 0666); // append to content
        if (newFD == -1) {
            perror("smash error: open failed");
            if (dup2(FDcpy, 1) == -1) {
                perror("smash error: dup2 failed");
                return;
            }
            if (close(FDcpy)) {//-1
                perror("smash error: close failed");
                return;
            }
            return;
        }
    }
    small_shell.executeCommand(m_command.c_str());

    if (close(newFD) == -1) {
        perror("smash error: close failed");
        exit(0);
    }
    if (dup2(FDcpy, 1) == -1) {
        perror("smash error: dup2 failed");
        return;
    }
    if (close(FDcpy)== -1) { //re -1
        perror("smash error: close failed");
        return;
    }
}
///////////////////////**COMMAND NUMBER 2 ---- pipes**///////////////////
bool _is_pipe_command(const char *cmd_line)
{
    if (std::string(cmd_line).find_first_of("|") != std::string::npos)
    {
        return true;
    }
    return false;
}

PipeCommand::PipeType PipeCommand::get_pipe_type(const char *cmd_line)
{
    std::string s(cmd_line);
    int index_of_line = s.find_first_of("|");
    int index_of_amp = s.find_first_of("&");

    if (index_of_line != -1)
    {
        if (index_of_amp == -1)
        {
            return PipeType::regular;
        }
        else
        {
            return PipeType::fault;
        }
    }
    else
    {
        throw std::logic_error("PipeCommand::PipeCommand");
    }
}

std::string PipeCommand::get_firstCMD(const char *cmd_line)
{
    std::string s(cmd_line);
    return s.substr(0, s.find_first_of("|"));
}

std::string PipeCommand::get_secondCMD(const char *cmd_line)
{

    std::string s(cmd_line);
    int index_of_line = s.find_first_of("|");
    int index_of_amp = s.find_first_of("&");

    if (index_of_amp != -1)
    {
        return s.substr(index_of_amp + 1, s.size() - index_of_amp);
    }
    std::string cmd2 = s.substr(index_of_line + 1, s.size() - index_of_line);
    return cmd2;
}

PipeCommand::PipeCommand(const char *cmd_line, const std::string& aliasName)
    : Command(cmd_line, aliasName)
{
  if (_is_pipe_command(cmd_line))
  {
    m_type_of_pipe = get_pipe_type(cmd_line);
    m_firstCMD = _trim(get_firstCMD(cmd_line));
    m_secondCMD = _trim(get_secondCMD(cmd_line));
    if (m_firstCMD == "" || m_secondCMD == "")
    {
      throw std::logic_error("PipeCommand::PipeCommand");
    }
  }
  else
  {
    throw std::logic_error("wrong name");
  }
}

PipeCommand::~PipeCommand()
{
}

void PipeCommand::execute()
{
  enum PIPE
  {
    READ = 0,
    WRITE = 1
  };

  int files[2];
  if (pipe(files) == -1)
  {
    perror("smash error: pipe failed");
    return;
  }

  SmallShell &smash = SmallShell::getInstance();

  // Fork and execute the first command
  pid_t pid1 = fork();
  if (pid1 == -1)
  {
    perror("smash error: fork failed");
    if (close(files[READ]) == -1)
    {
      perror("smash error: close failed");
      return;
    }
    if (close(files[WRITE]) == -1)
    {
      perror("smash error: close failed");
      return;
    }
    return;
  }

  if (pid1 == 0)
  { // Child process for the first command
    if (setpgrp() == -1)
    {
      perror("smash error: setpgrp failed");
      return;
    }
    if (m_type_of_pipe == PipeType::regular)
    {
      if (dup2(files[WRITE], STDOUT_FILENO) == -1) // Redirect stdout to pipe's write end
      {
        perror("smash error: dup2 failed");
        return;
      }
    }
    else
    {
      if (dup2(files[WRITE], STDERR_FILENO) == -1) // Redirect stderr to pipe's write end
      {
        perror("smash error: dup2 failed");
        return;
      }
    }
    if (close(files[READ]) == -1)
    {
      perror("smash error: close failed");
      return;
    }
    if (close(files[WRITE]) == -1) // Close after dup2
    {
      perror("smash error: close failed");
      return;
    }
    smash.executeCommand(m_firstCMD.c_str());
    exit(0);
  }
  else
  {
    if (close(files[WRITE]) == -1) // Close write end in parent after forking the first child
    {
      perror("smash error: close failed");
      return;
    }
  }

  // Fork and execute the second command
  pid_t pid2 = fork();
  if (pid2 == -1)
  {
    perror("smash error: fork failed");
    if (close(files[READ]) == -1) // Ensure read end is also closed on this error path
    {
      perror("smash error: close failed");
      return;
    }
    return;
  }

  if (pid2 == 0)
  { // Child process for the second command
    if (setpgrp() == -1)
    {
      perror("smash error: setpgrp failed");
      return;
    }
    if(dup2(files[READ], STDIN_FILENO) == -1) // Redirect stdin to pipe's read end
    {
      perror("smash error: dup2 failed");
      return;
    }
    if(close(files[READ]) == -1)              // Close after dup2
    {
      perror("smash error: close failed");
      return;
    }
    smash.executeCommand(m_secondCMD.c_str());
    exit(0);
  }
  else
  {
    if(close(files[READ])) // Close read end in parent after forking the second child
    {
      perror("smash error: close failed");
      return;
    }
  }

  // Wait for both child processes to complete
  if (waitpid(pid1, nullptr, WUNTRACED) == -1)
  {
    perror("smash error: waitpid failed");
  }

  if (waitpid(pid2, nullptr, WUNTRACED) == -1)
  {
    perror("smash error: waitpid failed");
  }
}


///////////////////////**COMMAND NUMBER 3 ---- listdir**///////////////////
ListDirCommand::ListDirCommand(const char *cmd_line, int indent,const std::string& aliasName) : Command(cmd_line,aliasName) {
    int len = strlen(cmd_line);
    char* cpy = (char*) malloc(sizeof(char) * (len + 1));
    strcpy(cpy, cmd_line);
    _removeBackgroundSign(cpy); //TODO
    m_args = getArgs(cpy);
    m_indent_level = indent;

    char* buffer = getcwd(nullptr, 0); // Allocate buffer dynamically
    if (buffer == nullptr) {
        perror("smash error: getcwd failed");
        return;
    }
    m_dir_path = std::string(buffer);

    if (m_args.size() > 2) {
        std::cerr << "smash error: listdir: too many arguments\n";
    } else if (m_args.size() == 1) {
        m_current_dir = std::string(buffer);
    } else {
        m_current_dir = m_args[1];
    }
    free(buffer); // Free dynamically allocated memory

}

void ListDirCommand::execute() {

    DIR* dir = opendir((m_current_dir).c_str());
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

        ListDirCommand recursive(("listdir " + m_current_dir + '/' + dir).c_str(), m_indent_level + 1,m_aliasName);
        recursive.execute();
    }

    for (const auto& file : m_files) {
        std::cout << std::string(m_indent_level, '\t') << file << std::endl;
    }
    return;
}
///////////////////////////////////////////////////////////////////////////////
WhoAmICommand::WhoAmICommand(const char *cmd_line,const std::string& aliasName) : Command(cmd_line,aliasName) {}


void WhoAmICommand::execute() {
    // Get the real user ID
    uid_t uid = getuid();

    // Open the /etc/passwd file
    int fd = open("/etc/passwd", O_RDONLY);
    if (fd < 0) {
        std::cerr << "Error: Could not open /etc/passwd file." << std::endl;
        return;
    }

    // Buffer to read data
    char buffer[4096];
    ssize_t bytes_read;
    std::string passwd_data;

    // Read the file into the buffer
    while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0) {
        buffer[bytes_read] = '\0';  // Null-terminate the buffer
        passwd_data += buffer;
    }

    // Close the file
    close(fd);

    if (bytes_read < 0) {
        std::cerr << "Error: Failed to read /etc/passwd file." << std::endl;
        return;
    }

    // Parse the /etc/passwd file line by line
    size_t start = 0;
    while (start < passwd_data.size()) {
        size_t end = passwd_data.find('\n', start);
        if (end == std::string::npos) {
            end = passwd_data.size();
        }

        // Extract a single line
        std::string line = passwd_data.substr(start, end - start);
        start = end + 1;

        // Parse the line into fields (username:x:uid:gid:info:home:shell)
        size_t field_start = 0, field_end;
        std::string fields[7];
        int field_index = 0;
        while (field_index < 7 && (field_end = line.find(':', field_start)) != std::string::npos) {
            fields[field_index] = line.substr(field_start, field_end - field_start);
            field_start = field_end + 1;
            field_index++;
        }
        if (field_index < 7 && field_start < line.size()) {
            fields[field_index] = line.substr(field_start);
            field_index++;
        }

        // Check if the UID matches
        if (field_index > 2 && std::stoi(fields[2]) == uid) {
            // Extract username and home directory
            std::string username = fields[0];
            std::string home_dir = (field_index >= 5) ? fields[5] : "Unknown";

            // Print the username and home directory
            std::cout << username << " " << home_dir << std::endl;
            return;
        }
    }

    // If no matching UID was found
    std::cerr << "Error: User not found in /etc/passwd." << std::endl;
}




//////////////////////////////////////////////////////////////////////////////

///////////////////////**COMMAND NUMBER 1 ---- CHPRMOPT**//////////////////////
ChangePromptCommand::ChangePromptCommand(const char *cmd_line,const std::string& aliasName): BuiltInCommand(cmd_line,aliasName) {} //ctor
void ChangePromptCommand::execute() {
    if(m_args.size() == 1){ //In case of zero arguments
        SmallShell::getInstance().setPrompt(DEFAULT_PROMPT); //TODO
    }
    if(m_args.size() > 1){ //More than one argument we take the first and ignore the others
        SmallShell::getInstance().setPrompt(m_args[1]);
    }
}
ChangePromptCommand::~ChangePromptCommand() = default;

///////////////////////**COMMAND NUMBER 2 ---- SHOWPID**//////////////////////
ShowPidCommand::ShowPidCommand(const char *cmd_line,const std::string& aliasName): BuiltInCommand(cmd_line,aliasName){}

void ShowPidCommand::execute() {
    std::cout << "smash pid is " << getpid() << std::endl;
}


///////////////////////**COMMAND NUMBER 3 ---- PWD**//////////////////////

GetCurrDirCommand::GetCurrDirCommand(const char *cmd_line,const std::string& aliasName): BuiltInCommand(cmd_line,aliasName) {}
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
ChangeDirCommand::ChangeDirCommand(const char *cmd_line,const std::string& aliasName):
BuiltInCommand(cmd_line,aliasName){
}

void ChangeDirCommand::execute() {
    if(m_args.size() > 2){
        std::cerr<<"smash error: cd: too many arguments\n";
        return;
    }else if(m_args[1] == "-" && m_prev_dir.size() == 0){
        std::cerr<<"smash error: cd: OLDPWD not set\n";
        return;
    }
    char buffer[COMMAND_MAX_LENGTH + 1];
    if(getcwd(buffer,COMMAND_MAX_LENGTH + 1) == nullptr){
        perror("smash error: getcwd failed");
    }
    if(m_args[1] == "-"){ // in case of '-': returning to prev dir
        if(chdir(m_prev_dir.c_str()) == 0) { // if chdir sucesssed
            m_prev_dir = buffer;
        }else{ //if chdir fails
            perror("smash error: chdir failed");
        }
    }
    else if(m_args[1] == ".."){ // in case returning to father ...
        if(chdir(getFatherDir(buffer).c_str()) == -1){ //in case of failiure
            perror("smash error: chdir failed");
        }else {
            m_prev_dir = buffer;
        }
    }
    else{
        if(chdir(m_args[1].c_str()) == -1){
            perror("smash error: chdir failed");
        }else {
            m_prev_dir = buffer;
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
JobsCommand::JobsCommand(const char *cmd_line,const std::string& aliasName): BuiltInCommand(cmd_line,aliasName) {}

void JobsCommand::execute() {
    SmallShell::getInstance().m_job_list.printJobsList(); //TODO::BETTER BE SETTER!

}

JobsList::JobsList():m_maxID(-1) {}

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
    int id = 0;
    removeFinishedJobs();
    if(cmd) { //AVOID SEG FAULT
        if (getLastJob()) {
            id = getLastJob()->getJobID();
        }
        m_jobs.push_back(JobEntry(id + 1, pid,
                                  cmd->getCommandLINE(),isStopped));
    }
}
JobsList::~JobsList() = default;

JobsList::JobEntry *JobsList::getLastJob() { //DONE
   return m_jobs.size() ? &m_jobs.back() : nullptr;
}


void JobsList::removeFinishedJobs() {
    std::vector<int> ids;
    for(JobsList::JobEntry &job : m_jobs){
        pid_t pid = job.getJobPid();
        int result = waitpid(pid, nullptr, WNOHANG);
        if(result == -1 || result == job.getJobPid()){ //TODO: if error occured what should we do
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

    for (std::vector<JobEntry>::iterator it = m_jobs.begin(); it != m_jobs.end(); ++it){
        if ((*it).getJobID()== jobId){
            m_jobs.erase(it);
            return;
        }
    }
}

std::string JobsList::JobEntry::getCMD() const {
    return m_command;
}

void JobsList::killAllJobs() {

    for (JobsList::JobEntry &job : m_jobs)
    {
        pid_t pid = job.getJobPid();
        std::cout << job.getJobPid() << ": " << job.getCMD() << "\n";
        if (kill(pid, SIGKILL) != 0) // failure
        {
            perror("smash error: kill failed");
        }
    }

    m_jobs.clear();
}

void JobsList::printJobsList() {
    for (JobsList::JobEntry &job : m_jobs)
    {
        std::cout << "[" << job.getJobID() << "] " << job.getCMD() << std::endl;
    }
}
int JobsList::getSize() const {

    return m_jobs.size();
}

///////////////////////**COMMAND NUMBER 6 ---- FG**//////////////////////
ForegroundCommand::ForegroundCommand(const char *cmd_line,const std::string& aliasName) : BuiltInCommand(cmd_line,aliasName) {


}


void ForegroundCommand::execute() {
    JobsList* lst = SmallShell::getInstance().getList();
    if (lst->getSize() == 0 && m_args.size() == 1) {
        std::cerr << "smash error: fg: jobs list is empty\n";
        return;
    }else if (m_args.size() > 2) { //if we have more than one argument
        std::cerr << "smash error: fg: invalid arguments\n";
        return;

    } else if(m_args.size() == 2 && !checkFormatNumber(m_args[1])){
            std::cerr << "smash error: fg: invalid arguments\n";
        return;
    }else if (m_args.size() == 2 && lst->getJobById(std::atoi(m_args[1].c_str())) == nullptr) { //TODO: WE NEED TO CHECK IF VALID FORMAT
            std::cerr << "smash error: fg: job-id " << std::atoi(m_args[1].c_str()) << " does not exist\n";
        return;
    }
    else {
        int id = -1;
        if (m_args.size() == 1) {
            id = lst->getLastJob()->getJobID();
        }else {
            id = atoi(m_args[1].c_str());
        }


        JobsList::JobEntry* j =lst->getJobById(id);
        pid_t p = j->getJobPid();
        SmallShell::getInstance().setCurrFGPID(p);

        std::cout<< j->getCMD() << " " << p <<"\n";

        SmallShell::getInstance().m_job_list.removeJobById(j->getJobID());

        if(waitpid(p, nullptr,WUNTRACED) == -1){
            perror("smash error: waitpid failed");
        }

        SmallShell::getInstance().setCurrFGPID(-1);
    }
}

///////////////////////**COMMAND NUMBER 7 ---- QUIT**//////////////////////
QuitCommand::QuitCommand(const char *cmd_line,const std::string& aliasName): BuiltInCommand(cmd_line,aliasName) {
}

void QuitCommand::execute() {
    if(m_args.size() >= 2 && m_args[1] == "kill" ){
        JobsList &j = SmallShell::getInstance().m_job_list;
        std::cout << "smash: sending SIGKILL signal to " << j.getSize() << " jobs:" <<"\n";
        if(j.getSize() > 0 ) {
            j.killAllJobs();
        }
    }
    delete this;
    exit(0);
}

///////////////////////**COMMAND NUMBER 8 ---- KILL**//////////////////////
bool checkSignum(std::string sig) {
    int i = 1;
    int num = 0;
    if (sig[0] != '-') {
        return false;
    }
    while (sig[i]) {
           if (!isdigit(sig[i])) {
            return false;
          }
           num = num * 10 + (sig[i] - '0');
           i++;
    }
    return true;

}

KillCommand::KillCommand(const char *cmd_line,const std::string& aliasName): BuiltInCommand(cmd_line,aliasName){


}

void KillCommand::execute() {
    if (m_args.size() != 3) {
        std::cerr << "smash error: kill: invalid arguments\n";
        return;
    }

    if (!checkSignum(m_args[1]) || !checkFormatNumber(m_args[2])) {
        std::cerr << "smash error: kill: invalid arguments\n";
        return;
    }

    m_signal_num = std::stoi(m_args[1].substr(1));
    m_jobID = std::atoi(m_args[2].c_str());

    JobsList* list = SmallShell::getInstance().getList();
    JobsList::JobEntry* job = list->getJobById(m_jobID);

    if (!job) {
        std::cerr << "smash error: kill: job-id " << m_jobID << " does not exist\n";
        return;
    }
    std::cout << "signal number " << m_signal_num << " was sent to pid " << job->getJobPid() << "\n";
    if (kill(job->getJobPid(), m_signal_num) != 0) {
        perror("smash error: kill failed");
        return;
    }


    // Handle SIGKILL explicitly
    if (m_signal_num == SIGKILL) {
        list->removeJobById(m_jobID);
    }
}

///////////////////////**COMMAND NUMBER 9 ---- ALIAS**//////////////////////
bool is_special(const std::string& name) {
    return name == "whoami"  || name == "listdir"
    || name == "netinfo";
}
bool is_builtin(const std::string& name) {
    return name == "pwd"  || name == "cd"
    || name == "alias" || name == "chprompt" || name == "showpid"
    || name == "fg" || name == "jobs" || name == "kill" || name == "quit"
    || name == "unalias";
}

aliasCommand::aliasCommand(const char *cmd_line,const std::string& aliasName) : BuiltInCommand(cmd_line,aliasName),m_A_command(),m_name() {
    // Convert cmd_line to a string for processing
}

void aliasCommand::execute() {
    if (m_args.size() != 1) {
        std::string input = m_cmd_line;
        // Check for the "alias" prefix
        if (input.find("alias ") != 0) {
            std::cerr << "smash error: alias: invalid alias format\n";
            return;
        }

        // Find the position of '='
        size_t equal_pos = input.find('=');
        if (equal_pos == std::string::npos) { // '=' is required
            std::cerr << "smash error: alias: invalid alias format\n";
            return;
        }

        // Extract alias name (remove "alias " prefix)
        std::string name = input.substr(6, equal_pos - 6); // From index 6 to just before '='
        name.erase(std::remove_if(name.begin(), name.end(), ::isspace), name.end()); // Remove whitespace

        // Validate the alias name using regex
        std::string name_pattern = R"(^[a-zA-Z0-9_]+$)";
        std::regex regex_name(name_pattern);
        if (!std::regex_match(name, regex_name)) {
            std::cerr << "smash error: alias: invalid alias format\n";
            return;
        }

        // Extract the command part between single quotes
        size_t start_quote = input.find('\'', equal_pos);
        size_t end_quote = input.rfind('\'');
        std::string command;
        if (start_quote != std::string::npos && end_quote != std::string::npos && start_quote < end_quote) {
            command = input.substr(start_quote + 1, end_quote - start_quote - 1);
        } else {
            std::cerr << "smash error: alias: invalid alias format\n";
            return;
        }

        // Check for duplicate alias names in the aliases map

        // Assign values to class members
        m_A_command = command;
        m_name = name;
    }

    if(m_args.size() == 1){
        for(const auto& p : SmallShell::getInstance().m_aliases){
            std::cout<<p.first<<"='"<< p.second << "'\n" ;
        }
    }else{

        for (const auto &p : SmallShell::getInstance().m_aliases) {
            if (p.first == m_name) {
                std::cerr << "smash error: alias: " << m_name << " already exists or is a reserved command\n";
                return;
            }
        }

        // Check if the name is a reserved command
        if (is_special(m_name) || is_builtin(m_name)) {
            std::cerr << "smash error: alias: " << m_name << " already exists or is a reserved command\n";
            return;
        }

        SmallShell::getInstance().m_aliases.push_back(pair<std::string,
                                                      std::string>(m_name,m_A_command));


    }

}

///////////////////////**COMMAND NUMBER 10 ---- UNALIAS**//////////////////////

unaliasCommand::unaliasCommand(const char *cmd_line,const std::string& aliasName) : BuiltInCommand(cmd_line,aliasName){

}
void unaliasCommand::execute() {
    if(m_args.size() == 1){
        std::cerr << "smash error: unalias: not enough arguments\n";
        return;
    }
    int size = m_args.size();
    for(int i = 1; i < size;i++){
        if(!SmallShell::getInstance().removeAlias(m_args[i])){
            std::cerr << "smash error: unalias: "<< m_args[i]<<" alias does not exist\n";
            return;
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
Command *SmallShell::CreateCommand(const char *cmd_line,const std::string& aliasName) {

    string cmd_s = _trim(string(cmd_line));
    size_t first_delim = cmd_s.find_first_of(" \n");
    std::string firstWord = cmd_s.substr(0, first_delim);

    // Extract the remaining part (args)
    std::string args;
    if (first_delim != std::string::npos) {
        // Skip the space after the first word
        args = cmd_s.substr(first_delim + 1);
        // Trim leading and trailing spaces from args
        size_t start = args.find_first_not_of(" \n");
        size_t end = args.find_last_not_of(" \n");
        if (start != std::string::npos) {
            args = args.substr(start, end - start + 1);
        } else {
            args = ""; // No args found
        }
    }
    char* cpy = strdup(cmd_line);
    if (cpy == nullptr) {
        std::cerr << "smash error: alias: strdup failed\n";
        free(cpy);
        return nullptr;
    }

    _removeBackgroundSign(cpy);

    for (const auto& alias : m_aliases) {
        if (alias.first == cpy) {
            std::cout << cmd_line << "\n";
            if (_isBackgroundComamnd(cmd_line)) {
                executeCommand((alias.second + ' ' +args + '&').c_str(),cmd_line);
            }
            executeCommand((alias.second + ' ' +args).c_str(),cmd_line);
            return nullptr;
        }
    }
    if (firstWord.compare("alias") == 0) {
        return new aliasCommand(cmd_line,aliasName);
    }
    if (is_redirectional(cmd_line) == true) {
        char* cpy = strdup(cmd_line);
        if (cpy == nullptr) {
            std::cerr << "smash error: alias: strdup failed\n";
            free(cpy);
            return nullptr;
        }
        _removeBackgroundSign(cpy);

        return new RedirectionCommand(cpy,aliasName);
    }
    if (_is_pipe_command(cmd_line)) {
        return  new PipeCommand(cmd_line,aliasName);
    }


    if (firstWord == "chprompt") {
    return new ChangePromptCommand(cmd_line,aliasName);
  }
  else if (firstWord == "showpid") {
    return new ShowPidCommand(cmd_line,aliasName);
  }
  else if (firstWord.compare("pwd") == 0) {
      return new GetCurrDirCommand(cmd_line,aliasName);
  }
  else if (firstWord.compare("cd") == 0) {
      return new ChangeDirCommand(cmd_line,aliasName);
  }
  else if (firstWord.compare("jobs") == 0) {
      return new JobsCommand(cmd_line,aliasName);
  }
  else if (firstWord.compare("fg") == 0) {
        return new ForegroundCommand(cmd_line,aliasName);
  }
  else if (firstWord.compare("quit") == 0) {
      return new QuitCommand(cmd_line,aliasName);
  }
  else if (firstWord.compare("kill") == 0) {
      return new KillCommand(cmd_line,aliasName);
  }
  else if (firstWord.compare("unalias") == 0) {
      return new unaliasCommand(cmd_line,aliasName);
  }else if (firstWord.compare("listdir") == 0) {
      return new ListDirCommand(cmd_line,1,aliasName);
  }else if (firstWord.compare("whoami") == 0) {
      return new WhoAmICommand(cmd_line,aliasName);
  }
  else {
    return new ExternalCommand(cmd_line,aliasName);
  }

    return nullptr;
}


JobsList SmallShell::m_job_list;
std::string SmallShell::m_smash_prompt = "smash";

void SmallShell::executeCommand(const char *cmd_line,const std::string& aliasCMD) {
    m_job_list.removeFinishedJobs();
    Command *command = CreateCommand(cmd_line,aliasCMD);


    if (command) {
        try {
            command->execute();
        }catch (std::exception &e) {

        }
        delete command;
    }
    setCurrFGPID(-1);
    // TODO: Add your implementation here
    // for example:
    // Command* cmd = CreateCommand(cmd_line);
    // cmd->execute();
    // Please note that you must fork smash process for some commands (e.g., external commands....)
}


JobsList *SmallShell::getList() const {
    return &(m_job_list);
}

bool SmallShell::removeAlias(const std::string& toRemove) {
    for (auto it = m_aliases.begin(); it != m_aliases.end(); ) {
        if (it->first == toRemove) {
            m_aliases.erase(it); // Erase and update iterator
            return true;
        }
        ++it; // Move to the next element
    }
    return false;
}

std::string SmallShell::getPrompt() const {
    return m_smash_prompt;
}
