#include <stdint.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <unistd.h>

#include <iostream>
#include <string>
#include <sstream>
#include <vector>

#include "linenoise.h"

#include "dwarf/dwarf++.hh"
#include "elf/elf++.hh"

std::vector<std::string> str_split(const char *str, char delim) {
    std::vector<std::string> strings;

    std::string s = str;
    std::stringstream ss(s);
    std::string item;

    while (std::getline(ss, item, delim))
        strings.push_back(item);

    return strings;
}

bool is_prefix(const std::string &prefix, const std::string &str) {
    if (prefix.size() > str.size()) 
        return false;
    return std::equal(prefix.begin(), prefix.end(), str.begin());
}

class Breakpoint {
private:
    pid_t pid;
    intptr_t addr;
    bool enabled;
    uint8_t saved_data;

public:
    Breakpoint(pid_t _pid, intptr_t _addr) {
        pid = _pid;
        addr = _addr;
        enabled = false;
        saved_data = 0x00;
    }

    bool is_enabled() { return enabled; }
    intptr_t get_addr() { return addr; }

    void enable() { 
        auto data = ptrace();


        enabled = true; 
    }

    void disable() {
        
        



        enabled = false; 
    }
};

class Debugger {
private:
    pid_t pid;
    const char *program;

public:
    Debugger(pid_t _pid, const char *_program) {
        pid = _pid;
        program = _program;
    }

    void handle_command(const char *line) {
        std::vector<std::string> cmd_args = str_split(line, ' ');
        std::string &cmd = cmd_args[0];

        if (is_prefix(cmd, "continue"))
            continue_execution(pid);
        else
            std::cerr << "Unknown command\n";
    }

    void continue_execution(pid_t pid) {
        ptrace(PTRACE_CONT, pid, NULL, NULL);
    
        int status;
        int options = 0;
        waitpid(pid, &status, options);
    }

    void run() {
        // Wait for the SIGTRAP signal from the child
        // which will be sent due to us tracing it with ptrace
        int status;
        int options = 0;
        waitpid(pid, &status, options);

        // Loop on debugger commands until we get EOF
        char *line = NULL;
        while ((line = linenoise("edb> ")) != NULL) {
            handle_command(line);
            linenoiseHistoryAdd(line);
            linenoiseFree(line);
        }
    }
};

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cerr << "Program name not specified\n";
        return -1;
    }

    const char *program = argv[1];
    pid_t pid = fork();

    if (pid < 0) {
        std::cerr << "Unable to fork child process\n";
        return -1;
    }
    else if (pid == 0) {
        std::cout << "In child\n";
        if (ptrace(PTRACE_TRACEME, pid, NULL, NULL) < 0) {
            std::cerr << "Unable to trace child process\n";
            return -1;
        }

        // TODO: NEED TO PASS PROGRAM ARGUMENTS AS SECOND PARAM
        execl(program, program, NULL);
    } else {
        std::cout << "In parent\n";
        Debugger dbg(pid, program);
        dbg.run();
    }

    return 0;
}


