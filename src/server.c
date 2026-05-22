#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h> 
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <time.h> 

#define BACKLOG 5
#define RECV_SIZE 1024
#define BUF_SIZE 4096

char cur_time[26];
time_t t;

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

int serve_500(int fd, char *requests)
{
    t = time(NULL);
    char header[500];
    snprintf(header, sizeof(header),
            "HTTP/1.1 500 Internal Server Error\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 27\r\n\r\n"
            "500 Internal Server Error\r\n");
    send_all(fd,header,strlen(header));

    strftime(cur_time, 26, "%Y-%m-%d %H:%M:%S", localtime(&t));
    printf("[%s] 127.0.0.1 6969 %s 500 Internal Server Error\n",cur_time,requests);
    return 0;
}

int serve_404(int fd,char *requests, char *requested_file)
{
    t = time(NULL);
    char header[500];
    struct stat st;
    FILE *html_file = fopen("pages/404.html","rb");
    if (html_file == NULL) {
        serve_500(fd,requests);
        return 1;
    }

    stat("pages/404.html",&st);
    snprintf(header,sizeof(header),"HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\nContent-Length: %ld\r\n\r\n",st.st_size);
    send_all(fd, header, strlen(header));
    
    int bytes_read;
    char file_data[BUF_SIZE];
    strftime(cur_time, 26, "%Y-%m-%d %H:%M:%S", localtime(&t));
    printf("[%s] 127.0.0.1 6969 %s %s 404 Not Found\n",cur_time,requests,requested_file);

    while ((bytes_read = fread(file_data, 1, BUF_SIZE, html_file)) > 0) {
        if ( send_all(fd,file_data,bytes_read) < 0) {
            perror("send");
            fclose(html_file);
            return 1;
        }    
    }
    return 0;
}

int serve_405(int fd, char *requests)
{
    t = time(NULL);
    strftime(cur_time, 26, "%Y-%m-%d %H:%M:%S", localtime(&t));
    printf("[%s] 127.0.0.1 6969 %s 405 Method Not Allowed\n",cur_time,requests);
    char header[500];
    snprintf(header,sizeof(header),
            "HTTP/1.1 405 Method Not Allowed\r\n"
            "Allow: GET\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 25\r\n\r\n"
            "405 Method Not Allowed\r\n");
    if ( send_all(fd, header, strlen(header)) < 0) {
        perror("send");
        return 1;
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
    t = time(NULL);
    strftime(cur_time, 26, "%Y-%m-%d %H:%M:%S", localtime(&t));
    printf("[%s] socket established\n",cur_time);
    
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
    t = time(NULL);
    strftime(cur_time, 26, "%Y-%m-%d %H:%M:%S", localtime(&t));
    printf("[%s] Binded\n",cur_time);

    // start Listening on the IP on that port (localhost:6969)
    if ( listen(sockfd, BACKLOG) < 0 ) { 
        perror("listen");
        return 1;
    }
    t = time(NULL);
    strftime(cur_time, 26, "%Y-%m-%d %H:%M:%S", localtime(&t));
    printf("[%s] listening on 127.0.0.1 6969\n",cur_time);

    struct sockaddr_storage guest_addr;
    socklen_t addr_size = sizeof(guest_addr);
    int new_fd;
    int bytes_read;
    FILE *html_file = NULL;
    
    // main loop where we accept new requests and close the new connection after their request was satisfied
    
    while (1) {
        t = time(NULL);

        new_fd = accept(sockfd, (struct sockaddr *)&guest_addr, &addr_size);
        addr_size = sizeof(guest_addr);

        if (new_fd < 0){
            perror("accept");
            freeaddrinfo(res);
            close(sockfd);
            return 1;
        }
        
        strftime(cur_time, 26, "%Y-%m-%d %H:%M:%S", localtime(&t));
        printf("[%s] accepted\n",cur_time);

        char buf[RECV_SIZE] = {0};

        bytes_read = recv(new_fd, buf, sizeof(buf), 0);   // get request
        
        char *requests = buf;
        char *space_1 = strchr(requests,' ');
        if (space_1 == NULL) {
            close(new_fd);
            continue;
        }
        *space_1 = 0;

        char *requested_file = buf + 4;
        char *file_name = buf + 5;
        char *space_2 = strchr(file_name, ' ');
        if (space_2 == NULL) {
            close(new_fd);
            continue;
        }
        *space_2 = 0;

        strftime(cur_time, 26, "%Y-%m-%d %H:%M:%S", localtime(&t));

        
        if (bytes_read > 0) {
            ;
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

        if (strcmp(requests,"GET") != 0) {
            serve_405(new_fd,requests); // 405 code if there is any request other than GET 
            close(new_fd);
            continue;
        }

        if(strstr(file_name,"..")) {
            printf("..hello\n");
            serve_404(new_fd,requests,requested_file);
            close(new_fd);
            continue;
        }

        char file_to_open[1024];
        snprintf(file_to_open,sizeof(file_to_open),"pages/%s", file_name);
        html_file = fopen(file_to_open,"rb");
        if(strlen(file_name) == 0) {
            strncpy(file_to_open,"pages/index.html",sizeof(file_to_open) - 1);
            html_file = fopen(file_to_open,"rb");
        }else if (html_file == NULL) {
            serve_404(new_fd,requests,requested_file);
            close(new_fd);
            continue;
        }
        char header[500];

        struct stat st;
        // get file size and other stats of the file we need
        stat(file_to_open, &st);

        strftime(cur_time, 26, "%Y-%m-%d %H:%M:%S", localtime(&t));
        printf("[%s] 127.0.0.1 6969 %s %s 200 OK\n",cur_time,requests,requested_file);

        snprintf(header,
                sizeof(header),
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
        html_file = NULL;
        close(new_fd);
    }
    freeaddrinfo(res);
    close(sockfd);
}
