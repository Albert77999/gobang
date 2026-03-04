#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define BUF_SIZE 1024
#define SMALL_BUF 100

void error_handing(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

void *request_handler(void *arg);
void send_data(FILE *fp, char *ct, char *file_name);
char *content_type(char *file);
void send_error(FILE *fp);

int main(int argc, char *argv[])
{
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_adr, clnt_adr;
    socklen_t adr_sz = sizeof(clnt_adr);
    char buf[BUF_SIZE];
    if (argc != 2)
    {
        error_handing("usage error()");
    }

    serv_sock = socket(AF_INET, SOCK_STREAM, 0);
    memset(&serv_adr,0,sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[1]));
    if (serv_sock < 0)
        error_handing("socket error()");

    if (bind(serv_sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1)
        error_handing("bind error()");

    if (listen(serv_sock, 20) == -1)
        error_handing("listen error()");

    pthread_t tid;

    while (1)
    {
        clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_adr, &adr_sz);
        printf("Connection Request : %s:%d\n", inet_ntoa(clnt_adr.sin_addr), ntohs(clnt_adr.sin_port));
        int *t_sock = malloc(sizeof(int));
        *t_sock = clnt_sock;
        pthread_create(&tid, NULL, request_handler, t_sock);
        pthread_detach(tid);
    }
    close(serv_sock);
    return 0;
}


//GET /index.html / HTTP/1.1\r\n
void *request_handler(void *arg)
{
    int clnt_sock = *(int*)arg;
    free(arg);
    char req_line[SMALL_BUF];

    char method[10];
    char ct[15];
    char file_name[30];

    FILE *clnt_read = fdopen(clnt_sock,"r");
    FILE *clnt_write = fdopen(dup(clnt_sock),"w");

    if(fgets(req_line,SMALL_BUF,clnt_read) == NULL)
    {
        send_error(clnt_write);
        fclose(clnt_read);
        fclose(clnt_write);
        return NULL;
    }
    if(strstr(req_line,"HTTP/") == NULL)
    {
        send_error(clnt_write);
        fclose(clnt_read);
        fclose(clnt_write);
        return NULL;
    }
    strcpy(method,strtok(req_line," /"));
    strcpy(file_name,strtok(NULL," /"));
    strcpy(ct,content_type(file_name));

    fclose(clnt_read);
    send_data(clnt_write,ct,file_name);
    return NULL;
}
// < HTTP/1.0 200 OK
// < Server:Linux Web Server 
// < Content-length:9835
// < Content-type:text/html
void send_data(FILE *fp, char *ct, char *file_name)
{
    char protocol[] = "HTTP/1.0 200 OK \r\n";
    char server[] = "Server:Linux Web Server \r\n";
    char cnt_len[SMALL_BUF];
    char cnt_type[SMALL_BUF];
    char buf[BUF_SIZE];

    sprintf(cnt_type,"Content-type:%s\r\n\r\n",ct);
    FILE *send_file = fopen(file_name,"rb");
    fseek(send_file,0,SEEK_END);
    int size = ftell(send_file);
    rewind(send_file);
    sprintf(cnt_len,"Content-length:%d\r\n",size);

    fputs(protocol,fp);
    fputs(server,fp);
    fputs(cnt_len,fp);
    fputs(cnt_type,fp);

    int read_cnt;
    while((read_cnt = fread(buf,1,BUF_SIZE,send_file)) > 0 )
    {
        fwrite(buf,1,read_cnt,fp);
        fflush(fp);
    }
    fflush(fp);
    fclose(fp);
    fclose(send_file);
}
char *content_type(char *file)
{
    char extension[SMALL_BUF];
    char file_name[SMALL_BUF];
    strcpy(file_name,file);
    strtok(file_name,".");
    strcpy(extension,strtok(NULL,"."));

    if(!strcmp(extension,"html") || !strcmp(extension,"htm"))
    {
        return "text/html";
    }
    else if(!strcmp(extension,"jpg"))
        return "image/jepg";
    else 
        return "text/plain";
}
void send_error(FILE *fp)
{
    char protocol[] = "HTTP/1.0 404 Not Found \r\n";
    char server[] = "Server:Linux Web Server \r\n";
    char cnt_len[SMALL_BUF];
    char cnt_type[] = "Content-type:text/html \r\n\r\n";
    char buf[BUF_SIZE];

    FILE *send_file = fopen("404.html","rb");
    if(send_file == NULL) return;

    fseek(send_file,0,SEEK_END);
    int size = ftell(send_file);
    fseek(send_file,0,SEEK_SET);
    sprintf(cnt_len,"Content-length:%d\r\n",size);

    fputs(protocol,fp);
    fputs(server,fp);
    fputs(cnt_len,fp);
    fputs(cnt_type,fp);

    int read_cnt;
    while((read_cnt = fread(buf,1,BUF_SIZE,send_file)) > 0)
    {
        fwrite(buf,1,read_cnt,fp);
        fflush(fp);
    }
    fflush(fp);
    fclose(fp);
    fclose(send_file);
}