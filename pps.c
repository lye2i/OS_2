#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <ctype.h>
#include <time.h>
#include <linux/kdev_t.h>
#include <fcntl.h>

#define MAX_INPUT_SIZE 1024
#define MAX_TOKEN_SIZE 64
#define MAX_NUM_TOKENS 64

char **tokenize(char* line); //토큰 분리 함수
int is_digit(char *str); //pid가 숫자인지 확인
void get_pid(void); // /proc 안에 있는 파일들 확인하여 pid 저장
void get_stat(char *proc); //stat 정보 저장
char *get_cputime(ulong utime, ulong stime, int argc); //각 프로세스의 cpu 사용시간 구하기
int get_pcpu(ulong utime, ulong stime, ulong starttime, int seconds); //프로세스의 cpu사용량 구하기
int get_uptime(); //시스템 시간 구하기
char *time_str(int time, int argc, char m); //시간 문자열 형태로 저장
char *get_tty(int tty_nr); //tty 구하기
char *get_cmd(int pid); //cmd 구하기
int get_totalmem(void); //total_memory 구하기
char *get_starttime(float start, int uptime); //starttime 구하기
char proc_pid[MAX_INPUT_SIZE][30]; //pid 저장
char username[MAX_INPUT_SIZE][MAX_TOKEN_SIZE]; //프로세스의 user_name 저장
char stat_info[MAX_INPUT_SIZE][MAX_INPUT_SIZE]; //각 프로세스의 stat 저장
char proc_stat[MAX_INPUT_SIZE][MAX_INPUT_SIZE]; //각 프로세스의 stat 저장
int pid_num = 0; //pid 갯수
char cur_pid[MAX_INPUT_SIZE]; //현재 만들어진 프로세스의 pid
//char cmd[MAX_INPUT_SIZE][2000]; //cmd 저장

int main(int argc, char *argv[]){

	char *cpu_time = (char*)malloc(MAX_TOKEN_SIZE * sizeof(char));
	char *cmd = (char*)malloc(MAX_TOKEN_SIZE * sizeof(char));
	char *starttime = (char*)malloc(MAX_TOKEN_SIZE * sizeof(char));
	char **tokens = (char**)malloc(MAX_NUM_TOKENS * sizeof(char*));
	char *tty = (char*)malloc(MAX_TOKEN_SIZE * sizeof(char));
	int seconds = get_uptime();
	int total_cpu = 0;
	struct passwd *lpwd;
	lpwd = getpwuid(getuid());
	char uid[MAX_TOKEN_SIZE]={0,};
	strcpy(uid, lpwd->pw_name);
	int a = 0;
	int u = 0;
	int x = 0;

	get_pid(); //각각의 프로세스의 정보 구하기

	if(argc==2 && strchr(argv[1], 'a')!=NULL) //a 옵션이 있을때
		a = 1;

	if(argc==2&&strchr(argv[1], 'u')!=NULL) //u 옵션이 있을때
		u = 1;

	if(argc==2&&strchr(argv[1], 'x')!=NULL) //x 옵션이 있을때
		x = 1;
	
	if(argc==1){ //옵션 없이 ps만 입력
		char **cur_tokens = (char**)malloc(MAX_NUM_TOKENS * sizeof(char*));
		cur_tokens = tokenize(cur_pid);

		printf("%7s %2s %13s %2s\n", "PID", "TTY", "TIME", "CMD");
		
		for(int i=0; i<pid_num; i++){
			
			tokens = tokenize(proc_stat[i]); //stat 정보 토큰 단위로 저장
			if(strcmp(cur_tokens[6], tokens[6])==0){ //현재의 터미널과 같은 tty인 경우
				tty = get_tty(atoi(tokens[6])); //tty 구하기
				cpu_time = get_cputime(atoi(tokens[13]), atoi(tokens[14]), argc); //cpu 사용시간 구하기

				cmd = get_cmd(atoi(tokens[0])); //cmd 구하기
				printf("%7s %2s %11s %2s\n",tokens[0], tty, cpu_time, cmd);}
		}
		free(cur_tokens);
	}

	else if(a==1&&u==0&&x==0){ //옵션 'a'만 입력
		
		printf("%7s %6s %8s %7s %2s\n", "PID", "TTY", "STAT", "TIME", "COMMAND");
		for(int i=0; i<pid_num; i++){
			tokens = tokenize(proc_stat[i]); //stat 정보 토큰 단위로 저장
			if(atoi(tokens[6])!=0){ //터미널과 연관된 프로세스라면
				tty = get_tty(atoi(tokens[6])); //tty구하기
				cpu_time = get_cputime(atoi(tokens[13]), atoi(tokens[14]), argc); //cpu사용시간 구하기
				cmd = get_cmd(atoi(tokens[0])); //cmd 구하기
				printf("%7s %6s %8s %7s %2s\n", tokens[0], tty, tokens[2], cpu_time, cmd);
			}
		}
	}

	else if((a==0&&u==1&&x==0) || (a==1&&u==1&&x==0)){ //옵션 'u'만 입력과 옵션 'au'입력
		printf("%s %10s %2s %2s %8s %7s %6s %6s %8s %5s %2s\n", "USER", "PID", "%CPU", "%MEM", "VSZ", "RSS", "TTY", "STAT", "START", "TIME", "COMMAND");
		int totalmem = get_totalmem(); //total memory 구하기
		for(int i=0; i<pid_num; i++){
			tokens = tokenize(proc_stat[i]); //stat 정보 토큰 단위로 저장
			if(strcmp(uid, username[i])==0&&atoi(tokens[6])!=0){ //실행터미널의 소유자이고 터미널과 연관된 프로세스라면
				tty = get_tty(atoi(tokens[6])); //tty구하기
				cpu_time = get_cputime(atoi(tokens[13]), atoi(tokens[14]), argc); //cpu 사용시간 구하기
				int first = (atoi(tokens[23])*4*100)/totalmem; //%mem의 정수 구하기
				int second = (((atoi(tokens[23])*4*100)-totalmem*first)*10)/totalmem; //%mem의 소수점 자리수 구하기
				int pcpu = get_pcpu(atoi(tokens[13]), atoi(tokens[14]), atoi(tokens[21]), seconds); //%cpu 구하기
				cmd = get_cmd(atoi(tokens[0])); //cmd 구하기
				starttime = get_starttime(atof(tokens[21]), seconds);
				printf("%s %11s %2d.%d %2d.%d %8.0f %7d %6s %6s %8s %5s %2s\n", username[i], tokens[0], pcpu/10, pcpu%10, first,second, atof(tokens[22])/1024, atoi(tokens[23])*4, tty, tokens[2],starttime, cpu_time, cmd);	
			}
		}
	}
	

	else if(a==0&&u==0&&x==1){ //옵션 'x'만 입력
		printf("%7s %6s %8s %7s %2s\n", "PID", "TTY", "STAT", "TIME", "COMMAND");
		for(int i=0; i<pid_num; i++){
			tokens = tokenize(proc_stat[i]); //stat 정보 토큰 단위로 저장
			if(strcmp(uid, username[i])==0){ //터미널 실행 유저의 이름과 프로세스 소유자 이름이 같다면
				tty = get_tty(atoi(tokens[6])); //tty 구하기
				cpu_time = get_cputime(atoi(tokens[13]), atoi(tokens[14]), argc); //cpu 사용시간 구하기
				cmd = get_cmd(atoi(tokens[0])); //cmd 구하기
				printf("%7s %6s %8s %7s %2s\n", tokens[0], tty, tokens[2], cpu_time, cmd);
			}
		}
	}

	else if(a==1&&u==0&&x==1){ //옵션 'ax' 입력
		printf("%7s %6s %8s %7s %2s\n", "PID", "TTY", "STAT", "TIME", "COMMAND");
		for(int i=0; i<pid_num; i++){
			tokens = tokenize(proc_stat[i]); //stat 정보 토큰 단위로 저장
			tty = get_tty(atoi(tokens[6])); //tty 구하기
			cpu_time = get_cputime(atoi(tokens[13]), atoi(tokens[14]), argc); //cpu 사용시간 구하기
			if(strcmp(username[i], "root")!=0){ //프로세스 user이름이 root라면
				cmd = get_cmd(atoi(tokens[0])); //cmd 구하기
				printf("%7s %6s %8s %7s %2s\n", tokens[0], tty, tokens[2], cpu_time, cmd);}
			else
				printf("%7s %6s %8s %7s %2s\n", tokens[0], tty, tokens[2], cpu_time, tokens[1]);}
	}

	else if(a==0&&u==1&&x==1){ //옵션 'ux' 입력
		printf("%s %10s %2s %2s %8s %7s %6s %6s %8s %5s %2s\n", "USER", "PID", "%CPU", "%MEM", "VSZ", "RSS", "TTY", "STAT", "START", "TIME", "COMMAND");
		int totalmem = get_totalmem(); //total memory 구하기
		for(int i=0; i<pid_num; i++){
			tokens = tokenize(proc_stat[i]); //stat정보 토큰 단위로 저장
			if(strcmp(uid, username[i])==0){ //터미널 사용자 이름과 프로세스 user 이름이 같다면
				tty = get_tty(atoi(tokens[6])); //tty 구하기
				cpu_time = get_cputime(atoi(tokens[13]), atoi(tokens[14]), argc); //cpu 사용시간 구하기
				int first = (atoi(tokens[23])*4*100)/totalmem; //%mem의 정수 구하기
				int second = (((atoi(tokens[23])*4*100)-totalmem*first)*10)/totalmem; //%mem의 소수점 자리수 구하기
				
				int pcpu = get_pcpu(atoi(tokens[13]), atoi(tokens[14]), atoi(tokens[21]), seconds); //%cpu 구하기
				cmd = get_cmd(atoi(tokens[0])); //cmd 구하기
				starttime = get_starttime(atof(tokens[21]),seconds); //프로세스 시작시간 구하기
				printf("%s %11s %2d.%d %2d.%d %8.0f %7d %6s %6s %8s %5s %2s\n", username[i], tokens[0], pcpu/10, pcpu%10, first,second, atof(tokens[22])/1024, atoi(tokens[23])*4, tty, tokens[2],starttime, cpu_time, cmd);
			}
		}
	}

	else if(a==1&&u==1&&x==1){ //옵션 'aux'입력
		printf("%17s %10s %2s %2s %8s %7s %6s %6s %8s %5s %2s\n", "USER", "PID", "%CPU", "%MEM", "VSZ", "RSS", "TTY", "STAT", "START", "TIME", "COMMAND");
		int totalmem = get_totalmem(); //total memory 구하기
		for(int i=0; i<pid_num; i++){
			tokens = tokenize(proc_stat[i]); //stat 정보 토큰 단위로 저장
			tty = get_tty(atoi(tokens[6])); //tty 구하기
			cpu_time = get_cputime(atoi(tokens[13]), atoi(tokens[14]), argc); //cpu 사용시간 구하기
			int first = (atoi(tokens[23])*4*100)/totalmem; //%mem의 정수 구하기
			int second = (((atoi(tokens[23])*4*100)-totalmem*first)*10)/totalmem; //%mem의 소수점 자리수 구하기				
			int pcpu = get_pcpu(atoi(tokens[13]), atoi(tokens[14]), atoi(tokens[21]), seconds);	//%cpu 구하기
			starttime = get_starttime(atof(tokens[21]),seconds); //프로세스 시작시간 구하기
			if(strcmp(username[i], "root")!=0){ //프로세스 user이름이 root라면
				cmd = get_cmd(atoi(tokens[0])); //cmd 구하기
				printf("%17s %11s %2d.%d %2d.%d %8.0f %7d %6s %6s %8s %5s %2s\n", username[i], tokens[0], pcpu/10, pcpu%10, first,second, atof(tokens[22])/1024, atoi(tokens[23])*4, tty, tokens[2],starttime, cpu_time, cmd);
			}
			else
				printf("%17s %11s %2d.%d %2d.%d %8.0f %7d %6s %6s %8s %5s %2s\n", username[i], tokens[0], pcpu/10, pcpu%10, first,second, atof(tokens[22])/1024, atoi(tokens[23])*4, tty, tokens[2],starttime, cpu_time, tokens[1]);
		}
	}

	/*메모리 할당 해제*/
	for(int i=0; tokens[i]!=NULL; i++)
		free(tokens[i]);
	free(tty);
	free(cpu_time);
	free(starttime);
	free(cmd);
	return 0;
}

char **tokenize(char *line){ //토큰 분리 함수

	char **tokens = (char **)malloc(MAX_NUM_TOKENS * sizeof(char *));
	char *token = (char *)malloc(MAX_TOKEN_SIZE * sizeof(char));
	int i, tokenIndex = 0, tokenNo = 0;
	
	for(i =0; i < strlen(line); i++){
		char readChar = line[i];
	
		if (readChar == ' ' || readChar == '\n' || readChar == '\t'){
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

void get_pid(void){ //pid 저장
	DIR *dp;
	struct dirent *dirp;
	struct stat statbuf; //파일의 정보를 담을 stat 구조체
	int cpid = getpid();

	if((dp=opendir("/proc"))==NULL) {//"/proc" 디렉토리 오픈
		fprintf(stderr, "/proc open error\n");
		exit(0);
	}

	while((dirp = readdir(dp)) != NULL) //open된 디렉토리의 파일들 읽기
	{
		char pid[300] = {0,};

		if(!strcmp(dirp->d_name, ".") || !strcmp(dirp->d_name, "..")) //'.'과 '..'는 현재 폴더와 이전 폴더를 가리키므로 제외
			continue;

		sprintf(pid,"/proc/%s/stat", dirp->d_name);
        
		if (access(pid, F_OK) != 0) //stat 파일이 존재하는지 확인 
                continue;

		if(is_digit(dirp->d_name)==1){ //디렉토리 이름이 숫자라면

			struct passwd *upasswd;
            stat(pid,&statbuf);
			upasswd = getpwuid(statbuf.st_uid); //소유자의 사용자 ID 가져오기
            strcpy(username[pid_num], upasswd->pw_name);
			strcpy(proc_pid[pid_num], pid);
			
			get_stat(pid); //pid stat정보 가져오기
			
			if(atoi(dirp->d_name)==cpid); //현재 pid랑 같다면 저장
				strcpy(cur_pid, stat_info[pid_num]); 
			
			sprintf(proc_stat[pid_num], "%s", stat_info[pid_num]);
			pid_num++;
		}
	}
}

void get_stat(char* proc){ //각각의 프로세스의 stat 저장
	FILE *fp;
	char buf[512] = {0,};
	
	fp = fopen(proc, "r");
	if(fp==NULL){
		fprintf(stderr,"open error for %s\n", proc);
		exit(1);
	}

	fgets(buf, 511, fp);
	strncpy(stat_info[pid_num],buf,512);
	
	//printf("buf %s\n", buf); 
	//printf("stat %s\n", stat_info[pid_num]);
	fclose(fp);
}

char* get_cmd(int pid){ //명령어 가져오기
	char fname[100] = {0,};
	char buff[1024] = {0,};
	int cmdfd, ret;
	int num=0;
	char *cmd = (char*)malloc(MAX_INPUT_SIZE*sizeof(char));
	char tmp[MAX_NUM_TOKENS][MAX_INPUT_SIZE] = {0,};
    sprintf(fname, "/proc/%d/cmdline", pid);
    if ((cmdfd = open(fname, O_RDONLY)) < 0)
    {
        fprintf(stderr, "open error for cmdline\n");
		exit(1);
    }

	ret = read(cmdfd, buff, sizeof(buff)); //cmdline가져오기
    close(cmdfd);
    if (ret > strlen(buff)+1) //cmd 토큰이 2개 이상인 경우
    {
        char *nptr = strchr(buff, '\0');
		char *sptr = nptr + 1;

		sprintf(tmp[num++], "%s", buff); //첫번째 토큰 저장
		sprintf(tmp[num++], " %s", sptr); //두번째 토큰 저장
		while((nptr=strchr(sptr,'\0'))!=NULL){ // '/0'갯수 구하고 각 토큰 저장
			sptr = nptr + 1;
			if(strlen(sptr)==0)
				break;
			sprintf(tmp[num++], " %s", sptr);}

		for(int j=1; j<num; j++) //토큰들 합치기
			strcat(tmp[0],tmp[j]);

		sprintf(cmd, "%s", tmp[0]);

    }
    else
    {
        strcpy(cmd, buff);
    }

	return cmd;
}

char *get_tty(int tty_nr){ //tty 구하기
	char *tty = (char*)malloc(MAX_TOKEN_SIZE * sizeof(char));

	int major_num = MAJOR(tty_nr);
	int minor_num = MINOR(tty_nr);

	if(major_num==4)
		sprintf(tty, "tty%d", minor_num); 

	else if(major_num==5)
		tty = "tty";

	else if(major_num==136)
		sprintf(tty, "pts/%d", minor_num);
	
	else if(major_num==0)
		tty = "?";

	return tty;
}

int get_totalmem(void){ //total memory 구하기
	char **tokens = (char **)malloc(MAX_NUM_TOKENS * sizeof(char *));
	FILE *fp;
	char buf[100] = {0,};

	fp = fopen("/proc/meminfo", "r");
	if(fp==NULL){
		fprintf(stderr,"open error for /proc/meminfo\n");
		exit(1);
	}

	fgets(buf, 100, fp);

	tokens = tokenize(buf);

	return atoi(tokens[1]);
}

char *get_cputime(ulong utime, ulong stime, int argc){ //cpu 사용시간 구하기
	int cpu_time;
	char *result = (char*)malloc(MAX_TOKEN_SIZE * sizeof(char));
	cpu_time = (utime + stime)/sysconf(_SC_CLK_TCK);
	int hour = cpu_time / 3600;
	int min = (cpu_time-(3600*hour))/60;
	int sec = cpu_time-(3600*hour)-(60*min);

	if(argc==1){
		char *str_hour = time_str(hour, argc, 'h');
		char *str_min = time_str(min, argc, 'm');
		char *str_sec = time_str(sec, argc, 's');

		sprintf(result, "%s:%s:%s", str_hour,str_min, str_sec);}
	
	else if(argc==2){
		char *str_min = time_str(min, argc, 'm');
		char *str_sec = time_str(sec, argc, 's');

		sprintf(result, "%s:%s", str_min, str_sec);}

	return result;
}

char *time_str(int time, int argc, char m){ //시간을 형태에 맞게 저장
	char *str_time = (char*)malloc(3);

	if(argc==1){
		if(time == 0)
			str_time = "00";
	
		else if(0<time&&time<10)
			sprintf(str_time, "0%d", time);
	
		else 
			sprintf(str_time, "%d", time);}

	else if(argc==2){
		if(time==0 && m == 'm')
			str_time = "0";
		else if(time==0 && m == 's')
			str_time = "00";
		else if(time!=0&&m=='m')
			sprintf(str_time, "%d", time);
		else if(time>9&&m=='s')
			sprintf(str_time, "%d", time);
		else if(0<time&&time<10&&m=='s')
			sprintf(str_time, "0%d", time);
	}

	return str_time;
}
	
int get_pcpu(ulong utime, ulong stime, ulong starttime, int seconds) //%cpu 구하기
{   
    unsigned long long total_time;

    total_time = utime + stime;
	int pcpu = 0;
   	seconds = seconds - (int)(starttime/sysconf(_SC_CLK_TCK));

    if(seconds)
    {
		pcpu = (int)(total_time * 1000ULL/sysconf(_SC_CLK_TCK))/seconds;
	}

    return pcpu;
}

char *get_starttime(float start, int uptime) //프로세스의 시작시간 구하기
{
	char *result = (char*)malloc(10*sizeof(char));
	char *hour = (char*)malloc(3*sizeof(char));
	char *min = (char*)malloc(3*sizeof(char));
	double proc_starttime = start / sysconf(_SC_CLK_TCK); //프로세스의 시스템 부팅시간
	time_t current; //현재시간
	struct tm *proc_time; //tm 구조체
	
	time(&current); //현재시간 구하기

	double first_start = current - uptime; //처음 시스템이 시작한 시간
	time_t starttime = first_start + (start/sysconf(_SC_CLK_TCK)); 
	proc_time = localtime(&starttime);

	if((proc_time->tm_hour)<10)
		sprintf(hour, "0%d", proc_time->tm_hour);
	else
		sprintf(hour, "%d", proc_time->tm_hour);

	if((proc_time->tm_min)<10)
		sprintf(min, "0%d", proc_time->tm_min);

	else
		sprintf(min, "%d", proc_time->tm_min);

	sprintf(result, "%s:%s", hour, min);

	return result;
}


int get_uptime(void) //시스템 시작시간 구하기
{
    FILE *fp;
    char buf[36];
    double stime;
    double idletime;

    if ((fp = fopen("/proc/uptime", "r")) == NULL)
    {
        fprintf(stderr,"fopen error %s\n", "/proc/uptime");
        exit(0);
    }

    fgets(buf, 36, fp);
    sscanf(buf, "%lf %lf", &stime, &idletime);
    fclose(fp);

    return (int)stime;
}

int is_digit(char *str){ //pid가 숫자인지 확인
    for (int i = 0; i < strlen(str); i++){
        if (isdigit(str[i])==0)
            return 0;
    }
    return 1;
}
