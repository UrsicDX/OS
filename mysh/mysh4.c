#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <stdbool.h>
#include <dirent.h>
#include <errno.h>
#include <libgen.h>
#include <sys/stat.h>  // Header for the 'mkdir' function
#include <sys/types.h> // Header for definitions of data types used in system calls
#include <fcntl.h>     // For file control definitions
#include <unistd.h>    // For POSIX constants, also includes read, write, close functions
#include <sys/utsname.h>
#include <sys/wait.h>

#define MAX_LINE_LENGTH 1024
#define MAX_TOKENS 100
#define STAT_PATH_MAX 512

char original_line[MAX_LINE_LENGTH];
char line[MAX_LINE_LENGTH];
char *tokens[MAX_TOKENS];
int token_count = 0;
int debug_level = 0;
int last_command_status = 0;
char current_prompt[10] = "mysh";
char procfs_path[PATH_MAX] = "/proc";
char stat_path[STAT_PATH_MAX];

typedef int (*command_func_t)(int argc, char **argv);

typedef struct
{
    char *name;
    command_func_t func;
    char *help;
} command_t;

typedef struct
{
    pid_t pid;
    pid_t ppid;
    char state;
    char name[64];
} proc_info_t;

int cmd_debug(int argc, char **argv);
int cmd_exit(int argc, char **argv);
int cmd_prompt(int argc, char **argv);
int cmd_status(int argc, char **argv);
int cmd_help(int argc, char **argv);
int cmd_print(int argc, char **argv);
int cmd_echo(int argc, char **argv);
int cmd_len(int argc, char **argv);
int cmd_sum(int argc, char **argv);
int cmd_calc(int argc, char **argv);
int cmd_basename(int argc, char **argv);
int cmd_dirname(int argc, char **argv);
int cmd_dirch(int argc, char **argv);
int cmd_dirwd(int argc, char **argv);
int cmd_dirmk(int argc, char **argv);
int cmd_dirrm(int argc, char **argv);
int cmd_dirls(int argc, char **argv);
int cmd_rename(int argc, char **argv);
int cmd_unlink(int argc, char **argv);
int cmd_remove(int argc, char **argv);
int cmd_linkhard(int argc, char **argv);
int cmd_linksoft(int argc, char **argv);
int cmd_linkread(int argc, char **argv);
int cmd_linklist(int argc, char **argv);
int cmd_cpcat(int argc, char **argv);
int cmd_pid(int argc, char **argv);
int cmd_ppid(int argc, char **argv);
int cmd_uid(int argc, char **argv);
int cmd_euid(int argc, char **argv);
int cmd_gid(int argc, char **argv);
int cmd_egid(int argc, char **argv);
int cmd_sysinfo(int argc, char **argv);
int execute_builtin(int argc, char **argv, int input_fd, int output_fd, int background);
int execute_external(int argc, char **argv, int input_fd, int output_fd, int background);
int cmd_proc(int argc, char **argv);
int cmd_pids(int argc, char **argv);
int cmd_pinfo(int argc, char **argv);

command_t commands[] = {
    {"debug", cmd_debug, "Set or display the debug level"},
    {"exit", cmd_exit, "Exit the shell with an optional status"},
    {"prompt", cmd_prompt, "Print or set the command prompt"},
    {"status", cmd_status, "Display the status of the last executed command"},
    {"help", cmd_help, "Print a list of supported commands"},
    {"print", cmd_print, "Prints the given arguments to standard output (without a final jump to a new line)"},
    {"echo", cmd_echo, "Like the print command, except that it prints a jump to a new line"},
    {"len", cmd_len, "Print the total length of all arguments (as strings)"},
    {"sum", cmd_sum, "Adds its arguments (integers) and prints the sum"},
    {"calc", cmd_calc, "Performs an operation on two integers and prints the result"},
    {"basename", cmd_basename, "Prints the basename of the given path"},
    {"dirname", cmd_dirname, "Prints the directory name of the given path"},
    {"dirch", cmd_dirch, "Change the current working directory"},
    {"dirwd", cmd_dirwd, "Print the current working directory in full or basename form"},
    {"dirmk", cmd_dirmk, "Create a directory"},
    {"dirrm", cmd_dirrm, "Remove a directory"},
    {"dirls", cmd_dirls, "List contents of a directory"},
    {"rename", cmd_rename, "Rename the source file to the sink"},
    {"unlink", cmd_unlink, "Remove the file with the given name"},
    {"remove", cmd_remove, "Removes a file or directory entry with the given name"},
    {"linkhard", cmd_linkhard, "Creates a hard link with the given name to the target"},
    {"linksoft", cmd_linksoft, "Creates a symbolic link with the given name to the target"},
    {"linkread", cmd_linkread, "Prints the target of the given symbolic link"},
    {"linklist", cmd_linklist, "Searches the directory for all hard links to a file with the given name"},
    {"cpcat", cmd_cpcat, "Combines the functionality of cp and cat"},
    {"pid", cmd_pid, "Print the PID of the shell process"},
    {"ppid", cmd_ppid, "Print the PID of the parent shell process"},
    {"uid", cmd_uid, "Print the UID of the user who owns the shell process"},
    {"euid", cmd_euid, "Print the UID of the user who is the current owner of the shell process"},
    {"gid", cmd_gid, "Print the GID of the group to which the shell process belongs"},
    {"egid", cmd_egid, "Print the GID of the group to which the shell process currently belongs"},
    {"sysinfo", cmd_sysinfo, "Print basic system information"},
    {"proc", cmd_proc, "Set or print the path to the procfs filesystem"},
    {"pids", cmd_pids, "Print the PIDs of the current processes"},
    {"pinfo", cmd_pinfo, "Print information about the current processes"},
    {NULL, NULL, NULL}};

int proc_info_cmp(const void *a, const void *b)
{
    const proc_info_t *pa = (const proc_info_t *)a;
    const proc_info_t *pb = (const proc_info_t *)b;
    return pa->pid - pb->pid;
}

int cmd_pinfo(int argc, char **argv)
{
    char dir_path[PATH_MAX];
    snprintf(dir_path, sizeof(dir_path), "%s", procfs_path);
    DIR *dir = opendir(dir_path);
    if (dir == NULL)
    {
        perror("opendir");
        return errno;
    }

    proc_info_t *proc_infos = NULL;
    int count = 0;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        char *endptr;
        long pid = strtol(entry->d_name, &endptr, 10);
        if (*endptr == '\0' && pid > 0)
        {
            char stat_path[PATH_MAX];
            snprintf(stat_path, STAT_PATH_MAX, "%s/%d/stat", dir_path, (int)pid);
            FILE *stat_file = fopen(stat_path, "r");
            if (stat_file != NULL)
            {
                proc_info_t info;
                info.pid = pid;
                fscanf(stat_file, "%*d (%63[^)]) %c %d", info.name, &info.state, &info.ppid);
                fclose(stat_file);

                proc_infos = realloc(proc_infos, (count + 1) * sizeof(proc_info_t));
                proc_infos[count++] = info;
            }
        }
    }

    closedir(dir);

    if (proc_infos != NULL)
    {
        qsort(proc_infos, count, sizeof(proc_info_t), proc_info_cmp);
        printf("%5s %5s %6s %s\n", "PID", "PPID", "STANJE", "IME");
        for (int i = 0; i < count; i++)
        {
            printf("%5d %5d %6c %s\n", proc_infos[i].pid, proc_infos[i].ppid, proc_infos[i].state, proc_infos[i].name);
        }
        free(proc_infos);
    }

    return 0;
}
int compare_pids(const void *a, const void *b)
{
    return *(pid_t *)a - *(pid_t *)b;
}

int cmd_pids(int argc, char **argv)
{
    char dir_path[PATH_MAX];
    snprintf(dir_path, sizeof(dir_path), "%s", procfs_path);
    DIR *dir = opendir(dir_path);
    if (dir == NULL)
    {
        perror("opendir");
        return errno;
    }

    pid_t *pids = NULL;
    int count = 0;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        char *endptr;
        long pid = strtol(entry->d_name, &endptr, 10);
        if (*endptr == '\0' && pid > 0)
        {
            pids = realloc(pids, (count + 1) * sizeof(pid_t));
            pids[count++] = pid;
        }
    }

    closedir(dir);

    if (pids != NULL)
    {
        qsort(pids, count, sizeof(pid_t), compare_pids);
        for (int i = 0; i < count; i++)
        {
            printf("%ld\n", (long)pids[i]);
        }
        free(pids);
    }

    return 0;
}
int cmd_proc(int argc, char **argv)
{
    if (argc == 1)
    {
        printf("%s\n", procfs_path);
    }
    else
    {
        if (access(argv[1], F_OK | R_OK) == 0)
        {
            strncpy(procfs_path, argv[1], sizeof(procfs_path) - 1);
            procfs_path[sizeof(procfs_path) - 1] = '\0';
        }
        else
        {
            return 1;
        }
    }
    return 0;
}
// Function to print the PID of the shell process
int cmd_pid(int argc, char **argv)
{
    printf("%d\n", getpid());
    return 0;
}

// Function to print the PID of the parent shell process
int cmd_ppid(int argc, char **argv)
{
    printf("%d\n", getppid());
    return 0;
}

// Function to print the UID of the user who owns the shell process
int cmd_uid(int argc, char **argv)
{
    printf("%d\n", getuid());
    return 0;
}

// Function to print the UID of the user who is the current owner of the shell process
int cmd_euid(int argc, char **argv)
{
    printf("%d\n", geteuid());
    return 0;
}

// Function to print the GID of the group to which the shell process belongs
int cmd_gid(int argc, char **argv)
{
    printf("%d\n", getgid());
    return 0;
}

// Function to print the GID of the group to which the shell process currently belongs
int cmd_egid(int argc, char **argv)
{
    printf("%d\n", getegid());
    return 0;
}

// Function to print basic system information
int cmd_sysinfo(int argc, char **argv)
{
    struct utsname info;
    if (uname(&info) != 0)
    {
        perror("uname");
        return errno;
    }
    printf("Sysname: %s\n", info.sysname);
    printf("Nodename: %s\n", info.nodename);
    printf("Release: %s\n", info.release);
    printf("Version: %s\n", info.version);
    printf("Machine: %s\n", info.machine);
    return 0;
}

int cmd_status(int argc, char **argv)
{
    printf("%d\n", last_command_status);
    return last_command_status;
}

int cmd_prompt(int argc, char **argv)
{
    if (argc == 1)
    {
        printf("%s\n", current_prompt);
    }
    else if (strlen(argv[1]) > 8)
    {
        return 1;
    }
    else
    {
        strncpy(current_prompt, argv[1], sizeof(current_prompt) - 1);
        current_prompt[sizeof(current_prompt) - 1] = '\0';
    }
    return 0;
}

int cmd_debug(int argc, char **argv)
{
    if (argc > 1)
    {
        if (strcmp(argv[1], "off") == 0)
        {
            debug_level = 0;
        }
        else
        {
            char *endptr;
            long int new_level = strtol(argv[1], &endptr, 10);
            if (*endptr == '\0' && new_level >= 0 && new_level <= INT_MAX)
            {
                debug_level = (int)new_level;
            }
            else
            {
                return 1;
            }
        }
    }
    else
    {
        printf("%d\n", debug_level);
    }
    return 0;
}

int cmd_exit(int argc, char **argv)
{
    int exit_status = last_command_status;
    if (argc > 1)
    {
        char *endptr;
        long int arg_status = strtol(argv[1], &endptr, 10);
        if (*endptr == '\0' && arg_status >= 0 && arg_status <= INT_MAX)
        {
            exit_status = (int)arg_status;
        }
        else
        {
            fprintf(stderr, "Invalid exit status: %s\n", argv[1]);
            exit_status = 1;
        }
    }
    exit(exit_status);
}

int cmd_help(int argc, char **argv)
{
    printf("Supported commands:\n");
    for (command_t *cmd = commands; cmd->name != NULL; cmd++)
    {
        printf("  %s: %s\n", cmd->name, cmd->help);
    }
    return 0;
}

int cmd_print(int argc, char **argv)
{
    for (int i = 1; i < argc; i++)
    {
        printf("%s", argv[i]);
        if (i != argc - 1)
        {
            printf(" ");
        }
    }
    return 0;
}

int cmd_echo(int argc, char **argv)
{
    cmd_print(argc, argv);
    printf("\n");
    return 0;
}

int cmd_len(int argc, char **argv)
{
    int total_length = 0;
    for (int i = 1; i < argc; i++)
    {
        total_length += strlen(argv[i]);
    }
    printf("%d\n", total_length);
    return 0;
}

int cmd_sum(int argc, char **argv)
{
    int sum = 0;
    for (int i = 1; i < argc; i++)
    {
        char *endptr;
        long int value = strtol(argv[i], &endptr, 10);
        if (*endptr != '\0')
        {
            fprintf(stderr, "Invalid integer argument: %s\n", argv[i]);
            return 1;
        }
        sum += value;
    }
    printf("%d\n", sum);
    return 0;
}

int cmd_calc(int argc, char **argv)
{
    if (argc != 4)
    {
        fprintf(stderr, "Usage: calc arg1 op arg2\n");
        return 1;
    }
    char *endptr1, *endptr2;
    long int arg1 = strtol(argv[1], &endptr1, 10);
    long int arg2 = strtol(argv[3], &endptr2, 10);
    if (*endptr1 != '\0' || *endptr2 != '\0')
    {
        fprintf(stderr, "Invalid integer argument\n");
        return 1;
    }
    int result;
    char op = argv[2][0];
    switch (op)
    {
    case '+':
        result = arg1 + arg2;
        break;
    case '-':
        result = arg1 - arg2;
        break;
    case '*':
        result = arg1 * arg2;
        break;
    case '/':
        if (arg2 == 0)
        {
            fprintf(stderr, "Division by zero\n");
            return 1;
        }
        result = arg1 / arg2;
        break;
    case '%':
        if (arg2 == 0)
        {
            fprintf(stderr, "Modulus by zero\n");
            return 1;
        }
        result = arg1 % arg2;
        break;
    default:
        fprintf(stderr, "Invalid operation: %c\n", op);
        return 1;
    }
    printf("%d\n", result);
    return 0;
}

int cmd_basename(int argc, char **argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: basename path\n");
        return 1;
    }
    printf("%s\n", basename(argv[1]));
    return 0;
}

int cmd_dirname(int argc, char **argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: dirname path\n");
        return 1;
    }
    printf("%s\n", dirname(argv[1]));
    return 0;
}
int cmd_dirch(int argc, char **argv)
{
    char *dir = (argc > 1) ? argv[1] : "/";
    errno = 0;
    if (chdir(dir) != 0)
    {
        int chdir_err = errno;
        fflush(stdout);
        perror("dirch");
        last_command_status = chdir_err;
        return chdir_err;
    }
    last_command_status = 0;
    return 0;
}

int cmd_dirwd(int argc, char **argv)
{
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) == NULL)
    {
        perror("dirwd");
        fflush(stdout);
        last_command_status = errno;
        return errno;
    }

    if (argc == 1 || strcmp(argv[1], "base") == 0)
    {
        printf("%s\n", basename(cwd));
        fflush(stdout);
    }
    else if (strcmp(argv[1], "full") == 0)
    {
        printf("%s\n", cwd);
        fflush(stdout);
    }
    else
    {
        fprintf(stderr, "Usage: dirwd [base|full]\n");
        fflush(stdout);
        last_command_status = EINVAL;
        return EINVAL;
    }
    last_command_status = 0;
    fflush(stdout);
    return 0;
}
int cmd_dirmk(int argc, char **argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: dirmk directory\n");
        fflush(stdout);
        last_command_status = EINVAL;
        return EINVAL;
    }

    errno = 0;
    if (mkdir(argv[1], 0755) != 0)
    {
        int mk_err = errno;
        fflush(stdout);
        perror("dirmk");
        last_command_status = mk_err;
        return mk_err;
    }

    last_command_status = 0;
    return 0;
}
int cmd_dirrm(int argc, char **argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: dirrm directory\n");
        last_command_status = EINVAL;
        return EINVAL;
    }

    errno = 0;
    if (rmdir(argv[1]) != 0)
    {
        int saved_errno = errno;
        fflush(stdout);

        last_command_status = saved_errno;
        return saved_errno;
    }

    last_command_status = 0;
    return 0;
}

int cmd_dirls(int argc, char **argv)
{
    char *dir = (argc > 1) ? argv[1] : ".";
    DIR *dp;
    struct dirent *entry;
    if ((dp = opendir(dir)) == NULL)
    {
        perror("dirls");
        last_command_status = errno;
        return errno;
    }

    while ((entry = readdir(dp)) != NULL)
    {
        printf("%s  ", entry->d_name);
    }
    printf("\n");
    closedir(dp);
    last_command_status = 0;
    return 0;
}

command_t *find_builtin(char *cmd)
{
    for (command_t *p = commands; p->name != NULL; p++)
    {
        if (strcmp(cmd, p->name) == 0)
        {
            return p;
        }
    }
    return NULL;
}

int execute_builtin(int argc, char **argv, int input_fd, int output_fd, int background)
{
    command_t *cmd = find_builtin(argv[0]);
    if (cmd)
    {
        if (debug_level > 0)
        {
            printf("Executing builtin '%s' in foreground\n", argv[0]);
        }

        if (background)
        {
            if (fork() == 0)
            {
                if (input_fd != STDIN_FILENO)
                {
                    dup2(input_fd, STDIN_FILENO);
                    close(input_fd);
                }
                if (output_fd != STDOUT_FILENO)
                {
                    dup2(output_fd, STDOUT_FILENO);
                    close(output_fd);
                }
                int status = cmd->func(argc, argv);
                exit(status);
            }
            return 0;
        }
        else
        {
            fflush(stdout);
            int original_stdin = dup(STDIN_FILENO);
            int original_stdout = dup(STDOUT_FILENO);

            if (input_fd != STDIN_FILENO)
            {
                dup2(input_fd, STDIN_FILENO);
                close(input_fd);
            }
            if (output_fd != STDOUT_FILENO)
            {
                dup2(output_fd, STDOUT_FILENO);
                close(output_fd);
            }

            int status = cmd->func(argc, argv);
            fflush(stdout);

            dup2(original_stdin, STDIN_FILENO);
            dup2(original_stdout, STDOUT_FILENO);
            close(original_stdin);
            close(original_stdout);

            if (strcmp(argv[0], "status") != 0)
            {
                last_command_status = status;
            }
            return status;
        }
    }
    return -1;
}

int cmd_rename(int argc, char **argv)
{
    if (argc != 3)
    {
        fprintf(stderr, "Usage: rename source sink\n");
        return EINVAL;
    }
    if (rename(argv[1], argv[2]) != 0)
    {
        perror("rename");
        return errno;
    }
    return 0;
}

int cmd_unlink(int argc, char **argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: unlink name\n");
        return EINVAL;
    }
    if (unlink(argv[1]) != 0)
    {
        perror("unlink");
        return errno;
    }
    return 0;
}

int cmd_remove(int argc, char **argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: remove name\n");
        return EINVAL;
    }
    if (remove(argv[1]) != 0)
    {
        perror("remove");
        return errno;
    }
    return 0;
}

int cmd_linkhard(int argc, char **argv)
{
    if (argc != 3)
    {
        fprintf(stderr, "Usage: linkhard target name\n");
        return EINVAL;
    }
    if (link(argv[1], argv[2]) != 0)
    {
        perror("linkhard");
        return errno;
    }
    return 0;
}

int cmd_linksoft(int argc, char **argv)
{
    if (argc != 3)
    {
        fprintf(stderr, "Usage: linksoft target name\n");
        return EINVAL;
    }
    if (symlink(argv[1], argv[2]) != 0)
    {
        perror("linksoft");
        return errno;
    }
    return 0;
}

int cmd_linkread(int argc, char **argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: linkread name\n");
        return EINVAL;
    }
    char buf[1024];
    ssize_t len;
    if ((len = readlink(argv[1], buf, sizeof(buf) - 1)) == -1)
    {
        perror("linkread");
        return errno;
    }
    buf[len] = '\0';
    printf("%s\n", buf);
    return 0;
}

int cmd_linklist(int argc, char **argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: linklist name\n");
        return EINVAL;
    }
    DIR *dirp;
    struct dirent *entry;
    struct stat statbuf, linkstat;
    if (lstat(argv[1], &linkstat) != 0)
    {
        perror("Failed to get stat of the target file");
        return errno;
    }

    if ((dirp = opendir(".")) == NULL)
    {
        perror("Failed to open directory");
        return errno;
    }

    while ((entry = readdir(dirp)) != NULL)
    {
        lstat(entry->d_name, &statbuf);
        if (statbuf.st_ino == linkstat.st_ino)
        {
            printf("%s  ", entry->d_name);
        }
    }
    closedir(dirp);
    printf("\n");
    return 0;
}

int cmd_cpcat(int argc, char **argv)
{
    int input_fd, output_fd;
    char buffer[1024];
    ssize_t bytes_read, bytes_written;

    input_fd = STDIN_FILENO;
    output_fd = STDOUT_FILENO;

    if (argc > 1 && strcmp(argv[1], "-") != 0)
    {
        input_fd = open(argv[1], O_RDONLY);
        if (input_fd == -1)
        {
            if (errno == ENOENT)
            {
                fprintf(stderr, "cpcat: No such file or directory\n");
                last_command_status = 2;
                return 2;
            }
            else
            {
                fflush(stdout);
                perror("cpcat failed to open source file");
                last_command_status = errno;
                return errno;
            }
        }
    }

    if (argc > 2 && strcmp(argv[2], "-") != 0)
    {
        output_fd = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (output_fd == -1)
        {
            fflush(stdout);
            perror("cpcat failed to open/create destination file");
            if (input_fd != STDIN_FILENO)
                close(input_fd);
            return errno;
        }
    }

    while ((bytes_read = read(input_fd, buffer, sizeof(buffer))) > 0)
    {
        bytes_written = write(output_fd, buffer, bytes_read);
        if (bytes_written != bytes_read)
        {
            fflush(stdout);
            perror("cpcat failed to write all bytes to destination");
            if (input_fd != STDIN_FILENO)
                close(input_fd);
            if (output_fd != STDOUT_FILENO)
                close(output_fd);
            return errno;
        }
    }

    if (bytes_read == -1)
    {
        fflush(stdout);
        perror("cpcat failed to read from source file");
        if (input_fd != STDIN_FILENO)
            close(input_fd);
        if (output_fd != STDOUT_FILENO)
            close(output_fd);
        return errno;
    }

    if (input_fd != STDIN_FILENO)
        close(input_fd);
    if (output_fd != STDOUT_FILENO)
        close(output_fd);

    return 0;
}

int execute_external(int argc, char **argv, int input_fd, int output_fd, int background)
{
    pid_t pid = fork();
    if (pid == 0)
    { // Child process
        if (input_fd != STDIN_FILENO)
        {
            dup2(input_fd, STDIN_FILENO);
            close(input_fd);
        }
        if (output_fd != STDOUT_FILENO)
        {
            dup2(output_fd, STDOUT_FILENO);
            close(output_fd);
        }

        execvp(argv[0], argv);
        fprintf(stderr, "Failed to execute %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    else if (pid > 0)
    { // Parent process
        if (!background)
        {
            int status;
            waitpid(pid, &status, 0); // Wait only if not a background process
            return WEXITSTATUS(status);
        }
    }
    return 0; // If background, return immediately
}

void tokenize_line()
{
    char *p = line;
    char *start;
    int in_quote = 0;
    token_count = 0;

    while (*p)
    {

        while (isspace(*p))
            p++;

        if (*p == '#')
        {
            while (*p && *p != '\n')
                p++;
            continue;
        }

        if (!*p)
            break;

        if (*p == '"')
        {
            in_quote = 1;
            p++;
            start = p;
        }
        else
        {
            start = p;
        }

        while (*p && (in_quote || !isspace(*p)))
        {
            if (*p == '"' && in_quote)
            {
                *p++ = '\0';
                break;
            }
            p++;
        }

        tokens[token_count++] = start;

        if (in_quote)
        {
            in_quote = 0;
        }
        else if (*p)
        {
            *p++ = '\0';
        }
    }
}

void parse_special_symbols(int *input_fd, int *output_fd, int *background)
{
    *input_fd = STDIN_FILENO;
    *output_fd = STDOUT_FILENO;
    *background = 0;

    for (int i = 0; i < token_count; i++)
    {
        if (tokens[i][0] == '<')
        {
            *input_fd = open(tokens[i] + 1, O_RDONLY);
            if (*input_fd == -1)
            {
                perror("Failed to open input file");
                *input_fd = STDIN_FILENO;
            }
            tokens[i] = NULL;
        }
        else if (tokens[i][0] == '>')
        {
            *output_fd = open(tokens[i] + 1, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (*output_fd == -1)
            {
                perror("Failed to open output file");
                *output_fd = STDOUT_FILENO;
            }
            tokens[i] = NULL;
        }
        else if (tokens[i][0] == '&')
        {
            *background = 1;
            tokens[i] = NULL;
        }
    }

    // Remove NULL tokens
    int shift = 0;
    for (int i = 0; i < token_count; i++)
    {
        if (tokens[i] == NULL)
        {
            shift++;
        }
        else if (shift > 0)
        {
            tokens[i - shift] = tokens[i];
        }
    }
    token_count -= shift;
}

void redirect_and_execute(int input_fd, int output_fd, command_func_t func, int argc, char **argv)
{
    fflush(stdout); // Flush stdout to avoid any print overlap
    int original_stdin = dup(STDIN_FILENO);
    int original_stdout = dup(STDOUT_FILENO);

    // Setup redirection
    if (input_fd != STDIN_FILENO)
    {
        dup2(input_fd, STDIN_FILENO);
        close(input_fd);
    }
    if (output_fd != STDOUT_FILENO)
    {
        dup2(output_fd, STDOUT_FILENO);
        close(output_fd);
    }

    // Execute command
    func(argc, argv);

    // Restore original file descriptors
    dup2(original_stdin, STDIN_FILENO);
    dup2(original_stdout, STDOUT_FILENO);
    close(original_stdin);
    close(original_stdout);
}

int main()
{
    int interactive = isatty(STDIN_FILENO);

    while (1)
    {
        if (interactive)
        {
            printf("%s> ", current_prompt);
            fflush(stdout);
        }

        if (!fgets(line, MAX_LINE_LENGTH, stdin))
        {
            if (interactive)
            {
                printf("\n");
            }
            if (last_command_status != 0)
            {
                printf("Exit status: %d\n", last_command_status);
            }
            break;
        }

        strcpy(original_line, line);

        char *newline = strchr(line, '\n');
        if (newline)
            *newline = '\0';

        if (line[0] == '\0' || line[0] == '#')
            continue;

        tokenize_line();

        if (token_count == 0)
            continue;

        int input_fd, output_fd, background;
        parse_special_symbols(&input_fd, &output_fd, &background);

        int builtin_status = execute_builtin(token_count, tokens, input_fd, output_fd, background);
        if (builtin_status == -1)
        {
            last_command_status = execute_external(token_count, tokens, input_fd, output_fd, background);
        }
    }

    return 0;
}
