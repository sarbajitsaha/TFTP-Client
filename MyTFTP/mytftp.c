//
// Created by sarbajit on 27/3/17.
//
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>

#define MAX_SIZE 1024
#define PORT 69

/* Opcodes */
#define RRQ    1
#define WRQ    2
#define DATA   3
#define ACK	   4
#define ERROR  5
/*****/

/*Error codes*/
const char TFTP_error_messages[][MAX_SIZE] = {
        "Undefined error",                 // Error code 0
        "File not found",                  // 1
        "Access violation",                // 2
        "Disk full or allocation error",   // 3
        "Illegal TFTP operation",          // 4
        "Unknown transfer ID",             // 5
        "File already exists",             // 6
        "No such user"                     // 7
};
/****/

int parse_string(char *,char [][MAX_SIZE]);
void get_file(char *,char *);
void put_file(char *,char *);

int main(int argc, char *argv[])
{
    /***arguments**/
    if(argc!=2)
    {
        perror("Host address must be provided as argument");
    }

    char *host_address = argv[1];
    /*******/

    while(true)
    {
        printf("tftp> ");

        char tokens[MAX_SIZE][MAX_SIZE];

        size_t size = MAX_SIZE;
        char *msg = (char *)malloc(sizeof(char)*MAX_SIZE);

        getline(&msg,&size,stdin);
        if(strcmp(msg,"quit\n")==0)
        {
            break;
        }

        int len = parse_string(msg,tokens);
        if(len==1 || len>=3)
        {
            printf("Invalid command\n");
        }
        else
        {
            if(strcmp(tokens[0],"put")==0)
            {
                char *filename = tokens[1];
                put_file(filename,host_address);
            }
            else if(strcmp(tokens[0],"get")==0)
            {
                char *filename = tokens[1];
                get_file(filename,host_address);
            }
            else
            {
                printf("Invalid command\n");
            }
        }
    }

    return 0;
}

/*****break char* to array of char* with delimiter space***/
int parse_string(char *str,char tokens[][MAX_SIZE])
{
    int i = 0;
    const char s[] = " \n";
    char *token;

    token = strtok(str, s);
    strcpy(tokens[i],token);

    while(token!=NULL)
    {
        i++;
        token = strtok(NULL,s);
        if(token!=NULL)
            strcpy(tokens[i],token);
    }
    return i;
}

/***get file with name filename***/
void get_file(char *filename,char *host)
{
    FILE *fin;
    int total_bytes = 0;
    struct timeval t1, t2;
    double elapsedTime;

    // start timer
    gettimeofday(&t1, NULL);


    fin = fopen(filename,"wb");
    char send_buff[MAX_SIZE],recv_buff[MAX_SIZE];
    struct sockaddr_in serv_addr,serv_addr1;
    int sockfd;

    bzero(send_buff,MAX_SIZE);
    bzero(recv_buff,MAX_SIZE);
    bzero(&serv_addr,sizeof(serv_addr));
    bzero(&serv_addr1,sizeof(serv_addr1));

    if((sockfd=socket(AF_INET,SOCK_DGRAM,0)) < 0)
    {
        perror("Could not create socket\n");
        exit(1);
    }
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    if(inet_pton(AF_INET,host,&serv_addr.sin_addr)<=0)
    {
        perror("inet_pton error occured\n");
        exit(1);
    }
    if(connect(sockfd,(struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("Connect Failed \n");
        exit(1);
    }

    //connection established now send RRQ to port 69

    /*get sending port value*/
    struct sockaddr_in sin;
    socklen_t len = sizeof(sin);
    int sending_port;
    if (getsockname(sockfd, (struct sockaddr *)&sin, &len) == -1)
    {
        perror("Unable to get port number\n");
        exit(1);
    }
    else
        sending_port = ntohs(sin.sin_port);
    /******/

    /*properly format the RRQ*/
    int i,j;
    send_buff[0] = 0;
    send_buff[1] = RRQ;
    for(i=0;i<strlen(filename);i++)
        send_buff[i+2] = filename[i];
    i+=2;
    send_buff[i] = 0;
    i++;
    char mode[] = "octet";
    for(j=0;j<strlen(mode);j++,i++)
        send_buff[i] = mode[j];
    send_buff[i] = 0;
    int packet_len = i+1;
    /****/

    send(sockfd,send_buff,packet_len,0);
    shutdown(sockfd,2);
    close(sockfd);
    sockfd=socket(AF_INET,SOCK_DGRAM,0);

    serv_addr1.sin_family = AF_INET;
    serv_addr1.sin_port = sin.sin_port;

    if(inet_pton(AF_INET,host,&serv_addr1.sin_addr)<=0)
    {
        perror("inet_pton error occured\n");
        exit(1);
    }

    if(bind(sockfd, (struct sockaddr *)&serv_addr1, sizeof(serv_addr1))<0)
    {
        perror("Error in binding");
        exit(1);
    }

    struct sockaddr_in sender;
    socklen_t sendsize = sizeof(sender);
    bzero(&sender, sizeof(sender));
    packet_len = recvfrom(sockfd, recv_buff, MAX_SIZE, 0, (struct sockaddr *) &sender, &sendsize);
    memset(&serv_addr, 0, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = sender.sin_port;

    if(inet_pton(AF_INET,host,&serv_addr.sin_addr)<=0)
    {
        perror("inet_pton error occured\n");
        exit(1);
    }

    if(connect(sockfd,(struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("Connect Failed \n");
        exit(1);
    }

    int current_block = 0;

    while(packet_len==516 || recv_buff[1]==ERROR)
    {
        total_bytes += packet_len;
        int opcode = recv_buff[1];

        if(opcode==ERROR)
        {
            printf("%s\n",TFTP_error_messages[recv_buff[3]]);
            return;
        }

        int block_no = ((recv_buff[2]) * 10) + (recv_buff[3]);
        if(block_no-current_block==1)
        {
            for(int i=4;i<packet_len;i++)
            {
                fputc(recv_buff[i],fin);
            }
        }
        current_block++;

        send_buff[0] = 0;
        send_buff[1] = ACK;
        send_buff[2] = recv_buff[2];
        send_buff[3] = recv_buff[3];
        sendto(sockfd,send_buff,4,0,(struct sockaddr*)&serv_addr, sizeof(serv_addr));
        packet_len = recvfrom(sockfd, recv_buff, MAX_SIZE, 0, (struct sockaddr *) &sender, &sendsize);
    }

    for(int i=4;i<packet_len;i++)
    {
        fputc(recv_buff[i],fin);
    }
    total_bytes += packet_len;
    int opcode = recv_buff[1];
    int block_no = ((recv_buff[2]) * 10) + (recv_buff[3]);
    send_buff[0] = 0;
    send_buff[1] = ACK;
    send_buff[2] = recv_buff[2];
    send_buff[3] = recv_buff[3];
    sendto(sockfd,send_buff,4,0,(struct sockaddr*)&serv_addr, sizeof(serv_addr));

    fclose(fin);

    // stop timer
    gettimeofday(&t2, NULL);

    elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;      // sec to ms
    elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;   // us to ms

    printf("Received %d bytes in %f ms\n",total_bytes,elapsedTime);
}

/***put file with name filename***/
void put_file(char *filename, char *host)
{
    FILE *fin;
    int total_bytes = 0;
    struct timeval t1, t2;
    double elapsedTime;

    // start timer
    gettimeofday(&t1, NULL);


    fin = fopen(filename,"rb");
    char send_buff[MAX_SIZE],recv_buff[MAX_SIZE];
    struct sockaddr_in serv_addr,serv_addr1;
    int sockfd;

    bzero(send_buff,MAX_SIZE);
    bzero(recv_buff,MAX_SIZE);
    bzero(&serv_addr,sizeof(serv_addr));
    bzero(&serv_addr1,sizeof(serv_addr1));

    if((sockfd=socket(AF_INET,SOCK_DGRAM,0)) < 0)
    {
        perror("Could not create socket\n");
        exit(1);
    }
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    if(inet_pton(AF_INET,host,&serv_addr.sin_addr)<=0)
    {
        perror("inet_pton error occured\n");
        exit(1);
    }
    if(connect(sockfd,(struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("Connect Failed \n");
        exit(1);
    }

    //connection established now send RRQ to port 69

    /*get sending port value*/
    struct sockaddr_in sin;
    socklen_t len = sizeof(sin);
    int sending_port;
    if (getsockname(sockfd, (struct sockaddr *)&sin, &len) == -1)
    {
        perror("Unable to get port number\n");
        exit(1);
    }
    else
        sending_port = ntohs(sin.sin_port);
    /******/

    /*properly format the WR*/
    int i,j;
    send_buff[0] = 0;
    send_buff[1] = WRQ;
    for(i=0;i<strlen(filename);i++)
        send_buff[i+2] = filename[i];
    i+=2;
    send_buff[i] = 0;
    i++;
    char mode[] = "octet";
    for(j=0;j<strlen(mode);j++,i++)
        send_buff[i] = mode[j];
    send_buff[i] = 0;
    int packet_len = i+1;
    /****/

    send(sockfd,send_buff,packet_len,0);
    shutdown(sockfd,2);
    close(sockfd);
    sockfd=socket(AF_INET,SOCK_DGRAM,0);

    serv_addr1.sin_family = AF_INET;
    serv_addr1.sin_port = sin.sin_port;

    if(inet_pton(AF_INET,host,&serv_addr1.sin_addr)<=0)
    {
        perror("inet_pton error occured\n");
        exit(1);
    }

    if(bind(sockfd, (struct sockaddr *)&serv_addr1, sizeof(serv_addr1))<0)
    {
        perror("Error in binding");
        exit(1);
    }

    struct sockaddr_in sender;
    socklen_t sendsize = sizeof(sender);
    bzero(&sender, sizeof(sender));
    packet_len = recvfrom(sockfd, recv_buff, MAX_SIZE, 0, (struct sockaddr *) &sender, &sendsize);
    memset(&serv_addr, 0, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = sender.sin_port;

    if(inet_pton(AF_INET,host,&serv_addr.sin_addr)<=0)
    {
        perror("inet_pton error occured\n");
        exit(1);
    }

    if(connect(sockfd,(struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("Connect Failed \n");
        exit(1);
    }

    int current_block = 0;

    bool exit = false;

    while(!exit)
    {
        int opcode = recv_buff[1];

        if(opcode==ERROR)
        {
            printf("%s\n",TFTP_error_messages[recv_buff[3]]);
            return;
        }

        int block_no = ((recv_buff[2]) * 10) + (recv_buff[3]);
        printf("Block%d\n",block_no);
        if(block_no-current_block==0)
        {
            current_block++;
            int temp = 0;
            for(temp=0;temp<512;temp++)
            {
                char c = fgetc(fin);
                if(c!=EOF)
                    send_buff[temp+4]=c;
                else
                {
                    printf("asd\n");
                    exit = true;
                    break;
                }
            }
            packet_len = temp+4;
        }

        send_buff[0] = 0;
        send_buff[1] = DATA;
        send_buff[2] = current_block/10;
        send_buff[3] = current_block%10;
        total_bytes += packet_len;
        printf("%d\n",total_bytes);
        sendto(sockfd,send_buff,packet_len,0,(struct sockaddr*)&serv_addr, sizeof(serv_addr));
        packet_len = recvfrom(sockfd, recv_buff, MAX_SIZE, 0, (struct sockaddr *) &sender, &sendsize);
    }

    fclose(fin);

    // stop timer
    gettimeofday(&t2, NULL);

    elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;      // sec to ms
    elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;   // us to ms

    printf("Sent %d bytes in %f ms\n",total_bytes,elapsedTime);
}