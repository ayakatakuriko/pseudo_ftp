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
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <sys/ioctl.h>
#include "param.h"
#include "utility.h"
#include "my_socket.h"
#include "myftp.h"
#include "my_file.h"

struct proctable {
        uint8_t type;
        void (*func) ();
};

int status;
int s; //ソケット記述子
int cli_s; // accept以降で使う
struct sockaddr_in client;
int clen; // 上記の領域サイズ
in_port_t myport = 50021; // 自ポート
struct sockaddr_in myskt; // 自ソケットアドレス構造体
int pid;
int fd; // ファイルディスクリプタ
struct myftph_data head_with_data; //データアリのヘッダ
struct myftph head; //データなしのヘッダ

void init_socket() {
        if ((s = make_socket(TYPE_TCP)) < 0) {
                // TODO: ERROR
        }
        bind_my_ip(s, myport, &myskt);
        clen = sizeof(client);
}


void quit() {
        fprintf(stderr, "quit!!!\n");
        close(cli_s);
        fprintf(stderr, "Connection terminated(%s).\n", inet_ntoa(client.sin_addr));
}

void pwd() {
        memset(&head_with_data, 0, sizeof(head_with_data));
        getcwd(head_with_data.data, DATASIZE);
        // データを送信
        init_header_with_data(&head_with_data, T_OK, C_OK, strlen(head_with_data.data));
        if (send_tcp(cli_s, &head_with_data, sizeof(head_with_data)) < 0)
                fprintf(stderr, "put()\n");
        fprintf(stderr, "%s: %s\n", type2str(head_with_data.type),
                code2str(head_with_data.type, head_with_data.code));

}

void cd() {
        ssize_t size;
        char path[head.length + 1];

        /* パケットに格納されているファイル名を確保*/
        if ((size = read(cli_s, path, sizeof(path))) < 0) {
                perror("read");
        }
        fprintf(stderr, "cd: %s, len: %d\n", path, head.length);
        memset(&head, 0, sizeof(head));
        if (!chdir(path)) {
                //ディレクトリの移動に成功
                init_header(&head, T_OK, C_OK, 0);
                fprintf(stderr, "Success\n");
        } else {
                // 失敗
                switch (errno) {
                case EACCES:   //pathの構成要素のいずれかに検索許可がない
                        init_header(&head, T_FILE_ERR, C_PERMISSION_DENID, 0);
                        break;
                case ENOENT:   // そのディレクトリは存在しない
                        init_header(&head, T_FILE_ERR, C_FILE_NOT_EXIST, 0);
                        break;
                case ENOTDIR:   //pathの構成要素がディレクトリでない
                        init_header(&head, T_CMD_ERR, C_SYNTAX_ERR, 0);
                        break;
                default:
                        init_header(&head, T_UNKNOWN_ERR, C_UNKNOWN_ERR, 0);
                        break;
                }
        }
        if (send_tcp(cli_s, &head, sizeof(head)) < 0)
                fprintf(stderr, "cd()\n");
        fprintf(stderr, "%s: %s\n", type2str(head.type),
                code2str(head.type, head.code));

}

void dir() {
        int size;
        int dsize; //データサイズ
        char path[head.length + 1];
        char *str, *temp;
        int i;//

        if (head.length > 0) {
                /* パケットに格納されているファイル名を確保*/
                if ((size = read(cli_s, path, sizeof(path))) < 0) {
                        perror("read");
                }
                // 指定されたディレクトリの情報を格納
                str = myls(path, &dsize);
        } else {
                // カレントディレクトリの情報を格納
                str = myls(".", &dsize);
        }

        memset(&head, 0, sizeof(head));
        // エラーチェック
        if (str == NULL) {
                switch (dsize) {
                case EFAULT:   //アドレスが間違っている
                case ENOTDIR:// パスの構成要素がディレクトリでない
                        init_header(&head, T_CMD_ERR, C_SYNTAX_ERR, 0);
                        break;
                case ENOENT: //パスの構成要素が存在しない
                        init_header(&head, T_FILE_ERR, C_FILE_NOT_EXIST, 0);
                        break;
                case EACCES: //検索許可がない
                        init_header(&head, T_FILE_ERR, C_PERMISSION_DENID, 0);
                        break;
                default:
                        init_header(&head, T_UNKNOWN_ERR, C_UNKNOWN_ERR, 0);
                        break;
                }
                // データを送信
                if (send_tcp(cli_s, &head, sizeof(head)) < 0)
                        fprintf(stderr, "dir()\n");
                fprintf(stderr, "%s: %s\n", type2str(head.type),
                        code2str(head.type, head.code));
                return;
        }

        // エラーはなかった
        // リプライメッセージを送信
        init_header(&head, T_OK, C_OK_STOC, 0);
        if (send_tcp(cli_s, &head, sizeof(head)) < 0)
                fprintf(stderr, "dir()\n");
        fprintf(stderr, "%s: %s\n", type2str(head.type),
                code2str(head.type, head.code));

        // dsizeがDATASIZEより大きいなら分割して送信
        temp = str;
        while (dsize > DATASIZE) {
                memset(&head_with_data, 0, sizeof(head_with_data));
                // パケットにDATAIZE分だけコピー
                strncpy(head_with_data.data, temp, DATASIZE);
                // データを送信
                init_header_with_data(&head_with_data, T_DATA, C_DATA_HAS_NEXT, DATASIZE);
                if (send_tcp(cli_s, &head_with_data, sizeof(head_with_data)) < 0)
                        fprintf(stderr, "dir()\n");
                fprintf(stderr, "%s: %s Send %zu byts.\n", type2str(head_with_data.type),
                        code2str(head_with_data.type, head_with_data.code), DATASIZE);
                // tempの先頭を指すポインタをずらす。
                temp += DATASIZE;
                dsize -= DATASIZE;
        }
        // dsizeがDATASIZE以下
        memset(&head_with_data, 0, sizeof(head_with_data));
        // パケットにdsize分だけコピー
        strncpy(head_with_data.data, temp, dsize);
        // データを送信
        init_header_with_data(&head_with_data, T_DATA, C_DATA_END, dsize);
        if (send_tcp(cli_s, &head_with_data, sizeof(head_with_data)) < 0)
                fprintf(stderr, "dir()\n");
        fprintf(stderr, "%s: %s Send %zu byts.\n", type2str(head_with_data.type),
                code2str(head_with_data.type, head_with_data.code), dsize);
        free(str);
        str = NULL;
}

void get() {
        ssize_t size;
        char name[head.length + 1];
        char buf[DATASIZE];
        int stat;

        /* パケットに格納されているファイル名を確保*/
        if ((size = read(cli_s, buf, sizeof(buf))) < 0) {
                perror("read");
        }
        memset(name, 0, sizeof(name));
        strncpy(name, buf, head.length);

        memset(&head, 0, sizeof(head));
        // リプライを返す
        if ((stat = fread_open(&fd, name)) != FILE_OPEN) {
                fprintf(stderr, "Failed to open file %s.\n", name);
                switch (errno) {
                case EACCES:
                        init_header(&head, T_FILE_ERR, C_PERMISSION_DENID, 0);
                        break;
                case ENOENT:
                        init_header(&head, T_FILE_ERR, C_FILE_NOT_EXIST, 0);
                        break;
                case ENOTDIR:
                        init_header(&head, T_CMD_ERR, C_SYNTAX_ERR, 0);
                        break;
                default:
                        init_header(&head, T_UNKNOWN_ERR, C_UNKNOWN_ERR, 0);
                        break;
                }
                if (send_tcp(cli_s, &head, sizeof(head)) < 0)
                        fprintf(stderr, "put()\n");
                fprintf(stderr, "%s: %s\n", type2str(head.type),
                        code2str(head.type, head.code));
                return;
        }

        init_header(&head, T_OK, C_OK_STOC, 0);
        if (send_tcp(cli_s, &head, sizeof(head)) < 0) {
                fprintf(stderr, "put()\n");
                return;
        }
        fprintf(stderr, "%s: %s\n", type2str(head.type),
                code2str(head.type, head.code));

        // ファイルを転送
        size =  send_file(name, cli_s, fd);
        // 転送後、ファイルを消去
        if ((stat = remove(name)) != 0) {
                fprintf(stderr, "Failed to remove file\n");
        }
}

void put() {
        ssize_t size;
        char name[head.length + 1];
        char buf[DATASIZE];
        int stat;

        /* パケットに格納されているファイル名を確保*/
        if ((size = read(cli_s, buf, sizeof(buf))) < 0) {
                perror("read");
        }
        memset(name, 0, sizeof(name));
        strncpy(name, buf, head.length);

        // ファイルdistを作成し、書き足し可能なモードで開く
        if ((stat = fwrite_open(&fd, name)) != FILE_OPEN) {
                // ファイルをオープンできなかった
                if (stat == FILE_EXIST)
                        fprintf(stderr, "File %s already exists.\n", name);
                else
                        fprintf(stderr, "Failed to open file %s.\n", name);
                memset(&head, 0, sizeof(head));
                init_header(&head, T_UNKNOWN_ERR, C_UNKNOWN_ERR, 0);
                if (send_tcp(cli_s, &head, sizeof(head)) < 0)
                        fprintf(stderr, "put()\n");
                fprintf(stderr, "%s: %s\n", type2str(head.type),
                        code2str(head.type, head.code));
                return;
        }

        // リプライを返す
        memset(&head, 0, sizeof(head));
        init_header(&head, T_OK, C_OK_CTOS, 0);
        if (send_tcp(cli_s, &head, sizeof(head)) < 0)
                fprintf(stderr, "put()\n");
        fprintf(stderr, "%s: %s\n", type2str(head.type),
                code2str(head.type, head.code));


        // データの受信を開始
        if ((size = recv_file(name, cli_s, fd)) < 0) {
                fprintf(stderr, "Falied to receive file\n");
                exit(1);
        }
}

int main (int argc, char **argv) {
        struct proctable ptab[] = {
                {T_QUIT, quit},
                {T_PWD, pwd},
                {T_CWD, cd},
                {T_LIST, dir},
                {T_RETR, get},
                {T_STOR, put},
                {0, NULL}
        };
        // 引数の確認
        switch (argc) {
        case 1:
                // ディレクトリをデフォルトに設定
                break;
        case 2:
                // ディレクトリを与えられた引数に設定
                if (chdir(argv[1])) {
                        fprintf(stderr, "Invalid path %s\n", argv[1]);
                        exit(0);
                }
                break;
        default:
                fprintf(stderr, "Usage: myftps [Path to Directory]\n");
                exit(0);
        }

        // ソケットの初期化
        init_socket();

        while (1) {
                // クライアントからのコネクション確立要求待ち
                fprintf(stderr, "Wait connection...\n");
                prepare_tcp(s);
                // コネクション確立
                if ((cli_s = accept(s, (struct sockaddr *)&client, &clen)) < 0) {
                        perror("accept");
                }

                //　子プロセスを作成
                if ((pid = fork()) == 0) {
                        // 子プロセス
                        struct proctable *pt;
                        ssize_t size;
                        char *buf;

                        fprintf(stderr, "Connect to %s.\n", inet_ntoa(client.sin_addr));
                        while (1) {
                                memset(&head, 0, sizeof(head));
                                // クライアントからのヘッダを受信し、実行するコマンドを決定
                                if ((size = read(cli_s, &head, 4)) < 0) {
                                        fprintf(stderr, "Recv header Error\n");
                                }
                                fprintf(stderr, "Command %s\n", type2str(head.type));
                                for (pt = ptab; pt->type; pt++) {
                                        if (pt->type == head.type) {
                                                (*pt->func)();
                                                // ソケット内のデータがなくなるまで読み込む
                                                int len = 0;
                                                ioctl(cli_s, FIONREAD, &len);
                                                mem_alloc(buf, char, sizeof(char) * len, 1);
                                                if (len > 0) {
                                                        len = read(cli_s, buf, len);
                                                }
                                                free(buf);
                                                break;
                                        }
                                }
                                if (pt->type == T_QUIT) {
                                        exit(0);
                                }
                                if (pt->type == 0) {
                                        fprintf(stderr, "Error: Unexpected command was executed\n");
                                        exit(1);
                                }
                        }
                } else if (pid < 0) {
                        perror("fork");
                }
        }
}
