#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_LINE_LENGTH 200
#define MAX_NUM_WORDS 100

char *globalVariables[10000];
char *gv_values[10000];
int variables_size = 0;
int command_size;
char **parseInput() {
    char line[MAX_LINE_LENGTH];
    char **words = malloc(MAX_NUM_WORDS * sizeof(char *));
    if (words == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }

    printf("Moka's_Shell_command: ");
    fgets(line, sizeof(line), stdin);

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
    // printf("%s -> %s",words[num_words], words[num_words-1]);
    command_size = num_words;
    return words;
}
void getVar(char *input){
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
    if(left == NULL || right == NULL){
        return;
    }
    globalVariables[variables_size] = left;
    gv_values[variables_size] = right;
    variables_size++;
    printf("variable name: %s , variable value = %s,  varsSize = %d \n", left, right, variables_size);
}

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
    // printf("\n from replacing:  %s\n", string);
}
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
void evaluate_expression(char *string){
    char *space = " ";
    // printf("var size: %d\n", variables_size);
    for(int i = 0; i < variables_size; i++){
        char *substring = "$";
        // printf("%s\n", globalVariables[i]);
        // strcat(substring,globalVariables[i]);
        substring = concatenateStrings(substring, globalVariables[i]);
        // printf("")
        // substring = (substring, space);
        char *replacement = gv_values[i];
        replace_vars(string, substring, replacement);
    }
    // printf("%s", string);
}
void execute_shell_builtin(char** command){
    if(strcmp(command[0], "cd") == 0){
        chdir(command[1]);
    }
    else if(strcmp(command[0], "echo") == 0){
        for(int i = 1;;i++){
            if(command[i] == NULL)break;
            printf("%s ", command[i]);
        }
        printf("\n");
    }
    else if(strcmp(command[0], "export") == 0){
        getVar(command[1]);
    }
}

void shell(){
    bool flag = true;
    do {
        char **command = parseInput();
        for(int i = 1; i < command_size; i++){
            evaluate_expression(command[i]);
        }
        if(strcmp(command[0], "export") == 0 || strcmp(command[0], "cd") == 0 || strcmp(command[0], "echo") == 0){
            execute_shell_builtin(command);
            continue;
        }
        if(strcmp(command[0], "exit") == 0){
            flag = false;
            exit(0);
        }
        int pid = fork();
        if(pid == 0){
            if(strcmp(command[0], "cd") == 0 || strcmp(command[0], "echo") == 0 || strcmp(command[0], "export") == 0 ){
                exit(0);
            }
            else{
                execvp(command[0], command);
                printf("Error! unsupported command!\n");
            }
            // exit(0);
        }
        else{
            int status;
            waitpid(pid, &status, 0);
        }
    }
    while(flag);
}
int main(){

    shell();
    return 0;
}
