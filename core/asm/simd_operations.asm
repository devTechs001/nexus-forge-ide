; core/asm/simd_operations.asm
; SIMD-optimized operations for NexusForge IDE
; Assembled with NASM for x86-64

section .data
    align 32
    shuffle_mask: dd 0, 4, 1, 5, 2, 6, 3, 7

section .text

; Export symbols
global nexus_memcpy_avx2
global nexus_memset_avx2
global nexus_utf8_validate_sse42
global nexus_text_search_avx2
global nexus_color_blend_avx2
global nexus_fast_hash

;------------------------------------------------------------------------------
; Fast memory copy using AVX2
; void nexus_memcpy_avx2(void* dest, const void* src, size_t size)
;------------------------------------------------------------------------------
nexus_memcpy_avx2:
    push rbp
    mov rbp, rsp

    cmp rdx, 32
    jl .small_copy

    mov rax, rdi
    and rax, 31
    jz .aligned

    mov rcx, 32
    sub rcx, rax
    cmp rcx, rdx
    cmova rcx, rdx

.prefix_loop:
    mov al, [rsi]
    mov [rdi], al
    inc rsi
    inc rdi
    dec rdx
    dec rcx
    jnz .prefix_loop

.aligned:
    cmp rdx, 256
    jl .avx2_64

.avx2_256:
    vmovdqu ymm0, [rsi]
    vmovdqu ymm1, [rsi + 32]
    vmovdqu ymm2, [rsi + 64]
    vmovdqu ymm3, [rsi + 96]
    vmovdqu ymm4, [rsi + 128]
    vmovdqu ymm5, [rsi + 160]
    vmovdqu ymm6, [rsi + 192]
    vmovdqu ymm7, [rsi + 224]

    vmovdqa [rdi], ymm0
    vmovdqa [rdi + 32], ymm1
    vmovdqa [rdi + 64], ymm2
    vmovdqa [rdi + 96], ymm3
    vmovdqa [rdi + 128], ymm4
    vmovdqa [rdi + 160], ymm5
    vmovdqa [rdi + 192], ymm6
    vmovdqa [rdi + 224], ymm7

    add rsi, 256
    add rdi, 256
    sub rdx, 256
    cmp rdx, 256
    jge .avx2_256

.avx2_64:
    cmp rdx, 64
    jl .avx2_32

    vmovdqu ymm0, [rsi]
    vmovdqu ymm1, [rsi + 32]
    vmovdqa [rdi], ymm0
    vmovdqa [rdi + 32], ymm1

    add rsi, 64
    add rdi, 64
    sub rdx, 64
    jmp .avx2_64

.avx2_32:
    cmp rdx, 32
    jl .small_copy

    vmovdqu ymm0, [rsi]
    vmovdqa [rdi], ymm0

    add rsi, 32
    add rdi, 32
    sub rdx, 32

.small_copy:
    test rdx, rdx
    jz .done

.byte_loop:
    mov al, [rsi]
    mov [rdi], al
    inc rsi
    inc rdi
    dec rdx
    jnz .byte_loop

.done:
    vzeroupper
    pop rbp
    ret

;------------------------------------------------------------------------------
; Fast memory set using AVX2
;------------------------------------------------------------------------------
nexus_memset_avx2:
    push rbp
    mov rbp, rsp

    movzx eax, sil
    imul eax, 0x01010101
    vmovd xmm0, eax
    vpbroadcastd ymm0, xmm0

    cmp rdx, 32
    jl .small_set

    mov rax, rdi
    and rax, 31
    jz .aligned_set

    mov rcx, 32
    sub rcx, rax
    cmp rcx, rdx
    cmova rcx, rdx

.prefix_set:
    mov [rdi], sil
    inc rdi
    dec rdx
    dec rcx
    jnz .prefix_set

.aligned_set:
    cmp rdx, 128
    jl .set_32

.set_128:
    vmovdqa [rdi], ymm0
    vmovdqa [rdi + 32], ymm0
    vmovdqa [rdi + 64], ymm0
    vmovdqa [rdi + 96], ymm0
    add rdi, 128
    sub rdx, 128
    cmp rdx, 128
    jge .set_128

.set_32:
    cmp rdx, 32
    jl .small_set

    vmovdqa [rdi], ymm0
    add rdi, 32
    sub rdx, 32
    jmp .set_32

.small_set:
    test rdx, rdx
    jz .set_done

.byte_set:
    mov [rdi], sil
    inc rdi
    dec rdx
    jnz .byte_set

.set_done:
    vzeroupper
    pop rbp
    ret

;------------------------------------------------------------------------------
; Fast hash function (xxHash-inspired)
;------------------------------------------------------------------------------
nexus_fast_hash:
    push rbp
    mov rbp, rsp
    push rbx
    push r12
    push r13
    push r14
    push r15

    mov r8, 0x9E3779B185EBCA87
    mov r9, 0xC2B2AE3D27D4EB4F
    mov r10, 0x165667B19E3779F9
    mov r11, 0x85EBCA77C2B2AE63
    mov r12, 0x27D4EB2F165667C5

    mov r13, rdx
    add r13, r8
    add r13, r9
    mov r14, rdx
    add r14, r9
    mov r15, rdx
    mov rbx, rdx
    sub rbx, r8

    mov rcx, rsi
    mov rax, rdi

    cmp rcx, 32
    jl .process_remaining

.loop32:
    mov rdx, [rax]
    imul rdx, r9
    add r13, rdx
    rol r13, 31
    imul r13, r8

    mov rdx, [rax + 8]
    imul rdx, r9
    add r14, rdx
    rol r14, 31
    imul r14, r8

    mov rdx, [rax + 16]
    imul rdx, r9
    add r15, rdx
    rol r15, 31
    imul r15, r8

    mov rdx, [rax + 24]
    imul rdx, r9
    add rbx, rdx
    rol rbx, 31
    imul rbx, r8

    add rax, 32
    sub rcx, 32
    cmp rcx, 32
    jge .loop32

    rol r13, 1
    rol r14, 7
    rol r15, 12
    rol rbx, 18

    add r13, r14
    add r13, r15
    add r13, rbx

    jmp .finalize

.process_remaining:
    mov r13, rdx
    add r13, r12

.finalize:
    add r13, rsi

.loop8:
    cmp rcx, 8
    jl .loop1

    mov rdx, [rax]
    imul rdx, r9
    xor r13, rdx
    rol r13, 27
    imul r13, r8
    add r13, r11

    add rax, 8
    sub rcx, 8
    jmp .loop8

.loop1:
    test rcx, rcx
    jz .final_mix

    movzx edx, byte [rax]
    imul rdx, r12
    xor r13, rdx
    rol r13, 11
    imul r13, r8

    inc rax
    dec rcx
    jmp .loop1

.final_mix:
    mov rax, r13
    shr rax, 33
    xor r13, rax
    imul r13, r9

    mov rax, r13
    shr rax, 29
    xor r13, rax
    imul r13, r10

    mov rax, r13
    shr rax, 32
    xor rax, r13

    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret

section .rodata
    align 32
    ones: times 32 db 0xFF
