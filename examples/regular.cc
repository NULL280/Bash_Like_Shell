// regular expression 头
#include <sys/types.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>

const char * usage = ""
"Usage:\n"
"      regular regular-expresion string\n"
"\n"
"      Tells if \"string\" matches \"regular-expresion\".\n"
"\n"
"      '^' and '$' characters are added at the beginning and\n"
"      end of \"regular-expresion\" to force match of the entire\n"
"      \"string\"\n"
"\n"
"	To know more about regular expresions type \"man ed\"\n"
"Try:\n"
"      bash> ./regular aaa aaa\n"
"      bash> ./regular \"a*\" aaaa\n"
"      bash> ./regular \"a*\" aaa\n"
"      bash> ./regular \"a*\" aaaf\n"
"      bash> ./regular \"a.*\" akjhkljh \n"
"      bash> ./regular \"a.*\" jkjhkj\n"
"      bash> ./regular \"a.*\" aaalklkjlk\n"
"      bash> ./regular \".*\\..*\" kljhkljhlj.lkjhlkj\n"
"      bash> ./regular \".*\\..*\" kljhkljhlj\n\n";

int
main(int argc, char ** argv)
{
	// arg数量太少, 提示如何使用
	if ( argc < 3 ) {
		fprintf( stderr, "%s", usage );
		exit( -1 );
	}

	// RE表达, 对比string
	const char * regularExp = argv[1];
	const char * stringToMatch = argv[2];

	/*
	 *  Add ^ and $ at the beginning and end of regular expression
	 *  to force match of the entire string. See "man ed".
	 */

	
	char regExpComplete[ 1024 ];
	// 添加^和$
	sprintf(regExpComplete, "^%s$", regularExp );

	// buffer, 存储创建的FSA有限状态机
	regex_t re;
	// 编译
	int result = regcomp( &re, regExpComplete,  REG_EXTENDED|REG_NOSUB);

	// 如果出现error
	if( result != 0 ) {
		fprintf( stderr, "%s: Bad regular expresion \"%s\"\n",
			 argv[ 0 ], regExpComplete );
		exit( -1 );
      	}

	// 评估输入string是否匹配RE
	regmatch_t match;
	result = regexec( &re, stringToMatch, 1, &match, 0 );

	const char * matchResult = "MATCHES";
	// 0 不匹配
	if ( result != 0 ) {
		matchResult = "DOES NOT MATCH";
	}

	fprintf( stderr, "\t\"%s\" %s \"%s\"\n", matchResult,
		regExpComplete, stringToMatch);

	regfree(&re);

	return 0;
}

