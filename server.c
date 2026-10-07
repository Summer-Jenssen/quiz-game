#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <string.h>

  struct Entry {
        char prompt[1024];
        char options[3][50];
        int answer_idx;
    };

    struct Player {
        int fd;
        int score;
        char name[128];
    };


/* Segments char* string and puts the result in char* input[1024]. 
tokenizeBy is the character that separates sections.*/
void tokenize_string(char* string, const char* tokenizeBy, char* input[1024]){  //cd f hi -> [["cd"] ["f"] ["hi"]]
    // char *output[100];
    int currOutputIndex = 0;
    // char* curr;
    // char currToken[1024] = "";
    char* token = strtok(string, tokenizeBy);
    int index = 0;

    for (int i = 0; token != NULL; i++){
        input[i] = token;
        token = strtok(NULL, tokenizeBy);
        index = i; 
    }
    input[index+1]= NULL;
}




int read_questions(struct Entry* arr, char* filename){

    FILE* fp = fopen(filename, "r");
    if (fp == NULL) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }
    char buffer[1024];
    int lineNum = 0;
    int entryNum = 0;
    const char tokenize_by = ' ';
    char* tokenized_string[1024];

    while (fgets(buffer, 1024, fp)) {
        buffer[strcspn(buffer, "\n")] = '\0'; 
        if(lineNum%4==0){ //question!
            strcpy(arr[entryNum].prompt, buffer);
            lineNum++;
        }
        else if(lineNum%4==1){ //answers
            tokenize_string(buffer, &tokenize_by, tokenized_string);
            strcpy(arr[entryNum].options[0], tokenized_string[0]);
            strcpy(arr[entryNum].options[1], tokenized_string[1]);
            strcpy(arr[entryNum].options[2], tokenized_string[2]);
            lineNum++;
        }
        else if(lineNum%4 ==2){ //correct answer
            for(int i = 0; i< 3; i++){
                if(strcmp(buffer, arr[entryNum].options[i]) == 0){
                    arr[entryNum].answer_idx = i;
                }
            }
            entryNum++;
            lineNum++;
        }  else {
            lineNum++;
        }

        //otherwise the line is empty and separating two questions, so do nothing. 
    }
    return entryNum;

}

int main(int argc, char *argv[]){

    int opt;
    char* question_file = "qshort.txt";
    char* ip_add = "127.0.0.1";
    int port_num = 25555; 

    int fflag = 0;
    int hflag = 0;
    int iflag = 0;
    int pflag = 0;

    int server_fd;
    int client_fd = -1; 
    char buffer[1024];
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    socklen_t addr_size = sizeof(struct sockaddr_in);
    int max_clients = 2;

    int gameStarted = 0;
    char* emptyname = "";
    int last_question_was_answered = 1; //used to track if a question was answered in case interrupted by a new player trying to join
    int current_question_index = -1; //init to -1 bc every question increments it, including the first. 
    int max_num_questions = 0;
    char* gameStartMessage = "The game starts now!\n";

    opterr = 0;
    while((opt = getopt(argc, argv, "f:i:p:h")) != -1) 
    { 
        opterr = 0;
        switch(opt) 
        { 
            case 'h':
                hflag = 1;
                break;
            case 'f':
                question_file = optarg;
                fflag = 1;
                break;
            case 'i':

                iflag = 1;
                ip_add = optarg;
                break;
            case 'p':
                pflag = 1;
                port_num = atoi(optarg);
                break;
            case '?':
                if(optopt == 'i' || optopt == 'f' || optopt== 'p'){
                    fprintf(stderr, "Error: Option '-%c' requires an argument.\n", optopt);
                } else {
                    fprintf(stderr, "Error: Unknown option '-%c' received.\n", optopt);
                }
                exit(EXIT_FAILURE);
                break;
        } 
    } 

  
    if(hflag == 1){
        printf("Usage: %s [-f question_file] [-i IP_address] [-p port_number] [-h]\n\n", argv[0]);
        printf("   -f question_file       Default to \"qshort.txt\";\n");
        printf("   -i IP_address          Default to \"127.0.0.1\";\n");
        printf("   -p port_number         Default to 25555;\n");
        printf("   -h                     Display this help info.\n");
        exit(EXIT_SUCCESS); 
    }


    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_fd == -1){
        perror("socket");
        exit(EXIT_FAILURE);
    }


    int reuse = 1;
    int c = setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)); //needs to be here because if you hit ctrl+c 
    //to cancel program, the socket isn't properly closed so you need to tell it to reuse it
    if(c < 0){
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_num);
    server_addr.sin_addr.s_addr = inet_addr(ip_add);
    int check = 0;


    check = bind(server_fd, (struct sockaddr *)&server_addr, addr_size);
    if(check == -1){
        perror("bind");
        exit(EXIT_FAILURE);
    }

    check = 0;
    check = listen(server_fd, max_clients);
    if(check == -1){
        perror("listen");
        exit(EXIT_FAILURE);
    } else {
        printf("Welcome to 392 Trivia!\n");
    }



    //Reading questions from file

    struct Entry arr[50];
    max_num_questions = read_questions(arr, question_file);

    



    //Accepting players

    addr_size = sizeof(client_addr);

    int client_fds[max_clients + 1]; 

    for(int i =0; i< max_clients; i++){
        client_fds[i] = -1; 
    }
    int num_clients = 0;


    struct Player player_arr[max_clients]; //saves player details! 
    for (int j = 0; j < max_clients; j++){
        player_arr[j].fd = -1;
        player_arr[j].score = 0;
        strcpy(player_arr[j].name, emptyname);
    }


    
    while(1){ 





        fd_set read_fds; 
        FD_ZERO(&read_fds);

        FD_SET(server_fd, &read_fds);
        int max_fd = server_fd;

        for(int i = 0; i<max_clients; i++){
            if(client_fds[i] != -1){
                FD_SET(client_fds[i], &read_fds);
                if (client_fds[i] > max_fd){
                    max_fd = client_fds[i];
                }
            }
        }



        if(gameStarted == 1){ //game logic

            if(last_question_was_answered == 1){ //this means it's ok to print a new question!
                last_question_was_answered = 0;
                current_question_index++;
                if(current_question_index == max_num_questions){//display scores, end game. 


                    char* winners[max_clients]; 
                    int curr_winner_index =0;
                    int max = player_arr[0].score;
                    char output[1024] = "Congrats, ";
                    int num_winners = 0;

                    for(int i = 0; i < max_clients; i++){
                        if (player_arr[i].score > max){
                            max = player_arr[i].score;
                        }
                    }

                    for (int i = 0; i < max_clients; i++){ //count number of winners
                        if(player_arr[i].score == max){
                            num_winners++;
                        }
                    }
                    int count = 0;
                    for(int i = 0; i < max_clients; i++){
                        if(player_arr[i].score == max){
                            count++;
                            winners[curr_winner_index] = player_arr[i].name;
                            strcat(output, winners[curr_winner_index]);
                            curr_winner_index++;
                            if(count != num_winners){
                            char* comma = ", ";
                            strcat(output, comma);
                            }
                            
                        }
                    }

                    printf("%s!\n", output);
                    for (int j = 0; j < max_clients; j++) {
                        if (client_fds[j] != -1) {
                            close(client_fds[j]);
                        }
                    }
                    exit(EXIT_SUCCESS);










                } else {
                    int val = current_question_index + 1;
                    struct Entry temp = arr[current_question_index]; //this code looks really long without these substitutions
                    printf("Question %d: %s\n", val, temp.prompt);
                    printf("1: %s\n", temp.options[0]);
                    printf("2: %s\n", temp.options[1]);
                    printf("3: %s\n", temp.options[2]);


                    char print_to_clients[2048];

                    sprintf(print_to_clients, "Question %d: %s\nPress 1: %s\nPress 2: %s\nPress 3: %s\n", val, temp.prompt, 
                        temp.options[0], temp.options[1], temp.options[2]);


                    for(int i = 0; i< max_clients; i++){
                        int check = write(client_fds[i], print_to_clients, strlen(print_to_clients));
                        if (check < 0){
                            perror("write");
                            continue;
                        }
                    }

                }
            }


        }

        int activity = select(max_fd +1, &read_fds, NULL, NULL, NULL);
        if (activity < 0){
            perror("select");
            exit(EXIT_FAILURE);
        }


        //first check if the server fd was triggered
        if (FD_ISSET(server_fd, &read_fds)) { //kick a client, already full.
            if (gameStarted == 1 || num_clients == max_clients) {
                int reject_fd = accept(server_fd, (struct sockaddr *) &client_addr, &addr_size);
                close(reject_fd);
                printf("Max connection reached!\n");
            } else { //accept them!
                addr_size = sizeof(client_addr);
                int new_fd = accept(server_fd, (struct sockaddr *) &client_addr, &addr_size);
                // find an empty slot
                if(new_fd < 0){
                    perror("accept");
                    for (int j = 0; j < max_clients; j++) {
                        if (client_fds[j] != -1) {
                            close(client_fds[j]);
                        }
                    }
                    exit(EXIT_FAILURE);
                }
                for (int i = 0; i < max_clients; i++) {
                    if (client_fds[i] == -1) {
                        client_fds[i] = new_fd;
                        break;
                    }
                }
                num_clients++;


                for(int j = 0; j < max_clients; j++){
                    if (player_arr[j].fd == -1){
                        player_arr[j].fd = new_fd;
                        break;
                    }
                }

                char* enter_name = "Please type your name:\n";
                int check = write(new_fd, enter_name, strlen(enter_name));
                if (check < 0){
                    perror("write");
                    continue;
                    
                }
                if(num_clients == max_clients){
                        printf("Max connection reached!\n"); //ask if this is correct or to only print when a player tries to join while at the max number already
                } else {
                    printf("New connection detected!\n");
                }
                continue;
            }
        }







        //otherwise we check which player triggered select. 
        for(int i = 0; i< max_clients; i++){


            if(client_fds[i] == -1) {
                continue;
            }
            if (!FD_ISSET(client_fds[i], &read_fds)) continue;

            int n = read(client_fds[i], buffer, sizeof(buffer)-1);
            if(n==0){
                printf("Lost connection!\n");
                for (int j = 0; j < max_clients; j++) {
                    if (client_fds[j] != -1) {
                        close(client_fds[j]);
                    }
                }
                close(server_fd);
                exit(EXIT_FAILURE);
            } else if (n<0){
                perror("read");
                close(client_fds[i]);
                client_fds[i] = -1;
                for (int j = 0; j < max_clients; j++) {
                    if (client_fds[j] != -1) {
                        close(client_fds[j]);
                    }
                }
                close(server_fd);
                exit(EXIT_FAILURE);
                
            } else {
                buffer[n] = 0;


                if(strcmp(player_arr[i].name, emptyname) == 0){ //case for player entering their name
                    strcat(player_arr[i].name, buffer);
                    printf("Hi %s!\n", buffer);

                    int all_clients_named = 1;

                    for(int j =0; j < max_clients; j++){
                        if(strcmp(player_arr[j].name, emptyname) == 0){
                            all_clients_named = 0; 
                            break;                            
                        }
                    }

                    if((all_clients_named == 1) && (num_clients == max_clients)){
                        gameStarted = 1;
                        printf("The game starts now!\n");

                        for(int j = 0; j < max_clients; j++){
                            int check = write(client_fds[j], gameStartMessage, strlen(gameStartMessage));
                            if (check < 0){
                                perror("write");
                                continue;
                            }
                        }
                    }

                } else if (gameStarted == 1){ //case where playeer is answering a question
                    last_question_was_answered = 1;
                    
                     
                        if (atoi(&buffer[0]) -1 == arr[current_question_index].answer_idx){
                            player_arr[i].score++;
                            printf("\nCorrect!\n");
                        } else {
                            player_arr[i].score--;
                            printf("\n*LOUD INCORRECT BUZZER NOISES*\n");
                        }

                    
                    char answer[1024] = "The answer was: ";
                    switch(arr[current_question_index].answer_idx){
                        case 0:
                        strcat(answer, arr[current_question_index].options[0]);
                        break;
                        case 1:
                        strcat(answer, arr[current_question_index].options[1]);
                        break;
                        case 2:
                        strcat(answer, arr[current_question_index].options[2]);
                        break;
                    }
                    char* newline = "\n\n";
                    strcat(answer, newline);
                    printf("%s", answer);

                    for(int j =0; j < max_clients; j++){ //NOW we send the answer
                        int check = write(client_fds[j], answer, strlen(answer));
                        if (check < 0){
                            perror("write");
                            continue;
                    
                        }
                    }
                    

                }
            }
        }
    
    }


}