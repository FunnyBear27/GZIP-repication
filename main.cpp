#include <cstddef>
#include <functional>
#include <unordered_map>
#include <fstream>
#include <iostream>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <vector>
#include <math.h> 


#define MN1 std::byte{ 31 }
#define MN2 std::byte{ 139 }

struct TwoBytes {
	std::byte byte_zero;
	std::byte byte_one;

	std::byte& operator[](bool pos) {
		if (!pos) {
			return byte_zero;
		}

		else {
			return byte_one;
		}
	}

	bool operator==(const TwoBytes &other) const {
		return ( this->byte_zero == other.byte_zero ) & ( this->byte_one == other.byte_one );
	}
};

template<> // How does it work?
struct std::hash<TwoBytes> {
	std::size_t operator() ( const TwoBytes& o) const {
		std::size_t h1 = std::hash<std::byte>{}(o.byte_one);
		std::size_t h2 = std::hash<std::byte>{}(o.byte_one);
		return h1 ^ (h2 << 1);
	}
};


//struct Tree { 
	//TwoBytes value;
	//Tree *left_node, *right_node;
	//std::vector<TwoBytes> code_lengths_vector;

	//Tree (std::vector<TwoBytes> lengths_vector) {
		
	//}
//};



class Decompressor {

	std::vector<std::byte> current_chunk, chunk_to_write;
	std::ifstream stream_read;
	std::ofstream stream_write;
	bool curr_is_last { 0 };
	std::byte first_leftover { std::byte { 0 } }, second_leftover { std::byte { 0 } };
	short first_leftover_size{ 0 }, second_leftover_size { 0 };
	std::unordered_map<TwoBytes, TwoBytes> first_type_llc;

	void (Decompressor::*btype_function)();

	enum BlockType {
		Uncompressed_bt0,
		Static_Huffman_bt1,
		Dynamic_Huffman_bt2
	};
	
	BlockType current_blt;

// ----------------- private methods ----------------

	void readInputStream(std::vector<std::byte>& vect_to_read, const unsigned long chunk_size) {
		
		vect_to_read.clear();

		unsigned long i = 0;
	
		while (i < chunk_size) {
			if (!stream_read) {
				throw std::invalid_argument("Chunk size requested exceeds the length of the file");
			}

			vect_to_read.push_back((std::byte) stream_read.get());
			i++;
			if (i == chunk_size){
				break;
			}
		}
	}
	

	void writeOutputStream(std::vector<std::byte>& data) {
		for (std::byte elem : data){
			stream_write.put((char) elem);
		}
	}


	void generate_static_huffman() { // RFC 1951 DEFLATE specs - 3.2.6
		TwoBytes value;
		value[0] = std::byte { 0 };
		value[1] = std::byte{ 0 };

		TwoBytes current_code;
		current_code[0] = std::byte { 0 };
		current_code[1] = std::byte { 0x30 };

		TwoBytes max_val;
		max_val[0] = std::byte { 0 };
		max_val[1] = std::byte { 0x8f + 1 } ;

		while (value[1] != max_val[1]) {
			first_type_llc[current_code] = value;

			add_one(value);
			add_one(current_code);

		}
		value[1] = std::byte { 0x90 } ;
		max_val[1] = std::byte { 0x97 + 1 };
	}


	void proofMagicNumbers() { 
		if (current_chunk[0] != MN1 or current_chunk[1] != MN2 or current_chunk[2] != std::byte { 0x08 }) {
			throw std::invalid_argument("Magic numbers do not equal those specified in the gzip standard");
		}
	}


	void decodeZeroType() {
		first_leftover_size = 0;
		first_leftover = std::byte{ 0 };
		int datalen { 0 };

		readInputStream(current_chunk, 4);
		for (short i = 0; i < 2; i++) {
			datalen += std::to_integer<int>(current_chunk[i]) * pow(256, i);
		}
		readInputStream(chunk_to_write, datalen);


	}

	void decodeFirstType() {	
	}

	void decodeSecondType() {
	}


	public:

// ----------------- public methods ----------------

		Decompressor(std::string& read_file_name, std::string& write_file_name){
			


			current_chunk.reserve(500);
			stream_read.open(read_file_name, std::ios::binary);
			stream_write.open(write_file_name, std::ios::binary | std::ios::trunc);
			generate_static_huffman();

			if (!stream_read.is_open()) {
				throw std::invalid_argument("Check the filename");
			}

			try {
				readInputStream(current_chunk, 10);
				current_chunk.shrink_to_fit();

				proofMagicNumbers();
			}

			catch (std::invalid_argument& e){
				std::cerr << e.what() << std::endl;
			}
		}


		void readBlocktype() {	
			readInputStream(current_chunk, 1);
			std::byte chunk { current_chunk[0] };
			
			curr_is_last = (bool) (chunk & std::byte(0x01));
			chunk >>= 1;

			std::byte btype = chunk & std::byte(0x03);

			switch (btype) {
				case std::byte { 0x00 }:
					current_blt = Uncompressed_bt0;
					btype_function = &Decompressor::decodeZeroType;
					break;
				case std::byte { 0x01 }:
					current_blt = Static_Huffman_bt1;
					btype_function = &Decompressor::decodeFirstType;
					break;
				case std::byte { 0x02 }:
					current_blt = Dynamic_Huffman_bt2;
					btype_function = &Decompressor::decodeSecondType;
					break;
			}

			first_leftover_size = 5;
			chunk >>= 2;
			first_leftover = chunk;
		}


		bool decodeBlock() {
			(this->*btype_function)();

			writeOutputStream(chunk_to_write);

			if (curr_is_last) {
				return 1;
			}

			return 0;
		}


		~Decompressor() {
			stream_read.close();
			stream_write.close();
		}


	void add_one(TwoBytes &value, bool byte_number = 1) {
		TwoBytes carry;
		carry[byte_number] = std::byte { 0x01 };
		TwoBytes swap;

		while (carry[byte_number] != std::byte { 0 }) {
			swap[byte_number] = value[byte_number];
			value[byte_number] ^= carry[byte_number];	
			carry[byte_number] &= swap[byte_number];
			if ( carry[byte_number] != std::byte { 0x80 }){
				carry[1] <<= 1;
			}
			else {
				break;
			}
		}

		if (carry[byte_number] == std::byte {0x80}) {
			if (byte_number) {
				add_one(value, 0);
			}

			else {
				throw std::runtime_error ("Code length is invalid");
			}
		}
	}
};

// ---------------- main ----------------- 

int main(int argc, char** argv) {
	if (argc < 3) std::cerr << "Specify a file to decode and a file to write output to" << std::endl;
	else if (argc > 3) { std::cerr << "Too many arguments were passed" << std::endl; return 1; }

	std::string read_file_name(argv[1], strlen(argv[1]));
	std::string write_file_name(argv[2], strlen(argv[2]));
	std::unique_ptr<Decompressor> decompressor;
	
	try {
		decompressor = std::make_unique<Decompressor>(read_file_name, write_file_name);	
	}

	catch (std::invalid_argument& e){
		std::cerr << e.what() << std::endl;
		return 1;
	}
	
	decompressor->readBlocktype();
	decompressor->decodeBlock();

	return 0;
}
