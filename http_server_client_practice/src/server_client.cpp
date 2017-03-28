/*
*/
#include "./faci/facility.h"

#define BUFFER_SIZE 1000
#define PORT_NUMBER 1234

// one simple client
int SimpleClient(int argc, char** argv) {

    int socketfd;
    sockaddr_in address;
    char buffer[BUFFER_SIZE], read_buffer[BUFFER_SIZE];

    sprintf(buffer, "this is a string \r\n");

    socketfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&address, 0, sizeof(struct sockaddr_in));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr("127.0.0.1");
    address.sin_port = PORT_NUMBER;
    int result = connect(socketfd, (sockaddr*)&address, sizeof(address));

    exit_if(result == -1, "socket connect fail, please ensure server is online.");
    write(socketfd, &buffer, strlen(buffer));
    read(socketfd, &read_buffer, BUFFER_SIZE);
    close(socketfd);
    exit(0);
}

/* this is a implement for http server using following technic
 * 
 * epoll : pepoll() + application list + round robin scheduling
 */
int SimpleServer(int argc, char** argv) {

    exit(0);
}

/*
 * info client it's a bad request, or client side error
 * */
void response_bad_client(int clientfd) {

}

/*
 * info client it's a server side error
 * */
void response_bad_server(int clientfd) {

}
