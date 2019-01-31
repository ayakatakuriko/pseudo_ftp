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
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <sys/stat.h>
#include "param.h"
#include "utility.h"
#include "myftp.h"
#include "my_file.h"
#include "my_socket.h"


/**
 * @brief データアリのヘッダを初期化
 * @param head [description]
 * @param type [description]
 * @param code [description]
 * @param len  [description]
 */
void init_header_with_data(struct myftph_data *head, uint8_t type, uint8_t code,
                           uint16_t len) {
        head->type = type;
        head->code = code;
        head->length = len;
}

/**
 * @brief データなしのヘッダを初期化
 * @param head [description]
 * @param type [description]
 * @param code [description]
 * @param len  [description]
 */
void init_header (struct myftph *head, uint8_t type, uint8_t code, uint16_t len) {
        head->type = type;
        head->code = code;
        head->length = len;
}

/**
 * @brief ファイルを受信する関数
 * @param dist 書き込み先ファイル名
 * @return 書き込んだバイト数。エラー時は負の数
 */
ssize_t recv_file(char *dist, int s, int fd) {
        struct myftph head;
        char data[DATASIZE];
        ssize_t size; //実際に受信したサイズ
        ssize_t sum = 0; // 受信データの総量
        ssize_t psum; //パケットごとの受信データの総量
        int stat;
        ssize_t len; // 読み込むべきデータサイズ

        for (;;) {
                ssize_t i;
                // ヘッダを受信
                memset(&head, 0, sizeof(head));
                if ((size = read(s, &head, sizeof(head))) < 0) {
                        perror("read");
                        return errno;
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
                                return errno;
                        }
                        // ファイルにデータを書き込む
                        //  fprintf(stderr, "実際に読んだのは%zdバイト\n", size);
                        if ((size = write(fd, data, size)) < 0) {
                                perror("write");
                                close(fd);
                                return WRITE_ERROR;
                        }
                        sum += size;
                        fprintf(stderr, "Receive %4zu bytes.(total %zu)\n", size, sum);
                        psum += size;
                        size = len - psum;
                        //fprintf(stderr, "psum: %zd\n", psum);
                }
                if (head.code == C_DATA_END) {
                        // 受信終了
                        fprintf(stderr, "Finished receiving\n");
                        close(fd);
                        return sum;
                }
        }
}

/**
 * @brief ファイルを送信する
 * @param sourcef 送信するファイル名
 * @return 書き込んだバイト数。エラー時は負の数
 */
int send_file(char *sourcef, int s, int fd) {
        struct myftph_data crr, next;
        ssize_t size, nexts; //実際に送信したサイズ
        ssize_t sum = 0;
        int stat;


        /*一回目の読み込み*/
        memset(&crr, 0, sizeof(crr));
        if ((size = read(fd, crr.data, DATASIZE)) < 0) {
                perror("read");
                close(fd);
                return READ_ERROR;
        }
        crr.length = size;
        for (;;) {// ファイルの終わりまで
                memset(&next, 0, sizeof(next));
                // ファイルからデータを読む
                nexts = read(fd, next.data, DATASIZE);
                switch (nexts) {
                case -1:
                        perror("read");
                        close(fd);
                        return READ_ERROR;
                case 0:
                        // end of file
                        // headerに情報を詰める
                        next.length = nexts;
                        init_header_with_data(&crr, T_DATA, C_DATA_END, size);
                        break;
                default:
                        // 処理を続行
                        init_header_with_data(&crr, T_DATA, C_DATA_HAS_NEXT, size);
                        break;
                }

                //データを送信
                if ((size = send_tcp(s, &crr, sizeof(crr))) < 0) {
                        close(fd);
                        return -1;
                }
                sum += crr.length;
                fprintf(stderr, "Send %4zu bytes.(total %zu)\n", crr.length, sum);
                if (crr.code == C_DATA_END) {
                        // 送信終わり
                        close(fd);
                        fprintf(stderr, "Finished sending\n");
                        return sum;
                }
                crr = next; //次に送るデータを準備
                size = nexts;
        }
}

/**
 * @brief lsの出力を文字列として返す。現在のディレクトリを出すときは"."
 * @param  path 出力をしたいファイル、ディレクトリへのパス
 * @param size 出力した文字数を格納。最後の改行は含まない。
 * @return      lsの出力結果。エラーのときはNULLを返し、sizeにerrnoを格納
 */
char *myls(char *path, int *size) {
        char *output;
        int isdir; //ディレクトリなら1
        char *p;
        DIR *dp;
        struct dirent *dr;
        struct stat fstat;
        int err;
        *size = 0;
        int dsize = DATASIZE;

        // pathが指すのはディレクトリか、ファイルなのか
        if ((err = stat(path, &fstat)) < 0) {
                perror("stat");
                *size = errno;
                return NULL;
        }
        if (S_ISDIR(fstat.st_mode)) {
                // ディレクトリだった
                isdir = 1;
        } else {
                // ファイルだった
                isdir = 0;
        }


        if (!isdir) {
                // ファイルについて処理
                return file2str(path, fstat, size);
        }
        // ディレクトリについて
        // Open current directory
        if ((dp = opendir(path)) == NULL) {
                perror("open directory");
                *size = errno;
                return NULL;
        }

        // ディレクトリの内部情報を1行ずつ得る。
        mem_alloc(output, char, dsize, 1);
        memset(output, 0, dsize);
        while ((dr = readdir(dp)) != NULL) {
                int temp;
                char *str;
                char new_path[DATASIZE];

                joinpath(new_path, path, dr->d_name);
                if ((err = stat(new_path, &fstat)) == -1) {
                        perror("stat");
                        exit(1);
                }
                str = file2str(dr->d_name, fstat, &temp);
                if ((*size + temp) > dsize) {
                        dsize *= 2;
                        //fprintf(stderr, "realloc: %d\n", dsize);
                        mem_realloc(output, char, dsize, 1);
                }
                strcat(output, str);
                *size += temp;
        }
        closedir(dp);
        *size -= 1;
        return output;
}

/**
 * @brief 2つのパスをつなげる
 * @param path  新たなパスの格納先
 * @param path1 [description]
 * @param path2 [description]
 */
void joinpath(char *path, const char *path1, const char *path2)
{
        strcpy(path, path1);
        strcat(path, "/");
        strcat(path, path2);

        return;
}

/**
 * @brief fstatに格納されたファイルデータを文字列にして返す
 * @param  name  ファイル（ディレクトリ）名
 * @param  fstat ファイルの属性
 * @param size 返す文字列の長さを格納
 * @return       ファイルデータの文字列
 */
char *file2str(char *name, struct stat fstat, int *size) {
        struct tm *mtime;
        struct passwd *p;
        struct group *grp;
        char *str;
        char temp1[2];
        char temp2[DATASIZE - 2];

        if ((p = getpwuid(fstat.st_uid)) == NULL) {
                perror("getpwuid()");
                exit(2);
        }
        if ((grp = getgrgid(fstat.st_gid)) == NULL) {
                perror("getgrgit()");
                exit(2);
        }

        mtime = localtime(&fstat.st_mtime);
        if (S_ISDIR(fstat.st_mode))
                sprintf(temp1, "d");
        else if (S_ISCHR(fstat.st_mode))
                sprintf(temp1, "c");
        else if (S_ISLNK(fstat.st_mode))
                sprintf(temp1, "s");
        else
                sprintf(temp1, "-");
        snprintf(temp2, DATASIZE - 2, "%s%s%s%s%s%s%s%s%s %d %s %s %8d  %02d/%02d %02d:%02d %s\n",
                 (fstat.st_mode & S_IRUSR) ? "r" : "-",
                 (fstat.st_mode & S_IWUSR) ? "w" : "-",
                 (fstat.st_mode & S_IXUSR) ? "x" : "-",
                 (fstat.st_mode & S_IRGRP) ? "r" : "-",
                 (fstat.st_mode & S_IWGRP) ? "w" : "-",
                 (fstat.st_mode & S_IXGRP) ? "x" : "-",
                 (fstat.st_mode & S_IROTH) ? "r" : "-",
                 (fstat.st_mode & S_IWOTH) ? "w" : "-",
                 (fstat.st_mode & S_IXOTH) ? "x" : "-", fstat.st_nlink, p->pw_name,
                 grp->gr_name, fstat.st_size, mtime->tm_mon + 1, mtime->tm_mday,
                 mtime->tm_hour, mtime->tm_min, name);
        mem_alloc(str, char, DATASIZE, 1);
        *size = snprintf(str, DATASIZE, "%s%s", temp1, temp2);
        return str;
}

/**
 * @brief Typeを文字列に変換
 * @param  type 変換したいtype
 * @return      変換された文字列。該当するtypeがないときはNULLを返す
 */
char *type2str(uint8_t type) {
        struct typetable {
                uint8_t type;
                char *str;
        } ttab[] = {
                {T_QUIT, "quit"},
                {T_PWD, "pwd"},
                {T_CWD, "cd"},
                {T_LIST, "dir"},
                {T_RETR, "get"},
                {T_STOR, "put"},
                {T_OK, "Command OK"},
                {T_CMD_ERR, "Command Error"},
                {T_FILE_ERR, "File Error"},
                {T_UNKNOWN_ERR, "Unknown Error"},
                {T_DATA, "Data"},
                {0, NULL}
        };

        struct typetable *tp;
        for (tp = ttab; tp->type != 0; tp++) {
                if (tp->type == type)
                        return tp->str;
        }
        return tp->str;
}

/**
 * @brief codeを文字列に変換
 * @param  type [description]
 * @param  code [description]
 * @return      [description]
 */
char *code2str(uint8_t type, uint8_t code) {
        struct codetable {
                uint8_t code;
                char *str;
        };
        uint8_t err = 10;

        struct codetable ctab[] = {
                {C_OK, "Execute command."},
                {C_OK_STOC, "Execute command. Followed by DATA.(server->client)"},
                {C_OK_CTOS, "Execute command. Followed by DATA.(client->server)"},
                {err, NULL}
        };
        struct codetable etab[] = {
                {C_SYNTAX_ERR, "Syntax Error."},
                {C_UNKNOWN_CMD, "Unknown Command."},
                {C_PROTOCOL_ERR, "Protocol Error."},
                {err, NULL}
        };
        struct codetable ftab[] = {
                {C_FILE_NOT_EXIST, "There is no such file or directory."},
                {C_PERMISSION_DENID, "Permission denid."},
                {err, NULL}
        };
        struct codetable dtab[] = {
                {C_DATA_HAS_NEXT, "There is data yet."},
                {C_DATA_END, "End of data."},
                {err, NULL}
        };
        struct codetable *cp;

        switch (type) {
        case T_OK:
                for (cp = ctab; cp->code != err; cp++) {
                        if (cp->code == code)
                                return cp->str;
                }
                return cp->str;
        case T_CMD_ERR:
                for (cp = etab; cp->code != err; cp++) {
                        if (cp->code == code)
                                return cp->str;
                }
                return cp->str;
        case T_FILE_ERR:
                for (cp = ftab; cp->code != err; cp++) {
                        if (cp->code == code)
                                return cp->str;
                }
                return cp->str;
        case T_UNKNOWN_ERR:
                return "Undefine Error.";
        case T_DATA:
                for (cp = dtab; cp->code != err; cp++) {
                        if (cp->code == code)
                                return cp->str;
                }
                return cp->str;
        default:
                return NULL;
        }
}
