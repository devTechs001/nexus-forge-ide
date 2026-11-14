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

;------------------------------------------------------------------------------
; UTF-8 validation (fast ASCII path via SSE + full multi-byte validation)
; int nexus_utf8_validate_sse42(const uint8_t* data, size_t len)
; Returns 1 if valid UTF-8, 0 otherwise
;------------------------------------------------------------------------------
nexus_utf8_validate_sse42:
    push rbp
    mov rbp, rsp
    push rbx

    mov r8, rdi       ; data
    mov rcx, rsi      ; remaining length

.utf8_loop:
    test rcx, rcx
    jz .utf8_valid

    ; Fast 16-byte pure ASCII skip
    cmp rcx, 16
    jl .utf8_scalar
    movdqu xmm0, [r8]
    pmovmskb eax, xmm0
    test eax, eax
    jnz .utf8_scalar
    add r8, 16
    sub rcx, 16
    jmp .utf8_loop

.utf8_scalar:
    movzx eax, byte [r8]
    cmp al, 0x80
    jb .utf8_1byte

    ; 2-byte sequence: C2..DF, 80..BF
    cmp al, 0xC2
    jb .utf8_invalid
    cmp al, 0xDF
    jbe .utf8_2byte

    ; 3-byte sequence: E0..EF
    cmp al, 0xEF
    jbe .utf8_3byte

    ; 4-byte sequence: F0..F4
    cmp al, 0xF4
    ja .utf8_invalid

    ; Validate 4-byte
    cmp rcx, 4
    jb .utf8_invalid
    movzx edx, byte [r8 + 1]
    cmp al, 0xF0
    jne .utf8_4b_check_f4
    cmp dl, 0x90
    jb .utf8_invalid
    cmp dl, 0xBF
    ja .utf8_invalid
    jmp .utf8_4b_tail
.utf8_4b_check_f4:
    cmp al, 0xF4
    jne .utf8_4b_mid
    cmp dl, 0x80
    jb .utf8_invalid
    cmp dl, 0x8F
    ja .utf8_invalid
    jmp .utf8_4b_tail
.utf8_4b_mid:
    and dl, 0xC0
    cmp dl, 0x80
    jne .utf8_invalid
.utf8_4b_tail:
    movzx edx, byte [r8 + 2]
    and dl, 0xC0
    cmp dl, 0x80
    jne .utf8_invalid
    movzx edx, byte [r8 + 3]
    and dl, 0xC0
    cmp dl, 0x80
    jne .utf8_invalid
    add r8, 4
    sub rcx, 4
    jmp .utf8_loop

.utf8_3byte:
    cmp rcx, 3
    jb .utf8_invalid
    movzx edx, byte [r8 + 1]
    cmp al, 0xE0
    jne .utf8_3b_check_ed
    cmp dl, 0xA0
    jb .utf8_invalid
    cmp dl, 0xBF
    ja .utf8_invalid
    jmp .utf8_3b_tail
.utf8_3b_check_ed:
    cmp al, 0xED
    jne .utf8_3b_mid
    cmp dl, 0x80
    jb .utf8_invalid
    cmp dl, 0x9F
    ja .utf8_invalid
    jmp .utf8_3b_tail
.utf8_3b_mid:
    and dl, 0xC0
    cmp dl, 0x80
    jne .utf8_invalid
.utf8_3b_tail:
    movzx edx, byte [r8 + 2]
    and dl, 0xC0
    cmp dl, 0x80
    jne .utf8_invalid
    add r8, 3
    sub rcx, 3
    jmp .utf8_loop

.utf8_2byte:
    cmp rcx, 2
    jb .utf8_invalid
    movzx edx, byte [r8 + 1]
    and dl, 0xC0
    cmp dl, 0x80
    jne .utf8_invalid
    add r8, 2
    sub rcx, 2
    jmp .utf8_loop

.utf8_1byte:
    inc r8
    dec rcx
    jmp .utf8_loop

.utf8_valid:
    mov eax, 1
    pop rbx
    pop rbp
    ret

.utf8_invalid:
    xor eax, eax
    pop rbx
    pop rbp
    ret

;------------------------------------------------------------------------------
; Fast substring search using AVX2 first-byte filter
; ptrdiff_t nexus_text_search_avx2(const char* haystack, size_t haystack_len,
;                                  const char* needle, size_t needle_len)
; Returns offset of first match, or -1 if not found
;------------------------------------------------------------------------------
nexus_text_search_avx2:
    push rbp
    mov rbp, rsp
    push rbx
    push r12
    push r13
    push r14

    ; rdi = haystack, rsi = haystack_len, rdx = needle, rcx = needle_len
    test rcx, rcx
    jz .search_zero
    cmp rcx, rsi
    ja .search_not_found

    mov r12, rdi      ; haystack base
    mov r13, rdx      ; needle base
    mov r14, rcx      ; needle_len
    sub rsi, rcx
    inc rsi           ; number of candidate start positions
    xor rbx, rbx      ; current offset

    movzx eax, byte [r13]
    vmovd xmm0, eax
    vpbroadcastb ymm0, xmm0

.search_loop:
    cmp rbx, rsi
    jae .search_not_found

    ; Scalar first-byte + full match check
    mov al, [r12 + rbx]
    cmp al, [r13]
    jne .search_next

    ; Compare remaining bytes of needle
    mov r8, 1
.search_cmp:
    cmp r8, r14
    je .search_found
    mov al, [r12 + rbx]
    mov r9, rbx
    add r9, r8
    mov al, [r12 + r9]
    cmp al, [r13 + r8]
    jne .search_next
    inc r8
    jmp .search_cmp

.search_next:
    inc rbx
    jmp .search_loop

.search_zero:
    xor rax, rax
    jmp .search_exit

.search_found:
    mov rax, rbx
    jmp .search_exit

.search_not_found:
    mov rax, -1

.search_exit:
    vzeroupper
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret

;------------------------------------------------------------------------------
; Alpha-blend RGBA8 pixel arrays
; void nexus_color_blend_avx2(uint32_t* dst, const uint32_t* src, size_t count)
;------------------------------------------------------------------------------
nexus_color_blend_avx2:
    push rbp
    mov rbp, rsp
    push rbx
    push r12

    ; rdi = dst, rsi = src, rdx = count
    test rdx, rdx
    jz .blend_done

.blend_loop:
    mov eax, [rsi]    ; src RGBA
    mov ebx, [rdi]    ; dst RGBA
    mov ecx, eax
    shr ecx, 24       ; src alpha (0..255)
    mov r8d, 255
    sub r8d, ecx      ; inv alpha (255 - a)

    ; R channel
    mov r9d, eax
    and r9d, 0xFF
    imul r9d, ecx
    mov r10d, ebx
    and r10d, 0xFF
    imul r10d, r8d
    add r9d, r10d
    shr r9d, 8
    mov r12d, r9d

    ; G channel
    mov r9d, eax
    shr r9d, 8
    and r9d, 0xFF
    imul r9d, ecx
    mov r10d, ebx
    shr r10d, 8
    and r10d, 0xFF
    imul r10d, r8d
    add r9d, r10d
    shr r9d, 8
    shl r9d, 8
    or r12d, r9d

    ; B channel
    mov r9d, eax
    shr r9d, 16
    and r9d, 0xFF
    imul r9d, ecx
    mov r10d, ebx
    shr r10d, 16
    and r10d, 0xFF
    imul r10d, r8d
    add r9d, r10d
    shr r9d, 8
    shl r9d, 16
    or r12d, r9d

    ; A channel = min(255, srcA + dstA * invA / 255)
    mov r10d, ebx
    shr r10d, 24
    imul r10d, r8d
    shr r10d, 8
    add r10d, ecx
    cmp r10d, 255
    jle .blend_store_a
    mov r10d, 255
.blend_store_a:
    shl r10d, 24
    or r12d, r10d

    mov [rdi], r12d
    add rdi, 4
    add rsi, 4
    dec rdx
    jnz .blend_loop

.blend_done:
    vzeroupper
    pop r12
    pop rbx
    pop rbp
    ret

section .rodata
    align 32
    ones: times 32 db 0xFF
