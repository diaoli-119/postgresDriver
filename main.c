#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <pubFunc.h>

int main(int argc, char **argv)
{	
	if(connectToPostgres()) printf("connect to postgres ok!\n");
	else exit(EXIT_FAILURE);
	
	/*Infinite loop for reading meshData.txt every 20 seconds*/
	while(true)
	{
		readFile();
		sleep(20);
	}
	exit(EXIT_SUCCESS);
}
