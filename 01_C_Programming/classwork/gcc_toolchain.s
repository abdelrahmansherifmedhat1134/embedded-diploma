	.file	"gcc_toolchain.c"
	.text
	.def	__main;	.scl	2;	.type	32;	.endef
	.section .rdata,"dr"
.LC0:
	.ascii "left\0"
.LC1:
	.ascii ".\\gcc_toolchain.c\0"
.LC2:
	.ascii "19:17:07\0"
.LC3:
	.ascii "Nov 24 2025\0"
	.align 8
.LC4:
	.ascii "in %s in %s in %s: line %d:erroooooor\12\0"
	.text
	.globl	main
	.def	main;	.scl	2;	.type	32;	.endef
	.seh_proc	main
main:
	pushq	%rbp
	.seh_pushreg	%rbp
	movq	%rsp, %rbp
	.seh_setframe	%rbp, 0
	subq	$48, %rsp
	.seh_stackalloc	48
	.seh_endprologue
	call	__main
	leaq	.LC0(%rip), %rcx
	call	puts
	movl	$14, 32(%rsp)
	leaq	.LC1(%rip), %r9
	leaq	.LC2(%rip), %r8
	leaq	.LC3(%rip), %rdx
	leaq	.LC4(%rip), %rcx
	call	printf
	movl	$0, %eax
	addq	$48, %rsp
	popq	%rbp
	ret
	.seh_endproc
	.ident	"GCC: (GNU) 8.2.0"
	.def	puts;	.scl	2;	.type	32;	.endef
	.def	printf;	.scl	2;	.type	32;	.endef
