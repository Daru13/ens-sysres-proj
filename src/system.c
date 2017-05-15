#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <wait.h>
#include "toolbox.h"
#include "system.h"

#define PIPE_IN  1
#define PIPE_OUT 0

// -----------------------------------------------------------------------------
// SUBPROCESSES
// -----------------------------------------------------------------------------

// Run a command in a child process, and read from its output channel
// through an anonymous pipe, in the given buffer, before waiting for the child
// The function returns how many bytes have been actually read
int runReadableProcess (char* command, char* execvp_argv[],
                        char* output_buffer, int output_buffer_length)
{
    pid_t pipe_fds[2];
    int return_value = pipe(pipe_fds);
    if (return_value < 0)
        handleErrorAndExit("pipe() failed in runReadableProcess()");

    pid_t fork_pid = fork();
    if (fork_pid < 0)
        handleErrorAndExit("fork() failed in runReadableProcess()");

    // Child process
    if (fork_pid == 0)
    {
        // Program must output in its parent's pipe output
        return_value = close(pipe_fds[PIPE_OUT]);
        if (return_value < 0)
            handleErrorAndExit("close() failed in runReadableProcess()");

        return_value = dup2(pipe_fds[PIPE_IN], 1);
        if (return_value < 0)
            handleErrorAndExit("close() failed in runReadableProcess()");        

        execvp(command, execvp_argv);
        handleErrorAndExit("exec() failed in runReadableProcess()");
    }

    // Father process
    return_value = close(pipe_fds[PIPE_IN]);
    if (return_value < 0)
        handleErrorAndExit("close() failed in runReadableProcess()");

    // TODO: clean this part
    int nb_bytes_read = 0;
    while (nb_bytes_read < output_buffer_length)
    {
        int current_nb_bytes_read = read(pipe_fds[PIPE_OUT], output_buffer,
                                         output_buffer_length - nb_bytes_read);
        nb_bytes_read += current_nb_bytes_read;

        if (current_nb_bytes_read == 0)
            break;
        else
            output_buffer += current_nb_bytes_read;
    }

    if (nb_bytes_read < 0)
        handleErrorAndExit("read() failed in runReadableProcess()");

    return_value = close(pipe_fds[PIPE_OUT]);
    if (return_value < 0)
        handleErrorAndExit("close() failed in runReadableProcess()");

    return_value = waitpid(fork_pid, NULL, 0);
    if (return_value < 0)
        handleErrorAndExit("wait() failed in runReadableProcess()");

    return nb_bytes_read;
}
