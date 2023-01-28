#include <cstddef>
#include <fstream>
#include <iostream>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <vector>

#define MN1 std::byte{ 31 }
#define MN2 std::byte{ 139 }


class Decompressor {

	std::vector<std::byte> current_chunk;
	std::ifstream stream_read;
	std::ofstream stream_write;
	bool curr_is_last { 0 };
	std::byte first_leftover, second_leftover;
	short first_leftover_size, second_leftover_size;

	enum BlockType {
		Uncompressed_bt0,
		Static_Huffman_bt1,
		Dynamic_Huffman_bt2
	};
	
	BlockType current_blt;

// ----------------- private methods ----------------

	void readInputStream(const unsigned long chunk_size) {
		unsigned long i = 0;
	
		while (i < chunk_size) {
			if (!stream_read) {
				throw std::invalid_argument("Chunk size requested exceeds the length of the file");
			}

			current_chunk.push_back((std::byte) stream_read.get());
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


	void proofMagicNumbers() { 
		if (current_chunk[0] != MN1 or current_chunk[1] != MN2 or current_chunk[2] != std::byte { 0x08 }) {
			throw std::invalid_argument("Magic numbers do not equal those specified in the gzip standard");
		}
	}


	public:

// ----------------- public methods ----------------

		Decompressor(std::string& read_file_name, std::string& write_file_name){
			current_chunk.reserve(500);
			stream_read.open(read_file_name, std::ios::binary);
			stream_write.open(write_file_name, std::ios::binary | std::ios::trunc);

			if (!stream_read.is_open()) {
				throw std::invalid_argument("Check the filename");
			}

			try {
				readInputStream(10);
				current_chunk.shrink_to_fit();

				proofMagicNumbers();
			}

			catch (std::invalid_argument& e){
				std::cerr << e.what() << std::endl;
			}
		}

		void readBlocktype() {	
			readInputStream(1);
			std::byte chunk { current_chunk[0] };
			
			curr_is_last = (bool) (chunk & std::byte(0x01));
			chunk >>= 1;
			first_leftover_size = 7;

			short btype = std::to_integer<short>(chunk & std::byte(0x03));

			switch (btype) {
				case 0:
					current_blt = BlockType::Uncompressed_bt0;
				case 1:
					current_blt = BlockType::Static_Huffman_bt1;
				case 2:
					current_blt = BlockType::Dynamic_Huffman_bt2;
			}
		}


		~Decompressor() {
			stream_read.close();
			stream_write.close();
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

	return 0;
}
