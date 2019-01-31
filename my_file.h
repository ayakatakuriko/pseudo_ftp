#ifndef MY_FILE_H
#define MY_FILE_H

/* ファイルをオープンした際に返すステータス*/
#define FILE_OPEN 0 //正常にオープン
#define FILE_EXIST -1 //ファイルは既に存在する
#define FILE_ERRO -2 // 何らかのエラー

/* ファイル関連のステータス*/
#define WRITE_ERROR -3 //正常にwriteできなかった
#define READ_ERROR -4

/**
 * @brief ファイルの最後にデータを追加できるようにファイルを作成する。
 * @param  fd    ファイル記述子を格納するポインタ
 * @param  fname 作成するファイル名
 * @return ファイルのステータスを返す。
 *         すでにファイルが存在する場合はFILE_EXISTを返す
 */
int fwrite_open(int *fd, char *fname);

/**
 * @brief ファイルをread-onlyでオープン
 * @param  fd    ファイル記述子を格納するポインタ
 * @param  fname 作成するファイル名
 * @return ファイルのステータスを返す。
 */
int fread_open(int *fd, char *fname);

#endif
