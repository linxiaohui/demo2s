#include <stdio.h>
#include <string.h>
#include <stdlib.h>
int
main(int argc, char *argv[])
{
/*
	usage : bintoc symbol binfile
*/
	char *symbol;
	char namesymb_start[80];
	char namesymb_end[80];
	char lensymb[80];
	FILE *input;
	int bcount;
	unsigned char c;
	bcount = 0;
	symbol = argv[1];
	//open binary file
	input = fopen(argv[2], "rb");
	if (input == NULL) exit(1);
	//make start symbol
	strcpy(namesymb_start, "binary_user_");
	strcat(namesymb_start, symbol);
	strcat(namesymb_start, "_start");
	//make end symbol
	strcpy(namesymb_end, "binary_user_");
	strcat(namesymb_end, symbol);
	strcat(namesymb_end, "_end");
	printf("unsigned char %s[] = {\n", namesymb_start);
	while (fread(&c,sizeof(unsigned char), 1, input))
	{
		printf("0x%x, ",c);
		bcount++;
		if (!(bcount % 16))
			printf("\n");
	}
	printf("0x0\n};\n");
	//make length symbol
	strcpy(lensymb, "binary_user_");
	strcat(lensymb, symbol);
	strcat(lensymb, "_size");
	//make size
	printf("unsigned int %s = %d;\n",lensymb, bcount);
/*
	c = 0xff & bcount;
	printf("0x%x, ",c);
	bcount >>= 8;

	c = 0xff & bcount;
	printf("0x%x, ",c);
	bcount >>= 8;

	c = 0xff & bcount;
	printf("0x%x, ",c);
	bcount >>= 8;

	c = 0xff & bcount;
	printf("0x%x, ",c);

	printf("0x0\n};\n");
	printf("unsigned char %s[] = {0};\n", namesymb_end);*/

	close(input);

	return 0;
}   

