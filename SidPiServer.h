void handleWritePacket(int dataLength);
void handleDelayPacket(int sidNumber, int cycles);
void invalidCommandException(void *errMsg);
void processReadBuffer(int len);
void *sid_thread(void *ptr);
void signal_callback_handler(int signum);
void processReadBuffer(int len);
