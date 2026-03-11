; core/asm/memory_ops.asm
; Memory operation optimizations for NexusForge IDE
; Assembled with NASM for x86-64

section .data
    align 64
    zero_vector: times 32 db 0

section .text

global nexus_memcmp_avx2
global nexus_memmove_avx2
global nexus_bzero_avx2
global nexus_memchr_avx2
global nexus_strlen_sse42
global nexus_strncmp_avx2

;------------------------------------------------------------------------------
; Fast memory compare using AVX2
; int nexus_memcmp_avx2(const void* s1, const void* s2, size_t n)
;------------------------------------------------------------------------------
nexus_memcmp_avx2:
    push rbp
    mov rbp, rsp
    push rbx
    push r12
    push r13

    mov r12, rdi      ; s1
    mov r13, rsi      ; s2
    mov rcx, rdx      ; n

    test rcx, rcx
    jz .equal

    cmp rcx, 32
    jl .byte_compare

.compare_32:
    vmovdqu ymm0, [r12]
    vmovdqu ymm1, [r13]
    vpcmpeqb ymm2, ymm0, ymm1
    vpmovmskb eax, ymm2

    cmp eax, 0xFFFFFFFF
    jne .found_diff

    add r12, 32
    add r13, 32
    sub rcx, 32
    cmp rcx, 32
    jge .compare_32

    test rcx, rcx
    jz .equal

.byte_compare:
    movzx eax, byte [r12]
    movzx ebx, byte [r13]
    cmp al, bl
    jne .return_diff

    inc r12
    inc r13
    dec rcx
    jnz .byte_compare

.equal:
    vzeroupper
    xor eax, eax
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret

.found_diff:
    ; Find which byte differs
    bsf ecx, eax
    movzx eax, byte [r12 + rcx]
    movzx ebx, byte [r13 + rcx]
    sub eax, ebx

    vzeroupper
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret

.return_diff:
    sub eax, ebx
    vzeroupper
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret

;------------------------------------------------------------------------------
; Fast memmove using AVX2 (handles overlap)
; void* nexus_memmove_avx2(void* dest, const void* src, size_t n)
;------------------------------------------------------------------------------
nexus_memmove_avx2:
    push rbp
    mov rbp, rsp

    mov rax, rdi      ; save dest for return
    mov rcx, rdx      ; n

    test rcx, rcx
    jz .move_done

    ; Check for overlap
    cmp rdi, rsi
    je .move_done

    jb .forward_copy    ; dest < src, forward copy is safe

    ; Backward copy for overlapping regions
    lea rdi, [rdi + rcx - 1]
    lea rsi, [rsi + rcx - 1]
    std                 ; set direction flag

.backward_loop:
    mov al, [rsi]
    mov [rdi], al
    dec rsi
    dec rdi
    dec rcx
    jnz .backward_loop

    cld                 ; clear direction flag
    jmp .move_done

.forward_copy:
    cmp rcx, 32
    jl .forward_byte

.forward_32:
    vmovdqu ymm0, [rsi]
    vmovdqu [rdi], ymm0

    add rsi, 32
    add rdi, 32
    sub rcx, 32
    cmp rcx, 32
    jge .forward_32

.forward_byte:
    test rcx, rcx
    jz .move_done

    mov al, [rsi]
    mov [rdi], al
    inc rsi
    inc rdi
    dec rcx
    jnz .forward_byte

.move_done:
    vzeroupper
    pop rbp
    ret

;------------------------------------------------------------------------------
; Fast bzero using AVX2
; void nexus_bzero_avx2(void* s, size_t n)
;------------------------------------------------------------------------------
nexus_bzero_avx2:
    push rbp
    mov rbp, rsp

    mov rax, rdi
    mov rcx, rsi

    test rcx, rcx
    jz .bzero_done

    cmp rcx, 32
    jl .bzero_byte

    ; Align to 32 bytes
    mov rdx, rax
    and rdx, 31
    jz .bzero_aligned

    mov rdx, 32
    sub rdx, rax
    and rdx, 31
    cmp rdx, rcx
    cmova rdx, rcx

.bzero_align:
    mov byte [rax], 0
    inc rax
    dec rcx
    dec rdx
    jnz .bzero_align

.bzero_aligned:
    vxorps ymm0, ymm0, ymm0

    cmp rcx, 128
    jl .bzero_32

.bzero_128:
    vmovdqa [rax], ymm0
    vmovdqa [rax + 32], ymm0
    vmovdqa [rax + 64], ymm0
    vmovdqa [rax + 96], ymm0
    add rax, 128
    sub rcx, 128
    cmp rcx, 128
    jge .bzero_128

.bzero_32:
    cmp rcx, 32
    jl .bzero_byte

    vmovdqa [rax], ymm0
    add rax, 32
    sub rcx, 32
    jmp .bzero_32

.bzero_byte:
    test rcx, rcx
    jz .bzero_done

    mov byte [rax], 0
    inc rax
    dec rcx
    jnz .bzero_byte

.bzero_done:
    vzeroupper
    pop rbp
    ret

;------------------------------------------------------------------------------
; Fast memchr using AVX2
; void* nexus_memchr_avx2(const void* s, int c, size_t n)
;------------------------------------------------------------------------------
nexus_memchr_avx2:
    push rbp
    mov rbp, rsp

    mov rax, rdi      ; s
    movzx ecx, sil    ; c
    mov rdx, rsi      ; n (actually in rdx from calling convention fix)

    test rdx, rdx
    jz .not_found

    ; Broadcast character to all bytes of ymm0
    mov eax, ecx
    imul eax, 0x01010101
    vmovd xmm0, eax
    vpbroadcastb ymm0, xmm0

    cmp rdx, 32
    jl .memchr_byte

.memchr_32:
    vmovdqu ymm1, [rax]
    vpcmpeqb ymm2, ymm1, ymm0
    vpmovmskb ecx, ymm2

    test ecx, ecx
    jnz .found_char

    add rax, 32
    sub rdx, 32
    cmp rdx, 32
    jge .memchr_32

.memchr_byte:
    test rdx, rdx
    jz .not_found

    movzx ecx, byte [rax]
    cmp cl, sil
    je .found_single

    inc rax
    dec rdx
    jmp .memchr_byte

.found_char:
    bsf ecx, ecx
    add rax, rcx
    vzeroupper
    pop rbp
    ret

.found_single:
    vzeroupper
    pop rbp
    ret

.not_found:
    xor rax, rax
    vzeroupper
    pop rbp
    ret

;------------------------------------------------------------------------------
; Fast strlen using SSE4.2
; size_t nexus_strlen_sse42(const char* s)
;------------------------------------------------------------------------------
nexus_strlen_sse42:
    push rbp
    mov rbp, rsp

    mov rax, rdi
    mov rdx, rdi

    ; Align to 16 bytes
    mov rcx, rax
    and rcx, 15
    jz .strlen_aligned

    neg rcx
    add rcx, 16

.strlen_align:
    cmp byte [rax], 0
    je .strlen_done
    inc rax
    dec rcx
    jnz .strlen_align

.strlen_aligned:
    ; Load 16 bytes
    movdqu xmm0, [rax]

    ; Check for null byte
    pcmpeqb xmm0, xmm1
    pmovmskb ecx, xmm0

    test ecx, ecx
    jnz .found_null

    add rax, 16
    jmp .strlen_aligned

.found_null:
    bsf ecx, ecx
    add rax, rcx

.strlen_done:
    sub rax, rdx
    pop rbp
    ret

;------------------------------------------------------------------------------
; Fast strncmp using AVX2
; int nexus_strncmp_avx2(const char* s1, const char* s2, size_t n)
;------------------------------------------------------------------------------
nexus_strncmp_avx2:
    push rbp
    mov rbp, rsp
    push rbx

    mov rax, rdi
    mov rbx, rsi
    mov rcx, rdx

    test rcx, rcx
    jz .strncmp_equal

.strncmp_loop:
    dec rcx
    jz .strncmp_equal

    movzx edx, byte [rax]
    movzx esi, byte [rbx]

    cmp dl, 0
    je .strncmp_equal
    cmp sil, 0
    je .strncmp_diff

    cmp dl, sil
    jne .strncmp_diff

    inc rax
    inc rbx
    jmp .strncmp_loop

.strncmp_equal:
    xor eax, eax
    pop rbx
    pop rbp
    ret

.strncmp_diff:
    movzx eax, dl
    movzx ebx, sil
    sub eax, ebx
    pop rbx
    pop rbp
    ret
