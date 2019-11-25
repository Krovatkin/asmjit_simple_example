#include <asmjit/asmjit.h>
#include <stdio.h>
#include <iostream>
#include <vector>
#include <deque>
#include <string>

#include "scalar_kernel.h"

using namespace asmjit;


void print_offset(void* str, uint64_t i) {
  std::cout << reinterpret_cast<char*>(str) << std::hex << i << std::endl;
}

typedef void (*Add32)(const float*, const float*, float*, int, int);

typedef void (*Add32_2)(const float*, const float*, float*, int*, int*);

void generateLoopNested(int dimensions[], int rank) {
  


  JitRuntime jit;                         // Runtime specialized for JIT code execution.
  CodeHolder code;                        // Holds code and relocation information.
  FileLogger logger(stdout);
  logger.addFlags(FormatOptions::kFlagMachineCode | FormatOptions::kFlagAnnotations | FormatOptions::kIndentationComment);
  code.setLogger(&logger);
  code.init(jit.codeInfo());              // Initialize to the same arch as JIT runtime.
  x86::Compiler cc(&code);                // Create and attach x86::Compiler to `code`.
  cc.addFunc(                             // Begin the function of the following signature:
    FuncSignatureT<void,                  //   Return value - void      (no return value).
      const float*,
      const float*,
      float*,
      int*,
      int*>());


	cc.int3();
    Label L_Loop = cc.newLabel();           // Start of the loop.
    Label L_Exit = cc.newLabel();           // Used to exit early.

    x86::Gp src1 = cc.newIntPtr("src1");
    x86::Gp src2 = cc.newIntPtr("src2");
    x86::Gp dst = cc.newIntPtr("dst");
    x86::Gp starts = cc.newUIntPtr("starts");
    x86::Gp ends = cc.newUIntPtr("ends");

    cc.setArg(0, src1);
    cc.setArg(1, src2);
    cc.setArg(2, dst);
    cc.setArg(3, starts);
    cc.setArg(4, ends);

    //

    std::vector<Label> loop_exits;
    std::vector<Label> loop_starts;
    std::vector<x86::Gp> counters;
    std::vector<int> strides(rank);
    std::deque<std::string> debug_stmts;

    static char* offset_str = "offset = ";


    strides.at(rank - 1) = 1;
    //prologue
    for (int i = 0; i < rank - 1; i++) {
      Label L_Loop = cc.newLabel();           // Start of the loop.
      Label L_Exit = cc.newLabel();           // Used to exit early.
      loop_starts.push_back(L_Loop);
      loop_exits.push_back(L_Exit);
      
      //
      auto cnt_num = std::string("cnt_") + std::to_string(i);
      x86::Gp cnt = cc.newGpd(cnt_num.data());
      cc.mov(cnt, x86::dword_ptr(starts, i << 2));
      counters.push_back(cnt);
      cc.bind(L_Loop);

      //{
      //std::cout << "executing\n";
      //debug_stmts.push_back(cnt_num + " = ");
      //auto very_temp = cc.newGpd("very_temp");
      //cc.mov(very_temp, x86::dword_ptr(ends));
      //FuncCallNode* call_print = cc.call(reinterpret_cast<uint64_t>(&print_offset), FuncSignatureT<void, void*, uint64_t>());
      //call_print->setArg(0, Imm(reinterpret_cast<uint64_t>(debug_stmts.back().data())));
      //call_print->setArg(1, very_temp);
      //}

	  // TODO: switch to count downs to reduce memory accesses
      cc.cmp(cnt, x86::dword_ptr(ends, i << 2));
      cc.jge(loop_exits.at(i));



      //collect strides
      strides.at(rank - 2 - i) = strides.at(rank - 1 - i) * dimensions[rank - 1 - i];
    }

    for (int i = 0; i < rank; i++) {
      std::cout << "strides = " << strides.at(i) << std::endl;
    }

    x86::Gp start = cc.newGpd("offset");
    cc.mov(start, 0);
    x86::Gp tmp = cc.newGpd("tmp");
    for (int i = 0; i < rank - 1; i++) {
      
      cc.mov(tmp, counters.at(i));
	  //TODO: turn into shifts and adds
      cc.imul(tmp, static_cast<int>(strides.at(i)));
      cc.add(start, tmp);
    }
	// real offset in bytes
	cc.shl(start, 2);
    //FuncCallNode* call_print = cc.call(reinterpret_cast<uint64_t>(&print_offset), FuncSignatureT<void, void*, int>());
    //call_print->setArg(0, Imm(reinterpret_cast<uint64_t>(offset_str)));
    //call_print->setArg(1, start);


    //TODO: this can be done only once and cached
    x86::Gp cnt = cc.newGpd("inner");
    cc.mov(cnt, x86::dword_ptr(ends, (rank - 1) << 2));
    cc.sub(cnt, x86::dword_ptr(starts, (rank - 1) << 2));
	
    // loop body
    cc.bind(L_Loop);                        // Bind the beginning of the loop here.
    //auto xmm_a = cc.newXmmPs("a");
	auto xmm_a = cc.newYmmPs("a");
	//TODO: switch to aligned movs
    cc.vmovups(xmm_a, x86::dword_ptr(src1, start));
    //auto xmm_b = cc.newXmmPs("b");
	auto xmm_b = cc.newYmmPs("b");
	cc.vmovups(xmm_b, x86::dword_ptr(src2, start));
	auto xmm_c = cc.newYmmPs("c");
    cc.vaddps(xmm_c, xmm_a, xmm_b);
    cc.vmovups(x86::dword_ptr(dst, start), xmm_c);
    
	cc.add(start, 32);
    cc.sub(cnt, 8);   
    cc.jnz(L_Loop);
    cc.bind(L_Exit);
    //compute start index for innermost loop
    //do stuff

    //epilogue
    for (int i = rank - 2; i >= 0; i--) {
      //increment
      //close loop
      auto cnt = counters.at(i);
      cc.add(cnt, 1);
      //cc.cmp(cnt, x86::dword_ptr(ends, i << 2));
      //cc.jl(loop_starts.at(i));
	  cc.jmp(loop_starts.at(i));
      cc.bind(loop_exits.at(i));
    }

    
    cc.endFunc();  
    cc.finalize(); 




    // String sb;
    // cc.dump(sb, FormatOptions::kFlagMachineCode | FormatOptions::kFlagAnnotations | FormatOptions::kIndentationComment);
    // std::cout << sb.data();
    

    std::cout << "adding jitted code\n";
    Add32_2 add32_2;
    Error err = jit.add(&add32_2, &code);  // Add the generated code to the runtime.
    if (err) {
      std::cout << DebugUtils::errorAsString(err); 
    }


    std::cout << "running a kernel:\n";
    
    int num_elems = 1;
    for (int i = 0; i < rank; i++) {
      std::cout << "adding " << dimensions[i] << std::endl;
      num_elems *= dimensions[i];
    }
    std::cout << "num elements: " << num_elems << std::endl;
    std::vector<float> a(num_elems, 2.f);
    std::vector<float> b(num_elems, 3.f);
    std::vector<float> c(num_elems, 0.f);
    std::vector<int> starts_v(rank, 0);
	std::vector<int> ends_v{1,1,16};
    std::cout << "ends_v.data = " << ends_v.data() << " " << ends_v.data()[0] << std::endl;
    add32_2(a.data(), b.data(), c.data(), starts_v.data(), ends_v.data());
    std::cout << "c = ";
    for (int i = 0; i < num_elems; i++) {
      std::cout << " " << c.at(i); 
    }
    jit.release(add32_2);
}

int main(int argc, char* argv[]) {
  int dims [] = {1, 1, 16};
  generateLoopNested(dims, 3);
  return 0;
}


