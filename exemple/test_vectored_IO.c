#include <stdlib.h>
#include "../conduct.h"


int main(int argc, char **argv)
{

	struct conduct *c = conduct_create("TOTO", 7,22);
	conduct_show(c);

	struct iovec iov [10];

	char aaa[10]={'A','B','C','D','E','F','G','H','I','J'};
	for(int i=0;i<10;i++){
		iov[i].iov_base=&aaa[i];
		iov[i].iov_len=1;
	}

	int sizeWrite=conduct_writev(c,iov,10);
	printf("WRITE : %d\n",sizeWrite);
	conduct_show(c);

	struct iovec iov2 [10];

	char tt[11];

	for(int i=0;i<10;i++){
		iov2[i].iov_base=&tt[i];
		iov2[i].iov_len=1;
	}
	tt[10]=EOF;

	int sizeRead=	conduct_readv(c,iov2,10);


	printf("READ : %d | ",sizeRead);
	for(int i=0;i<sizeRead;i++){
		char * p=(char *) iov2[i].iov_base;
		printf("%c",*p);
	}
	printf("\n");

	conduct_show(c);


	
	conduct_write_eof(c);
	printf("PUSH EOF !\n");

	char buff4[11]={0};
	int toto=conduct_read(c,buff4,10);

	if(toto<0){
		printf("ERROR after READ -> GOOD\n");
		return 0;
	}else{
		printf("NO ERROR after READ -> BAD\n");
	}
	
	conduct_close(c);
	conduct_destroy(c);


	return 0;

}
