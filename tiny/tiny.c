/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
// void serve_static(int fd, char *filename, int filesize);
void serve_static(int fd, char *filename, int filesize, char *method);

void get_filetype(char *filename, char *filetype);
// void serve_dynamic(int fd, char *filename, char *cgiargs);
void serve_dynamic(int fd, char *filename, char *cgiargs, char *method);

void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);

void doit(int fd)   // fd : 연결 요청 후에 리턴받은 연결 식별자
{
  int is_static;
  struct stat sbuf;
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE];
  rio_t rio;    // rio_readlineb를 위해 rio_t 구조체의 읽기 버퍼를 선언

  /* Read request line and headers */
  // rio_readlineb : 텍스트 줄을 파일 rp에서부터 읽고 usrbuf로 복사, 읽은 텍스트 라인을 null로 바꾸고 종료. maxlen-1 만큼의 바이트를 읽고 나머지 텍스트는 잘러서 null문자로 종료
  Rio_readinitb(&rio, fd);      // 읽기 버퍼 초기화
  Rio_readlineb(&rio, buf, MAXLINE);    // 요청 라인을 읽고 분석, buf에 복사
  printf("Request headers:\n");
  printf("%s", buf);
  sscanf(buf, "%s %s %s", method, uri, version);    // 문자열에서 형식화된 데이터 읽어와서 각 변수에 맵핑

  // GET 요청 이외의 요청이 들어오면 함수 종료
  if (strcasecmp(method, "GET") && strcasecmp(method, "HEAD"))    // 대소문자 구별하지 않고 문자열 비교. 리턴 값이 0이면 문자열이 같다는 뜻. 같으면 if문을 거치치 않음
  {
    clienterror(fd, method, "501", "Not implemented", "Tiny does not implement this method");
    return;
  }
  read_requesthdrs(&rio);     // 다른 요청 헤더 무시

  /* Parse URI from GET request */
  is_static = parse_uri(uri, filename, cgiargs);    // 정적 컨텐츠인지 동적 컨텐츠인지 플래그 설정
  if (stat(filename, &sbuf) < 0)    // 파일이 디스크 상에 없는 경우 에러 리턴
  {
    clienterror(fd, filename, "404", "Not found", "Tiny couldn't find this file");
    return;
  }


  // S_ISREG : 정규파일이면 true
  // S_IRUSR : 소유자에게 읽기 권한이 있다

  if (is_static)  /* Serve static contant */
  {
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode))
    {
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't read the file");
      return;
    }
    serve_static(fd, filename, sbuf.st_size, method);   // 정적 컨텐츠를 클라이언트에 제공
  }

  // S_ISREG : 정규파일이면 true
  // S_IXUSR : 소유자에게 실행 권한이 있다

  else  /* Serve dynamic content */
  {
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode))
    {
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't run the CGI program");
      return;
    }
    serve_dynamic(fd, filename, cgiargs, method);
  }
}

// 서버에서 발생할 수 있는 에러 메시지의 폼 정의
// 명백한 오류에 대해 클라이언트에 보고. HTTP 응답을 응답 라인에 적절한 상태 코드와 상태 메시지와 함께 클라이언트로 전송
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
  char buf[MAXLINE], body[MAXBUF];

  /* Build the HTTP response body */
  // 브라우저 사용자에게 에러를 설명하는 응답 본체에 HTML도 함께 보냄
  // HTML 응답은 본체에서 컨텐츠의 크기와 타입을 나타내야 함. 
  // sprintf로 body에 계속해서 문자열 저장
  sprintf(body, "<html><title>Tiny Error</title>");
  sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);   // body에 문자열을 가산시키려면 파라미터로 넣으면서 \n을 해주는 수밖에 없나?
  sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

  /* Print the HTTP response */
  // buf에 문자열을 한 줄씩 추가하고 포인터를 당기면서 writen으로 전송
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
  Rio_writen(fd, buf, strlen(buf));     

  // 중첩시킨 html문자열 전송
  Rio_writen(fd, body, strlen(body));
}

// 다른 요청 헤더 무시
void read_requesthdrs(rio_t *rp)
{
  char buf[MAXLINE];

  // 한 줄씩 읽어가면서 모두 무시
  Rio_readlineb(rp, buf, MAXLINE);
  while (strcmp(buf, "\r\n"))
  {
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
  }
  return;
}

int parse_uri(char *uri, char *filename, char *cgiargs)
{
  char *ptr;

  if (!strstr(uri, "cgi-bin"))    /* Static content */
  {
    strcpy(cgiargs, "");
    strcpy(filename, ".");
    strcat(filename, uri);
    if (uri[strlen(uri)-1] == '/')
      strcat(filename, "home.html");
    return 1;
  }
  else    /* Dynamic content */
  {
    ptr = index(uri, '?');
    if (ptr) 
    {
      strcpy(cgiargs, ptr+1);
      *ptr = '\0';
    }
    else
      strcpy(cgiargs, ptr+1);
    strcpy(filename, ".");
    strcat(filename, uri);
    return 0;
  }
}

void serve_static(int fd, char *filename, int filesize, char *method)
{
  int srcfd;
  char *srcp, filetype[MAXLINE], buf[MAXBUF];

  /* Send response headers to client */
  // 헤더 정보를 바이트로 포맷. 반환값과 기능 중 하나를 포함하는 긴 문자열
  // 버퍼에 HTTP 문법에 맞게 헤더 출력
  get_filetype(filename, filetype);
  sprintf(buf, "HTTP/1.0 200 OK\r\n");    // \r : 커서를 현재 줄의 맨 앞으로 이동
  sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
  sprintf(buf, "%sConnection: close\r\n", buf);
  sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
  sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
  Rio_writen(fd, buf, strlen(buf));   // fd : 서버 입장에서 클라이언트와 연결된 소켓. fd에 지금까지 버퍼에 기록한 내용을 write
  printf("Response headers:\n");
  printf("%s", buf);

  if (strcasecmp(method, "HEAD") == 0)
    return;

  // /* Send response body to client */
  // srcfd = Open(filename, O_RDONLY, 0);
  // // Mmap: 파일이나 디바이스를 주소 공간 메모리에 대응. 
  // // 공간을 매핑하고자 하는 주소. 0은 디폴트, 주소 공간의 크기, 읽기 모드, 대응된 페이지 복사본 수정을 그 프로세스에만 보이게 할 것인지?, 파일 디스크럽터, 매핑하고자 하는 물리 주소
  // srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);   
  // Close(srcfd);   // 파일을 메모리에 맵핑한 후에는 식별자가 필요없음.
  // Rio_writen(fd, srcp, filesize);
  // Munmap(srcp, filesize);   // mmap() 함수로 할당된 메모리 영역 해제

  /* Send response body to client */
  // mmap을 malloc으로 구현
  srcfd = Open(filename, O_RDONLY, 0);
  // Mmap: 파일이나 디바이스를 주소 공간 메모리에 대응. 
  // 공간을 매핑하고자 하는 주소. 0은 디폴트, 주소 공간의 크기, 읽기 모드, 대응된 페이지 복사본 수정을 그 프로세스에만 보이게 할 것인지?, 파일 디스크럽터, 매핑하고자 하는 물리 주소
  srcp = malloc(filesize);
  Rio_readn(srcfd, srcp, filesize);
  Close(srcfd);   // 파일을 메모리에 맵핑한 후에는 식별자가 필요없음.
  Rio_writen(fd, srcp, filesize);
  free(srcp);
}

/*
 * get_filetype - Derive file type from filename
 */
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

  // 퀴즈 11.7
  else if (strstr(filename, ".mpg"))
    strcpy(filetype, "video/mpg");

  else
    strcpy(filetype, "text/plain");
}

void serve_dynamic(int fd, char *filename, char *cgiargs, char* method)
{
  char buf[MAXLINE], *emptylist[] = { NULL };

  /* Return first part of HTTP response */
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  Rio_writen(fd, buf, strlen(buf));   // 버퍼의 길이만큼 fd에 write
  sprintf(buf, "Server: Tiny Web Server\r\n");
  Rio_writen(fd, buf, strlen(buf));

  if (strcasecmp(method, "HEAD") == 0)
    return;

  if (Fork() == 0)    /* Child */     // serve_dynamin이 부모 프로세스, 아래 코드들이 자식 프로세스가 됨. 부모 프로세스에는 return pid, 자식 프로세스에는 return 0
  {
    /* Real server would set all CGI vars here */
    setenv("QUERY_STRING", cgiargs, 1);     // "QUERY_STRING" 이라는 환경 변수 추가 
    Dup2(fd, STDOUT_FILENO);                /* Redirect stdout to client */     // fd : 클라이언트와 서버의 소켓 커넥션. 표준 출력을 클라이언트와 연관된 연결식별자로 재지정. CGI프로그램의 모든 표준 출력은 클라이언트로 향함.
    Execve(filename, emptylist, environ);   /* Run CGI programs */    // 주소의 adder?15000&213  에서 15000&213 부분 참조
  }
  Wait(NULL); /* Parent waits for and reaps child */
}

int main(int argc, char **argv)   // ./tiny {포트번호} 로 실행했으니 argv[0] = tiny.exe, argv[1] = 포트번호
{
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  /* Check command line args */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  // Open_listenfd 함수를 호출해서 듣기 소켓 오픈. 인자로 포트 번호를 넘겨줌
  // listenfd에 듣기 식별자 리턴
  listenfd = Open_listenfd(argv[1]);

  // 요청 받는 무한 루프
  while (1) {
    clientlen = sizeof(clientaddr);   // 클라이언트 주소 길이
    connfd = Accept(listenfd, (SA *)&clientaddr,
                    &clientlen);  // line:netp:tiny:accept      // 듣기 식별자, 소켓 주소 구조체의 주소, 주소 길이를 파라미터로 입력. 연결 요청 접수
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE,
                0);   // 소켓 구조체를 호스트의 서비스들(문자열)로 변환
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    doit(connfd);   // line:netp:tiny:doit      // 트랜잭션 수행
    Close(connfd);  // line:netp:tiny:close     // 연결 끝. 소켓 닫기.
  }
}
