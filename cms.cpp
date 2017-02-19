// Source template borrowed from https://github.com/HandsOnOpenCL/Exercises-Solutions
// under Creative Commons Attribution 3.0 Unported License.

#define __CL_ENABLE_EXCEPTIONS
#include "cl.hpp"
#include "util.hpp"
#include "err_code.h"
#include "device_picker.hpp"
#include <sstream>

#define W (1 << 22)
#define D (8)

int main(int argc, char *argv[])
{

    std::ifstream data("dump");
    std::string line;
    while (true) {
        if (!std::getline(data, line)) {
            break;
        } else {
            int year = std::stoi(line.substr(0, 4));
            int month = std::stoi(line.substr(5, 2));
            std::string word;
            std::cout << year << month;
            std::istringstream stream(line);
            bool first = true;
            while (std::getline(stream, word, ' ')) {
                // First word is the date itself.
                if (first) {
                    first = false;
                    continue;
                }
                std::cout << word;
                // TODO: hash the word and put it away.
            }
        }
    }

    cl::Buffer d_a, d_b, d_c;   // Matrices in device memory

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

//--------------------------------------------------------------------------------
// Setup the buffers, initialize matrices, and write them into global memory
//--------------------------------------------------------------------------------

        //  Reset A, B and C matrices (just to play it safe)
        //d_a = cl::Buffer(context, h_A.begin(), h_A.end(), true);

        //d_b = cl::Buffer(context, h_B.begin(), h_B.end(), true);

        //d_c = cl::Buffer(context, CL_MEM_WRITE_ONLY, sizeof(float) * size);

//--------------------------------------------------------------------------------
// OpenCL matrix multiplication ... Naive
//--------------------------------------------------------------------------------

        // Create the compute program from the source buffer
        cl::Program program(context, util::loadProgram("cms.cl"), true);

        // Create the compute kernel from the program
        cl::make_kernel<int, cl::Buffer, cl::Buffer, cl::Buffer> cms(program, "cms");

        // cl::NDRange global(N, N);
        // naive_mmul(cl::EnqueueArgs(queue, global), N, d_a, d_b, d_c);

        queue.finish();

        // cl::copy(queue, d_c, h_C.begin(), h_C.end());

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
