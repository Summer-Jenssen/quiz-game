#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <string.h>


void parse_connect(int argc, char** argv, int* server_fd){
    int opt;
    char* ip_add = "127.0.0.1";
    int port_num = 25555; 

    int fflag = 0;
    int hflag = 0;
    int iflag = 0;
    int pflag = 0;

    int client_fd = -1; 
    char buffer[1024];
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    socklen_t addr_size = sizeof(struct sockaddr_in);
    int max_clients = 2;

    opterr = 0;
    while((opt = getopt(argc, argv, "i:p:h")) != -1) 
    { 
        opterr = 0;
        switch(opt) 
        { 
            case 'h':
                hflag = 1;
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
                if(optopt == 'i' || optopt== 'p'){
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
        printf("   -i IP_address          Default to \"127.0.0.1\";\n");
        printf("   -p port_number         Default to 25555;\n");
        printf("   -h                     Display this help info.\n");
        exit(EXIT_SUCCESS); 
    }


    memset(&client_addr, 0, sizeof(server_addr));
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(port_num);
    client_addr.sin_addr.s_addr = inet_addr(ip_add);

    int check = connect(*server_fd, (struct sockaddr *) &client_addr, addr_size);\
    if(check< 0){
        perror("connect");
        exit(EXIT_FAILURE);
    }

}


 int main(int argc, char *argv[]){

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_fd == -1){
        perror("socket");
        exit(EXIT_FAILURE);
    }

    parse_connect(argc, argv, &server_fd);

    int name_is_set = 0;
    char* one = "1";
    char* two = "2";
    char* three = "3";

    int gameStarted = 0;
    char* gameStartMessage = "The game starts now!";

    while(1){
        fd_set read_fds;
        FD_ZERO(&read_fds);
        char buffer[1024];

        FD_SET(server_fd, &read_fds);
        FD_SET(STDIN_FILENO, &read_fds); //read from stdin! :D
        int max_fd = server_fd;


        int activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        if (activity < 0){
            perror("select");
            exit(EXIT_FAILURE); //shouldn't this be exit failure?
        }

        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            fgets(buffer, sizeof(buffer), stdin);
            buffer[strcspn(buffer, "\n")] = '\0'; //BEGONE! remove the stupid annoying newline character
            if(name_is_set == 0){
                name_is_set = 1;
                int check = write(server_fd, buffer, strlen(buffer));
                if(check < 0){
                    perror("write");
                    exit(EXIT_FAILURE);
                }
            } else if (((strcmp(buffer, one) == 0) || (strcmp(buffer, two) == 0) || (strcmp(buffer, three) == 0)) && gameStarted == 1){
                int check = write(server_fd, buffer, strlen(buffer));
            }else {
                if(gameStarted == 0){
                    printf("The game hasn't started!\n");
                } else {
                printf("%s is not a valid input!\n", buffer);
                }
            }
            
        }

        if (FD_ISSET(server_fd, &read_fds)) {
        
        int n = read(server_fd, buffer, sizeof(buffer) - 1);
        if (n == 0) { //server is gone
            exit(EXIT_SUCCESS);
        } else if (n <0){
            perror("read");
            exit(EXIT_FAILURE);
        }
        buffer[n] = 0;
        if ((strstr(buffer, gameStartMessage) != NULL) && gameStarted == 0){
            gameStarted = 1;
        }
        printf("%s", buffer);
    }

    }
 
    
 }