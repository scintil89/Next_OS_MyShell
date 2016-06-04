#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>


/* ��� ���� */
#define MAXLINE		1024
#define MAXARGS		128
#define MAXPATH		1024
#define MAXTHREAD	10

#define DEFAULT_FILE_MODE	0664
#define DEFAULT_DIR_MODE	0775


/* ���� ���� ���� */
char prompt[] = "myshell> ";
const char delim[] = " \t\n";


/* ���� ���� ���� */


/* �Լ� ���� */
void myshell_error(char *err_msg);
void process_cmd(char *cmdline);
int parse_line(char *cmdline, char **argv);
int builtin_cmd(int argc, char **argv);

// ���� ��ɾ� ó�� �Լ�
int list_files(int argc, char **argv);      //ls
int list_all_files(int argc, char **argv);  //ll
int copy_file(int argc, char **argv);       //cp
int remove_file(int argc, char **argv);     //rm
int move_file(int argc, char **argv);       //move
int change_directory(int argc, char **argv);//cd
int print_working_directory(void);          //pwd
int make_directory(int argc, char **argv);  //mkdir
int remove_directory(int argc, char **argv);//rmdir
int copy_directory(int argc, char **argv);  //cpdir



/*
* main - MyShell's main routine
*/
int main()
{
	char cmdline[MAXLINE];

	/* ��ɾ� ó�� ����: �� ��ɾ �а� ó���Ѵ�. */
	while (1) {
		// ������Ʈ ���
		printf("%s", prompt);
		fflush(stdout);

		// ��� ���� �б�
		if (fgets(cmdline, MAXLINE, stdin) == NULL) {
			return 1;
		}

		// ��� ���� ó��
		process_cmd(cmdline);

		fflush(stdout);
	}

	/* ���α׷��� �������� ���Ѵ�. */
	return 0;
}


/*
* process_cmd
*
* ��� ������ ���� (argument) �迭�� ��ȯ�Ѵ�.
* ���� ��� ó�� �Լ��� �����Ѵ�.
* ���� ����� �ƴϸ� �ڽ� ���μ����� �����Ͽ� ������ ���α׷��� �����Ѵ�.
* ������(|)�� ����ϴ� ��쿡�� �� ���� �ڽ� ���μ����� �����Ѵ�.
*/
void process_cmd(char *cmdline)
{
	int argc;
	char *argv[MAXARGS];

	// ��� ������ �ؼ��Ͽ� ���� (argument) �迭�� ��ȯ�Ѵ�.
	argc = parse_line(cmdline, argv);


	/* ���� ��� ó�� �Լ��� �����Ѵ�. */
	if (builtin_cmd(argc, argv) == 0) {

		// ���� ��� ó���� �Ϸ��ϰ� �����Ѵ�.
		return;
	}


	/*
	* �ڽ� ���μ����� �����Ͽ� ���α׷��� �����Ѵ�.
	*/

	// ���μ��� ����
	// �ڽ� ���μ������� ���α׷� ����
	// ������ �����̸� �ڽ� ���μ����� �ϳ� �� �����Ͽ� �������� ����
	// foreground �����̸� �ڽ� ���μ����� ������ ������ ��ٸ���.

	return;
}


/*
* parse_line
*
* ��� ������ ����(argument) �迭�� ��ȯ�Ѵ�.
* ������ ����(argc)�� �����Ѵ�.
* �������� ��׶��� ����, �����̷����� �ؼ��ϰ� flag�� ���� ������
*   �����Ѵ�.
*/
int parse_line(char *cmdline, char **argv)
{
	// delimiter ���ڸ� �̿��Ͽ� cmdline ���ڿ� �м�
	int factor = 0; //���� �� ī��Ʈ
   
    //strtok�� ���ڿ��� �м��� �� ������ ���� ���ڿ��� �ּҸ� ��ȯ
    argv[factor] = strtok(cmdline, delim);
    
    while( argv[factor] != NULL )
	{
        ++factor;
        //NULL�� �־ ���� �۵��� �ּҺ��� �ٽ� ��ū�м�
        argv[factor] = strtok(NULL, delim);
	}

	return factor;
}


/*
* builtin_cmd
*
* ���� ����� �����Ѵ�.
* ���� ����� �ƴϸ� 1�� �����Ѵ�.
*/
int builtin_cmd(int argc, char **argv)
{ 
	// ���� ��ɾ� ���ڿ��� argv[0]�� ���Ͽ� ������ ó�� �Լ� ȣ��
	if ( (!strcmp (argv[0], "quit")) || (!strcmp (argv[0], "exit")) ) {
		exit(0);
	}
    if ( !strcmp(argv[0], "ls")){
       return list_files(argc, argv);
    }
    if ( !strcmp(argv[0], "ll")){
       return list_all_files(argc, argv);
    }
    if ( !strcmp(argv[0], "cp")){
        return copy_file(argc, argv);
    }
    if ( !strcmp(argv[0], "rm")){
        return remove_file(argc, argv);
    }
    if ( !strcmp(argv[0], "move")){
        return move_file(argc, argv);
    }
    if ( !strcmp(argv[0], "cd")){
        return change_directory(argc, argv);
    }
    if ( !strcmp(argv[0], "pwd")){
        return print_working_directory();
    }
    if ( !strcmp(argv[0], "mkdir")){
        return make_directory(argc, argv);
    }
    if ( !strcmp(argv[0], "rmdir")){
        return remove_directory(argc, argv);
    }
    if ( !strcmp(argv[0], "cpdir")){
        return copy_directory(argc, argv);
    }
        
	// ���� ��ɾ �ƴ�.
    printf("command not found\n");
	return 1;
}


/*
*
* ���� ��� ó�� �Լ���
* argc, argv�� ���ڷ� �޴´�.
*
*/
int list_files(int argc, char **argv)
{
    int i;
    printf("argc = %d\n", argc);
    
    for(i = 0; i < argc; ++i)
        printf("argv[%d] = %s\n", i, argv[i]);
    
	return 0;
}

int list_all_files(int argc, char **argv)
{
    int i;
    printf("argc = %d\n", argc);
    
    for(i = 0; i < argc; ++i)
        printf("argv[%d] = %s\n", i, argv[i]);
    
	return 0;
}

int copy_file(int argc, char **argv)
{
    int i;
    printf("argc = %d\n", argc);
    
    for(i = 0; i < argc; ++i)
        printf("argv[%d] = %s\n", i, argv[i]);
    
	return 0;
}


int remove_file(int argc, char **argv)
{
    int i;
    printf("argc = %d\n", argc);
    
    for(i = 0; i < argc; ++i)
        printf("argv[%d] = %s\n", i, argv[i]);
    
	return 0;
}


int move_file(int argc, char **argv)
{
    int i;
    printf("argc = %d\n", argc);
    
    for(i = 0; i < argc; ++i)
        printf("argv[%d] = %s\n", i, argv[i]);
    
	return 0;
}


int change_directory(int argc, char **argv)
{
    int i;
    printf("argc = %d\n", argc);
    
    for(i = 0; i < argc; ++i)
        printf("argv[%d] = %s\n", i, argv[i]);
    
	return 0;
}


int print_working_directory(void)
{
    printf("print working directory\n");
	return 0;
}


int make_directory(int argc, char **argv)
{
    int i;
    printf("argc = %d\n", argc);
    
    for(i = 0; i < argc; ++i)
        printf("argv[%d] = %s\n", i, argv[i]);
    
	return 0;
}


int remove_directory(int argc, char **argv)
{
    int i;
    printf("argc = %d\n", argc);
    
    for(i = 0; i < argc; ++i)
        printf("argv[%d] = %s\n", i, argv[i]);
    
	return 0;
}


int copy_directory(int argc, char **argv)
{
    int i;
    printf("argc = %d\n", argc);
    
    for(i = 0; i < argc; ++i)
        printf("argv[%d] = %s\n", i, argv[i]);
    
	return 0;
}
