#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_INPUT_SIZE 1024
#define MAX_TOKEN_SIZE 64
#define MAX_NUM_TOKENS 64
#define STDIN_PIPE 0x1
#define STDOUT_PIPE 0x2

char **tokenize(char* line); //토큰 분리 함수
int exec_command(char **command, int fd[2], int flags); //파이프 명령어 수행 함수

char **tokenize(char *line){ //토큰 분리 함수
	
	char **tokens = (char **)malloc(MAX_NUM_TOKENS * sizeof(char *));
	char *token = (char *)malloc(MAX_TOKEN_SIZE * sizeof(char));
	int i, tokenIndex = 0, tokenNo = 0;
	
	for(i =0; i < strlen(line); i++){
		char readChar = line[i];
	
		if (readChar == ' ' || readChar == '\n' || readChar == '\t'){ //공백문자, 개행문자라면 쪼개기
			token[tokenIndex] = '\0';
			if (tokenIndex != 0){
				tokens[tokenNo] = (char*)malloc(MAX_TOKEN_SIZE*sizeof(char));
				strcpy(tokens[tokenNo++], token);
				tokenIndex = 0;
			}
		} 
		else {
			token[tokenIndex++] = readChar;
		}
	}

	free(token);
	tokens[tokenNo] = NULL;
	return tokens;
}

int exec_command(char **command, int fd[2], int flags){ //파이프 명령어 구현
        
	pid_t cpid = fork(); //자식프로세스 생성
    int status; 

	if (cpid > 0){ //부모프로세스라면
		pid_t wait_pid;
		wait_pid = wait(&status);
	
		if(wait_pid == -1)
			printf("wait함수 에러\n");
		
		else{
			if(WEXITSTATUS(status)==10){ //자식프로세스가 exec 함수 호출 실패 시
			//	printf("failed\n");
				return -1;}

			else if(WIFSIGNALED(status)) { //자식프로세스가 강제종료 된 경우
                printf("wait : 자식 프로세스 비정상 종료 %d\n",WTERMSIG(status));
				return -1;
            }

			else{ //자식프로세스가 exec 함수 호출 성공 시
			//	printf("success\n");
				return 1;}
		}
	}
	
	else if (cpid < 0) //fork 오류 시
        perror("fork");

    if (flags & STDIN_PIPE){ //파이프에 표준입력 연결
    	dup2(fd[0], STDIN_FILENO);
	}

    if (flags & STDOUT_PIPE){ //파이프에 표준출력 연결
        dup2(fd[1], STDOUT_FILENO);
	}

    close(fd[0]); //파이프 표준입력 닫기
    close(fd[1]); //파이프 표준출력 닫기

	if(strcmp(command[0], "pps")==0){ //pps 명령어 실행
		if(execvp("./pps", command)==-1)
			exit(10);
	}
					
	else{
		if(execvp(command[0], command)<0) //내장 명령어 수행
			exit(10);
	}
}

int main(int argc, char* argv[]) {
	
	char line[MAX_INPUT_SIZE];
	char cp_line[MAX_INPUT_SIZE];
	char **tokens;
	FILE* fp;
	pid_t pid;
	int status;

	if(argc == 2) { //배치식 모드일 때
		fp = fopen(argv[1],"r");
		if(fp < 0) { //파일이 존재하지 않으면
			printf("File doesn't exists.");
			return -1;
		}
	}

	while(1) {/* BEGIN: TAKING INPUT */
		bzero(line, sizeof(line)); //배열 초기화
		
		if(argc == 2) { // batch mode
			if(fgets(line, sizeof(line), fp) == NULL) { //file reading finished
				break;
			}
			line[strlen(line) - 1] = '\0';
			printf("$ %s\n", line);
		}

		else { // interactive mode
				printf("$ ");
				scanf("%[^\n]", line);
				getchar();
		}

		if(strlen(line)==0)
			continue;

		line[strlen(line)] = '\n'; //terminate with new line

//		printf("Command entered: %s (remove this debug output later)\n", line); //입력한 명령문 출력

		if(strchr(line, '|') != NULL){ //파이프 사용하는 경우

			char *cp[MAX_TOKEN_SIZE] = {0,}; //입력한 명령어들
			int fd[MAX_TOKEN_SIZE][2]; //파이프로 만든 파일디스크립터
			int pipe_num = 0; //파이프 갯수
			int command_num = 0; //명령어 갯수
			char **command1_token = (char**)malloc(MAX_NUM_TOKENS * sizeof(char*)); //파이프 기준 왼쪽 명령어
			char **command2_token = (char**)malloc(MAX_NUM_TOKENS * sizeof(char*)); //파이프 기준 오른쪽 명령어

			char *ptr = line;

			for(int i=0; ptr[i]!=0; i++){ //파이프 갯수 세고 만들기
				if(ptr[i]=='|')
					pipe(fd[pipe_num++]);
			}

			char *ptr1 = strtok(line, "|");
			while(ptr1!=NULL){ //파이프 기준으로 명령어 자르기
				if(command_num==0)
					cp[command_num++] = ptr1;
				
				else
					cp[command_num++] = ptr1 + 1;
				
				ptr1 = strtok(NULL, "|");
			}

//			for(int i=0; i<command_num; i++)
//				printf("%s\n", cp[i]);

			if(pipe_num==1){ //파이프가 하나인 경우
				command1_token = tokenize(cp[0]); //왼쪽 명령어 토큰 쪼개기
				command2_token = tokenize(cp[1]); //오른쪽 명령어 토큰 쪼개기

				if(exec_command(command1_token, fd[0], STDOUT_PIPE)==-1){ //왼쪽 명령어 수행 및 출력 닫기
					printf("SSUShell : Incorrect command\n"); //실패 시 오류메시지 출력
					continue;}

				close(fd[0][1]);
				
				if(exec_command(command2_token, fd[0], STDIN_PIPE)==-1){ //오른쪽 명령어 수행 및 입력 닫기
					printf("SSUShell : Incorrect command\n"); //실패 시 오류메시지 출력
					continue;}

				close(fd[0][0]);
			}

			else if(pipe_num>1){ //파이프가 두개이상인 경우

				int error_num = 0;
				command1_token = tokenize(cp[0]); //첫번째 명령어 토큰 쪼개기
				command2_token = tokenize(cp[command_num-1]); //마지막 명령어 토큰 쪼개기

				if(exec_command(command1_token, fd[0], STDOUT_PIPE)==-1){ //첫번 째 명령어 수행 및 출력 닫기
					printf("SSUShell : Incorrect command\n"); //실패 시 오류메시지 출력
					continue;}

				close(fd[0][1]);
				
				for(int i=1; i<pipe_num; i++){ //가운데 명령어 수행 및 입출력 연결
					char **command_token = (char**)malloc(MAX_NUM_TOKENS * sizeof(char*));
					command_token = tokenize(cp[i]);

					int temp_fd[] = {fd[i-1][0], fd[i][1]};

					if(exec_command(command_token, temp_fd, STDIN_PIPE|STDOUT_PIPE)==-1){
						printf("SSUShell : Incorrect command\n"); //실패 시 오류메시지 출력
						error_num = -1;
						break;}
				
					close(fd[i-1][0]);
					close(fd[i][1]);

				}

				if(error_num == -1) //잘못된 명령어 입력시
					continue;

				if(exec_command(command2_token, fd[pipe_num-1], STDIN_PIPE)==-1){ //마지막 명령어 수행 및 입력 닫기
					printf("SSUShell : Incorrect command\n"); //실패 시 오류메시지 출력
					continue;}
				close(fd[pipe_num-1][0]);
			}

			/*메모리 할당 해제*/
			for(int i=0; command1_token[i]!=NULL; i++)
				free(command1_token[i]);
			for(int i=0; command2_token[i]!=NULL; i++)
				free(command2_token[i]);
	}

		else{ //파이프가 없는 경우
			
			/* END: TAKING INPUT 토큰 분리 */
			tokens = tokenize(line);

			pid = fork(); //자식프로세스 생성

			if(pid==0){ //자식프로세스인 경우
				if(strcmp(tokens[0], "pps")==0){ //pps 명령어 실행
					if(execvp("./pps", tokens)==-1){
						printf("SSUShell : Incorrect command\n");
						exit(10);}
				}

				else{
					if(execvp(tokens[0],tokens)==-1){ //명령어 실행
						printf("SSUShell : Incorrect command\n"); //실패 시 오류메시지 출력
						exit(10);}
				}
			}
	
			if(pid>0){ //부모프로세스인 경우
				
				int waitpid = wait(&status);	
			/*	for(i=0;tokens[i]!=NULL;i++){ //분리된 토큰 출력
					printf("found token %s (remove this debug output later)\n", tokens[i]);
				}*/

				if(waitpid==-1) //wait함수 에러시
					printf("wait함수 에러\n");

				else{
					if(WIFEXITED(status)==10){ //명령어 실행 실패
						//printf("자식 프로세스 비정상 종료\n");
					}

					else if(WIFSIGNALED(status)){ //자식프로세스 강제종료
						printf("자식 프로세스 비정상 종료 상태 : %d\n", WTERMSIG(status));}

					else if(WIFSTOPPED(status))
							printf("자식프로세스 중단\n");

				}
				
				/*메모리 할당 해제*/
				for(int i=0;tokens[i]!=NULL;i++)
					free(tokens[i]);
				free(tokens);
			}
		}
	}
	return 0;
}
