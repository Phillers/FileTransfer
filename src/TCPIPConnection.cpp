#include "Connection.cpp"

class SenderTCPIPConnection : public SenderConnection {
    private:
    
    int sfd;
    struct sockaddr_in saddr;
    char buf[BUFLEN];
    
    size_t send_string(string data, int num) {
        assert(num >= -5);
        size_t size = data.size();
        write(sfd, &size, sizeof(size_t));
        strncpy(buf, data.c_str(), data.size());
        size_t total_written = write(sfd, buf, data.size());
        return total_written;
    }
    
    size_t send_bytes(char *arr, size_t bytes, int chunk_num) {
        assert(chunk_num >= -5); // -Wunused-parameter
        size_t b = bytes;
        write(sfd, &b, sizeof(size_t));
        size_t total_written = write(sfd, arr, b);
        return total_written;
    }

    public:
    
    SenderTCPIPConnection(char *ip_addr, int port) {
        sfd = socket(PF_INET, SOCK_STREAM, 0);
        memset(&saddr, 0, sizeof(saddr));
        saddr.sin_family = AF_INET;
        saddr.sin_addr.s_addr = inet_addr(ip_addr);
        saddr.sin_port = htons(port);
        int c = connect(sfd, (struct sockaddr*) &saddr, sizeof(saddr));
        if (c < 0) {
            printf("Connection failed.\n");
        } else {
            printf("Connection succeeded.\n");
        }
    }
    
    ~SenderTCPIPConnection() {
         close(sfd);
    }
};

class ReceiverTCPIPConnection : public ReceiverConnection {
    private:
    
    socklen_t sl;
    int sfd, cfd;
    struct sockaddr_in saddr, caddr;
    char buf[BUFLEN];
    
    string receive_string(int num) {
        receive_bytes(buf, num);
        return string(buf);
    }
    
    size_t receive_bytes(char *arr, int chunk_num){
        assert(chunk_num >= -5);
        size_t to_be_received;
        size_t *ptr = &to_be_received;
        int received;
        size_t total_received = 0;
        
        // Determine how many bytes there will be.
        while (total_received < sizeof(size_t)) {
            received = recv(cfd, ptr + total_received * sizeof(char),
                            sizeof(size_t) - total_received, 0);
            if (received <= 0)
                return -1;
            total_received += received;
        }
        
        for (int i = 0; i < BUFLEN; i++)
            arr[i] = 0;
        total_received = 0;
        // Receive actual data.
        while (total_received < to_be_received) {
            received = recv(cfd, arr + total_received * sizeof(char),
                            to_be_received - total_received, 0);
            if (received <= 0)
                return -2;
            total_received += received;
        }
        return total_received;
    }
    
    public:
    
    ReceiverTCPIPConnection(int port) {
        memset(&saddr, 0, sizeof(saddr));
        saddr.sin_family = AF_INET;
        saddr.sin_addr.s_addr = INADDR_ANY;
        saddr.sin_port = htons(port);
        sfd = socket(PF_INET, SOCK_STREAM, 0);
        int one = 1;
        setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, (char*) &one, sizeof(one));
        bind(sfd, (struct sockaddr*) &saddr, sizeof(saddr));
        listen(sfd, 5);
    }
    
    void receiver_loop(string root_directory) {
        while (1) {
            memset(&caddr, 0, sizeof(caddr));
            sl = sizeof(caddr);
            cfd = accept(sfd, (struct sockaddr*) &caddr, &sl);
            while (1) {
                int result = receive_file(root_directory);
                // end of transmission
                if (result == 1)
                    break;
            }
            close(cfd);
        }
    }
    
    ~ReceiverTCPIPConnection() {
        close(sfd);
    }
};
