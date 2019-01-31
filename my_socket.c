/**
 * my_socket.c
 * @brief ソケットの割り当てなどを行う関数群
 * @author AYAKA OHWADA
 * @deta 208/12/21
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include "my_socket.h"


/**
 * @brief ソケットを生成する。
 * @param protocol TYPE_TCPかTYPE_UDPを指定。
 * @return ソケット記述子. エラーの時は負数を返す.
 */
int make_socket(int protcol) {
        int s;
        int type;

        switch (protcol) {
        case TYPE_TCP:
                type = SOCK_STREAM;
                break;
        case TYPE_UDP:
                type = SOCK_DGRAM;
                break;
        default:
                fprintf(stderr, "ERROR: Invalid protocol\n");
                return -1;
        }

        if ((s = socket(AF_INET, type, 0)) < 0) {
                perror("socket");
                return -1;
        }

        return s;
}

/**
 * @brief ソケットを初期化する関数
 * @param s ソケット記述子
 * @param port 割り当てるポート。ネットワークバイトオーダーでない。
 * @param skt ソケットアドレス構造体
 * @param ip IPアドレス。ネットワークバイトオーダー
 */
void init_skt(in_port_t port, struct sockaddr_in *skt, in_addr_t ip) {
        memset(skt, 0, sizeof *skt);
        skt->sin_family = AF_INET;
        skt->sin_port = htons(port);
        skt->sin_addr.s_addr = ip;
}

/**
 * @brief ソケットの初期化と割り当てを行う関数
 * @param s ソケット記述子
 * @param port 割り当てるポート。ネットワークバイトオーダーでない。
 * @param skt ソケットアドレス構造体
 * @param ip IPアドレス。ネットワークバイトオーダー
 */
int bind_socket(int s, in_port_t port, struct sockaddr_in *skt, in_addr_t ip) {
        init_skt(port, skt, ip);
        if (bind(s, (struct sockaddr *)skt, sizeof *skt) < 0) {
                perror("bind");
                return -1;
        }
        return 0;
}

/**
 * @brief 自分のIPアドレスを設定する
 * @param s ソケット記述子
 * @param port 割り当てるポート。ネットワークバイトオーダーでない。
 * @param skt ソケットアドレス構造体
 */
int bind_my_ip(int s, in_port_t port, struct sockaddr_in *skt) {
        in_addr_t ip = htonl(INADDR_ANY);
        return bind_socket(s, port, skt, ip);
}

/**
 * @brief 任意のIPアドレスで初期化。
 * @param s ソケット記述子
 * @param port 割り当てるポート。ネットワークバイトオーダーでない。
 * @param skt ソケットアドレス構造体
 * @param ip IPアドレス。0.0.0.0のような形式で与える
 * @return 正常なら0を返す。
 */
int init_specific_ip(in_port_t port, struct sockaddr_in *skt, char *ip) {
        struct in_addr temp;
        if (inet_aton(ip, &temp) < 1) {
                fprintf(stderr, "ERROR: bind\n");
                return 1;
        }
        init_skt(port, skt, temp.s_addr);
        return 0;
}


/**
 * @brief UDPでデータを送信する
 * @param  skt     ソケット記述子
 * @param  msg     送信データが格納されている領域のポインタ
 * @param  datalen 送信データのサイズ
 * @param  dist    送信先のソケットアドレス構造体のポインタ
 * @return         送信したデータのサイズ。エラーのときは―1が返る。
 */
int send_udp(int skt, void *msg, int datalen, struct sockaddr_in *dist) {
        int count;
        if ((count = sendto(skt, msg, datalen, 0, (struct sockaddr *)dist, sizeof *dist)) < 0)
                perror("sendto");
        return count;
}

/**
 * @brief UDPでデータを受け取る
 * @param  s       ソケット記述子
 * @param  msg     受信データを格納する領域のポインタ
 * @param  from    送信側ソケットの情報を格納するポインタ
 * @param datalen
 * @return         受信したデータのサイズ。エラーなら―1を返す。
 */
int recv_udp(int s, void *msg, struct sockaddr_in *from, size_t datalen) {
        int count;
        socklen_t sktlen = sizeof(*from);
        if ((count = recvfrom(s, msg, datalen, 0, (struct sockaddr *)from, &sktlen)) < 0)
                perror("recvfrom");
        return count;
}

/**
 * @brief TCPにおけるコネクション確立要求受信準備を行う
 * @param  s ソケット記述子
 * @return   [description]
 */
int prepare_tcp(int s) {
        int error;
        if ((error = listen(s, 5)) < 0) {
                perror("listen");
        }
        return error;
}

/**
 * @brief コネクション確立要求を行う
 * @param  s    ソケット記述子
 * @param  name ソケットアドレス構造体のポインタ
 * @return      [description]
 */
int connect_tcp(int s, struct sockaddr_in *name) {
        int error;
        if ((error = connect(s, (struct sockaddr *)name, sizeof(*name))) < 0) {
                perror("connect");
        }
        return error;
}

/**
 * @brief TCPで受信
 * @param  s   ソケット記述子
 * @param  buf 受信データを格納するバッファ
 * @return     受信したデータサイズ
 */
ssize_t recv_tcp(int s, void *buf) {
        ssize_t size;
        if ((size = recv(s, buf, sizeof(*buf), 0)) < 0) {
                perror("recv");
        }
        return size;
}

/**
 * @brief TCPで送信
 * @param  s   ソケット記述子
 * @param  buf 送信データへのポインタ
 * @param len 送信データサイズ
 * @return     送信したデータサイズ
 */
ssize_t send_tcp(int s, void *buf, size_t len) {
        ssize_t size;
        if ((size = send(s, buf, len, 0)) < 0) {
                perror("send");
        }
        return size;
}
