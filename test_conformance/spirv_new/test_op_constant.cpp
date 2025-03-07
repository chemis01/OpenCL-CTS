//
// Copyright (c) 2016-2023 The Khronos Group Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include "testBase.h"
#include "types.hpp"



template<typename T>
int test_constant(cl_device_id deviceID, cl_context context,
                  cl_command_queue queue, const char *name,
                  std::vector<T> &results,
                  bool (*notEqual)(const T&, const T&) = isNotEqual<T>)
{
    if(std::string(name).find("double") != std::string::npos) {
        if(!is_extension_available(deviceID, "cl_khr_fp64")) {
            log_info("Extension cl_khr_fp64 not supported; skipping double tests.\n");
            return 0;
        }
    }
    clProgramWrapper prog;
    cl_int err = get_program_with_il(prog, deviceID, context, name);
    SPIRV_CHECK_ERROR(err, "Failed to build program");

    clKernelWrapper kernel = clCreateKernel(prog, name, &err);
    SPIRV_CHECK_ERROR(err, "Failed to create kernel");

    int num = (int)results.size();

    size_t bytes = num * sizeof(T);
    clMemWrapper mem = clCreateBuffer(context, CL_MEM_READ_WRITE, bytes, NULL, &err);
    SPIRV_CHECK_ERROR(err, "Failed to create buffer");

    err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &mem);
    SPIRV_CHECK_ERROR(err, "Failed to set kernel argument");

    size_t global = num;
    err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global, NULL, 0, NULL, NULL);
    SPIRV_CHECK_ERROR(err, "Failed to enqueue kernel");

    std::vector<T> host(num);
    err = clEnqueueReadBuffer(queue, mem, CL_TRUE, 0, bytes, &host[0], 0, NULL, NULL);
    SPIRV_CHECK_ERROR(err, "Failed to copy from cl_buffer");

    for (int i = 0; i < num; i++) {
        if (notEqual(host[i], results[i])) {
            log_error("Values do not match at location %d\n", i);
            return -1;
        }
    }
    return 0;
}

#define TEST_CONSTANT(NAME, type, value)                                       \
    REGISTER_TEST(op_constant_##NAME##_simple)                                 \
    {                                                                          \
        std::vector<type> results(1024, (type)value);                          \
        return test_constant(device, context, queue,                           \
                             "constant_" #NAME "_simple", results);            \
    }

// Boolean tests
TEST_CONSTANT(true  , cl_int  , 1                   )
TEST_CONSTANT(false , cl_int  , 0                   )

// Integer tests
TEST_CONSTANT(int   , cl_int   , 123                )
TEST_CONSTANT(uint  , cl_uint  , 54321               )
TEST_CONSTANT(char  , cl_char  , 20                 )
TEST_CONSTANT(uchar , cl_uchar , 19                  )
TEST_CONSTANT(ushort, cl_ushort, 65000               )
TEST_CONSTANT(long  , cl_long  , 34359738368L       )
TEST_CONSTANT(ulong , cl_ulong , 9223372036854775810UL)

#ifdef __GNUC__
// std::vector<cl_short> is causing compilation errors on GCC 5.3 (works on gcc 4.8)
// Needs further investigation
    TEST_CONSTANT(short , int16_t , 32000              )
#else
TEST_CONSTANT(short , cl_short , 32000              )
#endif

// Float tests
TEST_CONSTANT(float   , cl_float  , 3.1415927        )
TEST_CONSTANT(double  , cl_double , 3.141592653589793)

REGISTER_TEST(op_constant_int4_simple)
{
    cl_int4 value = { { 123, 122, 121, 119 } };
    std::vector<cl_int4> results(256, value);
    return test_constant(device, context, queue, "constant_int4_simple",
                         results);
}

REGISTER_TEST(op_constant_int3_simple)
{
    cl_int3 value = { { 123, 122, 121, 0 } };
    std::vector<cl_int3> results(256, value);
    return test_constant(device, context, queue, "constant_int3_simple",
                         results, isVectorNotEqual<cl_int3, 3>);
}

REGISTER_TEST(op_constant_struct_int_float_simple)
{
    AbstractStruct2<int, float> value = {1024, 3.1415};
    std::vector<AbstractStruct2<int, float> > results(256, value);
    return test_constant(device, context, queue,
                         "constant_struct_int_float_simple", results);
}

REGISTER_TEST(op_constant_struct_int_char_simple)
{
    AbstractStruct2<int, char> value = { 2100483600, (char)128 };
    std::vector<AbstractStruct2<int, char> > results(256, value);
    return test_constant(device, context, queue,
                         "constant_struct_int_char_simple", results);
}

REGISTER_TEST(op_constant_struct_struct_simple)
{
    typedef AbstractStruct2<int, char> CustomType1;
    typedef AbstractStruct2<cl_int2, CustomType1> CustomType2;

    CustomType1 value1 = { 2100483600, (char)128 };
    cl_int2 intvals = { { 2100480000, 2100480000 } };
    CustomType2 value2 = {intvals, value1};

    std::vector<CustomType2> results(256, value2);
    return test_constant(device, context, queue,
                         "constant_struct_struct_simple", results);
}

REGISTER_TEST(op_constant_half_simple)
{
    PASSIVE_REQUIRE_FP16_SUPPORT(device);
    std::vector<cl_float> results(1024, 3.25);
    return test_constant(device, context, queue, "constant_half_simple",
                         results);
}
