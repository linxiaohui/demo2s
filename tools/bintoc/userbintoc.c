#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define LINELEN 80
int
main(int argc, char *argv[])
{
/*
	usage : bintoc symbol binfile
if (binfile == dir1/dir2/file)
	binary_dir1_dir2_symbol
*/
	char *symbol;
	char start[LINELEN]; //symbol name of C array
	char len[LINELEN]; //symbol name of C array length
	char s[LINELEN]; //temporary string
	FILE *input; //FILE stream of binfile
	int bcount; //length of binfile
	char *r; //temporary char *
	unsigned char c;
	bcount = 0;
	symbol = argv[1];
	//open binary file
	input = fopen(argv[2], "rb");
	if (input == NULL) exit(1);
	//make start symbol
	strcpy(start, "binary_user");
	//make length symbol
	strcpy(len, "binary_user");

	//concatenate start symbol
	strcat(start, "_");
	strcat(start, symbol);
	//finish start symbol
	strcat(start, "_start");

	//concatenate length symbol
	strcat(len, "_");
	strcat(len, symbol);
	//finish length symbol
	strcat(len, "_size");
	//output binary file to C source file
	printf("unsigned char %s[] = {\n", start);
	while (fread(&c,sizeof(unsigned char), 1, input))
	{
		printf("0x%x, ",c);
		bcount++;
		if (!(bcount % 16))
			printf("\n");
	}
	printf("0x0\n};\n");
	//make size
	printf("unsigned int %s = %d;\n",len, bcount);

	close(input);

	return 0;
}   

