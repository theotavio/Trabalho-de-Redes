#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>

#define SERVER_IP "127.0.0.1"
#define PORT 8080
#define BUFFER_SIZE 1024

struct Packet {
    int seqNum;
    char data[BUFFER_SIZE];
};

struct AckPacket {
    int ackNum;
};

int main() {
    int sockfd;
    struct sockaddr_in serverAddr;
    socklen_t addr_len = sizeof(serverAddr);

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Falha na criação do socket");
        exit(EXIT_FAILURE);
    }

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);

    struct timeval tv;
    tv.tv_sec = 2;
    tv.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("Erro ao definir timeout");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    std::string message;
    int seqNum = 0;

    std::cout << "Transmissor UDP Confiável. Digite 'sair' para terminar." << std::endl;
    
    while (true) {
        std::cout << "Digite a mensagem para enviar: ";
        std::getline(std::cin, message);

        if (message == "sair") {
            break;
        }

        Packet packet;
        packet.seqNum = seqNum;
        strncpy(packet.data, message.c_str(), BUFFER_SIZE - 1);
        packet.data[BUFFER_SIZE - 1] = '\0';

        bool ackReceived = false;
        while (!ackReceived) {
            std::cout << "Enviando pacote com seq=" << seqNum << "..." << std::endl;
            sendto(sockfd, &packet, sizeof(packet), 0, (const struct sockaddr *)&serverAddr, addr_len);

            AckPacket ack;
            ssize_t recv_len = recvfrom(sockfd, &ack, sizeof(ack), 0, (struct sockaddr *)&serverAddr, &addr_len);

            if (recv_len > 0) {
                if (ack.ackNum == seqNum) {
                    std::cout << "ACK para seq=" << ack.ackNum << " recebido." << std::endl;
                    ackReceived = true;
                    seqNum = 1 - seqNum;
                } else {
                    std::cout << "ACK duplicado/inválido recebido (esperando " << seqNum << ", recebeu " << ack.ackNum << "). Ignorando." << std::endl;
                }
            } else {
                 std::cout << "Timeout! Retransmitindo pacote com seq=" << seqNum << "..." << std::endl;
            }
        }
    }

    close(sockfd);
    return 0;
}