
/*
TITLE:			Overwrite
DESCRIPTION:	Overwrite empty space on disk, write number of small files and data.
CODE:			github.com/muhammd/Erasure
LICENSE:		GNU General Public License v3.0 http://www.gnu.org/licenses/gpl.html
AUTHOR:			Muhammad Haidari
WEBSITE:		mhaidari.com
DATE:			2019-10-11
VERSION:		1.1
*/

// Include -----
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <limits.h>
#include <ctype.h>

#ifdef _WIN32	// _WINDOWS
// _mkdir, _rmdir windows
#include <direct.h>
#else
// mkdir, rmdir, linux
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif


// Definitions -----
#define BLOCK_SIZE 4096				// Default block size, used in NTFS EXT4.
#define DIR_NAME "temp123456789"	// Temporary directory name for files and data.
#define FILE_NAME "000000000"		// Temporary file name for large data.
#define FILE_PREFIX "000000000"		// File prefix for small temporary files.
#define VERSION "Version 1.1 2019-10-06"	// Version


// Arguments structure
struct args {
	int test;				// For test, don't delete written files
	int one;				// Write one's
	int ran;				// Write random data
	int blocksize;			// Block size
	int files;				// Number of files to write
	char *data;				// Data to write, mb, gb, all
	char *path;				// Path where to write 
	unsigned char *blockbuf;			// Block buffer init with calloc.

	char dirpath[1024];		// Temp dir path.
	char filepath[1024];	// Temp files path.

};


// Headers -----
struct args parseargs(int argc, char *argv[]);
struct args prepargs(struct args argsv);
int writefiles(struct args argsv);
int writefile(char *filepath, unsigned char *buf, int bufsize);
int writedata(struct args argsv);
void cleandata(struct args argsv);
void freemem(struct args argsv);

int makedir(const char* name);
int remdir(const char* name);
void randbuf(unsigned char *buf, int bufsize);
int strstarti(const char *str, const char *substr);
int strendi(char* str, char* substr);
int strcmpi2(char const *str1, char const *str2, int num);

void errmsg(struct args argsv, char *message);
void exitmsg(struct args argsv, char *message);
void exiterr(struct args argsv, char *message);
void errcheckint(int res, struct args argsv, char *message);
void errcheckptr(void *res, struct args argsv, char *message);
void usage();



// main
int main(int argc, char *argv[])
{
	// Local vars
	int res;
	struct args argsv;
	clock_t ticks;

	// Init 
	printf("\n");
	ticks = clock();

	// Parse, prepare arguments
	argsv = parseargs(argc, argv);
	argsv = prepargs(argsv);

	// Write files
	res = writefiles(argsv);
	//if(res){ errmsg(argsv, "Error writing files."); }

	// Write data
	res = writedata(argsv);
	//if(res){ errmsg(argsv, "Error writing data."); }

	// Clear
	freemem(argsv);
	if(!argsv.test){
		cleandata(argsv);
	}

	// Done
	ticks = clock() - ticks;
	printf("Time: %.3f seconds \n", (float)ticks / CLOCKS_PER_SEC);
	printf("Done. \n");
	//system("pause");
	return 0;

}// 


// Parse arguments
struct args parseargs(int argc, char *argv[])
{
	// 
	int index = 0;
	int res = 0;
	struct args argsv;

	// init argsv
	argsv.test = 0;
	argsv.one = 0;
	argsv.ran = 0;
	argsv.blocksize = BLOCK_SIZE;
	argsv.files = 0;
	argsv.data = NULL;
	argsv.path = NULL;
	argsv.blockbuf = NULL;

	strcpy(argsv.dirpath, "");
	strcpy(argsv.filepath, "");

	// usage
	if(argc < 2){ usage(); exit(0); }

	// parse args
	for(index = 0; index < argc; index++)
	{
		// 
		if(strstarti(argv[index], "-h") || strstarti(argv[index], "--help") ){ 
			usage();
			exit(0);
		}
		// 
		else if(strstarti(argv[index], "-v") || strstarti(argv[index], "--version") ){ 
			printf(VERSION);
			printf("\n");
			exit(0);
		}
		else if(strstarti(argv[index], "-test")){ 
			argsv.test = 1; 
		}
		else if(strstarti(argv[index], "-one")){ 
			argsv.one = 1; 
		}
		else if(strstarti(argv[index], "-rand")){ 
			argsv.ran = 1; 
		}
		else if(strstarti(argv[index], "-block:")){
			argsv.blocksize = atoi(argv[index] + 7);
			if(argsv.blocksize == 0){ argsv.blocksize = BLOCK_SIZE; }
		}
		else if(strstarti(argv[index], "-files:")){
			argsv.files = atoi(argv[index] + 7);
		}
		else if(strstarti(argv[index], "-data:")){
			argsv.data = argv[index] + 6;
		}
		else if(strstarti(argv[index], "-path:")){
			argsv.path = argv[index] + 6;
		}

	}// for

	// Check for space in args
	for(index = 0; index < argc; index++)
	{
		if(strendi(argv[index], ":")){
			exitmsg(argsv, "Error, arguments contain space or ends with : ");
		}
	}

	// Files or data args are required
	if(argsv.files == 0 && argsv.data == NULL)
	{
		exitmsg(argsv, "Error, arguments -files: or -data: are missing.");
	}

	// Path arg is required
	if(argsv.path == NULL)
	{
		exitmsg(argsv, "Error, argument -path: is missing.");
	}

	return argsv;

}//



// Prepare arguments, memory, directory
struct args prepargs(struct args argsv)
{
	// 
	int index = 0;
	int res = 0;

	// Initialize blockbuf with calloc, set with 0.
	argsv.blockbuf = (unsigned char*) calloc(argsv.blocksize, sizeof (unsigned char));
	errcheckptr(argsv.blockbuf, argsv, "Block buffer initialization error. ");
	//printf("blockbuf %d \n", blockbuf);

	// If one, set buffer with one's
	if(argsv.one){ memset(argsv.blockbuf, 255, argsv.blocksize);  }

	// If random set buffer with random data
	if(argsv.ran){ randbuf(argsv.blockbuf, argsv.blocksize); }

	// Prepare path
	strcat(argsv.dirpath, argsv.path);
	if(!strendi(argsv.dirpath, "/") && !strendi(argsv.dirpath, "\\")){
		strcat(argsv.dirpath, "/");
	}
	strcat(argsv.dirpath, DIR_NAME);
	strcat(argsv.dirpath, "/");

	// Create temporary folder
	remdir(argsv.dirpath);
	res = makedir(argsv.dirpath);
	if(res == -1 && errno != 17) // Error 17, File exists.
	{
		errcheckint(res, argsv, "Error mkdir parameter -path: ");
	}

	return argsv;

}// 


// writefiles
int writefiles(struct args argsv)
{
	// 
	int index;
	int res;
	float stat = 0.1F;
	char stri[16];


	// 
	if(!argsv.files){ return 0; }

	// Write files
	printf("Writing files: \n");
	for(index = 1; index <= argsv.files; index++)
	{
		// File path
		sprintf(stri, "%d", index);
		strcpy(argsv.filepath, argsv.dirpath);
		strcat(argsv.filepath, FILE_PREFIX);
		strcat(argsv.filepath, stri);

		// Writefile
		res = writefile(argsv.filepath, argsv.blockbuf, argsv.blocksize);
		if(res){ return -1; }

		// Statistics
		if(index > argsv.files * stat)
		{
			printf("%.0f%% ", stat * 100 );
			stat += 0.1F;

		}// if

	}// for
	printf("100%% \n");
	return 0;

}//


// writefile
int writefile(char *filepath, unsigned char *buf, int bufsize)
{
	size_t rest;
	int resi;

	FILE *fp = fopen(filepath, "wb");
	if(fp == NULL){ return -1; }

	rest = fwrite(buf, bufsize, 1, fp);
	if(rest < 1){ return -1; }

	resi = fclose(fp);
	if(resi == EOF){ return -1; }

	return 0;

}// 


// writedata
int writedata(struct args argsv)
{
	int mb = 0;
	int blocks = 0;
	int blockscont = 0;
	int blockscont2 = 0;
	int cont = 0;

	int index = 0;
	int resi = 0;
	size_t rest = 0;
	double stat = 0.1;
	FILE *fp;

	// 
	if(!argsv.data){ return 0; }

	// How many blocks to write, 1Mb / blocksize
	blockscont = 1024000 / argsv.blocksize;
	blockscont2 = blockscont;

	// Calculate blocks to write
	if(strendi(argsv.data, "mb")) { blocks = atoi(argsv.data) * blockscont; }
	else if(strendi(argsv.data, "gb")) { blocks = (1024 * atoi(argsv.data)) * blockscont; }
	else if(strstarti(argsv.data, "all")) { blocks = INT_MAX; }
	else{ return 0; }

	// Statistics for -data:all argument
	blockscont = (1024000 * 1024) / argsv.blocksize;
	blockscont2 = blockscont;

	// Open file
	strcpy(argsv.filepath, argsv.dirpath);
	strcat(argsv.filepath, FILE_NAME);
	fp = fopen(argsv.filepath, "wb");
	if(fp == NULL){ return -1; }

	// Write data
	printf("Writing data: \n");
	for(index = 0; index < blocks; index++)
	{
		// write block
		rest = fwrite(argsv.blockbuf, argsv.blocksize, 1, fp);
		if(rest != 1){ 
			printf("100%% \n");
			fclose(fp);
			return -1;
		}

		// Statistics
		if(blocks != INT_MAX && index > blocks * stat)
		{
			printf("%.0f%% ", stat * 100 );
			stat += 0.1;

		}// if

		// Statistics for -data:all argument, up to 10gb
		if(blocks == INT_MAX && cont < 10 && index >= blockscont2)
		{
			cont++;
			blockscont2 += blockscont;
			printf("%dGb ", cont );
		}

	}// for
	printf("100%% \n");

	// close file
	fclose(fp);
	return 0;

}// 


// cleandata
void cleandata(struct args argsv)
{
	// 
	int index;
	int res;
	float stat = 0.1F;
	char stri[16];

	// Delete data file if exists
	strcpy(argsv.filepath, argsv.dirpath);
	strcat(argsv.filepath, FILE_NAME);
	remove(argsv.filepath);

	// Delete files
	printf("Cleaning: \n");
	for(index = 1; index <= argsv.files; index++)
	{
		// filepath
		sprintf(stri, "%d", index);
		strcpy(argsv.filepath, argsv.dirpath);
		strcat(argsv.filepath, FILE_PREFIX);
		strcat(argsv.filepath, stri);

		// delete file
		res = remove(argsv.filepath);
		//errcheckint(res, argsv, "Error deleting file.");

		// Statistics
		if(index > argsv.files * stat)
		{
			printf("%.0f%% ", stat * 100 );
			stat += 0.1F;

		}// if

	}// for
	printf("100%% \n\n");

	// Delete temp directory
	res = remdir(argsv.dirpath);
	//errcheckint(res, argsv, "Error deleting folder.");

}//


// Free memory 
void freemem(struct args argsv)
{
	errno = 0;

	if(argsv.blockbuf != NULL){
		free(argsv.blockbuf);
		argsv.blockbuf = NULL;
	}

	if(errno != 0){
		printf("Error on free(): %s \n", strerror(errno));
	}

}// 


// Util functions ---------------------

// Create directory, windows linux
int makedir(const char* name) 
{
#ifdef _WIN32	
	return _mkdir(name);
#else      
	return mkdir(name, 0700);  //  __linux__ , __unix__ 
#endif

}// 


// Remove directory, windows linux
int remdir(const char* name) 
{
#ifdef _WIN32	//  __linux__
	return _rmdir(name);
#else      
	return rmdir(name); 
#endif
}// 


// Set buffer with random data
void randbuf(unsigned char *buf, int bufsize)
{
	int index;
	srand((unsigned)time(NULL));

	for(index = 0; index < bufsize - 1; index++)
	{
		buf[index] = rand() % 256; // 
	}
}


// String starts with, case insensitive, 0 = false, 1 = true
int strstarti(const char *str, const char *substr)
{
	return strcmpi2(str, substr, strlen(substr) - 1);
	//return strncmp(substr, str, strlen(substr)) == 0;
}


// String ends with, case insensitive, 0 = false, 1 = true
int strendi(char* str, char* substr){
	return strcmpi2(str + strlen(str) - strlen(substr), substr, strlen(substr));
}


// Compare strings case insensitive, windows linux, 0 = false, 1 = true
int strcmpi2(char const *str1, char const *str2, int num)
{
	int res = 0;
	int cont = 0;

	// Check if some of the chars are different
	for (;; str1++, str2++, cont++) {
		res = tolower((unsigned char)*str1) - tolower((unsigned char)*str2);
		if (res != 0 || !*str1 || cont >= num) { break; }
	}

	// Set true false
	if(res){ res = 0; }
	else { res = 1; }

	return res;
}// 


// Exit with message
void exitmsg(struct args argsv, char *message)
{
	printf("%s \n", message);
	freemem(argsv);
	exit(0);

}// 


// Print error message, do not exit
void errmsg(struct args argsv, char *message)
{
	printf("Error %d, %s. \n%s \n", errno, strerror(errno), message);
}// 


// Exit with error message
void exiterr(struct args argsv, char *message)
{
	printf("Error %d, %s. \n%s \n", errno, strerror(errno), message);
	freemem(argsv);
	exit(errno);

}// 


// Check error integer
void errcheckint(int res, struct args argsv, char *message)
{
	if(res == -1){ exiterr(argsv, message); }
}// 


// Check error pointer
void errcheckptr(void *res, struct args argsv, char *message)
{
	if(res == NULL){ exiterr(argsv, message); }
}// 


// 
void usage()
{
	printf(
		"Overwrite empty space on disk, write small files and data. \n\n"
		"USAGE: \n"
		"overwrite [-h -v -test -one -rand -block:] (-files: &| -data:) -path: \n"
		"  -h \t\t Print help and usage message. \n"
		"  -v \t\t Print program version. \n"
		"  -test \t For test, don't delete written files. \n"
		"  -one \t\t Overwrite with 1, default with 0. \n"
		"  -rand \t Overwrite with random data, default with 0. \n"
		"  -block \t Block size, default 4096 bytes as NTFS and EXT4. \n"
		"  -files \t Number of files to write, block size each. \n"
		"  -data \t Quantity of data to write, ex: 1mb, 1gb, all. \n"
		"  -path \t Path to directory where to write data. \n\n"
		"EXAMPLE: \n"
		"# Write 10 files and 10Mb data, on Windows \n"
		"overwrite -files:10 -data:10mb -path:c:\\ \n\n"
		"# Write random data, block size 512 bytes on Linux \n"
		"overwrite -rand -block:512 -files:10 -data:10mb -path:/mnt/usbdisk/ \n\n"
		"Copyright GPLv3 http://github.com/muhammd/Erasure \n"

		);

}// 

