/*
 *   Driver for DS Linux
 *
 *       Copyright 2006 Bret Thaeler - bthaeler@aol.com
 *
 */

// Wifi regs
#define WIFI_REG(ofs)   (*(volatile u16*)(0x04800000+(ofs)))
#define WIFI_WEPKEY0    ( (volatile u16*)0x04805F80)
#define WIFI_WEPKEY1    ( (volatile u16*)0x04805FA0)
#define WIFI_WEPKEY2    ( (volatile u16*)0x04805FC0)
#define WIFI_WEPKEY3    ( (volatile u16*)0x04805FE0)

#define WIFI_MODE_RST   (*(volatile u16*)0x04800004)
#define WIFI_MODE_WEP   (*(volatile u16*)0x04800006)
#define WIFI_IF         (*(volatile u16*)0x04800010)
#define WIFI_IE         (*(volatile u16*)0x04800012)
#define WIFI_MACADDR    ( (volatile u16*)0x04800018)
#define WIFI_BSSID      ( (volatile u16*)0x04800020)
#define WIFI_AIDS       (*(volatile u16*)0x04800028)
#define WIFI_RETRLIMIT  (*(volatile u16*)0x0480002C)
#define WIFI_POWERSTATE (*(volatile u16*)0x0480003C)
#define WIFI_RANDOM     (*(volatile u16*)0x04800044)

#define WIFI_BBSIOCNT   (*(volatile u16*)0x04800158)
#define WIFI_BBSIOWRITE (*(volatile u16*)0x0480015A)
#define WIFI_BBSIOREAD  (*(volatile u16*)0x0480015C)
#define WIFI_BBSIOBUSY  (*(volatile u16*)0x0480015E)
#define WIFI_RFSIODATA2 (*(volatile u16*)0x0480017C)
#define WIFI_RFSIODATA1 (*(volatile u16*)0x0480017E)
#define WIFI_RFSIOBUSY  (*(volatile u16*)0x04800180)

enum WEPMODES {
	WEPMODE_NONE = 0,
	WEPMODE_40BIT = 1,
	WEPMODE_128BIT = 2
};

enum WIFI_STATE {
	WIFI_STATE_UP = 0x0001,
	WIFI_STATE_ASSOCIATED = 0x0002,
	WIFI_STATE_ASSOCIATING = 0x0004,
	WIFI_STATE_CANNOTASSOCIATE = 0x0008,
	WIFI_STATE_AUTHENTICATED = 0x0010,
	WIFI_STATE_SAW_TX_ERR = 0x0020,
	WIFI_STATE_CHANNEL_SCANNING = 0x0040,

	WIFI_STATE_TXPENDING = 0x0100,
	WIFI_STATE_RXPENDING = 0x0200,
	WIFI_STATE_APQUERYPEND = 0x0400,
};

/*
 * Memory layout
 *
 * 0x4000 - 0x5FFF (8192 bytes)
 *
 * Recieve buffer 0x4C28 -- 0x5F5F  (4920 bytes)
 *
 * Our MTU 1556 bytes
 *    Tx header:      12 bytes
 *    802.11 headers: 24 bytes
 *    Wep keys info:   4 bytes
 *    LLC header:      8 bytes
 *    1500 data:    1500 bytes
 *    WEP IVC:         4 bytes
 *    CRC:             4 bytes
 * 
 * Transmit buffers 0x4000 - 0x4C27 (3112 bytes)
 * Each transmit buffer, 1556
 *
 * Transmit buffer1 0x4000 - 0x4613
 * Transmit buffer2 0x4614 - 0x4C27
 */
#define RX_START_SETUP 0x4C28
#define RX_END_SETUP   0x5F60
#define RX_CSR_SETUP   ((RX_START_SETUP - 0x4000) >> 1)
#define TX_MTU_BYTES   1556
#define TX1_MAC_START  0x4000
#define TX1_SETUP_VAL  (0x8000 | ((TX1_MAC_START - 0x4000) >> 1))
#define TX2_MAC_START  0x4614
#define TX2_SETUP_VAL  (0x8000 | ((TX2_MAC_START - 0x4000) >> 1))

#define MAX_KEY_SIZE 13		// 128 (?) bits


#define WFLAG_PACKET_DATA		0x0001
#define WFLAG_PACKET_MGT		0x0002
#define WFLAG_PACKET_BEACON		0x0004
#define WFLAG_PACKET_CTRL		0x0008

#define WFLAG_PACKET_ALL		0xFFFF

#define WFLAG_APDATA_ADHOC			0x0001
#define WFLAG_APDATA_WEP			0x0002
#define WFLAG_APDATA_WPA			0x0004
#define WFLAG_APDATA_COMPATIBLE		0x0008
#define WFLAG_APDATA_EXTCOMPATIBLE	0x0010
#define WFLAG_APDATA_SHORTPREAMBLE	0x0020
#define WFLAG_APDATA_ACTIVE			0x8000


#define WIFI_CHANNEL_SCAN_DWEL 150 // 150 micro seconds

typedef struct WIFI_TXHEADER  Wifi_TxHeader;
struct WIFI_TXHEADER {
	u16 enable_flags;
	u16 unknown;
	u16 countup;
	u16 beaconfreq;
	u16 tx_rate;
	u16 tx_length;
};

typedef struct WIFI_RXHEADER Wifi_RxHeader;
struct WIFI_RXHEADER {
	u16 a;
	u16 b;
	u16 c;
	u16 d;
	u16 byteLength;
	u16 rssi_;
};

typedef struct WIFI_ACCESSPOINT Wifi_AccessPoint;
struct WIFI_ACCESSPOINT {
	char ssid[33];		// 0-32byte data, zero
	char ssid_len;
	u8 bssid[6];
	u8 macaddr[6];
	u16 maxrate;		// max rate is measured in steps of 1/2Mbit - 5.5Mbit will be represented as 11, or 0x0B
	u32 timectr;
	u16 rssi;
	u16 flags;
/*
	u32 spinlock;
*/
	u8 channel;
	u8 rssi_past[8];
	u8 base_rates[16];	// terminated by a 0 entry
};

typedef struct Wifi_Data_Struct Wifi_Data;
struct Wifi_Data_Struct {

	u16 curChannel, reqChannel;
	u16 curMode, reqMode;
	u16 authlevel, authctr;
	u8 curWepmode, reqWepMode;
	char ssid[34];
	u8 baserates[16];

	u16 scanChannel;

	u16 state;

	u8 MacAddr[6];
	u8 bssid[6];
	u8 apmac[6];
	u8 wepkey[4][MAX_KEY_SIZE + 1];
	u16 maxrate;
	u16 wepkeyid;

	u8 FlashData[512];

	/* pointers to buffers we get handed from ARM9 */
	volatile u32 *stats;
	Wifi_AccessPoint *aplist;

};

enum WIFI_STATS
{
	WIFI_STATS_RXPACKETS = 0,
	WIFI_STATS_RXBYTES,
	WIFI_STATS_RXDATABYTES,

	WIFI_STATS_TXPACKETS,
	WIFI_STATS_TXBYTES,
	WIFI_STATS_TXDATABYTES,
	WIFI_STATS_TXQREJECT,

	WIFI_STATS_TXRAWPACKETS,
	WIFI_STATS_RXRAWPACKETS,
	WIFI_STATS_RXOVERRUN,

	WIFI_STATS_DEBUG1,
	WIFI_STATS_DEBUG2,
	WIFI_STATS_DEBUG3,
	WIFI_STATS_DEBUG4,
	WIFI_STATS_DEBUG5,
	WIFI_STATS_DEBUG6,

	WIFI_STATS_HW_1B0,
	WIFI_STATS_HW_1B4,
	WIFI_STATS_HW_1B8,
	WIFI_STATS_HW_1BC,
	WIFI_STATS_HW_1C0,
	WIFI_STATS_HW_1D0,
	WIFI_STATS_HW_1D4,
	WIFI_STATS_HW_1D8,
	WIFI_STATS_HW_1DC,

	WIFI_STATS_MAX
};

enum WIFI_AP_MODE
{
	WIFI_AP_INFRA,
	WIFI_AP_ADHOC,
};

/* Ethernet MTU is 1500 */
#define NDS_WIFI_MAX_PACKET_SIZE 1600

#define WIFI_ARM7_TIMEOUT	100
#define WIFI_MAX_AP		32

/* Used for transmission to arm7 */
typedef struct nds_tx_packet nds_tx_packet;
struct nds_tx_packet {
	u16 len;	
	uchar *data;
	void *skb;
};

typedef struct nds_rx_packet nds_rx_packet;
struct nds_rx_packet {
	u16 len;	
	uchar data[NDS_WIFI_MAX_PACKET_SIZE];
};

extern Wifi_Data wifi_data;
extern nds_rx_packet *rx_packet;

void wifi_init(void);
void wifi_open(void);
void wifi_close(void);
void wifi_mac_query(void);
void wifi_interrupt(void);
void wifi_send_ether_packet(u16 length, uchar * data);
void wifi_stats_query(void);

void Wifi_RequestChannel(int channel);
void Wifi_SetWepKey(int key, int off, u8 b1, u8 b2);
void Wifi_SetWepKeyID(int key);
void Wifi_SetWepMode(int wepmode);
void Wifi_SetSSID(int off, char b1, char b2);
void Wifi_SetAPMode(enum WIFI_AP_MODE mode);
void Wifi_GetAPMode(void);

void wifi_ap_query(u16 start_stop);
void wifi_start_scan(void);
void wifi_timer_handler(void);
void wifi_rx_q_complete(void);