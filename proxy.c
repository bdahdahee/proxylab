
/*Behzad Dah Dahee*/
/*bvd5281@psu.edu*/



#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "csapp.h"
#include "cache.h"
/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
void doit(int fd, CacheList *cache);
int parse_url(const char *url, char *host, char *port, char *path);
int main(int argc, char **argv) 
{
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;
  CacheList cache_store;

  /* Check command line args */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  cache_init(&cache_store);
  Signal(SIGPIPE, SIG_IGN);
  listenfd = Open_listenfd(argv[1]);
  while (1) {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); //line:netp:tiny:accept
    Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, 
        port, MAXLINE, 0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    doit(connfd, &cache_store);                               //line:netp:tiny:doit
    Close(connfd);                                            //line:netp:tiny:close
  }
}




void doit(int fd, CacheList *cache) 
{

  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE], host[MAXLINE], port[MAXLINE], path[MAXLINE], content_buffer[MAXLINE];
  char requestname[MAXLINE], requestheaders[MAXLINE] = "", userbuf[MAXLINE], serverheaders[MAXLINE] = "", content_size_str[MAXLINE]="";
  rio_t rio, rio_server;
  int i, serverfd, content_amount, hostcount = 0, connectioncount = 0, server_okay = 0, contentl = 0, content_size = 0, memcpyoffet = 0;
  void* cache_buffer = malloc(MAX_OBJECT_SIZE);
  CachedItem* item;


  /* Read request line and headers */
  rio_readinitb(&rio, fd); 
  if (!rio_readlineb(&rio, buf, MAXLINE)){  
    return;
  }
  sscanf(buf, "%s %s %s", method, uri, version);      

  /*if it is not a GET request or not in proper format do it returns*/
  if (strcasecmp(method, "GET")) {                  
    return;
  }
  if (parse_url(uri,host,port,path) == 0){
    return;
  }

  /* item is assigned the website's data if the requested website is in the cache and if website is stored in the cache, 
  it's content are retrieved without connecting to the server
  */
  item = find(uri,cache);
  if (item != NULL){
    printf("cache\n");
    rio_writen(fd,item->headers,strlen(item->headers));
    rio_writen(fd, item->item_p, item->size);
    return;
  } 


  /*the rest of the code runs if the website is not in the cache.*/
  /*the GET request is stored as the first line of the requestheaders that is going to be sent to the server*/
  strcat(requestheaders,"GET ");
  strcat(requestheaders,path);
  strcat(requestheaders, " HTTP/1.0\r\n");

  /* the while loop goes through all of the headers sent from the client and using if statements it modifies and of requestheaders
  * that need to be changed such as connection, proxy-connection etc
  */
  while(rio_readlineb(&rio, buf, MAXLINE)>2){
    requestname[0] = '\0';
    i = 0;
    while (buf[i] != ':'){
      i+= 1;
    }
    strncpy(requestname, buf, i);
    if(strcasecmp(requestname, "host") == 0){
      hostcount += 1;
      strcat(requestheaders,buf);
    }else{
      if(strcasecmp(requestname,"connection") == 0){
        strcat(requestheaders,"Connection: close\r\n");
        connectioncount+=1;
      }else{
        if(strcasecmp(requestname,"proxy-connection") == 0){
          strcat(requestheaders,"Proxy-Connection: close\r\n");
        }else{
          if(strcasecmp(requestname,"user-agent") == 0){
            strcat(requestheaders,user_agent_hdr);
          }else{
            if((strcasecmp(requestname,"if-modified-since")) || (strcasecmp(requestname,"if-none-match"))){
              strcat(requestheaders,buf);
            }
          }
        }
      }
    }

  }
  /*these two if statements add the connection and host header if they are missing */
  if (connectioncount == 0){
    strcat(requestheaders,"Connection: close\r\n");
  }
  if (hostcount == 0){
    strcat(requestheaders,"Host: ");
    strcat(requestheaders,host);
    strcat(requestheaders,"\r\n");
  }
  /*\r\n signifies that the request headers are done */
  strcat(requestheaders, "\r\n");


  /*a connection is made to the server using open_clientfd and the modified request headers are sent*/
  serverfd = open_clientfd(host,port);
  rio_writen(serverfd,requestheaders,strlen(requestheaders));
  rio_readinitb(&rio_server, serverfd);
  
  /*this while loop goes through the server response headers and it writes them to userbuf, while doing that it also checks
  * if the response headers include http/1.0 200 ok and content-length for caching purposes
  */
  while(rio_readlineb(&rio_server, userbuf, MAXLINE)>2){
    if (strncasecmp(userbuf,"http/1.0 200 ok",15) == 0){
      server_okay = 1;
    }
    if (strncasecmp(userbuf,"Content-Length", 14) == 0){
      strcat(content_size_str,userbuf+15);
      contentl = 1;
    }
    strcat(serverheaders,userbuf);
    rio_writen(fd,userbuf,strlen(userbuf));
  }
  strcat(serverheaders,"\r\n");
  rio_writen(fd,"\r\n",2);
  if (contentl == 1){
    sscanf(content_size_str, "%d", &content_size);
  }


  /* this while loop reads the binary content of the page and stores it in the content_buffer */
  while((content_amount=rio_readnb(&rio_server,content_buffer,MAXLINE))>0){
  /* if the contentlength header and the http/1.0 200 okay and the content size is less than the max available space then
  *it is copied into the cache_buffer. The memcpyoffset stores the amount of bytes read by each iteration to avoid overwriting
  *previous content
  */
  if((contentl == 1) && (server_okay == 1)){
    if (content_size <= MAX_OBJECT_SIZE){
      memcpy(cache_buffer + memcpyoffet ,content_buffer,content_amount);
      memcpyoffet += content_amount;
    }
  }
  /* the binary content is sent to the client */
    rio_writen(fd, content_buffer,content_amount);
  }

  /* once again the cache requirements are checked and if they are met, the cache_buffer content is saved to the cache under the uri*/
  if((contentl == 1) && (server_okay == 1)){
    if (content_size <= MAX_OBJECT_SIZE){
      content_size = (size_t)content_size;
      cache_URL(uri,serverheaders,cache_buffer,content_size, cache);
      printf("cache save\n");
    }
  }

  /*the connection to the server is closed*/
  close(serverfd);
  return;
}


// returns 1 if the url starts with http:// (case insensitive)
// and returns 0 otherwise
// If the url is a proper http scheme url, parse out the three
// components of the url: host, port and path and copy
// the parsed strings to the supplied destinations.
// Remember to terminate your strings with 0, the null terminator.
// If the port is missing in the url, copy "80" to port.
// If the path is missing in the url, copy "/" to path.
int parse_url(const char *url, char *host, char *port, char *path) { 
  int i=0;
  int portindex = 0;
  int pathindex = 0;
  char* nurl = malloc(MAXLINE);
  char* http = malloc(MAXLINE);
  //The decelerations above sets the indexes for the overall url (i) and the indexes for port annd path to zero. While
  //allocating memory for the http section of the url and the rest of the url (nurl)
  host[0] = '\0';
  path[0] = '\0';
  port[0] = '\0';
  strncpy(http, url, 7);
  //The host, port, and path strings are set to empty strings
  while (http[i] != '\0'){
    http[i] = tolower(http[i]);
    i += 1;  
  }
  i = 0;
  //the http section of the url is converted to lowercase in the while loop to allow the case insensitive comparison
  //below
  if (strcmp(http,"http://")==0){
    free(http);
    strcpy(nurl,url+7);
    while (nurl[i] != ':' && nurl[i] != '/' && nurl[i] != '\0') {
      host[i] = nurl[i];
      i += 1;      
    }
    //The while loop goes through the chracters of the host until it runs into a character that signifies end of the
    //host
    host[i] = '\0';
    if (nurl[i] == '\0' || (nurl[i] == '/' && nurl[i + 1] == '\0')){
      port[0] = '8';
      port[1] = '0';
      port[2] = '\0';
      path[0] = '/';
      path[1] = '\0';
      free(nurl);
      return (1);
    //above if statement checks if the url ends after the host, if so the port and path are set to "80" and "/", the
    //memory is freed and 1 is returned    
    }else{
      if(nurl[i] == ':'){
        i += 1;
        while(nurl[i] != '/' && nurl[i] != '\0'){
          port[portindex] = nurl[i];
          i += 1;
          portindex += 1;
        }
        //if the port comes after the host, the while loop goes through the port until the end of url or start of path
        port[portindex] = '\0';
        if (nurl[i] == '\0'){
          path[0] = '/';
          path[1] = '\0';
          free(nurl);
          return(1);
        }
        if (nurl[i + 1] == '\0'){
          path[0] = '/';
          path[1] = '\0';
          free(nurl);  
          return(1);
        }
        //if the url ends after the port the path is set to be "/" and the memory is freed and 1 is returned
      }
      while(nurl[i] != '\0'){
        path[pathindex] = nurl[i];
        i += 1;
        pathindex += 1;
      }
      path[pathindex] = '\0';
      //if the path comes after the port the while loop goes through the path characters until the end of url
      if (port[0] == '\0'){
        port[0] = '8';
        port[1] = '0';
        port[2] = '\0';
      }
      //if the port is still empty at this point of the code then there is no port in the url and it is set to be 80
      free(nurl);
      return(1);
      // memory is freed and 1 is returned      
    } 
  }
  // if the url does not start with the http:// 0 is returned 
  free(http);
  free(nurl);
  return 0;
}
