/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
//思路 先将整个大矩阵分成多个小块 将A的每个小块放到B中沿对角线对称的位置
//分块的思想是针对bxb大小的块 A中按行缓存b个变量  只有第一个元素miss 
//B中最初jj递增的时候块中第一列全部miss 但是之后按列访问的时候每一列的元素都已经进入缓存
//这里导致冲突的原因是B和A中，转置元素所处的位置的set_index相同 由于每个set只有一个line 所以加载A[ii][jj]之后相同位置的缓存被B[jj][ii]覆盖
//s=5 E=1 b=5 setNum=1<<5=32  blocksize=1<<5=32 也就是说每个set可以缓存8个整形变量
char transpose_test_desc[] = "Transpose test";
void transpose_test(int M, int N, int A[N][M], int B[M][N])
{
     int i,j,ii,jj,b;
     switch(M)
     {  
        
        case 32:b=8;break;
        case 64:b=4;break;
        default:b=1;
     };

     for(i=0;i<N;i+=b)
     {
        for(j=0;j<M;j+=b)
        { 
           for(ii=i;ii<i+b;ii++)
              for(jj=j;jj<j+b;jj++) 
              {
                  B[jj][ii]=A[ii][jj];
                  /*
                  if(cnt<20)
                  {
                   printf("A数组地址:%p,B数组地址:%p\n",&A[ii][jj],&B[jj][ii]); 
                  ++cnt;}*/            
              }
        }
     }
     
}

//思路 为了缓解A数组和B数组中set index的冲突 这里先将A中第i行加载连续b个元素 再将B中第j列加载连续b个元素 这样缓解了在缓存中B中元素覆盖A中元素的情况
char transpose_test2_desc[] = "Transpose test2";
void transpose_test2(int M, int N, int A[N][M], int B[M][N])
{
     int i,j,n0,n1,n2,n3,n4,n5,n6,n7;
     if(M==32)  //M=32 N=32 miss=287  300以下满分
      for(j=0;j<M;j+=8)
      {
        for(i=0;i<N;i++)
        { 
           n0=A[i][j];
           n1=A[i][j+1];
           n2=A[i][j+2];
           n3=A[i][j+3];
           n4=A[i][j+4];
           n5=A[i][j+5];
           n6=A[i][j+6];
           n7=A[i][j+7];
           
           B[j][i]=n0;
           B[j+1][i]=n1;
           B[j+2][i]=n2;
           B[j+3][i]=n3;
           B[j+4][i]=n4;
           B[j+5][i]=n5;
           B[j+6][i]=n6;
           B[j+7][i]=n7;
        }
      }
    else if(M==64)  //M=64 N=64 miss=1651 1300以下满分
      for(j=0;j<M;j+=4)
      {
        for(i=0;i<N;i++)
        { 
           n0=A[i][j];
           n1=A[i][j+1];
           n2=A[i][j+2];
           n3=A[i][j+3];
           //n4=A[i][j+4];
           //n5=A[i][j+5];
           //n6=A[i][j+6];
           //n7=A[i][j+7];
           
           B[j][i]=n0;
           B[j+1][i]=n1;
           B[j+2][i]=n2;
           B[j+3][i]=n3;
           
           //B[j+4][i]=n4;
           //B[j+5][i]=n5;
           //B[j+6][i]=n6;
           //B[j+7][i]=n7;
        }
      } 
   else if(M==61)  //M=61 N=67 miss=2225 2000以下满分
      for(j=0;j<M;j+=4)
      {
        for(i=0;i<N;i++)
        { 
           n0=A[i][j];
           if(j<M)
           {n1=A[i][j+1];
            n2=A[i][j+2];
            n3=A[i][j+3];}
           //n4=A[i][j+4];
           //n5=A[i][j+5];
           //n6=A[i][j+6];
           //n7=A[i][j+7];
           
           B[j][i]=n0;
           if(j<M){
            B[j+1][i]=n1;
            B[j+2][i]=n2;
            B[j+3][i]=n3;}
           
           //B[j+4][i]=n4;
           //B[j+5][i]=n5;
           //B[j+6][i]=n6;
           //B[j+7][i]=n7;
        }
      } 
}

//最终提交版本
//B中对角线上的元素由于在按列扫描的时候已经将 
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
     int i,j,ii,jj,n0,n1,n2,n3,n4,n5,n6,n7;
     if(M==32)  //M=32 N=32 miss=287  300以下满分
     /*
     对于在对角线上的块，A中每读一行，会有一次miss，也就是miss次数是读取操作的1/8，对于B数组的话，第一次读取这行会产生一次miss，之后对于第i行，只有A中读到第i行的时候(A取第i行和 B取第i列冲突)，会被移除出Cache，然后存的时候会产生一次miss。可以粗略计算为miss次数是读取次数的1/4。
     对于不在对角线上的块，做转置的时候，A还是1/8的miss率，B的每行在Cache中和A的行不冲突 ，所以也是1/8的miss率，我们计算下最后大概多少次miss呢？大概是
     4x64x(1/8+1/4)+12x64x2x1/8=288  4个在对角线的块 12个不在 每个块有64个元素
     */
      for(j=0;j<M;j+=8)
      {
        for(i=0;i<N;i++)
        { 
           n0=A[i][j];
           n1=A[i][j+1];
           n2=A[i][j+2];
           n3=A[i][j+3];
           n4=A[i][j+4];
           n5=A[i][j+5];
           n6=A[i][j+6];
           n7=A[i][j+7];
           
           B[j][i]=n0;
           B[j+1][i]=n1;
           B[j+2][i]=n2;
           B[j+3][i]=n3;
           B[j+4][i]=n4;
           B[j+5][i]=n5;
           B[j+6][i]=n6;
           B[j+7][i]=n7;
        }
      }
    /*
首先考虑Cache中只能放4行A中的行，如果再用8×8的块，前面4行可以填入，后面4行会在Cache中发生冲突，导致miss次数增加。
如果只用4×4的块呢？那么每次Cache中放入8个int，我们却只用4个，浪费严重，我用这个方法最少也只能做到1677次miss。
有一种很巧妙的方法，就是还用8×8的块来做，题目说A数组不能变换，但是说B数组可以任意操作啊。我们必须要一步到位嘛？可否考虑先把数字移动到B中，然后在B中自己做变化。
考虑用同样的miss次数，把更多的数据移动到B中，但是不一定是正确的位置，然后再用同样的miss次数，把A中部分数据移动到B中时，完成把B中前面位置错误数据的纠正。
    */
    else if(M==64){  //M=64 N=64 miss=1651 1300以下满分
      for(i=0;i<N;i+=8)
      {
        for(j=0;j<M;j+=8)
        {
           /*
           1.先考虑把A的上半部分存入到B，但是为了考虑Cache不冲突，所以把右上角的4×4的区域也存在B的右上角。对于在对角线上的块，A的miss率是1/8，B的左上角部分miss率是   1/2。对于不在对角线上的块，A的miss率还是1/8，B左上角部分的miss率为1/4.
           */
           for(ii=i;ii<i+4;ii++)
           {
              n0=A[ii][j];
              n1=A[ii][j+1];
              n2=A[ii][j+2];
              n3=A[ii][j+3];
              n4=A[ii][j+4];
              n5=A[ii][j+5];
              n6=A[ii][j+6];
              n7=A[ii][j+7];
           
              B[j][ii]=n0;
              B[j+1][ii]=n1;
              B[j+2][ii]=n2;
              B[j+3][ii]=n3;
           
              B[j][ii+4]=n4;
              B[j+1][ii+4]=n5;
              B[j+2][ii+4]=n6;
              B[j+3][ii+4]=n7;                 
           } 
           /*
           接下来这步是减少miss率的关键，把A左下角的一列4个数据读出，B右上角的一行4个数据读出，都用int变量暂存，然后把前四个填入B右上角行中，后四个填入B的左下角行中。 因为从B右上角读取的时候，把块放入了Cache，然后从A往B中填的时候，就不会出现miss操作。
           */
           for(jj=j;jj<j+4;jj++)
           {
              n0=A[i+4][jj];
              n1=A[i+5][jj];
              n2=A[i+6][jj];
              n3=A[i+7][jj];
              n4=B[jj][i+4];
              n5=B[jj][i+5];
              n6=B[jj][i+6];
              n7=B[jj][i+7];

              B[jj][i+4]=n0;
              B[jj][i+5]=n1;
              B[jj][i+6]=n2;
              B[jj][i+7]=n3; 
           
              B[jj+4][i]=n4;
              B[jj+4][i+1]=n5;
              B[jj+4][i+2]=n6;
              B[jj+4][i+3]=n7;

              
               
           } 
           /*
           3. 最后一步就是把A的右下角填入B的右下角，对于在对角线上的块，A的miss率为1/4，B的miss率为1/2.不在对角线上的块，A，B的miss率都为0.
           */  
           for(ii=i+4;ii<i+8;ii++)
           {
              
              n0=A[ii][j+4];
              n1=A[ii][j+5];
              n2=A[ii][j+6];
              n3=A[ii][j+7];

              B[j+4][ii]=n0;
              B[j+5][ii]=n1;
              B[j+6][ii]=n2;
              B[j+7][ii]=n3;
           
           } 

        }
      } 
   }
   else if(M==61)  //M=61 N=67 如果单纯使用16x16分块miss数目略高于2000 加入局部变量之后可以使得miss=1810
     for(j=0;j<M;j+=16)
      {
        for(i=0;i<N;i++)
        { 
           n0=A[i][j];
           if(j<M)
           {
           n1=A[i][j+1];
           n2=A[i][j+2];
           n3=A[i][j+3];
           n4=A[i][j+4];
           n5=A[i][j+5];
           n6=A[i][j+6];
           n7=A[i][j+7];}
           
           B[j][i]=n0;
           if(j<M){
           B[j+1][i]=n1;
           B[j+2][i]=n2;
           B[j+3][i]=n3;
           
           B[j+4][i]=n4;
           B[j+5][i]=n5;
           B[j+6][i]=n6;
           B[j+7][i]=n7;
           
           n0=A[i][j+8];
           n1=A[i][j+9];
           n2=A[i][j+10];
           n3=A[i][j+11];
           n4=A[i][j+12];
           n5=A[i][j+13];
           n6=A[i][j+14];
           n7=A[i][j+15];

           B[j+8][i]=n0;
           B[j+9][i]=n1;
           B[j+10][i]=n2;
           B[j+11][i]=n3;
           B[j+12][i]=n4;
           B[j+13][i]=n5;
           B[j+14][i]=n6;
           B[j+15][i]=n7;}
        }
      } 

            
}

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    

}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc); 

    /* Register your test function */
    registerTransFunction(transpose_test, transpose_test_desc); 
    registerTransFunction(transpose_test2, transpose_test2_desc); 

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc); 

}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}

