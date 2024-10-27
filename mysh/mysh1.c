#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>

#define MAX_LINE_LENGTH 1024
#define MAX_TOKENS 100

char original_line[MAX_LINE_LENGTH];
char line[MAX_LINE_LENGTH];
char *tokens[MAX_TOKENS];
int token_count = 0;
int debug_level = 0;
int last_command_status = 0;
char current_prompt[10] = "mysh";

typedef int (*command_func_t)(int argc, char **argv);

typedef struct
{
    char *name;
    command_func_t func;
    char *help;
} command_t;

int cmd_debug(int argc, char **argv);
int cmd_exit(int argc, char **argv);
int cmd_prompt(int argc, char **argv);
int cmd_status(int argc, char **argv);
int cmd_help(int argc, char **argv);

int execute_builtin(int argc, char **argv);
int execute_external(int argc, char **argv);

command_t commands[] = {
    {"debug", cmd_debug, "Set or display the debug level"},
    {"exit", cmd_exit, "Exit the shell with an optional status"},
    {"prompt", cmd_prompt, "Print or set the command prompt"},
    {"status", cmd_status, "Display the status of the last executed command"},
    {"help", cmd_help, "Print a list of supported commands"},
    {NULL, NULL, NULL}};

int cmd_status(int argc, char **argv)
{
    printf("%d\n", last_command_status);
    return -2;
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
                debug_level = 0;
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
    int exit_status = last_command_status; // Use last command's status by default

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

int execute_builtin(int argc, char **argv)
{
    command_t *cmd = find_builtin(argv[0]);
    if (cmd)
    {
        if (debug_level > 0)
        {
            printf("Executing builtin '%s' in foreground\n", argv[0]);
        }
        int status = cmd->func(argc, argv);
        // Update last_command_status, except for 'status' command to avoid overwriting its own result
        if (strcmp(argv[0], "status") != 0)
        {
            last_command_status = status;
        }
        return status;
    }
    return -1; // Indicate that no built-in command was found
}
int execute_external(int argc, char **argv)
{
    if (debug_level > 0)
    {
        char debug_output[MAX_LINE_LENGTH] = {0};
        int included_args = 0;

        for (int i = 0; i < argc; ++i)
        {

            if (argv[i][0] == '>' || argv[i][0] == '<')
            {
                break;
            }

            // Append the token to the debug string, checking for buffer overflow
            if (included_args > 0)
            {
                strncat(debug_output, " ", sizeof(debug_output) - strlen(debug_output) - 1);
            }
            strncat(debug_output, argv[i], sizeof(debug_output) - strlen(debug_output) - 1);
            included_args++;
        }

        printf("External command '%s'\n", debug_output);
    }

    return 0;
}

void tokenize_line()
{
    char *p = line;
    char *start;
    int in_quote = 0;

    token_count = 0;

    while (*p)
    {
        // Skip whitespace
        while (isspace(*p))
            p++;

        // Skip comments
        if (*p == '#')
        {
            while (*p && *p != '\n')
                p++;
            continue;
        }

        if (!*p)
            break; // End of line

        // Check for quoted string
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

        // Scan for end of token
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

void parse_special_symbols()
{
    char *input_redirect = NULL;
    char *output_redirect = NULL;
    int background = 0;

    // Check the last three tokens for special symbols
    for (int i = token_count - 1; i >= 0 && i >= token_count - 3; --i)
    {
        if (tokens[i][0] == '<')
        {
            input_redirect = tokens[i] + 1;
        }
        else if (tokens[i][0] == '>')
        {
            output_redirect = tokens[i] + 1;
        }
        else if (tokens[i][0] == '&')
        {
            background = 1;
        }
    }

    // Print out detected special characters
    if (debug_level > 0)
    {
        if (input_redirect)
        {
            printf("Input redirect: '%s'\n", input_redirect);
        }
        if (output_redirect)
        {
            printf("Output redirect: '%s'\n", output_redirect);
        }
        if (background)
        {
            printf("Background: 1\n");
        }
    }
}

int main()
{
    int interactive = isatty(STDIN_FILENO);

    while (1)
    {
        if (interactive)
        {
            printf("mysh> ");
            fflush(stdout);
        }

        if (fgets(line, MAX_LINE_LENGTH, stdin) == NULL)
        {
            if (interactive)
            {
                printf("\n");
            }
            break;
        }

        // Store the original line
        strcpy(original_line, line);

        // Remove trailing newline from both 'line' and 'original_line'
        char *newline = strchr(original_line, '\n');
        if (newline)
            *newline = '\0';

        newline = strchr(line, '\n');
        if (newline)
            *newline = '\0';

        // Skip empty lines and comments
        if (line[0] == '\0' || line[0] == '#')
        {
            continue;
        }

        // Check if the line contains only whitespace
        char *p = line;
        while (*p && isspace(*p))
            p++;
        if (*p == '\0')
        {

            continue;
        }

        tokenize_line();

        if (debug_level > 0)
        {
            printf("Input line: '%s'\n", original_line);
            for (int i = 0; i < token_count; i++)
            {
                printf("Token %d: '%s'\n", i, tokens[i]);
            }
        }

        parse_special_symbols();

        int builtin_status = execute_builtin(token_count, tokens);
        if (builtin_status == 0)
        {
            last_command_status = 0;
        }
        else
        {
            // Attempt to execute as an external command
            execute_external(token_count, tokens);
            last_command_status = 1;
        }
    }

    return 0;
}
