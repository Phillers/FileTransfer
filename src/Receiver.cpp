#include <cstdio>
#include <cstdlib>
#include <sys/stat.h>
#include <sys/types.h>
#include <string>
#include <fcntl.h>
#include "TCPIPConnection.cpp"
#include "UDPIPConnection.cpp"
#include "IPConnection.cpp"
#include "EthernetConnection.cpp"

using namespace std;

int main(int argc, char **argv) {
    if (argc < 4) {
        printf("Arguments missing.\n");
        exit(EXIT_FAILURE);
    } else {
        string conn_type = string(argv[3]);
        ReceiverConnection *conn = nullptr;
        // create root directory if it doesn't exist
        mkdir(argv[2], 0755);
        if (conn_type == "TCPIP") {
            conn = new ReceiverTCPIPConnection(atoi(argv[1]));
        } else if (conn_type == "UDPIP") {
            conn = new ReceiverUDPIPConnection(atoi(argv[1]));
        } else if (conn_type == "IP") {
            conn = new ReceiverIPConnection(atoi(argv[1]));
        } else if (conn_type == "ETHERNET") {
            if (argc < 5) {
                printf("Arguments missing.\n");
                exit(EXIT_FAILURE);
            }
            conn = new ReceiverEthernetConnection(atoi(argv[1]), argv[4]);
        } else {
            printf("Not implemented yet or unknown connection type.\n");
        }
        if (conn != nullptr) {
            string directory_name = string(argv[2]);
            if (directory_name.back() == '/')
                directory_name.resize(directory_name.size() - 1);
            conn -> receiver_loop(directory_name);
        }
    }
}
