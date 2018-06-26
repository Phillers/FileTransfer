#include <cstdio>
#include <cstdlib>
#include <sys/stat.h>
#include <sys/types.h>
#include <string>
#include <fcntl.h>
#include <dirent.h>
#include "TCPIPConnection.cpp"
#include "UDPIPConnection.cpp"
#include "IPConnection.cpp"
#include "EthernetConnection.cpp"

using namespace std;

void process_file(string base_name, string name, SenderConnection *conn) {
    struct stat sb;
    string full_name = base_name + name;
    if (stat(full_name.c_str(), &sb) == 0) {
        if (S_ISREG(sb.st_mode)) {
            // Regular file.
            conn -> send_file(full_name.c_str(), false);
        } else if (S_ISDIR(sb.st_mode)) {
            // Directory.
            string bname = full_name + "/";
            conn -> send_file(bname.c_str(), true);
            DIR *dir;
            struct dirent *ent;
            if ((dir = opendir(bname.c_str())) != NULL) {
                while ((ent = readdir(dir)) != NULL) {
                    string file_name = string(ent -> d_name);
                    if (file_name != "." && file_name != "..")
                        // Recursive call for each file in this directory.
                        process_file(bname, file_name, conn);
                }
                closedir(dir);
            } else
                printf("Error: cannot open directory %s.\n", bname.c_str());
        } else {
            printf("Unsupported type of file %s.\n", full_name.c_str());
        }
    } else
        printf("Error: cannot stat file %s.\n", full_name.c_str());
}

int main(int argc, char **argv) {
    if (argc < 5) {
        printf("Arguments missing.\n");
        exit(EXIT_FAILURE);
    } else {
        string conn_type = string(argv[4]);
        SenderConnection *conn = nullptr;
        if (conn_type == "TCPIP") {
            conn = new SenderTCPIPConnection(argv[2], atoi(argv[3]));
        } else if (conn_type == "UDPIP") {
            conn = new SenderUDPIPConnection(argv[2], atoi(argv[3]));
        } else if (conn_type == "IP") {
            conn = new SenderIPConnection(argv[2], atoi(argv[3]));
        } else if (conn_type == "ETHERNET") {
            if (argc < 6) {
                printf("Arguments missing.\n");
                exit(EXIT_FAILURE);
            }
            conn = new SenderEthernetConnection(argv[2], atoi(argv[3]), argv[5]);
        } else {
            printf("Not implemented yet or unknown connection type.\n");
        }
        if (conn != nullptr) {
            string directory_name = string(argv[1]);
            if (directory_name.back() == '/')
                directory_name.resize(directory_name.size() - 1);
            process_file("", directory_name, conn);
            conn -> send_eot();
        }
    }
}
