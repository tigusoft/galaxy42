
#include "strings_utils.hpp"

#include "libs1.hpp"

// ==================================================================

string_as_hex::string_as_hex(const std::string & s) : data(s) { }

unsigned char hexchar2int(char c) {
	if ((c>='0')&&(c<='9')) return c-'0';
	if ((c>='a')&&(c<='f')) return c-'a' +10;
	throw std::invalid_argument(  string("Invalid character (")+string(1,c)+string(") in parsing hex number")  );
}

unsigned char doublehexchar2int(string s) {
	if (s.size()!=2) throw std::invalid_argument("Invalid double-hex string: '"+s+"'");
	unsigned char h = s.at(0);
	unsigned char l = s.at(1);
	return hexchar2int(h)*16 + hexchar2int(l);
}

// ==================================================================

string_as_bin::string_as_bin(std::string bin)
	: bytes(bin)
{ }

string_as_bin::string_as_bin(const char * ptr, size_t size)
 : bytes(ptr,size)
{ }

string_as_bin::string_as_bin(const string_as_hex & encoded) {
try {
	// "ff020a" = ff , 02 , 0a
	//   "020a" = 02 , 0a
	//    "20a" = 02 , 0a
	_info("Processing string: ["<< (encoded.data) << "]");
	const auto es = encoded.data.size();
	if (!es) return; // empty string

	size_t retsize = es/2; // size of finall string of bytes data
	if (0 != (es % 2)) retsize++;
	assert(retsize > 0);
	assert( (retsize < es) || (retsize==es==1) ); // both are ==1 for e.g. "a"
	bytes.resize(retsize);

	size_t pos=0, out=0; // position of input, and output
	for( ; pos<es ; pos+=2, ++out) {
		// _info("pos="<<pos<<" out="<<out<<" encoded="<<encoded.data);
		// "02" -> cl="2" ch="0"
		//  "2" -> cl="2" ch="0"
		char cl,ch;
		if (pos+1 < es) { // pos and pos+1 are valid positions in string
			ch = encoded.data.at(pos);
			cl = encoded.data.at(pos+1);
		} else {
			ch = '0';
			cl = encoded.data.at(pos);
		}
		unsigned char octet = hexchar2int(ch)*16 + hexchar2int(cl);
		bytes.at(out) = octet;
	}

	assert( out == retsize ); // all expected positions of data allocated above in .resize() were written
} catch(std::exception &e) { _erro("Failed to parse string [" << encoded.data <<"]"); throw ; }
}

// ==================================================================

string_as_dbg::string_as_dbg(const string_as_bin & bin)
	: string_as_dbg( bin.bytes.begin() , bin.bytes.end() )
{ }

string_as_dbg::string_as_dbg(const char * data, size_t data_size)
	: string_as_dbg( string_as_bin( std::string(data,data_size) ) )
{ }

void string_as_dbg::print(std::ostream & os, char v)
{
	unsigned char uc = static_cast<unsigned char>(v);
	signed char widthH=-1; // -1 is normal print, otherwise the width of hex
	signed char widthD; // width of dec
	if (uc<32) { widthH=2; widthD=2; }
	if (uc>127) { widthH=2; widthD=3; }
	if (widthH!=-1) { // escape it
		os << "0x" << std::hex << std::setfill('0') << std::setw(widthH) << static_cast<int>(uc)
		   << '(' << std::dec << std::setfill('0') << std::setw(widthD) << static_cast<int>(uc) << ')';
	}
	else os<<v; // normal
}
void string_as_dbg::print(std::ostream & os, signed char v) { print(os, static_cast<char>(v)); }
void string_as_dbg::print(std::ostream & os, unsigned char v) { print(os, static_cast<char>(v)); }


string_as_dbg::operator const std::string & () const {
	return this->dbg;
}
const std::string & string_as_dbg::get() const {
	return this->dbg;
}

