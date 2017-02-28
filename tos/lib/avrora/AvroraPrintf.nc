
interface AvroraPrintf
{
	command void print_char(char c);
	command void print_int8(int8_t c);
	command void print_int16(int16_t i);
	command void print_int32(int32_t i);
	command void print_str(const char * const s);
	command void print_hex_buf(const uint8_t *b, uint16_t len);
	command void print_hex8(uint8_t c);
	command void print_hex16(uint16_t i);
	command void print_hex32(uint32_t i);
}
