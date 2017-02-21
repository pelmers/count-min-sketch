// Source template borrowed from https://github.com/HandsOnOpenCL/Exercises-Solutions
// under Creative Commons Attribution 3.0 Unported License.

#define __CL_ENABLE_EXCEPTIONS
#include "cl.hpp"
#include "util.hpp"
#include "err_code.h"
#include "device_picker.hpp"
#include <cstdio>
#include <sstream>
#include <thread>
#include <queue>

#define W (1 << 22)
#define D (8)
#define M (4194303)
#define BUF_SIZE 8000
#define NUM_BUFFERS 12

unsigned int string_hash(const std::string& s) {
	unsigned long hash = 5381;
	int c;
	int i = 0;
	while ((c = s[i++]))
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

	return hash;
}

void update_CMS(std::vector<uint32_t> *hashes, std::vector<uint32_t> *counts, std::vector<uint32_t> *params) {
	int i, k;
	for (i = 0; i < hashes->size(); i++) {
		for (k = 0; k < 8; k++) {
			unsigned int a = params->at(k * 2);
			unsigned int b = params->at(k * 2 + 1);
			unsigned int idx = (a * hashes->at(i) + b) % M;
			counts->at(idx + (k << 22))++;
		}
	}
}

int main(int argc, char *argv[])
{

//--------------------------------------------------------------------------------
// Create a context and queue
//--------------------------------------------------------------------------------
	try
	{

		if (argc < 2) {
			std::cerr << "First argument must be input filename." << std::endl;
			return 1;
		}

		std::string input_filename(argv[1]);
		std::ifstream data(input_filename);
		std::string line;

		std::queue<std::vector<uint32_t>*> buf_queue;
		std::condition_variable buf_queue_full_cv;
		std::condition_variable buf_queue_empty_cv;

		std::mutex m;
		std::atomic_bool finished(false);
		int line_count = 0;

		/*
		int min_year = 9999;
		int min_month = 0;
		int max_year = 0;
		int max_month = 0;
		 */

		std::vector<uint32_t> counts(W*D, 0);
		std::vector<uint32_t> params = {
				1, 0, 636928, 2567793, 909030, 4151203, 1128352, 3152829,
				2068697, 2587417, 2052571, 3764501, 1619340, 2920635, 868570, 3079234
		};

		auto kernel_dispatch = std::thread([&]() {
			while (!finished || buf_queue.size() > 0) {
				std::vector<uint32_t>* buf;
				std::unique_lock<std::mutex> lk(m);
				buf_queue_empty_cv.wait(lk, [&] {return buf_queue.size() > 0;});

				buf = buf_queue.front();
				buf_queue.pop();
				if (buf_queue.size() == NUM_BUFFERS - 1) {
					lk.unlock();
					buf_queue_full_cv.notify_one();
				} else {
					lk.unlock();
				}
				update_CMS(buf, &counts, &params);
				delete buf;
			}
		});

		auto push_queue = [&](std::vector<uint32_t>* buf) {
			std::unique_lock<std::mutex> lk(m);
			buf_queue_full_cv.wait(lk, [&] {return buf_queue.size() < NUM_BUFFERS;});
			buf_queue.push(buf);
			if (buf_queue.size() == 1) {
				lk.unlock();
				buf_queue_empty_cv.notify_one();
			}
		};

		std::vector<uint32_t> *h_hashes = new std::vector<uint32_t>();
		h_hashes->reserve(BUF_SIZE);
		while (!finished) {
			if (!std::getline(data, line)) {
				if (h_hashes->size() > 0) {
					push_queue(h_hashes);
				}
				finished = true;
				break;
			} else {
				line_count++;
				if (line_count % 50000 == 0) {
					std::printf("%d lines...\n", line_count);
				}
				int year = std::stoi(line.substr(0, 4));
				int month = std::stoi(line.substr(5, 2));

				/*
				if (year < min_year || (year == min_year && month < min_month)) {
					min_year = year;
					min_month = month;
				}
				if (year > max_year || (year == max_year && month > max_month)) {
					max_year = year;
					max_month = month;
				}
				*/

				std::string word;
				std::istringstream stream(line);
				bool first = true;
				while (std::getline(stream, word, ' ')) {
					// First word is the date itself.
					if (first) {
						first = false;
						continue;
					}

					// Multiply year by two to avoid colliding year-month pairs.
					unsigned int hash = string_hash(word) + (year*12) + month;
					h_hashes->push_back(hash);
					if (h_hashes->size() == BUF_SIZE) {
						// push_queue(h_hashes);
						h_hashes = new std::vector<uint32_t>();
						h_hashes->reserve(BUF_SIZE);
					}
				}
			}
		}
		kernel_dispatch.join();

		for (int i = 0; i < 8; i++) {
			std::string filename = "bench_counts_0";
			if (argc >= 3) {
				filename = argv[2];
			}
			filename[filename.size() - 1] += i;
			std::ofstream out_counts(filename);
			for (int k = 0; k < W; k++) {
			    out_counts << counts[k + i*W] << ' ';
			}
		}

		// std::cout << "First edit date: " << min_year << ":" << min_month << std::endl;
		// std::cout << "Last edit date: " << max_year << ":" << max_month << std::endl;

	} catch (cl::Error err)
	{
		std::cout << "Exception\n";
		std::cerr << "ERROR: "
				  << err.what()
				  << "("
				  << err_code(err.err())
				  << ")"
				  << std::endl;
	}

	return EXIT_SUCCESS;
}

