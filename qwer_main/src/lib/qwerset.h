#ifndef QWERSET_H
#define QWERSET_H value

#define X_SO_LINGER true    //!< set true would using socket option linger to avoid TIME_WAIT, where would cause tcp ends with RST instead of 4 hands close.
#define X_HTTP_KEEPALIVE false //!< set true would open http keepalive // TODO implemnt it
#define X_HTTP_KEEPALIVE_DURATION 20    //! set 'ms' for keepalive duration
#define X_TCP_KEEPALIVE false //!< set true would open tcp keepalive, no need to use two keepalive
#endif /* ifndef QWERSET_H */
