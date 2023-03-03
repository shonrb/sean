; A brainfuck interpreter in x86-64 assembly
%define MEMORY_SIZE 2000
global _start

section .data
running_str:   db "Running script: ", 0
usage_str:     db "An input file is needed", 10, 0
file_err_str:  db "Failed to open file", 10, 0
size_err_str:  db "Failed to get file size", 10, 0
alloc_err_str: db "Failed to allocate memory", 10, 0
dp_err_str:    db "Data pointer out of bounds", 10, 0
get_char_str:  db "Input a character: ", 0

section .bss
program_ip:      resq 1
program_dp:      resq 1
program_memory:  resb MEMORY_SIZE
script_buf:      resq 1
script_buf_size: resq 1

section .text
; ----------------------------------------
; main function 
; ----------------------------------------
_start:
    ; make stack frame
    ; 0   - 144 : struct stat
    ; 144 - 152 : fd for script file
    push rbp
    mov rbp, rsp
    sub rsp, 144 + 8

    ; get argc count, abort if incorrect
    mov eax, [rbp + 8]
    cmp eax, 2
    jne .error_bad_argv

    ; open the file we've been given
    mov rax, 2          ; sys_open
    mov rdi, [rbp + 24] ; filename in argv[1]
    mov rsi, 0          ; O_RDONLY read only mode
    syscall
    cmp rax, 0
    jl .error_file_open
    mov [rsp + 144], rax 

    ; get the size of the file
    mov rax, 5   ; sys_fstat
    mov rdi, [rsp + 144] ; file descriptor
    mov rsi, rsp ; pointer to stat buffer
    syscall      ; filesize should now be in stat struct
    cmp rax, 0   
    jne .error_file_size

    ; allocate memory for our script
    mov rax, 9          ; sys_mmap
    mov rdi, 0          ; address = NULL
    mov rsi, [rsp + 48] ; size, 48 is offset of filesize in stat
    mov rdx, 3          ; prot = PROT_READ | PROT_WRITE
    mov r10, 34         ; flags = MAP_ANON | MAP_PRIVATE
    mov r8,  -1         ; fd = -1
    mov r9,  0          ; offset = 0
    syscall
    cmp rax, 0
    jl .error_alloc
    mov [script_buf], rax
    mov rax, [rsp + 48]
    mov [script_buf_size], rax
    
    ; write our script to memory
    mov rax, 0                 ; sys_read
    mov rdi, [rsp + 144]       ; fd for our file
    mov rsi, [script_buf]      ; pointer to our block of memory
    mov rdx, [script_buf_size] ; size
    syscall
    cmp rax, 0
    jl .error_file_size

    ; close our file
    mov rax, 3           ; sys_close
    mov rdi, [rsp + 144] ; our fp
    syscall

    ; print a message to stdout
    ; print everything before the filename
    mov rdi, running_str
    call _print_cstr
    ; print the filename
    mov rdi, [rbp + 24]
    call _print_cstr
    ; print a newline
    push 10    ; 10 is newline
    mov rdi, rsp
    mov rsi, 1
    call _print_str
    pop rax ; discard

    ; start our read loop
.loop_start:
    mov rax, [program_ip]      ; get the instruction pointer
    cmp rax, [script_buf_size] ; make sure it isn't out of bounds
    jge .exit
    mov rbx, [script_buf]      ; get the script address
    lea rax, [rbx + rax]       ; get the address of the op pointed to by IP
    mov al, [rax]              ; get the instruction

    mov rcx, [program_dp]  ; get the data pointer
    cmp rcx, MEMORY_SIZE   ; make sure it isnt out of bounds
    jge .error_dp_oob      
    lea rbx, [program_memory + rcx]

    ; Execute the specific instruction
    cmp al, '>'
    je .op_increment_dp
    cmp al, '<'
    je .op_decrement_dp
    cmp al, '+'
    je .op_increment_value
    cmp al, '-'
    je .op_decrement_value
    cmp al, '.'
    je .op_out
    cmp al, ','
    je .op_in
    cmp al, '['
    je .op_jump_forwards
    cmp al, ']'           
    je .op_jump_backwards
    ; Anything else is a comment
    jmp .next_loop        

.op_increment_dp:
    ; >
    ; Increment the data pointer (to point to the next cell to the right). 
    add qword [program_dp], 1
    jmp .next_loop
.op_decrement_dp:
    ; <
    ; Decrement the data pointer (to point to the next cell to the left). 
    sub qword [program_dp], 1
    jmp .next_loop
.op_increment_value:
    ; +
    ; Increment (increase by one) the byte at the data pointer. 
    add byte [rbx], 1
    jmp .next_loop
.op_decrement_value:
    ; - 
    ; Decrement (decrease by one) the byte at the data pointer. 
    sub byte [rbx], 1
    jmp .next_loop
.op_out:
    ; .
    ; Output the byte at the data pointer. 
    mov rdi, rbx
    mov rsi, 1
    call _print_str
    jmp .next_loop
.op_in:
    ; ,
    ; Accept one byte of input, storing its value in the 
    ; byte at the data pointer. 
    mov rax, 0    ; sys_read
    mov rdi, 0    ; stdin
    mov rsi, rbx  ; 
    mov rdx, 1    ; 1 byte
    syscall
    jmp .next_loop
.op_jump_forwards:
    ; [
    ; If the byte at the data pointer is zero, then instead of moving the 
    ; instruction pointer forward to the next command, jump it forward to the 
    ; command after the matching ] command. 
    cmp byte [rbx], 0
    jne .next_loop
    mov dil, '['
    mov sil, ']'
    mov rdx, 1
    call _jump_to_matching_brace
    jmp .next_loop
.op_jump_backwards:
    ; ]
    ; If the byte at the data pointer is nonzero, then instead of moving the 
    ; instruction pointer forward to the next command, jump it back to the 
    ; command after the matching [ command.
    cmp byte [rbx], 0
    je .next_loop
    mov dil, ']'
    mov sil, '['
    mov rdx, -1
    call _jump_to_matching_brace
    jmp .next_loop 

.next_loop:
    add qword [program_ip], 1
    jmp .loop_start

.exit:
    mov rax, 60
    mov rdi, 0
    syscall
.error_bad_argv:
    mov rdi, usage_str
    call _print_cstr
    jmp .exit
.error_file_open:
    mov rdi, file_err_str
    call _print_cstr
    jmp .exit
.error_file_size:
    mov rdi, size_err_str
    call _print_cstr
    jmp .exit
.error_alloc:
    mov rdi, alloc_err_str
    call _print_cstr
    jmp .exit
.error_dp_oob:
    mov rdi, dp_err_str
    call _print_cstr
    jmp .exit

; ---------------------------------------
; _jump_to_matching_brace
; Helper function to set IP to a matching brace
; matching brace example: [   [   [      ]    [     ]     ]    ]
;                             ^---------------------------^ 
; params: 
;   dil: character we're jumping from ('[' or ']') 
;   sil: character we're jumping to   ('[' or ']') 
;   rdx: change in ip per step        (1 or -1)
; ---------------------------------------
_jump_to_matching_brace:
    mov rax, 0 ; brace level
.loop_start:
    ; step the instruction pointer
    add qword [program_ip], rdx
    ; get the character at the ip
    mov r8, [script_buf]
    mov r9, [program_ip]
    lea rbx, [r8 + r9]
    mov bl, [rbx]
    ; if the sym is our starting one, increases the level
    cmp bl, dil
    je .increase_level
    ; if the sym is not our starting or ending one, ignore
    cmp bl, sil
    jne .loop_start
    ; the sym is our ending one, if the level is not 0 then decrease it
    cmp rax, 0
    jg .decrease_level
    ; otherwise its 0, we have found the target
    jmp .end

.increase_level:
    add rax, 1
    jmp .loop_start
.decrease_level:
    sub rax, 1
    jmp .loop_start
.end:
    ret

; ------------------------------------------
; _print_cstr
; prints a null-terminated string to stdout
; params:
;   string: rdi
; ------------------------------------------
_print_cstr:
    mov r8, rdi
    call _string_len
    mov rdi, r8
    mov rsi, rax
    call _print_str
    ret

; -----------------------------------------
; _print_str
; prints a string to stdout
; params:
;   string: rdi
;   len: rsi
; -----------------------------------------
_print_str:
    mov rax, 1   ; sys_write
    mov rdx, rsi ; length
    mov rsi, rdi ; buffer
    mov rdi, 1   ; stdout
    syscall
    ret

; ----------------------------------------
; _string_len
; returns the length of a string in rax
; params:
;   string: rdi
; ----------------------------------------
_string_len:
    mov rax, 0
.string_len_start:
    mov cl, [rdi]         ; load the byte at rdi
    cmp cl, 0             ; check if it's a null terminator
    je .string_len_end    ; end if it it
    add rax, 1            ; advance the counter
    add rdi, 1            ; advance the string pointer
    jmp .string_len_start
.string_len_end:
    ret