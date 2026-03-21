INCLUDE Irvine32.inc

.data
buffer      BYTE 128 DUP(0)
byteCount   DWORD ?
cntA        DWORD 0
cntE        DWORD 0
cntI        DWORD 0
cntO        DWORD 0
cntU        DWORD 0
prompt      BYTE "Enter a string: ",0
strHeader   BYTE "Vowel Count",0
outA        BYTE "a or A = ",0
outE        BYTE "e or E = ",0
outI        BYTE " i or I = ",0
outO        BYTE " o or O = ",0
outU        BYTE " u or U = ",0
tabSpace    BYTE " ",0  

.code
main PROC

mov edx, OFFSET prompt
call WriteString

mov edx, OFFSET buffer
mov ecx, SIZEOF buffer
call ReadString
mov byteCount, eax

mov esi, OFFSET buffer
mov ecx, byteCount

cmp ecx, 0
je PrintResults

ParseLoop:
mov al, [esi]

cmp al, 'A'
je FoundA
cmp al, 'a'
je FoundA

cmp al, 'E'
je FoundE
cmp al, 'e'
je FoundE

cmp al, 'I'
je FoundI
cmp al, 'i'
je FoundI

cmp al, 'O'
je FoundO
cmp al, 'o'
je FoundO

cmp al, 'U'
je FoundU
cmp al, 'u'
je FoundU

jmp NextChar

FoundA:
inc cntA
jmp NextChar
FoundE:
inc cntE
jmp NextChar
FoundI:
inc cntI
jmp NextChar
FoundO:
inc cntO
jmp NextChar
FoundU:
inc cntU
jmp NextChar

NextChar:
inc esi
loop ParseLoop

PrintResults:

call Crlf
mov edx, OFFSET strHeader
call WriteString
call Crlf

mov edx, OFFSET outA
call WriteString
mov eax, cntA
call WriteDec

mov edx, OFFSET outI
call WriteString
mov eax, cntI
call WriteDec

mov edx, OFFSET outU
call WriteString
mov eax, cntU
call WriteDec
call Crlf

mov edx, OFFSET outE
call WriteString
mov eax, cntE
call WriteDec

mov edx, OFFSET outO
call WriteString
mov eax, cntO
call WriteDec
call Crlf

exit
main ENDP
END main