#include <stdlib.h>
#include "../conduct.h"


int main(int argc, char **argv)
{

	struct conduct *c = conduct_create("TOTO", 5,20);
	
	conduct_close(c);

	c=NULL;

	c=conduct_open("TOTO");

	

	if(c==NULL){
		printf("OPEN FAIL\n");
		return -1;
	}

	conduct_show(c);

	struct conduct *a =c;

	int size=conduct_write(a,"ABCDEFGHIJK",6);
	printf("SIZE WRITE :%d\n",size);
	conduct_show(c);
	

	size=conduct_write(a,"MNOPQRS",4);
	printf("SIZE WRITE :%d\n",size);
	conduct_show(c);


	char buff[12]={0};
	size=conduct_read(a,buff,12);
	printf("SIZE READ : %d | READ : %s\n",size,buff);
	conduct_show(c);


	size=conduct_write(a,"COXOVOCOXOVOT",13);
	printf("SIZE WRITE :%d\n",size);
	conduct_show(c);


	char buff2[11]={0};
	size=conduct_read(c,buff2,10);
	printf("SIZE READ : %d | READ : %s\n",size,buff2);
	conduct_show(c);


	size=conduct_write(a,"PAMATA",6);
	printf("SIZE WRITE :%d\n",size);
	conduct_show(c);

	char buff3[11]={0};
	size=conduct_read(c,buff3,10);
	printf("SIZE READ : %d | READ : %s\n",size,buff3);
	conduct_show(c);

	/*
	char buff4[11]={0};
	size=conduct_read(c,buff4,10);
	printf("SIZE READ : %d | READ : %s\n",size,buff4);
	conduct_show(c);
	*/


}
