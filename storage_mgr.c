/*
Author: Adithya Ramachandran, Prasanna Shanuganathan
Description: The goal of this project is to implement a simple storage manager -
 a module that is capable of reading blocks from a file on disk into memory and writing
 blocks from memory to a file on disk. The storage manager deals with pages (blocks) of fixed
 size (PAGE_SIZE). In addition to reading and writing pages from a file, it provides methods for
 creating, opening, and closing files. The storage manager has to maintain several types of information
 for an open file: The number of total pages in the file, the current page position (for reading and writing),
 the file name, and a POSIX file descriptor or FILE pointer.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "storage_mgr.h"
#include "dberror.h"

//Function declaration
void * allocateMemory(int size,int flag);
int isStreamFault(FILE *file);
void releaseMemory(void *);

//Global variable declaration
void *memory_ptr;
FILE *file_ptr;
int unit =sizeof(char);

void initStorageManager (){
  void *memory_ptr=NULL;
  FILE *file_ptr=NULL;
}

//used to release the memory
void releaseMemory(void *memoryPointer){
  free(memoryPointer);
}

//used to check if there is any fault or error in the file stream
int isStreamFault(FILE *file){
  if( ferror(file) ){
      return -1;
   }
   else{
     return 0;
   }
}

//used to allocate memory of required size and type
void * allocateMemory(size, flag){
  void *memory_ptr=NULL;
  if(flag ==1){
  memory_ptr= (char *)calloc(size,unit);//calloc is used so that it initialize the memmory value to zero
  return memory_ptr;
  }
  else{
    return NULL;
  }
}

//Used to create a new page file with the given name
RC createPageFile (char *fileName){
  initStorageManager();
  int writeResult=0;
  if(PAGE_SIZE > 0 && fileName != ""){
    memory_ptr =allocateMemory(PAGE_SIZE,1);
    if(memory_ptr == NULL){
      return RC_MEMORY_ERROR;
    }
    file_ptr = fopen(fileName, "w+b");//open a binary file in write mode
    if((access(fileName, W_OK) != -1) && file_ptr != NULL){
    writeResult = fwrite(memory_ptr,unit, PAGE_SIZE,file_ptr);//write the memory value to the file
    }
    else{
      releaseMemory(memory_ptr);
      fclose(file_ptr);
      printf("Error:File Not Found");
      return RC_FILE_NOT_FOUND;
    }
    if(writeResult == PAGE_SIZE){
    releaseMemory(memory_ptr);
    fclose(file_ptr);
    return RC_OK;
    }
    else{
      return RC_WRITE_ERROR;
    }
  }
  else{
    perror("Error:NULL parameter error");
    return RC_NULL_ERROR;
  }
}

//used to open an existing page file in read/update mode
RC openPageFile (char *fileName, SM_FileHandle *fHandle){
  initStorageManager();
  int totalBytes=0;
  int totalPages=0;
  char *data=NULL;
  if(fileName != NULL){
    file_ptr =fopen(fileName,"r+");
  }
  else{
    perror("Error:NULL parameter error");
    return RC_NULL_ERROR;
  }
  /*
  if(feof(file_ptr)){
    return RC_NULL_ERROR;
  }
  */
  if(file_ptr!=NULL){
    fseek (file_ptr, 0, SEEK_END);
    totalBytes=ftell(file_ptr);
    totalPages=totalBytes/PAGE_SIZE;
    fHandle->fileName=fileName;
    fHandle->curPagePos=0;
    fHandle->totalNumPages=totalPages;
    fHandle->mgmtInfo=file_ptr;
    return RC_OK;
  }
  else{
    printf("Error:File Not Found");
    return RC_FILE_NOT_FOUND;
  }
}

//used to close the page file which was opened
RC closePageFile (SM_FileHandle *fHandle){
  int result;
  if(fHandle != NULL && fHandle->mgmtInfo != NULL &&(access(fHandle->fileName, W_OK) != -1)){
    result= fclose(fHandle->mgmtInfo);
    if(result ==0){
      return RC_OK;
    }
    else{
      printf("Error:File Not Found");
      return RC_FILE_NOT_FOUND;
    }
  }
  else{
    printf("Error:NULL parameter error");
    return RC_NULL_ERROR;
  }
}

//used to destroy the file using the file name
RC destroyPageFile (char *fileName){
  int result;
  if(fileName != NULL && (access(fileName, W_OK) != -1) ){
    result = remove(fileName);
    if(result == 0 && (access(fileName, W_OK) == -1)){
      return RC_OK;
    }
    else{
      printf("Error:File Not Found");
      return RC_FILE_NOT_FOUND;
    }
  }
  else{
    printf("Error:NULL parameter or file not accessable error");
    return RC_NULL_ERROR;
  }
}

//used to read the block with the given page #
RC readBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage){
  if(fHandle != NULL){
    int seekResult,readResult;
    if(pageNum > fHandle->totalNumPages || pageNum<0){
      printf("Error:Page not exist");
      return RC_READ_NON_EXISTING_PAGE;
    }
    else{

      if(fHandle->mgmtInfo != NULL){
        //rewind(fHandle->mgmtInfo );
        rewind(fHandle->mgmtInfo );
        seekResult = fseek(fHandle->mgmtInfo,pageNum*PAGE_SIZE,SEEK_SET);
        if(seekResult ==0){
          readResult = fread(memPage,unit,PAGE_SIZE,fHandle->mgmtInfo);
          //Additional Functionality: Code for if I need to bring the pointer back to starting position
          //rewind(fHandle->mgmtInfo );
          //seekResult = fseek(fHandle->mgmtInfo,pageNum*PAGE_SIZE,SEEK_SET);
          //fHandle->curPagePos=0;
          if(readResult==PAGE_SIZE){
          fHandle->curPagePos=pageNum;
          return RC_OK;
          }
          else{
            return RC_READ_ERROR;
          }
        }
        else{
          printf("Error:Page not exist");
          return RC_READ_NON_EXISTING_PAGE;
        }
      }
      else{
        return RC_FILE_NOT_FOUND;
      }
    }
  }
  else{
    printf("Error:NULL parameter error");
    return RC_NULL_ERROR;
  }
}

//used to get the current block position
int getBlockPos (SM_FileHandle *fHandle){
  if(fHandle->curPagePos >=0){
    return fHandle->curPagePos;
  }
  else{
    printf("Error:NULL or negative parameter error");
    return RC_NULL_ERROR;
  }
}

//used to read the first block from the page file
RC readFirstBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){
  if(fHandle != NULL){
    int seekResult,readResult;
    if(fHandle->mgmtInfo != NULL){
        rewind(fHandle->mgmtInfo);
        seekResult = fseek(fHandle->mgmtInfo,0,SEEK_SET);
        if(seekResult ==0){
          readResult = fread(memPage,unit,PAGE_SIZE,fHandle->mgmtInfo);
          fHandle->curPagePos=0;
          return RC_OK;
        }
        else{
          printf("Error:Page not exist");
          return RC_READ_NON_EXISTING_PAGE;
        }
      }
      else{
        return RC_FILE_NOT_FOUND;
      }
    }
    else{
      return RC_NULL_ERROR;
    }
  }

//used to read the previous block from the page file using the curPagePos
RC readPreviousBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){
  //return readBlock(fHandle->curPagePos-1,fHandle,memPage);
  int pageNum = (fHandle->curPagePos)-2;
  if(fHandle != NULL){
    int seekResult,readResult;
      if(fHandle->mgmtInfo != NULL){
        rewind(fHandle->mgmtInfo );
        seekResult = fseek(fHandle->mgmtInfo,pageNum*PAGE_SIZE,SEEK_SET);
        if(seekResult ==0){
          readResult = fread(memPage,unit,PAGE_SIZE,fHandle->mgmtInfo);
          fHandle->curPagePos=pageNum;
          return RC_OK;
        }
        else{
          printf("Error:Seek error");
          return RC_READ_NON_EXISTING_PAGE;
        }
      }
      else{
        printf("Error:File stream empty error");
        return RC_FILE_NOT_FOUND;
      }
  }
  else{
    printf("Error:NULL parameter error");
    return RC_NULL_ERROR;
  }
}

//Used to read the current block from the page file
RC readCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){
  //return readBlock(fHandle->curPagePos,fHandle,memPage);
  if(fHandle != NULL){
    int seekResult,readResult;
      if(fHandle->mgmtInfo != NULL){
        rewind(fHandle->mgmtInfo );
        int pageNum= fHandle->curPagePos;
          seekResult = fseek(fHandle->mgmtInfo,pageNum*PAGE_SIZE,SEEK_SET);
          readResult = fread(memPage,unit,PAGE_SIZE,fHandle->mgmtInfo);
          if(readResult==PAGE_SIZE){
          fHandle->curPagePos=pageNum;
          return RC_OK;
        }
        else{
          printf("Error:Page not exist");
          return RC_READ_NON_EXISTING_PAGE;
        }
      }
      else{
        return RC_FILE_NOT_FOUND;
      }
  }
  else{
    printf("Error:NULL parameter error");
    return RC_NULL_ERROR;
  }

}

//used to read the next block from page file based on the current position of the filee
RC readNextBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){
  //return readBlock(fHandle->curPagePos+1,fHandle,memPage);
  if(fHandle != NULL){
    int seekResult,readResult;
      if(fHandle->mgmtInfo != NULL){
        rewind(fHandle->mgmtInfo );
        int pageNum= fHandle->curPagePos+1;
          seekResult = fseek(fHandle->mgmtInfo,pageNum*PAGE_SIZE,SEEK_SET);
          readResult = fread(memPage,unit,PAGE_SIZE,fHandle->mgmtInfo);
          rewind(fHandle->mgmtInfo );
          if(readResult==PAGE_SIZE){
          fHandle->curPagePos=pageNum;
          return RC_OK;
        }
        else{
          printf("Error:Page not exist");
          return RC_READ_NON_EXISTING_PAGE;
        }
      }
      else{
        return RC_FILE_NOT_FOUND;
      }
  }
  else{
    printf("Error:NULL parameter error");
    return RC_NULL_ERROR;
  }

}

//Used to read the last block in the page file
RC readLastBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){
  if(fHandle != NULL){
    int seekResult,readResult;
    if(fHandle->mgmtInfo != NULL){
        rewind(fHandle->mgmtInfo );
        seekResult = fseek(fHandle->mgmtInfo,(fHandle->totalNumPages*PAGE_SIZE),SEEK_SET);
        if(seekResult ==0){
          readResult = fread(memPage,unit,PAGE_SIZE,fHandle->mgmtInfo);
          return RC_OK;
        }
        else{
          printf("Error:Page not exist");
          return RC_READ_NON_EXISTING_PAGE;
        }
      }
      else{
        return RC_FILE_NOT_FOUND;
      }
    }
    else{
      return RC_NULL_ERROR;
    }
}

//Used to write a block intot the file based on the page #
RC writeBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage){
  int seekResult,writeResult;
  if(pageNum > fHandle->totalNumPages){
    return RC_WRITE_NON_EXISTING_PAGE;
  }
  else if(fHandle == NULL){
    return RC_NULL_ERROR;
  }
  else{
    seekResult = fseek(fHandle->mgmtInfo,pageNum*PAGE_SIZE,SEEK_SET);
    if(seekResult== 0){
      writeResult=fwrite(memPage,unit, PAGE_SIZE , fHandle->mgmtInfo);
      //seekResult = fseek(fHandle->mgmtInfo,pageNum*PAGE_SIZE,SEEK_SET);
      if(writeResult != PAGE_SIZE){
        return RC_WRITE_FAILED;
      }
      else{
        return RC_OK;
      }
    }
    else{
      return RC_SEEK_FAILURE;
    }
  }
}

//used to write the current block
RC writeCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){
  return writeBlock(fHandle->curPagePos,fHandle,memPage);
}

//Used to append empty block to the end of the page
RC appendEmptyBlock (SM_FileHandle *fHandle){
  initStorageManager();
  int writeResult,seekResult;
  memory_ptr =allocateMemory(PAGE_SIZE,1);
  if(memory_ptr ==NULL){
    return RC_MEMORY_ERROR;
  }
  seekResult = fseek(fHandle->mgmtInfo,0,SEEK_END);
  if(seekResult==0){
    writeResult=fwrite(memory_ptr,unit, PAGE_SIZE , fHandle->mgmtInfo);
    if(writeResult == PAGE_SIZE){
      releaseMemory(memory_ptr);
      fHandle->totalNumPages = (fHandle->totalNumPages)+1;
      fHandle->curPagePos =(fHandle->curPagePos)+1;
      fseek(fHandle->mgmtInfo,0,SEEK_END);
      return RC_OK;
    }
    else{
      return RC_WRITE_FAILED;
    }
  }
  else{
    return RC_SEEK_FAILURE;
  }

}

//use to ensure that page file specified page size, if not it is added
RC ensureCapacity (int numberOfPages, SM_FileHandle *fHandle){
  initStorageManager();
  if(numberOfPages > fHandle->totalNumPages){
    int extraPages =numberOfPages-fHandle->totalNumPages;
    int writeResult,seekResult;
    memory_ptr =allocateMemory(PAGE_SIZE*extraPages,1);
    if(memory_ptr ==NULL){
      return RC_MEMORY_ERROR;
    }
    seekResult = fseek(fHandle->mgmtInfo,0,SEEK_END);
    if(seekResult==0){
      writeResult=fwrite(memory_ptr,unit, PAGE_SIZE*extraPages , fHandle->mgmtInfo);
      if(writeResult == PAGE_SIZE*extraPages){
        //seekResult = fseek(fHandle->mgmtInfo,0,SEEK_END);
        fHandle->totalNumPages = (fHandle->totalNumPages)+extraPages;
        fHandle->curPagePos =fHandle->totalNumPages;
        releaseMemory(memory_ptr);
        return RC_OK;
      }
      else{
        releaseMemory(memory_ptr);
        return RC_WRITE_FAILED;
      }
    }
    else{
      return RC_SEEK_FAILURE;
    }
  }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
