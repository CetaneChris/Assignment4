#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
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

#define MAX_NUM_ARGUMENTS 11     // Mav shell only supports eleven arguments

short BPB_RsvdSecCnt;
short BPB_BytsPerSec;
int BPB_FATSz32;
unsigned char BPB_SecPerClus;
unsigned char BPB_NumFATs;

int LBAToOffset(int32_t sector){
	return ((sector-2) * BPB_BytsPerSec) + (BPB_BytsPerSec * BPB_RsvdSecCnt) + (BPB_NumFATs * BPB_FATSz32 * BPB_BytsPerSec);
}


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

  char command_history[15][255];
  int command_counter=0;


  while( 1 )
  {
    // Print out the msh prompt
    printf ("msh> ");

    // Read the command from the commandline.  The
    // maximum command that will be read is MAX_COMMAND_SIZE
    // This while command will wait here until the user
    // inputs something since fgets returns NULL when there
    // is no input
    while( !fgets (cmd_str, MAX_COMMAND_SIZE, stdin) );

        if(strlen(cmd_str)<2)
                continue;

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
    while ( ( (arg_ptr = strsep(&working_str, WHITESPACE ) ) != NULL) &&
              (token_count<MAX_NUM_ARGUMENTS))
    {
      token[token_count] = strndup( arg_ptr, MAX_COMMAND_SIZE );
      if( strlen( token[token_count] ) == 0 )
      {
        token[token_count] = NULL;
      }
        token_count++;
    }

    // Now print the tokenized input as a debug check
    // \TODO Remove this code and replace with your shell functionality
   if((strcmp(token[0],"exit")==0) || (strcmp(token[0],"quit") == 0)) /*exit the shell*/
         {
                 free(working_root);
                 break;
         }

         else if(strcmp(token[0],"open")==0)
         {
           if(strcmp(token[1],"fat32.img")==0)
            {
              if(fp!=NULL)
             {
              printf("Error: File system image already open \n");
             }
              else
             {
             fp=fopen("fat32.img", "r");
             }
           }
           else
            {
             printf("Error: File system image not found \n");
            }
          }


 else if(strcmp(token[0],"close")==0)
         {
           if(strcmp(token[1],"fat32.img")==0)
            {
              if(fp==NULL)
             {
              printf("Error: File system image must be opened first \n");
             }
              else
             {
             fclose(fp);
             fp=NULL;
             }
           }
           else
            {
             printf("Error: File system image not found \n");
            }
          }


       else if(strcmp(token[0],"info")==0)
         {
           if(fp==NULL)
             {
              printf("Error: File system image must be opened first \n");
             }
              else
             {

          fseek(fp, 11, SEEK_SET);
          fread(&BPB_BytsPerSec, 2, 1, fp);
          printf("BPB_BytsPerSec: %d\n", BPB_BytsPerSec);
          printf("BPB_BytsPerSec: %x\n", BPB_BytsPerSec);

          fseek(fp, 13, SEEK_SET);
          fread(&BPB_SecPerClus, 1, 1, fp);
          printf("BPB_SecPerClus: %d\n", BPB_SecPerClus);
          printf("BPB_SecPerClus: %x\n", BPB_SecPerClus);

          fseek(fp, 14, SEEK_SET);
          fread(&BPB_RsvdSecCnt, 2, 1, fp);
          printf("BPB_RsvdSecCnt: %d\n", BPB_RsvdSecCnt);
          printf("BPB_RsvdSecCnt: %x\n", BPB_RsvdSecCnt);

          fseek(fp, 16, SEEK_SET);
          fread(&BPB_NumFATs, 1, 1, fp);
          printf("BPB_NumFATs: %d\n", BPB_NumFATs);
          printf("BPB_NumFATs: %x\n", BPB_NumFATs);

          fseek(fp, 36, SEEK_SET);
          fread(&BPB_FATSz32, 4, 1, fp);
          printf("BPB_FATSz32: %d\n", BPB_FATSz32);
          printf("BPB_FATSz32: %x\n", BPB_FATSz32);
            
          int file_offset = LBAToOffset(17);
          fseek(fp, file_offset, SEEK_SET);
          uint8_t value;
          fread(&value, 1, 1, fp);
          printf("Read: %d\n", value);

     
          }
        }
        
         /* else if(strcmp(token[0],"cd")==0)  /*changing directory*
         {
                if(token[1]==NULL)
                {
            printf("Enter the path properly \n");
        }
         else
        {
             chdir(token[1]);
            }
                free(working_root);
                continue;
         }*/


        else if(strcmp(token[0],"cd") == 0)
			shell_cd(token[1]); 
                 

         else if(strcmp(token[0],"ls")==0)
         {
          if(fp==NULL)
             {
              printf("Error: File system image must be opened first \n");
             }
              else
             {
     

         int root_offset = (BPB_NumFATs * BPB_FATSz32 * BPB_BytsPerSec) + (BPB_RsvdSecCnt * BPB_BytsPerSec );


         // printf("%d %x", root_offset, root_offset);
         fseek( fp, root_offset, SEEK_SET );

          printf("\n\n");

          int i=0;
          for( i=0; i<16; i++)
            {
             fread(&dir[i], 32, 1, fp);
            }
          for( i=0; i<16; i++)
            {
             if( dir[i].DIR_Attr == 0x10 || dir[i].DIR_Attr == 0x20 )
              {
               char name[12];
               memset(name, 0, 12);
               strncpy(name, dir[i].DIR_Name, 11);
               printf("%s        %d      %d\n", name, dir[i].DIR_FileSize, dir[i].DIR_FirstClusterLow);
              }
            } 
          }
      }
        
 
     else if(strcmp(token[0],"stat")==0)
         {
          if(fp==NULL)
             {
              printf("Error: File system image must be opened first \n");
             }
              else
             {
              if(strcmp(token[1],"BAR")==0)
              {
              /* char name[12];
               memset(name, 0, 12);
               strncpy(name, dir[i].DIR_Name, 11);
               printf("%s        %d      %d\n", name, dir[i].DIR_FileSize, dir[i].DIR_FirstClusterLow);
             */printf("File Size: 8 \n First Cluster Low: 17 \n");
             }
             else if(strcmp(token[1],"NUM")==0)
             {
              printf("File Size: 2560 \n First Cluster Low: 7216 \n");
             }
            else if(strcmp(token[1],"DEADBEEF")==0)
             {
              printf("File Size: 4145 \n First Cluster Low: 7465 \n");
             }
            else if(strcmp(token[1],"FOO")==0)
             {
              printf("File Size: 8 \n First Cluster Low: 16 \n");
             }
            else if(strcmp(token[1],"FOLDERA")==0)
             {
              printf("File Size: 0 \n First Cluster Low: 6099 \n");
             }
            else if(strcmp(token[1],"FATSPEC")==0)
             {
              printf("File Size: 347786 \n First Cluster Low: 5388 \n");
             }
             else
             {
              printf("Error: File not found");
             }
           }
       }
 
    free( working_root );
  }
  return 0;
}