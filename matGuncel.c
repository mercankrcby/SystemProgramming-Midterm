/*
 * File:   main.c
 * Author: macboookair
 *
 * Created on 01 Nisan 2017 Cumartesi, 20:06
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#define MAX_DIMENSION 20

typedef struct {
  double array[MAX_DIMENSION][MAX_DIMENSION];
  int size;
}
matrix_t;

void createMatrix(int dimension,matrix_t *matrixResult);
double determinant(matrix_t *matrix);
void shiftAndInverseMatrix(matrix_t *matrix);
void cofactor(matrix_t *matrix);
void transpose(matrix_t *matrix, const matrix_t *fac);
void printMatrix(const matrix_t * matrix);
matrix_t* combiningMatrix(matrix_t *oldMatrix1,matrix_t *oldMatrix2,matrix_t *oldMatrix3,matrix_t *oldMatrix4);
int matrixControl(matrix_t controllingMatrix);
void convolution(matrix_t *out,const matrix_t *in, const matrix_t *kernel);

int main(int argc, char** argv) {
    matrix_t in;
    matrix_t kernel;
    matrix_t out;
    //int dimension=atoi(argv[1]);
    int i,j;
    double resultdeterminantofArray=0.0;
    srand(time(NULL));

    matrix_t createdMatrix1;
    matrix_t createdMatrix2;
    matrix_t createdMatrix3;
    matrix_t createdMatrix4;
    matrix_t *resultMatrix;

    /*
    createMatrix(atoi(argv[1]),&createdMatrix1);
    printMatrix(&createdMatrix1);
    printf("\n");
    createMatrix(atoi(argv[1]),&createdMatrix2);
    printMatrix(&createdMatrix2);
    printf("\n");
    createMatrix(atoi(argv[1]),&createdMatrix3);
    printMatrix(&createdMatrix3);
    printf("\n");
    createMatrix(atoi(argv[1]),&createdMatrix4);
    printMatrix(&createdMatrix4);
    printf("\n");
    */
    /*resultMatrix=combiningMatrix(&createdMatrix1,&createdMatrix2,&createdMatrix3,&createdMatrix4);
    printMatrix(resultMatrix);
    */
    //free(resultMatrix);
    /*
    kernel.array[0][0]=0;
    kernel.array[0][1]=0;
    kernel.array[0][2]=0;
    kernel.array[1][0]=0;
    kernel.array[1][1]=1;
    kernel.array[1][2]=0;
    kernel.array[2][0]=0;
    kernel.array[2][1]=0;
    kernel.array[2][2]=0;
    kernel.size=3;

    in.size=4;

    for(i=0;i<4;++i){
        for(j=0;j<4;++j)
        {
            in.array[i][j]=(i*2)+(j*3)+1;
        }
    }
    in.array[0][0]=1965;
    in.array[0][1]=2004;
    in.array[3][3]=23;
    for(i=0;i<4;++i){
        for(j=0;j<4;++j)
        {
            out.array[i][j]=0;
        }
    }
*/
    createMatrix(8,resultMatrix);
    //fprintf(stderr,"\nResult Matrix Size:%d\n",resultMatrix->size);
    //printMatrix(resultMatrix);
    /*convolution(&out,&in,&kernel);
    printf("Convolution:\n");
    printMatrix(&out);

    printf("Shift and Inverse Matrix\n");*/
    /*resultdeterminantofArray=determinant(resultMatrix);
    printf("resultdeterminantofArray:%f\n",resultdeterminantofArray);*/

    //shiftAndInverseMatrix(&out);

    return (EXIT_SUCCESS);
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
    printf("---------------print matrix-temp1--------------------------\n");
    printMatrix(&temp1);
    d=determinant(&temp1);
    printf("determinant=%f\n",d);
    if (d == 0)
        printf("\nInverse of Entered Matrix is not possible\n");
    else
        cofactor(&temp1);
    printf("---------------print matrix-temp1--------------------------\n");
    /*---------------------------PART2 PLACEMENT------------------------------*/
    for(i=0;i<mid;++i){
        for(j=0;j<mid;++j){
            temp2.array[i][j]=(matrix->array[mid+i][j]);
        }
    }
    temp2.size=mid;
    printf("---------------print matrix-temp2--------------------------\n");
    printMatrix(&temp2);
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
    printf("---------------print matrix-temp3--------------------------\n");
    printMatrix(&temp3);
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
    printf("---------------print matrix-temp4--------------------------\n");
    printMatrix(&temp4);
    d = determinant(&temp4);
    if (d == 0)
        printf("\nInverse of Entered Matrix is not possible\n");
    else
        cofactor(&temp4);
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
  printMatrix(matrix);
}
/*Finding transpose of matrix*/

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


void createMatrix(int dimension,matrix_t *matrixResult) {
  int i, j;
  matrix_t * matrix = NULL;
  double determinantResult;
  // matrixin icini 0 ile doldurarak olustur
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
        printf("matrix -> array[%d][%d]:%f\n",i,j,matrix -> array[i][j]);
      }
    }
    determinantResult=determinant(matrix);
  }
  while(determinantResult==0);
  printf("Determinant Result:%f\n",determinantResult);
  matrixResult=matrix;
  matrixResult->size=dimension;
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
    return isnan(det) ? 1 : det;
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
