#if !defined(_IOHOME_H)
#define _IOHOME_H

#include <RadioLib.h>
#include <vector>     // Required for std::vector
#include <algorithm>  // Required for std::copy

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
#define IOHOME_MAC_DEST_POS                 (0x3)
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
  uint8_t c0;
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
  bool isValid; // Indicates if the frame was successfully parsed and CRC validated
};

/*!
  \class IoHomeNode
  \brief io-homecontrol node.
*/
class IoHomeNode {
  public:

    /*!
      \brief Default constructor.
      \param phy Pointer to the PhysicalLayer radio module.
      \param channel Pointer to the io-homecontrol channel to use.
    */
    IoHomeNode(PhysicalLayer* phy, const IoHomeChannel_t* channel);

    /*!
      \brief $description
      \param channel $description
      \param sourceNodeID $description
      \param destinationNodeId $description
      \param stackKey $description
      \param systemKey $description
      \returns \ref status_codes
    */
    void begin(const IoHomeChannel_t* channel, NodeId source_node_id, NodeId destination_node_id, uint8_t* stack_key, uint8_t* system_key);

    PhysicalLayer* phyLayer = NULL;
    const IoHomeChannel_t* channel; // Const member pointer, must be initialized in constructor

    // configure common physical layer properties (preamble, sync word etc.)
    int16_t setPhyProperties();

    // crc16-kermit that takes a uint8_t array of even length and calculates the checksum
    static uint16_t crc16(const uint8_t* data, size_t length);

    // network-to-host conversion method - takes data from network packet and converts it to the host endians
    template<typename T>
    static T ntoh(uint8_t* buff, size_t size = 0) {
      uint8_t* buffPtr = buff;
      size_t targetSize = sizeof(T);
      if(size != 0) {targetSize = size;}
      T res = 0;
      for(size_t i = 0; i < targetSize; i++) {res |= (uint32_t)(*(buffPtr++)) << 8*i;}
      return(res);
    }

    // host-to-network conversion method - takes data from host variable and and converts it to network packet endians
    template<typename T>
    static void hton(uint8_t* buff, T val, size_t size = 0) {
      uint8_t* buffPtr = buff;
      size_t targetSize = sizeof(T);
      if(size != 0) {targetSize = size;}
      for(size_t i = 0; i < targetSize; i++) {*(buffPtr++) = val >> 8*i;}
    }

    /*!
      \brief Validates the CRC-16 of an io-homecontrol frame.
      \param frame Pointer to the complete io-homecontrol frame (including CRC).
      \param frameLength The total length of the frame in bytes.
      \returns True if the calculated CRC matches the CRC in the frame, false otherwise.
    */
    static bool validateFrameCrc(const uint8_t* frame, size_t frameLength) {
      if (frameLength < IOHOME_FRAME_CRC_LEN) {
        return false; // Frame too short to even contain a CRC
      }

      // Calculate CRC over the data portion (excluding the 2 CRC bytes at the end)
      uint16_t calculatedCrc = IoHomeNode::crc16(frame, IOHOME_FRAME_CRC_POS(frameLength));

      // Extract the received CRC from the end of the frame
      uint16_t receivedCrc = IoHomeNode::ntoh<uint16_t>((uint8_t*)frame + IOHOME_FRAME_CRC_POS(frameLength));

      return calculatedCrc == receivedCrc;
    }

    /*!
      \brief Builds a complete io-homecontrol frame including CRC-16.
      \param ctrlByte0 The first control byte. The length field in this byte will be overwritten by the actual message body length.
      \param ctrlByte1 The second control byte.
      \param sourceMac The source node ID.
      \param destMac The destination node ID.
      \param commandId The command ID.
      \param payload The message payload.
      \returns A std::vector<uint8_t> containing the complete frame.
    */
    static std::vector<uint8_t> buildFrame(
      uint8_t ctrlByte0,
      uint8_t ctrlByte1,
      NodeId sourceMac,
      NodeId destMac,
      uint8_t commandId,
      const std::vector<uint8_t>& payload
    );

    /*!
      \brief Parses an io-homecontrol frame and validates its CRC.
      \param frame Pointer to the raw io-homecontrol frame data.
      \param frameLength The total length of the raw frame data.
      \param parsedFrame Reference to an IoHomeFrame_t struct to populate with parsed data.
      \returns True if the frame was successfully parsed and its CRC is valid, false otherwise.
    */
    static bool parseFrame(const uint8_t* frame, size_t frameLength, IoHomeFrame_t& parsedFrame);
};

#endif
