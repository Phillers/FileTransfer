#include "Connection.cpp"

class SenderUDPIPConnection : public SenderConnection {
private:
    static const int BUFSIZE=BUFLEN+ sizeof(msg_header);
    int sfd;
    struct sockaddr_in saddr, caddr;
    char frame[BUFSIZE];
    msg_header hdr;
    msg_header hdr2;

    size_t send_string(string data, int num) {
        memset(frame, 0,BUFSIZE);
        size_t size = data.size();
        hdr.num=num;
        hdr.type='l';
        memcpy(frame, &hdr, sizeof(hdr));
        memcpy(frame+sizeof(hdr), &size, sizeof(size));
        int received;
        do{
            write(sfd, frame, sizeof(size_t)+sizeof(hdr));
            received=recv(sfd, &hdr2, sizeof(hdr2), 0);
        }while(received<(int)sizeof(hdr2)||hdr.num!=hdr2.num||hdr.type!=hdr2.type);
        hdr.type='t';
        memcpy(frame, &hdr, sizeof(hdr));
        strncpy(frame+sizeof(hdr), data.c_str(), data.size());
        size_t total_written;
        do {
            total_written = write(sfd, frame, size + sizeof(hdr));
            received=recv(sfd, &hdr2, sizeof(hdr2), 0);
        }while(received<(int)sizeof(hdr2)||hdr.num!=hdr2.num||hdr.type!=hdr2.type);
        return total_written;
    }

    size_t send_bytes(char *arr, size_t bytes, int chunk_num) {
        memset(frame, 0,BUFSIZE);
        size_t b = bytes;
        hdr.num=chunk_num;
        hdr.type='s';
        memcpy(frame, &hdr, sizeof(hdr));
        memcpy(frame+sizeof(hdr), &b, sizeof(b));
        int received;
        do {
            write(sfd, frame, sizeof(size_t)+sizeof(hdr));
            received=recv(sfd, &hdr2, sizeof(hdr2), 0);
        }while(received<(int)sizeof(hdr2)||hdr.num!=hdr2.num||hdr.type!=hdr2.type);
        hdr.type='b';
        memcpy(frame, &hdr, sizeof(hdr));
        memcpy(frame+sizeof(hdr), arr, b);
        size_t total_written;
        do {
            total_written = write(sfd, frame, bytes + sizeof(hdr));
            received=recv(sfd, &hdr2, sizeof(hdr2), 0);
        }while(received<(int)sizeof(hdr2)||hdr.num!=hdr2.num||hdr.type!=hdr2.type);
        return total_written;
    }

public:

    SenderUDPIPConnection(char *ip_addr, int port) {
        sfd = socket(PF_INET, SOCK_DGRAM, 0);
        timeval timeout;
        timeout.tv_sec=1;
        timeout.tv_usec=0;
        setsockopt (sfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
        memset(&saddr, 0, sizeof(saddr));
        saddr.sin_family = AF_INET;
        saddr.sin_addr.s_addr = inet_addr(ip_addr);
        saddr.sin_port = htons(port);
        memset(&caddr, 0, sizeof(saddr));
        caddr.sin_family = AF_INET;
        caddr.sin_addr.s_addr = INADDR_ANY;
        caddr.sin_port = 0;
        bind(sfd, (struct sockaddr*) &caddr, sizeof(caddr));
        int c = connect(sfd, (struct sockaddr*) &saddr, sizeof(saddr));
        if (c < 0) {
            printf("Connection failed.\n");
        } else {
            printf("Connection succeeded.\n");
        }
    }

    ~SenderUDPIPConnection() {
        close(sfd);
    }
};

class ReceiverUDPIPConnection : public ReceiverConnection {
private:
    static const int BUFSIZE=BUFLEN+ sizeof(msg_header);
    socklen_t sl;
    int sfd, cfd;
    struct sockaddr_in saddr, caddr;
    char frame[BUFSIZE];
    char string_next=0;
    msg_header hdr;

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
        // Determine how many bytes there will be.
        char expected= string_next ? 'l' : 's';
        do {
            received = recvfrom(cfd, frame,
                            BUFSIZE, 0, (struct sockaddr*)&caddr, &sl);
            memcpy(&hdr, frame, sizeof(hdr));
            sendto(sfd, &hdr, sizeof(hdr), 0, (sockaddr*)&caddr, sizeof(caddr));
        }while(received<(int)(sizeof(hdr)+sizeof(to_be_received))||hdr.type!=expected||hdr.num!=chunk_num);
        memcpy(ptr, (char*)frame+sizeof(hdr), sizeof(size_t));

        for (int i = 0; i < BUFLEN; i++)
            arr[i] = 0;

        // Receive actual data.
        memset(frame, 0, BUFSIZE);
        expected= string_next ? 't' : 'b';
        do{
            received = recvfrom(cfd, frame,
                            BUFSIZE, 0, (struct sockaddr*)&caddr, &sl);
            memcpy(&hdr, frame, sizeof(hdr));
            sendto(sfd, &hdr, sizeof(hdr), 0, (sockaddr*)&caddr, sizeof(caddr));
        }while(received<(int)(to_be_received+sizeof(hdr))||hdr.type!=expected||hdr.num!=chunk_num);
        memcpy(arr, frame+sizeof(hdr), to_be_received);
        return to_be_received;
    }

public:

    ReceiverUDPIPConnection(int port) {
        memset(&saddr, 0, sizeof(saddr));
        saddr.sin_family = AF_INET;
        saddr.sin_addr.s_addr = INADDR_ANY;
        saddr.sin_port = htons(port);
        sfd = socket(PF_INET, SOCK_DGRAM, 0);
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

    ~ReceiverUDPIPConnection() {
        close(sfd);
    }
};
