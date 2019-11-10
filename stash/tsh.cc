// 
// tsh - A tiny shell program with job control
// 
// <Put your name and login ID here>
//

using namespace std;

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <string>

#include "globals.h"
#include "jobs.h"
#include "helper-routines.h"

#include <iostream>
#include <deque>
#include <list>
#include <queue>
#include <vector>


//
// Needed global variable definitions
//

static char prompt[] = "tsh> ";
int verbose = 0;

//
// You need to implement the functions eval, builtin_cmd, do_bgfg,
// waitfg, sigchld_handler, sigstp_handler, sigint_handler
//
// The code below provides the "prototypes" for those functions
// so that earlier code can refer to them. You need to fill in the
// function bodies below.
// 

void eval(char *cmdline);
int builtin_cmd(char **argv);
void do_bgfg(char **argv);
void waitfg(pid_t pid);

void sigchld_handler(int sig);
void sigtstp_handler(int sig);
void sigint_handler(int sig);

void Sigprocmask(int how, const sigset_t *set, sigset_t *oldest);

//Custom Error-Handling wrappers
pid_t Fork(void);
void Kill(pid_t pid, int sig);

//
// main - The shell's main routine 
//
int main(int argc, char **argv) 
{
  int emit_prompt = 1; // emit prompt (default)

  //
  // Redirect stderr to stdout (so that driver will get all output
  // on the pipe connected to stdout)
  //
  dup2(1, 2);

  //Recall that argc stands for argument count which contains the
  //number of arguments passed to the program
  //
  //argv contains the arguments passed to the program. Argv is
  //a 1-D array of strings
  //
  //source:  https://gnu.org/software/libc/manual/html_node/Using-Getopt.html#Using-Getopt

  /* Parse the command line */
  char c;
  while ((c = getopt(argc, argv, "hvp")) != EOF) {
    switch (c) {
    case 'h':             // print help message
      usage();
      break;
    case 'v':             // emit additional diagnostic info
      verbose = 1;
      break;
    case 'p':             // don't print a prompt
      emit_prompt = 0;  // handy for automatic testing
      break;
    default:
      usage();
    }
  }

  //
  // Install the signal handlers
  //

  //
  // These are the ones you will need to implement
  //
  Signal(SIGINT,  sigint_handler);   // ctrl-c
  Signal(SIGTSTP, sigtstp_handler);  // ctrl-z
  Signal(SIGCHLD, sigchld_handler);  // Terminated or stopped child

  //
  // This one provides a clean way to kill the shell
  //
  Signal(SIGQUIT, sigquit_handler); 

  //
  // Initialize the job list
  //
  initjobs(jobs);

  //
  // Execute the shell's read/eval loop
  //
  for(;;) {
    //
    // Read command line
    //
    if (emit_prompt) {
      printf("%s", prompt);
      fflush(stdout);
    }

    char cmdline[MAXLINE];

    if ((fgets(cmdline, MAXLINE, stdin) == NULL) && ferror(stdin)) {
      app_error("fgets error");
    }
    //
    // End of file? (did user type ctrl-d?)
    //
    if (feof(stdin)) {
      fflush(stdout);
      exit(0);
    }

    //
    // Evaluate command line
    //
    //
    eval(cmdline);
    fflush(stdout);
    fflush(stdout);
  } 

  exit(0); //control never reaches here
}
  
/////////////////////////////////////////////////////////////////////////////
//
// eval - Evaluate the command line that the user has just typed in
// 
// If the user has requested a built-in command (quit, jobs, bg or fg)
// then execute it immediately. Otherwise, fork a child process and
// run the job in the context of the child. If the job is running in
// the foreground, wait for it to terminate and then return.  Note:
// each child process must have a unique process group ID so that our
// background children don't receive SIGINT (SIGTSTP) from the kernel
// when we type ctrl-c (ctrl-z) at the keyboard.
//
void eval(char *cmdline) 
{
  /* Parse command line */
  //
  // The 'argv' vector is filled in by the parseline
  // routine below. It provides the arguments needed
  // for the execve() routine, which you'll need to
  // use below to launch a process.
  //
  char *argv[MAXARGS];
  pid_t pid; /* Process id*/
  sigset_t mask;

  //cout <<  "eval()" <<  " : " <<  cmdline << endl;

  


  //
  // The 'bg' variable is TRUE if the job should run
  // in background mode or FALSE if it should run in FG
  //

  int bg = parseline(cmdline, argv); 
  if (argv[0] == NULL)
    return;   /* ignore empty lines */

  //Pass the array of string commands to the builtin function
  //To determine if the first command is one of the selected builtins
  //(quit, jobs, bg, or fg)
 
  if (!builtin_cmd(argv))
  {
	  //cout << "not a built in function..." << endl;
	  //cout <<  "builtin argv[0]" <<  " : " <<  argv[0] << endl;

	  //If the first command is not a builtin, then Fork the process
	  //pid = Fork();


	  //We know if we are in the forked child process, if the fork command returns 0
	  if((pid = Fork()) == 0)
	  {
		 setpgid(0,0);
		  
		 
		  //Take the first string command, and pass in the rest as an argument vector
		  //If this fails, print out an error message and exit ther process
		  if(execv(argv[0], argv) < 0)
		  {
			 // printf("Command does not exist!\n");
			 // cout <<  "builtin argv[0]" <<  " : " <<  argv[0] << endl;
			  exit(0);
		  }



		  return;


	  }


	  //If we are not in a background process, have the parent wait for the child process
	  //then reap the child process once it's done	
	  if(!bg)
	  {
		  //Wait, but we don't really care about the return value from waiting for the child process
		  //equivalet to  waitpid(-1, &status, 0)
		  //where pid = -1 pg 744 (the wait set consists of all t parent's child processes. Otherwise pid > 0 is a specific child id.
		  //where &status = exit status tat will be set to parent by child
		  //optio = 0 in this case parent will wait until the child is terminate
		  //cout << "foreground process command: " << argv[0] << endl;
		  cout << "foreground pid process launched:" << pid << endl;
		 // addjob(jobs, pid, 1, cmdline);
		 // Sigprocmask(SIG_UNBLOCK, &mask, NULL);
		  
		  //listjobs(jobs);
		  waitfg(pid);
		  cout << "process pid returned and deleted " << pid << endl;
		  //deletejob(jobs, pid); 

	  }

	  //If we are in the background process, print the value of bg, the PID value and the command.	
	  else
	  {


		 //User the built in features of the lab
		 addjob(jobs, pid, 2, cmdline);
		 Sigprocmask(SIG_UNBLOCK, &mask, NULL);
		 printf("[%d] (%d) %s",pid2jid(pid), pid, cmdline);


		  
	  }
  }

  return;
}


/////////////////////////////////////////////////////////////////////////////
//
// builtin_cmd - If the user has typed a built-in command then execute
// it immediately. The command name would be in argv[0] and
// is a C string. We've cast this to a C++ string type to simplify
// string comparisons; however, the do_bgfg routine will need 
// to use the argv array as well to look for a job number.
//
int builtin_cmd(char **argv) 
{
  string cmd(argv[0]);

 //cout << argv[0] << endl;


 
  //Check to see if the user wants to exit the shell
  //if the first command is "quit" exit the tiny shell.
  if(strcmp("quit", argv[0]) == 0)
  {
	  exit(0);
  }

  if(strcmp("jobs", argv[0]) == 0)
  {

	  listjobs2(jobs, 2);  
	  return 1;
  }





  return 0;     /* not a builtin command */
}

/////////////////////////////////////////////////////////////////////////////
//
// do_bgfg - Execute the builtin bg and fg commands
//
void do_bgfg(char **argv) 
{
  struct job_t *jobp=NULL;
    
  /* Ignore command if no argument */
  if (argv[1] == NULL) {
    printf("%s command requires PID or %%jobid argument\n", argv[0]);
    return;
  }
    
  /* Parse the required PID or %JID arg */
  if (isdigit(argv[1][0])) {
    pid_t pid = atoi(argv[1]);
    if (!(jobp = getjobpid(jobs, pid))) {
      printf("(%d): No such process\n", pid);
      return;
    }
  }
  else if (argv[1][0] == '%') {
    int jid = atoi(&argv[1][1]);
    if (!(jobp = getjobjid(jobs, jid))) {
      printf("%s: No such job\n", argv[1]);
      return;
    }
  }	    
  else {
    printf("%s: argument must be a PID or %%jobid\n", argv[0]);
    return;
  }

  //
  // You need to complete rest. At this point,
  // the variable 'jobp' is the job pointer
  // for the job ID specified as an argument.
  //
  // Your actions will depend on the specified command
  // so we've converted argv[0] to a string (cmd) for
  // your benefit.
  //
  string cmd(argv[0]);

  return;
}

/////////////////////////////////////////////////////////////////////////////
//
// waitfg - Block until process pid is no longer the foreground process
//
void waitfg(pid_t pid)
{
       	return;
}

/////////////////////////////////////////////////////////////////////////////
//
// Signal handlers
//


/////////////////////////////////////////////////////////////////////////////
//
// sigchld_handler - The kernel sends a SIGCHLD to the shell whenever
//     a child job terminates (becomes a zombie), or stops because it
//     received a SIGSTOP or SIGTSTP signal. The handler reaps all
//     available zombie children, but doesn't wait for any other
//     currently running children to terminate.  
//
void sigchld_handler(int sig) 
{
	//cout << "sigchld_handler called with a sig value of: " <<sig << endl;
	return;
}

/////////////////////////////////////////////////////////////////////////////
//
// sigint_handler - The kernel sends a SIGINT to the shell whenver the
//    user types ctrl-c at the keyboard.  Catch it and send it along
//    to the foreground job.  
//
void sigint_handler(int sig) 
{
	cout << "sigint_handler called with a sig value of: " << sig << endl;
	//If a signal of 2 is sent ensure that pid is also not zero
	
	pid_t pid = fgpid(jobs);

	cout << "sigint_handler found a fg procsss with a pid of: " << pid << endl;


	job_t *deadjob = getjobpid(jobs, pid);

	if(sig == 2 && pid != 0)
	{
		cout << "Kill Called from sigint!" << endl;
		Kill(-pid, sig);
	}

	printf("Job [%d] (%d) terminated by signal 2\r\n", deadjob->jid, deadjob->pid);

       	return;
}

/////////////////////////////////////////////////////////////////////////////
//
// sigtstp_handler - The kernel sends a SIGTSTP to the shell whenever
//     the user types ctrl-z at the keyboard. Catch it and suspend the
//     foreground job by sending it a SIGTSTP.  
//
void sigtstp_handler(int sig)
{

	//cout << "sigstp_handler called with a a sign value of: " << sig << endl;


	pid_t pid = fgpid(jobs);

	if(sig == 2 && pid != 0)
	{
		
		cout << "Kill Called from sigstp!" << endl;
		Kill(pid, sig);
	}
	
	return;
}

/*********************
 * End signal handlers
 *********************/


//Error Handling Wrapper as per Steves[110]
pid_t Fork(void)
{
	pid_t pid;

	if ((pid = fork()) < 0)
	{
		unix_error("Fork error");
	}

	return pid;
}

void Kill(pid_t pid, int sig)
{
	if(kill(pid, sig) < 0)
	{
		unix_error("kill error");
	}
}

void Sigprocmask(int how, const sigset_t *set, sigset_t *oldest)
{
	if(sigprocmask(how, set, oldest) < 0)
	{
		app_error("sigpromask error");
	}

}

