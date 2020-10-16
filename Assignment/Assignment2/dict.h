#ifndef __DICTH
#define __DICTH

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#define FRM_NONE   0x00
#define FRM_LOWER  0x01
#define FRM_UPPER  0x02


// Simple String copy safe

char* newcpy_ext(char* orig,int l);
char* newcpy(char* orig);

// Convert the string into upper or lower case for all
// characters
char* convertStr(const char* str,int mode);

// DICT: Represents a dictionary of words. Can be imported
typedef struct __dict {
    char** words;
    int size;
} DICT;


// Dictionary memory handling methods
DICT* createDictionary(int count); // Create blank dictionary
DICT* importDictionary(const char* fp); // Import dictionary from file data
void modifyDictionary(DICT* dict,int loc,char* word);
void clearDictionary(DICT* dict); // Remove all words from dictionary but keep struct *
void destroyDictionary(DICT* dict); // Destroy dictionary



// Checks to see if a string exists in the dictionary
// Returns 0 if string already exists in list.
// Returns 1 if string is unique in collection

int uniqueElement(const DICT* dict,const char* str);

#endif