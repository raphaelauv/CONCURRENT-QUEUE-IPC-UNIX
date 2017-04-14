#include <stdlib.h>
#include "conduct.h"


int main(int argc, char **argv)
{

	struct conduct *c = conduct_create(NULL, 5,10);

	//conduct_close(c);
	//conduct_destroy(c);

	if(c==NULL){
			printf("FAIL\n");
		}


	//

	//c=conduct_open("TOTO");

	struct conduct *a =c;

	//struct conduct *c = conduct_open("TOTO");


	int size=conduct_write(a,"ABCDEFGHIJK",6);
	conduct_show(c);

	//printf("SIZE WRITE :%d\n",size);

	size=conduct_write(a,"ABCDEFGIJK",4);
	conduct_show(c);

	//printf("SIZE WRITE :%d\n",size);

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

	char buff4[11]={0};
	conduct_read(c,buff4,10);
	printf("\nDEDANS : %s\n\n",buff4);
	conduct_show(c);


	printf("SIZE WRITE :%d\n",size);
}