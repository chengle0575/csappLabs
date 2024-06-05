#include <stdio.h>
#include <stdint.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h> 
#include <unistd.h>
#include <stdlib.h>
#include <netdb.h>
#include <errno.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";


//data structure
typedef struct 
            {
                int connectfd;
                char *buffer;
            } thread_arg;

typedef struct 
            {
                char request[MAX_OBJECT_SIZE];
                int proxyfdc;
                char response[MAX_CACHE_SIZE];
            }cacheblock;

typedef struct 
            {
                cacheblock cachblock;
            }cachemap;

pthread_mutex_t lock;
cachemap cachm[0];
int cachet=1;


//prototype
ssize_t rio_readn(int, void *, size_t ) ;
ssize_t rio_writen(int , void *, size_t); 
void rio_readinitb(rio_t *, int); 
ssize_t rio_readnb(rio_t *rp, void *usrbuf, size_t n);
void proxyserverinit(uint16_t);
void proxyclientconnect(int proxyfdc,struct sockaddr_in *, char* ,char * ,char*);
int parseproxyrequest(char *,char*,char*,char*);
void *threadstart(void *arg);
void sendhttptoserver(int proxyfdc,char* originserverdomainadd, char * filepath);
char* checkreplirequest(char* buffer);

int main(int argc,char * argv[])
{
    
    printf("%s", user_agent_hdr);
     
    uint16_t port=(uint16_t)atoi(argv[1]);

    proxyserverinit(port);
  

    return 0;
}




void proxyserverinit(uint16_t port){

    //create a internet socket
    int proxyfds=socket(AF_INET,SOCK_STREAM,0);
    if(proxyfds==-1){printf("create proxy server fd failed\n");}
    
    
    //struct the sockaddress, the ip address and port should be stored in network byte order(big endian)
    struct sockaddr_in myaddr; //sin_port and sin_addr memebers shall be in network byte order
    memset(&myaddr,0,sizeof(myaddr)); //clear the memory for this socket address and fill them all 0
    myaddr.sin_family=AF_INET;
    inet_pton(AF_INET,"0.0.0.0",&(myaddr.sin_addr)); //converts numbers-and-dots notation into network byte order, and copies to myaddr
    myaddr.sin_port=htons(port); //convert port number to network byte order and assign to myaddr

    
    //bind the addr to my proxy machine
    bind(proxyfds,(struct sockaddr *)&myaddr,sizeof(myaddr)); //use generic sockaddr struct cast to fullfill bind parameter needs


    listen(proxyfds,1); 

    
    struct sockaddr originclientaddr; //this is the structure to store peer address
    socklen_t addlen=sizeof(originclientaddr);
    memset(&originclientaddr,0,sizeof(originclientaddr));
    
    while (1)
    {
        int connectfd=accept(proxyfds,&originclientaddr,&addlen); 
        if(connectfd!=-1){

            char buffer[MAX_OBJECT_SIZE];
            //create a new thread
            pthread_t threadid;
            
            thread_arg *arg=malloc(sizeof(thread_arg));
            arg->connectfd=connectfd;
            arg->buffer=buffer;

            pthread_create(&threadid,NULL,threadstart,arg);

        }
            
    }

}


//thread start route
void *threadstart(void *arg){
     //read buffer
            thread_arg *args = (thread_arg *)arg;
             int connectfd=args->connectfd;



             char *buffer=malloc(MAX_OBJECT_SIZE);

             //pthread_mutex_lock(&lock);
             //strcmp(buffer,args->buffer);
             read(connectfd,buffer,MAX_OBJECT_SIZE); //read from file to memory
             //pthread_mutex_unlock(&lock);
             
             printf("server recived:%s\n",buffer);

             //variable to store  parse
             char* originalserverhostname=malloc(MAX_OBJECT_SIZE);
             char *filepath=malloc(MAX_OBJECT_SIZE);
             char port[10];
             
             //paser the request, decide whether a valid HTTP request
             if(parseproxyrequest(buffer,originalserverhostname,port,filepath)==-1){
                return NULL;};
           
            //check avaible response in cache 
            char* cachedresponse=checkreplirequest(buffer);

            //conver originalserver's hostname into socket address
            struct addrinfo *res;
            struct addrinfo hints;
            memset(&hints,0,sizeof(hints));
            hints.ai_family=AF_INET; //only remains those ip using ipv4
            hints.ai_socktype=SOCK_STREAM; //only remmain those socket of STREAM type

            if(getaddrinfo(originalserverhostname,port,&hints,&res)!=0){printf("getaddrinfo error\n");};
                    
            struct sockaddr_in *originserversockaddr=(struct sockaddr_in *)res->ai_addr;
    
            freeaddrinfo(res); //the getaddrinfo allocate mwmory in heap for res, so need to fress manully


            if(cachedresponse!=NULL){       
                //duplicate request and already has response saved in  memory, no need to connect again
                printf("is response:%s\n",cachedresponse);
                
                int err0=rio_writen(connectfd,cachedresponse,strlen(cachedresponse));
                printf("error?: %s\n",strerror(err0));
   
            }else{

                    char httpreply[MAX_OBJECT_SIZE];  

                    //create a intenet socket
                    int newproxyfdc=socket(AF_INET,SOCK_STREAM,0);
                    if(newproxyfdc==-1){printf("create proxy client fd failed\n");}


                    pthread_mutex_lock(&lock);
                    //put into cachemap
                  
                    strcpy(cachm[cachet].cachblock.request,buffer);
                    cachm[cachet].cachblock.proxyfdc=newproxyfdc;
                    cachm[cachet].cachblock.response[0] = '\0';
                    pthread_mutex_unlock(&lock);
                    cachet++;   
                    

                    proxyclientconnect(newproxyfdc,originserversockaddr,originalserverhostname,filepath,httpreply);

                    //write to connectfd reply to original client
                    rio_writen(connectfd,httpreply,MAX_OBJECT_SIZE);

                    pthread_mutex_lock(&lock);
                    strcpy(cachm[cachet-1].cachblock.response, httpreply);  
                    pthread_mutex_unlock(&lock);

                   
      
            }

           
            close(connectfd);
            printf("reply to original client\n");
            
            free(originalserverhostname);
            free(filepath);
}


char* checkreplirequest(char* buffer){
    pthread_mutex_lock(&lock);
    for(int i=1;i<cachet;i++){
        printf("checking replicate:%d, request:%s\n",i,cachm[i].cachblock.request);
        if(cachm[i].cachblock.request!=NULL &&strcmp(cachm[i].cachblock.request,buffer)==0 && cachm[i].cachblock.response[0]!='\0'){
            pthread_mutex_unlock(&lock);
            return cachm[i].cachblock.response;
        }
    }
    pthread_mutex_unlock(&lock);
    return NULL;
}


void proxyclientconnect(int proxyfdc,struct sockaddr_in * originserversockaddr, char* originserverdomainadd, char * filepath, char *httpreply){
    /*work as client to the original server*/

    if(connect(proxyfdc,(struct sockaddr *)originserversockaddr,sizeof(struct sockaddr_in))==-1)
        {printf("connect to original server fail,erro:%s\n",strerror(errno)); 
         return;
        }
    else{
        printf("connected to original server successfully\n");

        sendhttptoserver(proxyfdc,originserverdomainadd,filepath);

        //reply from originalserver
        char *tembuf=malloc(MAX_OBJECT_SIZE);
        rio_t *rp=malloc(MAX_OBJECT_SIZE);
        rio_readinitb(rp, proxyfdc); 
        while(1){
              //checking socket to see the reply of original server
              if(rio_readnb(rp,httpreply,MAX_OBJECT_SIZE)>0){ 
                //the rio_readn read all content from  socket in one run. Because its implementation is looping until EOF or reach required length
                //printf("this is server reply:%s\n",httpreply);
                break;
              };           
        }
        free(tembuf);
        free(rp);
    };   
    
}

void sendhttptoserver(int proxyfdc,char* originserverdomainadd, char * filepath){
        //struct the HTTP request
    char *httprequest=malloc(MAX_OBJECT_SIZE);
    memset(httprequest,0,sizeof httprequest);
   
    sprintf(httprequest,"GET %s HTTP/1.0\r\n",filepath);

    char *httpheader=malloc(MAX_OBJECT_SIZE);
    memset(httpheader,0,sizeof httpheader);
    sprintf(httpheader,"Host:%s\r\n",originserverdomainadd);
    strcat(httpheader,user_agent_hdr);
    strcat(httpheader,"\r\n");

    strcat(httprequest,httpheader);

     //printf("http request is :%s\n",httprequest);
        //send httprequest to the original server

        write(proxyfdc,httprequest,strlen(httprequest));
        free(httpheader);
        free(httprequest);


}


int parseproxyrequest(char *buffer, char* originalserverhostname,char* port,char *filepath){
        char method[MAX_OBJECT_SIZE],uri[MAX_OBJECT_SIZE],version[MAX_OBJECT_SIZE];
        
        int v= sscanf(buffer,"%s %s %s",method,uri,version);  
       /*sscanf method: "%s %s %s" tell sscanf how to interpret the buffer string. (%s stops reading at whilespace char)
                        Each %s is a string, and the space between the %s means that sscanf should skip over any space.
                        method,uri,version are pointers to the char arrays where sscanf will store the parsed strings*/
        printf("memthod:%s, uri:%s,version:%s\n",method,uri,version);

        if(strcasecmp(method,"GET")){
            printf("this method is not implemented\n");
            return -1;
        }
        if(!strstr(uri,"http://")/*&&!strstr(uri,"https://")*/){
            printf("This URI is not started with http:// \n");
            return -1;
        }

        
        //example of uri :   http://www.abcd/efg
        //find the server ip to connect, and identify the request static or dynamic
        if(!strstr(uri,"cgi-bin")){
            /* the uri doesn't contain "cgi-bin", require static content from server disk*/
            char *pt=strchr(uri+7,':');
            char *p=strchr(uri+7,'/'); //find the first occurence of '/' after https://
            char *endp=uri+strlen(uri);

            if(pt!=NULL){
                //port exits
                int hostnamelenth=pt-(uri+7);
                strncpy(originalserverhostname,uri+7,hostnamelenth);
                originalserverhostname[hostnamelenth]='\0';

                if(p!=NULL){
                    //filepath exist
                  
                    int portlenth=p-(pt+1);
                    strncpy(port,pt+1,portlenth);
                    port[portlenth]='\0'; //add null termination character

                    int filepathlenth=endp-p;
                    strncpy(filepath,p,filepathlenth);
                    filepath[filepathlenth]='\0';
                }else{
                    int portlenth=endp-(pt+1);
                    strncpy(port,pt+1,portlenth);
                    port[portlenth]='\0'; //add null termination character


                    strcpy(filepath,"/home.html\0");//use default filepath when no filepath declared
                }
            }else{
                strcpy(port,"80\0"); //default is "80"
                if(p!=NULL){
                    //filepath exist
                    int filepathlenth=endp-p;
                    strncpy(filepath,p,filepathlenth);
                    filepath[filepathlenth]='\0';

                    int hostnamelenth=p-(uri+7);
                    strncpy(originalserverhostname,uri+7,hostnamelenth);
                    originalserverhostname[hostnamelenth]='\0';
                }else{
                    strcpy(filepath,"/home.html\0");//use default filepath when no filepath declared
                    
                    int hostnamelenth=endp-(uri+7);
                    strncpy(originalserverhostname,uri+7,hostnamelenth);
                    originalserverhostname[hostnamelenth]='\0';
                }

            }
            
            
            printf("this is hostname:%s ,port:%s,and path:%s\n",originalserverhostname,port,filepath);
            return 0;
        }else{
            /* the uri contains "cgi-bin", require dynamic content from server calling a program with paramenters */
            return 1;
        }


}

