#include <stdio.h>

#include "csapp.h"

#include "cache.h"

/*编译错误stray ‘\343’ in program解决办法 这种错误是由于代码中含有中文的引号or其他的全脚符号引起的，而小编遇到的是由中文空格引起的，不易发现
通过命令 cat -A proxy.c 查看到乱码的就是中文符号引起的
*/

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/*cache function*/
void cache_init();
int cache_find(char *url);
int cache_eviction();
void cache_Update_Stamp(int index);
void cache_url(char *url,char *buf);
void readerPre(int i);
void readerAfter(int i);
void writerPre(int i);
void writerAfter(int i);
//LRU_MAGIC_NUMBER 是LRU可能出现的最大时间戳
//由于缓存总大小为　1MB 缓存对象最大大小为1KB 所以设计一个包括10个缓存块(每个块的大小为1KB)的缓存
#define LRU_MAGIC_NUMBER 99999
#define CACHE_OBJS_COUNT 10
typedef struct 
{
   char cache_obj[MAX_OBJECT_SIZE];
   char cache_url[MAXLINE];
   int isEmpty;
   int stamp;//时间戳
   int readcnt; //reader 计数器
   int writecnt ;// writer 计数器
   sem_t rdcntmutex;//保护对readcnt进行访问
   sem_t wmutex;//保护对cache block进行读操作
   sem_t wtcntmutex;//保护对writecnt进行访问
   sem_t queue;
}cache_block;

typedef struct
{
  cache_block cache_objs[CACHE_OBJS_COUNT];
  int cache_num;
}Cache;

int doit(int fd) ;
void connect_to_server(char* new_uri,char *host,char* port,rio_t* rio_client,int connfd,char* original_url);

void clienterror(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg) ;

int parse_uri(char *uri, char *newuri,char *host, char *port) ;

char * PortPlusOne(char* port) ;

char* find(char* s,char c,int num);

void* thread(void* vargp);


/*Global variable*/

Cache cache;

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

int main(int argc,char **argv)
{
    int listenfd, *connfdp;
    char hostname[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pthread_t tid;

    /* Check command line args */
    if (argc != 2) {
	fprintf(stderr, "usage: %s <port>\n", argv[0]);
	exit(1);
    }
   
    cache_init();//初始化缓存
     
    //需要两个端口　一个监听客户端，和客户端构成连接　另一个监听服务端，和服务端构成连接
    char* port=argv[1];
    listenfd = Open_listenfd(port);//开启监听客户端端口port
    while (1) {

	clientlen = sizeof(clientaddr);
        connfdp=malloc(sizeof(int));
	*connfdp = Accept(listenfd, (SA *)&clientaddr, &clientlen); 
        Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, 
                    port, MAXLINE, 0);
        printf("Accepted connection from (%s, %s)\n", hostname, port);
        
        //创建对等线程　　在thread函数中　会将connfdp类型转换并赋值给connfd 
        //因此*connfdp = Accept(listenfd, (SA *)&clientaddr, &clientlen); 这句如果发生在thread函数中　赋值给connfd之后　
        //thread函数获得的连接描述符就是上一个连接的描述符--产生竞争
        //所以这里使用malloc　将每次循环时对connfdp分配一个新的内存区域　这样就能在对等线程中获得正确的连接描述符
        Pthread_create(&tid,NULL,thread,connfdp);
                                                   
    }

    //printf("%s", user_agent_hdr);
    return 0;
}

char * PortPlusOne(char* port) //返回port+1的字符串形式
{
   int len=strlen(port);
   char* newPort=malloc(len);
   strcpy(newPort,port);
   newPort[len-1]++;
   return newPort;
}


/*
 * doit - handle one HTTP request/response transaction
 */
/* $begin doit */
int doit(int fd) 
{

    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char urlcpy[MAXLINE];
    char new_uri[MAXLINE],host[MAXLINE], *port=malloc(6);//端口号最大为65536 所以分配6个字节
    rio_t rio;

    /* Read request line and headers */
    Rio_readinitb(&rio, fd);
    if (!Rio_readlineb(&rio, buf, MAXLINE))  
        return -1;
    printf("%s", buf);
    //uri在没有解析之前是http://www.baiu.com/　也就是url的形式
    sscanf(buf, "%s %s %s", method, uri, version);      
    //判断是否是合法的HTTP请求
    if (strcasecmp(method, "GET")) {                     
        clienterror(fd, method, "501", "Not Implemented",
                    "Tiny does not implement this method");
        return -1;
    }  
    
    
    strcpy(urlcpy,uri);//保存一个原始uri的副本urlcpy
    

    int i;
    //可以在缓存中找到与当前url对应的缓存块
    if((i=cache_find(urlcpy))!=-1)
    { 
       readerPre(i);
       //printf("i:%d\n",i);
       Rio_writen(fd,cache.cache_objs[i].cache_obj,strlen(cache.cache_objs[i].cache_obj));
       readerAfter(i);
       cache_Update_Stamp(i);
       return 0;
    }
    
    printf("No cache,connect to end server!\n");
    //无法在缓存中找到与当前url对应的缓存块　那么需要和终端服务器连接　向其请求数据


    //解析uri
    //port=PortPlusOne(proxy_port) ;//服务器的默认监听端口为代理监听端口+1
    if(!parse_uri(uri, new_uri,host,port))
    {
        port="80";//当url中不包含端口　默认web端口号为80
    }
    printf("new uri:%s,host:%s,port:%s\n",new_uri,host,port);

    //如果是合法的HTTP请求，那么开启和服务器的连接 并且将http请求发送给服务器
    connect_to_server(new_uri,host, port ,&rio,fd,urlcpy);   

    
  
    return 0;
                                                                     


}

/*
HTTP请求报文由4个部分组成，它们分别是请求行、请求头部、空行和报文主体；请求行、请求头部和空行是必需的而报文主体是可选的。
(1) 请求行又包含3个部分：请求方法、URL和协议版本；它们之间用空格分开，请求行最后以一个回车符和一个换行符结尾。回车符是"\r"，换行符是"\n"
(2) 请求头部用来告知服务器该请求和客户端本身的一些额外信息，每个请求头都是一个键值对，键和值之间用英文冒号隔开。每个请求头单独形成一行，它们的末尾都是一个回车符和换行符。
(3)在请求头部的后面是一个空行，它只包含一个回车符和一个换行符，不包含其它任何内容，连空格也不能包含。这个空行用于标记请求头部已结束；它是必须要有的，即便使用GET这样不包含报文主体的请求方法时也要有这个空行
(4)请求报文中的报文主体就是要提交给服务器的数据，比如当我们使用POST方法提交表单时，表单中的内容就包含在请求报文的报文主体中。在某些情况下请求报文中是没有报文主体的，比如使用GET方法，此时若要向服务器传递参数，那么参数就只能包含在URL的查询字符串（query string）中。

注意:　GET方法没有请求主体
*/

/*当客户端发起一个连接请求时，客户端套接字地址的端口是由一个内核自动分配的，称为临时端口，而服务器套接字地址中的端口通常是某个知名端口－－比如web服务器使用80端口*/
void connect_to_server(char* new_uri,char *host,char* port,rio_t* rio_client,int connfd,char* original_url)
{
    //对于服务器来说　代理相当于客户端
   int clientfd = Open_clientfd(host,port);//连接到客户端端口port

   if(clientfd<0){
        printf("connection failed\n");
        return;
    }

   char buf_client[MAXLINE];
   rio_t rio_server;//读服务器HTTP响应的缓冲区
    
    printf("HTTP request start\n");
    //先将HTTP请求发送给服务器
    /*先发送请求行*/
    sprintf(buf_client, "GET %s HTTP/1.0\r\n", new_uri);
    Rio_writen(clientfd, buf_client, strlen(buf_client));
    //再发送请求头部
    /*以下内容是必须发送的*/
    sprintf(buf_client, "Host: %s\r\n", host);
    Rio_writen(clientfd, buf_client, strlen(buf_client));
    sprintf(buf_client, "%s",user_agent_hdr);
    Rio_writen(clientfd, buf_client, strlen(buf_client));
    sprintf(buf_client, "Connection: close\r\n");
    Rio_writen(clientfd, buf_client, strlen(buf_client));
    sprintf(buf_client, "Proxy-Connection: close\r\n");
    Rio_writen(clientfd, buf_client, strlen(buf_client));
    
    //rio_client是代理和服务器连接对应的读缓冲区　根据要求，如果浏览器发送了其余的请求头，直接发送即可
    Rio_readlineb(rio_client,buf_client, MAXLINE);
    while(strcmp(buf_client, "\r\n")) {          
        //忽略已经发送的请求头 
        /*strstr(const char*src,char * sub) 查找首次src中首次出现子串sub的位置　如果没有找到返回NULL*/
        if(!strstr(buf_client,"Host") && !strstr(buf_client,"User-Agent") && !strstr(buf_client,"Connection") && !strstr(buf_client,"Proxy-Connection"))
        {
             //printf("%s",buf_client);
	     Rio_writen(clientfd, buf_client, strlen(buf_client));
        }

        Rio_readlineb(rio_client, buf_client, MAXLINE);
    }
    printf("HTTP request end\r\n");
    //发送空行　因为以上循环的终止条件是buf_clinet为空行 buf_clinet=="\r\n" strcmp返回0
    Rio_writen(clientfd, "\r\n", strlen("\r\n"));
    
    
    printf("HTTP response start\n");
    //再将从服务器获得的HTTP响应发送给客户端
    Rio_readinitb(&rio_server, clientfd);
    //coonfd是代理和客户端之间建立连接的描述符
    //HTTP响应结束是EOF 

    /*receive message from end server and send to the client*/
    size_t n;
    char cache_buf[MAX_OBJECT_SIZE];//缓存当前HTTP响应
    int HTTP_reponse_size=0;//HTTP响应的大小
    while((n=Rio_readlineb(&rio_server,buf_client,MAXLINE))!=0)
    {
        //printf("reponse size:%d\n",HTTP_reponse_size);
        HTTP_reponse_size+=n;
        if(HTTP_reponse_size<MAX_OBJECT_SIZE) strcat(cache_buf,buf_client);
        Rio_writen(connfd,buf_client,n);
    }
   
    
    //不能使用以下方法的原因是strlen遇到字符串终止符'\0' 然而结束字符c并没有字符串终止符　这样会导致访问非法的地址
    /*
    while(rio_readnb(&rio_server,&c,1))
    {
      Rio_writen(connfd,&c,strlen(&c));  
    }*/

    printf("HTTP response end\n");

    Close(clientfd); 
    
    //如果HTTP响应的大小没有超过设计的最大缓存块大小　说明可以在代理本地缓存该HTTP响应的内容
    if(HTTP_reponse_size<MAX_OBJECT_SIZE)
    {
       cache_url(original_url,cache_buf);
    }

}



/*
 * clienterror - returns an error message to the client
 */
/* $begin clienterror */
void clienterror(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg) 
{
    char buf[MAXLINE];

    /* Print the HTTP response headers */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n\r\n");
    Rio_writen(fd, buf, strlen(buf));

    /* Print the HTTP response body */
    sprintf(buf, "<html><title>Tiny Error</title>");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<body bgcolor=""ffffff"">\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "%s: %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<p>%s: %s\r\n", longmsg, cause);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<hr><em>The Tiny Web server</em>\r\n");
    Rio_writen(fd, buf, strlen(buf));
}

/* 
当用户在浏览器输入URL地址，浏览器会向代理发送类似这样的请求

GET http://www.cmu.edu/hub/index.html HTTP/1.0

也可包含自定义的服务器的web端口号

GET http://www.cmu.edu:8080/hub/index.html HTTP/1.0

代理应该将请求解析为:1)主机名:www.cmu.edu; 2) 端口号 8080(如果存在)　３)路径/查询: /hub/index.html

主机名说明服务器在哪里，端口号说明服务器上的监听端口号

这样代理就可以确定他应该打开一个到www.cmu.edu的连接，并且发送一个HTTP请求:

GET /hub/index.html HTTP/1.0

HTTP请求以回车换行为结束('\r\n')

现代的浏览器会产生以HTTP/1.1为结尾的HTTP请求，但是代理的请求是HTTP/1.0。在本次实验中，代理一个处理HTTP/1.0请求，然后向服务器发送HTTP/1.0请求。*/

int parse_uri(char *uri, char *newuri,char *host, char *port) 
{
    char *ptr;
    ptr = find(uri, '/',3); //查找第3个出现'/'的位置 http://www.cmu.edu:8080/                     
    if (ptr) {
        //将解析后的新uri保存　并且将原uri分割成　http://www.cmu.edu　和　/hub/index.html
	strcpy(newuri, ptr);
	*ptr = '\0';
    }
    else  strcpy(newuri, ""); 

    ptr = find(uri, '/',2); //查找第2个出现'/'的位置 http://www.cmu.edu               
    if (ptr) {
        //将host解析为  www.cmu.edu:8080
	strcpy(host, ptr+1);
	*ptr = '\0';
    } 
    ptr=find(host, ':',1);  //查找第2个出现':'的位置 www.cmu.edu:8080   
    //if (ptr!=NULL)printf("aa\n");   
    if (ptr!=NULL) {  /* 在uri中存在端口号*/ 
        //将解析后的port保存　并且将原uri分割成host为　www.cmu.edu　和　端口号8080
        //port=malloc(6);//端口号最大为65536 所以分配6个字节
        
	strcpy(port, ptr+1); 
        *ptr = '\0';                                                                                
	return 1;
    }
    
    return 0;
}

//寻找在字符串s中　第num次出现字符'c'的位置，返回指向该位置的指针
char* find(char* s,char c,int num){
   int cnt=0;
   while(*s!='\0')
   {
      //printf("%c",*s);
      if(*s==c)cnt++;
      if(cnt==num) { //printf("\n"); 
      return s;} 
      ++s;
   }
   //printf("\n");
   return NULL;
}

/*Thread routine*/
/*对于高性能web服务器,每次收到web浏览器的连接请求时都创造一个对等线程，每个连接都是一个单独的对等线程处理的。所以对于服务器而言，就没有必要显示的等待每个线程终止。所以每个对等线程在开始处理请求之前都应该在它处理请求之前分离(deatched)它自身，这样就能在它终止之后回收它的内存资源了*/
void* thread(void* vargp)
{
   int connfd=*(int *)((int*) vargp);
   Pthread_detach(pthread_self());
   Free(vargp);
   //printf("bb\n");
   doit(connfd) ;
	                                         
   Close(connfd); 
   
   return NULL;
   
}


/*

如何根据HTTP请求确定所需缓存区是否存在:由于每个HTTP请求都对应一个url，所以利用url作为键值可以唯一确定一个缓存

为每个连接创建一个缓存区，由于用一个大的独占锁来保护对缓存的访问是不可接受的解决方案，因此可以在读写数据时，针对每个缓存区进行加锁操作
*/

/*cache function*/

//注意所有访问缓存的操作都需要加锁


void cache_init()
{
   cache.cache_num=0;
   for(int i=0;i<CACHE_OBJS_COUNT;i++)
   {
        cache.cache_objs[i].isEmpty=1;
        cache.cache_objs[i].stamp=-1;
        cache.cache_objs[i].readcnt=0;
        cache.cache_objs[i].writecnt=0;
        Sem_init(&cache.cache_objs[i].rdcntmutex,0,1);/*wmutex初始化为１*/
        Sem_init(&cache.cache_objs[i].wmutex,0,1);
        Sem_init(&cache.cache_objs[i].wtcntmutex,0,1);/*wmutex初始化为１*/
        Sem_init(&cache.cache_objs[i].queue,0,1);
   }

}

//找到当前HTTP请求的url是否被缓存　返回对应的缓存块索引
int cache_find(char *url)
{
   int i;
   for(i=0;i<CACHE_OBJS_COUNT;i++)
   {
        readerPre(i);
        //printf("i: %d\n",i);
        if(cache.cache_objs[i].isEmpty==0 && strcmp(url,cache.cache_objs[i].cache_url)==0)break;
        readerAfter(i);
   }
   if(i==CACHE_OBJS_COUNT)return -1;
   return i;
}

//选择一个缓存块用于替换
//时间戳=０表示最近使用　LRU算法的本质是找到离上一次使用时间维度上最远的那个缓存块　所以需要找到拥有最大时间戳的缓存块
int cache_eviction()
{
   int i;
   int max_stamp=-2,max_stamp_idx=-1;
   for(i=0;i<CACHE_OBJS_COUNT;i++)
   {
        readerPre(i);
        if(cache.cache_objs[i].isEmpty) //找到一个空缓存块　直接返回
        {
           cache.cache_objs[i].isEmpty=0;
           max_stamp_idx=i;
           readerAfter(i);
           break;
        }
        if(cache.cache_objs[i].stamp>max_stamp) //搜索具有最大时间戳的缓存块
        {  
            max_stamp_idx=i;
            max_stamp=cache.cache_objs[i].stamp;
        }
        readerAfter(i);
   }
   return max_stamp_idx;
}

//更新LRU时间戳
void cache_Update_Stamp(int index)
{

  writerPre(index);
  cache.cache_objs[index].stamp=0;//将这个块设置位最近使用
  writerAfter(index);
  for(int i=0;i<CACHE_OBJS_COUNT;i++)
   {
        if(i!=index && cache.cache_objs[i].isEmpty==0)
        {
           writerPre(i);
           cache.cache_objs[i].stamp++;
           writerAfter(i);
        }
   }
}

//缓存uri和对应HTTP响应的内容
void cache_url(char *url,char *buf)
{
   int i=cache_eviction();//找到一个可以被替换的缓存块
   writerPre(i);
   strcpy(cache.cache_objs[i].cache_url,url);
   strcpy(cache.cache_objs[i].cache_obj,buf);
   cache.cache_objs[i].isEmpty=0;
   writerAfter(i);
   
   cache_Update_Stamp(i);
   
}


/*
信号量mutex保护对全局变量 readcnt的访问　　readcnt统计当前临界区中读者的数量
信号量w堆访问共享对象(比如缓存)的临界区的访问
以下代码基于第一类读者-写者问题　读者优先，要求不让读者等待，除非已经把使用对象的权限已经赋予了一个写着
*/



//进入读者模式之前的操作
void readerPre(int i)
{
   P(&cache.cache_objs[i].rdcntmutex);
   cache.cache_objs[i].readcnt++;
   if(cache.cache_objs[i].readcnt==1) P(&cache.cache_objs[i].wmutex); //first in
   V(&cache.cache_objs[i].rdcntmutex);
}

//读完数据之后的操作
void readerAfter(int i)
{
   P(&cache.cache_objs[i].rdcntmutex);
   cache.cache_objs[i].readcnt--;
   if(cache.cache_objs[i].readcnt==0) V(&cache.cache_objs[i].wmutex); //Last out
   V(&cache.cache_objs[i].rdcntmutex);
}

//进入写者模式之前的操作
void writerPre(int i)
{
   P(&cache.cache_objs[i].wmutex);
}

//写完数据之后的操作
void writerAfter(int i)
{
  V(&cache.cache_objs[i].wmutex);
}













