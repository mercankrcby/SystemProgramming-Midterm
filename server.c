#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
  /* ********************************************
   Dosyaya yazma islemleri icin kullanilan Flag'ler*/
#define WRITE_FLAGS (O_WRONLY | O_CREAT | O_EXCL)
#define WRITE_PERMS (S_IRUSR | S_IWUSR)
  /* Kod icinde kullanilan makrolar*/
#define BUFFER_SIZE 100
#define MINI_BUFFER_SIZE 20
#define MAX_DIMENSION 20
#define MILLION 1000000L
#define MAX_FIFO_SIZE 400
#define ERROR -1
#define SLEEPGOOD 100000
#define MILITOMIKRO 1000
#define MAX_CHILD_NUM 255

  /* *********************************************/
# define MAIN_LOG_FILE "./logs/log.txt"

typedef struct {
  double array[MAX_DIMENSION][MAX_DIMENSION];
  int size;
}
matrix_t;

typedef struct{
  pid_t child[MAX_CHILD_NUM];
  int number;
}childs_t;

/*Fonksiyon prototipleri*/
double determinant(const matrix_t * matrix);
matrix_t * createMatrix(int dimension);
void printMatrix(const matrix_t * matrix);
matrix_t* combiningMatrix(matrix_t *oldMatrix1,matrix_t *oldMatrix2,matrix_t *oldMatrix3,matrix_t *oldMatrix4);
/*********************************************/
int childProcCon(pid_t clientPID, int matrixSize);
int copyFile(FILE * to, FILE *from);


static volatile int sig_exit_occur = 0;
void sig_exit_handler(int signo) {
  sig_exit_occur = 1;
  fprintf(stderr, "Signal:%d handled\n",signo);
}

static volatile int sig_req_occur = 0;
void sig_req_handler(int signo) {
  sig_req_occur = 1;
  /*fprintf(stderr, "Request Signal:%d handled\n",signo);*/
}

/* GLOBAL VARIABLES */
struct timespec tpend, tpstart;
childs_t parentChilds;
sigset_t sigmask; /* SIGINT maskeler */
int signalControlFreq;

int main(int argc, char * argv[]) {
    int fdMainFifo;
    char buffer[BUFFER_SIZE];
    char childLogName[MINI_BUFFER_SIZE];
    char logFileBuffer[BUFFER_SIZE];
    pid_t clientPID;
    pid_t childPID;
    pid_t pid;

    FILE *mainLog=NULL;
    FILE *childLog=NULL;

    int toFD;
    ssize_t bytesRead;
    int dimension=0;
    matrix_t * inversibleMatrix = NULL;

    /*random sayı olusturmada kullanılacak*/
    srand(time(NULL));

    if (argc != 4) {
      fprintf(stderr, "Usage: %s ticksInMS n mainFifoName \n", argv[0]);
      return 1;
    }

    dimension = atoi(argv[2]);
    if(dimension>10 || dimension<0){
      fprintf(stderr, "Dimension must be between [0-10]\n");
      return 1;
    }

    mkdir("logs",0777);

    /* ana fifoyu olustur*/
    if (mkfifo(argv[3], (S_IRUSR | S_IWUSR | S_IWGRP | S_IWOTH)) == ERROR) {
      perror("Server failed to create a FIFO");
      return 1;
    }

    fdMainFifo = open(argv[3], O_RDONLY | O_NONBLOCK);
    if (fdMainFifo == ERROR) {
      perror("Server failed to open its FIFO");
      return 1;
    }

    /* sinyal handle islemleri*/
    signal(SIGINT, sig_exit_handler);
    sigemptyset(&sigmask);
    sigaddset(&sigmask,SIGINT);
    signalControlFreq=atoi(argv[1]);

    /* sistemin baslangic zamanını belirle */
    if (clock_gettime(CLOCK_REALTIME, & tpstart) == -1) {
      perror("Failed to get ending time");
      return 1;
    }

    pid = getpid();

    printf("Main Server:%d started. Fifo:%s created.\n",pid,argv[3]);
    printf("Waiting for clients...\n");
    parentChilds.number=0;

    /*sinyal gelene kadar beklemede kal*/
    while (!sig_exit_occur) {
      /* clinet yoksa biraz uyu, CPU yu mesgul etme */
      if (read(fdMainFifo, & clientPID, sizeof(clientPID)) <= 0){
        usleep(atoi(argv[1])*MILITOMIKRO);
        continue;
      } /* sırada kimse yoksa bekle*/

      childPID = fork();
      if (childPID == ERROR) {
        perror("fork");
        exit(1); /* ALERT, cok acil cıkması lazım. fork onemli */
      } else if (childPID == 0) {
        pid = getpid();
        printf("Mini Server:%d created.\n", pid);
        childProcCon(clientPID, 2*dimension);
        printf("Mini Server:%d closed.\n", pid);
        exit(1);
      } else {
        /* olusturulan cocugu kaydet */
        parentChilds.child[parentChilds.number]=childPID;
        parentChilds.number++;
      }
    }

    /* kritik bolge, sinyal gelmeden gerekli kapatmalar yapilmalı */
    sigprocmask(SIG_BLOCK,&sigmask,NULL);
    if(sig_exit_occur){
      int i=0;
      for(i=0;i<parentChilds.number;++i){
        kill(parentChilds.child[i],SIGINT);
      }
    }

    mainLog = fopen(MAIN_LOG_FILE,"w");
    if(mainLog==NULL){
      perror("main log create");
      return 1;
    }

    /* clientleri bekle ve gelen cocugun logunu kendinkine ekle */
    while ((childPID = wait(NULL)) != ERROR) {
      printf("Child %d dead\n", (int) childPID);
      bzero(childLogName,sizeof(childLogName));
      sprintf(childLogName,".%d",childPID);
      childLog = fopen(childLogName,"r");
      if(childLog==NULL){
        perror("child log");
        continue;
      }
      copyFile(mainLog,childLog);
      fclose(childLog);
      unlink(childLogName);
    }

    fclose(mainLog);
    unlink(argv[3]); /* main fifoyu sil */
    sigprocmask(SIG_UNBLOCK,&sigmask,NULL);
    return 1;
  } /* End main*/

int copyFile(FILE * to, FILE *from){
  char ch;
  while(fscanf(from,"%c",&ch)!=EOF){
    fprintf(to, "%c",ch);
  }
  return 1;

}

long double getdifInMilli(struct timeval *start, struct timeval *end) {
    return 1000 * (end->tv_sec - start->tv_sec) + (end->tv_usec - start->tv_usec) / 1000.0;
}

int childProcCon(pid_t clientPID, int matrixSize) {
  matrix_t * matrix = NULL;
  double detRes;
  long timedif;
  double timeDifDoub;
  int clientFifoFD;
  char clientFifoName[MINI_BUFFER_SIZE];
  char buffer[BUFFER_SIZE];
  int pid;
  int writedByteSize = 0;
  FILE *childLogfile=NULL;
  char childLogName[MINI_BUFFER_SIZE];
  sigset_t signal_set;
  struct timeval tFirst; /* times */
  struct timeval tSecond;
  /* signal blocking mechanizm*/
  signal(SIGUSR1,sig_req_handler);

  sigemptyset(&signal_set);
  sigaddset(&signal_set, SIGUSR1);

  bzero(clientFifoName,sizeof(MINI_BUFFER_SIZE));
  sprintf(clientFifoName, "%d.fifo", (int) clientPID);
  if (mkfifo(clientFifoName, (S_IRUSR | S_IWUSR | S_IWGRP | S_IWOTH)) == ERROR) {
    perror("MiniServer create FIFO");
    return 1;
  }

  clientFifoFD = open(clientFifoName, O_WRONLY);
  usleep(SLEEPGOOD);

  pid=getpid();
  write(clientFifoFD, &pid, sizeof(pid_t));

  bzero(childLogName,sizeof(childLogName));
  sprintf(childLogName,".%d",pid);
  childLogfile = fopen(childLogName,"w");
  if(childLogName==NULL){
    perror("child log");
    close(clientFifoFD);
    return -1;
  }

  /* handle user matrix request signals */


  while(!sig_exit_occur){
    sigprocmask(SIG_BLOCK,&signal_set,NULL);

    if(sig_req_occur){
      int i,j;
      long double determinantOfM;
      gettimeofday(&tFirst,NULL);
      matrix = createMatrix(matrixSize);
      gettimeofday(&tSecond,NULL);
      long double ldDifTime = getdifInMilli(&tFirst,&tSecond);
      writedByteSize= write(clientFifoFD,matrix,sizeof(matrix_t));

      if(writedByteSize==ERROR){
        perror("Client connection error.\n");
        break;
      }

      determinantOfM = determinant(matrix);
      bzero(buffer,BUFFER_SIZE);
      fprintf(childLogfile, "Sid:%5d Cid:%5d Time:%Lf Determinant:%Lf\n",pid,clientPID,ldDifTime,determinantOfM);
      free(matrix);
    }
    /*printf("%d byte %d writed.\n",writedByteSize,temp);*/
    sig_req_occur=0;

    /* belirtilen sure kadar bekle*/
    usleep(signalControlFreq*MILITOMIKRO);
    sigprocmask(SIG_UNBLOCK,&signal_set,NULL);
  }

    /* kritik bolge, sinyal gelmeden gerekli kapatmalar yapilmalı*/
  sigprocmask(SIG_BLOCK,&sigmask,NULL);
  if(sig_exit_occur){
    kill(getppid(),SIGINT);
    kill(clientPID,SIGINT);
  }

  usleep(SLEEPGOOD);

  fclose(childLogfile);
  close(clientFifoFD);
  unlink(clientFifoName);
  matrix = NULL;
  sigprocmask(SIG_UNBLOCK,&sigmask,NULL);

  return 0;
}

matrix_t * createMatrix(int dimension) {
  int i, j;
  matrix_t * matrix = NULL;
  double determinantResult;
  /* matrixin icini 0 ile doldurarak olustur */
  matrix = (matrix_t * ) calloc(1,sizeof(matrix_t));

  if (NULL == matrix) {
    printf("matrix construction");
    exit(1);
  }
  matrix -> size = dimension;
  do{
    for (i = 0; i < dimension; ++i) {
      for (j = 0; j < dimension; ++j) {
        matrix -> array[i][j] = 1 + rand() % 100;
      }
    }
    determinantResult=determinant(matrix);
  }
  while(determinant==0);
  return matrix;
}

void printMatrix(const matrix_t * matrix) {
  int i = 0, j = 0;
  int size = matrix -> size;
  printf("Matrix: %d*%d\n", size, size);
  for (i = 0; i < size; ++i) {
    for (j = 0; j < size; ++j)
      printf("%6.3f ", matrix -> array[i][j]);
    printf("\n");
  }
}

double determinant(const matrix_t *matrix)
{
    double ratio, det;
    int i, j, k;
    int size;
    matrix_t temp;
    size=matrix->size;

    for(i=0;i<size;++i)
      for(j=0;j<size;++j)
        temp.array[i][j]=matrix->array[i][j];


    for(i = 0; i <size; i++){
        for(j = 0; j < size; j++){
            if(j>i){
                ratio = temp.array[j][i]/ temp.array[i][i];
                for(k = 0; k < size; k++){
                    temp.array[j][k] -= ratio * temp.array[i][k];
                }
            }
        }
    }
    det = 1; /*storage for determinant*/
    for(i = 0; i < size; i++)
        det *= temp.array[i][i];
    //printf("The determinant of matrix is: %.2f\n", det);
    return det;
}
matrix_t* combiningMatrix(matrix_t *oldMatrix1,matrix_t *oldMatrix2,matrix_t *oldMatrix3,matrix_t *oldMatrix4)
{
    int i,j;
    matrix_t* newMatrix = malloc(sizeof(matrix_t));
    int size=(oldMatrix1->size)*2;
    int haflsize=size/2;

    for(i=0;i<size;++i){
        for(j=0;j<size;++j){
            if(i<haflsize && j<haflsize){
                newMatrix->array[i][j] = oldMatrix1->array[i][j];
            }
            else if(i>=haflsize && j<haflsize){
                newMatrix->array[i][j] = oldMatrix2->array[i-haflsize][j];
            }else if(i<haflsize && j>=haflsize){
                newMatrix->array[i][j] = oldMatrix3->array[i][j-haflsize];
            }else{
                newMatrix->array[i][j] = oldMatrix4->array[i-haflsize][j-haflsize];
            }
        }
    }
    newMatrix->size=size;
    return newMatrix;
}
