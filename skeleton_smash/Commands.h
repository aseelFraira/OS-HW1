#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>

#define COMMAND_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)



/*In command class we will have a 'struct' of each class and how we are going
 * to use them*/
class Command {
protected:
    std::string m_cmd_line;
    std::string m_command;
    bool m_isBackGround;

    // TODO: Add your data members
public:
    Command(const char *cmd_line);

    virtual ~Command();

    virtual void execute() = 0;
    std::string getCommand();
    std::string getCommandLINE();
    //virtual void prepare();
    //virtual void cleanup();
    // TODO: Add your extra methods if needed
    std::string m_remove_background_char(const char *cmd_line) const;

};

class BuiltInCommand : public Command {
protected:
    std::vector<std::string> m_args; // contains the command and the arguments
public:
    BuiltInCommand(const char *cmd_line);

    virtual ~BuiltInCommand() {
    }
};




class ExternalCommand : public Command {

public:
    pid_t m_pid;
    bool m_is_complex;

    bool complexity(const char *cmd_line);

    ExternalCommand(const char *cmd_line);

    virtual ~ExternalCommand() {
    }
    const pid_t get_pid() const
    {
        return m_pid;
    }
    void set_pid(pid_t pid)
    {
        m_pid = pid;
    }

    void execute() override;
};

class PipeCommand : public Command {
    // TODO: Add your data members
public:
    PipeCommand(const char *cmd_line);

    virtual ~PipeCommand() {
    }

    void execute() override;
};

class RedirectionCommand : public Command {
    // TODO: Add your data members
    enum class RedirectionType
    {
        one_arrow, two_arrows
    };
    RedirectionType m_redirection_1_2;
    std::string m_command;
    std::string m_file_path;
    RedirectionType getRedirectionType(const char *cmd_line);
public:

    explicit RedirectionCommand(const char *cmd_line);

    virtual ~RedirectionCommand() = default;

    void execute() override;
};
/** Command number 1:
 chprompt command will allow the user to change the prompt displayed by the smash while
  waiting for the next command.
  If no parameters are provided, the prompt shall be reset to smash. If more than on
 */
class ChangePromptCommand : public BuiltInCommand{
public:
    explicit ChangePromptCommand(const char *cmd_line);
    ~ChangePromptCommand() override;
    void execute() override;
};

/** Command number 2:
 showpid command prints the smash PID
  If any number of arguments were provided with this command, they will be ignored.
 */

class ShowPidCommand : public BuiltInCommand {
public:
    ShowPidCommand(const char *cmd_line);

    virtual ~ShowPidCommand() {
    }

    void execute() override;
};

/** Command number 3:
  pwd command has no arguments.
  The pwd command prints the full path of the current working directory. In the next section,
  we will learn how to change the current working directory using the `cd` command.
  You may use getcwd system call to retrieve the current working directory.
 */
class GetCurrDirCommand : public BuiltInCommand {
public:
    GetCurrDirCommand(const char *cmd_line);

    virtual ~GetCurrDirCommand() {
    }

    void execute() override;
};


/** Command number 4:
  The cd (Change Directory) command takes a single argument <path> that specifies either a
  relative or full path to change the current working directory to.
  There's a special argument, `-`, that cd can accept. When `-` is the only argument provided
  to the cd command, it instructs the shell to change the current working directory to the
  last working directory.
  For example, if the current working directory is X, and then the cd command is used to
  change the directory to Y, executing cd - would then set the current working directory
  back to X, executing it again would set the current directory to Y.
  You may use chdir system call to change the current working directory.
  If no argument is provided, this command has no impact
  */
class ChangeDirCommand : public BuiltInCommand {
public:
    static std::string m_prev_dir;
    std::string getFatherDir(const std::string& path);

    // TODO: Add your data members public:
    ChangeDirCommand(const char *cmd_line); //Why do we need this thing?

    virtual ~ChangeDirCommand() {
    }

    void execute() override;
};



class JobsList;
/** Command number 7:
 quit command exits the smash. Only if the kill argument was specified (which is optional)
 then smash should kill (by sending SIGKILL signal) all of its unfinished jobs and print (before
 exiting) the number of processes/jobs that were killed, their PIDs and command-lines (see
 the example for output formatting)
 Note: You may assume that the kill argument, if present, will appear first.

 **Error handling**:
 If any number of arguments (other than kill) were provided with this command, they will
 be ignored.
 */
class QuitCommand : public BuiltInCommand {
    // TODO: Add your data members public:
public:
    QuitCommand(const char *cmd_line);

    virtual ~QuitCommand() {
    }

    void execute() override;
};


class JobsList {
public:
    class JobEntry {
        int m_jobID; //we give the id as the order
        pid_t m_job_pid;
        std::string m_command; //the command of the job
        bool m_is_stopped;
    public:
        int getJobID() const;
        pid_t getJobPid() const;
        std::string getCMD() const;

        JobEntry(int jobID,  pid_t pid, const std::string& command,bool isStopped);
    };
    std::vector<JobEntry> m_jobs;
     int m_maxID;

    // TODO: Add your data members
public:
    JobsList();

    ~JobsList();

    void addJob(Command *cmd,pid_t pid, bool isStopped = false);

    void printJobsList();

    void killAllJobs();

    void removeFinishedJobs();

    JobEntry *getJobById(int jobId);

    void removeJobById(int jobId);

    JobEntry *getLastJob(); //DONE

    JobEntry *getLastStoppedJob(int *jobId); //WHERE DO WE NEED THIS

    int getSize() const;

    // TODO: Add extra methods or modify exisitng ones as needed
};
/** command number 5:
 jobs command prints the jobs list which contains the unfinished jobs
 (Those running in the background).
 The list should be printed in the following format: [<job-id>] <command>, where
 <command> is the original command provided by the user (including aliases as
 discussed later).
 The jobs list should be printed in a sorted order w.r.t the job-id.
 Make sure you delete all finished jobs before printing the jobs list.
 If any number of arguments were provided with this command, they will be ignored.
 */

class JobsCommand : public BuiltInCommand {
    // TODO: Add your data members
public:
    JobsCommand(const char *cmd_line);

    virtual ~JobsCommand() {
    }

    void execute() override;
};

/** Command number 8:
 Kill command sends a signal whose number is specified by <signum> to a job whose
 sequence ID in jobs list is <job-id> (same as job-id in jobs command) and prints a message
 reporting that the specified signal was sent to the specified job (see example below).
 */
class KillCommand : public BuiltInCommand {
    int m_jobID;
    int m_signal_num;
public:
    KillCommand(const char *cmd_line);

    virtual ~KillCommand() {
    }

    void execute() override;
};


/** Command number 6:
 fg command brings a process that runs in the background to the foreground.
 fg command prints the command line of that job along with its PID (as can be seen in the
 example) and then waits for it (hint: waitpid), which in effect will bring the requested
 process to run in the foreground.
 The job-id argument is an optional argument. If it is specified, then the specific job which
 its job id (as printed in jobs command) should be brought to the foreground. If the job-id
 argument is not specified, then the job with the maximal job id in the jobs list should be
 selected to be brought to the foreground.
 Side effects: After bringing the job to the foreground, it should be removed from the
 jobs list.
 */
class ForegroundCommand : public BuiltInCommand {
    // TODO: Add your data members
public:
    ForegroundCommand(const char *cmd_line);

    virtual ~ForegroundCommand() {
    }

    void execute() override;
};

class ListDirCommand : public Command {
    std::vector<std::string> m_directories;
    std::vector<std::string> m_files;
    std::vector<std::string> m_args;
    int m_indent_level;
    std::string m_dir_path;
    std::string m_current_dir;

public:
    ListDirCommand(const char *cmd_line,int indent);

    virtual ~ListDirCommand() {
    }

    void execute() override;
};

class WhoAmICommand : public Command {
public:
    WhoAmICommand(const char *cmd_line);

    virtual ~WhoAmICommand() {
    }

    void execute() override;
};

class NetInfo : public Command {
    // TODO: Add your data members
public:
    NetInfo(const char *cmd_line);

    virtual ~NetInfo() {
    }

    void execute() override;
};

class aliasCommand : public BuiltInCommand {
    std::string m_A_command;
    std::string m_name;
public:
    aliasCommand(const char *cmd_line);

    virtual ~aliasCommand() {
    }

    void execute() override;
};

class unaliasCommand : public BuiltInCommand {

public:
    unaliasCommand(const char *cmd_line);

    virtual ~unaliasCommand() {
    }

    void execute() override;
};



/*our small shell is basically a son process of the linux shell
 * so  we need to save it's pid
 * Jobs: all the commands that were sent to the backgroun
 * We need A jobList class and Job class
 * */
class SmallShell {
private:
    // TODO: Add your data members
    SmallShell();

public:
    static std::string m_smash_prompt;
    static pid_t m_pid;
    static JobsList m_job_list;
    pid_t m_current_process;

    std::vector<std::pair<std::string,std::string>> m_aliases;

   // std::string m_current_cmd;
   // std::string m_old_cmd;

    void setPrompt(const std::string& newPrompt);
    void setPid(pid_t pid);
    pid_t getPid() const;
    JobsList* getList() const;
    bool removeAlias(const std::string& toRemove);

    Command *CreateCommand(const char *cmd_line);

    SmallShell(SmallShell const &) = delete; // disable copy ctor
    void operator=(SmallShell const &) = delete; // disable = operator
    static SmallShell &getInstance() // make SmallShell singleton
    {
        static SmallShell instance; // Guaranteed to be destroyed.
        // Instantiated on first use.
        return instance;
    }
    std::string getPrompt() const;

    ~SmallShell();

    void executeCommand(const char *cmd_line);

    // TODO: add extra methods as needed
};

#endif //SMASH_COMMAND_H_
