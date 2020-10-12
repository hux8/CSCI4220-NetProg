#include "../unpv13e/lib/unp.h"
#include <ctype.h>

#define MAX_LEN     1024
#define MAX_CLNT    5

char* buff;
int lstn_fd, conn_fd, sckt_fd;
int num_clnt, n_ready, ser_max_length;

char* new_str(char* buff, int len) {
    
    buff[len-1] = '\0';

    char* str = (char*) malloc(len);
    strcpy(str, buff);
    
    return str;
}

/*  Parameter
        STR -> a string

    Modify:

    Return:
        A pointer to the newly dynamically allocated string in lower case
*/
char* low_str(const char* STR) {

    int len = strlen(STR);
    char* str = (char*) malloc(len + 1);
    strcpy(str, STR);

    for (int i = 0; i < len; i++) {
        str[i] = tolower(str[i]);
    }

    return str;
}

/*  Parameter:
        dict_file -> the dictionary file
        dict_size

    Modify:
        dict_size -> the actual size of the dictionary

    Return:
        A pointer to the newly dynamically allocated dictionary
*/
char** read_dict(char* dict_file, int* dict_size) {

    // Open the dictionary
    FILE* fptr = fopen(dict_file, "r");

    // Create a buffer
    char* buff = (char*) malloc(MAX_LEN);

    // Count the # of words
    int num_word = 0;
    while (fgets(buff, MAX_LEN, fptr) != NULL) {
        num_word++;
    }
    rewind(fptr);

    // Read the dictionary
    char** dict = (char**) malloc(num_word*sizeof(char*));
    int word_len;
    for (int i = 0; i < num_word; i++) {
        fgets(buff, MAX_LEN, fptr); 
        word_len = strlen(buff);
        dict[i] = new_str(buff, word_len);
    }
    *dict_size = num_word;

    // Close the dictionary
    fclose(fptr);
    // Free the buffer
    free(buff);

    return dict;
}

/*  Parameter:
        clnt_name -> all usernames that are currently being used
        input -> the username to be verified

    Modify:

    Return:
        1 if no one is using this username
        0 if someone is using this username
*/
int ok_name(const char** clnt_name, const char* input) {

    // Create a lower case copy of the input
    char* low_input = low_str(input);

    // Compare each username
    for (int i = 0; i < MAX_CLNT; i++) {
        // Check if there exist a username
        if (clnt_name[i] == NULL) {
            continue;
        }
        // Create a lower case copy of the client's username
        char* low_name = low_str((const char*) clnt_name[i]);
 
        if (strcmp(low_name, low_input) == 0) {
            // Free the dynamically allocated memory
            free(low_name);
            free(low_input);
            
            return 0;
        }

        // Free the copy of the client's username
        free(low_name);
    }

    // Free the copy of the input
    free(low_input);

    return 1;
}

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
    char* low_word = low_str(secret_word);
    char* low_input = low_str(input);

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

int check_client(char* secret_word, char** clnt_name, int* clnt_fd, fd_set r_fds, fd_set all_fds){
  	int n_byte;
    int word_len = strlen(secret_word);
    
		for (int i = 0; i < MAX_CLNT; i++)
   	{
     	if ((sckt_fd = clnt_fd[i]) == 0)
     	{
        	continue;
     	}

     if (FD_ISSET(sckt_fd, &r_fds))
     {
        // Read the data
       if ((n_byte = read(sckt_fd, buff, MAX_LEN)) == -1)
       {
         fprintf(stderr, "read error\n");
         return EXIT_FAILURE;
       }

        // Check if the client has closed the connection
       if (n_byte == 0)
       {
          // Close the connection
         close(sckt_fd);

          // Update the variables
         FD_CLR(sckt_fd, &all_fds);

         num_clnt--;
         clnt_fd[i] = 0;
         free(clnt_name[i]);
         clnt_name[i] = NULL;
       }

        // Check if the client just hit enter
       else if (n_byte == 1)
       {
         continue;
       }
       else
       {
         char *input = new_str(buff, n_byte);

         if (clnt_name[i] == NULL)
         {
           if (ok_name((const char **) clnt_name, (const char *) input))
           {
             clnt_name[i] = input;

              // Ask for a guess
             sprintf(buff, "Let's start playing, %s\n", input);
             write(sckt_fd, (void*) buff, strlen(buff));

             sprintf(buff, "There are %d player(s) playing."
               " The secret word is %d letter(s).\n", num_clnt, word_len);
             write(sckt_fd, (void*) buff, strlen(buff));

             continue;
           }
           else
           {
             sprintf(buff, "Username %s is already taken,"
               " please enter a different username\n", input);
             write(sckt_fd, (void*) buff, strlen(buff));
           }
         }
         else
         {
           int num_T = 0;
           int num_t = 0;
           /* Two conditions:
                   A correct guess
                   An incorrect guess
           */
           if (ok_guess((const char *) secret_word, (const char *) input, &num_T, &num_t))
           {
             sprintf(buff, "%s has correctly guessed the word %s\n", clnt_name[i], secret_word);

              // Broadcast to all clients and disconnect all clients
             for (int j = 0; j < MAX_CLNT; j++)
             {
               if (clnt_fd[j] == 0)
               {
                 continue;
               }

               write(clnt_fd[j], (void*) buff, strlen(buff));
               close(clnt_fd[j]);
             }

             free(input);

             return 2;
           }
           else
           {
             /* Two conditions:
                     Incorrect length
                     Correct length
             */
             if (strlen(input) != word_len)
             {
               sprintf(buff, "Invalid guess length."
                 " The secret word is %d letter(s).\n", word_len);
               write(sckt_fd, (void*) buff, strlen(buff));
             }
             else
             {
               sprintf(buff, "%s guessed %s: %d letter(s) were correct"
                 " and %d letter(s) were correctly placed.\n", clnt_name[i], input, num_t, num_T);

                // Broadcast to all clients
               for (int j = 0; j < MAX_CLNT; j++)
               {
                 if (clnt_fd[j] == 0)
                 {
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
       if (--n_ready == 0)
       {
         return 1;
       }
     }
   }
	return 0;
}


int main(int argc, char* argv[]) {
  	// Variables for arguments
    unsigned int seed;
    int port;
    char* dict_file;
  	
  	// Varibles for word dictionary
    int dict_size = 0;
    char** dict;

  	// Read in the arguments
  	seed = atoi(argv[1]);
  	port = atoi(argv[2]);
  	dict_file = argv[3];
    ser_max_length = atoi(argv[4]);

    buff = (char*) calloc(MAX_LEN,sizeof(char));

  
  	// Random
    srand(seed);

  	
  	// Read the words into dict
  	dict = read_dict(dict_file, &dict_size);
      
    // Start a socket
	  lstn_fd = socket(PF_INET, SOCK_STREAM, 0);


    // Define the address
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // Start by binding and listening on the port 
    bind(lstn_fd, (SA*) &serv_addr, sizeof(serv_addr));

  
  	listen(lstn_fd, MAX_CLNT);

    
    // While loop that simulates each game.
    // Each round of the game has a random word for players to guess.
    // The server will keep listening on the port for additional players
    // Each game will not end until the word is guessed out
    while (1) {
        // Choosing the secret word by random and mod
        int n = rand() % dict_size;
        char* secret_word = dict[n];
        while (strlen(secret_word) > ser_max_length)
        {
          n = rand() % dict_size;
          secret_word = dict[n];
        }
        fprintf(stderr, "DEBUG: secrect word is:\n\t %s\n", secret_word);

        // Declare and initialize variables to handle multi-client
        
        int skip = 0;
        int max_fd = lstn_fd;
        fd_set r_fds, all_fds;
        FD_ZERO(&all_fds);
        FD_SET(lstn_fd, &all_fds);

        num_clnt = 0;
        int clnt_fd[MAX_CLNT] = {0};
        char** clnt_name = (char**) calloc(MAX_CLNT, sizeof(char*));
        for (int i = 0; i < MAX_CLNT; i++) {
            clnt_name[i] = NULL;
        }

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
                
              	if(num_clnt >= MAX_CLNT){
                  conn_fd = accept(lstn_fd, (SA*) NULL, NULL);
                   close(conn_fd);
                }
                // Check if the # of clients has reached the limit
                else {
                    // Ask for a username
                    if ( (conn_fd = accept(lstn_fd, (SA*) NULL, NULL)) < 0) {
                      fprintf(stderr, "accept error\n");
                      return EXIT_FAILURE;
                    }
                  
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

                // Update and check
                if (--n_ready == 0) {
                    skip = 1;
                }
            }
        }

            // Check all clients for data
      	if(!skip) {
        		int response = check_client(secret_word, clnt_name, clnt_fd, r_fds, all_fds);

            if (response == 2) {
               end_of_game = 1;
            }
          	// Free the dynamically allocated memory
            for (int i = 0; i < MAX_CLNT; i++) {
                free(clnt_name[i]);
            }
            free(clnt_name);
        }
    }

    // Free the dynamically allocated memory
    for (int i = 0; i < dict_size; i++) {
        free(dict[i]);
    }
    free(dict);
    free(buff);

    // Shut down the server
    close(lstn_fd);

    return EXIT_SUCCESS;
}
