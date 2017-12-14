#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <signal.h>
#include <math.h>
#include <sys/wait.h>

#define BUFFER_SIZE 100
#define MAX_FIFO_SIZE 400
#define ERROR -1
#define SLEEPGOOD 100000
#define MILITOMIKRO 1000
#define MAX_DIMENSION 20

#define SHOW_RES_FIFO "show_res.ff"
/* ************************************/

static volatile int sig_exit_occur = 0;

void sig_exit_handler(int signo) {
  sig_exit_occur = 1;
  fprintf(stderr,"Signal exit handled\n");
}

typedef struct {
  double array[MAX_DIMENSION][MAX_DIMENSION];
  int size;
}
matrix_t;

void printMatrix(const matrix_t * matrix);
void shiftAndInverseMatrix(matrix_t *matrix);
void cofactor(matrix_t *matrix);
void transpose(matrix_t *matrix, const matrix_t *fac);
matrix_t* combiningMatrix(matrix_t *oldMatrix1,matrix_t *oldMatrix2,matrix_t *oldMatrix3,matrix_t *oldMatrix4);
void convolution(matrix_t *out,const matrix_t *in, const matrix_t *kernel);
void fileOperations(matrix_t *shiftedMatrix,matrix_t *convolutionMatrix,matrix_t *originalMatrix);

// GLOBAL VALUES
matrix_t kernelMatrix;


int main(int argc, char *argv[]) {
  time_t currentTime;
  char buffer[BUFFER_SIZE];
  int mainFifoFd;
  int byteCount;
  int byteCountForClient;
  int bufferLength;
  int fifoFD;
  char fifoName[BUFFER_SIZE];
  int i, j;
  pid_t pid;
  pid_t serverPID;
  int matrixSize;
  sigset_t sigmask;
  pid_t childPID;
  int pipes[2][2];
  int showResFD;
  double convRes;
  double shiftRes;
  char showResMsg[BUFFER_SIZE];

  signal(SIGINT, sig_exit_handler);
  sigemptyset(&sigmask);
  sigaddset(&sigmask, SIGINT);


  if (argc != 2) {
    fprintf(stderr, "Usage: %s fifo_name\n", argv[0]);
    return 1;
  }

  mainFifoFd = open(argv[1], O_WRONLY);
  if (mainFifoFd == -1) {
    fprintf(stderr,"Please check time server\n");
    perror("Client failed to open connection FIFO");
    return 1;
  }

  pid = getpid();
  printf("Client pid: %d\n",pid);
  if (write(mainFifoFd, &pid, sizeof(pid_t)) < 0) {
    perror("Client failed to write to FIFO");
    return 1;
  } else

  close(mainFifoFd);
  sprintf(fifoName, "%d.fifo", (int)pid);

  for(i=0;i<3;++i)
    for(j=0;j<3;++j)
      if(i==1 && j==1)
        kernelMatrix.array[i][j]=1;
      else kernelMatrix.array[i][j]=0;


  /*usleep ile serverin fifoyu acmasi saglandi*/
  while (!sig_exit_occur && (fifoFD = open(fifoName, O_RDONLY)) == ERROR) {
    usleep(SLEEPGOOD);
  }

  printf("Client connected fifo:%s\n",fifoName);

  usleep(SLEEPGOOD);
  memset(buffer, 0, BUFFER_SIZE);
  read(fifoFD, &serverPID, sizeof(pid_t));
  printf("Client connected to Server:%d\n", serverPID);

  printf("Waiting for show result...\n");

  while(!sig_exit_occur && (showResFD = open(SHOW_RES_FIFO,O_WRONLY))==ERROR){
    usleep(SLEEPGOOD);
  }

  bzero(showResMsg,BUFFER_SIZE);

  while(!sig_exit_occur){
    int temp;
    int size=0;
    matrix_t matrix;
    int whoDead;

    bzero(&matrix,sizeof(matrix));

    kill(serverPID,SIGUSR1);
    /* wait until input come or an interrupt occur*/
    sigprocmask(SIG_BLOCK, &sigmask, NULL);
    while(!sig_exit_occur && (size = read(fifoFD,&matrix,sizeof(matrix_t)))<1){
      if(errno>0){
        sig_exit_occur=1;
      }
      usleep(SLEEPGOOD);
    }

    for(i=0;i<2;++i){
      pipe(pipes[i]);
      if((childPID=fork())==ERROR){
        raise(SIGINT);
      }else if(childPID==0){
        close(pipes[i][0]);
        if(i==0){ // calculate shifted matrix
          double val=3;
          shiftAndInverseMatrix(&matrix);
          write(pipes[i][1],&val,sizeof(double));

        }else if(i==1){ // calculate convolution matrix
          matrix_t conv;
          double val=2;
          convolution(&conv,&matrix,&kernelMatrix);
          write(pipes[i][1],&val,sizeof(double));
        }
        close(pipes[i][1]);
        exit(i); // kim oldugunu belirtmek icin i yi dondur
      }else{
        close(pipes[i][1]);
      }
    }

    while((childPID = wait(&whoDead))!=ERROR){
      double val;
      whoDead=WEXITSTATUS(whoDead);
      read(pipes[whoDead][0],&val,sizeof(double));
      if(whoDead==0)
        shiftRes=val;
      else if(whoDead==1)
        convRes=val;
      
      // TODO:
      // Bu okudugu degerleri sadece showRes e yollaması kaldı
      // printf("ChildDead%d, whoDead:%d val:%f\n",childPID,whoDead,val);
    }

    sprintf(showResMsg,"PID:%d %f %f",pid,shiftRes,convRes);
    write(showResFD,showResMsg,BUFFER_SIZE);

    bzero(&showResMsg,BUFFER_SIZE);
    sigprocmask(SIG_UNBLOCK, &sigmask,NULL);
  }

  kill(serverPID,SIGINT);
  close(fifoFD);
  close(showResFD);

  printf("Program ended.\n");
  return 0;
} /* End main*/

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
double determinant(matrix_t *matrix)
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
    det = 1;
    for(i = 0; i < size; i++)
        det *= temp.array[i][i];
    return isnan(det) ? 0 : det;
}
void convolution(matrix_t *out,const matrix_t *in,const matrix_t *kernel)
{
    int kCenter;
    int kSize;
    int inSize;
    int i,j,m,n;
    int mm,nn,ii,jj;

    kCenter = kernel->size / 2;
    inSize=in->size;
    kSize = kernel->size;

    out->size=4;
    for(i=0; i < inSize; ++i){
        for(j=0; j < inSize; ++j){
            for(m=0; m < kSize; ++m){
                mm = kSize - 1 - m;
                for(n=0; n < kSize; ++n){
                    nn = kSize - 1 - n;
                    ii = i + (m - kCenter);
                    jj = j + (n - kCenter);
                    if( ii >= 0 && ii < inSize && jj >= 0 && jj < inSize ){
                        out->array[i][j] += in->array[ii][jj] * kernel->array[mm][nn];
                    }
                }
            }
        }
    }
}
void transpose(matrix_t *matrix, const matrix_t *fac){
  int i, j,d;
  int size = fac->size;
  matrix_t b,inverse;

  for (i = 0;i < size; i++)
    {
     for (j = 0;j < size; j++)
       {
         b.array[i][j]=fac->array[j][i];
        }
    }
  d = determinant(matrix);
  for (i = 0;i < size; i++)
    {
     for (j = 0;j < size; j++)
       {
        matrix->array[i][j]= b.array[i][j] / d;
        }
    }
}
void cofactor(matrix_t *matrix)
{

 matrix_t b, fac;
 int p, q, m, n, i, j;
 int size=matrix->size;
 double size1=(double)matrix->size;
 for (q = 0;q < size; q++)
 {
   for (p = 0;p < size; p++)
    {
     m = 0;
     n = 0;
     for (i = 0;i < size; i++)
     {
       for (j = 0;j < size; j++)
        {
          if (i != q && j != p)
          {
            b.array[m][n] = matrix->array[i][j];
            if (n < (size - 2))
             n++;
            else
             {
               n = 0;
               m++;
               }
            }
        }
      }
      b.size=matrix->size-1;
      size1=(double)matrix->size-1;
      fac.array[q][p] = pow(-1, q + p) * determinant(&b);
    }
  }
  fac.size=size;
  transpose(matrix,&fac);
  //printMatrix(matrix);
}
void shiftAndInverseMatrix(matrix_t *matrix){
    int i,j;
    double d;
    matrix_t temp1,temp2,temp3,temp4;
    int mid=matrix->size/2;

    /*---------------------------PART1 PLACEMENT------------------------------*/
    for(i=0;i<mid;++i){
        for(j=0;j<mid;++j){
            temp1.array[i][j]=(matrix->array[i][j]);
        }
    }
    temp1.size=mid;
    //printf("---------------print matrix-temp1--------------------------\n");
    //printMatrix(&temp1);
    d=determinant(&temp1);
    //printf("determinant=%f\n",d);
    if (d == 0)
        printf("\nInverse of Entered Matrix is not possible\n");
    else
        cofactor(&temp1);
    //printf("---------------print matrix-temp1--------------------------\n");
    /*---------------------------PART2 PLACEMENT------------------------------*/
    for(i=0;i<mid;++i){
        for(j=0;j<mid;++j){
            temp2.array[i][j]=(matrix->array[mid+i][j]);
        }
    }
    temp2.size=mid;
    //printf("---------------print matrix-temp2--------------------------\n");
    //printMatrix(&temp2);
    d = determinant(&temp2);
    if (d == 0)
        printf("\nInverse of Entered Matrix is not possible\n");
    else
        cofactor(&temp2);


    /*---------------------------PART3 PLACEMENT------------------------------*/
    for(i=0;i<mid;++i){
        for(j=0;j<mid;++j){
            temp3.array[i][j]=matrix->array[i][j+mid];
        }
    }
    temp3.size=mid;
    //printf("---------------print matrix-temp3--------------------------\n");
    //printMatrix(&temp3);
    d = determinant(&temp3);
    if (d == 0)
        printf("\nInverse of Entered Matrix is not possible\n");
    else
        cofactor(&temp3);
    /*---------------------------PART4 PLACEMENT------------------------------*/
    for(i=0;i<mid;++i){
        for(j=0;j<mid;++j){
            temp4.array[i][j]=matrix->array[i+mid][j+mid];
        }
    }
    temp4.size=mid;
    //printf("---------------print matrix-temp4--------------------------\n");
    //printMatrix(&temp4);
    d = determinant(&temp4);
    if (d == 0)
        printf("\nInverse of Entered Matrix is not possible\n");
    else
        cofactor(&temp4);
}
void fileOperations(matrix_t *shiftedMatrix,matrix_t *convolutionMatrix,matrix_t *originalMatrix)
{

    int i,j;
    char buffer[BUFFER_SIZE];
    char buffer1[BUFFER_SIZE];
    int size= shiftedMatrix->size;
    /*Dosya ismi seeWhat-clientPID seklinde olabilir*/
    int filedesc = open("testfile.txt", O_WRONLY | O_APPEND);
    write(filedesc,"Shifted Matrix\n",15);
    write(filedesc,"[",1);
    /*Shifted Matrix 'in matlab formatında dosyaya basilmasi*/
    for(i=0;i<size;++i)
    {
        for(j=0;j<size;++j)
        {
            sprintf(buffer,"%f ",shiftedMatrix->array[i][j]);
            strcat(buffer1,buffer);
        }
        strcat(buffer1,"\n");
        printf("Buffer1:%s\n",buffer1);
        write(filedesc,buffer1,strlen(buffer1));
        write(filedesc,";",1);
        memset(buffer,0,strlen(buffer));
        memset(buffer1,0,strlen(buffer1));
        printf("buffer Delete:%s\n",buffer);
        printf("buffer1 Delete:%s\n",buffer1);
    }
    write(filedesc,"]",1);
    /*Convolution Matrix'in dosyaya MATLAB formatinda basilmasi*/
    write(filedesc,"\nConvolution Matrix\n",20);
    write(filedesc,"[",1);
    for(i=0;i<size;++i)
    {
        for(j=0;j<size;++j)
        {
            sprintf(buffer,"%f ",convolutionMatrix->array[i][j]);
            strcat(buffer1,buffer);
        }
        strcat(buffer1,"\n");
        printf("Buffer1:%s\n",buffer1);
        write(filedesc,buffer1,strlen(buffer1));
        write(filedesc,";",1);
        memset(buffer,0,strlen(buffer));
        memset(buffer1,0,strlen(buffer1));
        printf("buffer Delete:%s\n",buffer);
        printf("buffer1 Delete:%s\n",buffer1);
    }
    write(filedesc,"]",1);
    write(filedesc,"\nOriginal Matrix\n",17);
    /*Original Matrix'in matlab formatinda dosyaya basilmasi*/
    write(filedesc,"[",1);
    for(i=0;i<size;++i)
    {
        for(j=0;j<size;++j)
        {
            sprintf(buffer,"%f ",originalMatrix->array[i][j]);
            strcat(buffer1,buffer);
        }
        strcat(buffer1,"\n");
        printf("Buffer1:%s\n",buffer1);
        write(filedesc,buffer1,strlen(buffer1));
        write(filedesc,";",1);
        memset(buffer,0,strlen(buffer));
        memset(buffer1,0,strlen(buffer1));
        printf("buffer Delete:%s\n",buffer);
        printf("buffer1 Delete:%s\n",buffer1);
    }
    write(filedesc,"]",1);
    close(filedesc);
}
