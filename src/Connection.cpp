#ifndef CONNECTION
#define CONNECTION
#define BUFLEN 1024
#define TCPIP_CONN 0
#define UDPIP_CONN 1
#define IP_CONN 2
#define NAME_NUM -3
#define BYTES_NUM -2
#define CHUNKS_NUM -1

#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <cstring>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cassert>
#include <cmath>

using namespace std;

struct msg_header{
    char type; //b bytes, t string, s size(bytes), l length(string)
    int num;
};

class SenderConnection {
    private:
    
    // methods to be implemented by derived class
    // which perform sending using specific protocol
    virtual size_t send_bytes(char *arr, size_t bytes, int chunk_num) = 0;
    virtual size_t send_string(string data, int num) = 0;

    public:
    
    int send_file(const char *filename, bool isdir) {
        if (!isdir) {
            char buf[BUFLEN];
            int chunks;
            // determine filesize
            struct stat tr_file;
            if (stat(filename, &tr_file) == -1) {
                printf("Error: cannot stat %s.\n", filename);
                return -1;
            }
            long long filesize;
            filesize = tr_file.st_size;
            chunks = ceil((float)filesize / (float)BUFLEN);
            int fd = open(filename, O_RDONLY);
            if (fd < 0) {
                printf("Error: cannot open %s.\n", filename);
                return -1;
            }
            printf("Sending %s (%lld bytes).\n", filename, filesize);
            // send preamble
            send_string("FILNAME;" + string(filename),NAME_NUM);
            // sending number of bytes isn't strictly neccessary
            // but can be used in the future to check if all bytes were for sure received
            send_string("BYTES;" + to_string(filesize),BYTES_NUM);
            send_string("CHUNKS;" + to_string(chunks), CHUNKS_NUM);
            // read from file in chunks and send them
            for (int i = 0; i < chunks; i++) {
                int read_succ = read(fd, buf, BUFLEN);
                printf("Sending chunk %d/%d (%d bytes).     \r", i + 1, chunks, read_succ);
                send_bytes(buf, read_succ, i);
            }
            printf("\n");
            close(fd);
        } else {
            // in case of directory only its name is sent
            printf("Sending directory %s\n", filename);
            send_string("DIRNAME;" + string(filename), NAME_NUM);
        }
        return 0;
    }
    
    void send_eot() {
        // after sending all files and directories
        // 'end of transmission' should be sent by client
        // for server to close the connection
        send_string("EOT", NAME_NUM);
        // FIXME add statistics about connection/transfer speed :)
    }
};

class ReceiverConnection {
    private:

    // methods to be implemented by derived class
    // which perform receiving using specific protocol
    virtual string receive_string(int num) = 0;
    virtual size_t receive_bytes(char *arr, int chunk_num) = 0;
    char buf[BUFLEN];


    public:
    
    virtual void receiver_loop(string root_directory) = 0;

    // returns 0 in case of successful retrieval,
    // 1 in case transmission ended and client will disconnect
    int receive_file(string root_directory) {
        string msg;
        msg = receive_string(NAME_NUM);
        if (msg.substr(0,3) == "EOT") {
            // end of transmission
            return 1;
        }
        string file_type = msg.substr(0, min(7, (int)msg.size()));
        if (file_type != "FILNAME" && file_type != "DIRNAME") {
            printf("Protocol error: expected FILNAME or DIRNAME but got %s\n", file_type.c_str());
        } else {
            string filepath(root_directory + "/" + msg.substr(8, string::npos));
            if (file_type == "FILNAME") {
                int fd = open(filepath.c_str(), O_WRONLY | O_CREAT, 0755);
                if (fd < 0) {
                    printf("Error: cannot open %s.\n", filepath.c_str());
                    return 1;
                }
                msg = receive_string(BYTES_NUM);
                string bytes_string = msg.substr(0, min(5, (int)msg.size()));
                if (bytes_string != "BYTES") {
                    printf("Protocol error: expected BYTES but got %s\n", bytes_string.c_str());
                } else {
                    int total_bytes = stoi(msg.substr(6, string::npos));
                    printf("Receiving file %s (%d bytes).\n", filepath.c_str(), total_bytes);
                }
                msg = receive_string(CHUNKS_NUM);
                string chunks_string = msg.substr(0, min(6, (int)msg.size()));
                if (chunks_string != "CHUNKS") {
                    printf("Protocol error: expected CHUNKS but got %s\n", chunks_string.c_str());
                } else {
                    int chunks = stoi(msg.substr(7, string::npos));
                    size_t rec;
                    for (int i = 0; i < chunks; i++) {
                        rec = receive_bytes(buf, i);
                        printf("Receiving chunk %d/%d (%lu bytes).     \r", i + 1, chunks, (long unsigned)rec);
                        write(fd, buf, rec);
                    }
                    printf("\n");
                }
                close(fd);
            } else if (file_type == "DIRNAME") {
                printf("Creating directory %s\n", filepath.c_str());
                mkdir(filepath.c_str(), 0755);
            }
        }
        return 0;
    }
};

#endif
