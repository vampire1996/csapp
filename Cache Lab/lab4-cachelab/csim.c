#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdio.h>
# include <string.h>
#include <stdlib.h> 
#include "cachelab.h"

typedef int bool;

#define true 1
#define false 0

typedef struct line //由于只是模拟cache缓存 的miss hit eviction 不涉及到数据的修改 所以无需设置block的数据
{
    bool isValid;//有效位
    int tag;
    int stamp;//时间戳
} Line;




void initCache();

void updateStamp();
bool* cacheUpdate(int idx,int tag,int E,int *hit,int *miss,int *evict);
void showVerbose(char* op,unsigned *addr,unsigned * size,bool* res);
void printHelpMenu();

Line* cache;
int lineNum;

int main(int argc,char *argv[])       
{      
        int s=0,E=0,b=0;//缓存参数
        char* t;//trace文件路径
        bool v=false; //v=true 表示显示详细信息
	int ch;       
        //读取命令行
        //-s <s> set索引位数   2^s=# of set
        //-E <E> :连接性 每个set的line数目
        //-b <b>:每个block的位数  B=2^b=block size       
	while((ch=getopt(argc,argv,"vs:E:b:t:h"))!=-1)       
	{       

                switch(ch)     
		{       
 			case 's':
                                s=atoi(optarg);
				//printf("option s:%d\n",s);       
				break;       
			case 'E':
                                E=atoi(optarg);       
				//printf("option E:%d\n",E);       
				break;       
			case 'b':       
                                b=atoi(optarg);
				//printf("option b:%d\n",b);       
				break;       
			case 't': 
                                t=optarg;
				//printf("option t:%s\n",t);       
				break;   
			case 'v': 
                                v=true;
				//printf("option v:%d\n",v);      
				break;   
                        case 'h': printHelpMenu();break;
			default:       
				printf("Knowed option:%c\n",ch);
		}		//printf("optopt+%c\n",optopt);       
	}


        lineNum=(1<<s)*E;
        
        initCache();
        
        
        int hit=0, miss=0, evict=0;
     
        //按行读取trace文件
        FILE *fp;
	fp=fopen(t,"r");
	if(fp==NULL)
	{
		printf("can not load file!");
		return 1;
 	}
        unsigned addr,size;
        char op[10];
        int idx,tag;//idx=set_idx*E  E=# of line/set
        bool* res=malloc(sizeof(bool)*4); //res[0]=true 表示hit res[1]=miss 表示miss res[2]=true 表示evict
        int i=0;
        

        while(fscanf(fp,"%s%x,%d ",op,&addr,&size)>0)
        {
            
            // addr x xx x
            //      t s  b
            tag=addr>>(b+s);
            idx=(((tag<<(b+s))^addr)>>b)*E;//(tag<<(b+s))^addr 将高位的tag疑惑变为0 0异或任何数都为原数
           // l--load data m--modify data s-store data
            switch(op[0])
           {
             case 'L':res=cacheUpdate(idx,tag,E,&hit,&miss,&evict); break;
             case 'M':res=cacheUpdate(idx,tag,E,&hit,&miss,&evict); 
             case 'S':res=cacheUpdate(idx,tag,E,&hit,&miss,&evict); //对于modify:如果发生miss需要多一次存储  如果发生hit  
           }   
           if(v==true)showVerbose(op,&addr,&size,res);
           updateStamp();             
           ++i;

        }
        /*
        while(fscanf(fp,"%s%x,%d ",op,&addr,&size)>0 && i<20)
        {
            
            // addr x xx x
            //      t s  b
            tag=addr>>(b+s);
            idx=(((tag<<(b+s))^addr)>>b)*E;//(tag<<(b+s))^addr 将高位的tag疑惑变为0 0异或任何数都为原数
           // l--load data m--modify data s-store data
           printf("tag:%d,idx:%d  ",tag,idx);
            switch(op[0])
           {
             case 'L':res=cacheUpdate(idx,tag,E,&hit,&miss,&evict); break;
             case 'M':res=cacheUpdate(idx,tag,E,&hit,&miss,&evict); 
             case 'S':res=cacheUpdate(idx,tag,E,&hit,&miss,&evict); //对于modify:如果发生miss需要多一次存储  如果发生hit  
           }   
           if(v==true)showVerbose(op,&addr,&size,res);
           updateStamp();             
           ++i;

        }*/
        
        
        fclose(fp);        
        free(cache);            // malloc 完要记得 free 并且关文件
         
        printSummary(hit, miss, evict);             
        
	return 0;
}


void initCache()
{
     
     cache=malloc(sizeof(Line)*lineNum);
     memset(cache,0,sizeof(Line)*lineNum);
        
     //初始化缓存
     for(int i=0;i<lineNum;i++)
     {
        cache[i].isValid=false;
        cache[i].tag=-1;
        cache[i].stamp=-1;
     }
}

bool* cacheUpdate(int idx,int tag,int E,int *hit,int *miss,int *evict)
{
     bool* res=malloc(sizeof(bool)*4); //res[0]=true 表示hit res[1]=miss 表示miss res[2]=true 表示evict
     memset(res,0,sizeof(bool)*4);
     int max_stamp=-2,max_stamp_idx=-1; 
     for(int i=idx;i<idx+E;i++)
     {
          
          if(cache[i].tag==tag)
          {
             *hit+=1;
             res[0]=true;
             cache[i].stamp=0;//stamp=0表示最近使用
             return res;
          }
     } 

     for(int i=idx;i<idx+E;i++)
     {
          if(cache[i].isValid==false)  //表示空行 
          {
             *miss+=1;
             res[1]=true;
             cache[i].stamp=0;
             cache[i].tag=tag;
             cache[i].isValid=true;
             return res;
          }
     }

     //printf("idx:%d,tag:%d\n",idx,tag);
     //如果既没有空line 又没有hit 说明需要找到最早使用的line 将其替换为新的数据
     *evict+=1;
     *miss+=1;
     res[1]=true;
     res[2]=true;
     
     for(int i=idx;i<idx+E;i++)
     {
          //if(tag==6152&&idx==8) printf("cache[%d].tag:%d,\n",i,cache[i].tag);   
          if(cache[i].stamp>max_stamp)  //表示空行 
          {
             max_stamp=cache[i].stamp;
             max_stamp_idx=i;
          }
     }
     cache[max_stamp_idx].stamp=0;
     cache[max_stamp_idx].tag=tag;
     return res;  

}

void updateStamp()
{
    for(int i=0;i<lineNum;i++)
    { 
       if(cache[i].isValid==true) cache[i].stamp++;
    }
}
	

void showVerbose(char* op,unsigned *addr,unsigned * size,bool* res)
{
    char buf[1000];
    memset(buf,0,1000);
    if(res[0]==true)strcat(buf," hit");
    if(res[1]==true)strcat(buf," miss");
    if(res[2]==true)strcat(buf," eviction");
    printf("%s %x,%d %s\n",op,*addr,*size,buf);
}





 
void printHelpMenu(){
    printf("Usage: ./csim [-hv] -s <num> -E <num> -b <num> -t <file>\n");
    printf("Options:\n");
    printf("-h         Print this help message.\n");
    printf("-v         Optional verbose flag.\n");
    printf("-s <num>   Number of set index bits.\n");
    printf("-E <num>   Number of lines per set.\n");
    printf("-b <num>   Number of block offset bits.\n");
    printf("-t <file>  Trace file.\n\n\n");
    printf("Examples:\n");
    printf("linux>  ./csim -s 4 -E 1 -b 4 -t traces/yi.trace\n");
    printf("linux>  ./csim -v -s 8 -E 2 -b 4 -t traces/yi.trace\n");
    printf("\n");
}


