#include <fstream>
#include <iostream>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <vector>


class Decompressor {
	
	std::vector<unsigned char> current_chunk;
	std::ifstream read_stream;
	std::ofstream write_stream;

	void input_stream_read(std::vector<unsigned char>& data, std::ifstream& my_file, const unsigned long chunk_size) {
		unsigned long i = 0;
	
		while (i < chunk_size) {
			if (!my_file) {
				throw std::invalid_argument("Chunk size requested exceeds the length of the file");
			}

			data.push_back(my_file.get());
			i++;
			if (i == chunk_size){
				break;
			}
		}
	}
	
	
	void output_stream_write(std::vector<unsigned char>& data, std::ofstream& my_file) {
		for (short elem : data) {
			my_file.put(elem);
		}
	}

	public:
		Decompressor(std::string& read_file_name, std::string& write_file_name){
			current_chunk.reserve(500);
			read_stream.open(read_file_name, std::ios::binary);
			write_stream.open(write_file_name, std::ios::binary | std::ios::trunc);

			if (!read_stream.is_open()) {
				throw std::invalid_argument("Check the filename");
			}

			try {
				input_stream_read(current_chunk, read_stream, 2);
				current_chunk.shrink_to_fit();
			}

			catch (std::invalid_argument& e){
				std::cerr << e.what() << std::endl;
			}

			output_stream_write(current_chunk, write_stream);
		}

		~Decompressor() {
			read_stream.close();
			write_stream.close();
		}
};

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

	return 0;
}
