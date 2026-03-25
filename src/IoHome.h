#if !defined(_IOHOME_H)
#define _IOHOME_H

#include <RadioLib.h>
#include <vector>     // Required for std::vector
#include <algorithm>  // Required for std::copy

// Define DEBUG_IOHOME to enable debug logs for IoHomeNode
#define DEBUG_IOHOME

#pragma region CONST
// preamble
#define IOHOME_PREAMBLE_LEN                 (512) // Preamble Length in Bits

// sync word
#define IOHOME_SYNC_WORD                    (0x57FD99)
#define IOHOME_SYNC_WORD_LEN                (3)

// frame header
#define IOHOME_CTRLBYTE0_MODE_TWOWAY        (0x00 << 7)        //  7     7
#define IOHOME_CTRLBYTE0_MODE_ONEWAY        (0x01 << 7)        //  7     7
#define IOHOME_CTRLBYTE0_ORDER_0            (0x00 << 5)        //  7     6
#define IOHOME_CTRLBYTE0_ORDER_1            (0x01 << 5)        //  7     6
#define IOHOME_CTRLBYTE0_ORDER_2            (0x02 << 5)        //  7     6
#define IOHOME_CTRLBYTE0_ORDER_3            (0x03 << 5)        //  7     6
#define IOHOME_CTRLBYTE0_LENGTH(IOHOME_LEN) (IOHOME_LEN << 0)  //  5     0

// frame header layout
#define IOHOME_CTRLBYTE0_POS                (0x0)
#define IOHOME_CTRLBYTE1_POS                (0x1)
#define IOHOME_MAC_SOURCE_POS               (0x2)
#define IOHOME_MAC_DEST_POS                 (IOHOME_MAC_SOURCE_POS + IOHOME_FRAME_MAC_LEN) // Corrected position: after source MAC
#define IOHOME_MSG_LEN(MSG)                 ((MSG[IOHOME_CTRLBYTE0_POS] & 0x1F) + 1) // +1 for the control byte itself
#define IOHOME_FRAME_CRC_LEN                (2) // CRC-16 is 2 bytes long
#define IOHOME_FRAME_CRC_POS(FRAME_LEN)     ((FRAME_LEN) - IOHOME_FRAME_CRC_LEN)
#define IOHOME_NUM_COMMANDS                 (2)
#define IOHOME_CMD_0x00                     (0x00)
#define IOHOME_CMD_0x01                     (0x01)

// Frame building constants
#define IOHOME_FRAME_HEADER_CORE_LEN        (1 + 1) // CTRL0 + CTRL1
#define IOHOME_FRAME_MAC_LEN                (3)
#define IOHOME_FRAME_HEADER_LEN             (IOHOME_FRAME_HEADER_CORE_LEN + 2 * IOHOME_FRAME_MAC_LEN) // CTRL0, CTRL1, SRC_MAC (3), DEST_MAC (3) = 8 bytes
#define IOHOME_COMMAND_ID_LEN               (1)
// Layer 3 constants
#define IOHOME_SECURITY_COUNTER_LEN         (2)
#define IOHOME_SECURITY_MAC_LEN             (6)
#define IOHOME_SECURITY_FOOTER_LEN          (IOHOME_SECURITY_COUNTER_LEN + IOHOME_SECURITY_MAC_LEN) // 8 bytes

#pragma endregion CONST

/*!
  \struct IoHomeCommands_t
  \brief Command specification structure.
*/
struct IoHomeCommands_t {
  /*! \brief Command ID */
  uint8_t cid;

  /*! \brief Command Length */
  uint8_t len;

  /*! \brief Number of Parameters */
  uint8_t pnum;

  /*! \brief Whether this command needs authentication */
  bool auth;
};
const IoHomeCommands_t CMD_TABLE[IOHOME_NUM_COMMANDS] = {
  { IOHOME_CMD_0x00, 10, 1, true },
  { IOHOME_CMD_0x01, 10, 1, true },
  // ...
};

struct NodeId {
  uint8_t n0;
  uint8_t n1;
  uint8_t n2;
};

struct IoHomeChannel_t {
  uint16_t c0;
  uint8_t c1;
};

/*!
  \struct IoHomeFrame_t
  \brief Structure to hold parsed io-homecontrol frame data.
*/
struct IoHomeFrame_t {
  uint8_t ctrlByte0;
  uint8_t ctrlByte1;
  NodeId sourceMac;
  NodeId destMac;
  uint8_t commandId;
  std::vector<uint8_t> payload;
  bool isValid = false; // Indicates if the frame was successfully parsed and CRC validated
};

/*!
  \class IoHomeNode
  \brief io-homecontrol node.
*/
class IoHomeNode {
  public:
    IoHomeNode(PhysicalLayer* phy, const IoHomeChannel_t* channel);

    void begin(const IoHomeChannel_t* channel,
               NodeId source_node_id,
               NodeId destination_node_id,
               uint8_t* stack_key,
               uint8_t* system_key);

    // Returns true if a valid frame was captured and parsed
    bool loop(IoHomeFrame_t& rxFrame);

    int16_t setPhyProperties();
    int16_t transmitFrame(const std::vector<uint8_t>& frame);
    int16_t receiveFrame(IoHomeFrame_t& receivedFrame);

    static uint16_t crc16(const uint8_t* data, size_t length);
    static bool validateFrameCrc(const uint8_t* frame, size_t frameLength);

    std::vector<uint8_t> buildFrame(
      uint8_t ctrlByte0, uint8_t ctrlByte1,
      NodeId sourceMac, NodeId destMac,
      uint8_t commandId, const std::vector<uint8_t>& payload
    );

    bool parseFrame(const uint8_t* frame, size_t frameLength, IoHomeFrame_t& parsedFrame);

    // Commands
    int16_t sendWink(NodeId targetMac);

    // Template helpers (Must be in header)
    template<typename T>
    static T ntoh(uint8_t* buff, size_t size = 0) {
      uint8_t* buffPtr = buff;
      size_t targetSize = (size == 0) ? sizeof(T) : size;
      T res = 0;
      for(size_t i = 0; i < targetSize; i++) {
        res |= (static_cast<T>(*(buffPtr++)) << (8 * i));
      }
      return res;
    }

    template<typename T>
    static void hton(uint8_t* buff, T val, size_t size = 0) {
      uint8_t* buffPtr = buff;
      size_t targetSize = (size == 0) ? sizeof(T) : size;
      for(size_t i = 0; i < targetSize; i++) {
        *(buffPtr++) = static_cast<uint8_t>((val >> (8 * i)) & 0xFF);
      }
    }

  private:
    // Encapsulated members (The "Guts" of the node)
    PhysicalLayer* _phyLayer;
    const IoHomeChannel_t* _channel;

    NodeId _source_node_id;
    NodeId _destination_node_id;

    uint8_t _stack_key[16];
    uint8_t _system_key[16];

    // Counter for the Security Layer (u2 counter in .ksy)
    uint16_t _sequence_counter = 0;
}; // <--- This closes the class

#endif // <--- This closes the header guard
