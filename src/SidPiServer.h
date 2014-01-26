#define PORT "6581"  // the port users will be connecting to
#define BACKLOG 10     // how many pending connections queue will hold
#define DATA_READ_SIZE (65536 * 4 + 16384)
#define DATA_WRITE_SIZE 260

#define SID_PI				"SidPi"
#define DEFAULT_LATENCY		500000
#define DEFAULT_DELAY_MULTI	200
#define DEFAULT_THRESHOLD	1

#define	FLUSH				0
#define	TRY_SET_SID_COUNT	1
#define	MUTE				2
#define	TRY_RESET			3
#define	TRY_DELAY			4
#define	TRY_WRITE			5
#define	TRY_READ			6
#define	GET_VERSION			7
#define	TRY_SET_SAMPLING	8
#define	TRY_SET_CLOCKING	9
#define	GET_CONFIG_COUNT	10
#define	GET_CONFIG_INFO		11
#define	SET_SID_POSITION	12
#define	SET_SID_LEVEL		13
#define	TRY_SET_SID_MODEL	14
#define SET_THRESHOLD		15
#define SET_LATENCY			16
#define SET_DELAY_MULTI		17

#define OK					0
#define	BUSY				1
#define	ERR					2
#define	READ				3
#define	VERSION				4
#define	COUNT				5
#define	INFO				6

#define SID_NETWORK_PROTOCOL_VERSION	2

void handleWritePacket(int dataLength);
void handleDelayPacket(int sidNumber, int cycles);
void invalidCommandException(void *errMsg);
void processReadBuffer(int len);
void *sid_thread(void *ptr);
void signal_callback_handler(int signum);
void processReadBuffer(int len);
void sigchld_handler(int s);
void *get_in_addr(struct sockaddr *sa);

