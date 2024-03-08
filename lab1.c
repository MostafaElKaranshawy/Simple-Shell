    #include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

#define MAX_LINE_LENGTH 20000
#define MAX_NUM_WORDS 1000

void shell();
FILE *log_file;
FILE *history;
char *globalVariables[10000];
char *gv_values[10000];
int variables_size = 0;
int command_size;


void write_to_file(char *line, FILE *file){
    fprintf(file, "%s", line);
    fclose(file);
}

// Signal handler function
void on_child_exit(int sig) {
    int status;
    pid_t pid;

    char *process = "Child process terminated\n";
    log_file = fopen("logs.txt", "a");
    write_to_file(process, log_file);

    // Reap zombie processes
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        printf("\nChild process %d terminated\n", pid);
    }
}

// Set up environment
void setup_environment() {
    char* line = (char*)malloc(MAX_LINE_LENGTH * sizeof(char));
    if (line == NULL) {
        printf("Memory allocation failed\n");
    }
    line = "/home/mustafa/Desktop/OS";
    chdir(line);
}

// Parent main function
void parent_main() {

    // Register signal handler for SIGCHLD
    signal(SIGCHLD, on_child_exit);

    // Set up environment
    setup_environment();

    // Shell function
    shell();
}


// Replace Command variables with its values Function.
void replace_vars(char *string, const char *substring, const char *replacement){
    char *temp = strstr(string, substring); // Find the first occurrence of the substring
    if (temp == NULL) {
        return; // Substring not found, no replacements needed
    }

    int substringLength = strlen(substring);
    int replacementLength = strlen(replacement);
    int originalLength = strlen(string);
    int lengthDifference = replacementLength - substringLength;

    // Calculate the new length of the string after replacements
    int newLength = originalLength + (strlen(replacement) - strlen(substring)) + 1;

    // Create a temporary buffer to store the modified string
    char buffer[newLength];
    memset(buffer, 0, sizeof(buffer));

    // Copy characters from the original string to the buffer, replacing the substring
    char *p = string;
    while (temp != NULL) {
        // Copy characters before the substring
        strncpy(buffer, p, temp - p);
        buffer[temp - p] = '\0';

        // Concatenate the replacement substring
        strcat(buffer, replacement);

        // Move pointers past the original substring
        p = temp + substringLength;

        // Find the next occurrence of the substring
        temp = strstr(p, substring);
        if (temp == NULL) {
            // Copy the remaining characters from the original string
            strcat(buffer, p);
        }
    }

    // Copy the modified string back to the original string
    strcpy(string, buffer);
}

// Read command function.
char* read_input() {
    char* line = (char*)malloc(MAX_LINE_LENGTH * sizeof(char));
    if (line == NULL) {
        printf("Memory allocation failed\n");
        return NULL;
    }

    // Take input.
    printf("Moka's Shell Command: ");
    fgets(line, MAX_LINE_LENGTH, stdin);

    return line;
}

// Handle quotes Function.
void removeCharFromString(char *str, char ch) {
    int i, j;

    // Iterate through the string
    for (i = 0, j = 0; str[i] != '\0'; i++) {
        // If the current character is not the one to remove
        if (str[i] != ch) {
            str[j] = str[i]; // Move it to the new position
            j++;
        }
    }
    str[j] = '\0'; // Add null terminator to end the new string
}

// Parse input function.
char **parseInput(char *line) {
    char **words = malloc(MAX_NUM_WORDS * sizeof(char *));
    if (words == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }

    char *singleQ = "'";
    char *doubleQ = "\"";
    char *space = " ";
    removeCharFromString(line, '"');
    removeCharFromString(line, '\'');
    char *token = strtok(line, " \n");
    int num_words = 0;
    while (token != NULL && num_words < MAX_NUM_WORDS) {
        words[num_words] = strdup(token);
        if (words[num_words] == NULL) {
            fprintf(stderr, "Memory allocation failed\n");
            exit(1);
        }
        num_words++;
        token = strtok(NULL, " \n");
    }
    words[num_words] = NULL; // Null-terminate the array
    command_size = num_words;
    return words;
}

// Exporting Vaariables function.
void getVar(char *input){
    char *equal = "=";
    char *result = strstr(input, equal);
    if(result == NULL)return;
    char *token, *left, *right;
    token = strtok(input, "="); // Tokenize the string using '=' as delimiter
    if (token != NULL) {
        left = strdup(token); // Store the left part of the string
        token = strtok(NULL, "="); // Move to the next token
        if (token != NULL) {
            right = strdup(token); // Store the right part of the string
        } else {
            right = NULL; // If there's no right part, set it to NULL
        }
    } else {
        left = NULL; // If there's no left part, set it to NULL
    }

    // Invalid export 
    if(left == NULL || right == NULL){
        return;
    }

    // Set variables.
    globalVariables[variables_size] = left;
    gv_values[variables_size] = right;
    variables_size++;
}

// Concatenate Strings Function.
char *concatenateStrings(const char *str1, const char *str2) {

    // Calculate the length of the concatenated string
    size_t len1 = strlen(str1);
    size_t len2 = strlen(str2);
    size_t totalLen = len1 + len2;

    // Allocate memory for the concatenated string (+1 for the null terminator)
    char *result = (char *)malloc((totalLen + 1) * sizeof(char));
    if (result == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    // Copy the first string into the result
    strcpy(result, str1);

    // Concatenate the second string to the end of the result
    strcat(result, str2);

    return result;
}

// Evaluate Expression.
void evaluate_expression(char *string){
    char *space = " ";
    for(int i = 0; i < variables_size; i++){
        char *substring = "$";
        substring = concatenateStrings(substring, globalVariables[i]);
        char *replacement = gv_values[i];

        // check for each varaible in the expression.
        replace_vars(string, substring, replacement);
    }
}

// Execute Shell Built-in functions.
void execute_shell_builtin(char** command){

    // CD Command.
    if(strcmp(command[0], "cd") == 0){
        chdir(command[1]);
    }

    //  Echo Command.
    else if(strcmp(command[0], "echo") == 0){
        for(int i = 1;;i++){
            if(command[i] == NULL)break;
            printf("%s ", command[i]);
        }
        printf("\n");
    }

    // Export Command.
    else if(strcmp(command[0], "export") == 0){
        getVar(command[1]);
        char *space = " ";
        for(int i =2; i < command_size; i++){
            strcat(gv_values[variables_size-1], space);
            strcat(gv_values[variables_size-1], command[i]);
        }
    }
}

// Shell main process.
void shell(){
    bool flag = true;
    do {
        // Take input
        char *line = read_input();

        // Write current command to history file.
        history = fopen("history.txt", "a");
        write_to_file(line, history);

        // Evaluate Expression.
        evaluate_expression(line);

        // Parse input.
        char **command = parseInput(line);

        // Check for empty command.
        if(command_size == 0)continue;

        // Check for Built-in shell commands.
        if(strcmp(command[0], "export") == 0 || strcmp(command[0], "cd") == 0 || strcmp(command[0], "echo") == 0){
            execute_shell_builtin(command);
            continue;
        }

        // Check for exit command.
        if(strcmp(command[0], "exit") == 0){
            flag = false;
            exit(0);
        }

        // Handling Background Processes Execution.
        if(strcmp(command[command_size-1] ,"&") == 0){
            printf("Background\n");
            command[command_size-1] = NULL;

            int pid = fork();

            // Child Process.
            if(pid == 0){
                if(strcmp(command[0], "cd") == 0 || strcmp(command[0], "echo") == 0 || strcmp(command[0], "export") == 0 ){
                    exit(0);
                }
                else{
                    execvp(command[0], command);
                    printf("Error! unsupported command!\n");
                    exit(0);
                }
            }
        }

        // Handling Foreground Processes Execution.
        else{
            int pid = fork();

            // Child Process.
            if(pid == 0){
                if(strcmp(command[0], "cd") == 0 || strcmp(command[0], "echo") == 0 || strcmp(command[0], "export") == 0 ){
                    exit(0);
                }
                else{
                    // if()
                    execvp(command[0], command);
                    printf("Error! unsupported command!\n");
                    exit(0);
                }
            }
            // Parent Process.
            else{
                int status;
                waitpid(pid, &status, 0);
            }
        }
    }
    while(flag);
}

// Main Function.
int main(){
    parent_main();
    return 0;
}