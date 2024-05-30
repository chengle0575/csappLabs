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

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";



//prototype
void proxyserverinit(char *,uint16_t);
void proxyclientinit(struct sockaddr_in *, char* ,char * ,char*);
int parseproxyrequest(char *,char*,char*,char*);

int main(int argc,char * argv[])
{
    
    printf("%s", user_agent_hdr);
     
    char buffer[MAX_OBJECT_SIZE];
    
    uint16_t port=(uint16_t)atoi(argv[1]);
    //printf("port is %d\n",port);

    proxyserverinit(buffer,port);
    //proxyclientinit("127.0.0.1",(uint16_t )80,"/");
  

    return 0;
}




void proxyserverinit(char *buffer,uint16_t port){
    //work as server to the original client

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

        
             //read buffer
             read(connectfd,buffer,MAX_CACHE_SIZE); //read from file to memory
             printf("server recived:%s\n",buffer);

             //paser the request, decide whether a valid HTTP request
             char* originalserverhostname=malloc(MAX_OBJECT_SIZE);
             char *filepath=malloc(MAX_OBJECT_SIZE);
             char port[10];
             
             
             if(parseproxyrequest(buffer,originalserverhostname,port,filepath)!=-1){
                
                    //conver originalserver's hostname into socket address
                    struct addrinfo *res;
                    struct addrinfo hints;
                    memset(&hints,0,sizeof(hints));
                    hints.ai_family=AF_INET; //only remains those ip using ipv4
                    hints.ai_socktype=SOCK_STREAM; //only remmain those socket of STREAM type

                    if(getaddrinfo(originalserverhostname,port,&hints,&res)!=0){printf("getaddrinfo error\n");};
                    
                    struct sockaddr_in *originserversockaddr=(struct sockaddr_in *)res->ai_addr;
    
                    freeaddrinfo(res); //the getaddrinfo allocate mwmory in heap for res, so need to fress manully
                   

                     char httpreply[MAX_OBJECT_SIZE];

                    proxyclientinit(originserversockaddr,originalserverhostname,filepath,httpreply);


                    //write to connectfd reply to original client
                    write(connectfd,httpreply,strlen(httpreply));
                    printf("reply to original server\n");
             
             };


            free(originalserverhostname);
            free(filepath);
        
        
       

    }

}


void 



void proxyclientinit(struct sockaddr_in * originserversockaddr, char* originserverdomainadd, char * filepath, char *httpreply){
    /*work as client to the original server*/
    
    //create a intenet socket
    int proxyfdc=socket(AF_INET,SOCK_STREAM,0);
    if(proxyfdc==-1){printf("create proxy client fd failed\n");}


    //struct the HTTP request
    char *httprequest=malloc(MAX_OBJECT_SIZE);
    memset(httprequest,0,sizeof httprequest);
    printf("in proxyclient,filepath:%s\n",filepath);
    sprintf(httprequest,"GET %s HTTP/1.0\r\n",filepath);

    char *httpheader=malloc(MAX_OBJECT_SIZE);
    memset(httpheader,0,sizeof httpheader);
    sprintf(httpheader,"Host:%s\r\n",originserverdomainadd);
    strcat(httpheader,user_agent_hdr);
    strcat(httpheader,"\r\n");

    strcat(httprequest,httpheader);


    if(connect(proxyfdc,(struct sockaddr *)originserversockaddr,sizeof(struct sockaddr_in))==-1)
        {printf("connect to original server fail,erro:%s\n",strerror(errno)); 
         return;
        }
    else{
        printf("connected to original server successfully\n");

        //send httprequest to the original server

        write(proxyfdc,httprequest,strlen(httprequest));
        free(httpheader);
        free(httprequest);

        //reply from originalserver

        while(1){
              //checking socket to see the reply of original server
              if(read(proxyfdc,httpreply,MAX_OBJECT_SIZE)>0){
                printf("this is server reply:%s\n",httpreply);
                break;
              };
        }
    
    };   
    
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
                strncpy(originalserverhostname,uri+7,pt-(uri+7));

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
                    originalserverhostname[hostnamelenth]='0';
                }else{
                    strcpy(filepath,"/home.html\0");//use default filepath when no filepath declared
                    
                    int hostnamelenth=endp-(uri+7);
                    strncpy(originalserverhostname,uri+7,hostnamelenth);
                    originalserverhostname[hostnamelenth]='0';
                }

            }
            
            
            printf("this is hostname:%s ,port:%s,and path:%s\n",originalserverhostname,port,filepath);
            return 0;
        }else{
            /* the uri contains "cgi-bin", require dynamic content from server calling a program with paramenters */
            return 1;
        }


}

