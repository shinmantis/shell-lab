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
static char childprocs = 0;

struct child
{
    short jobID = 0;
    pid_t pid = 0;
    char* state;
    char* location;
    char* command;
} ;

deque<child*> jobHolder;


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

void job_printer(child* process);
child* buildChild(int pid, short jobID, char* state, char* location, char* command);

//Custom Error-Handling wrappers
pid_t Fork(void);

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

    if ((fgets(cmdline, MAXLINE, stdin) == nullptr) && ferror(stdin)) {
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

    //cout <<  "eval()" <<  " : " <<  cmdline << endl;




    //
    // The 'bg' variable is TRUE if the job should run
    // in background mode or FALSE if it should run in FG
    //
    int bg = parseline(cmdline, argv);
    if (argv[0] == nullptr)
        return;   /* ignore empty lines */

    //Pass the array of string commands to the builtin function
    //To determine if the first command is one of the selected builtins
    //(quit, jobs, bg, or fg)
    if (!builtin_cmd(argv))
    {
        //cout << "not a built in function..." << endl;
        //cout <<  "builtin argv[0]" <<  " : " <<  argv[0] << endl;

        //If the first command is not a builtin, then Fork the process
        pid = Fork();


        //We know if we are in the forked child process, if the fork command returns 0
        if(pid == 0)
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

            child* c = buildChild(1, pid, "Running", "FG", cmdline);
            jobHolder.push_back(c);

            //cout << "Foreground process called " << pid <<endl;
            wait(nullptr);
            jobHolder.front();
            jobHolder.pop_front();

        }

            //If we are in the background process, print the value of bg, the PID value and the command.
        else
        {

            short jobID = --childprocs;

            printf("[%d] (%d) %s",jobID, pid, cmdline);

            //creat a child structure to store child fork information
            child* c = buildChild(1, pid, "Running", "BG", cmdline);

            //quality checks
            //cout << "structure checK " << " jobID: " << mychild.jobID << endl;
            //cout << "structure checK " << " pid: " << mychild.pid << endl;
            //cout << "structure checK " << " state: " << mychild.state << endl;
            //cout << "structure checK " << " cmd " << mychild.command << endl;

            //store the structure in a queue
            jobHolder.push_back(c);
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


    if (strcmp(argv[0], "quit") == 0 || strcmp(argv[0], "q") == 0)
    {
        printf("Quitting...\n");
        exit(0);
    }

    if (strcmp(argv[0], "jobs") == 0 || strcmp(argv[0], "j") == 0)
    {
        for (child* c : jobHolder)
        {
            job_printer(c);
        }
    }

    if (strcmp(argv[0], "bg") == 0 || strcmp(argv[0], "fg") == 0)
    {
        //TODO: This is not finished.
        do_bgfg(argv);
    }

    return 0;     /* not a builtin command */
}

/////////////////////////////////////////////////////////////////////////////
//
// do_bgfg - Execute the builtin bg and fg commands
//
void do_bgfg(char **argv) 
{
  struct job_t *jobp = nullptr;
    
  /* Ignore command if no argument */
  if (argv[1] == nullptr) {
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
	//cout << "sigint_handler called with a sig value of: " << sig << endl;

	//If a signal of 2 is sent
	if(sig == 2)
	{

		//get the child process in the jobHolder
		child* mychild = jobHolder.front();

		//Update the state message of the structure
		mychild->state = "terminated by signal 2";

		//print out the job information
		cout << "Job [" << mychild->jobID <<"] ";
		cout << "(" << mychild->pid << ") ";
		cout << mychild->state << " " << endl;

		//Kill the process by passing along the appopriate signal and pid identifier 
		kill(mychild->pid, sig);
	}

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

	childprocs++;

	return pid;
}

/*
 * Child printer
 */
void job_printer(child* process)
{
    printf("pid: %d { job ID: %d, state: %s, location: %s, command: %s}",
           process->pid, process->jobID, process->state, process->location, process->command);
}

child* buildChild(int pid, short jobID, char* state, char* location, char* command)
{
    auto *c = new child();
    c->pid = pid;
    c->jobID = jobID;
    c->state = state;
    c->location = location;
    c->command = command;
    return c;
}