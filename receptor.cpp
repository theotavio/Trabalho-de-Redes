#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

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
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t addr_len = sizeof(clientAddr);

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Falha na criação do socket");
        exit(EXIT_FAILURE);
    }

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    if (bind(sockfd, (const struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Falha no bind");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    int expectedSeqNum = 0;
    std::cout << "Receptor UDP Confiável. Aguardando mensagens..." << std::endl;

    while (true) {
        Packet packet;
        ssize_t recv_len = recvfrom(sockfd, &packet, sizeof(packet), 0, (struct sockaddr *)&clientAddr, &addr_len);
        
        if (recv_len > 0) {
            std::cout << "Pacote recebido com seq=" << packet.seqNum << "." << std::endl;

            if (packet.seqNum == expectedSeqNum) {
                std::cout << "Mensagem entregue: " << packet.data << std::endl;
                
                AckPacket ack;
                ack.ackNum = expectedSeqNum;
                sendto(sockfd, &ack, sizeof(ack), 0, (const struct sockaddr *)&clientAddr, addr_len);
                std::cout << "Enviando ACK para seq=" << expectedSeqNum << "." << std::endl;

                expectedSeqNum = 1 - expectedSeqNum;
            } else {
                std::cout << "Pacote duplicado/fora de ordem. Esperando seq=" << expectedSeqNum << ". Descartando." << std::endl;

                AckPacket ack;
                ack.ackNum = 1 - expectedSeqNum;
                sendto(sockfd, &ack, sizeof(ack), 0, (const struct sockaddr *)&clientAddr, addr_len);
                std::cout << "Reenviando ACK para seq=" << (1-expectedSeqNum) << "." << std::endl;
            }
        }
    }

    close(sockfd);
    return 0;
}