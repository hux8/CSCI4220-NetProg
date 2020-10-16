#include "dict.h"

// Dictionary Methods -----------------------------
DICT* createDictionary(int count)
{
    DICT* dh = (DICT*) malloc(sizeof(DICT));
    
    dh->words = (char**) calloc(count,sizeof(char*));
    dh->size = count;
    return dh;
}
void modifyDictionary(DICT* dict,int loc,char* word)
{
    if (dict == NULL)
    {
        perror("modifyDictionary() null dictionary handle");
        exit(EXIT_FAILURE);
    } else if (loc < 0 || loc >= dict->size)
    {
        perror("modifyDictionary() invalid location");
        exit(EXIT_FAILURE);
    }
    char* val = newcpy(word);
    strcpy(val,word);
    dict->words[loc] = val; // Dict loc points to val
}

DICT* importDictionary(const char* fp)
{
    FILE* fh = fopen(fp,"r");
    if (!fh)
    {
        // Error opening the file
        perror("importDictionary() Could not open file");
        exit(EXIT_FAILURE);
    }
    // Get number of words
    char* temp = (char*) malloc(1024);
    int count = 0;
    int i;
    while(fgets(temp,1024,fh))
    {
        count++;
    }

    rewind(fh); // Reset cursor
    DICT* dictionary = createDictionary(count);
    // Insert words into dictionary
    for (i = 0; i < count; i++) {
        fgets(temp, 1024, fh); 
        dictionary->words[i] = newcpy(temp);
    }
    return dictionary;
}

void destroyDictionary(DICT* dict)
{
    int i;
    for (i = 0; i < dict->size;i++)
    {
        if (dict->words[i] != NULL)
        {
            free(dict->words[i]);
        }        
    }
    free(dict->words);
    free(dict);
}

int uniqueElement(const DICT* dict,const char* str)
{
    if (dict == NULL || str == NULL)
    {
        perror("uniqueElement() null input");
        exit(EXIT_FAILURE);
    }
    if (strlen(str) < 1)
    {
        perror("uniqueElement() string too short");
        exit(EXIT_FAILURE);
    }
    
    char* cvStr = convertStr(str,FRM_UPPER);
    int i = 0;
    for (i = 0; i < dict->size;i++)
    {
        if (!dict->words[i])
        {
            continue; // NO name stored here
        }
        char* upper = convertStr(dict->words[i],FRM_UPPER);
        if (strcmp(cvStr,upper) == 0)
        {
            free(upper);
            free(cvStr);
            return 0;
        }
        free(upper);
    }
    free(cvStr);
    return 1;
}

// Helper methods ---------------------

char* convertStr(const char* str,int mode)
{
    if (str == NULL || strlen(str) == 0 ||
         (mode != FRM_LOWER && mode != FRM_UPPER))
    {
        // Do nothing
        perror("convertStr() invalid parameters or null");
        exit(EXIT_FAILURE);
    }
    char* result = (char*)malloc(strlen(str) + 1);
    strcpy(result,str);
    int i;
    for (i = 0; i < strlen(str); i++) {
        if (mode == FRM_LOWER)
        {
            result[i] = tolower(result[i]);
        } else {
            result[i] = toupper(result[i]);
        }
        
    }
    return result;
}
char* newcpy(char* orig)
{
    return newcpy_ext(orig,-1);
}
char* newcpy_ext(char* orig,int len)
{
    int al = len;
    if (al < 1)
    {
        al = strlen(orig);
    }
    if (al < 1)
    {
        perror("newcpy() invalid string length");
        exit(EXIT_FAILURE);
    }
    orig[al - 1] = '\0';
    char* newStr = (char*) calloc(al,1);
    strcpy(newStr,orig);
    
    return newStr;
}

