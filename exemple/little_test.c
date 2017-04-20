#include <stdlib.h>
#include "../conduct.h"


int main(int argc, char **argv)
{

	struct conduct *c = conduct_create("TOTO", 10,10);
	conduct_show(c);

	struct iovec iov [10];

	char aaa[10]={'A','A','A','A','A','A','A','A','A','A'};
	for(int i=0;i<10;i++){
		iov[i].iov_base=&aaa[i];
		iov[i].iov_len=1;
	}

	conduct_writev(c,iov,10);

	conduct_show(c);

	struct iovec iov2 [10];

	char tt[10];

	for(int i=0;i<10;i++){
		iov2[i].iov_base=&tt[i];
		iov2[i].iov_len=1;
	}

	conduct_readv(c,iov2,10);


	printf("READ : ");
	for(int i=0;i<10;i++){
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
	


	return 0;

	//conduct_close(c);
	//conduct_destroy(c);

	c=conduct_open("TOTO");

	if(c==NULL){
		printf("OPEN FAIL\n");
		return -1;
	}

	struct conduct *a =c;

	int size=conduct_write(a,"ABCDEFGHIJK",6);
	conduct_show(c);

	printf("SIZE WRITE :%d\n",size);

	size=conduct_write(a,"ABCDEFGIJK",4);
	conduct_show(c);

	printf("SIZE WRITE :%d\n",size);

	char buff[11]={0};
	conduct_read(a,buff,10);
	printf("\nDEDANS : %s\n\n",buff);
	conduct_show(c);


	size=conduct_write(a,"COCOCO",6);
	conduct_show(c);


	//printf("SIZE WRITE :%d\n",size);

	//size=conduct_write(a,"ZZ",2);

	char buff2[11]={0};
	conduct_read(c,buff2,10);
	printf("\nDEDANS : %s\n\n",buff2);
	conduct_show(c);

	size=conduct_write(a,"COCOCO",6);
	conduct_show(c);

	char buff3[11]={0};
	conduct_read(c,buff3,10);
	printf("\nDEDANS : %s\n\n",buff3);

	conduct_show(c);

	printf("\nDEDANS : %s\n\n",buff4);
	conduct_show(c);



	printf("SIZE WRITE :%d\n",size);



}
