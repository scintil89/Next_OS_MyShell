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


/* 상수 정의 */
#define MAXLINE		1024
#define MAXARGS		128
#define MAXPATH		1024
#define MAXTHREAD	10

#define DEFAULT_FILE_MODE	0664
#define DEFAULT_DIR_MODE	0775


/* 전역 변수 정의 */
char prompt[] = "myshell> ";
const char delim[] = " \t\n";


/* 전역 변수 선언 */


/* 함수 선언 */
void myshell_error(char *err_msg);
void process_cmd(char *cmdline);
int parse_line(char *cmdline, char **argv);
int builtin_cmd(int argc, char **argv);

// 내장 명령어 처리 함수
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

	/* 명령어 처리 루프: 셸 명령어를 읽고 처리한다. */
	while (1) {
		// 프롬프트 출력
		printf("%s", prompt);
		fflush(stdout);

		// 명령 라인 읽기
		if (fgets(cmdline, MAXLINE, stdin) == NULL) {
			return 1;
		}

		// 명령 라인 처리
		process_cmd(cmdline);

		fflush(stdout);
	}

	/* 프로그램이 도달하지 못한다. */
	return 0;
}


/*
* process_cmd
*
* 명령 라인을 인자 (argument) 배열로 변환한다.
* 내장 명령 처리 함수를 수행한다.
* 내장 명령이 아니면 자식 프로세스를 생성하여 지정된 프로그램을 실행한다.
* 파이프(|)를 사용하는 경우에는 두 개의 자식 프로세스를 생성한다.
*/
void process_cmd(char *cmdline)
{
	int argc;
	char *argv[MAXARGS];

	// 명령 라인을 해석하여 인자 (argument) 배열로 변환한다.
	argc = parse_line(cmdline, argv);


	/* 내장 명령 처리 함수를 수행한다. */
	if (builtin_cmd(argc, argv) == 0) {

		// 내장 명령 처리를 완료하고 리턴한다.
		return;
	}


	/*
	* 자식 프로세스를 생성하여 프로그램을 실행한다.
	*/

	// 프로세스 생성
	// 자식 프로세스에서 프로그램 실행
	// 파이프 실행이면 자식 프로세스를 하나 더 생성하여 파이프로 연결
	// foreground 실행이면 자식 프로세스가 종료할 때까지 기다린다.

	return;
}


/*
* parse_line
*
* 명령 라인을 인자(argument) 배열로 변환한다.
* 인자의 개수(argc)를 리턴한다.
* 파이프와 백그라운드 실행, 리다이렉션을 해석하고 flag와 관련 변수를
*   설정한다.
*/
int parse_line(char *cmdline, char **argv)
{
	// delimiter 문자를 이용하여 cmdline 문자열 분석
	int factor = 0; //인자 수 카운트
   
    //strtok로 문자열을 분석한 후 떨어져 나온 문자열의 주소를 반환
    argv[factor] = strtok(cmdline, delim);
    
    while( argv[factor] != NULL )
	{
        ++factor;
        //NULL을 넣어서 전에 작동한 주소부터 다시 토큰분석
        argv[factor] = strtok(NULL, delim);
	}

	return factor;
}


/*
* builtin_cmd
*
* 내장 명령을 수행한다.
* 내장 명령이 아니면 1을 리턴한다.
*/
int builtin_cmd(int argc, char **argv)
{ 
	// 내장 명령어 문자열과 argv[0]을 비교하여 각각의 처리 함수 호출
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
        
	// 내장 명령어가 아님.
    printf("command not found\n");
	return 1;
}


/*
*
* 내장 명령 처리 함수들
* argc, argv를 인자로 받는다.
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
