/* Program name : Pomodoro Timer 
 * Author       : Geoffrey Olson Jr.
 * Date         : July 4, 2018
 * Purpose      : A time management tool/timer. Based off of the pomodoro technique. Set 
 * the length of your breaks you would like to take and how long you want to work 
 * conitnously between breaks. This program uses a state machine to manage whether 
 * it's tracking your work or break session or whether it's alerting you of the end
 * of a session. The alert flashes the screen by forking a new process that changes 
 * the background. When the user confirms they're ready to change states the process 
 * will end. 
 */

#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>


//holds the state and data for the state machine
struct settings{
  WINDOW * win;//pointer to the curses window
  int breaklen;
  int worklen;
  time_t start;
  int elapsed;
  enum{
    WORK,
    BREAK,
    ALARMW,//transitioning from WORK state
    ALARMB,//transitioning from BREAK state
    HALT
  }state;
};

//return an initialized settings struct
struct settings * initProc(char * argv[]){
  struct settings * proc = malloc(sizeof(struct settings));
  proc->worklen = atoi(argv[1])*60;
  proc->breaklen = atoi(argv[2])*60;
  proc->start = time(NULL);
  proc->elapsed = 0;
  proc->state = WORK;
}

//countdown function displays elapsed time
void countdown(int len, int start){
  int elapsed = 0;
  while( elapsed < len ){
    clear();//clears the screen
    elapsed = time(NULL) - start;
    printw("%d seconds left", len - elapsed);//prints remaining time
    refresh();
  }
}

//executes aplay with a wav file generating a beep
void beepWav(){
  char * args[4];
  args[0] = "aplay";
  args[1] = "-q";
  args[2] = "beep.wav";
  args[3] = NULL;
  pid_t pid = fork();
  if(pid == 0){
    execvp(args[0], args);
  }
}

//flashes the screen for the alarm to alert user a state change occurred
void flasher(struct settings * proc, pid_t pid){
  int pair = 0;
  if(pid == 0){
    beepWav();
    while(1){
      if(pair == 0)
        pair = 1; 
      else
        pair = 0;
      sleep(1);
      wbkgd(proc->win, COLOR_PAIR(pair));
      refresh();
    }
  }
}

//transition function for the alarm states
void alarmTrans(struct settings * proc){
  pid_t pid = fork();
  char brk[] = "break";
  char wrk[] = "work session";
  printw("press enter to start your %s", proc->state == ALARMW ? brk : wrk);
  flasher(proc, pid);
  getch();
  kill(pid, SIGKILL);
  wait(NULL);
  proc->state = (proc->state == ALARMW)? BREAK : WORK; 
  wbkgd(proc->win, COLOR_PAIR(0));
}


//state machine for managing work, breaks, alarms and halt
void manager( struct settings proc){
  while(proc.state != HALT){
    clear();
    switch(proc.state){
      case WORK :
        countdown(proc.worklen, time(NULL));
        proc.state = ALARMW;
        break;
      case BREAK :
        countdown(proc.breaklen, time(NULL));
        proc.state = ALARMB;
        break;
      case ALARMW :
        alarmTrans(& proc);
        break;
      case ALARMB :
        alarmTrans(& proc);
        break;
      default :
        printw("EXITING");
        break;
    }
  }
}

// TODO: add to ui an instruction on screen on how to exit and provide an alternative
// to ctrl-c
int main(int argc, char * argv[]){
  if(argc < 3){
    printf("Usage: %s [work length in minutes] [break length in minutes]\n", argv[0]);
    return -1;
  }
  struct settings * proc = initProc(argv);
  proc->win = initscr(); //creates standard screen
  cbreak(); //allows exiting with ctrl-x
  start_color();
  init_pair(0, COLOR_WHITE, COLOR_BLACK);//pair, foreground color, background color
  init_pair(1, COLOR_BLACK, COLOR_WHITE);//pair, foreground color, background color
  wbkgd(proc->win, COLOR_PAIR(0));
  manager(*proc); 
  getch();//used to pause the screen until user presses enter
  endwin();
  free(proc);
  return 0;
}
