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

int setupServer(int port) {
    struct sockaddr_in address;
    int server_fd;
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    
    listen(server_fd, 4);

    return server_fd;
}

int acceptClient(int server_fd) {
    int client_fd;
    struct sockaddr_in client_address;
    int address_len = sizeof(client_address);
    client_fd = accept(server_fd, (struct sockaddr *)&client_address, (socklen_t*) &address_len);

    return client_fd;
}

char* convert_int_to_string(int number){
    int m = number;
    int digit = 0;
    while (m) {
        digit++;
        m /= 10;
    }
    char* arr;
    char arr1[digit];
    arr = (char*)malloc(digit);
    int index = 0;
    while (number) {
        arr1[++index] = number % 10 + '0';
        number /= 10;
    }

    int i;
    for (i = 0; i < index; i++) {
        arr[i] = arr1[index - i];
    }
    arr[i] = '\0';
 
    return (char*)arr;
}

void send_players_info_to_client(char* curr_port_str, char* curr_player1_str, char* curr_player2_str, char* is_first_player, int player_index){
    char player_msg[2048] = "";

    strcat(player_msg, curr_port_str);
    strcat(player_msg, SEPARATOR);
    strcat(player_msg, curr_player1_str);
    strcat(player_msg, SEPARATOR);
    strcat(player_msg, curr_player2_str);
    strcat(player_msg, SEPARATOR);
    strcat(player_msg, is_first_player);

    write(player_index, player_msg, sizeof(player_msg));
}

void assign_game_to_players(int curr_player1 ,int curr_player2 ,int game_num)
{
    int curr_port = game_num + GAME_PORT;

    char* curr_port_str = convert_int_to_string(curr_port);
    char* curr_player1_str = convert_int_to_string(curr_player1);
    char* curr_player2_str = convert_int_to_string(curr_player2);
    
    send_players_info_to_client(curr_port_str, curr_player1_str, curr_player2_str, FIRST_PLAYER, curr_player1);
    send_players_info_to_client(curr_port_str, curr_player2_str, curr_player1_str, SECOND_PLAYER, curr_player2);
}

void watch_on_performing_games(int id, int games_num){
    char on_performing_games[2048] = "";

    if(games_num <= 0){
        strcat(on_performing_games, "No game has started yet.\n");
    }
    else{
        for(int i = 0; i < games_num;i++){
            int game_port = GAME_PORT + i;
            char* curr_port = convert_int_to_string(game_port);
            strcat(on_performing_games,curr_port);
            if(i != games_num - 1){
                strcat(on_performing_games,SEPARATOR);
            }
        }
    }
    write(id, on_performing_games, sizeof(on_performing_games));
}

void run_server(int default_port){
    int server_fd, new_socket, max_sd;
    char buffer[2048] = {0};
    fd_set master_set, working_set;
       
    server_fd = setupServer(default_port);

    FD_ZERO(&master_set);
    max_sd = server_fd;
    FD_SET(server_fd, &master_set);

    write(1, "Server is running\n", sizeof("Server is running\n"));

    int games_num = 0;
    int curr_player1 = EMPTY_PLAYER;
    int curr_player2 = EMPTY_PLAYER;

    while (1) {
        working_set = master_set;
        select(max_sd + 1, &working_set, NULL, NULL, NULL);

        for (int i = 0; i <= max_sd; i++) {
            if (FD_ISSET(i, &working_set)) {
                
                if (i == server_fd) { // new client
                    new_socket = acceptClient(server_fd);
                    FD_SET(new_socket, &master_set);
                    if (new_socket > max_sd)
                        max_sd = new_socket;
                    printf("\nNew client connected. fd = %d\n", new_socket);
                }
                
                else { // client sending msg
                    int bytes_received;
                    bytes_received = recv(i , buffer, 2048, 0);
                    
                    if (bytes_received == 0) { // EOF
                        printf("client fd = %d closed\n", i);
                        close(i);
                        FD_CLR(i, &master_set);
                        continue;
                    }

                    buffer[strcspn(buffer, "\n")] = 0;

                    // Commands
                    if(strcmp(buffer, START_GAME) == 0){
                        if(curr_player1 >= 0){
                            curr_player2 = i;

                            assign_game_to_players(curr_player1, curr_player2, games_num);
                            games_num++;
                            
                            curr_player1 = EMPTY_PLAYER;
                            curr_player2 = EMPTY_PLAYER;
                        }
                        else{
                            curr_player1 = i;
                        }

                        printf("client %d: %s\n", i, buffer);
                        memset(buffer, 0, 2048);                        
                    }
                    else if(strcmp(buffer, SHOW_PORTS) == 0){
                        watch_on_performing_games(i,games_num);

                        printf("client %d: %s\n", i, buffer);
                        memset(buffer, 0, 2048);
                    }
                }
            }
        }

    }
}

int main(int argc, char const *argv[]){
    run_server(atoi(argv[1]));
    return 0;
}