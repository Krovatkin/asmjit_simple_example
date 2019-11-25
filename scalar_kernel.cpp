void generateScalarKernel() {

	JitRuntime jit;                         // Runtime specialized for JIT code execution.
	CodeHolder code;                        // Holds code and relocation information.
	FileLogger logger(stdout);
	logger.addFlags(FormatOptions::kFlagMachineCode);
	code.setLogger(&logger);
	code.init(jit.codeInfo());              // Initialize to the same arch as JIT runtime.
	x86::Compiler cc(&code);                // Create and attach x86::Compiler to `code`.
	cc.addFunc(                             // Begin the function of the following signature:
		FuncSignatureT<void,                  //   Return value - void      (no return value).
		const float*,
		const float*,
		float*,
		int,
		int>());

	Label L_Loop = cc.newLabel();           // Start of the loop.
	Label L_Exit = cc.newLabel();           // Used to exit early.

	x86::Gp src1 = cc.newIntPtr("src1");
	x86::Gp src2 = cc.newIntPtr("src2");
	x86::Gp dst = cc.newIntPtr("dst");
	x86::Gp cnt = cc.newUIntPtr("cnt");
	x86::Gp offset = cc.newUIntPtr("offset");

	cc.setArg(0, src1);
	cc.setArg(1, src2);
	cc.setArg(2, dst);
	cc.setArg(3, cnt);
	cc.setArg(4, offset);

	cc.test(cnt, cnt);                      // Early exit if length is zero.
	cc.jz(L_Exit);

	cc.bind(L_Loop);                        // Bind the beginning of the loop here.

	auto xmm_a = cc.newXmmSs("a");
	cc.movss(xmm_a, x86::dword_ptr(src1, offset, 2));
	auto xmm_b = cc.newXmmSs("b");
	cc.movss(xmm_b, x86::dword_ptr(src2, offset, 2));
	cc.addss(xmm_a, xmm_b);
	cc.movss(x86::dword_ptr(dst, offset, 2), xmm_a);

	cc.add(src1, 4);
	cc.add(src2, 4);
	cc.add(dst, 4);

	cc.dec(cnt);
	cc.jnz(L_Loop);

	cc.bind(L_Exit);
	cc.endFunc();
	cc.finalize();

	const int size = 3;
	float a[] = { 2.f, 3.f, 4.f };
	float b[] = { 1.f, 1.f, 1.f };
	float c[] = { 0.f, 0.f, 0.f };


	for (auto i = 0; i < size; i++) {
		std::cout << " " << c[i];
	}

	Add32 add32;
	Error err = jit.add(&add32, &code);  // Add the generated code to the runtime.
	add32(a, b, c, size - 1, 1);
	jit.release(add32);

	for (auto i = 0; i < size; i++) {
		std::cout << " " << c[i];
	}
}
