#include <stdio.h>
#include <string.h>
#include <time.h>

#include <stdlib.h>

#include "functions.h"


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
    lineTaken = rand()%numberOfLines;
    printf("Linetaken in %d.\n\n", lineTaken);
    
    int lineCounter = 0; // this shows the current line 
    while (getline(&line_buf,&bufsize, fp)){
        if (lineCounter == lineTaken)
	    break;
        lineCounter++; 

    }
    printf("Line is= %s\n", line_buf);

    return line_buf;

}


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
    printf("The file %s has %d lines\n ", filename, count);

    return count;
}
