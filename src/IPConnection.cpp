#include "Connection.cpp"
#include<netinet/ip.h>

class SenderIPConnection : public SenderConnection {
private:

    int sfd;
    struct sockaddr_in saddr;
    char buf[BUFLEN+ sizeof(msg_header)];
    msg_header hdr;
    msg_header hdr2;

    size_t send_string(string data, int num) {
        memset(buf, 0, BUFLEN+ sizeof(msg_header));
        size_t size = data.size();
        hdr.num=num;
        hdr.type='l';
        memcpy(buf, &hdr, sizeof(hdr));
        memcpy(buf+sizeof(hdr), &size, sizeof(size));
        int received;
        int ips=60+sizeof(hdr);
        iphdr* ip=(iphdr*)malloc(ips);
        do{
            write(sfd,buf, sizeof(size_t)+sizeof(hdr));
            received=recv(sfd, ip, ips, 0);
            memcpy(&hdr2, (char*)ip+ip->ihl*4, sizeof(hdr2));
            //printf("%c %d %c %d\n", hdr.type, hdr.num, hdr2.type, hdr2.num);
        }while(received<(int)(sizeof(hdr2)+ip->ihl*4)||hdr.num!=hdr2.num||hdr.type!=hdr2.type);
        hdr.type='t';
        memcpy(buf, &hdr, sizeof(hdr));
        strncpy(buf+sizeof(hdr), data.c_str(), data.size());
        size_t total_written;
        do {
            total_written = write(sfd, buf, size + sizeof(hdr));
            received=recv(sfd, ip, ips, 0);
            memcpy(&hdr2, (char*)ip+ip->ihl*4, sizeof(hdr2));
            //printf("%c %d %c %d\n", hdr.type, hdr.num, hdr2.type, hdr2.num);
        }while(received<(int)sizeof(hdr2)||hdr.num!=hdr2.num||hdr.type!=hdr2.type);
        free(ip);
        return total_written;
    }

    size_t send_bytes(char *arr, size_t bytes, int chunk_num) {
        memset(buf, 0, BUFLEN+ sizeof(hdr));
        size_t b = bytes;
        hdr.num=chunk_num;
        hdr.type='s';
        memcpy(buf, &hdr, sizeof(hdr));
        memcpy(buf+sizeof(hdr), &b, sizeof(b));
        int received;
        int ips=60+sizeof(hdr);
        iphdr* ip=(iphdr*)malloc(ips);
        do {
            write(sfd, buf, sizeof(size_t)+sizeof(hdr));
            received=recv(sfd, ip, ips, 0);
            memcpy(&hdr2, (char*)ip+ip->ihl*4, sizeof(hdr2));
            //printf("%c %d %c %d\n", hdr.type, hdr.num, hdr2.type, hdr2.num);
        }while(received<(int)(sizeof(hdr2)+ip->ihl*4)||hdr.num!=hdr2.num||hdr.type!=hdr2.type);
        hdr.type='b';
        memcpy(buf, &hdr, sizeof(hdr));
        memcpy(buf+sizeof(hdr), arr, b);
        size_t total_written;
        do {
            total_written = write(sfd, buf, bytes + sizeof(hdr));
            received=recv(sfd, ip, ips, 0);
            memcpy(&hdr2, (char*)ip+ip->ihl*4, sizeof(hdr2));
            //printf("%c %d %c %d\n", hdr.type, hdr.num, hdr2.type, hdr2.num);
        }while(received<(int)sizeof(hdr2)||hdr.num!=hdr2.num||hdr.type!=hdr2.type);
        free(ip);
        return total_written;
    }

public:

    SenderIPConnection(char *ip_addr, int port) {
        sfd = socket(PF_INET, SOCK_RAW, port);
        timeval timeout;
        timeout.tv_sec=1;
        timeout.tv_usec=0;
        setsockopt (sfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
        memset(&saddr, 0, sizeof(saddr));
        saddr.sin_family = AF_INET;
        saddr.sin_addr.s_addr = inet_addr(ip_addr);
        saddr.sin_port = 0;//htons(port);
        int c = connect(sfd, (struct sockaddr*) &saddr, sizeof(saddr));
        if (c < 0) {
            perror("Connection failed.\n");
        } else {
            printf("Connection succeeded.\n");
        }
    }

    ~SenderIPConnection() {
        close(sfd);
    }
};

class ReceiverIPConnection : public ReceiverConnection {
private:
    static const int BUFSIZE=BUFLEN+sizeof(msg_header)+15*4;
    socklen_t sl;
    int sfd, cfd;
    struct sockaddr_in saddr, caddr;
    char frame[BUFSIZE];
    struct iphdr* ip;
    char string_next=0;
    msg_header hdr;
    char* data;

    string receive_string(int num) {
        string_next=1;
        char buf2[BUFLEN];
        receive_bytes(buf2, num);
        string_next=0;
        return string(buf2);
    }

    size_t receive_bytes(char *arr, int chunk_num){
        size_t to_be_received;
        size_t *ptr = &to_be_received;
        int received;
        memset(&caddr, 0, sizeof(caddr));
        sl = sizeof(caddr);

        memset(frame, 0, BUFSIZE);
        char expected= string_next ? 'l' : 's';

        // Determine how many bytes there will be.
        do {
            received = recvfrom(cfd, frame,
                                BUFSIZE, 0, (struct sockaddr *) &caddr, &sl);
            ip = (struct iphdr *) frame;
            data = frame + ip->ihl * 4;
            memcpy(&hdr, data, sizeof(hdr));
            //printf("%d %c %d %c %d\n", received, hdr.type, hdr.num, expected, chunk_num);
            sendto(sfd, &hdr, sizeof(hdr), 0, (sockaddr *) &caddr, sizeof(caddr));
        }while(received<(int)(sizeof(hdr)+sizeof(to_be_received)+ip->ihl*4)
               ||hdr.type!=expected||hdr.num!=chunk_num);

         memcpy(ptr, data+sizeof(hdr), sizeof(size_t));

        for (int i = 0; i < BUFLEN; i++)
            arr[i] = 0;

        expected= string_next ? 't' : 'b';
        // Receive actual data.
        do{
            received = recvfrom(cfd, frame,
                                BUFSIZE, 0, (struct sockaddr*)&caddr, &sl);
            ip = (struct iphdr*)frame;
            data = frame + ip->ihl * 4;
            memcpy(&hdr, data, sizeof(hdr));
            //printf("%d %c %d %c %d\n", received, hdr.type, hdr.num, expected, chunk_num);
            sendto(sfd, &hdr, sizeof(hdr), 0, (sockaddr *) &caddr, sizeof(caddr));
        }while(received<int(to_be_received+sizeof(hdr)+ip->ihl*4)||hdr.type!=expected||hdr.num!=chunk_num);

        memcpy(arr, data+sizeof(hdr), to_be_received);

        return to_be_received;
    }

public:

    ReceiverIPConnection(int port) {
        memset(&saddr, 0, sizeof(saddr));
        saddr.sin_family = AF_INET;
        saddr.sin_addr.s_addr = INADDR_ANY;
        saddr.sin_port = 0;//htons(port);
        sfd = socket(PF_INET, SOCK_RAW, port);
        int one = 1;
        setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, (char*) &one, sizeof(one));
        bind(sfd, (struct sockaddr*) &saddr, sizeof(saddr));
        //       listen(sfd, 5);
    }

    void receiver_loop(string root_directory) {
        while (1) {
            memset(&caddr, 0, sizeof(caddr));
            sl = sizeof(caddr);
            cfd = sfd;//accept(sfd, (struct sockaddr*) &caddr, &sl);
            while (1) {
                int result = receive_file(root_directory);
                // end of transmission
                if (result == 1)
                    break;
            }
        }
    }

    ~ReceiverIPConnection() {
        close(sfd);
    }
};
