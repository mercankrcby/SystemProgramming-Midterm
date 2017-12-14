#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <signal.h>
#include <string.h>

#define BUF_SIZE 100
#define ERROR -1
#define SLEEPGOOD 100000
#define SHOW_RES_FIFO "show_res.ff"

static volatile int sig_exit_occur = 0;
void sig_exit_handler(int signo) {
  sig_exit_occur = 1;
  fprintf(stderr, "Signal:%d handled\n",signo);
}


int main(int argc, char* argv[]) {

    int input_fd, output_fd;    /* Input and output file descriptors */
    ssize_t ret_in, ret_out;    /* Number of bytes returned by read() and write() */
    char buffer[BUF_SIZE];      /* Character buffer */

    /* Are src and dest file name arguments missing */
    if(argc != 1){
        printf ("Usage: ./showResults");
        return 1;
    }

    signal(SIGINT, sig_exit_handler);

    /* show result fifo*/
    if (mkfifo(SHOW_RES_FIFO, (S_IRUSR | S_IWUSR | S_IWGRP | S_IWOTH)) == ERROR) {
      
    }

    input_fd = open (SHOW_RES_FIFO, O_RDONLY);
    if (input_fd == -1) {
        perror ("open fifo");
        return 2;
    }



    /* Copy process */
    bzero(buffer,BUF_SIZE);
    while(!sig_exit_occur){
        while(!sig_exit_occur &&(ret_in = read (input_fd, &buffer, BUF_SIZE)) < 1){
            if(errno>0){
                sig_exit_occur=1;
            }
            usleep(SLEEPGOOD);   
        }

        printf("%s\n",buffer);
        bzero(buffer,BUF_SIZE);
    }

    /* Close file descriptors */
    close (input_fd);
    unlink(SHOW_RES_FIFO);

    return (EXIT_SUCCESS);
}
