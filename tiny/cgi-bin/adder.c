/*
 * adder.c - a minimal CGI program that adds two numbers together
 */
/* $begin adder */
#include "csapp.h"

int main(void) {
  char *buf, *p;
  char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE];
  int n1 = 0, n2 = 0;

  /* Extract the two arguments */
  if ((buf = getenv("QUERY_STRING")) != NULL)   // 환경 변수 리스트에서 QUERY_STRING을 탐색하고 찾으면 그 포인터 리턴
  // 만약 수를 3과 5를 입력했다고 가정하면,
  // buf : first=3&second=5     의 형태가 됨.
  {
    p = strchr(buf, '&');   // buf 문자열에서 '&'이 존재하는 위치(포인터) 리턴
    *p = '\0';          // \0은 문자열의 끝을 의미
    // strcpy(arg1, buf);
    // strcpy(arg2, p+1);
    // n1 = atoi(arg1);    // 문자열을 정수로
    // n2 = atoi(arg2);

    // 11.10 
    sscanf(buf, "first=%d", &n1);   // buf 위치에서 first= 다음 위치의 정수를 n1에 입력
    sscanf(p+1, "second=%d", &n2);  // & 다음 위치의 second= 다음 위치의 정수를 n2에 입력
  }

  /* Make the response body */
  sprintf(content, "QUERY_STRING=%s", buf);
  sprintf(content, "Welcome to add.com: ");
  sprintf(content, "%sTHE Internet addition portal. \r\n<p>", content);
  sprintf(content, "%sThe answer is: %d + %d = %d\r\n<p>", content, n1, n2, n1 + n2);
  sprintf(content, "%sThanks for visiting!\r\n", content);

  /* Generate the HTTP response */
  printf("Connection: close\r\n");
  printf("Content-length: %d\r\n", (int)strlen(content));
  printf("Content-type: text/html\r\n\r\n");
  printf("%s", content);
  fflush(stdout);   // 출력 스트림 버퍼에 남아있는 내용을 스트림에 출력. serve_dynamic에서 Dup2로 출력을 클라이언트로 설정해놓았으므로 클라이언트에 출력

  exit(0);
}
/* $end adder */
