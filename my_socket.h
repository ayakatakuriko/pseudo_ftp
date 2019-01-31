#define TYPE_TCP 0x00
#define TYPE_UDP 0x01

/**
 * @brief ソケットを生成する。
 * @param protocol TYPE_TCPかTYPE_UDPを指定。
 * @return ソケット記述子. エラーの時は負数を返す.
 */
int make_socket(int protcol);

/**
 * @brief ソケットを初期化する関数
 * @param s ソケット記述子
 * @param port 割り当てるポート。ネットワークバイトオーダーでない。
 * @param skt ソケットアドレス構造体
 * @param ip IPアドレス。ネットワークバイトオーダー
 */
void init_skt(in_port_t port, struct sockaddr_in *skt, in_addr_t ip);

/**
 * @brief ソケットの初期化と割り当てを行う関数
 * @param s ソケット記述子
 * @param port 割り当てるポート。ネットワークバイトオーダーでない。
 * @param skt ソケットアドレス構造体
 * @param ip IPアドレス。ネットワークバイトオーダー
 */
int bind_socket(int s, in_port_t port, struct sockaddr_in *skt, in_addr_t ip);

/**
 * @brief 自分のIPアドレスを設定する
 * @param s ソケット記述子
 * @param port 割り当てるポート。ネットワークバイトオーダーでない。
 * @param skt ソケットアドレス構造体
 */
int bind_my_ip(int s, in_port_t port, struct sockaddr_in *skt);

/**
 * @brief 任意のIPアドレスで初期化。
 * @param s ソケット記述子
 * @param port 割り当てるポート。ネットワークバイトオーダーでない。
 * @param skt ソケットアドレス構造体
 * @param ip IPアドレス。0.0.0.0のような形式で与える
 * @return 正常なら0を返す。
 */
int init_specific_ip(in_port_t port, struct sockaddr_in *skt, char *ip);

/**
 * @brief UDPでデータを送信する
 * @param  skt     ソケット記述子
 * @param  msg     送信データが格納されている領域のポインタ
 * @param  datalen 送信データのサイズ
 * @param  dist    送信先のソケットアドレス構造体のポインタ
 * @return         送信したデータのサイズ。エラーのときは―1が返る。
 */
int send_udp(int skt, void *msg, int datalen, struct sockaddr_in *dist);

/**
 * @brief UDPでデータを受け取る
 * @param  s       ソケット記述子
 * @param  msg     受信データを格納する領域のポインタ
 * @param  from    送信側ソケットの情報を格納するポインタ
 * @return         受信したデータのサイズ。エラーなら―1を返す。
 */
int recv_udp(int s, void *msg, struct sockaddr_in *from, size_t datalen);
/**
 * @brief TCPにおけるコネクション確立要求受信準備を行う
 * @param  s ソケット記述子
 * @return   [description]
 */
int prepare_tcp(int s);
/**
 * @brief コネクション確立要求を行う
 * @param  s    ソケット記述子
 * @param  name ソケットアドレス構造体のポインタ
 * @return      [description]
 */
int connect_tcp(int s, struct sockaddr_in *name);

/**
 * @brief TCPで受信
 * @param  s   ソケット記述子
 * @param  buf 受信データを格納するバッファ
 * @return     受信したデータサイズ
 */
ssize_t recv_tcp(int s, void *buf);

/**
 * @brief TCPで送信
 * @param  s   ソケット記述子
 * @param  buf 送信データへのポインタ
 * @param len 送信データサイズ
 * @return     送信したデータサイズ
 */
ssize_t send_tcp(int s, void *buf, size_t len);
