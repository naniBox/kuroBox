/*
 This packet is transmitted over RS232 at user-configurable baud rate 
 within 1ms after the arrival of LTC's SYNC_WORD last bit. It is stored
 in memory as a packed little-endian structure.
 */
struct ambient_status_t
{
	/*
	 * 2 byte sequence to identify start of packet: 0x6E, 0x42
	 * */
	uint16_t preamble;

	/*
	 * Instantaneous YAW at time when the LTC SYNC_WORD comes in.
	 * This is a ieee 754 32-bit floating number
	 */
	float yaw;

	/*
	 * Instantaneous PITCH at time when the LTC SYNC_WORD comes in.
	 * This is a ieee 754 32-bit floating number
	 */
	float pitch;

	/*
	 * Instantaneous ROLL at time when the LTC SYNC_WORD comes in.
	 * This is a ieee 754 32-bit floating number
	 */
	float roll;

	/*
	 * Packet Id sent since powerup.
	 */
	uint32_t packet_id;

	/*
	 * 16-bit checksum calculated based on entire packet except the checksum
	 * field, that is, the first 18bytes of the packet. It's calculated based
	 * on the fletcher algorithm (from uBlox datasheet):
	 * 		http://www.ietf.org/rfc/rfc1145.txt
	 *		http://en.wikipedia.org/wiki/Fletcher's_checksum
	 * Pseudocode:

		CK_A = 0, CK_B = 0
		For(I=0;I<N;I++)
		{
			CK_A = CK_A + Buffer[I]
			CK_B = CK_B + CK_A
		}
		RES = ( CK_B << 8 ) | CK_A

	 */
	uint16_t checksum;
};