#include "csapp.h"

void doit(int fd);
int read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize, char *method);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs, char *method);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);

// void doit(int fd)
// {
//     rio_t rio;
//     int is_static;
//     struct stat statbuf;
//     char buf[MAXLINE];
//     char method[MAXLINE], uri[MAXLINE], version[MAXLINE];
//     char filename[MAXLINE], cgiarg[MAXLINE];

//     rio_readinitb(&rio, fd);
//     rio_readlineb(&rio, buf, MAXLINE);
//     printf("Request headers:\n");
//     printf("%s\n", buf);
//     // rio_writen(fd, buf, strlen(buf));
//     if (sscanf(buf, "%s %s %s", method, uri, version) < 3)
//     {
//         clienterror(fd, buf, "400", "Bad Request", "Server received a malformed request");
//         return;
//     }
//     if (strcasecmp(method, "GET") && strcasecmp(method, "HEAD") && strcasecmp(method, "POST"))
//     {
//         clienterror(fd, method, "501", "Not Implemented", "Server has not implement this method yet");
//         return;
//     }
//     read_requesthdrs(&rio);
//     is_static = parse_uri(uri, filename, cgiarg);

//     if (strcasecmp(method, "POST") == 0)
//     {
//         rio_readlineb(&rio, cgiarg, MAXLINE);
//     }

//     if (stat(filename, &statbuf) < 0)
//     {
//         clienterror(fd, filename, "404", "Not found", "Required file cannot be found");
//         return;
//     }

//     if (is_static)
//     {
//         if (!(S_ISREG(statbuf.st_mode)) || !(S_IRUSR & statbuf.st_mode))
//         {
//             clienterror(fd, filename, "403", "Forbidden", "Required file cannot be read");
//             return;
//         }
//         serve_static(fd, filename, statbuf.st_size, method);
//     }
//     else
//     {
//         if (!(S_ISREG(statbuf.st_mode)) || !(S_IRUSR & statbuf.st_mode))
//         {

//             clienterror(fd, filename, "403", "Forbidden", "Required CGI program cannot be run");
//             return;
//         }
//         serve_dynamic(fd, filename, cgiarg, method);
//     }
// }
void doit(int fd)
{
    rio_t rio;
    int is_static;
    struct stat statbuf;
    char buf[MAXLINE];
    char method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char filename[MAXLINE], cgiarg[MAXLINE] = {0}; // 初始化清零

    rio_readinitb(&rio, fd);
    if (rio_readlineb(&rio, buf, MAXLINE) <= 0)
        return;

    printf("Request headers:\n");
    printf("%s", buf);

    if (sscanf(buf, "%s %s %s", method, uri, version) < 3)
    {
        clienterror(fd, buf, "400", "Bad Request", "Server received a malformed request");
        return;
    }
    if (strcasecmp(method, "GET") && strcasecmp(method, "HEAD") && strcasecmp(method, "POST"))
    {
        clienterror(fd, method, "501", "Not Implemented", "Server has not implement this method yet");
        return;
    }

    // 1. 获取 Header 中的 Content-Length
    int content_length = read_requesthdrs(&rio);

    is_static = parse_uri(uri, filename, cgiarg);

    // 2. 如果是 POST，使用 rio_readnb 精确读取 content_length 字节
    if (strcasecmp(method, "POST") == 0)
    {
        if (content_length > 0 && content_length < MAXLINE)
        {
            rio_readnb(&rio, cgiarg, content_length); // 精确读取 content_length 字节
            cgiarg[content_length] = '\0';            // 手动追加 null 终止符
            printf("POST body (content_length=%d): [%s]\n", content_length, cgiarg); // 调试：打印收到的正文
        }
        else
        {
            printf("POST body: content_length=%d, 未读取正文\n", content_length); // 调试：无正文可读
        }
    }

    if (stat(filename, &statbuf) < 0)
    {
        clienterror(fd, filename, "404", "Not found", "Required file cannot be found");
        return;
    }

    if (is_static)
    {
        if (!(S_ISREG(statbuf.st_mode)) || !(S_IRUSR & statbuf.st_mode))
        {
            clienterror(fd, filename, "403", "Forbidden", "Required file cannot be read");
            return;
        }
        serve_static(fd, filename, statbuf.st_size, method);
    }
    else
    {
        if (!(S_ISREG(statbuf.st_mode)) || !(S_IRUSR & statbuf.st_mode))
        {
            clienterror(fd, filename, "403", "Forbidden", "Required CGI program cannot be run");
            return;
        }
        serve_dynamic(fd, filename, cgiarg, method);
    }
}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
    char buf[MAXLINE], body[MAXLINE];
    // sprintf with a offset "len", as strcat does not accept placeholders like "%s" while something like "sprintf(buf, "%s %s", buf, uri)" is undefined
    int len = 0;
    len += sprintf(body + len, "<html><title>Server Error</title>");
    len += sprintf(body + len, "<body bgcolor=\"ffffff\">\r\n");
    len += sprintf(body + len, "%s: %s\r\n", errnum, shortmsg);
    len += sprintf(body + len, "<p>%s: %s\r\n", longmsg, cause);
    len += sprintf(body + len, "<hr><em>The Server Web server</em>\r\n");

    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Connection: close\r\n");
    Rio_writen(fd, buf, strlen(buf));
    // although the buffer zone won't be flushed, sprintf will attach a \0 at the end and strlen(buf) will only count to \0
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}

int read_requesthdrs(rio_t *rp)
{
    char buf[MAXLINE];
    int content_length = 0;

    rio_readlineb(rp, buf, MAXLINE);

    while (strcmp(buf, "\r\n") && strcmp(buf, "\n"))
    {
        printf("%s", buf);
        // 解析 Content-Length Header
        if (strncasecmp(buf, "Content-Length:", 15) == 0)
        {
            content_length = atoi(buf + 15);
        }
        rio_readlineb(rp, buf, MAXLINE);
    }
    return content_length; // 返回 Body 长度
}

int parse_uri(char *uri, char *filename, char *cgiargs)
{
    char *ptr;
    if (!strstr(uri, "cgi-bin"))
    {                          // static content, as all dynamic content should be inside the folder "cgi-bin"
        strcpy(cgiargs, "");   // erasing cgiargs, as client requests a static content (but there's nothing inside anyway)
        strcpy(filename, "."); // same for filename
        strcat(filename, uri);
        if (filename[strlen(filename) - 1] == '/')
            strcat(filename, "home.html");
        return 1; // return 1 if its static
    }
    else
    {
        ptr = strchr(uri, '?'); // return the pointer to the required char
        if (ptr)
        {
            strcpy(cgiargs, ptr + 1); // the next char after '?'
            *ptr = '\0';              // uri: from "/cgi-bin/adder?150&120" to "/cgi-bin/adder \0 150&120", cutting the uri string. the arguments part "150&120" will not be read again due to the '\0'
        }
        else
        {
            strcpy(cgiargs, "");
        }
        strcpy(filename, ".");
        strcat(filename, uri);
        return 0; // return 0 it its dynamic
    }
}

void serve_static(int fd, char *filename, int filesize, char *method)
{
    int srcfd;
    char *srcp, filetype[MAXLINE], buf[MAXBUF];
    get_filetype(filename, filetype);
    int ofs = 0;
    ofs += sprintf(buf + ofs, "HTTP/1.0 200 OK\r\n");
    ofs += sprintf(buf + ofs, "Server: Server\r\n");
    ofs += sprintf(buf + ofs, "Connection: close\r\n");
    ofs += sprintf(buf + ofs, "Content-length: %d\r\n", filesize);
    ofs += sprintf(buf + ofs, "Content-type: %s\r\n\r\n", filetype); // two consecutive \r\n marks the ending of the headers
    rio_writen(fd, buf, strlen(buf));

    printf("Response headers:\n");
    printf("%s", buf);
    if (strcasecmp(method, "HEAD") == 0)
        return;

    srcfd = open(filename, O_RDONLY, 0);
    // srcp = mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
    // Q11.9: read the static file with malloc and rio_readn
    srcp = malloc(filesize);
    rio_readn(srcfd, srcp, filesize);
    close(srcfd);
    rio_writen(fd, srcp, filesize);
    free(srcp);
    // munmap(srcp, filesize);
}

void get_filetype(char *filename, char *filetype)
{
    if (strstr(filename, ".html"))
        strcpy(filetype, "text/html");
    else if (strstr(filename, ".gif"))
        strcpy(filetype, "image/gif");
    else if (strstr(filename, ".png"))
        strcpy(filetype, "image/png");
    else if (strstr(filename, ".jpg"))
        strcpy(filetype, "image/jpeg");
    else if (strstr(filename, ".mpg") || strstr(filename, "mp4"))
        strcpy(filetype, "video/mpeg");
    else
        strcpy(filetype, "text/plain");
}

void serve_dynamic(int fd, char *filename, char *cgiargs, char *method)
{
    char buf[MAXLINE];
    // 正确初始化 argv，使 argv[0] 为可执行文件名
    char *cgiargv[] = {filename, NULL};

    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Server: Server\r\n");
    Rio_writen(fd, buf, strlen(buf));

    if (fork() == 0)
    {
        setenv("QUERY_STRING", cgiargs, 1);
        setenv("REQUEST_METHOD", method, 1);
        dup2(fd, STDOUT_FILENO);            // 重定向标准输出到套接字
        execve(filename, cgiargv, environ); // 传入 cgiargv
    }
    wait(NULL); // 回收子进程，避免僵尸进程
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        unix_error("format: ./server <port>");
        exit(0);
    }
    int listenfd, connfd;
    char host[MAXLINE];
    char port[MAXLINE];
    struct sockaddr_storage cliaddr;
    socklen_t clilen;

    listenfd = open_listenfd(atoi(argv[1]));
    while (1)
    {
        clilen = sizeof(struct sockaddr_storage);
        connfd = accept(listenfd, (SA *)&cliaddr, &clilen);
        getnameinfo((SA *)&cliaddr, clilen, host, MAXLINE, port, MAXLINE, 0);
        printf("Connected to: %s:%s\n", host, port);
        doit(connfd);
        close(connfd);
    }

    exit(0);
}
