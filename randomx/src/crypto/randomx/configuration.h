/*
Copyright (c) 2018-2019, tevador <tevador@gmail.com>

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
	* Redistributions of source code must retain the above copyright
	  notice, this list of conditions and the following disclaimer.
	* Redistributions in binary form must reproduce the above copyright
	  notice, this list of conditions and the following disclaimer in the
	  documentation and/or other materials provided with the distribution.
	* Neither the name of the copyright holder nor the
	  names of its contributors may be used to endorse or promote products
	  derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

//Cache size in KiB. Must be a power of 2.
#define RANDOMX_ARGON_MEMORY       262144

//Number of Argon2d iterations for Cache initialization.
#define RANDOMX_ARGON_ITERATIONS   3

//Number of parallel lanes for Cache initialization.
#define RANDOMX_ARGON_LANES        1

//Argon2d salt
#define RANDOMX_ARGON_SALT         "RandomX\x03"

//Number of random Cache accesses per Dataset item. Minimum is 2.
#define RANDOMX_CACHE_ACCESSES     8

//Target latency for SuperscalarHash (in cycles of the reference CPU).
#define RANDOMX_SUPERSCALAR_LATENCY   170

//Dataset base size in bytes. Must be a power of 2.
#define RANDOMX_DATASET_BASE_SIZE  2147483648

//Dataset extra size. Must be divisible by 64.
#define RANDOMX_DATASET_EXTRA_SIZE 33554368

//Number of instructions in a RandomX program. Must be divisible by 8.
#define RANDOMX_PROGRAM_SIZE       256

//Number of iterations during VM execution.
#define RANDOMX_PROGRAM_ITERATIONS 2048

//Number of chained VM executions per hash.
#define RANDOMX_PROGRAM_COUNT      8

// Increase it if some configs use more cache accesses
#define RANDOMX_CACHE_MAX_ACCESSES 16

// Increase it if some configs use larger superscalar latency
#define RANDOMX_SUPERSCALAR_MAX_LATENCY 256

// Increase it if some configs use larger cache
#define RANDOMX_CACHE_MAX_SIZE  268435456

// Increase it if some configs use larger dataset
#define RANDOMX_DATASET_MAX_SIZE  2181038080

// Increase it if some configs use larger programs
#define RANDOMX_PROGRAM_MAX_SIZE       320

// Increase it if some configs use larger scratchpad
#define RANDOMX_SCRATCHPAD_L3_MAX_SIZE      2097152
