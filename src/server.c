#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h> 
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#define BACKLOG 5
#define RECV_SIZE 1024
#define BUF_SIZE 4096
int main()
{
    struct addrinfo  hints, *res;
    int status;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; 
    hints.ai_socktype = SOCK_STREAM; 
    int yes = 1;

    if ( (status = getaddrinfo(NULL, "6969", &hints, &res)) < 0) { 
        fprintf(stderr, "getaddrinfo: %s\n",gai_strerror(status)); 
        return 1;
    }
    
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if( sockfd < 0 ) {
        perror("socket");
        return 1;
    }
    
    printf("socket established...\n");

    if ( setsockopt(sockfd,SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0 ) {
        perror("setsockopt");
        return 1;
    }
    if ( bind(sockfd, res->ai_addr, res->ai_addrlen) < 0 ) {
        perror("bind");
        return 1;
    }
    printf("binded...\n");
    
    if ( listen(sockfd, BACKLOG) < 0 ) { 
        perror("listen");
        return 1;
    }
    printf("Listening on IP 127.0.0.1 on port 6969...\n");
    
    struct sockaddr_storage guest_addr;
    socklen_t addr_size = sizeof(guest_addr);
    int new_fd;
    int bytes_read;
    char *status_msg;
    int bytes_sent;
    FILE *html_file;
    while (1) {
        status_msg = "200 OK";
        new_fd = accept(sockfd, (struct sockaddr *)&guest_addr, &addr_size);
        if (new_fd < 0){
            perror("accept");
            return 1;
        }
    
        printf("accepted...\n\n");
        
        char buf[RECV_SIZE] = {0};

        bytes_read = recv(new_fd, buf, sizeof(buf), 0);
        if (bytes_read > 0) {
            printf("%s\n",buf);
        } else if ( bytes_read == 0 ) {
            fprintf(stderr, "Connection is closed\n");
            close(new_fd);
            continue;
        } else {
            perror("recv");
            return 1;
        }
        char *f = buf + 5;
        *strchr(f, ' ') = 0;
    
        html_file = fopen(f,"rb");
        if(strlen(f) == 0) {
            f = "index.html";
            html_file = fopen(f,"rb"); 
            status_msg = "200 OK";
        }
        if (html_file == NULL) {
            f  = "404.html";
            status_msg = "404 Not Found";
            html_file = fopen(f,"rb");

        }
    
        char header[500];
        struct stat st;
        stat(f, &st);
        sprintf(header,
                "HTTP/1.1 %s\r\n"
                "Content-Type: text/html\r\n"
                "Content-Length: %ld\r\n\r\n",
                status_msg,
                st.st_size);

        char file_data[BUF_SIZE];
        if ( send(new_fd, header, strlen(header), 0) < 0) {
            perror("send");
            return 1;
        }
        while ((bytes_read = fread(file_data, 1, BUF_SIZE, html_file)) > 0) {
            bytes_sent = send(new_fd, file_data, bytes_read, 0);
                if ( bytes_sent < 0) {
                    perror("send");
                    return 1;
                }
            
        }
        fclose(html_file);
        close(new_fd);
        printf("Connection closed...waiting for next guest...\n");
    }
    freeaddrinfo(res);
}
