#define _POSIX_C_SOURCE 200809L
#include "systemcalls.h"
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

static bool successful(int status)
{
    return status != -1 && WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

bool do_system(const char *cmd)
{
    if (!cmd) return false;
    return successful(system(cmd));
}

static bool execute(const char *outputfile, int count, va_list arguments)
{
    if (count <= 0 || (size_t)count > SIZE_MAX / sizeof(char *) - 1) return false;
    char **command = calloc((size_t)count + 1, sizeof(*command));
    if (!command) return false;
    for (int i = 0; i < count; ++i) {
        command[i] = va_arg(arguments, char *);
        if (!command[i]) { free(command); return false; }
    }
    if (command[0][0] != '/') { free(command); return false; }
    fflush(NULL);
    pid_t child = fork();
    if (child == -1) { free(command); return false; }
    if (child == 0) {
        if (outputfile) {
            int fd = open(outputfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd == -1) _exit(127);
            if (fd != STDOUT_FILENO) {
                if (dup2(fd, STDOUT_FILENO) == -1) { close(fd); _exit(127); }
                close(fd);
            }
        }
        execv(command[0], command);
        _exit(127);
    }
    free(command);
    int status;
    pid_t waited;
    do { waited = waitpid(child, &status, 0); } while (waited == -1 && errno == EINTR);
    return waited == child && successful(status);
}

bool do_exec(int count, ...)
{
    va_list arguments;
    va_start(arguments, count);
    bool result = execute(NULL, count, arguments);
    va_end(arguments);
    return result;
}

bool do_exec_redirect(const char *outputfile, int count, ...)
{
    if (!outputfile || !*outputfile) return false;
    va_list arguments;
    va_start(arguments, count);
    bool result = execute(outputfile, count, arguments);
    va_end(arguments);
    return result;
}
