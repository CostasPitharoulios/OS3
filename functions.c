#include <stdio.h>
#include <string.h>
#include <time.h>

#include <stdlib.h>

#include "functions.h"


#include <sys/types.h>
#include <unistd.h>

#include <openssl/md5.h> // for md5 hashing

#define LINE_SIZE 64

//====================================================================================
//   createRandomTextFile(): creates a random tetx file with lines
//====================================================================================
int createRandomTextFile(){

    srand(time(NULL));
    FILE * fp;
    fp = fopen(FILENAME, "w");

    char line[LINE_SIZE];

    // fopen() returns NULL if last operation was unsuccessful 
    if(fp == NULL)
    {
        printf("ERROR: Can't create txt file.\n");
        exit(EXIT_FAILURE);
    }

    int i;
    int endLine = (rand()%30) +1; //number of lines -> range [1,30]
    for (i=0; i<endLine; i++){
	int numberOfChars = (rand()%(LINE_SIZE-1)) + 1; //number of characters in line -> range [1,LINE_SIZE-1]
        rand_str(line, numberOfChars );
        fputs(line, fp);
    }


    /* Close file to save file data */
    fclose(fp);




    return 0;
}

//====================================================================================
//  rand_str(): creates a random line for text file
//====================================================================================


void rand_str(char *dest, size_t length) {
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    if (length){
	--length;
	size_t i;
	for (i=0; i< length; i++){
	    int key = rand() % (int) (sizeof charset - 1);
	    dest[i] = charset[key];
	}
	dest[length] = '\n';
	dest[length+1] = '\0';
    }
}
//====================================================================================
//   pickRandomLine(): returns a random line of our .txt file
//====================================================================================


char* pickRandomLine(){

    srand(time(0));

    // opening gile to read a random line
    FILE *fp;
    fp = fopen(FILENAME, "r");
    if (fp == NULL){
        printf("Could not open file %s",FILENAME);
        return NULL;
    }

    // counting how many lines the whole file has in order to pick one randomly
    int numberOfLines;
    numberOfLines = countLines();


    char *line_buf = NULL;
    size_t bufsize = 0;

    int lineTaken; // this is the line we are going to take from file
    lineTaken = (getpid())%numberOfLines;
    
    int lineCounter = 0; // this shows the current line 
    while (getline(&line_buf,&bufsize, fp)){
        if (lineCounter == lineTaken)
	    break;
        lineCounter++; 

    }

    fclose(fp); //closing .txt file

    line_buf[strlen(line_buf)-1] = '\0'; // we do this to remove '\n' from the end of the string

    return line_buf;

}

//====================================================================================
//    countLines(): returns the number of lines of our .txt file
//====================================================================================

int countLines(){

    FILE *fp;
    int count = 0;  // this this is the number of lines
    char* filename = FILENAME;
    char c;  // To store a character read from file

    // Open the file
    fp = fopen(filename, "r");

    // Check if file exists
    if (fp == NULL)
    {
        printf("Could not open file %s", filename);
        return 0;
    }

    // Extract characters from file and store in character c
    for (c = getc(fp); c != EOF; c = getc(fp))
        if (c == '\n') // Increment count if this character is newline
            count = count + 1;

    // Close the file
    fclose(fp);

    return count;
}

//====================================================================================
//  str2md5(): takes a string and converts it to md5 hashed string
//  I got infos about how to use MD5 from  here: https://stackoverflow.com/questions/7627723/how-to-create-a-md5-hash-of-a-string-in-c
//====================================================================================

char* str2md5(const char *str, int length){

    int k;
    char *final = (char*)malloc(33);
    unsigned char proc[16];

    MD5_CTX c;
    MD5_Init(&c);

    while (length > 0) {
        if (length > 512) {
            MD5_Update(&c, str, 512);
        } else {
            MD5_Update(&c, str, length);
        }
        length -= 512;
        str += 512;
    }

    MD5_Final(proc, &c);

    for (k = 0; k < 16; ++k) {
        snprintf(&(final[k*2]), 16*2, "%02x", (unsigned int)proc[k]);
    }
    return final;

}
