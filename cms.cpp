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

int main(int argc, char *argv[])
{

//--------------------------------------------------------------------------------
// Create a context and queue
//--------------------------------------------------------------------------------
    try
    {

        cl_uint deviceIndex = 0;
        parseArguments(argc, argv, &deviceIndex);

        // Get list of devices
        std::vector<cl::Device> devices;
        unsigned numDevices = getDeviceList(devices);

        // Check device index in range
        if (deviceIndex >= numDevices)
        {
          std::cout << "Invalid device index (try '--list')\n";
          return EXIT_FAILURE;
        }

        cl::Device device = devices[deviceIndex];

        std::string name;
        getDeviceName(device, name);
        std::cout << "\nUsing OpenCL device: " << name << "\n";

        std::vector<cl::Device> chosen_device;
        chosen_device.push_back(device);
        cl::Context context(chosen_device);
        cl::CommandQueue queue(context, device);

        // Setup global shared buffers.
        std::vector<uint32_t> h_counts(W*D, 0);
        //std::vector<uint32_t> h_params = {
        //    3005958, 995248, 636928, 2567793, 909030, 4151203, 1128352, 3152829,
        //        2068697, 2587417, 2052571, 3764501, 1619340, 2920635, 868570, 3079234
        //};
        std::vector<uint32_t> h_params = {
            1, 0, 636928, 2567793, 909030, 4151203, 1128352, 3152829,
                2068697, 2587417, 2052571, 3764501, 1619340, 2920635, 868570, 3079234
        };
        cl::Buffer d_counts(context, h_counts.begin(), h_counts.end(), true);
        cl::Buffer d_params(context, h_params.begin(), h_params.end(), true);

        // Create the compute program from the source buffer
        cl::Program program(context, util::loadProgram("cms.cl"), true);

        // Create the compute kernel from the program
        cl::make_kernel<cl::Buffer, cl::Buffer, cl::Buffer> cms(program, "cms");

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
                cl::NDRange range(buf->size());

                // Put it into cl buffer and send to gpu.
                cl::Buffer d_hashes(context, buf->begin(), buf->end(), true);

                cms(cl::EnqueueArgs(queue, range), d_params, d_hashes, d_counts);
                delete buf;
                queue.finish();
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
                if (line_count % 1000 == 0) {
                    std::printf("%d lines...\n", line_count);
                }
                int year = std::stoi(line.substr(0, 4));
                int month = std::stoi(line.substr(5, 2));
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
                    unsigned int hash = string_hash(word) + year*2 + month;
                    h_hashes->push_back(hash);
                    if (h_hashes->size() == BUF_SIZE) {
                        push_queue(h_hashes);
                        h_hashes = new std::vector<uint32_t>();
                        h_hashes->reserve(BUF_SIZE);
                    }
                }
            }
        }

        kernel_dispatch.join();
        cl::copy(queue, d_counts, h_counts.begin(), h_counts.end());
        for (int i = 0; i < 8; i++) {
            //std::string filename = "outcountsA";
            //filename[filename.size() - 1] += i;
            //std::ofstream out_counts(filename);
            //for (int k = 0; k < W; k++) {
            //    out_counts << h_counts[k + i*W] << ' ';
            //}
        }

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
