/* 
 * CS:APP Data Lab 
 * 
 * <Please put your name and userid here>
 * 
 * bits.c - Source file with your solutions to the Lab.
 *          This is the file you will hand in to your instructor.
 *
 * WARNING: Do not include the <stdio.h> header; it confuses the dlc
 * compiler. You can still use printf for debugging without including
 * <stdio.h>, although you might get a compiler warning. In general,
 * it's not good practice to ignore compiler warnings, but in this
 * case it's OK.  
 */

#if 0
/*
 * Instructions to Students:
 *
 * STEP 1: Read the following instructions carefully.
 */

You will provide your solution to the Data Lab by
editing the collection of functions in this source file.

INTEGER CODING RULES:
 
  Replace the "return" statement in each function with one
  or more lines of C code that implements the function. Your code 
  must conform to the following style:
 
  int Funct(arg1, arg2, ...) {
      /* brief description of how your implementation works */
      int var1 = Expr1;
      ...
      int varM = ExprM;

      varJ = ExprJ;
      ...
      varN = ExprN;
      return ExprR;
  }

  Each "Expr" is an expression using ONLY the following:
  1. Integer constants 0 through 255 (0xFF), inclusive. You are
      not allowed to use big constants such as 0xffffffff.
  2. Function arguments and local variables (no global variables).
  3. Unary integer operations ! ~ Unary--一元
  4. Binary integer operations & ^ | + << >>
    
  Some of the problems restrict the set of allowed operators even further.
  Each "Expr" may consist of multiple operators. You are not restricted to
  one operator per line.

  You are expressly forbidden to:
  1. Use any control constructs such as if, do, while, for, switch, etc.
  2. Define or use any macros.  macros--宏
  3. Define any additional functions in this file.
  4. Call any functions.
  5. Use any other operations, such as &&, ||, -, or ?:
  6. Use any form of casting.
  7. Use any data type other than int.  This implies that you
     cannot use arrays, structs, or unions.

 
  You may assume that your machine:
  1. Uses 2s complement, 32-bit representations of integers.
  2. Performs right shifts arithmetically.  算数右移
  3. Has unpredictable behavior when shifting if the shift amount
     is less than 0 or greater than 31.


EXAMPLES OF ACCEPTABLE CODING STYLE:
  /*
   * pow2plus1 - returns 2^x + 1, where 0 <= x <= 31
   */
  int pow2plus1(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     return (1 << x) + 1;
  }

  /*
   * pow2plus4 - returns 2^x + 4, where 0 <= x <= 31
   */
  int pow2plus4(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     int result = (1 << x);
     result += 4;
     return result;
  }

FLOATING POINT CODING RULES

For the problems that require you to implement floating-point operations,
the coding rules are less strict.  You are allowed to use looping and
conditional control.  You are allowed to use both ints and unsigneds.
You can use arbitrary integer and unsigned constants. You can use any arithmetic,
logical, or comparison operations on int or unsigned data.

You are expressly forbidden to:
  1. Define or use any macros.
  2. Define any additional functions in this file.
  3. Call any functions.
  4. Use any form of casting.
  5. Use any data type other than int or unsigned.  This means that you
     cannot use arrays, structs, or unions.
  6. Use any floating point data types, operations, or constants.


NOTES:
  1. Use the dlc (data lab checker) compiler (described in the handout) to 
     check the legality of your solutions.
  2. Each function has a maximum number of operations (integer, logical,
     or comparison) that you are allowed to use for your implementation
     of the function.  The max operator count is checked by dlc.
     Note that assignment ('=') is not counted; you may use as many of
     these as you want without penalty.
  3. Use the btest test harness to check your functions for correctness.
  4. Use the BDD checker to formally verify your functions
  5. The maximum number of ops for each function is given in the
     header comment for each function. If there are any inconsistencies 
     between the maximum ops in the writeup and in this file, consider
     this file the authoritative source.

/*
 * STEP 2: Modify the following functions according the coding rules.
 * 
 *   IMPORTANT. TO AVOID GRADING SURPRISES:
 *   1. Use the dlc compiler to check that your solutions conform
 *      to the coding rules.
 *   2. Use the BDD checker to formally verify that your solutions produce 
 *      the correct answers.
 */


#endif
# include <stdio.h>
//1
/* 
 * bitXor - x^y using only ~ and & 
 *   Example: bitXor(4, 5) = 1
 *   Legal ops: ~ &
 *   Max ops: 14
 *   Rating: 1
 */
int bitXor(int x, int y) {
  //两个bit相异或有4中可能
  //  1  0 1 0  first bit     0  1 0 1  ~first bit
  //  0  1 1 0  second bit    1  0 0 1  ~second bit
  //&=0  0 1 0              &=0  0 0 1
  //~=1  1 0 1              ~=1  1 1 0
  //1101b&1110b=1100b=1010b^0110b
   //printf("%#x\n",1);
  return (~(x&y))&(~((~x)&(~y)));
}
/* 
 * tmin - return minimum two's complement integer 
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 4
 *   Rating: 1
 */
int tmin(void) {
  //0x8000=Tmin
  return 1<<31;

}
//2
/*
 * isTmax - returns 1 if x is the maximum, two's complement number,
 *     and 0 otherwise 
 *   Legal ops: ! ~ & ^ | +
 *   Max ops: 10
 *   Rating: 1
 */
int isTmax(int x) {
  //Tmax+1=Tmin  这种方法不知道为什么不行
  //~Tmax=Tmin
  //Tmin^(~x)=0 if x=Tmax 
  //Tmin^(~x)!=0 if x!=Tmax 
  //Tmax=0x7fff
  //!(0)=1  !(!0)=0
  //return !((1<<31)^(x+1));
  return !((1<<31)^(~x));
}
/* 
 * allOddBits - return 1 if all odd-numbered bits in word set to 1
 *   where bits are numbered from 0 (least significant) to 31 (most significant)
 *   Examples allOddBits(0xFFFFFFFD) = 0, allOddBits(0xAAAAAAAA) = 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 2
 */
int allOddBits(int x) {
  //思路1 
  //x是偶数位全为0 x>>1是奇数位全为0
  //x|(x>>1)=0xffff=-1  if x是偶数位全为0
  //~(0xffff)=0  if x是偶数位全为0
  //!(0)=1  !(!0)=0
  //这里存在的边界条件是 x=0x0d=1101b  x>>1=0110  x|x>>1=1111 错误
  //构造一个偶数为全为1 奇数位全为1的数 0xaaaaaaa=0xaa | (0xaa<<8) | (0xaa<<16) | (0xaa<<24)  
  //x&0xaaaaaaa=1000 将奇数位置0 偶数位保留原来的数 
  // 1000>>1 | 1101=1101 !=1111 所以判断不是偶数位为1 这样奇数位移位后到偶数位为0不会改变 |的结果
   //return !(~(x|((x&( 0xaa | (0xaa<<8) | (0xaa<<16) | (0xaa<<24) ))>>1)));
  

   /*思路2 :采用掩码方式解决。首先要构造掩码，使用移位运算符构造出奇数位全1的数 mask ，然后获取输入 x 值的奇数位，其他位清零（mask&x），然后与 mask 进行异或操作，若相同则最终结果为0，然后返回其值的逻辑非。*/
   int mask=0xAA+(0xAA<<8);
   mask=mask+(mask<<16);
   return !((mask&x)^mask);
}
/* 
 * negate - return -x 
 *   Example: negate(1) = -1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 5
 *   Rating: 2
 */

int negate(int x) {
  //取反+1就是求负数
  return ~x+1;
}
//3
/* 
 * isAsciiDigit - return 1 if 0x30 <= x <= 0x39 (ASCII codes for characters '0' to '9')
 *   Example: isAsciiDigit(0x35) = 1.
 *            isAsciiDigit(0x3a) = 0.
 *            isAsciiDigit(0x05) = 0.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 15
 *   Rating: 3
 */
int isAsciiDigit(int x) {
  //注意这里右移是算数右移 高位补符号位
  //首先排除符号位为 1的情况 !(x>>31&1)
  //x只能是0x3n  0<=n<=9   (x>>4)^0x03排除 x高位不为1的情况
  //x=0xff32  x>>4^0x03=0x0ff0!=0 
  //判断n的情况  n由4位组成  0000-1001 因此 最高位为0都是ascii码
  //x&0x08 取出n的最高位  !(x&0x08) x最高位为1 结果0  x最高位为0 结果1
  //排除最高位为0的情况 只剩下 1000和 1001两种情况
  //由于排除最高位为0的情况 因此或这得两种情况无需判断最高位是否为1
  //x&0x07 取出x最低3位 x&0x07=0 最低3位全为0 !(x&0x07)=1
  //x&(0x07>>1)=0 最低3位为001 !(x&(0x07>>1))=1
  //return (!((x>>4&0x03)^0x03)) &( (!(x&0x08)) | (!(x&0x07)) |(!(x&0x07>>1)) );
  //return (!(x>>4+(~0x03+1))) &( (!(x&0x08)) | (!(x&0x07)) |(!(x&0x07>>1)) ); 
  // ./btest -f isAsciiDigit -1 0x39  调试函数 -1是第一个参数 -2是第二个参数 中间可以printf 如果只用 ./btest printf输出不正常
  //思路1
  //return (!(x>>31&1)) & (!(x>>4^0x03)) &( (!(x&0x08)) | (!(x&0x07)) |(!((x&0x07)>>1)) ); 
  
  /*
  通过位级运算计算 x 是否在 0x30 - 0x39 范围内就是这个题的解决方案。那如何用位级运算来操作呢？
  我们可以使用两个数，一个数是加上比0x39大的数后符号由正变负，另一个数是加上比0x30小的值时是负数。
  这两个数是代码中初始化的 upperBound 和 lowerBound，然后加法之后获取其符号位判断即可。
  x-0x30>=0
  0x39-x>=0
  x-0x30 和 0x39-x符号位同时为0 则返回1
  */
  int upperbound=0x39;
  int lowerbound=0x30;
  int sign1=(upperbound+1+~x)>>31;
  int sign2=(~lowerbound+1+x)>>31;
  return !((sign1&1)|(sign2&1));
}
/* 
 * conditional - same as x ? y : z 
 *   Example: conditional(2,4,5) = 4
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 16
 *   Rating: 3
 */
int conditional(int x, int y, int z) {
  //!x=0 if x!=0  ~(!x)+1=0--取反加一变为负数  0&z=0  
  //!x=0 if x!=0  !!x=1  ~(!!x)+1=0xffff 0xffff&y=y  
  //!x=1 if x==0  ~(!x)+1=0xffff   0xffff&z=z  
  //!x=1 if x==0  !!x=0  ~(!!x)+1=0  0&y=0  
  // ./btest -f conditional -1 0x80000000 -2 0x7fffffff -3 0x80000000
  return ((~(!x)+1)&z)+((~(!!x)+1)&y);
}
/* 
 * isLessOrEqual - if x <= y  then return 1, else return 0 
 *   Example: isLessOrEqual(4,5) = 1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 24
 *   Rating: 3
 */
int isLessOrEqual(int x, int y) {
  //x y为异号的情况  x>>31 最低位为x符号位 若x为负数 y为非负数 x>>31=1 !(y>>31)=1 相与结果为1  其余结果都为0   ((x>>31)&!(y>>31)))
  //如果x y异号 (!((x>>31)^(y>>31)))=0 排除若x为负数 y为正数的情况剩下  x为非负数 y为负数  结果为0 不须计算之后x-y的情况
  //x y同号的情况 若果同号 除符号位外哪个数更大那么该数就更大 
  //(~x+1)求x的相反数 y+(~x+1)=y-x 右移31位将最低位置为y-x的符号位  y-x>=0  符号位为0  y-x<0  符号位为1
  // ./btest -f isLessOrEqual -1 0x7fffffff  -2 0 
  return (x>>31)&(!(y>>31)) | (!((x>>31)^(y>>31))) & (!((y+(~x+1))>>31));
}
//4
/* 
 * logicalNeg - implement the ! operator, using all of 
 *              the legal operators except !
 *   Examples: logicalNeg(3) = 0, logicalNeg(0) = 1
 *   Legal ops: ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 4 
 */
int logicalNeg(int x) {
  //x和x的相反数符号位相反  x!=0  
  //x的符号位=-x符号位 x=0
  //x|(-x) 右移31位则最低位为1 x!=0 0x8000000000=~0x8000000000+1 只有0和-0 两个数都没有负数 因此判断符号位是否有1 x|(-1)存在1 则返回0
  //最低位为0 x==0 
  //存在1 ~(x|(~x+1)))>>31最低位为0  
  //不存在1 ~(x|(~x+1)))>>31最低位为1  由于是算数右移 高位补符号位 所以和0x01相于  
  // ./btest -f logicalNeg -1 0x7fffffff
   return ((~(x|(~x+1)))>>31)&1;
}
/* howManyBits - return the minimum number of bits required to represent x in
 *             two's complement
 *  Examples: howManyBits(12) = 5
 *            howManyBits(298) = 10
 *            howManyBits(-5) = 4
 *            howManyBits(0)  = 1
 *            howManyBits(-1) = 1
 *            howManyBits(0x80000000) = 32
 *  Legal ops: ! ~ & ^ | + << >>
 *  Max ops: 90
 *  Rating: 4
 */
//(x>>31) 将x符号位右移31位   若x为正数 则为0x0000 取反加1为0x0000    若x为负数 则为0x0001 取反加1为0xffff 
//(~x+1)>>31 将-x符号位右移31位 若x为负数 则为0x0000 取反加1为0x0000    若x为正数 则为0x0001 取反加1为0xffff 
//表示一个正数 需要确定最高位1的位置 
//表示一个负数 需要确定最高位0的位置 +1 即为总共需要的在two's compliment 中表示的位数
//位数最多有31+1=32 位 31=111111b 等于六位二进制数 
//b16=1000 / 0000 代表最高位1是否大于16位 若大于16 位说明x的低16位都是需要的 将该位置1
//b8=100 / 000 代表最高位1是否大于16位 若大于0 位说明x的低16位都是需要的 将该位置1
int howManyBits(int x) {
  int b16,b8,b4,b2,b1,b0;
  int sign=(x>>31);
  x=(x&~sign)|(~x&sign);//统一判断最高位1的位置 同时将符号位置0
  b16=!!(x>>16)<<4;//高16位>0 说明低16位需要用于表示x
  x=x>>b16;//去除已经判断过的16位数
  b8=!!(x>>8)<<3;//判断剩余16位数的高8位数是否>0 若大于0 位说明剩余的低8位都是需要的 将该位置1
  x=x>>b8;
  b4=!!(x>>4)<<2;
  x=x>>b4;
  b2=!!(x>>2)<<1;
  x=x>>b2;
  b1=!!(x>>1);
  x=x>>b1;
  b0=!!(x);
  return b16+b8+b4+b2+b1+b0+1;
}
//float
/* 
 * floatScale2 - Return bit-level equivalent of expression 2*f for
 *   floating point argument f.
 *   Both the argument and result are passed as unsigned int's, but
 *   they are to be interpreted as the bit-level representation of
 *   single-precision floating point values.
 *   When argument is NaN, return argument
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
unsigned floatScale2(unsigned uf) {
  //单精度浮点型  s(1 bit)  exp(8 bit) frac(23 bit)
  //将f*2实际上就是将指数部分 +1
// ./btest -f floatScale2 -1 0xfffffff
// ./btest -f floatScale2 -1 0x7fffff
/*
  unsigned sign=uf&(1<<31);
  unsigned exp=(uf&(0xff<<23))>>23;//取出指数部分 其余位为0  并移位到低8位
   unsigned frac=uf&0x7fffff;//取出frac 低23位
  if(!((exp)^0xff)) return uf;//如果exp部分和0xff相同 异或结果为0 返回uf  因为exp+1会溢出  
  if(!(frac) && !(exp)) return uf;//若exp hefrac都为0 返回uf
  //将uf*2 有两种情况 一种是exp!=0 那么直接在exp上+1即可
  //还有一种情况是 exp=0 那么frac<<1 如果出现进位则exp+1
  if(!(exp))
  {
     if(frac&(1<<22))exp+=1;//frac最高位为1 左移会出现进位
    frac=(frac<<1)&0x7fffff; 
  }
  else
  {
    exp+=1;
  }
  return exp<<23|frac|sign;*/
  //思路2
   unsigned sign=uf&(1<<31);
  unsigned exp=(uf&(0xff<<23))>>23;//取出指数部分 其余位为0  并移位到低8位
  if(exp==0) return (uf<<1)|sign;//exp=0 左移frac
  if(exp==255) return uf;
  exp=exp+1;//exp+1即为原来的数*2
  if(exp==255) return sign|(0xff<<23);
  return (exp<<23)|(uf& ~(0xff<<23));
}
/* 
 * floatFloat2Int - Return bit-level equivalent of expression (int) f
 *   for floating point argument f.
 *   Argument is passed as unsigned int, but
 *   it is to be interpreted as the bit-level representation of a
 *   single-precision floating point value.
 *   Anything out of range (including NaN and infinity) should return
 *   0x80000000u.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
int floatFloat2Int(unsigned uf) {
  unsigned sign=uf>>31;
  unsigned exp=(uf&(0xff<<23))>>23;//取出指数部分 其余位为0  并移位到低8位
  unsigned frac=(uf&0x7fffff)|(1<<23);//取出frac 低23位 同时将第24位置1 转化为M=1.xxxx的形式
  exp=exp?(exp-((1<<7)-1)) :(1-((1<<7)-1));
  //printf("exp:%x\n",exp);
  //./btest -f floatFloat2Int -1 0x800000
  if(!((uf<<1)))return 0;//uf<<1 去掉符号位 如果frac 和exp部分都为0 那么结果返回0
  if((exp>>31)) return 0;//float 2 int 是向下取整 小数点部分都舍去 因此exp小于0 代表 1.xxxx 右移 向下取整均为0
  if(exp>31) return 1<<31; //超出表示范围
  if(exp>23) frac=frac<<(exp-23);//frac左移23位才是真实的表示  1.xxxxxx  因此exp超过23 左移exp-23位就是真实的frac舍去小数部分后的值 
  else frac=frac>>(23-exp);
  if((frac>>31)==0&&sign==0) return frac;
  else if((frac>>31)&1) return 1<<31;//超出表示范围
  else return ~frac+1;//符号位1 取反
}
/* 
 * floatPower2 - Return bit-level equivalent of the expression 2.0^x
 *   (2.0 raised to the power x) for any 32-bit integer x.
 *
 *   The unsigned value that is returned should have the identical bit
 *   representation as the single-precision floating-point number 2.0^x.
 *   If the result is too small to be represented as a denorm, return
 *   0. If too large, return +INF.
 * 
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. Also if, while 
 *   Max ops: 30 
 *   Rating: 4
 */
unsigned floatPower2(int x) {
  //./btest -f floatPower2 -1 0x80000000
  //思路1
  /*
   if(x<0)  //x<0  2^x取整只能为1 因此exp=0  E=1-(2^7-1)=-126 1/2^126*frac=2^x   frac=126-x  frac<0 则超出表示范围 返回0  
   {
     unsigned frac=((1<<7)-2)+x;//取出frac 低23位 
     //printf("%x\n",(frac>>31));
     return frac>>31?0:frac;
   }
   else//x>=0 (-1)^s *  M *exp  这里M=1.0000...000  exp-(2^7-1)=x   x>2^7 超出表示范围 返回0xff<<23
   {
      if(x>(1<<7)) return 0xff<<23;
      return (x+((1<<7)-1))<<23;
   }*/
  //思路2
  int INF = 0xff<<23;
  int exp = x + 127;
  if(exp <= 0) return 0;
  if(exp >= 255) return INF;
  return exp << 23;
}
