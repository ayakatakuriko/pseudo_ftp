#ifndef MYFTP_H
#define MYFTP_H

/**
 * @brief データアリのヘッダを初期化
 * @param head [description]
 * @param type [description]
 * @param code [description]
 * @param len  [description]
 */
void init_header_with_data(struct myftph_data *head, uint8_t type, uint8_t code,
                           uint16_t len);

/**
 * @brief データなしのヘッダを初期化
 * @param head [description]
 * @param type [description]
 * @param code [description]
 * @param len  [description]
 */
void init_header(struct myftph *head, uint8_t type, uint8_t code, uint16_t len);

/**
 * @brief ファイルを受信する関数
 * @param dist 書き込み先ファイル名
 * @param s ソケット記述子
 * @param fd ファイルディスクリプタ
 * @return 書き込んだバイト数。エラー時は負の数
 */
ssize_t recv_file(char *dist, int s, int fd);

/**
 * @brief ファイルを送信する
 * @param sourcef 送信するファイル名
 * @param s ソケット記述子
 * @param fd ファイルディスクリプタ
 * @return 書き込んだバイト数。エラー時は負の数
 */
int send_file(char *sourcef, int s, int fd);

/**
 * @brief lsの出力を文字列として返す。
 * @param  path [description]
 * @return      [description]
 */
char *myls(char *path, int *size);

/**
 * @brief 2つのパスをつなげる
 * @param path  新たなパスの格納先
 * @param path1 [description]
 * @param path2 [description]
 */
void joinpath(char *path, const char *path1, const char *path2);

/**
 * @brief fstatに格納されたファイルデータを文字列にして返す
 * @param  name  ファイル（ディレクトリ）名
 * @param  fstat ファイルの属性
 * @param size 返す文字列の長さを格納
 * @return       ファイルデータの文字列
 */
char *file2str(char *name, struct stat fstat, int *size);

/**
 * @brief Typeを文字列に変換
 * @param  type 変換したいtype
 * @return      変換された文字列。該当するtypeがないときはNULLを返す
 */
char *type2str(uint8_t type);

/**
 * @brief codeを文字列に変換
 * @param  type [description]
 * @param  code [description]
 * @return      [description]
 */
char *code2str(uint8_t type, uint8_t code);
#endif
