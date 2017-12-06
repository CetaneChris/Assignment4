// The MIT License (MIT)
// 
// Copyright (c) 2016, 2017 Trevor Bakker 
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <stdint.h>

#define WHITESPACE " \t\n"      // We want to split our command line up into tokens
                                // so we need to define what delimits our tokens.
                                // In this case  white space
                                // will separate the tokens on our command line

#define MAX_COMMAND_SIZE 255    // The maximum command-line size

#define MAX_NUM_ARGUMENTS 5     // Mav shell only supports five arguments

typedef int bool;
#define true 1
#define false 0

struct fat_details{
	int  BPB_BytesPerSec;
	int  BPB_SecPerClus;
	int  BPB_RsvdSecCnt;
	int  BPB_NumFATS;
	int  BPB_FATSz32;
	char name[11];
};

FILE *img;
struct fat_details* FAT;
int LBAToOffset(int32_t sector);
int16_t NextLB(uint32_t sector);
int search(char*);

struct __attribute__((__packed__)) DirectoryEntry {
	char DIR_Name[11];
	uint8_t DIR_Attr;
	uint8_t Unused1[8];
	uint16_t DIR_FirstClusterHigh;
	uint8_t Unused2[4];
	uint16_t DIR_FirstClusterLow;
	uint32_t DIR_FileSize;
};

struct DirectoryEntry dir[16];

int main()
{
	char * cmd_str = (char*) malloc( MAX_COMMAND_SIZE );

	bool state = false;
	bool been_closed = false;
	FAT = malloc(sizeof(struct fat_details));
	char buffer[100];
	int  i;

	while( 1 ){
		// Print out the msh prompt
		printf ("mfs> ");

		// Read the command from the commandline.  The
		// maximum command that will be read is MAX_COMMAND_SIZE
		// This while command will wait here until the user
		// inputs something since fgets returns NULL when there
		// is no input
		while( !fgets (cmd_str, MAX_COMMAND_SIZE, stdin) );

		/* Parse input */
		char *token[MAX_NUM_ARGUMENTS];

		int   token_count = 0;

		// Pointer to point to the token
		// parsed by strsep
		char *arg_ptr;

		char *working_str  = strdup( cmd_str ); 

		// we are going to move the working_str pointer so
		// keep track of its original value so we can deallocate
		// the correct amount at the end
		char *working_root = working_str;

		// Tokenize the input stringswith whitespace used as the delimiter
		while (((arg_ptr = strsep(&working_str, WHITESPACE)) != NULL) && (token_count<MAX_NUM_ARGUMENTS)){
			token[token_count] = strndup( arg_ptr, MAX_COMMAND_SIZE );
			if( strlen( token[token_count] ) == 0 )
				token[token_count] = NULL;
			token_count++;
		}

		if(token[0] == NULL)
			continue;
		else if(!strcmp(token[0], "exit") || !strcmp(token[0], "quit")){
			free(working_root);             //Checking for exit condition, freeing used data, and exiting
			if(state)
				fclose(img);
			break;
		}else if(strcmp(token[0],"open") && been_closed && !state)
			fprintf(stderr, "Error: File system image must be opened first\n");
		else if(strcmp(token[0],"open") && !state)
			fprintf(stderr, "Error: File system not open\n");
		else if(!strcmp(token[0],"open")){
			if(state)		// If a file is already open
				fprintf(stderr,"Error: File system image already open\n");
			else if(token[1] != NULL){	// Ensuring a file was selected
				img   = fopen(token[1], "r+");	// Open file in read only mode
				if(img == NULL)		// If file not found, print error
					fprintf(stderr,"Error: File system image not found\n");
				else{
					state = true;		// Set program state to open

					// Ensure buffer location
					fseek(img,11,SEEK_SET);

					// Retrieve attributes for 'info'
					fread(&FAT->BPB_BytesPerSec, 2, 1, img);
					fread(&FAT->BPB_SecPerClus, 1, 1, img);
					fread(&FAT->BPB_RsvdSecCnt, 2, 1, img);
					fread(&FAT->BPB_NumFATS, 1, 1, img);

					fseek(img,36,SEEK_SET);

					fread(&FAT->BPB_FATSz32, 4, 1, img);

					fseek(img,71,SEEK_SET);

					fread(&FAT->name, 11, 1, img);

					// Fill directory array
					int root_offset = (FAT->BPB_NumFATS * FAT->BPB_FATSz32 * FAT->BPB_BytesPerSec) + (FAT->BPB_RsvdSecCnt * FAT->BPB_BytesPerSec);

					fseek( img, root_offset, SEEK_SET );

					for(i=0; i<16; i++)
						fread(&dir[i], 32, 1, img);
				}
			}
		}else if(!strcmp(token[0],"close")){
			if(state){	// If file open, then close it
				state = false;		// Set program state to closed
				been_closed = true;	// Program knows that 'close' has been ran
				fclose(img);		// Close the file
			}else		// Else, print error
				fprintf(stderr,"Error: File system not open\n");
		}else if(!strcmp(token[0],"info")){
			printf("BPB_BytesPerSec = %d\n", FAT->BPB_BytesPerSec);		//Base 10
			printf("BPB_BytesPerSec = %x\n\n", FAT->BPB_BytesPerSec);	//Hex

			printf("BPB_SecPerClus  = %d\n", FAT->BPB_SecPerClus);		//Base 10
			printf("BPB_SecPerClus  = %x\n\n", FAT->BPB_SecPerClus);	//Hex

			printf("BPB_RsvdSecCnt  = %d\n", FAT->BPB_RsvdSecCnt);		//Base 10
			printf("BPB_RsvdSecCnt  = %x\n\n", FAT->BPB_RsvdSecCnt);	//Hex

			printf("BPB_NumFATS     = %d\n", FAT->BPB_NumFATS);		//Base 10
			printf("BPB_NumFATS     = %x\n\n", FAT->BPB_NumFATS);		//Hex

			printf("BPB_FATSz32     = %d\n", FAT->BPB_FATSz32);		//Base 10
			printf("BPB_FATSz32     = %x\n\n", FAT->BPB_FATSz32);		//Hex
		}else if(!strcmp(token[0],"stat")){
			int index = search(token[1]);					//Find directory entry for file/folder
			if(index < 0)			// -1 means file not found
				fprintf(stderr,"Error: File not found\n");
			else
				printf("File Attribute: 0x%x\nFile Size: %d\nStarting Cluster Number: %d\n\n", dir[index].DIR_Attr,  dir[index].DIR_FileSize, dir[index].DIR_FirstClusterLow);
		}else if(!strcmp(token[0],"get")){
			char *fname     = (char*)malloc(strlen(token[1]) * sizeof(char));
			strncpy(fname, token[1], 11);			//File name
			int index       = search(token[1]);				//Find directory entry
			if(index < 0){			// -1 means file not found
				fprintf(stderr, "Error: File not found");
				continue;
			}
			int size        = dir[index].DIR_FileSize;			//Total length of file
			int cluster     = dir[index].DIR_FirstClusterLow;		//Starting cluster for file
			int root_offset = LBAToOffset(cluster);
			char *buffer    = (char*)malloc((size + 1) * sizeof(char));	//buffer to hold file during write
			memset(buffer, 0, size + 1);
			FILE *fp = fopen(fname,"w");			//Output file

			while(size > 512){				//If file is longer than one page, loop through all bytes
				fseek(img, root_offset, SEEK_SET);	//Move to start of cluster
				fread(buffer, 512, 1, img);		//Read current cluster
				fwrite(buffer, 512, 1, fp);		//Write current cluster locally

				size -= 512;
				cluster = NextLB(cluster);		//Move to next cluster
				root_offset = LBAToOffset(cluster);
			}

			fseek(img, root_offset, SEEK_SET);		//Move to last cluster
                        fread(buffer, size, 1, img);			//Read remaining data
 			fwrite(buffer, size, 1 , fp);			//Write last bytes
			fclose(fp);					//Close output file
		}else if(!strcmp(token[0],"cd")){
			int index, root_offset;
			if(token[1] != NULL){				//Error handling
				char *tok = (char*)strtok(token[1],"/");//Search for all listed folders
				while(tok != NULL){
					if(strcmp(tok,"..")==0){	//Navigate to parent directory
						if(dir[1].DIR_Attr == 0x10 && dir[1].DIR_FirstClusterLow != 0)
							root_offset = LBAToOffset(dir[1].DIR_FirstClusterLow);
						else
							root_offset = (FAT->BPB_NumFATS * FAT->BPB_FATSz32 * FAT->BPB_BytesPerSec) + (FAT->BPB_RsvdSecCnt * FAT->BPB_BytesPerSec);
					}else if(!strcmp(tok,"."))	//Remain in current directory
						continue;
					else{				//Else, search for new directory
						index = search(tok);
						if(index > 0)
							root_offset = LBAToOffset(dir[index].DIR_FirstClusterLow);
						else
							fprintf(stderr,"Error: Folder not found!\n");
					}
					fseek (img, root_offset, SEEK_SET);	//move to found new cluster
					for(i=0; i < 16; i++){
						memset(&dir[i].DIR_Name,0,32);	//Empty current directory array
						fread(&dir[i],32,1,img);	//Refill directory with new data
					}
					tok = strtok(NULL,"/");			//Move to next folder and keep looking
				};
			}else
				fprintf(stderr,"Invalid entry\n"); 
		}else if(!strcmp(token[0],"ls")){
			for(i=0; i<16; i++)		//Printing elements of dir
				if((dir[i].DIR_Attr == 0x10 || dir[i].DIR_Attr == 0x20 || dir[i].DIR_Attr == 0x30 || dir[i].DIR_Attr == 0x01) && (dir[i].DIR_Name[0] != 0xffffffe5))
					printf("%.11s\t0x%x\t%d\t%d\n", dir[i].DIR_Name, dir[i].DIR_Attr, dir[i].DIR_FileSize, dir[i].DIR_FirstClusterLow);
		}else if(!strcmp(token[0],"read")){ 
			// Move to position token[1] and begin reading
			if(token_count == 5 && token[2] >= 0 && token[3] >= 0){
				int index    = search(token[1]);
				int root_offset = LBAToOffset(dir[index].DIR_FirstClusterLow);
				int user_offset = atoi(token[2]);
				uint8_t value;
				if( user_offset < FAT->BPB_BytesPerSec){
					int position = atoi(token[2]) + root_offset;
					int length   = atoi(token[3]);
					fseek(img, position, SEEK_SET);
					printf("read\n");
					for( i=1; i<=length; i++){
						fread(&value, 1, 1, img);
						printf("%d ",value);
					}
					printf("\n");
				}else{
					int block = dir[index].DIR_FirstClusterLow;
					while( user_offset > FAT->BPB_BytesPerSec ){
						block = NextLB(block);
						user_offset -= FAT->BPB_BytesPerSec;
					}
					int file_offset = LBAToOffset(block);
					int length   = atoi(token[3]);
					fseek( img, file_offset + user_offset, SEEK_SET);
					printf("read\n");
					for( i=1; i<=length; i++){
						fread(&value, 1, 1, img);
						printf("%d ",value);
					}
					printf("\n");
				}
			}else
				fprintf(stderr,"Error: Invalid arguments\n");
		}else if(!strcmp(token[0],"volume")){
			// Print FAT->name
			if(!strcasecmp(FAT->name, "NO NAME    "))		//If no drive name listed, print error
				fprintf(stderr,"Error: volume name not found");
			else							//Else print name
				printf("Volume Name: %s\n", FAT->name);
		}else
			printf("Command not found\n");

		free(working_root);
	}
	return 0;
}

int LBAToOffset(int32_t sector){
        return ((sector-2) * FAT->BPB_BytesPerSec) + (FAT->BPB_BytesPerSec * FAT->BPB_RsvdSecCnt) + (FAT->BPB_NumFATS * FAT->BPB_FATSz32 * FAT->BPB_BytesPerSec);
}

int16_t NextLB(uint32_t sector){
        uint32_t FATAddress = (FAT->BPB_BytesPerSec * FAT->BPB_RsvdSecCnt) + (sector * 4);
        int16_t val;
        fseek(img, FATAddress, SEEK_SET);
        fread(&val, 2, 1, img);
        return val;
}

int search(char *search){
	int i;
	char name[12], in_name[12];
	memset(name,32,11);
	name[11] = 0, in_name[11] = 0;
	char *in = (char*)strtok(search,".");

	strncpy(name,in,strlen(in));
	in = strtok(NULL, ".");
	if(in != NULL)
		strcpy(&name[8],in);
	for(i = 0; i < 16; i++){
		memset(in_name,32,10);
		strncpy(in_name,dir[i].DIR_Name,11);

		if(!strncasecmp(name,in_name,11))
			return i;
	}
	return -1;
}
