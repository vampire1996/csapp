/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"


/*Basic Constants and macros */

#define WSIZE 4 /*Word and header/footer size(bytes) */
#define DSIZE 8 /*Double word size (bytes)*/
#define CHUNKSIZE (1<<12) /*Extend heap by this amount 2^10=1Kb  1Kb*8=1KB */

#define MAX(x,y) ((x) > (y) ? (x) : (y))

/*Pack a size and allocated bit into word*/
//将块大小和已分配位(a/f)头部(header)信息
#define PACK(size,alloc) ((size) | (alloc))

/*Read and write a word at address p*/
#define GET(p) (*(unsigned int *)(p))
#define PUT(p,val) (*(unsigned int *)(p)=(val))

/*Read the size and allocated fields from address p*/
//每一个字有32位 高29位表示当前块的大小(双字对齐 所以低3位都应该为0)  低3位表示当前块是否被分配(001已分配 000未分配)
#define GET_SIZE(p) (GET(p) & ~0x7)  //获得当前指针指向块的大小
#define GET_ALLOC(p) (GET(p) & 0X1)  //获得当前块是否被分配的信息

/*Given block ptr bp,compute address of its header and footer*/
/*header(1 byte)  bp(size-2 bytes)... footer(1 byte)*/
#define HDRP(bp) ((char*)(bp)-WSIZE)
#define FTRP(bp) ((char*)(bp)+GET_SIZE(HDRP(bp))-DSIZE) //这里bp指向的是block的第二个字节，所以需要-DSIZE而不是-WSIZE 

/*Given block ptr bp , compute address of next and previous blocks*/
/*header(1 byte)  bp(size-2 bytes)... footer(1 byte) |  header(1 byte)  bp(size-2 bytes)... footer(1 byte)*/
/*                         block n                                   block n+1                          */
#define NEXT_BLKP(bp) ((char *)(bp)+GET_SIZE(((char *)(bp)-WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp)-GET_SIZE(((char *)(bp)-DSIZE)))


/*将地址q处的值复制到地址p*/
#define COPY(p,q) (*(char *)(p)=*(char *)(q))

//memlib.c包模拟内存系统的相关函数--用于动态内存分配器
/*
void *mem_sbrk(int incr) 将堆扩展size个字节,incr是一个正整数，返回指向新分配的堆区域的首字节的指针
和UNIX的sbrk作用是相同的 
*/

/*
void *mem_heap_lo(void) 返回指向堆首字节的指针
*/

/*
void *men+heap_hi(void) 返回指向堆的最后一个指针
*/

/*
size_t mem_heapsize()返回当前堆的大小(以字节为单位)
*/

/*
size_t mem_pagesize(void) 返回系统的页大小  Linux 系统是4K
 */

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

static char *heap_listp=NULL;//总是指向序言块的下一个块
static char *prev_listp=NULL;//总是指向上一次查询的块

static void *find_fit(size_t asize);
static void place(void *bp,size_t size);
static void *extend_heap(size_t words);
static void *coalesce(void *bp);
int mm_check(void);
static void* next_fit(size_t asize);

//static int largest_free_block=0;//维护当前最大的空闲块 如果malloc的大小大于该空闲块 直接扩展堆

/*
堆起始位置 | 序言块  | 普通块1 |普通块2 ... | 结尾块hdr

*/

/* 
 * mm_init - initialize the malloc package.
 */
//初始化 比如分配初始堆区域  如果出现问题返回-1 否则返回0
int mm_init(void)
{
    /*Create the initail empty heap*/
    if( (heap_listp = mem_sbrk(4*WSIZE))== (void*)-1) return -1;//当前空间不足
    PUT(heap_listp,0);//对齐填充
    //printf("%x,%x\n",heap_listp,heap_listp+(1*WSIZE));
    //序言块是只包含header和footer的8字节已分配块--永不释放
    PUT(heap_listp+(1*WSIZE),PACK(DSIZE,1));//序言块header
    PUT(heap_listp+(2*WSIZE),PACK(DSIZE,1));//序言块footer
    PUT(heap_listp+(3*WSIZE),PACK(0,1));//结尾块header    
    
    

    
    
    heap_listp+=(2*WSIZE);//heap_listp指向序言块的下一个块--第一个普通块
    //printf("mm_init\n");  
   
    /*Extend the empty heap with a free block of CHUNKSIZE bytes*/
    if(extend_heap(CHUNKSIZE/WSIZE)==NULL) return -1;//如果当前无法扩展一个堆 那么返回-1
    prev_listp=mem_heap_lo()+4*WSIZE;//指向有效载荷的首地址
    return 0;
}

static void *extend_heap(size_t words)
{
    char* bp=NULL;
    size_t size;

    /*Allocate an even number of words to maintain alignment*/
    size=(words%2)?(words+1)*WSIZE:words*WSIZE;//对奇数向上取整以保证双字对齐
    if((long)(bp=mem_sbrk(size))==-1) return NULL;//内存空间不足
    //mem_sbrk 返回一个双字对齐的内存片 紧跟在结尾块的后面 这个头部变成了新的空闲块的头部

    //printf("new_head:%x,alloc:%d,size:%d\n",HDRP(bp),GET_ALLOC(HDRP(bp)),GET_SIZE(HDRP(bp)));

    /*Initialize free block header/footer and the epilogue header*/
    //这里新块的header实际上就是之前的结尾块header    
    /*   结尾header(4 byte)==HDRP(BP) | bp  */
    PUT(HDRP(bp),PACK(size,0));//释放块的header
    PUT(FTRP(bp),PACK(size,0));//释放块的footer
    PUT(HDRP(NEXT_BLKP(bp)),PACK(0,1));//新的结尾块header
    
    //largest_free_block=MAX(largest_free_block,size);
   
    //Coalesce if the previous block was free
    return coalesce(bp);
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
//返回一个指向已分配区域(有效载荷至少为size)--这个区域应该在堆内部 同时不能和其他的块重叠
//以8字节实现对齐
void *mm_malloc(size_t size)
{
    /*
    int newsize = ALIGN(size + SIZE_T_SIZE);
    void *p = mem_sbrk(newsize);
    if (p == (void *)-1)
	return NULL;
    else {
        *(size_t *)p = size;
        return (void *)((char *)p + SIZE_T_SIZE);
    }*/

   size_t asize;  /*Adjust block size*/
   size_t extendsize; /*Amount to extend heap if no fit*/
   char *bp=NULL;
    
   
   
   /*Ignore spurious requests*/
   if(size==0) return NULL;

   /*Adjust block size to include overhead and alignment requests*/
   if(size<=DSIZE) asize=2*DSIZE;
   else asize=DSIZE * ( (size+(DSIZE)+(DSIZE-1)) /DSIZE );
   
   //printf("mm_malloc:%d,%d\n",size,asize);  

   /*Search the free list for a fit*/
   //if((bp=find_fit(asize))!=NULL) /*first fit*/
   if((bp=next_fit(asize))!=NULL) /*next fit*/
   {
      place(bp,asize);
      /*printf("hit\n");
      mm_check();
      printf("\n");*/
      return bp;
   }
   
   //printf("mm_malloc:%d,%d\n",size,asize);  
   
   /*No fit found.Get more memory and place block*/
   extendsize=MAX(asize,CHUNKSIZE);
   if((bp=extend_heap(extendsize/WSIZE))==NULL)return NULL;
   place(bp,asize);
   /*printf("miss\n");
   mm_check();
   printf("\n");*/
   return bp;
}

static void *coalesce(void *bp)
{
    size_t prev_alloc=GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc=GET_ALLOC(HDRP(NEXT_BLKP(bp)));

    int flag=0;
    if((char*)bp==prev_listp || (char*)PREV_BLKP(bp)==prev_listp || (char*)NEXT_BLKP(bp)==prev_listp)flag=1;
    //如果前块 当前块 后块中有上一次扫描位置prev_listp 需要将其更新

    size_t size=GET_SIZE(HDRP(bp));//当前块的大小
    //printf("size:%d,prev:%x,%d,next:%x,%d\n",size,PREV_BLKP(bp),prev_alloc,NEXT_BLKP(bp),next_alloc);
    if(prev_alloc && next_alloc)//如果前后块都已经被分配
    {
       return bp;
    }
    else if(prev_alloc && !next_alloc)//如果前块已经被分配 后块未被分配
    {
       //printf("prev:%x,next:%x\n",bp,NEXT_BLKP(bp));
       size+=GET_SIZE(HDRP(NEXT_BLKP(bp)));//更新当前块的大小
       PUT(HDRP(bp),PACK(size,0));//由于size的大小发生了改变 因此下一句计算FTRP时当前块的size已经发生了改变，实际上指向的是下一块的footer
       PUT(FTRP(bp),PACK(size,0));
       
       //由于next block 是空闲块 所以它的header中的值是什么并不重要--无需修改
    }
    else if(!prev_alloc && next_alloc)//如果前块都未被分配 后块已经被分配
    {
       size+=GET_SIZE(HDRP(PREV_BLKP(bp)));//更新当前块的大小
       PUT(FTRP(bp),PACK(size,0));//更改当前块的footer和前块的header中的size 将prev block的有效载荷首地址作为新的块的首地址
       PUT(HDRP(PREV_BLKP(bp)),PACK(size,0));
       bp=PREV_BLKP(bp);
    }
    else//如果前后块都未被分配
    {
       size+=GET_SIZE(HDRP(PREV_BLKP(bp)))+GET_SIZE(HDRP(NEXT_BLKP(bp)));//更新当前块的大小    
       PUT(HDRP(PREV_BLKP(bp)),PACK(size,0));
       PUT(FTRP(NEXT_BLKP(bp)),PACK(size,0));//更改前块的header和后块的footer中的size 将prev block的有效载荷首地址作为新的块的首地址
       bp=PREV_BLKP(bp);
    }
    
    if(flag==1)prev_listp=bp;//更新prev_listp

    return bp; 
}

/*
 * mm_free - Freeing a block does nothing.
 */
//将ptr指向的区域释放  
//只有ptr是通过之前mm_malloc或者mm_realoc返回的指针 同时还没有被释放(二次free()可能会free()已经被重新分配的区域) 才能正确完成free()操作
void mm_free(void *ptr)
{
    size_t size=GET_SIZE(HDRP(ptr));
    
    //printf("mm_free,%x,%x,%x\n",(char *)ptr,HDRP(ptr),FTRP(ptr));  
    //printf("mm free,ptr:%x\n",(char *)ptr);
    
    PUT(HDRP(ptr),PACK(size,0));//释放块的header
    PUT(FTRP(ptr),PACK(size,0));//释放块的footer
    
    coalesce(ptr);
    /*mm_check();
    printf("\n");*/
}

void copy(char *p,char* q,int size)
{
    for(int i=0;i<size;i++)
    {
        COPY(p,q);
        p+=1;
        q+=1;
    }
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
//返回一个指向已分配区域(有效载荷至少为size)--这个区域应该在堆内部 同时不能和其他的块重叠
//以8字节实现对齐
//如果ptr为null 那么等同于调用mm_malloc(size) 
//如果size为0 等同于调用mm_free(ptr)
//如果ptr不为NULL ptr一定是通过之前mm_malloc 或者mm_realoc返回的指针 
//调用mm_realoc 改变了ptr指向的原来块的大小(old block) 改变为size 返回新block的地址
//可能新块的地址和旧块的地址相同 也可能不同 取决于实现，旧块内部碎片的数目以及size的大小
//新块的内容 和旧ptr块的内容相同(截止到新旧大小的最小值) 其它都是未初始化的
//比如旧块的大小是8字节 新块的大小是12字节  那么新块的钱8个字节和旧块是相同的 后面的4字节是未初始化的
//在shell中输入man malloc 来查看完整文档 
/*
//next fit Perf index = 44 (util) + 24 (thru) = 68/100
//first fit Perf index=59/100
void *mm_realloc(void *ptr, size_t size)
{
    
   
    void *newptr;
    if(ptr==NULL)//如果ptr为null 那么等同于调用mm_malloc(size) 
    {
        newptr = mm_malloc(size);
        return newptr;
    }
    
    if(size==0)//如果size为0 等同于调用mm_free(ptr)
    {
       mm_free(ptr);
       return NULL;
    }
   
   
   
    //如果ptr不为NULL ptr一定是通过之前mm_malloc 或者mm_realoc返回的指针 
    size_t copySize=GET_SIZE(HDRP(ptr));//之前块的大小
    //if (size < copySize) copySize = size;//复制长度为min(copySize,size)
    //释放当前块
    PUT(HDRP(ptr),PACK(copySize,0));//释放块的header
    PUT(FTRP(ptr),PACK(copySize,0));//释放块的footer
    newptr=coalesce(ptr);//将当前块和前面以及后面的空闲块合并
    size_t coalesceSize=GET_SIZE(HDRP(newptr));//合并之后块的大小
  
    if (size < copySize) copySize = size;//复制长度为min(copySize,size)

    size_t asize;  //Adjust block size
    //Adjust block size to include overhead and alignment requests
    if(size<=DSIZE) asize=2*DSIZE;
    else asize=DSIZE * ( (size+(DSIZE)+(DSIZE-1)) /DSIZE );
    
    //printf("Before Realloc,ptr:%x,newptr:%x,size:%d,copySize:%d,coaSize:%d\n",ptr,newptr,size,copySize,coalesceSize);
    //mm_check();
    if(coalesceSize<asize)//合并之后空闲块大小小于asize 说明无法在当前空闲块执行calloc 需要开辟新的块
    {
         newptr = mm_malloc(size);//找到符合要求大小size的块
         memcpy(newptr, ptr, copySize);//执行复制操作
    }
    else //合并之后空闲块大小大于等于size 说明可以在当前空闲块执行calloc 无需开辟新的块
    {
        //memcpy(newptr, ptr, copySize);
   
        //printf("ptr:%x,newptr:%x,copySize:%d\n",ptr,newptr,copySize);
        //执行复制操作
        if((char *)newptr!=(char *)ptr) copy(newptr,ptr,copySize);//如果合并后的新指针和ptr相同 则无需复制

        place(newptr,asize);
    }

    return newptr;
}*/

//简化操作 realloc执行速度有了大幅提升 next fit--Perf index = 44 (util) + 40 (thru) = 84/100
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;

    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    size = GET_SIZE(HDRP(oldptr));//旧块大小(包括header和footer)
    copySize = GET_SIZE(HDRP(newptr));//新块大小(包括header和footer)
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize-DSIZE);//减去header和footer
    mm_free(oldptr);
    return newptr;
}




int mm_check(void)
{
    char* bp=mem_heap_lo();

    bp+=4*WSIZE;//指向有效载荷的首地址
    
    while(GET_SIZE(HDRP(bp))!=0) ////结尾块大小为0 所以当前block大小为0 说明到链表末端了  
    { 
        printf("check:addr:%x,alloc:%d,size:%d,hd:%x,ft:%x,next:%x\n",bp,GET_ALLOC(FTRP(bp)),GET_SIZE(FTRP(bp)),HDRP(bp),FTRP(bp),NEXT_BLKP(bp));  
        
        bp=NEXT_BLKP(bp);  
        
    } 
    return 1;//堆是连续的
    return 0;
}

/*
堆起始位置(4 byte) | 序言块header(4 byte) footer(4 byte)  | header(4 byte) 有效载荷 footer(4 byte) |普通块2 ... | 结尾块hdr

*/

static void *find_fit(size_t asize)//first fit
{
    if(mem_heapsize()<=4)return NULL;

    char* bp=mem_heap_lo();

    bp+=4*WSIZE;//指向有效载荷的首地址
    
    

    while(GET_SIZE(HDRP(bp))!=0) ////结尾块大小为0 所以当前block大小为0 说明到链表末端了  
    //while(bp<=(char *)mem_heap_hi() ) ////结尾块大小为0 所以当前block大小为0 说明到链表末端了  
    { 
        //printf("Next:%x,cur:%x,alloc:%d,size:%d\n",NEXT_BLKP(bp),bp,GET_ALLOC(HDRP(bp)),GET_SIZE(HDRP(bp)));
        //printf("alloc:%d,size:%d,addr:%x,next:%x\n",GET_ALLOC(HDRP(bp)),GET_SIZE(HDRP(bp)),bp,NEXT_BLKP(bp));  
        if(GET_ALLOC(HDRP(bp))==0 && GET_SIZE(HDRP(bp))>=asize) return bp;
        bp=NEXT_BLKP(bp);  
    }
    
    

    return NULL;
}


static void place(void *bp,size_t size)
{
   size_t old_size=GET_SIZE(HDRP(bp));
   char* old_FT=FTRP(bp);

   /*header(1 byte)  bp(size-2 bytes)... footer(1 byte) |  header(1 byte)  bp(size-2 bytes)... footer(1 byte)*/
   /*                         block n                                   block n+1                          */
   
   PUT(HDRP(bp),PACK(size,1));//将新块的header设置为已分配
   ////这里bp指向的是block的第二个字节，所以需要-DSIZE而不是-WSIZE 
   PUT(bp+(size-DSIZE),PACK(size,1));//将新块的footer设置为已分配 
   if(old_size>size)
   {
      //如果当前块分割出size大小之后还有剩余 更新剩余块的大小
      PUT(bp+(size-WSIZE),PACK(old_size-size,0));//将新块的footer设置为已分配
      PUT(old_FT,PACK(old_size-size,0));
      //printf("dd:alloc:%d,size:%d,next:%x\n",GET_ALLOC(FTRP(NEXT_BLKP(bp))),GET_SIZE(FTRP(NEXT_BLKP(bp))),NEXT_BLKP(bp));  
      
   }    
    
}





/* 
 *next_fit - use next fit strategy to find an empty block.
从上一次结束扫描的位置开始扫描
这里注意，除了在mm_init中初始化prev_listp,在合并操作中，如果前块 当前块 后块中有上一次扫描位置prev_listp 需要将其更新
 */
static void* next_fit(size_t asize)
{
    for (char* bp = prev_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp))
    {
        if (!GET_ALLOC(HDRP(bp)) && GET_SIZE(HDRP(bp)) >= asize)
        {
            prev_listp = bp;
            return bp;
        }
    }

    for (char* bp = heap_listp; bp != prev_listp; bp = NEXT_BLKP(bp))
    {
        if (!GET_ALLOC(HDRP(bp)) && GET_SIZE(HDRP(bp)) >= asize)
        {
            prev_listp = bp;
            return bp;
        }
    }
    return NULL;
}





