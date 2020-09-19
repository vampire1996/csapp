/* 
 * tsh - A tiny shell program with job control
 * 
 * <Put your name and login ID here>
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

/* Misc manifest constants */
#define MAXLINE    1024   /* max line size */
#define MAXARGS     128   /* max args on a command line */
#define MAXJOBS      16   /* max jobs at any point in time */
#define MAXJID    1<<16   /* max job ID */

/* Job states */
#define UNDEF 0 /* undefined */
#define FG 1    /* running in foreground */
#define BG 2    /* running in background */
#define ST 3    /* stopped */

/* 
 * Jobs states: FG (foreground), BG (background), ST (stopped)
 * Job state transitions and enabling actions:
 *     FG -> ST  : ctrl-z
 *     ST -> FG  : fg command
 *     ST -> BG  : bg command
 *     BG -> FG  : fg command
 * At most 1 job can be in the FG state.
 */

/* Global variables */
extern char **environ;      /* defined in libc */
char prompt[] = "tsh> ";    /* command line prompt (DO NOT CHANGE) */
int verbose = 0;            /* if true, print additional output */
int nextjid = 1;            /* next job ID to allocate */
char sbuf[MAXLINE];         /* for composing sprintf messages */

struct job_t {              /* The job struct */
    pid_t pid;              /* job PID */
    int jid;                /* job ID [1, 2, ...] */
    int state;              /* UNDEF, BG, FG, or ST */
    char cmdline[MAXLINE];  /* command line */
};
struct job_t jobs[MAXJOBS]; /* The job list */

volatile sig_atomic_t busy;
/* End global variables */


/* Function prototypes */

/* Here are the functions that you will implement */
void eval(char *cmdline);
int builtin_cmd(char **argv);
void do_bgfg(char **argv);
void waitfg(pid_t pid);

void sigchld_handler(int sig);
void sigtstp_handler(int sig);
void sigint_handler(int sig);

/* Here are helper routines that we've provided for you */
int parseline(const char *cmdline, char **argv); 
void sigquit_handler(int sig);

void clearjob(struct job_t *job);
void initjobs(struct job_t *jobs);
int maxjid(struct job_t *jobs); 
int addjob(struct job_t *jobs, pid_t pid, int state, char *cmdline);
int deletejob(struct job_t *jobs, pid_t pid); 
pid_t fgpid(struct job_t *jobs);
struct job_t *getjobpid(struct job_t *jobs, pid_t pid);
struct job_t *getjobjid(struct job_t *jobs, int jid); 
int pid2jid(pid_t pid); 
void listjobs(struct job_t *jobs);

void usage(void);
void unix_error(char *msg);
void app_error(char *msg);
typedef void handler_t(int);
handler_t *Signal(int signum, handler_t *handler);

/*
 * main - The shell's main routine 
 */
int main(int argc, char **argv) 
{
    char c;
    char cmdline[MAXLINE];
    int emit_prompt = 1; /* emit prompt (default) */

    /* Redirect stderr to stdout (so that driver will get all output
     * on the pipe connected to stdout) */
    dup2(1, 2);

    /* Parse the command line */
    while ((c = getopt(argc, argv, "hvp")) != EOF) {
        switch (c) {
        case 'h':             /* print help message */
            usage();
	    break;
        case 'v':             /* emit additional diagnostic info */
            verbose = 1;
	    break;
        case 'p':             /* don't print a prompt */
            emit_prompt = 0;  /* handy for automatic testing */
	    break;
	default:
            usage();
	}
    }

    /* Install the signal handlers */

    /* These are the ones you will need to implement */
    Signal(SIGINT,  sigint_handler);   /* ctrl-c */
    Signal(SIGTSTP, sigtstp_handler);  /* ctrl-z */
    Signal(SIGCHLD, sigchld_handler);  /* Terminated or stopped child */

    /* This one provides a clean way to kill the shell */
    Signal(SIGQUIT, sigquit_handler); 

    /* Initialize the job list */
    initjobs(jobs);

    /* Execute the shell's read/eval loop */
    while (1) {

	/* Read command line */
	if (emit_prompt) {
	    printf("%s", prompt);
	    fflush(stdout);
	}
	if ((fgets(cmdline, MAXLINE, stdin) == NULL) && ferror(stdin))
	    app_error("fgets error");
	if (feof(stdin)) { /* End of file (ctrl-d) */
	    fflush(stdout);
	    exit(0);
	}

	/* Evaluate the command line */
	eval(cmdline);
	fflush(stdout);
	fflush(stdout);
    } 

    exit(0); /* control never reaches here */
}
  
/* 
 * eval - Evaluate the command line that the user has just typed in
 * 
 * If the user has requested a built-in command (quit, jobs, bg or fg)
 * then execute it immediately. Otherwise, fork a child process and
 * run the job in the context of the child. If the job is running in
 * the foreground, wait for it to terminate and then return.  Note:
 * each child process must have a unique process group ID so that our
 * background children don't receive SIGINT (SIGTSTP) from the kernel
 * when we type ctrl-c (ctrl-z) at the keyboard.  
*/
//后台运行进程是不受SIGINT(终止前台进程) 和SIGSTP(停止前台进程)影响的
void eval(char *cmdline) 
{
    char *argv[MAXARGS];  //argument list of execve()
    char buf[MAXLINE];    //holds mofidified command lines
    int bg;               // if work in background bg=1
    pid_t pid;            //process id
    sigset_t mask_one,prev,mask_all;
   
    sigfillset(&mask_all);
    sigemptyset(&mask_one);
    sigaddset(&mask_one,SIGCHLD);
   
    
    strcpy(buf,cmdline);
    bg=parseline(buf,argv);
    if(argv[0]==NULL) return ;//ignore empty lines
  
    if(!builtin_cmd(argv))
    {
        sigprocmask(SIG_BLOCK,&mask_one,&prev);//在父进程中阻塞SIGCHLD  避免addjob和deletejob竞争--在addjob之前已经在ahndler中调用了deletejob
        if((pid=fork())==0)
        {
           sigprocmask(SIG_SETMASK,&prev,NULL);//在子进程中取消阻塞SIGCHLD
           /*
           当前运行的shell是运行在前台进程组。如果你的进程创建了一个孩子进程,按照默认设置 那个孩子进程也是前台进程组的一员 
           如果键入CTRL+C，发送SIGINT到前台每个进程，SIGINT也会发送进程SIGINT到你的shell，以及每个shell创建的进程，这显然是不对的
           这里的变通方法是:在fork之后，execve之前，子进程中调用setpgid(0,0) 将孩子进程放到一个新的进程组中--group ID=孩子的PID 
           这保证了只有一个进程==你的shell 会在前台进程组 当你输入ctrl+c shell会捕获SIGINT然后发送到合适的前台工作中(更准确的说:包含前台工作的进程组)

           理解:这里前台进程 后台进程的概念只考虑shell父进程创建的子进程 以及执行execve创建的子进程 
           shell父进程只要其它子进程结束 它进行一些处理工作也就结束 所以不考虑在前台进程的范围之内
           因此setpgid(0,0) 将建立一个新的进程组 将execve所在子进程的pid作为进程组id--对应一个job  
           该进程可能创建新的子进程 也属于该进程组，构成一个前台或者后台进程组(由job.state确定)
           见课本图8-28
           */
           setpgid(0,0);
           if(execve(argv[0],argv,environ)<0)
           {
              printf("%s: Command not found.\n",argv[0]);
              exit(0); 
           }
        }
        
       
        int state=bg?BG:FG;
        sigprocmask(SIG_BLOCK,&mask_all,NULL);//必须在加入工作列表时屏蔽所有信号  访问全局变量
        addjob(jobs, pid, state, cmdline); //将子进程加入到工作列表

        //parent waits for foreground job to terminate 
        sigprocmask(SIG_SETMASK,&prev,NULL);//在父进程中等待前台子进程结束时中取消阻塞
        if(!bg) waitfg(pid);  
        else
        {
            struct job_t *cur_job=NULL;
            cur_job=getjobpid(jobs, pid);
            printf("[%d] (%d) %s", cur_job->jid, cur_job->pid,cur_job->cmdline);
        }
        //sigprocmask(SIG_SETMASK,&prev,NULL);
    }
    return;
}

/* 
 * parseline - Parse the command line and build the argv array.
 * 
 * Characters enclosed in single quotes are treated as a single
 * argument.  Return true if the user has requested a BG job, false if
 * the user has requested a FG job.  
 */
int parseline(const char *cmdline, char **argv) 
{
    static char array[MAXLINE]; /* holds local copy of command line */
    char *buf = array;          /* ptr that traverses command line */
    char *delim;                /* points to first space delimiter */
    int argc;                   /* number of args */
    int bg;                     /* background job? */

    strcpy(buf, cmdline);
    buf[strlen(buf)-1] = ' ';  /* replace trailing '\n' with space */
    while (*buf && (*buf == ' ')) /* ignore leading spaces */
	buf++;

    /* Build the argv list */
    argc = 0;
    if (*buf == '\'') {
	buf++;
	delim = strchr(buf, '\'');
    }
    else {
	delim = strchr(buf, ' ');
    }

    while (delim) {
	argv[argc++] = buf;
	*delim = '\0';
	buf = delim + 1;
	while (*buf && (*buf == ' ')) /* ignore spaces */
	       buf++;

	if (*buf == '\'') {
	    buf++;
	    delim = strchr(buf, '\'');
	}
	else {
	    delim = strchr(buf, ' ');
	}
    }
    argv[argc] = NULL;
    
    if (argc == 0)  /* ignore blank line */
	return 1;

    /* should the job run in the background? */
    if ((bg = (*argv[argc-1] == '&')) != 0) {
	argv[--argc] = NULL;
    }
    return bg;
}

/* 
 * builtin_cmd - If the user has typed a built-in command then execute
 *    it immediately.  
 */
int builtin_cmd(char **argv) 
{
    sigset_t prev,mask_all;
    sigfillset(&mask_all);
    if(!strcmp(argv[0],"quit"))   //quit command
         exit(0);
    if(!strcmp(argv[0],"&"))   //ignore &
         return 1;
    if(!strcmp(argv[0],"bg"))   //background builtin cmd
    {
        do_bgfg(argv);  
        return 1;
    }
    if(!strcmp(argv[0],"fg"))   //background builtin cmd
    {
        do_bgfg(argv);  
        return 1;
    }
    if(!strcmp(argv[0],"jobs"))   //list all running or stopped jobs
    {
        //前台工作都已经结束 因此不会输出forground的结果
        //只要访问全局变量都需要阻塞信号
        sigprocmask(SIG_BLOCK,&mask_all,&prev); 
        listjobs(jobs); 
        sigprocmask(SIG_SETMASK,&prev,NULL);  
        return 1;
    }
    return 0;     /* not a builtin command */
}

/* 
 * do_bgfg - Execute the builtin bg and fg commands
 */
void do_bgfg(char **argv) 
{
    pid_t _pid;              /* job PID */
    int jid;                /* job ID [1, 2, ...] */ 
    struct job_t *cur_job=NULL;
    int isgroup=0;
    
    if(argv[1]==NULL)
    {
      printf("%s command requires PID ro %%jobid argument\n",argv[0]);
      return ;
    }    

    sigset_t prev,mask_all;
   
    sigfillset(&mask_all);
    
       
     
    //只要访问全局变量都需要阻塞信号
    sigprocmask(SIG_BLOCK,&mask_all,&prev); 
    if(argv[1][0]=='%')
    {
        ++argv[1];//忽略%
        if((jid=atoi(argv[1]))!=0)   
       {
           if((cur_job=getjobjid(jobs, jid))==NULL)
           {
               printf("[%d]: No such job\n",jid);
               return ;
           } 
       }
       else 
       { 
          printf("%s argument must be a PID ro %%jobid argument\n",argv[0]);
          return ;
       }
        --argv[1];
        isgroup=1;
    }
    else 
    {
       if((_pid=atoi(argv[1]))!=0)   
       {
           if((cur_job=getjobpid(jobs, _pid))==NULL)
           {
               printf("(%d): No such process\n",_pid);
               return ;
           } 
       }
       else 
       { 
          printf("%s argument must be a PID ro %%jobid argument\n",argv[0]);
          return ;
       }  
    }

    if(!strcmp(argv[0],"bg"))   //background builtin cmd
    {
        if(cur_job->state!=UNDEF)
        {
           
           
           if(cur_job->state==ST)
           {
              //如果是jid 
             if(isgroup)kill(-cur_job->pid,SIGCONT);  
             else kill(cur_job->pid,SIGCONT);  
           }
           printf("[%d] (%d) %s", cur_job->jid, cur_job->pid,cur_job->cmdline);
           cur_job->state=BG;//注意这里修改cur_job的状态一定要在发送SIGCONT之后 因为进程响应SIGCONT时还没有变更状态 
                             //所以如果发生中断结束当前进程 那么永远有一个待处理的BG job
           
        }
        else 
        {
           printf("%s: No such job \n",argv[1]);
        }
    }
    else if(!strcmp(argv[0],"fg"))   //background builtin cmd
    {
        if(cur_job->state!=UNDEF)
        {
           //cur_job->state=FG;
           //如果子进程本身是BG 那么无需发送SIGCONT 但是需要完成BG到FG的转换 这里只需要将state转换为FG 
          //由于是内建命令  所以本身jobs中没有FG工作 所以cur_job是唯一的FG工作 
          //在child_handler中会判断结束的工作是否是FG工作 将busy改变 实现结束waitfg的功能
           if(cur_job->state==ST)
           {
              //如果是jid 
             if(isgroup)kill(-cur_job->pid,SIGCONT);  
             else kill(cur_job->pid,SIGCONT);  
           }  
           cur_job->state=FG;        //注意这里修改cur_job的状态一定要在发送SIGCONT之后 因为进程响应SIGCONT时还没有变更状态 
                                     //所以如果发生中断结束当前进程 那么永远有一个待处理的BG job   
           waitfg(cur_job->pid); 
        }
        else 
        {
           printf("%s: No such job \n",argv[1]);
        }
    }
    sigprocmask(SIG_SETMASK,&prev,NULL); 
    return;
}

/* 
 * waitfg - Block until process pid is no longer the foreground process
 */
void waitfg(pid_t pid)
{
    sigset_t prev;
    sigemptyset(&prev);
    busy=0;
    while(!busy)sigsuspend(&prev);//使用sissuspend避免addjob和deletejob竞争 
    return;
}

/*****************
 * Signal handlers
 *****************/

/* 
 * sigchld_handler - The kernel sends a SIGCHLD to the shell whenever
 *     a child job terminates (becomes a zombie), or stops because it
 *     received a SIGSTOP or SIGTSTP signal. The handler reaps all
 *     available zombie children, but doesn't wait for any other
 *     currently running children to terminate.  
 */
//处理程序也是在父进程中执行的 只不过只有接收到信号或者待处理信号强制让父进程响应信号的时候才运行
//使用SSIGCHLD处理程序来回收子进程 而不是显示的等待子进程终止
void sigchld_handler(int sig) 
{
    int olderrno=errno;
    int status;
    int pid;
    
    sigset_t prev,mask_all;
    sigfillset(&mask_all);
    /*
    waitpid:waitpid(pid_t pid,int *statusp,int options)  返回 如果成功 则为子进程的pid 如果为WNOHANG则为0 如果其它错误 则为-1
    默认情况下 options=0 挂起调用进程的执行 直到等待集合中的一个子进程终止  如果等待集合中的一个进程在刚调用的时刻就已经终止了 那么waitpid就立即返回
    这两种情况下waitpid返回导致waitpid返回的已经终止的子进程的PID
    pid>0 那么等待集合就是一个单独的子进程，进程ID等于pid
    pid=-1 那么等待集合就是父进程的所有子进程
    修改默认行为-options:
    WNOHANG:如果等待集合中的任何子进程都还没有终止，就立即返回--no hang--无挂起
    WUNTRACED:挂起调用进程的执行，直到等待集合中的一个进程变成已经终止或者被停止 返回导致waitpid返回的已经终止或者被停止的子进程的PID
    WNOHANG | WUNTRACED 立即返回 如果等待集合中的子进程都没有被停止或者终止，则返回值位0;如果有一个停止或者终止,则返回值为该孩子进程的PID
    */
    while((pid=waitpid(-1,&status,WNOHANG | WUNTRACED))>0)
    {
        //只要访问全局变量都需要阻塞信号
        sigprocmask(SIG_BLOCK,&mask_all,&prev); 
        
        int fg_pid=fgpid(jobs);//如果是前台进程 那么结束busy
        if(fg_pid==pid)busy=pid;

        //检查已回收子进程的退出状态
        if(WIFEXITED(status)) //如果正常退出  (子进程调用exit或者返回一个return)
        {
           //printf("%d %d\n",fg_pid,pid);
           deletejob(jobs, pid);
        }
        else if(WIFSIGNALED(status))//如果因为一个被捕获的信号终止 那么返回true
        {
           int cur_sig=WTERMSIG(status);//返回导致子进程终止的信号的编号
           printf("Job [%d] (%d) treminated by signal %d \n",pid2jid(pid),pid,cur_sig);   
           deletejob(jobs, pid);
             
        }

        else if(WIFSTOPPED(status))//如果引起返回的信号是停止的，那么返回true
        {
           struct job_t *cur_job=NULL;
           cur_job=getjobpid(jobs, pid);
           cur_job->state=ST;
           int cur_sig=WSTOPSIG(status);//返回导致子进程停止的信号的编号
           printf("Job [%d] (%d) stoppeded by signal %d \n",pid2jid(pid),pid,cur_sig);   
        }
        sigprocmask(SIG_SETMASK,&prev,NULL); 
    }
    
    
    
 
    errno=olderrno; 
    return;
}

/* 
 * sigint_handler - The kernel sends a SIGINT to the shell whenver the
 *    user types ctrl-c at the keyboard.  Catch it and send it along
 *    to the foreground job.  
 */
//默认的sigint_handler是在接收到SIGINT信号之后利用内核将SIGINT发送到前台工作组的每一个进程
//这里需要实现这种功能 当当前进程接受到SIGINT之后 将SIGINT发送到前台工作组的每一个进程
void sigint_handler(int sig) 
{
    int olderrno=errno;
    sigset_t prev,mask_all;
    sigfillset(&mask_all);
    //只要访问全局变量都需要阻塞信号
    sigprocmask(SIG_BLOCK,&mask_all,&prev); 
    int pid=fgpid(jobs);     //获取前台进程组ID   
    sigprocmask(SIG_SETMASK,&prev,NULL);  
    /*-pid<0 那么向|-pid|进程组的每一个进程都发送SIGINT信号--发送给前台进程组的每一个进程  */
    if(pid>0)kill(-pid,SIGINT);  
 
    errno=olderrno; 
    return;
}

/*
 * sigtstp_handler - The kernel sends a SIGTSTP to the shell whenever
 *     the user types ctrl-z at the keyboard. Catch it and suspend the
 *     foreground job by sending it a SIGTSTP.  
 */
void sigtstp_handler(int sig) 
{
    int olderrno=errno;

    sigset_t prev,mask_all;
    sigfillset(&mask_all);
    //只要访问全局变量都需要阻塞信号
    sigprocmask(SIG_BLOCK,&mask_all,&prev); 
    int pid=fgpid(jobs);  //获取前台进程组ID   
    sigprocmask(SIG_SETMASK,&prev,NULL);     

    if(pid>0)kill(-pid,SIGTSTP);  
 
    errno=olderrno; 
    return;
}

/*********************
 * End signal handlers
 *********************/

/***********************************************
 * Helper routines that manipulate the job list
 **********************************************/

/* clearjob - Clear the entries in a job struct */
void clearjob(struct job_t *job) {
    job->pid = 0;
    job->jid = 0;
    job->state = UNDEF;
    job->cmdline[0] = '\0';
}

/* initjobs - Initialize the job list */
void initjobs(struct job_t *jobs) {
    int i;

    for (i = 0; i < MAXJOBS; i++)
	clearjob(&jobs[i]);
}

/* maxjid - Returns largest allocated job ID */
int maxjid(struct job_t *jobs) 
{
    int i, max=0;

    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].jid > max)
	    max = jobs[i].jid;
    return max;
}

/* addjob - Add a job to the job list */
int addjob(struct job_t *jobs, pid_t pid, int state, char *cmdline) 
{
    int i;
    
    if (pid < 1)
	return 0;

    for (i = 0; i < MAXJOBS; i++) {
	if (jobs[i].pid == 0) {
	    jobs[i].pid = pid;
	    jobs[i].state = state;
	    jobs[i].jid = nextjid++;
	    if (nextjid > MAXJOBS)
		nextjid = 1;
	    strcpy(jobs[i].cmdline, cmdline);
  	    if(verbose){
	        printf("Added job [%d] %d %s\n", jobs[i].jid, jobs[i].pid, jobs[i].cmdline);
            }
            return 1;
	}
    }
    printf("Tried to create too many jobs\n");
    return 0;
}

/* deletejob - Delete a job whose PID=pid from the job list */
int deletejob(struct job_t *jobs, pid_t pid) 
{
    int i;

    if (pid < 1)
	return 0;

    for (i = 0; i < MAXJOBS; i++) {
	if (jobs[i].pid == pid) {
	    clearjob(&jobs[i]);
	    nextjid = maxjid(jobs)+1;
	    return 1;
	}
    }
    return 0;
}

/* fgpid - Return PID of current foreground job, 0 if no such job */
pid_t fgpid(struct job_t *jobs) {
    int i;

    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].state == FG)
	    return jobs[i].pid;
    return 0;
}

/* getjobpid  - Find a job (by PID) on the job list */
struct job_t *getjobpid(struct job_t *jobs, pid_t pid) {
    int i;

    if (pid < 1)
	return NULL;
    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].pid == pid)
	    return &jobs[i];
    return NULL;
}

/* getjobjid  - Find a job (by JID) on the job list */
struct job_t *getjobjid(struct job_t *jobs, int jid) 
{
    int i;

    if (jid < 1)
	return NULL;
    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].jid == jid)
	    return &jobs[i];
    return NULL;
}

/* pid2jid - Map process ID to job ID */
int pid2jid(pid_t pid) 
{
    int i;

    if (pid < 1)
	return 0;
    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].pid == pid) {
            return jobs[i].jid;
        }
    return 0;
}

/* listjobs - Print the job list */
void listjobs(struct job_t *jobs) 
{
    int i;
    
    for (i = 0; i < MAXJOBS; i++) {
	if (jobs[i].pid != 0) {
	    printf("[%d] (%d) ", jobs[i].jid, jobs[i].pid);
	    switch (jobs[i].state) {
		case BG: 
		    printf("Running ");
		    break;
		case FG: 
		    printf("Foreground ");
		    break;
		case ST: 
		    printf("Stopped ");
		    break;
	    default:
		    printf("listjobs: Internal error: job[%d].state=%d ", 
			   i, jobs[i].state);
	    }
	    printf("%s", jobs[i].cmdline);
	}
    }
}
/******************************
 * end job list helper routines
 ******************************/


/***********************
 * Other helper routines
 ***********************/

/*
 * usage - print a help message
 */
void usage(void) 
{
    printf("Usage: shell [-hvp]\n");
    printf("   -h   print this message\n");
    printf("   -v   print additional diagnostic information\n");
    printf("   -p   do not emit a command prompt\n");
    exit(1);
}

/*
 * unix_error - unix-style error routine
 */
void unix_error(char *msg)
{
    fprintf(stdout, "%s: %s\n", msg, strerror(errno));
    exit(1);
}

/*
 * app_error - application-style error routine
 */
void app_error(char *msg)
{
    fprintf(stdout, "%s\n", msg);
    exit(1);
}

/*
 * Signal - wrapper for the sigaction function
 */
handler_t *Signal(int signum, handler_t *handler) 
{
    struct sigaction action, old_action;

    action.sa_handler = handler;  
    sigemptyset(&action.sa_mask); /* block sigs of type being handled */
    action.sa_flags = SA_RESTART; /* restart syscalls if possible */

    if (sigaction(signum, &action, &old_action) < 0)
	unix_error("Signal error");
    return (old_action.sa_handler);
}

/*
 * sigquit_handler - The driver program can gracefully terminate the
 *    child shell by sending it a SIGQUIT signal.
 */
void sigquit_handler(int sig) 
{
    printf("Terminating after receipt of SIGQUIT signal\n");
    exit(1);
}



