	.text
	.file	"factorial.c"
	.globl	main                            # -- Begin function main
	.p2align	4, 0x90
	.type	main,@function
main:                                   # @main
	.cfi_startproc
# %bb.0:                                # %entry
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	subq	$16, %rsp
	movq	$1, (%rax)
	movl	$.L.str, %edi
	xorl	%eax, %eax
	callq	printf
	leaq	-4(%rbp), %rsi
	movl	$.L.str.1, %edi
	xorl	%eax, %eax
	callq	__isoc99_scanf
	cmpl	$0, -4(%rbp)
	js	.LBB0_1
# %bb.2:                                # %if.else
	movl	$1, %eax
	cmpl	-4(%rbp), %eax
	jg	.LBB0_5
	.p2align	4, 0x90
.LBB0_4:                                # %for.body
                                        # =>This Inner Loop Header: Depth=1
	movq	(%rax), %rcx
	imulq	%rax, %rcx
	movq	%rcx, (%rax)
	addq	$1, %rax
	cmpl	-4(%rbp), %eax
	jle	.LBB0_4
.LBB0_5:                                # %for.end
	movl	-4(%rbp), %esi
	movq	(%rax), %rdx
	movl	$.L.str.3, %edi
	xorl	%eax, %eax
	callq	printf
	jmp	.LBB0_6
.LBB0_1:                                # %if.then
	movl	$.L.str.2, %edi
	xorl	%eax, %eax
	callq	printf
.LBB0_6:                                # %if.end
	xorl	%eax, %eax
	addq	$16, %rsp
	popq	%rbp
	.cfi_def_cfa %rsp, 8
	retq
.Lfunc_end0:
	.size	main, .Lfunc_end0-main
	.cfi_endproc
                                        # -- End function
	.type	.L.str,@object                  # @.str
	.section	.rodata.str1.1,"aMS",@progbits,1
.L.str:
	.asciz	"Enter an integer: "
	.size	.L.str, 19

	.type	.L.str.1,@object                # @.str.1
.L.str.1:
	.asciz	"%d"
	.size	.L.str.1, 3

	.type	.L.str.2,@object                # @.str.2
.L.str.2:
	.asciz	"Error! Factorial of a negative number doesn't exist."
	.size	.L.str.2, 53

	.type	.L.str.3,@object                # @.str.3
.L.str.3:
	.asciz	"Factorial of %d = %llu"
	.size	.L.str.3, 23

	.ident	"clang version 13.0.1 (https://github.com/llvm/llvm-project.git 19b8368225dc9ec5a0da547eae48c10dae13522d)"
	.section	".note.GNU-stack","",@progbits
