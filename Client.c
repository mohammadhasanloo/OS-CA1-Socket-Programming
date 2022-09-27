#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include "ConstantValues.h"
#include <string.h>
#include <arpa/inet.h>
#include <sys/time.h>

void DieWithError(char *errorMessage){
    printf("%s",errorMessage);
    exit(0);
}

int connectServer(int port) {
    int fd;
    struct sockaddr_in server_address;
    
    fd = socket(AF_INET, SOCK_STREAM, 0);
    
    server_address.sin_family = AF_INET; 
    server_address.sin_port = htons(port); 
    server_address.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) { // checking for errors
        printf("Error in connecting to server\n");
    }

    return fd;
}

char* draw_curr_table(char table[3][3])
{
    char* table_res = (char*)malloc(16);

    for(int i=0;i<3;i++){
        for(int j=0;j<3;j++){
            strncat(table_res, &table[i][j], 1);
            if(j != 2){
                char space = ' ';
                strncat(table_res, &space, 1);
            }
        }
        char new_line = '\n';
        strncat(table_res, &new_line, 1);
    }
    return table_res;
}

int has_game_finished(char table[3][3]){
    int res = 1;
    for(int i=0;i<3;i++){
        for(int j=0;j<3;j++){
            if(table[i][j] != X && table[i][j] != O){
                res = 0;
                break;
            }
        }
    }
    return res;
}

int has_won(char table[3][3], char X_or_O){
    if((table[0][0] == X_or_O && table[0][1] == X_or_O && table[0][2] == X_or_O ) ||
        (table[1][0] == X_or_O && table[1][1] == X_or_O && table[1][2] == X_or_O ) ||
        (table[2][0] == X_or_O && table[2][1] == X_or_O && table[2][2] == X_or_O ) ||
        (table[0][0] == X_or_O && table[1][1] == X_or_O && table[2][2] == X_or_O ) ||
        (table[0][2] == X_or_O && table[1][1] == X_or_O && table[2][0] == X_or_O ) ||
        (table[0][0] == X_or_O && table[1][0] == X_or_O && table[2][0] == X_or_O ) ||
        (table[0][1] == X_or_O && table[1][1] == X_or_O && table[2][1] == X_or_O ) ||
        (table[0][2] == X_or_O && table[1][2] == X_or_O && table[2][2] == X_or_O )
    ){
        return 1;
    }
    return 0;
}

int has_game_tied(char table[3][3]){
    if(has_won(table, X) == 1){
        return 0;
    }
    else if(has_won(table, O) == 1){
        return 0;
    }
    return 1;
}

int has_chosen_correct_cell(char table[3][3], int selected_cell){
    if(selected_cell > 9 || selected_cell < 1){
        return 0;
    }
    if(table[(selected_cell - 1)/3][(selected_cell - 1)%3] == X ||
    table[(selected_cell - 1)/3][(selected_cell - 1)%3] == O){
        return 0;
    }
    if((table[(selected_cell - 1)/3][(selected_cell - 1)%3]) - '0' != selected_cell){
        return 0;
    }
    return 1;
}

void save_result(char table[3][3]){
    int file_fd;
    char* final_table = draw_curr_table(table);
    strcat(final_table, "\n");
    file_fd = open("results.txt", O_APPEND | O_RDWR);
    write(file_fd, final_table, strlen(final_table));
    close(file_fd);
}

void alarm_handler(int sig){
    printf("Your turn's time has been finished\n");
}

void handle_client1(int game_port,int clientid_1,int clientid_2,int fd,int sock,struct sockaddr_in broadcastAddr){
    int socket_desc;
    struct sockaddr_in server_addr, client_addr;
    char server_msg[2048], client_msg[2048];
    int client_struct_length = sizeof(client_addr);
    
    // Clean buffers:
    memset(server_msg, '\0', sizeof(server_msg));
    memset(client_msg, '\0', sizeof(client_msg));
    
    // Create UDP socket:
    socket_desc = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    
    if(socket_desc < 0){
        printf("Error while creating socket\n");
        return;
    }
    printf("Socket created successfully\n");
    
    // Set port and IP:
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(game_port);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    
    // Bind to the set port and IP:
    if(bind(socket_desc, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
        printf("Couldn't bind to the port\n");
        return;
    }

    // Receive client's message:
    if (recvfrom(socket_desc, client_msg, sizeof(client_msg), 0,
         (struct sockaddr*)&client_addr, &client_struct_length) < 0){
        printf("Couldn't receive\n");
        exit(0);
    }
    char table[3][3];
    int counter = 1;
    for(int i=0;i<3;i++){
        for(int j=0;j<3;j++){
            table[i][j] = counter + '0';
            counter++;
        }
    }

    printf("%s",draw_curr_table(table));  

    strcpy(server_msg, draw_curr_table(table));
    
    // Send the table
    if (sendto(socket_desc, server_msg, strlen(server_msg), 0,
         (struct sockaddr*)&client_addr, client_struct_length) < 0){
        printf("Can't send\n");
        exit(0);
    }

    int my_turn = 1;
    char buff[2048] = "";

    while(1)
    {
        if(has_game_finished(table) == 1){

            if(has_game_tied(table) == 1){
                printf("The game has been tied!\n");
            }
            else if(has_won(table, X) == 1){
                printf("You have won the match\n");
            }
            else if(has_won(table, O) == 1){
                printf("Your opponent has won the match\n");
            }

            memset(server_msg,0,2048);
            strcpy(server_msg, FINISH_MSG);
    
            if (sendto(socket_desc, server_msg, strlen(server_msg), 0,
                (struct sockaddr*)&client_addr, client_struct_length) < 0){
                printf("Can't send\n");
                exit(0);
            }
            save_result(table);
            break;
        }
        else{
            if(has_won(table, X) == 1){
                printf("You have won the match\n");
                
                memset(server_msg,0,2048);
                strcpy(server_msg, FINISH_MSG);
    
                if (sendto(socket_desc, server_msg, strlen(server_msg), 0,
                    (struct sockaddr*)&client_addr, client_struct_length) < 0){
                    printf("Can't send\n");
                    exit(0);
                }
                save_result(table);
                break;                
            }
            else if(has_won(table, O) == 1){
                printf("Your opponent has won the match\n");
                
                memset(server_msg,0,2048);
                strcpy(server_msg, FINISH_MSG);
    
                if (sendto(socket_desc, server_msg, strlen(server_msg), 0,
                    (struct sockaddr*)&client_addr, client_struct_length) < 0){
                    printf("Can't send\n");
                    exit(0);
                }
                save_result(table);
                break;
            }
        }



        if(my_turn == 1){
            int correct_select = 0;
            while(correct_select == 0){
                printf("Your Turn\n");
                memset(buff,0,2048);
                read(0, buff, 2048);
                buff[strcspn(buff, "\n")] = 0;

                int selected_cell = atoi(buff);
                if(has_chosen_correct_cell(table, selected_cell) == 0){
                    printf("Select a valid value\n");
                    printf("\n%s\n",draw_curr_table(table));
                    continue;
                }
                correct_select = 1;
                table[(selected_cell - 1)/3][(selected_cell - 1)%3] = X;


                printf("\n%s\n",draw_curr_table(table));
                memset(buff,0,2048);
                strcpy(server_msg, draw_curr_table(table));
        
                if (sendto(socket_desc, server_msg, strlen(server_msg), 0,
                    (struct sockaddr*)&client_addr, client_struct_length) < 0){
                    printf("Can't send\n");
                    exit(0);
                }


                /* Broadcast sendString in datagram to clients every 3 seconds*/
                if (sendto(sock, server_msg, strlen(server_msg), 0, (struct sockaddr*) 
                    &broadcastAddr, sizeof(broadcastAddr)) != strlen(server_msg))
                    DieWithError("sendto() sent a different number of bytes than expected");

                my_turn = 0;
            }
        }
        else if(my_turn == 0){
            int correct_select = 0;
            printf("Wait for opponent\n");

            while(correct_select == 0){
                memset(client_msg,0,2048);
                if (recvfrom(socket_desc, client_msg, sizeof(client_msg), 0,
                    (struct sockaddr*)&client_addr, &client_struct_length) < 0){
                    printf("Couldn't receive\n");
                    exit(0);
                }

                int selected_cell = atoi(client_msg);
                if(has_chosen_correct_cell(table, selected_cell) == 0){
                    memset(server_msg,0,2048);
                    strcat(server_msg, "Select a valid value\n");
                    if (sendto(socket_desc, server_msg, strlen(server_msg), 0,
                        (struct sockaddr*)&client_addr, client_struct_length) < 0){
                        printf("Can't send\n");
                        exit(0);
                    }
                    continue;
                }
                correct_select = 1;
                table[(selected_cell - 1)/3][(selected_cell - 1)%3] = O;

            }

            int selected_cell = atoi(client_msg);
            table[(selected_cell - 1)/3][(selected_cell - 1)%3] = O;

            printf("\n%s\n",draw_curr_table(table));
            memset(server_msg,0,2048);
            strcpy(server_msg, draw_curr_table(table));
    
            if (sendto(socket_desc, server_msg, strlen(server_msg), 0,
                (struct sockaddr*)&client_addr, client_struct_length) < 0){
                printf("Can't send\n");
                exit(0);
            }

            /* Broadcast sendString in datagram to clients every 3 seconds*/
            if (sendto(sock, server_msg, strlen(server_msg), 0, (struct sockaddr *) 
                &broadcastAddr, sizeof(broadcastAddr)) != strlen(server_msg))
                DieWithError("sendto() sent a different number of bytes than expected");

            my_turn = 1;
        }
    }
    close(socket_desc);
}

void handle_client2(int game_port,int clientid_1,int clientid_2){
    int socket_desc;
    struct sockaddr_in server_addr;
    char server_msg[2048], client_msg[2048];
    int server_struct_length = sizeof(server_addr);
    
    // Clean buffers:
    memset(server_msg, '\0', sizeof(server_msg));
    memset(client_msg, '\0', sizeof(client_msg));
    
    // Create socket:
    socket_desc = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    
    if(socket_desc < 0){
        printf("Error while creating socket\n");
        return;
    }
    printf("Socket created successfully\n");
    
    // Set port and IP:
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(game_port);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    strcpy(client_msg,START_GAME);

    // Send the message to server:
    if(sendto(socket_desc, client_msg, strlen(client_msg), 0,
         (struct sockaddr*)&server_addr, server_struct_length) < 0){
        printf("Unable to send message\n");
        return;
    }

    // Get the table 
    if(recvfrom(socket_desc, server_msg, sizeof(server_msg), 0,
        (struct sockaddr*)&server_addr, &server_struct_length) < 0){
        printf("Error while receiving server's msg\n");
        return;
    }

    printf("\n%s\n", server_msg);

    int my_turn = 0;
    char buff[2048] = "";

    while(1){
        if(my_turn == 0)
        {
            printf("Wait for opponent\n");

            // Get the table
            memset(server_msg,0,2048);
            if(recvfrom(socket_desc, server_msg, sizeof(server_msg), 0,
                (struct sockaddr*)&server_addr, &server_struct_length) < 0){
                printf("Error while receiving server's msg\n");
                return;
            }

            if(strcmp(server_msg,FINISH_MSG) == 0){
                printf("Don't wait...The game has been finished:)\n");
                break;
            }            
            
            printf("\n%s\n", server_msg);

            my_turn = 1;
        }
        else if(my_turn == 1){
            int correct_select = 0;
            while (correct_select == 0){
                printf("Your Turn\n");
                memset(buff,0,2048);
                read(0, buff, 2048);
                buff[strcspn(buff, "\n")] = 0;

                memset(client_msg,0,2048);
                strcpy(client_msg,buff);
                
                if(sendto(socket_desc, client_msg, strlen(client_msg), 0,
                    (struct sockaddr*)&server_addr, server_struct_length) < 0){
                    printf("Unable to send message\n");
                    exit(0);
                }

                memset(server_msg,0,2048);
                
                if(recvfrom(socket_desc, server_msg, sizeof(server_msg), 0,
                    (struct sockaddr*)&server_addr, &server_struct_length) < 0){
                    printf("Error while receiving server's msg\n");
                    exit(0);
                }
                if(strcmp(server_msg, "Select a valid value\n") == 0){
                    printf("%s\n", server_msg);
                    continue;
                }
                correct_select = 1;

                if(strcmp(server_msg,FINISH_MSG) == 0){
                    printf("The game has been finished!\n");
                    exit(0);
                }

                printf("\n%s\n", server_msg);

                my_turn = 0;
            }
            
        }

    }
    close(socket_desc);
}

void make_broadcast_connection(int port, int user_type, int id, int opponent_id, int fd){
    if (user_type == atoi(FIRST_PLAYER)){
        int sock, broadcast = 1, opt = 1;  /* Socket */
        struct sockaddr_in broadcastAddr;  /* Broadcast address */
        char *broadcastIP;                 /* IP broadcast address */
        unsigned short broadcastPort;      /* Server port */
        char *sendString;                  /* String to broadcast */
        int broadcastPermission;           /* Socket opt to set permission to broadcast */
        unsigned int sendStringLen;        /* Length of string to broadcast */

        broadcastIP = "127.0.0.1";            /* First arg:  broadcast IP address */ 
        broadcastPort = port + BROADCAST_PORT_CHANGE_BASE;    /* Second arg:  broadcast port */

        /* Create socket for sending/receiving datagrams */
        if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
            DieWithError("socket() failed");

        setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));    
        
        /* Set socket to allow broadcast */
        broadcastPermission = 1;
        if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (void *) &broadcastPermission, 
            sizeof(broadcastPermission)) < 0)
            DieWithError("setsockopt() failed");

        /* Construct local address structure */
        memset(&broadcastAddr, 0, sizeof(broadcastAddr));   /* Zero out structure */
        broadcastAddr.sin_family = AF_INET;                 /* Internet address family */
        broadcastAddr.sin_addr.s_addr = inet_addr(broadcastIP);/* Broadcast IP address */
        broadcastAddr.sin_port = htons(broadcastPort);         /* Broadcast port */

        handle_client1(port, id, opponent_id, fd, sock, broadcastAddr);

    }
    else{
        handle_client2(port, id, opponent_id);

    }
}

void connect_to_chosen_port(int port){
    int sock, broadcast = 1, opt = 1; /* Socket */
    struct sockaddr_in broadcastAddr; /* Broadcast Address */
    unsigned short broadcastPort;     /* Port */
    char recvString[2048];            /* Buffer for received string */
    int recvStringLen;                /* Length of received string */

    broadcastPort = port + BROADCAST_PORT_CHANGE_BASE;   /* First arg: broadcast port */

    /* Create a best-effort datagram socket using UDP */
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        DieWithError("socket() failed");
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
    setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));    

    /* Construct bind structure */
    memset(&broadcastAddr, 0, sizeof(broadcastAddr));   /* Zero out structure */
    broadcastAddr.sin_family = AF_INET;                 /* Internet address family */
    broadcastAddr.sin_addr.s_addr = htonl(INADDR_ANY);  /* Any incoming interface */
    broadcastAddr.sin_port = htons(broadcastPort);      /* Broadcast port */

    /* Bind to the broadcast port */
    if (bind(sock, (struct sockaddr *) &broadcastAddr, sizeof(broadcastAddr)) < 0)
        DieWithError("bind() failed");

    
    while(1){
        /* Receive a single datagram from the server */
        if ((recvStringLen = recvfrom(sock, recvString, 2048, 0, NULL, 0)) < 0)
            DieWithError("recvfrom() failed");

        recvString[recvStringLen] = '\0';
        printf("%s\n", recvString);    /* Print the received string */
    }
    
    
    close(sock);
}

void print_game_description(int port, int id, int opponent_id, int is_host_player){
    printf("\nDescrption:\n");
    printf("Port = %d\n",port);
    printf("Your id = %d\n",id);
    printf("Opponent id = %d\n",opponent_id);
    if(is_host_player==atoi(FIRST_PLAYER)){
        printf("You are the host, Let's go!\n\n");
    }
    else{
        printf("You are the guest...\n\n");
    }
}

void start_game_fun(int fd, char* buff){
    send(fd, buff, strlen(buff), 0);
    memset(buff, 0, 2048);
    recv(fd, buff, 2048, 0);

    int port, id, opponent_id, user_type;

   char* separate_input = strtok(buff, SEPARATOR);
   for(int i=0; i<PARS_NUM; i++){
       if(i==PORT_LOC){port = atoi(separate_input);}
       else if(i==CURR_PLAYER_LOC){id = atoi(separate_input);}
       else if(i==OPPONENT_PLAYER_LOC){opponent_id = atoi(separate_input);}
       else if(i==IS_STARTER_PLAYER_LOC){user_type = atoi(separate_input);}
       separate_input = strtok(NULL, SEPARATOR);
   }

    memset(buff, 0, 2048);
    print_game_description(port, opponent_id, id, user_type);

    make_broadcast_connection(port, user_type, id, opponent_id, fd);
}

int show_ports(char* buff, int fd){
    send(fd, buff, strlen(buff), 0);
    memset(buff, 0, 2048);
    recv(fd, buff, 2048, 0);

    if(strcmp(buff, "No game has started yet.\n") == 0){
        printf("No game has started yet.\n");
        return 1;
    }

    printf("Choose one port from below list:\n%s\n", buff);
    return 0;
}

void choose_port(char* buff){
    memset(buff, 0, 2048);
    read(0, buff, 2048);
    buff[strcspn(buff, "\n")] = 0;

    int port = atoi(buff);

    connect_to_chosen_port(port);
}

void run_client(int default_port)
{
    int fd;
    char buff[2048] = {0};
    fd = connectServer(default_port);

    printf("Welcome, you entered as client\n");
    printf("Select one command:\n->%s\n->%s\n", START_GAME, SHOW_PORTS);

    while (1){
        memset(buff, 0, 2048);
        read(0, buff, 2048);
        buff[strcspn(buff, "\n")] = 0;
        
        // Commands
        if(strcmp(buff,START_GAME) == 0){
            start_game_fun(fd, buff);
            memset(buff, 0, 2048);
            break;
        }
        else if(strcmp(buff,SHOW_PORTS) == 0){
            int to_continue = show_ports(buff, fd);

            if(to_continue == 1){
                continue;
            }

            choose_port(buff);
            memset(buff, 0, 2048);

            break;
        }
    }
    close(fd);
}

int main(int argc, char const *argv[]) {
    run_client(atoi(argv[1]));
    return 0;
}