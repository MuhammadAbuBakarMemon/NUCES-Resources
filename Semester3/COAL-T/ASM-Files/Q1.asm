INCLUDE Irvine32.inc

.data
Dividend    DWORD 0D4A4h
Divisor     DWORD 0Ah
Threshold   DWORD 5h
msgDiv      BYTE "Dividend: ",0
msgDvs      BYTE "Divisor:  ",0
msgQuo      BYTE "Quotient: ",0

.code
main PROC
mov edx, OFFSET msgDiv
call WriteString
mov eax, Dividend
call WriteHex
call Crlf

mov edx, OFFSET msgDvs
call WriteString
mov eax, Divisor
call WriteHex
call Crlf

mov eax, Dividend
mov ebx, Divisor
    
call CalcRecursiveDiv

mov edx, OFFSET msgQuo
call WriteString
call WriteHex
call Crlf

exit
main ENDP

CalcRecursiveDiv PROC
cmp eax, Threshold
jle BaseCase

sub eax, ebx
call CalcRecursiveDiv
    
inc eax
ret

BaseCase:
mov eax, 0
ret
CalcRecursiveDiv ENDP

END main