#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "my_file.h"

/**
 * @brief ファイルの最後にデータを追加できるようにファイルを作成する。
 * @param  fd    ファイル記述子を格納するポインタ
 * @param  fname 作成するファイル名
 * @return ファイルのステータスを返す。
 *         すでにファイルが存在する場合はFILE_EXISTを返す
 */
int fwrite_open(int *fd, char *fname) {
        if ((*fd = open(fname, O_CREAT | O_EXCL | O_WRONLY | O_APPEND, S_IREAD | S_IWRITE)) < 0) {
                if (errno == EEXIST) {
                        // ファイルはすでに存在していた
                        fprintf(stderr, "%s already exists.\n", fname);
                        return FILE_EXIST;
                }
                perror("open");
                return FILE_ERRO;
        }
        return FILE_OPEN;
}

/**
 * @brief ファイルをread-onlyでオープン
 * @param  fd    ファイル記述子を格納するポインタ
 * @param  fname 作成するファイル名
 * @return ファイルのステータスを返す。
 */
int fread_open(int *fd, char *fname) {
        if ((*fd = open(fname, O_RDONLY, 0644)) < 0) {
                perror("open");
                return FILE_ERRO;
        }
        return FILE_OPEN;
}
