#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <inttypes.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <sys/ioctl.h>
#include <time.h>
#include <netdb.h>
#include "param.h"
#include "utility.h"
#include "my_socket.h"
#include "myftp.h"
#include "my_file.h"
#include "io.h"

#define TOKEN 3 //格納可能なトークンの初期個数

struct proctable {
        char *cmd;
        void (*func) (int, char **);
};

int status;
int s; //ソケット記述子
in_port_t myport;
int fd; // ファイルディスクリプタ
char port[6] = "50021";
struct sockaddr_in skt; // サーバのソケットアドレス構造体
struct sockaddr_in myskt;
char path_help[DATASIZE]; // helpファイルへのパス
struct myftph_data head_with_data; //データアリのヘッダ
struct myftph head; //データなしのヘッダ
struct addrinfo hints, *res;

/**
 * @brief host名をもとにソケットを作成
 * @param host　ホスト名。0.0.0.0やyahoo.co.jpみたいな形式で。
 * @return  正常に終了すれば０、それ以外は1
 */
int make_socket_by_hostname(char *host) {
        int err;
        memset(&hints, 0, sizeof(hints));
        hints.ai_socktype =  SOCK_STREAM;
        if ((err = getaddrinfo(host, port, &hints, &res)) < 0) {
                fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(err));
                return 1;
        }
        if ((s = socket(res->ai_family, res->ai_socktype,
                        res->ai_protocol)) < 0) {
                perror("socket");
                return 1;
        }
        return 0;
}

void quit(int ac, char **av) {
        memset(&head, 0, sizeof(head));
        init_header(&head, T_QUIT, C_OK, 0);
        if (send_tcp(s, &head, sizeof(head)) < 0)
                fprintf(stderr, "quit\n");
        fprintf(stderr, "Terminated connection\n");
        close(s);
}

void pwd(int ac, char **av) {
        int size;

        if (ac != 1) {
                fprintf(stderr, "Invalid argument.\nUsage: pwd\n");
                return;
        }
        // コマンドメッセージを送信
        memset(&head, 0, sizeof(head));
        init_header(&head, T_PWD, C_OK_CTOS, 0);
        if (send_tcp(s, &head, sizeof(head)) < 0) {
                fprintf(stderr, "pwd\n");
                return;
        }
        //fprintf(stderr, "%s: %s\n",type2str(head.type), code2str(T_OK, head.code));

        // リプライメッセージを受信
        memset(&head_with_data, 0, sizeof(head_with_data));
        if ((size = read(s, &head_with_data, sizeof(head_with_data))) < 0) {
                perror("read");
        }
        //fprintf(stderr, "%s: %s\n", type2str(head_with_data.type),
        //        code2str(head_with_data.type, head_with_data.code));
        //受信データの長さだけデータを表示
        printf("%.*s\n", head_with_data.length, head_with_data.data);
}

void cd(int ac, char **av) {
        int size;
        if (ac != 2) {
                fprintf(stderr, "Invalid argument.\nUsage: cd <target path>\n");
                return;
        }

        // コマンドメッセージを送信
        memset(&head_with_data, 0, sizeof(head_with_data));
        strncpy(head_with_data.data, av[1], strlen(av[1]));
        init_header_with_data(&head_with_data, T_CWD, C_OK_CTOS, strlen(av[1]));
        if (send_tcp(s, &head_with_data, sizeof(head_with_data)) < 0) {
                fprintf(stderr, "cd\n");
                return;
        }

        // リプライメッセージを受信
        memset(&head, 0, sizeof(head));
        if ((size = read(s, &head, sizeof(head))) < 0) {
                perror("read");
                return;
        }
        //fprintf(stderr, "%s: %s\n", type2str(head.type),
        //        code2str(head.type, head.code));

        if (head.type != T_OK)
                fprintf(stderr, "Failed to change directory.\n");
}

void dir(int ac, char **av) {
        struct myftph head;
        char data[DATASIZE];
        ssize_t size; //実際に受信したサイズ
        ssize_t sum = 0; // 受信データの総量
        ssize_t psum; //パケットごとの受信データの総量
        int stat;
        ssize_t len; // 読み込むべきデータサイズ
        char *buf;

        memset(&head_with_data, 0, sizeof(head_with_data));
        switch (ac) {
        case  1:
                init_header_with_data(&head_with_data, T_LIST, C_OK_CTOS, 0);
                break;
        case 2:
                strncpy(head_with_data.data, av[1], strlen(av[1]));
                init_header_with_data(&head_with_data, T_LIST, C_OK_CTOS, strlen(av[1]));
                break;
        default:
                fprintf(stderr, "Invalid arguments.\nUsage: dir [target path]\n");
                return;
        }

        // コマンドメッセージを送信
        if (send_tcp(s, &head_with_data, sizeof(head_with_data)) < 0) {
                fprintf(stderr, "dir\n");
                return;
        }
        //fprintf(stderr, "%s: %s\n", type2str(head_with_data.type),
        //        code2str(T_OK, head_with_data.code));

        // リプライメッセージを受信
        memset(&head, 0, sizeof(head));
        if ((size = read(s, &head, sizeof(head))) < 0) {
                perror("read");
        }
        //fprintf(stderr, "%s: %s\n", type2str(head.type),
        //        code2str(head.type, head.code));

        if (head.type != T_OK) {
                fprintf(stderr, "Failed to get information.\n");
                return;
        }

        // データメッセージを取得
        for (;;) {
                ssize_t i;
                // ヘッダを受信
                memset(&head, 0, sizeof(head));
                if ((size = read(s, &head, sizeof(head))) < 0) {
                        perror("read");
                        return;
                }

                // データを受信
                if (head.code == C_DATA_HAS_NEXT) {
                        //　後続データがあるから、読み込みサイズはDATASIZE
                        len = DATASIZE;
                } else if (head.code == C_DATA_END) {
                        len = head.length;
                } else {
                        fprintf(stderr, "Invalid code\n");
                        exit(1);
                }
                //fprintf(stderr, "len %zd\n", len);
                size = len;
                for (psum = 0; psum < len; ) {
                        memset(data, 0, sizeof(data));
                        //fprintf(stderr, "読み込むべきは%zdバイト\n", size);
                        if ((size = read(s, &data, size * sizeof(char))) < 0) {
                                perror("read");
                                return;
                        }
                        //受信データの長さだけデータを表示
                        printf("%s", data);
                        sum += size;
                        psum += size;
                        size = len - psum;
                        //fprintf(stderr, "psum: %zd\n", psum);
                }

                if (head.code == C_DATA_END) {
                        // 受信終了
                        printf("\n");
                        // ソケット内のデータがなくなるまで読み込む
                        ioctl(s, FIONREAD, &len);
                        mem_alloc(buf, char, sizeof(char) * len, 1);
                        if (len > 0) {
                                len = read(s, buf, len);
                        }
                        free(buf);
                        return;
                }
        }
}

void lpwd(int ac, char **av) {
        char buf[DATASIZE];

        getcwd(buf, DATASIZE);
        fprintf(stderr, "%s\n", buf);
}

void lcd(int ac, char **av) {
        if (ac != 2) {
                fprintf(stderr, "Invalid arguments.\nUsage: lcd <target path>\n");
                return;
        }
        if (chdir(av[1])) {
                perror("chdir");
                return;
        }
}

void ldir(int ac, char **av) {
        char *str;
        int size;

        switch (ac) {
        case 1:
                str = myls(".", &size);
                break;
        case 2:
                str = myls(av[1], &size);
                break;
        default:
                fprintf(stderr, "Invalid argument.\nUsage: ldir [target path]\n");
                return;
        }
        if (str == NULL) {
                fprintf(stderr, "There is no such file %s\n", av[1]);
                return;
        }
        fprintf(stderr, "%s", str);
}

void get(int ac, char **av) {
        ssize_t size;
        char *name;
        int stat;
        int len;
        char *buf;

        switch (ac) {
        case 2:
                // サーバと同じファイル名で保存
                name = av[1];
                break;
        case 3:
                // 指定したファイル名で保存
                name = av[2];
                break;
        default:
                fprintf(stderr, "Invalid arguments\nUsage: get <target path> [name to save]\n");
                exit(1);
        }

        // ファイルを作成し、書き足し可能なモードで開く
        if ((stat = fwrite_open(&fd, name)) != FILE_OPEN) {
                if (stat == FILE_EXIST)
                        fprintf(stderr, "File %s already exists.\n", name);
                else
                        fprintf(stderr, "Failed to open file %s.\n", name);
                return;
        }

        // コマンドメッセージを送信
        memset(&head_with_data, 0, sizeof(head_with_data));
        strncpy(head_with_data.data, av[1], strlen(av[1]));
        init_header_with_data(&head_with_data, T_RETR, C_OK_CTOS, strlen(av[1]));
        if (send_tcp(s, &head_with_data, sizeof(head_with_data)) < 0) {
                fprintf(stderr, "get\n");
                return;
        }
        //fprintf(stderr, "%s: %s\n", type2str(T_OK),
        //        code2str(head.type, head.code));

        // リプライメッセージを受信
        memset(&head, 0, sizeof(head));
        if ((size = read(s, &head, sizeof(head))) < 0) {
                perror("read");
        }
        fprintf(stderr, "%s: %s\n", type2str(head.type),
                code2str(head.type, head.code));
        if (head.type != T_OK) {
                return;
        }

        // データの受信を開始
        if ((size = recv_file(name, s, fd)) < 0) {
                fprintf(stderr, "Falied to receive file\n");
                exit(1);
        }
        // ソケット内のデータがなくなるまで読み込む
        ioctl(s, FIONREAD, &len);
        mem_alloc(buf, char, sizeof(char) * len, 1);
        if (len > 0) {
                len = read(s, buf, len);
        }
        free(buf);
}

void put(int ac, char **av) {
        ssize_t size;
        char *name;
        int len;
        int stat;

        switch (ac) {
        case 2:
                // クライアントと同じファイル名で保存
                name = av[1];
                break;
        case 3:
                // 指定したファイル名で保存
                name = av[2];
                break;
        default:
                fprintf(stderr, "Invalid arguments\nUsage: put  <target path> [name to save]\n");
                exit(1);
        }

        if ((stat = fread_open(&fd, av[1])) != FILE_OPEN) {
                // ファイルをオープンできなかった
                fprintf(stderr, "Failed to open file %s\n", name);
                return;
        }

        // コマンドメッセージを送信
        memset(&head_with_data, 0, sizeof(head_with_data));
        strncpy(head_with_data.data, name, strlen(name));
        init_header_with_data(&head_with_data, T_STOR, C_OK_CTOS, strlen(name));
        if (send_tcp(s, &head_with_data, sizeof(head_with_data)) < 0)
                fprintf(stderr, "put\n");
        // リプライメッセージを受信
        memset(&head, 0, sizeof(head));
        if ((size = read(s, &head, sizeof(head))) < 0) {
                perror("read");
        }
        fprintf(stderr, "%s: %s\n", type2str(head.type),
                code2str(head.type, head.code));
        if (head.type != T_OK)
                return;

        // データの送信を開始
        // ファイルを転送
        size =  send_file(av[1], s, fd);
        // 転送後、ファイルを消去
        if ((stat = remove(name)) != 0) {
                fprintf(stderr, "Failed to remove file\n");
        }
}

void help(int ac, char **av) {
        FILE *fp;
        char buf[100];
        char *c;

        file_open(fp, path_help, "r", 404);
        c = fgets(buf, 100, fp);
        while (c != NULL) {
                fputs(buf, stdout);
                c = fgets(buf, 100, fp);
        }
        fclose(fp);
}

void getargs(int *ac, char **av, int len) {
        char *line;
        mem_alloc(line, char, DATASIZE, 1);
        fgets(line, DATASIZE, stdin);
        *ac = split(line, av, 3);
}

struct proctable ptab [] = {
        {"quit", quit},
        {"pwd", pwd},
        {"cd", cd},
        {"dir", dir},
        {"lpwd", lpwd},
        {"lcd", lcd},
        {"ldir", ldir},
        {"get", get},
        {"put", put},
        {"help", help},
        {"end", NULL}
};

/**
 * @brief 関数を実行する
 * @param  ac 引数の数
 * @param  av 引数
 * @return    QUITなら-1、エラーは-1、正常では0を返す
 */
int execptab(int ac, char **av) {
        struct proctable *pt;
        int i;

        if (ac == 0) return 0;
        for (pt = ptab; strcmp(pt->cmd, "end") != 0; pt++) {
                if ((i = strcmp(pt->cmd,av[0])) == 0) {
                        (*pt->func)(ac, av);
                        break;
                }
        }
        if ((i = strcmp(pt->cmd, "quit")) == 0) {
                // quit
                return 1;
        }
        if ((i = strcmp(pt->cmd, "end")) == 0) {
                // ERROR
                return -1;
        }
        return 0;
}

int main(int argc, char **argv) {
        int ac;
        char **av;

        if (argc != 2) {
                fprintf(stderr, "Usage: ./myftpc <host name>\n");
                exit(0);
        }

        // help.txtへのパス
        getcwd(path_help, DATASIZE);
        strcat(path_help, "/help.txt");
        // 通信を確立
        if (make_socket_by_hostname(argv[1]) == 1) {
                // 通信エラー
                exit(1);
        }
        if (connect(s, res->ai_addr, res->ai_addrlen) < 0) {
                perror("connect");
                exit(1);
        }

        printf("connection established\n");
        while (1) {
                mem_alloc(av, char *, TOKEN, 1);
                printf("\x1b[36m");
                printf("myFTP%% ");
                printf("\x1b[39m");

                //標準入力から入力を得る
                getargs(&ac, av, TOKEN);
                status = execptab(ac, av);
                if (status == 1) {
                        free(av);
                        exit(0);
                }
                if (status == -1) {
                        fprintf(stderr, "There is no such command: %s\n", av[0]);
                }

                free(av);
                av = NULL;
        }
}
