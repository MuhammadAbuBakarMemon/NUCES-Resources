INCLUDE Irvine32.inc

FindFive PROTO :PTR SDWORD, :DWORD

.data
array1      SDWORD 5, 5, 5, 5, 5, 10, 20
array2      SDWORD 1, 2, 5, 5, 5, 5, 5, 3
array3      SDWORD 5, 5, 5, 0, 5, 5, 5
array4      SDWORD 1, 2, 3, 4, 6, 7, 8
    
strTest1    BYTE "Test 1 (Start):  ",0
strTest2    BYTE "Test 2 (Middle): ",0
strTest3    BYTE "Test 3 (Broken): ",0
strTest4    BYTE "Test 4 (None):   ",0
strPass     BYTE "Found sequence!",0
strFail     BYTE "No sequence found.",0

.code
main PROC
mov edx, OFFSET strTest1
call WriteString
INVOKE FindFive, ADDR array1, LENGTHOF array1
call ShowResult

mov edx, OFFSET strTest2
call WriteString
INVOKE FindFive, ADDR array2, LENGTHOF array2
call ShowResult

mov edx, OFFSET strTest3
call WriteString
INVOKE FindFive, ADDR array3, LENGTHOF array3
call ShowResult

mov edx, OFFSET strTest4
call WriteString
INVOKE FindFive, ADDR array4, LENGTHOF array4
call ShowResult

exit
main ENDP

ShowResult PROC
cmp eax, 1
je IsFound
mov edx, OFFSET strFail
jmp DoPrint
IsFound:
mov edx, OFFSET strPass
DoPrint:
call WriteString
call Crlf
call Crlf
ret
ShowResult ENDP

FindFive PROC USES esi ecx ebx,
ptrArray:PTR SDWORD,
arrSize:DWORD

mov esi, ptrArray
mov ecx, arrSize
mov ebx, 0

cmp ecx, 5
jb NotFound

ScanLoop:
mov eax, [esi]
cmp eax, 5
jne ResetCount

inc ebx
cmp ebx, 5
je Found
jmp NextIter

ResetCount:
mov ebx, 0

NextIter:
add esi, 4
loop ScanLoop

NotFound:
mov eax, 0
jmp TheEnd

Found:
mov eax, 1

TheEnd:
ret
FindFive ENDP

END main