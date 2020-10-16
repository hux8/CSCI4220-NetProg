#include "../../../Repo/unpv13e/lib/unp.h"
#include "dict.h"
#include <ctype.h>

#define MAX_LEN     1024
#define MAX_CLNT    5


/*  Parameter:
        secret_word -> the secret word
        input -> the guess to be verified
        num_T
        num_t

    Modify:
        num_T -> the # of letters that are correctly placed
        num_t -> the # of letters that are correct

        If the length of the secret word and the input don't match,
    both of the num_T and the num_t are changed to -1.

    Return:
        1 if the guess is correct
        0 if the guess is incorrect
*/
int ok_guess(const char* secret_word, const char* input, int* num_T, int* num_t) {

    int word_len = strlen(secret_word);
    // Check if the length of the secret word and the input match
    if (strlen(input) != word_len) {
        *num_T = -1;
        *num_t = -1;
        return 0;
    }

    // Create a lower case copy of the secret word and the input
    char* low_word = convertStr(secret_word,FRM_LOWER);
    char* low_input = convertStr(input,FRM_LOWER);

    int tmp_T = 0;
    // Count the # of letters that are correctly placed
    for (int i = 0; i < word_len; i++) {
        if (low_input[i] == low_word[i]) {
            tmp_T++;
        }
    }
    // Check if the guess is correct
    if (tmp_T == word_len) {
        *num_T = tmp_T;
        *num_t = 0;
        return 1;
    }

    int tmp_t = 0;
    // Count the # of letters that are correct
    for (int i = 0; i < word_len; i++) {
        for (int j = 0; j < word_len; j++) {
            if (low_input[i] == low_word[j]) {
                tmp_t++;
                low_word[j] = '*';
                break;
            }
        }
    }
    *num_T = tmp_T;
    *num_t = tmp_t;

    // Free the dynamically allocated memory
    free(low_word);
    free(low_input);

    return 0;
}

int main(int argc, char* argv[]) {

    /* Check the # of command-line arguments, which should have
        ./word_guess.out [seed] [port] [dictionary_file] [longest_word_length]
    */
    if (argc != 4) {
        fprintf(stderr, "command-line argument\n");
        return EXIT_FAILURE;
    }
    // Load the command-line arguments
    unsigned int seed = atoi(argv[1]);
    int port = atoi(argv[2]);    
    DICT* wordDict = importDictionary(argv[3]);
    DICT* userDict = createDictionary(MAX_CLNT);
    // Create a buffer
    char* buff = (char*) malloc(MAX_LEN);
    
    // Read the dictionary
    

    // Create the socket
    int lstn_fd, conn_fd, sckt_fd;
    if ( (lstn_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "socket error\n");
        return EXIT_FAILURE;
    }

    // Define the address
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // Setup the server
    if ( (bind(lstn_fd, (SA*) &serv_addr, sizeof(serv_addr))) < 0) {
        fprintf(stderr, "bind error\n");
        return EXIT_FAILURE;
    }
    if ( (listen(lstn_fd, MAX_CLNT)) < 0) {
        fprintf(stderr, "listen error\n");
        return EXIT_FAILURE;
    }
    
    // Set the seed
    srand(seed);

    // Start the game
    while (1) {
        // Select the secret word
        int n = rand() % wordDict->size;
        char* secret_word = wordDict->words[n];
        int word_len = strlen(secret_word);
        printf("The secret work is %s\n",secret_word);
        // Declare and initialize variables to handle multi-client
        int n_ready, n_byte;
        int max_fd = lstn_fd;
        fd_set r_fds, all_fds;
        FD_ZERO(&all_fds);
        FD_SET(lstn_fd, &all_fds);

        int num_clnt = 0;
        int clnt_fd[MAX_CLNT] = {0};
        

        int end_of_game = 0;
        
        while (!end_of_game) {
            // Make use of the select() call
            r_fds = all_fds;
            if ( (n_ready = select(max_fd+1, &r_fds, NULL, NULL, NULL)) < 0) {
                fprintf(stderr, "select error\n");
                return EXIT_FAILURE;
            }

            // Check for new client connection
            if (FD_ISSET(lstn_fd, &r_fds)) {
                // Make a new connection
                if ( (conn_fd = accept(lstn_fd, (SA*) NULL, NULL)) < 0) {
                    fprintf(stderr, "accept error\n");
                    return EXIT_FAILURE;
                }
                // Check if the # of clients has reached the limit
                if (num_clnt < MAX_CLNT) {
                    // Ask for a username
                    sprintf(buff, "Welcome to Guess the Word, please enter your username.\n");
                    write(conn_fd, (void*) buff, strlen(buff));

                    // Update the variables
                    FD_SET(conn_fd, &all_fds);
                    if (conn_fd > max_fd) {
                        max_fd = conn_fd;
                    }
                    
                    num_clnt++;
                    for (int i = 0; i < MAX_CLNT; i++) {
                        if (clnt_fd[i] == 0) {
                            clnt_fd[i] = conn_fd;
                            break;
                        }
                    }
                }
                else {
                    // Close the connection
                    close(conn_fd);
                }

                // Update and check
                if (--n_ready == 0) {
                    continue;
                }
            }

            // Check all clients for data
            for (int i = 0; i < MAX_CLNT; i++) {
                if ( (sckt_fd = clnt_fd[i]) == 0) {
                    continue;
                }
                if (FD_ISSET(sckt_fd, &r_fds)) {
                    // Read the data
                    if ( (n_byte = read(sckt_fd, buff, MAX_LEN)) == -1) {
                        fprintf(stderr, "read error\n");
                        return EXIT_FAILURE;
                    }

                    // Check if the client has closed the connection
                    if (n_byte == 0) {
                        // Close the connection
                        close(sckt_fd);

                        // Update the variables
                        FD_CLR(sckt_fd, &all_fds);

                        num_clnt--;
                        clnt_fd[i] = 0;
                        free(userDict->words[i]);
                        userDict->words[i] = NULL;// ?
                    }
                    // Check if the client just hit enter
                    else if (n_byte == 1) {
                        continue;
                    }
                    else {
                        char* input = newcpy_ext(buff, n_byte);
                        /*  Two conditions:
                                Client has typed a username
                                Client has made a guess
                        */
                        if (userDict->words[i] == NULL) {
                            /*  Two conditions:
                                    A valid username
                                    An invalid username
                            */
                            if (uniqueElement(userDict,input)) {
                                userDict->words[i] = input;

                                // Ask for a guess
                                sprintf(buff, "Let's start playing, %s\n", input);
                                write(sckt_fd, (void*) buff, strlen(buff));

                                sprintf(buff, "There are %d player(s) playing."
                                        " The secret word is %d letter(s).\n"
                                        , num_clnt, word_len);
                                write(sckt_fd, (void*) buff, strlen(buff));

                                continue;
                            }
                            else {
                                sprintf(buff, "Username %s is already taken,"
                                        " please enter a different username\n"
                                        , input);
                                write(sckt_fd, (void*) buff, strlen(buff));
                            }
                        }
                        else {
                            int num_T = 0;
                            int num_t = 0;
                            /*  Two conditions:
                                    A correct guess
                                    An incorrect guess
                            */
                            if (ok_guess( (const char*) secret_word, (const char*) input
                                         , &num_T, &num_t)) {
                                sprintf(buff, "%s has correctly guessed the word %s\n"
                                        , userDict->words[i], secret_word);

                                // Broadcast to all clients and disconnect all clients
                                for (int j = 0; j < MAX_CLNT; j++) {
                                    if (clnt_fd[j] == 0) {
                                        continue;
                                    }
                                    write(clnt_fd[j], (void*) buff, strlen(buff));
                                    close(clnt_fd[j]);
                                }
                                // Mark the end of the game
                                end_of_game = 1;

                                // Free the input
                                free(input);

                                break;
                            }
                            else {
                                /*  Two conditions:
                                        Incorrect length
                                        Correct length
                                */
                                if (strlen(input) != word_len) {
                                    sprintf(buff, "Invalid guess length."
                                            " The secret word is %d letter(s).\n"
                                            , word_len);
                                    write(sckt_fd, (void*) buff, strlen(buff));
                                }
                                else {
                                    sprintf(buff, "%s guessed %s: %d letter(s) were correct"
                                            " and %d letter(s) were correctly placed.\n"
                                            , userDict->words[i], input, num_t, num_T);

                                    // Broadcast to all clients
                                    for (int j = 0; j < MAX_CLNT; j++) {
                                        if (clnt_fd[j] == 0) {
                                            continue;
                                        }
                                        write(clnt_fd[j], (void*) buff, strlen(buff));
                                    }
                                }
                            }
                        }

                        // Free the input
                        free(input);
                    }

                    // Update and check
                    if (--n_ready == 0) {
                        break;
                    }
                }
            }
        }

        
    }

    
    destroyDictionary(userDict);
    destroyDictionary(wordDict);
    free(buff);

    // Shut down the server
    close(lstn_fd);

    return EXIT_SUCCESS;
}