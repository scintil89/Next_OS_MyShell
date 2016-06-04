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
#define MAXTHREAD		10

#define DEFAULT_FILE_MODE	0664
#define DEFAULT_DIR_MODE	0775


/* 전역 변수 정의 */
char prompt[] = "\nmyshell> ";
const char delim[] = " \t\n";
int status = 0;
//int cpChecker = 0;
int pipe_checker = 0;
int redi_checker = 0;//redirection checker
int thread_checker = 0;

/* 전역 변수 선언 */
char* tsrc;
char* tdst;

/* 함수 선언 */
void myshell_error(char *err_msg);
void process_cmd(char *cmdline);
int parse_line(char *cmdline, char **argv);
int builtin_cmd(int argc, char **argv);

// 내장 명령어 처리 함수
int list_files(int argc, char **argv);      		//ls
int list_all_files(int argc, char **argv);  		//ll
int copy_file(int argc, char **argv);       		//cp
int remove_file(int argc, char **argv);     		//rm
int move_file(int argc, char **argv);       		//mv
int change_directory(int argc, char **argv);		//cd
int print_working_directory(int argc);     		//pwd
int make_directory(int argc, char **argv);  		//mkdir
int remove_directory(int argc, char **argv);		//rmdir
int help(int argc);					//help
/////////////////////////////////////////////////////
int mypipe(int checker, int argc, char** argv);
int mythread(int checker, int argc, char** argv);
void* copy_thread(void* arg);
/////////////////////////////////////////////////////

/*
* main - MyShell's main routine
*/
int main()
{
	char cmdline[MAXLINE];

	/* 명령어 처리 루프: 셸 명령어를 읽고 처리한다. */
	while (1) 
	{
		//call waitpid
		waitpid(-2, &status, WNOHANG);
		//	if (WIFEXITED(status) > 0)
		//       {
		//            fprintf(stderr, "%d\n", WEXITSTATUS(status));
		//        }

		//Init
		pipe_checker 	= 0;
		redi_checker 	= 0;
		thread_checker 	= 0;

		// 프롬프트 출력
		fprintf(stdout, "%s", prompt);
		fflush(stdout);

		// 명령 라인 읽기
		if (fgets(cmdline, MAXLINE, stdin) == NULL) 
		{
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
	char* argv[MAXARGS];
	
	//리다이렉션을 처리하기 위한 변수들.
	int fd;
	int saved_stdout;
	

	// 명령 라인을 해석하여 인자 (argument) 배열로 변환한다.
	argc = parse_line(cmdline, argv);

	//
	if(thread_checker > 0)
	{
		if(mythread(thread_checker, argc, argv) < 0)
		{
			fprintf(stderr, "thread program error\n");
			return;
		}
		
		return;
	}

	// 인자 배열에 파이프가 존재하는지 검사한다.
	//pipe_checker = pipe_check(argc, argv);
	if(pipe_checker > 0)
	{	
		if(mypipe(pipe_checker, argc, argv) < 0)
		{
			fprintf(stderr, "pipe error\n");
			return;
		}
	}

	// 인자 배열에 리다이렉션이 존재하는지 검사한다.
	//redi_checker = redirection_check(argc, argv);

	// 리다이렉션이 존재 할 경우 파일디스크립터 복제
	// 문자열 처리
	// ">"과 다음 인자를 NULL로 변경
	if(redi_checker > 0)
	{
		//파일디스크립터 복제
		fd = creat(argv[redi_checker+1], DEFAULT_FILE_MODE);
		if(fd < 0)
		{
			fprintf(stderr, "file descriptor creat error\n");
			return;
		}
	
		saved_stdout = dup(1); //stdout

		if( dup2(fd, 1) < 0 )
		{
			fprintf(stderr, "dup error\n");
			return;
		}
		
		//문자열 처리
		argc = redi_checker;
		argv[redi_checker] = NULL;
		argv[redi_checker+1] = NULL;
	}
	else if(redi_checker < 0)
	{	
		//redi_checker error	
		fprintf(stderr, "redirection error\n");
		return;
	}

	/* 내장 명령 처리 함수를 수행한다. */
	//if used pipe, builtin_cmd don't enter
	int result;	
	if(pipe_checker == 0)
	{	
		result = builtin_cmd(argc, argv);
	
		//cmd no error
		if( result == 0 )
		{
			if( redi_checker > 0 )
			{
				//리다이렉션을 수행한 경우 stdout복구
				close(fd);
				dup2(saved_stdout, 1);
			}
	
			//내장명령 처리를 완료하고 리턴
			return;
		}
		else if(result < 0)
		{
			//fprintf(stderr, "command try error\n");
			return;
		}

		//내장 명령어의 처리가 끝남
		/////////////////////////////////////////////////////////////
		//이 밑으로는 내장 명령어 목록에 존재하지 않는 명령에 대하여
		//외부 프로그램을 실행한다.

		/*
		* 자식 프로세스를 생성하여 프로그램을 실행한다.
		*/
		//fprintf(stderr, "try external program\n");
	
		//백그라운드 명령이 있는지 검사한다.
		int bgchecker = 0;
	
		if( !strcmp( argv[argc -1], "&") )
		{
			bgchecker = 1;
			argv[argc - 1] = NULL;
			argc--;
		} 	

		pid_t pid, pid1;
	
		// 프로세스 생성
		pid = fork();
	
		if(pid == 0)	//자식 프로세스
		{
			pid1 = getpid();

			/*
			execv 함수를 사용할때 사용된 코드
			//외부 명령어를 실행하기 위하여 argv를 셋팅해준다.	
			//char temp[128] = "/bin/";
			
			//strcat(temp, argv[0]);

			//문자열 결합 확인을 위한 출력
			//puts(temp);1
			
			//변경된 문자열을 argv의 첫 인자로 교체해준다.
			//argv[0] = temp;		
			*/

			//외장명령어 실행		

			if (execvp(argv[0], argv) < 0)
			{
				//실행 오류시 자식 에러 메세지를 출력하고
				//자식 프로세스를 종료시킨다.
				fprintf(stdout, "command does not exist\n");
				exit(0);
				return;
			}
			else
			{
				fprintf(stderr, "PID %d is terminated\n", pid1);
			}
		
			return;
		}
		else if(pid > 0)	//부모 프로세스
		{
			switch(bgchecker)
			{
			case 0:
				//자식 프로세스를 백그라운드로 실행하지 않을 경우
				//자식 프로세스의  종료를 기다린다.
				waitpid(pid, &status, 0);
				break;		
			default:
				break;
			}
		}
		else
		{
			fprintf(stderr, "fork error\n");
			return;
		}

		// 자식 프로세스에서 프로그램 실행
		// 파이프 실행이면 자식 프로세스를 하나 더 생성하여 파이프로 연결
		// foreground 실행이면 자식 프로세스가 종료할 때까지 기다린다.
	

		//If redirection is used, close fd

		if( result > 0)
		{
			if(redi_checker != 0)
			{
				//fprintf(stderr, "redirection error\n");
				close(fd);
				dup2(saved_stdout, 1);
				return;
			}
		}
	}
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
   	
	if(!cmdline)
		return factor;
 
    	//strtok로 문자열을 분석한 후 떨어져 나온 문자열의 주소를 반환
    	argv[factor] = strtok(cmdline, delim);
    
    	while( argv[factor] != NULL )
	{
        	++factor;
        	//NULL을 넣어서 전에 작동한 주소부터 다시 토큰분석
        	argv[factor] = strtok(NULL, delim);
	}

	int i;
	for(i = 0; i < factor; ++i)
	{
		//문자열 검사

		if( !strcmp(argv[i], "|") )
		{
			//인자 검사
			if(!argv[i+1])
			{
				fprintf(stderr, "pipe argument error\n");
				return -1;
			}
			pipe_checker = i;	
		}

		if( !strcmp(argv[i], ".") )
		{
			//인자 검사
			if(!argv[i+1])
			{
				fprintf(stderr, "thread argument error\n");
				return -1;
			}
			
			thread_checker = i;
		}

		if( !strcmp(argv[i], ">") )
		{
			//인자 검사
			if(!argv[i+1])
			{
				fprintf(stderr, "redirection argument error\n");
				return -1;
			}

			redi_checker = i;
		}
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
	//아무것도 안들어올 경우 예외처리
	if(argc == 0)
	{
		return -1;
	}
	//에러처리
	if(argc < 0)
	{
		fprintf(stderr ,"error. check your commane and retry\n");
		return -1;
	}
	// 내장 명령어 문자열과 argv[0]을 비교하여 각각의 처리 함수 호출
	if ( (!strcmp (argv[0], "quit")) || (!strcmp (argv[0], "exit")) ) 
	{
		exit(0);
	}
	if ( !strcmp(argv[0], "ls"))
	{
		return list_files(argc, argv);
	}
   	if ( !strcmp(argv[0], "ll"))
	{
       		return list_all_files(argc, argv);
    	}
    	if ( !strcmp(argv[0], "cp"))
	{
        	return copy_file(argc, argv);
    	}
    	if ( !strcmp(argv[0], "rm"))
	{
        	return remove_file(argc, argv);
    	}
    	if ( !strcmp(argv[0], "mv"))
	{
        	return move_file(argc, argv);
    	}
    	if ( !strcmp(argv[0], "cd"))
	{
        	return change_directory(argc, argv);
    	}
    	if ( !strcmp(argv[0], "pwd"))
	{
        	return print_working_directory(argc);
    	}
    	if ( !strcmp(argv[0], "mkdir"))
	{
        	return make_directory(argc, argv);
    	}
    	if ( !strcmp(argv[0], "rmdir"))
	{
        	return remove_directory(argc, argv);
    	}
	if( !strcmp(argv[0], "help"))
	{
		return help(argc);
	}
        
	// 내장 명령어가 아님.
   	//fprintf(stderr,"command not found\n");
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
	//문법 오류 처리
	if((argc != 1) && (argc != 2))
	{
		fprintf(stderr, "argument error\n");
		fprintf(stderr, "usage : ls <path>\n");
		return -1;
	}	

	DIR* p_DIR;
	struct dirent* p_DIRentry; //정보를 담을 구조체
	unsigned int line_checker = 0; //라인 정렬 체크
	const unsigned int line_end = 3; //한 라인에 표시될 수

	if(argc == 1)
	{
		p_DIR = opendir("."); //현재 디랙터리 설정
	}
	else if(argc == 2)
	{
		p_DIR = opendir(argv[1]);
	}

	//디렉터리 오픈 예외처리
	if(p_DIR == NULL)
	{
		fprintf(stderr, "directory open error\n");
		return 1;
	}

	//디렉토리 읽기
	while ( (p_DIRentry = readdir(p_DIR)) )
	{
		//출력
		fprintf(stdout, "%-20s	", p_DIRentry->d_name);
		++line_checker;

		//line up 
		if(line_checker == line_end)
		{
			fprintf(stdout, "\n");
			line_checker = 0;
		}
	}
	printf("\n");

	closedir(p_DIR);
	
	return 0;
}

int list_all_files(int argc, char **argv)
{
	//문법 오류 처리
	if((argc != 1) && (argc != 2))
	{
		fprintf(stderr, "argument error\n");
		fprintf(stderr, "usage : ll <path>\n");
		return -1;
	}

	struct stat sb;//stat buff
	int ret;
	DIR* p_DIR;
	struct dirent* p_DIRentry; //정보를 담을 구조체
	int filenum = 0;
	char buf[512];
	
	if(argc == 1)
	{
		p_DIR = opendir("."); //현재 디랙터리 설정
	}
	else if(argc == 2)
	{
		p_DIR = opendir(argv[1]);
	}

	//디렉터리 오픈 예외처리
	if(p_DIR == NULL)
	{
		fprintf(stderr, "directory open error\n");
		return 1;
	}

	//디렉터리 읽기
	while ( (p_DIRentry = readdir(p_DIR)) )
	{
		if(argc == 1)
		{
			ret = stat(p_DIRentry->d_name, &sb);
		}
		else if(argc ==2)
		{
			//받아온 파일 이름이 디렉토리가 포함된 파일 이름이 아님,
			//ex) dir/file 이 아님
			sprintf(buf, "%s/%s", argv[1], p_DIRentry->d_name);

			//stat를 sb에 받음
			ret = stat(buf, &sb);
		}

		if (ret < 0)
		{
			fprintf(stderr, "staterr\n");
		}
		else
		{
			++filenum; //파일 수 증가
			
			//파일명
			fprintf(stdout, "%2d ", filenum);
		
		
			//파일모드
			switch(sb.st_mode & S_IFMT)
			{
				case S_IFDIR:
					fprintf(stdout, "d");
					break;
				default:
					fprintf(stdout, "-");
					break;
			}
    			fprintf(stdout, (sb.st_mode & S_IRUSR) ? "r" : "-");
   			fprintf(stdout, (sb.st_mode & S_IWUSR) ? "w" : "-");
   			fprintf(stdout, (sb.st_mode & S_IXUSR) ? "x" : "-");
 			fprintf(stdout, (sb.st_mode & S_IRGRP) ? "r" : "-");
    			fprintf(stdout, (sb.st_mode & S_IWGRP) ? "w" : "-");
    			fprintf(stdout, (sb.st_mode & S_IXGRP) ? "x" : "-");
    			fprintf(stdout, (sb.st_mode & S_IROTH) ? "r" : "-");
    			fprintf(stdout, (sb.st_mode & S_IWOTH) ? "w" : "-");
    			fprintf(stdout, (sb.st_mode & S_IXOTH) ? "x" : "-");
			//printf("%lo", (unsigned long)sb.st_mode);
		
			//링크수			
			fprintf(stdout, " %2ld", (long)sb.st_nlink);	
			//유저id			
			fprintf(stdout, " %ld", (long)sb.st_uid);
			//그룹id
			fprintf(stdout, " %ld", (long)sb.st_gid);
			//용량
			fprintf(stdout, " %-5lld",(long long) sb.st_size);
			//수정시간
			//modify time string
			char* mtime = ctime(&sb.st_mtime);
			int len_mtime = strlen(mtime);
			mtime[len_mtime - 1] = '\0';
		
			fprintf(stdout, " %s", mtime);
			//file name
			fprintf(stdout, " %s\n",  p_DIRentry->d_name);
		}		
	}

	closedir(p_DIR);
	
	return 0;
}

int copy_file(int argc, char **argv)
{
	//문법 오류 처리
	if(argc != 3)
	{
		fprintf(stderr, "argument error\n");
		fprintf(stderr, "usage : cp <source> <destination>\n");
		return -1;
	}
	
	//디렉터리일 경우 예외처리
	struct stat sb; //stat buff
	if (stat(argv[1], &sb) == 0)
	{
		if( (sb.st_mode & S_IFMT) == S_IFDIR )
		{
			fprintf(stderr, "%s is directory\n", argv[1]);
			fprintf(stderr, "cp doesn't support directory\n");
			return -1;
		}
	}
	else
	{
		//파일 속성 정보를 받아오지 못할경우
		fprintf(stderr, "failed to check mode\n");
		return -1;
	}

	char buf;
	int src = 0; //source
	int dst = 0; //destination
	long read_count = 0; //read count

	//파일 오픈
	src = open(argv[1], O_RDONLY);
	//파일 오픈 예외 처리
	if(src < 0)
	{
		fprintf(stderr, "file open error. check your file.\n");
		return -1;
	}

	dst = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, DEFAULT_FILE_MODE);
	//파일 오픈 예외 처리
	if(dst < 0)
	{
		fprintf(stderr, "file open error. check your file. \n");
		return -1;
	}

	//파일 쓰기
	while ( (read_count = read(src, &buf, 1)) )
	{
		write(dst, &buf, read_count);
	}
	
	//파일 쓰기 오류메세지
	if(read_count < 0)
	{
		fprintf(stderr, "file write error\n");
	}

	//close
	if(close(src) < 0)
	{
		fprintf(stderr, "file close error");
	}
	if(close(dst) < 0)
	{
		fprintf(stderr, "file close error");
	}

	return 0;
}


int remove_file(int argc, char **argv)
{
	//문법 오류 처리
	if(argc != 2)
	{
		fprintf(stderr, "argument error\n");
		fprintf(stderr, "usage : rm <file_name>\n");
		return -1;
	}

	//디렉터리일 경우 예외처리
	struct stat sb; //stat buff
	if (stat(argv[1], &sb) == 0)
	{
		if( (sb.st_mode & S_IFMT) == S_IFDIR )
		{
			fprintf(stderr, "%s is directory\n", argv[1]);
			fprintf(stderr, "rm doesn't support directory\n");
			return -1;
		}
	}
	else
	{
		//파일 속성 정보를 받아오지 못할경우
		fprintf(stderr, "failed to check mode\n");
		return -1;
	}
	
	//파일 삭제
	if(remove(argv[1]) < 0)
	{
		fprintf(stderr, "error : file remove failed\n");
		return -1;
	}	

	return 0;
}


int move_file(int argc, char **argv)
{
	//문법 오류 처리
	if(argc != 3)
	{
		fprintf(stderr, "argument error\n");
		fprintf(stderr, "usage : mv <source> <destination>\n");
		return -1;
	}
	
	//파일 이동
	if(rename(argv[1], argv[2]) < 0)
	{
		fprintf(stderr, "%s can't move to %s\n", argv[1], argv[2]);
		return -1;
	}
	
	return 0;

}

int change_directory(int argc, char **argv)
{
	//문법 오류 처리
	if(argc != 2)
	{
		fprintf(stderr, "argument error\n");
		fprintf(stderr, "usage : cd <path>\n");
		return -1;
	}
	
	//디렉터리 변경
	if(chdir(argv[1]) < 0)
	{
		fprintf(stderr, "failed to change directory\n");
		return -1;
	}

	return 0;
}


int print_working_directory(int argc)
{
	char buf[256];
	
	//문법 오류 처리
	if(argc != 1)
	{
		fprintf(stderr, "argument error\n");
		fprintf(stderr, "usage : pwd\n");
		return -1;	
	}

	//get pwd
	if( getcwd(buf, sizeof(buf)) )
	{
		fprintf(stdout, "%s\n", buf);
	}
	else //error check
	{
		fprintf(stderr, "failed to get working directory.\n");
		return -1;
	}

	return 0;
}


int make_directory(int argc, char **argv)
{
	//문법 오류 처리
	if(argc != 2)
	{
		fprintf(stderr, "argument error\n");
		fprintf(stderr, "usage : mkdir <directory_name>\n");
		return -1;
	}

	//디렉터리 생성
	if(mkdir(argv[1], DEFAULT_DIR_MODE) < 0)
	{
		fprintf(stderr, "faild to make directory\n");
		return -1;
	}   

	return 0;
}


int remove_directory(int argc, char **argv)
{
    	//문법 오류 처리
	if(argc != 2)
	{
		fprintf(stderr, "argument error\n");
		fprintf(stderr, "usage : rmdir <directory_name>\n");
		return -1;
	}

	//디렉터리가 아닐 경우 예외처리
	struct stat sb; //stat buff
	if (stat(argv[1], &sb) == 0)
	{
		if( (sb.st_mode & S_IFMT) != S_IFDIR )
		{
			fprintf(stderr ,"mkdir demand directory!\n");
			return -1;
		}
	}
	else
	{
		//파일 속성 정보를 받아오지 못할경우
		fprintf(stderr, "failed to check mode\n");
		return -1;
	}
	
	//디렉터리 제거
	if(rmdir(argv[1]) < 0)
	{
		fprintf(stderr, "failed to remove directory\n");
		fprintf(stderr, "check your directory\n");
		return -1;
	}
    	
	return 0;
}

int help(int argc)
{
	//문법 오류 처리
	if(argc != 1)
	{
		fprintf(stderr, "argument error\n");
		return -1;
	}

	fprintf(stdout, "U Can Use under command\n");
	fprintf(stdout, "ls\n");
	fprintf(stdout, "ll\n");
	fprintf(stdout, "cp\n");
	fprintf(stdout, "rm\n");
	fprintf(stdout, "mv\n");
	fprintf(stdout, "mkdir\n");
	fprintf(stdout, "rmdir\n");
	fprintf(stdout, "pwd\n");
	fprintf(stdout, "help\n");
	fprintf(stdout, "if your cmd does not exist in over list, shell try external program\n");
	fprintf(stdout, "& : back ground play\n");
	fprintf(stdout, "> : redirection \n\tex) src > dst\n");
	fprintf(stdout, "| : pipe \n\tex) src | dst\n"); 
	fprintf(stdout, ". : directory copy \n\tex) src . dst\n"); 
	return 0;
}

int mypipe(int checker, int argc, char** argv)
{
	char* temp[MAXARGS];	

	//파이프를 처리하기 위한 변수들
	int pipefd[2];

	if(checker > 0)
	{	
		pid_t pipe_pid;

		//명령어 분리
		int temp_i = 0;
		int pipe_i;
		for(pipe_i = checker+1 ; pipe_i < argc; ++pipe_i)
		{
			temp[temp_i] = argv[pipe_i];
			argv[pipe_i] = NULL;
			++temp_i;
		}
		argv[checker] = NULL;
		temp[temp_i] = NULL;
		
		//파이프 생성
		if( pipe(pipefd) == -1)
		{
			fprintf(stderr, "pipe creat error\n");
			return -1;
		}
		
		pipe_pid = fork();
		if(pipe_pid < 0)
		{
			fprintf(stderr, "fork failed\n");
		}	
	
		if(pipe_pid == 0)
		{	
			close(pipefd[0]);
			
			//파이프 출력을 stdout으로 복제
			if( dup2(pipefd[1], STDOUT_FILENO) < 0)
			{
				fprintf(stderr, "pipefd dup error\n");
				exit(1);
			}
			
			//명령 실행
			if(execvp(argv[0], argv) < 0)
			{
				fprintf(stdout, "pipe exec err\n");
				exit(1);
			}
		}	
		
		pipe_pid = fork();
		if(pipe_pid < 0)
		{
			fprintf(stderr, "fork failed\n");
		}

		if(pipe_pid == 0)
		{
			close(pipefd[1]);

			//파이프 입력을 stdin으로 복제
			if (dup2(pipefd[0], STDIN_FILENO) < 0)
			{
				fprintf(stderr, "pipefd2 dup error\n");
				exit(1);
			}

			//명령 실행
			if(execvp(temp[0], temp) < 0)
			{
				fprintf(stdout, "pipe exec2 err\n");
				exit(1);
			}
		}
		
		//부모 프로세스는 파이프의 read / write 모두 닫는다
		close(pipefd[0]);
		close(pipefd[1]);
	
		//부모 프로세스는 두개의 자식 프로세스가 죵료할때까지 기다린다.
		while(wait(NULL) >= 0);
		
		return 0;
	}

	return -1;
}

int mythread(int checker, int argc, char** argv)
{
	if(argc != 3)
	{
		fprintf(stderr, "thread copy need 3 arguments\nex) src_dir . dst_dir\n");
		return -1;
	}
	
	//set directory
	tsrc = argv[checker - 1];
	tdst = argv[checker + 1];

	//set thread 
	pthread_t 	threads[MAXTHREAD];
	int 		tret[MAXTHREAD];
	int 		tn = 0;
	
////////////////////////////////////////////////////////////////////////////////////////////
	struct stat sb; //stat buff

	//check src is dir
	if (stat(tsrc, &sb) == 0)
	{
		if( (sb.st_mode & S_IFMT) != S_IFDIR )
		{
			fprintf(stderr ,"first argument must be directory!\n");
			return -1;
		}
	}
	
	//check dst is dir
	if (stat(tdst, &sb) == 0)
	{
		if( (sb.st_mode & S_IFMT) != S_IFDIR )
		{
			fprintf(stderr ,"third argument must be directory!\n");
			return -1;
		}
	}
///////////////////////////////////////////////////////////////////////////////////////////

	DIR* p_DIR;
	struct dirent* p_DIRentry; //정보를 담을 구조체

	p_DIR = opendir(tsrc);
	//디렉터리 오픈 예외처리
	if(p_DIR == NULL)
	{
		fprintf(stderr, "directory open error\n");
		return -1;
	}

	//디렉터리 읽기
	while ( (p_DIRentry = readdir(p_DIR)) )
	{
		char file_buf[512];
		sprintf(file_buf, "%s/%s", tsrc, p_DIRentry->d_name);
		
		int ret = stat(file_buf, &sb);
		if (ret < 0)
		{
			fprintf(stderr, "can't take file status\n");
			//pthread_exit( (void*)-1 );
			return -1;
		}
		else
		{
			switch(sb.st_mode & S_IFMT)
			{
			case S_IFDIR:
			break;
			
			default:
			{
				int result = pthread_create(&threads[tn], NULL, 
					&copy_thread, (void*)p_DIRentry->d_name);

				if( result < 0)
				{
					fprintf(stderr, "create thread error\n");
					return -1;
				}
				else
				{
					//pthread_join(threads[tn], (void**)&tret[tn]);
					++tn;		
				}
			} 
			break;
			}
		}
	
		if( tn == MAXTHREAD )
		{
			//fprintf(stderr, "MAXTHREAD\n");
			int i;
			for(i = 0; i < tn; ++i)
			{
				pthread_join(threads[i], (void**)&tret[i]);
			}
			tn = 0;					
		}		
	}

	int j;
	for(j = 0; j < tn; ++j)
	{
		pthread_join(threads[j], (void**)&tret[j]);
	}

	closedir(p_DIR);

	return 0;
}

void* copy_thread(void* arg)
{
	char* tfilename = (char*)arg;
	
	//받아온 파일 이름이 디렉토리가 포함된 파일 이름이 아님,
	//ex) dir/file 이 아님
	char src_buf[512];
	sprintf(src_buf, "%s/%s", tsrc, tfilename);

	//File Copy					
	//fprintf(stderr,"%s\n", buf);
	char buf;
	long read_count = 0; //read count

	//파일 오픈
	int src = open(src_buf, O_RDONLY);
	//파일 오픈 예외 처리
	if(src < 0)
	{
		fprintf(stderr, "src file open error. check your file.\n");
		pthread_exit( (void*)-1 );
	}

	char dst_buf[512];
	sprintf(dst_buf, "%s/%s", tdst, tfilename);
	
	int dst = open(dst_buf, O_WRONLY | O_CREAT | O_TRUNC, DEFAULT_FILE_MODE);
	//파일 오픈 예외 처리
	if(dst < 0)
	{
		fprintf(stderr, "dst file open error. check your file. \n");
		pthread_exit( (void*)-1 );
	}

	//파일 쓰기
	while ( (read_count = read(src, &buf, 1)) )
	{
		write(dst, &buf, read_count);
	}

	//파일 쓰기 오류메세지
	if(read_count < 0)
	{
		fprintf(stderr, "file write error\n");
	}
	//close
	else if(close(src) < 0)
	{
		fprintf(stderr, "file close error");
	}
	else if(close(dst) < 0)
	{
		fprintf(stderr, "file close error");
	}
	else
	{
		fprintf(stderr, "copy %s into %s\n", src_buf, dst_buf);
	}

	pthread_exit( (char*)0);
}	




