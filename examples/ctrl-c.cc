
//
// Example of how to ignore ctrl-c
//

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>

// 处理信号
void disp( int sig )
{
	fprintf( stderr, "\nsig:%d      Ouch!\n", sig);
}

int main()
{
	printf( "Type ctrl-c or \"exit\"\n");
    
	// 信号结构
    struct sigaction sa;
	// 添加处理者 (function pointer)
    sa.sa_handler = disp;
	// 清除信号面具(多个信号重叠), 防止被别的信号打断
    sigemptyset(&sa.sa_mask);
	// 默认flag (0), 会干扰system call, SA_RESTART不会出错
    sa.sa_flags = SA_RESTART;

	// sigaction
	// SIGINT: ctrl c 信号, sa 信号结构
    if(sigaction(SIGINT, &sa, NULL)){
		printf( "get in\n");
        perror("sigaction");
        exit(2);
    }

	// ;; 表示while true 无限循环
	for (;;) {
		
		char s[ 20 ];
		// 不断显示(通过fgets)
		printf( "prompt>");
		// 如果有\n, 自动flush
		fflush( stdout );
		// 读取input
		// ctrl c 导致return
		fgets( s, 20, stdin );

		s[19] = '\0';
		printf("s=%s\n", s);


		if ( !strcmp( s, "exit\n" ) ) {
			printf( "Bye!\n");
			exit( 1 );
		}
	}

	return 0;
}


