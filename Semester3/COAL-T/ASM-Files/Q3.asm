INCLUDE Irvine32.inc

.data
myString    BYTE "###FAST",0
charToTrim  BYTE "#"
    
msgOriginal BYTE "Original string: ",0
msgTrimmed  BYTE "Trimmed string:  ",0

.code
main PROC
mov edx, OFFSET msgOriginal
call WriteString
mov edx, OFFSET myString
call WriteString
call Crlf

mov edx, OFFSET myString
mov al, charToTrim
call Str_trim_leading

mov edx, OFFSET msgTrimmed
call WriteString
mov edx, OFFSET myString
call WriteString
call Crlf

exit
main ENDP

Str_trim_leading PROC USES eax ebx esi edi
mov esi, edx
mov edi, edx
mov bl, al

ScanLoop:
mov al, [esi]
cmp al, 0
je StartCopy
cmp al, bl
jne StartCopy
inc esi
jmp ScanLoop

StartCopy:
    
CopyLoop:
mov al, [esi]
mov [edi], al
cmp al, 0
je Done
inc esi
inc edi
jmp CopyLoop

Done:
ret
Str_trim_leading ENDP

END main