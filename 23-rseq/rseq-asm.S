// int operation_rseq(struct rseq * rseq, struct cacheline *counters) {
// According to the Calling Convention the arguments come in registers:
//     rseq:    %rdi
//     counter: %rsi
.p2align 4
.globl	operation_rseq
.type	operation_rseq, @function
operation_rseq:
.cfi_startproc
        // We will return the number of aborts in %eax. Initialize
        // eax with zero
        xor	%eax, %eax    // %eax = 0

        // We inform the kernel that we are now within a restartable
        // sequence by moving a pointer to operation_rseq_cs (see below)
        // to the kernel-registered rseq object.
        // After an abort, we also jump to this label (restart_ip)
.restart_ip:
        // FIXME: Update rseq->rseq_cs

        // The restartable sequence
        // Implements: [rseq->cpu_id].counter ++;
.start_ip: // Start of restartable sequence
        // HINT: Structure of rseq is documented in /usr/include/linux/rseq.h
        // HINT: rseq->cpu_id == 4(%rdi)
        // HINT: Each counter-cacheline is 64 bytes
.end_ip: // End of restartable sequence
	ret

        // The abort trampoline

        // Before the abort label, the kernel will check if a specific
        // signature is present. We hide this signature in a
        // well-crafted assembler instruction.
        // ud1 <sig>(%rip),%edi
	.byte 0x0f, 0xb9, 0x3d
	.long 0x53053053       // RSEQ_SIG
.abort_ip: // On abort, the kernel will jump here
        // FIXME: count aborts in %eax
	jmp .restart_ip

// } End of operation_rseq()
.cfi_endproc
.size	operation_rseq, .-operation_rseq

// struct rseq_cs operation_rseq_cs -- descriptor for our rseq
.section        .data.rel.local,"aw"
.align 32
.type   operation_rseq_cs, @object
.size   operation_rseq_cs, 32
operation_rseq_cs:
        .long   0   // __u32 version
        .long   0   // __u32 flags
        .quad   .start_ip            // __u64 start_ip
        .quad   .end_ip - .start_ip  // __u64 post_commit_offset
        .quad   .abort_ip            // __u64 abort_ip

.section        .note.GNU-stack,"",@progbits
