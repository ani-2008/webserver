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

int send_all(int fd, const void *buffer, size_t len)
{
    size_t n;
    const char *p = buffer;
    while (len > 0) {
        n = send(fd,p,len,0);
        if ( n <= 0 ) {
            return -1;
        }
        p += n;
        len -= n;
    }
    return 0;
}

int serve_500(int fd)
{
    char header[500];
    sprintf(header,"HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n");
    send_all(fd,header,strlen(header));
    return 0;
}

int serve_404(int fd)
{
    char header[500];
    struct stat st;
    FILE *html_file = fopen("404.html","r");
    if (html_file == NULL) {
        serve_500(fd);
        return 1;
    }

    stat("404.html",&st);
    sprintf(header, "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\nContent-Length: %ld\r\n\r\n",st.st_size);
    send_all(fd, header, strlen(header));
    
    int bytes_read;
    char file_data[BUF_SIZE];
 
    while ((bytes_read = fread(file_data, 1, BUF_SIZE, html_file)) > 0) {
        if ( send_all(fd,file_data,bytes_read) < 0) {
            perror("send");
        }    
    }
    return 0;
}

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
    
    // create socket where we listen for requests
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if( sockfd < 0 ) {
        perror("socket");
        return 1;
    }
    
    printf("socket established...\n");
    
    // reuse socket if it's already in use
    if ( setsockopt(sockfd,SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0 ) {
        perror("setsockopt");
        return 1;
    }

    // bind to that port 
    if ( bind(sockfd, res->ai_addr, res->ai_addrlen) < 0 ) {
        perror("bind");
        return 1;
    }
    printf("binded...\n");
    
    // start Listening on the IP on that port (localhost:6969)
    if ( listen(sockfd, BACKLOG) < 0 ) { 
        perror("listen");
        return 1;
    }
    printf("Listening on IP 127.0.0.1 on port 6969...\n");
    
    struct sockaddr_storage guest_addr;
    socklen_t addr_size = sizeof(guest_addr);
    int new_fd;
    int bytes_read;
    FILE *html_file;
    
    // main loop where we accept new requests and close the new connection after their request was satisfied
    while (1) {
        new_fd = accept(sockfd, (struct sockaddr *)&guest_addr, &addr_size);
        addr_size = sizeof(guest_addr);

        if (new_fd < 0){
            perror("accept");
            freeaddrinfo(res);
            close(sockfd);
            return 1;
        }
    
        printf("accepted...\n\n");
        
        char buf[RECV_SIZE] = {0};

        bytes_read = recv(new_fd, buf, sizeof(buf), 0);   // get request
        if (bytes_read > 0) {
            printf("%s\n",buf);
        } else if ( bytes_read == 0 ) {
            fprintf(stderr, "Connection is closed\n");
            close(new_fd);
            continue;
        } else {
            perror("recv");
            freeaddrinfo(res);
            close(sockfd);
            return 1;
        }
        char *f = buf + 5;
        *strchr(f, ' ') = 0;

        if(strstr(f,"..")) {
            printf("..hello\n");
            serve_404(new_fd);
            close(new_fd);
            continue;
        }
        html_file = fopen(f,"rb"); // read the file they want 
        if(strlen(f) == 0) {
            f = "index.html";
            html_file = fopen(f,"rb");
        }else if (html_file == NULL) {
            serve_404(new_fd);
            close(new_fd);
            continue;
        }
        char header[500];

        if (html_file == NULL) {
            serve_404(new_fd);
            close(new_fd);
            continue;
        }

        struct stat st;
        // get file size and other stats of the file we need
        stat(f, &st);
        sprintf(header,
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/html\r\n"
                "Content-Length: %ld\r\n\r\n",
                st.st_size);

        char file_data[BUF_SIZE];
        
        // Send header first
        if ( send_all(new_fd, header, strlen(header)) < 0) {
            perror("send");
            freeaddrinfo(res);
            close(sockfd);
            return 1;
        }

        // send the file that's requested
        while ((bytes_read = fread(file_data, 1, BUF_SIZE, html_file)) > 0) {
            if ( send_all(new_fd,file_data,bytes_read) < 0) {
                perror("send");
                freeaddrinfo(res);
                close(sockfd);
                return 1;
            }    
        }

        fclose(html_file);
        close(new_fd);
        printf("Connection closed...waiting for next guest...\n");
    }
    freeaddrinfo(res);
    close(sockfd);

}
