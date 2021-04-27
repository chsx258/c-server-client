#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>


#define BUF_SIZE 128

#define MAX_AUCTIONS 5
#ifndef VERBOSE
#define VERBOSE 0
#endif

#define ADD 0
#define SHOW 1
#define BID 2
#define QUIT 3

/* Auction struct - this is different than the struct in the server program
 */
struct auction_data {
    int sock_fd;
    char item[BUF_SIZE];
    int current_bid;
};

/* Displays the command options available for the user.
 * The user will type these commands on stdin.
 */

void print_menu() {
    printf("The following operations are available:\n");
    printf("    show\n");
    printf("    add <server address> <port number>\n");
    printf("    bid <item index> <bid value>\n");
    printf("    quit\n");
}

/* Prompt the user for the next command 
 */
void print_prompt() {
    printf("Enter new command: ");
    fflush(stdout);
}


/* Unpack buf which contains the input entered by the user.
 * Return the command that is found as the first word in the line, or -1
 * for an invalid command.
 * If the command has arguments (add and bid), then copy these values to
 * arg1 and arg2.
 */
int parse_command(char *buf, int size, char *arg1, char *arg2) {
    int result = -1;
    char *ptr = NULL;
    if(strncmp(buf, "show", strlen("show")) == 0) {
        return SHOW;
    } else if(strncmp(buf, "quit", strlen("quit")) == 0) {
        return QUIT;
    } else if(strncmp(buf, "add", strlen("add")) == 0) {
        result = ADD;
    } else if(strncmp(buf, "bid", strlen("bid")) == 0) {
        result = BID;
    } 
    ptr = strtok(buf, " "); // first word in buf

    ptr = strtok(NULL, " "); // second word in buf
    if(ptr != NULL) {
        strncpy(arg1, ptr, BUF_SIZE);
    } else {
        return -1;
    }
    ptr = strtok(NULL, " "); // third word in buf
    if(ptr != NULL) {
        strncpy(arg2, ptr, BUF_SIZE);
        return result;
    } else {
        return -1;
    }
    return -1;
}

/* Connect to a server given a hostname and port number.
 * Return the socket for this server
 */
int add_server(char *hostname, int port) {
        // Create the socket FD.
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("client: socket");
        exit(1);
    }
    
    // Set the IP and port of the server to connect to.
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    struct addrinfo *ai;
    
    /* this call declares memory and populates ailist */
    if(getaddrinfo(hostname, NULL, NULL, &ai) != 0) {
        close(sock_fd);
        return -1;
    }
    /* we only make use of the first element in the list */
    server.sin_addr = ((struct sockaddr_in *) ai->ai_addr)->sin_addr;

    // free the memory that was allocated by getaddrinfo for this list
    freeaddrinfo(ai);

    // Connect to the server.
    if (connect(sock_fd, (struct sockaddr *)&server, sizeof(server)) == -1) {
        perror("client: connect");
        close(sock_fd);
        return -1;
    }
    if(VERBOSE){
        fprintf(stderr, "\nDebug: New server connected on socket %d.  Awaiting item\n", sock_fd);
    }
    return sock_fd;
}
/* ========================= Add helper functions below ========================
 * Please add helper functions below to make it easier for the TAs to find the 
 * work that you have done.  Helper functions that you need to complete are also
 * given below.
 */

// helper funtcion check if the sock is closed

int isnotclosed(int sock_fd){
    char buf[32];
    int rec = recv(sock_fd,buf,sizeof(buf),MSG_PEEK);
    if (rec <= 0){return 0;}
    return 1;
}
int findindex(struct auction_data *a, char*item, int size){
    for (int i=0;i<size;i++){
        if (a[i].item == item){
            return i;
        }
    }
    return -1;
}
/* Print to standard output information about the auction
 */
void print_auctions(struct auction_data *a, int size) {
    printf("Current Auctions:\n");

    /* TODO Print the auction data for each currently connected 
     * server.  Use the follosing format string:
     *     "(%d) %s bid = %d\n", index, item, current bid
     * The array may have some elements where the auction has closed and
     * should not be printed.
     */
    for (int i=0; i< size; i++){
        if ((a[i].current_bid != -2)&&(a[i].sock_fd != -1)){
            printf("(%d) %s bid = %d\n",i,a[i].item,a[i].current_bid);}

    }
}

/* Process the input that was sent from the auction server at a[index].
 * If it is the first message from the server, then copy the item name
 * to the item field.  (Note that an item cannot have a space character in it.)
 */
void update_auction(char *buf, int size, struct auction_data *a, int index) {
    
    // TODO: Complete this function
    
    // fprintf(stderr, "ERROR malformed bid: %s", buf);
    // printf("\nNew bid for %s [%d] is %d (%d seconds left)\n",           );
    char *blank = " ";
    char *item = strtok(buf, blank);
    char *h_bid = strtok(NULL, blank);
    char *sec = strtok(NULL, blank);
    // index = findindex(a, item, size);
    a[index].current_bid = atoi(h_bid);
    strcpy(a[index].item,item);
    a[index].current_bid = atoi(h_bid);
    printf("\nNew bid for %s [%d] is %d (%d seconds left)\n",item,index,atoi(h_bid),atoi(sec));

}

int main(void) {

    char name[BUF_SIZE];

    // Declare and initialize necessary variables
    // TODO
    char buf[BUF_SIZE];
    char arg1[BUF_SIZE];
    char arg2[BUF_SIZE];
    int size = 0;
    struct auction_data auctions[MAX_AUCTIONS];
    int op_code = 0;
    int server_id = 0;
    int newport = 0;
    int index = 0;
    int maxbid = 0;
    for (int i=0;i<MAX_AUCTIONS;i++){
        auctions[i].sock_fd = -1;
        auctions[i].current_bid = -1;
    }

    // Get the user to provide a name.
    printf("Please enter a username: ");
    fflush(stdout);
    int num_read = read(STDIN_FILENO, name, BUF_SIZE);
    if(num_read <= 0){
        fprintf(stderr, "ERROR: read from stdin failed\n");
        exit(1);
    }
    print_menu();
    
    // TODO
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("client: socket");
        exit(1);
    }
    int max_fd = sock_fd;
    fd_set all_fds;
    FD_ZERO(&all_fds);
    FD_SET(STDIN_FILENO,&all_fds);
    int c_fd = 0;
    while(1) {
        print_prompt();
        // TODO
        fd_set listen_fds = all_fds;
        c_fd = select(max_fd + 1, &listen_fds, NULL, NULL, NULL);
        if (c_fd == -1) {
            perror("clent: select");
            exit(1);
        }
        if (FD_ISSET(STDIN_FILENO,&listen_fds)){
            num_read = read(c_fd, buf, BUF_SIZE);
            if (num_read == 0) {
                break;
                }
            op_code = parse_command(buf,size,arg1,arg2);
            // printf("op is %d\n",op_code);
            if (op_code == SHOW){
                print_auctions(auctions,size);
            } 
            if (op_code == ADD){
                server_id = atoi(arg2);
                newport = add_server(arg1,server_id);
                if (newport > max_fd){max_fd = newport;}
                FD_SET(newport,&all_fds);
                int len=strlen(name);
                auctions[size].sock_fd = newport;
                write(newport,name,len-1);
                size ++;
            }
            if (op_code == BID){
                index = atoi(arg1);
                maxbid = atoi(arg2);
                int client_fd = auctions[index].sock_fd;
                char s[BUF_SIZE];
                sprintf(s,"%d",maxbid);
                write(client_fd,s,sizeof(s));
            }
            if (op_code == QUIT){
                FD_ZERO(&all_fds);
                exit(1);
            }
            if (op_code == -1){
                print_menu();
            }
        }
        for (index=0;index<MAX_AUCTIONS;index++){
            int client_fd = auctions[index].sock_fd;
            if (client_fd>-1 &&FD_ISSET(client_fd, &listen_fds)){
                num_read = read(auctions[index].sock_fd, buf, BUF_SIZE);
                if (num_read == 0) {
                        
                    }
                else{
                // printf("get from server\n");
                char *key = "Auction closed: ";
                if (strstr(buf,key)){printf("%s",buf);auctions[index].current_bid=-2;FD_CLR(auctions[index].sock_fd,&all_fds);}
                else{
                update_auction(buf,size,auctions,index);}}
            }
        }   
    }
    return 0; // Shoud never get here
}
