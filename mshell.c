#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <signal.h>
#include <dirent.h>
#include <sys/stat.h>
#include <ctype.h>
#include <libgen.h>

#define MAX_INPUT_SIZE 1024
#define MAX_PATH_SIZE 256
#define MAX_ARGS 64

static const char* COMMANDS[8] = {"cd [dir]", "ls [-a] [dir ...]", "cat [file]", "pwd", "echo [arg ...]", "help", "exit", NULL};
static pid_t pid;

void parse_input(char*, char**);
void handle_cd(char*, char**);
void handle_echo(char**);
void handle_exit(char**);
void handle_help(char**);
void handle_pwd(char*, char**);
void handle_exec(char**);
void handle_cat(char*, char**);
void handle_ls2(char*, int);
void handle_ls(char*, char**); 

void sigint_callback(int);

void display_prompt(char* path)
{
	printf("[%s] $ ", path);
}

int main()
{
	char input[MAX_INPUT_SIZE];
	char *args[MAX_ARGS];
	char path[MAX_PATH_SIZE];
	
	if (getcwd(path, MAX_PATH_SIZE) == NULL) 
	{
        perror("getcwd");
    }
	signal(SIGINT, sigint_callback);

	while (1)
	{
		pid=1;
		display_prompt(path);
		fgets(input, MAX_INPUT_SIZE-1, stdin);

		parse_input(input, args);

		if (args[0] == NULL){
			continue;
		}
		if (strcmp(args[0], "^C") == 0)
		{
			continue;
		}
		else if (strcmp(args[0], "cd") == 0)
		{
			handle_cd(path, args);
		}
		else if (strcmp(args[0], "echo") == 0)
		{
			handle_echo(args);
		}
		else if (strcmp(args[0], "pwd") == 0)
		{
			handle_pwd(path, args);
		}
		else if (strcmp(args[0], "help") == 0)
		{
			handle_help(args);
		}
		else if (strcmp(args[0], "exit") == 0)
		{
			handle_exit(args);
		}
		else if (strcmp(args[0], "cat") == 0)
		{
			handle_cat(path, args);
		}
		else if (strcmp(args[0], "ls") == 0)
		{
			handle_ls(path, args);
		}
		else 
		{
			handle_exec(args);
		}
	}

	return 0;
}

void parse_input(char *input, char **args) 
{
    int i = 0;
    int in_quotes = 0;
    
    while (*input)
    {
        while (*input == ' ' || *input == '\t' || *input == '\n')
            input++;
        
        if (*input == '\0')
            break;
        
        if (*input == '"') 
        {
            in_quotes = !in_quotes;
            input++; 
            args[i++] = input;
        } 
        else 
        {
            args[i++] = input;
        }
        
        while (*input)
        {
            if (*input == '"') 
            {
                in_quotes = !in_quotes;
                *input = '\0';
                input++;
                break;
            }
            
            if (!in_quotes && (*input == ' ' || *input == '\t' || *input == '\n'))
            {
                *input = '\0';
                input++;
                break;
            }
            
            input++;
        }
    }
    
    args[i] = NULL;
}

void handle_cd(char* path, char** args)
{
	char *home = getenv("HOME");

    if (args[1] == NULL) 
	{

        if (home != NULL) 
		{
            if (chdir(home) == 0) 
			{
                if (getcwd(path, MAX_PATH_SIZE) == NULL) 
				{
                    perror("getcwd");
                }
            } else 
			{
                perror("cd");
            }
        } else 
		{
            fprintf(stderr, "cd: No home directory found\n");
        }
    } 
    else 
	{
		if (strcmp(args[1], "~") == 0)
		{
			if (chdir(home) == 0) 
			{
            	if (getcwd(path, MAX_PATH_SIZE) == NULL) 
				{
                	perror("getcwd");
				}
			}
		}
        else if (chdir(args[1]) == 0) 
		{
            if (getcwd(path, MAX_PATH_SIZE) == NULL) 
			{
                perror("getcwd");
            }
        } 
		else 
		{
            perror("cd");
        }
    }
}

void handle_echo(char** args)
{
	for (int i = 1; args[i] != NULL; i++)
    {
        printf("%s ", args[i]);
    }
	printf("\n");
}

void handle_exec(char** args)
{
	pid = fork();
	if (pid == 0)
	{

		if (execvp(args[0], args) == -1)
		{
			perror(NULL);
			fprintf(stderr, "Invalid command\n");
			fprintf(stderr, "Commands:\n");
			int i = 0;
			while (COMMANDS[i] != NULL)
			{
				printf("%s\n", COMMANDS[i]);
				i++;
			}
			exit(0);
		}
	}
	else
	{
		wait(NULL);
	}

}

void handle_ls2(char* full_path, int is_all)
{
	struct dirent* entry;
	DIR* dir;

	struct stat statbuf;
    if (stat(full_path, &statbuf) == 0 && S_ISREG(statbuf.st_mode)) {
        printf("%s\n", basename(full_path));
        return;
    }

    dir = opendir(full_path);
    if (dir == NULL) {
        fprintf(stderr, "ls: cannot access '%s': No such file or directory\n", full_path);
        return;
    }

	printf("%s:\n", full_path);

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.' && !is_all) {
            continue;
        }
        printf("%s\n", entry->d_name);
    }

    closedir(dir);
}

void handle_ls(char* path, char** args) 
{
    char full_path[1024];

	int is_all = 0;
	int i = 1;
	while (args[i] != NULL)
	{
		if (strcmp(args[i], "-a") == 0)
		{
			is_all = 1;
			break;
		}
		i++;
	}

	if (args[1] == NULL || (strcmp(args[1], "-a") == 0 && args[2] == NULL))
	{
			strncpy(full_path, path, sizeof(full_path) - 1);
			handle_ls2(full_path, is_all);
			return;
	}

	i = 1;
	while (args[i] != NULL)
	{
    	if (strcmp(args[i], "-a") != 0) 
		{
			if (args[i][0] == '/') 
			{
				strncpy(full_path, args[i], sizeof(full_path) - 1);
			} 
			else 
			{
				snprintf(full_path, sizeof(full_path), "%s/%s", path, args[i]);
			}
			handle_ls2(full_path, is_all);
		} 
		i++;
	}
}


void handle_cat(char* path, char** args)
{
    if (args[1] == NULL) 
	{
        fprintf(stderr, "Usage: cat <filename>\n");
        return;
    }

    char full_path[MAX_PATH_SIZE];

    if (args[1][0] == '/') 
	{
        snprintf(full_path, sizeof(full_path), "%s", args[1]);
    } 
	else 
	{
        snprintf(full_path, sizeof(full_path), "%s/%s", path, args[1]);
    }

    struct stat path_stat;
    if (stat(full_path, &path_stat) == 0) 
	{
        if (S_ISDIR(path_stat.st_mode)) 
		{
            fprintf(stderr, "Error: '%s' is a directory\n", full_path);
            return;
        }
    } 
	else 
	{
        perror("Error checking file status");
        return;
    }

    FILE *file = fopen(full_path, "r");

    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), file)) 
	{	
		int length = strlen(buffer);
		while (length > 0 && isspace((unsigned char)buffer[length - 1])) 
		{
			buffer[length - 1] = '\0';
			length--;
		}	
		if (length > 0)
		{
			printf("%s\n", buffer);
		}
    }

    fclose(file);
}

void handle_pwd(char* path, char** args)
{
	printf("%s\n", path);
}

void handle_exit(char** args)
{
	exit(0);
}

void handle_help(char** args)
{
	printf("Author: Wojciech Kalota\n");
	printf("Available commands:\n");
	int i = 0;
	while (COMMANDS[i] != NULL)
	{
		printf("%s\n", COMMANDS[i]);
		i++;
	}
}

void sigint_callback(int signal)
{
	if(pid==0)
	{
		exit(0);
	}
}