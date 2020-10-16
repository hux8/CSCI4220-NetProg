#include "../../../Repo/unpv13e/lib/unp.h"
#include "dict.h"
#include "server.h"
#include <ctype.h>
#include <limits.h>

#define MAX_LEN     1024
#define MAX_CLIENT    5
#define MAX_ATTEMPT 10000

// Choose a random word given a word dictionary and maximum length
// restriction.
char* generateSecretWord(DICT* wordDict,int maxLen)
{
    int n;
    char* chosen;
    int len;
    int safety = 0; // Safety counter to prevent infinite loops
    do
    {
        n = rand() % wordDict->size;
        chosen = wordDict->words[n];
        len = strlen(chosen);
        safety++;
    } while (len > maxLen && safety < MAX_ATTEMPT);
    return chosen;
}

// Will verify if the guess input matches the secret word, as well
// as computing the number of correct letters (countNum)
// and the number of correctly placed letters (countPlace)
// If the length of the secret word and the input don't match
// both counts are -1
// Return 0: guess is incorrect, 1: guess is correct
int verifyGuess(const char* input, const char* secret, int* countPlace, int* countNum) {

    // Check that the word lengths match
    int wordLength = strlen(secret);  
    int i;  
    if (strlen(input) != wordLength) {
        *countPlace = -1;
        *countNum = -1;
        return 0;
    } else {
        // Convert to same case for every letter
        char* convWord = convertStr(secret,FRM_LOWER);
        char* convInput = convertStr(input,FRM_LOWER);
        int tmpcountPlace = 0;    
        for (i = 0; i < wordLength; i++) {
            if (convInput[i] == convWord[i]) {
                tmpcountPlace++;
            }
        }    
        if (tmpcountPlace == wordLength) {
            *countPlace = tmpcountPlace;
            *countNum = 0;
            return 1;
        }
        int tmpCountTotal = 0;  
        int j;  
        for (i = 0; i < wordLength; i++) {
            for (j = 0; j < wordLength; j++) {
                if (convInput[i] == convWord[j]) {
                    tmpCountTotal++;
                    convWord[j] = '*';
                    break;
                }
            }
        }
        *countPlace = tmpcountPlace;
        *countNum = tmpCountTotal;
        
        free(convWord);
        free(convInput);

        return 0;
    }

    
}


void readClientData(SERVER* server,DICT* wordDict,DICT* userDict,char* secretWord,
    char* buffer,fd_set* readFDs,fd_set* allFDs)
{
    int n; // Read bytes

    for (int i = 0; i < MAX_CLIENT; i++) {
        if ( (server->socketFD = server->clientFDs[i]) == 0) {
            continue;
        }
        if (FD_ISSET(server->socketFD, readFDs)) {
            // Read the data
            if ( (n = read(server->socketFD, buffer, MAX_LEN)) == -1) {
                fprintf(stderr, "read error\n");
                exit(EXIT_FAILURE);
            }
            // Check if the client ended connection
            if (n == 0) {
                close(server->socketFD);
                // Update vars
                FD_CLR(server->socketFD, allFDs);

                server->numClients--;
                server->clientFDs[i] = 0;
                free(userDict->words[i]);
                userDict->words[i] = NULL;
            }
            else if (n == 1) {
                continue; // Loose emter keystroke?
            }
            else {
                char* input = newcpy_ext(buffer, n);
                /*  Two conditions:
                        Client has typed a username
                        Client has made a guess
                */
                if (userDict->words[i] == NULL) {
                    // Check to ensure username is ok
                    if (uniqueElement(userDict,input)) {
                        userDict->words[i] = input;

                        // Ask for a guess
                        sprintf(buffer, "Let's start playing, %s\n", input);
                        write(server->socketFD, (void*) buffer, strlen(buffer));

                        sprintf(buffer, "There are %d player(s) playing."
                                " The secret word is %d letter(s).\n"
                                , server->numClients, (int)strlen(secretWord));
                        write(server->socketFD, (void*) buffer, strlen(buffer));

                        continue;
                    } else {
                        sprintf(buffer, "Username %s is already taken,"
                                " please enter a different username\n"
                                , input);
                        write(server->socketFD, (void*) buffer, strlen(buffer));
                    }
                }
                else {
                    int num_T = 0;
                    int num_t = 0;
                    /*  Two conditions:
                            A correct guess
                            An incorrect guess
                    */
                    if (verifyGuess(input, secretWord, &num_T, &num_t)) {
                        sprintf(buffer, "%s has correctly guessed the word %s\n"
                                , userDict->words[i], secretWord);

                        // Broadcast to all clients and disconnect all clients
                        for (int j = 0; j < MAX_CLIENT; j++) {
                            if (server->clientFDs[j] == 0) {
                                continue;
                            }
                            write(server->clientFDs[j], (void*) buffer, strlen(buffer));
                            close(server->clientFDs[j]);
                        }
                        // Mark the end of the game
                        server->gameFlag = 1;

                        // Free the input
                        free(input);

                        break;
                    }
                    else {
                        // Length of input check
                        if (strlen(input) != strlen(secretWord)) {
                            sprintf(buffer, "Invalid guess length."
                                    " The secret word is %d letter(s).\n"
                                    , (int)strlen(secretWord));
                            write(server->socketFD, (void*) buffer, strlen(buffer));
                        } else {
                            sprintf(buffer, "%s guessed %s: %d letter(s) were correct"
                                    " and %d letter(s) were correctly placed.\n"
                                    , userDict->words[i], input, num_t, num_T);

                            // Broadcast to all clients
                            for (int j = 0; j < MAX_CLIENT; j++) {
                                if (server->clientFDs[j] == 0) {
                                    continue;
                                }
                                write(server->clientFDs[j], (void*) buffer, strlen(buffer));
                            }
                        }
                    }
                }
                // Free the input
                free(input);
            }
            // Update and check
            if (--server->numReady == 0) {
                break;
            }
        }
    }
}
void wordCycle(SERVER* server,DICT* wordDict,DICT* userDict,int maxLen)
{
    char* secretWord = generateSecretWord(wordDict,maxLen);
    printf("The secret word is %s\n",secretWord);
    char* buff = (char*) malloc(MAX_LEN);
    // Client and counter vars
    
    int max_fd = server->listenFD;
    
    int i;

    fd_set r_fds, all_fds;
    FD_ZERO(&all_fds);
    FD_SET(server->listenFD, &all_fds);    
    clearDictionary(userDict);
        
    while (!server->gameFlag) {
        // Make use of the select() call
        r_fds = all_fds;
        if ((server->numReady = select(max_fd+1, &r_fds, NULL, NULL, NULL)) < 0) {
            fprintf(stderr, "select error\n");
            exit(EXIT_FAILURE);
        }

        // Check for new client connection
        if (FD_ISSET(server->listenFD, &r_fds)) {
            // Make a new connection
            if ((server->connectFD = accept(server->listenFD, (SA*) NULL, NULL)) < 0) {
                fprintf(stderr, "accept error\n");
                exit(EXIT_FAILURE);
            }
            // Check if the # of clients has reached the limit
            if (server->numClients < MAX_CLIENT) {
                // Ask for a username
                sprintf(buff, "Welcome to Guess the Word, please enter your username.\n");
                write(server->connectFD, (void*) buff, strlen(buff));

                // Update the variables
                FD_SET(server->connectFD, &all_fds);
                if (server->connectFD > max_fd) {
                    max_fd = server->connectFD;
                }
                
                server->numClients++;
                for (int i = 0; i < MAX_CLIENT; i++) {
                    if (server->clientFDs[i] == 0) {
                        server->clientFDs[i] = server->connectFD;
                        break;
                    }
                }
            }
            else {
                // Close the connection
                close(server->connectFD);
            }

            // Update and check
            if (--server->numReady == 0) {
                continue;
            }
        }

        // Check all clients for data
        readClientData(server,wordDict,userDict,secretWord,buff,&r_fds,&all_fds);
    }
    free(buff);
    resetGame(server);
}

int main(int argc, char* argv[]) {

    /* Check the # of command-line arguments, which should have
        ./word_guess.out [seed] [port] [dictionary_file] [longest_word_length]
    */
    if (argc != 5) {
        fprintf(stderr, "command-line argument\n");
        return EXIT_FAILURE;
    }
    // Load the command-line arguments
    unsigned int seed = atoi(argv[1]);
    int port = atoi(argv[2]);    
    DICT* wordDict = importDictionary(argv[3]);
    int maxWordLen = atoi(argv[4]);
    DICT* userDict = createDictionary(MAX_CLIENT);
    SERVER* server = createServer(port);
        
    
    srand(seed);
    
    // MAIN SERVER LOOP
    while (1) {
        wordCycle(server,wordDict,userDict,maxWordLen);        
    }

    
    destroyDictionary(userDict);
    destroyDictionary(wordDict);
    destroyServer(server);

    return EXIT_SUCCESS;
}