#include "Connection.cpp"
#include <linux/if_arp.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <sys/ioctl.h>
#include <sys/time.h>

class SenderEthernetConnection : public SenderConnection {
private:

    int sfd;
    struct sockaddr_ll sall;
    char buf[ETH_DATA_LEN];
    msg_header hdr;
    msg_header hdr2;

    size_t send_string(string data, int num) {
        memset(buf, 0, ETH_DATA_LEN);
        size_t size = data.size();
        hdr.num=num;
        hdr.type='l';
        memcpy(buf, &hdr, sizeof(hdr));
        memcpy(buf+sizeof(hdr), &size, sizeof(size));
        int received;
        do{
            sendto(sfd, buf, sizeof(size_t)+sizeof(hdr), 0,
                   (struct sockaddr*) &sall, sizeof(struct sockaddr_ll));
            received=recv(sfd, &hdr2, sizeof(hdr2), 0);
            //printf("%c %d %c %d\n", hdr.type, hdr.num, hdr2.type, hdr2.num);
        }while(received<(int)sizeof(hdr2)||hdr.num!=hdr2.num||hdr.type!=hdr2.type);

        hdr.type='t';
        memcpy(buf, &hdr, sizeof(hdr));
        strncpy(buf+sizeof(hdr), data.c_str(), data.size());
        size_t total_written;
        do {
            total_written = sendto(sfd, buf, size + sizeof(hdr), 0,
                                          (struct sockaddr *) &sall, sizeof(struct sockaddr_ll));
            received=recv(sfd, &hdr2, sizeof(hdr2), 0);
            //printf("%c %d %c %d\n", hdr.type, hdr.num, hdr2.type, hdr2.num);
        }while(received<(int)sizeof(hdr2)||hdr.num!=hdr2.num||hdr.type!=hdr2.type);
        return total_written;
    }

    size_t send_bytes(char *arr, size_t bytes, int chunk_num) {
        memset(buf, 0, ETH_DATA_LEN);
        size_t b = bytes;
        hdr.num=chunk_num;
        hdr.type='s';
        memcpy(buf, &hdr, sizeof(hdr));
        memcpy(buf+sizeof(hdr), &b, sizeof(b));
        int received;
        do {
            sendto(sfd, buf, sizeof(size_t)+sizeof(hdr), 0,
                   (struct sockaddr *) &sall, sizeof(struct sockaddr_ll));
            received=recv(sfd, &hdr2, sizeof(hdr2), 0);
            //printf("%c %d %c %d\n", hdr.type, hdr.num, hdr2.type, hdr2.num);
        }while(received<(int)sizeof(hdr2)||hdr.num!=hdr2.num||hdr.type!=hdr2.type);
        hdr.type='b';
        memcpy(buf, &hdr, sizeof(hdr));
        memcpy(buf+sizeof(hdr), arr, b);
        size_t total_written;
        do {
             total_written = sendto(sfd, buf, bytes + sizeof(hdr), 0,
                                          (struct sockaddr *) &sall, sizeof(struct sockaddr_ll));
            received=recv(sfd, &hdr2, sizeof(hdr2), 0);
            //printf("%c %d %c %d\n", hdr.type, hdr.num, hdr2.type, hdr2.num);
        }while(received<(int)sizeof(hdr2)||hdr.num!=hdr2.num||hdr.type!=hdr2.type);
        return total_written;
    }

public:

    SenderEthernetConnection(char *hw_addr, int eth_type, char* ifname) {
        int ifindex;
        struct ifreq ifr;
        sfd = socket(PF_PACKET, SOCK_DGRAM, htons(eth_type));
        timeval timeout;
        timeout.tv_sec=1;
        timeout.tv_usec=0;
        setsockopt (sfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
        strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
        ioctl(sfd, SIOCGIFINDEX, &ifr);
        ifindex = ifr.ifr_ifindex;
        ioctl(sfd, SIOCGIFHWADDR, &ifr);
        memset(&sall, 0, sizeof(struct sockaddr_ll));
        sall.sll_family = AF_PACKET;
        sall.sll_protocol = htons(eth_type);
        sall.sll_ifindex = ifindex;
        sall.sll_hatype = ARPHRD_ETHER;
        sall.sll_pkttype = PACKET_OUTGOING;
        sall.sll_halen = ETH_ALEN;
        sscanf(hw_addr, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
               &sall.sll_addr[0], &sall.sll_addr[1], &sall.sll_addr[2],
               &sall.sll_addr[3], &sall.sll_addr[4], &sall.sll_addr[5]);
        bind(sfd, (struct sockaddr*) &sall, sizeof(struct sockaddr_ll));
    }

    ~SenderEthernetConnection() {
        close(sfd);
    }
};

class ReceiverEthernetConnection : public ReceiverConnection {
private:

    socklen_t sl;
    int sfd, cfd;
    struct sockaddr_ll sall, call;
    char buf[BUFLEN];
    char string_next=0;
    msg_header hdr;

    string receive_string(int chunk_num) {
        string_next=1;
        char buf2[BUFLEN];
        receive_bytes(buf2, chunk_num);
        string_next=0;
        return string(buf2);
    }

    size_t receive_bytes(char *arr, int chunk_num){
        size_t to_be_received;
        size_t *ptr = &to_be_received;
        int received;
        // Determine
        // how many bytes there will be.
        char* frame;
        frame = (char*)malloc(ETH_DATA_LEN);
        memset(frame, 0, ETH_DATA_LEN);
        char expected= string_next ? 'l' : 's';

        do {
            received = recvfrom(sfd, frame, ETH_DATA_LEN, 0, (struct sockaddr*)(&call),&sl);
            memcpy(&hdr, frame, sizeof(hdr));
            //printf("%d %c %d %c %d\n", received, hdr.type, hdr.num, expected, chunk_num);
            sendto(sfd, &hdr, sizeof(hdr), 0, (sockaddr*)&call, sizeof(call));
        }while(received<(int)(sizeof(hdr)+sizeof(to_be_received))||hdr.type!=expected||hdr.num!=chunk_num);

        memcpy(ptr, (char*)frame+sizeof(hdr), sizeof(size_t));

        for (int i = 0; i < BUFLEN; i++)
            arr[i] = 0;
        // Receive actual data.
        memset(frame, 0, ETH_DATA_LEN);
        expected= string_next ? 't' : 'b';
        do{
            received = recvfrom(sfd, frame, ETH_DATA_LEN, 0, (sockaddr*)&call, &sl);
            memcpy(&hdr, frame, sizeof(hdr));
            //printf("%d %c %d %c %d\n",received, hdr.type, hdr.num, expected, chunk_num);
            sendto(sfd, &hdr, sizeof(hdr), 0, (sockaddr*)&call, sizeof(call));
        }while(received<(int)(to_be_received+sizeof(hdr))||hdr.type!=expected||hdr.num!=chunk_num);

        memcpy(arr, frame+sizeof(hdr), to_be_received);

        free(frame);
        return to_be_received;
    }

public:

    ReceiverEthernetConnection(int eth_type, char* ifname) {
        struct ifreq ifr;

        sfd = socket(PF_PACKET, SOCK_DGRAM, htons(eth_type));
        strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
        ioctl(sfd, SIOCGIFINDEX, &ifr);
        memset(&sall, 0, sizeof(struct sockaddr_ll));
        sall.sll_family = AF_PACKET;
        sall.sll_protocol = htons(eth_type);
        sall.sll_ifindex = ifr.ifr_ifindex;
        sall.sll_hatype = ARPHRD_ETHER;
        sall.sll_pkttype = PACKET_HOST;
        sall.sll_halen = ETH_ALEN;
        bind(sfd, (struct sockaddr*) &sall, sizeof(struct sockaddr_ll));

    }

    void receiver_loop(string root_directory) {
        while (1) {
            cfd = sfd;//accept(sfd, (struct sockaddr*) &caddr, &sl);
            while (1) {
                int result = receive_file(root_directory);
                // end of transmission
                if (result == 1)
                    break;
            }
        }
    }

    ~ReceiverEthernetConnection() {
        close(sfd);
    }
};
