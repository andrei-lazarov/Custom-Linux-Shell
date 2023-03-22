#include     <stdio.h>
#include    <stdlib.h>
#include    <string.h>
#include    <unistd.h>
#include    <limits.h>
#include     <ctype.h>
#include <sys/types.h>
#include  <sys/stat.h>
#include  <sys/wait.h>

	#define MAX_LINE_LENGTH 200
	#define MAX_NAME 30
	
	unsigned int stop = 0;
	int exitCode;
	char cwd[PATH_MAX], username[MAX_NAME], hostname[MAX_NAME];


void setupDisplay () {
	
	gethostname (hostname, MAX_NAME);
	if (getlogin_r (username, MAX_NAME) != 0)
		strcpy (username, "user");
}

int updatecwd () {
	
	if (getcwd (cwd, sizeof(cwd)) == NULL) {
	
		perror ("getcwd");
		return -1;
	}
	return 0;
}

void display () {
	
	printf ("\033[1;32m%s@%s\033[0m:\033[1;34m%s\033[0m$ ", username, hostname, cwd);
}

char *getWord (char *line) {

	if (line == NULL)
		return NULL;

	char *word = (char*) calloc (MAX_LINE_LENGTH, sizeof (char));
	unsigned i = 0;
	while ((line[i] != ' ') && (line[i] != '\0')) {
		
		word[i] = line[i];
		i++;
	}
	word[i] = '\0';
	
	return word;
}

void removeExtraSpaces (char *str) {
	
	if (str[0] == '\0')
		return;
	
	char *dest = str;

	// trailing spaces
	while (isspace (str[strlen (str) - 1]))
		str[strlen (str) - 1] = 0;
		
	// leading spaces
	while (*str == ' ')
	 	str++;
	 	
	*dest = *str;

	// middle spaces
	// https://stackoverflow.com/questions/16790227/replace-multiple-spaces-by-single-space-in-c
	
	while (*str != '\0') {
       
		while (*str == ' ' && *(str + 1) == ' ')
			str++;
		*dest++ = *str++;
    }
    
	*dest = '\0';
}

char *getLine () {
	
	char *line = (char*) calloc (MAX_LINE_LENGTH, sizeof (char));
	size_t len = 0;
	getline (&line, &len, stdin);
	removeExtraSpaces (line);

	return line;
}

void interpret (char *line) {
	
	char *firstWord = getWord (line);

	if (strcmp (firstWord, "help") == 0) {
		
		printf ("Available commands:\n");
		printf ("  help           displays this message\n");
		printf ("  exit           closes the shell\n");
		printf ("  quit           -\n");
		printf ("  close          -\n");
		printf ("  logout         -\n");
		printf ("  cd             changes the current directory\n");
		printf ("  pwd            prints the current directory\n");
		printf ("  type           displays type of the given entry\n");
		printf ("  create         creates a new entry\n");
		printf ("  run            execute a command/program\n");
		printf ("  easyrun        execute a command/program with system()\n");
		printf ("  status         exit code of prev command\n");
		return;
	}
	
	if (firstWord[0] == '\0')
		return;
	
	if ((strcmp (firstWord, "exit")  == 0) ||
	    (strcmp (firstWord, "close") == 0) ||
		(strcmp (firstWord, "quit")  == 0) ||
		(strcmp (firstWord, "logout") == 0)) {
			
			printf ("Shell closed\n");
			stop = 1;
			return;
	}
	
	if ((strcmp (firstWord, "type") == 0)) {
		if (strcmp (firstWord, line) == 0) {
			
			printf ("  USAGE:   type <entry>\n");
			printf ("  EXAMPLE: type ./file.txt\n");
			return;
		}
		
		struct stat info;
		if (lstat (getWord (line + 5), &info) != 0) {
			
			perror ("lstat");
			return;
		}
		printf ("Target is a ");
		
		// straight out of man page
		switch (info.st_mode & S_IFMT) {
			case S_IFBLK:  printf ("block device\n");            break;
			case S_IFCHR:  printf ("character device\n");        break;
			case S_IFDIR:  printf ("directory\n");               break;
			case S_IFIFO:  printf ("FIFO/pipe\n");               break;
			case S_IFLNK:  printf ("symlink\n");                 break;
			case S_IFREG:  printf ("regular file\n");            break;
			case S_IFSOCK: printf ("socket\n");                  break;
			default:       printf ("unknown?\n");                break;
		}
		
		return;
	}
	
	if ((strcmp (firstWord, "create") == 0)) {
		
		if (strcmp (firstWord, line) == 0) {
			
			createUsage:
			printf ("  USAGE: create <type> <name> [<target>] [<dir>]\n");
			printf ("      types:\n");
			printf ("          -f regular file\n");
			printf ("          -l symbolic link\n");
			printf ("          -d directory\n");
			printf ("      target: target of symbolic link\n");
			printf ("      dir: where to create the entry\n\n");
			printf ("  EXAMPLE: create -f file.txt \n");
			
			return;
		}
		
		char *type, *name, *target, *dir;
		
		type = getWord (line + 7);
		name = getWord (line + 10);
		
		int i = 10;
		int argumentCount = 2;
		
		if (strcmp (type, "-l") == 0) {
			while ((line[i] != '\0') && (line[i] != ' '))
				i++;
			if (line[i] == ' ') {
				i++;
				argumentCount++;
				target = getWord (line + i);
			}
			
			if (target == NULL) return;
			
			while ((line[i] != '\0') && (line[i] != ' '))
				i++;
			if (line[i] == ' ') {
				i++;
				argumentCount++;
				dir = getWord (line + i);
			}
			
			if (argumentCount == 4) {
				
				char fullName[MAX_LINE_LENGTH];
				strcpy (fullName, dir);
				strcat (fullName, "/");
				strcat (fullName, name);
				if (symlink (target, fullName) != 0) perror ("create");
				return;
			}
			
			if (symlink (target, name) != 0) perror ("create");
			return;
		}
		
		while ((line[i] != '\0') && (line[i] != ' '))
			i++;
		
		if (line[i] == ' ') {
			
			i++;
			argumentCount++;
			dir = getWord (line + i);
		}
		
		if (strcmp (type, "-d") == 0) {
		
			if (argumentCount == 3) {
				
				if (chdir (dir) == -1) {perror ("cd"); return;}
				if (mkdir (name, 0700) != 0) perror ("create");
				if (chdir (cwd) == -1) perror ("cd");
				return;
			}
			else
				if (mkdir (name, 0700) != 0)
					perror ("create");
		
			return;
		}
		
		if (strcmp (type, "-f") == 0) {
		
			if (argumentCount == 3) 
				if (chdir (dir) == -1) {perror ("cd"); return;}

			FILE *file = fopen (name, "w");
			if (file == NULL) perror ("create");
				
			if (argumentCount == 3)	
				if (chdir (cwd) == -1) perror ("cd");
			
			return;
		}
		
		goto createUsage;
		return;
	}
	
	if ((strcmp (firstWord, "pwd") == 0)) {
		
		printf("Current working dir: %s\n", cwd);
		return;
	}
	
	if (strcmp (firstWord, "cd") == 0) {
		
		if (strcmp (firstWord, line) == 0) {
			
			printf ("  USAGE:   cd <directory>\n");
			printf ("  EXAMPLE: cd ./NewFolder\n");
			return;
		}
		
		if (chdir (getWord (line + 3)) == -1)
			perror ("cd");
		
		return;
	}
	
	if (strcmp (firstWord, "run") == 0) {
		
		if (strcmp (firstWord, line) == 0) {
			printf ("  USAGE:   run <cmd> <arg1> <arg2> ... <argN>\n");
			printf ("  EXAMPLE: run wc -l shell.c\n");
			return;
		}
		
		char *arg, *argv[10], *argv2[10];
		int argc = 0, argc2 = 0;
		int lineLen = strlen (line);
		int foundPipe = 0;
		int offset = 4;	// "run "
		arg = getWord (line + offset);
		while (arg != NULL) {
		
			if (foundPipe == 0) {
				if ((arg[0] == '|') && (arg[1] == '\0')) {
					foundPipe = 1;
				}
				else {
					argv[argc] = arg;
					argc++;
				}
			}
			else {
				argv2[argc2] = arg;
				argc2++;
			}
			
			offset = offset + strlen (arg) + 1;
			
			if (offset < lineLen)
				arg = getWord (line + offset);
			else
				arg = NULL;
		}
		
		argv[argc]   = NULL;
		argv2[argc2] = NULL;
		
		int pfd[2];
		if (pipe (pfd) < 0) {
			perror ("Pipe creation error\n");
			exit (1);
		}
		
			char command[MAX_NAME];
			strcpy (command, "/bin/");
			strcat (command, argv[0]);

			pid_t pid;
			int wstatus;		
			if ((pid = fork ()) < 0)
				perror (argv[0]);
				
			if (pid==0) {
				
				if (foundPipe == 1) {
					if (dup2 (pfd[1], 1) == -1)
						perror ("dup2");
				}
				exitCode = execvp (argv[0], argv);
				if (exitCode != 0)
					perror ("execvp");
				exit (0);
			}
			
			wait (&wstatus);
			close (pfd[1]);
			
			if (foundPipe == 1) {
				if ((pid = fork ()) < 0)
					perror (argv[0]);

				if (pid == 0) {
				
					if (dup2 (pfd[0], 0) == -1)
						perror ("dup2");
					exitCode = execvp (argv2[0], argv2);
					if (exitCode != 0)
						perror ("run");
					exit(0);
				}
			}
			
			wait (&wstatus);
			close (pfd[0]);
			
				

		return;
		
	}
	
	if (strcmp (firstWord, "easyrun") == 0) {
		
		if (strcmp (firstWord, line) == 0) {
			printf ("  USAGE:   easyrun <cmd> <arg1> <arg2> ... <argN>\n");
			printf ("  EXAMPLE: easyrun wc -l shell.c\n");
			return;
		}
		
		system (line + 8);
		return;
	}
	
	if ((strcmp (firstWord, "status") == 0)) {
		
		printf("Exit code of previous command: %d\n", exitCode);
		return;
	}
	
	printf ("%s: command not found\n", firstWord);
	return;
}

int main (int argc, char *argv[]) {

	setupDisplay ();

	char *line = NULL;
	
	while (!stop) {
	
		if (updatecwd () == -1) return -1;
		display ();
		line = getLine ();
		interpret (line);
	}

    return 0;
}
