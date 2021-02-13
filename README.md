# sicxe_machine_prj
Sogang University CSE4100 System Programming Project

## Project 1
<br>
SIC/XE 머신을 구현하기 위한 전 단계로 이 프로그램을 작성하였습니다. <br>
Shell의 기본적인 명령어들을 수행할 수 있으며, 이후 어셈블러, 링커, 로더를 실행 할 것입니다. <br>
컴파일을 통해 만들어진 object 코드가 적재되고, 실행될 메모리 공간과 mnemonic을 opcode 값으로 변환하는 opcode table을 구현하였습니다.
<hr/>

## Project 2
<br>
Project 1에서 구현한 SIC Machine을 바탕으로 *.asm 파일을 입력 받아 *.lst 파일과 *.obj 파일을 생성하는 프로그램을 만들었습니다. <br>
어셈블리 과정 중 pass 1과 pass 2 알고리즘을 이용하여 SYMTAB을 hash table 형태로 생성하였으며, 오류를 검출 할 수 있게 했습니다.<br>
또한 type 명령어를 이용하여 파일을 볼 수 있게 해주었으며, symbol 명령어도 구현하여 가장 최근에 assemble에 성공한 파일의 SYMTAB을 확인 할 수 있게 해주었습니다.
<hr/>

## Project 3
<br>
프로젝트 1과 2에서 구현한 shell에서 linking loading 기능을 추가한 프로그램이다. <br>
프로젝트2 에서 생성된 obj 파일을 link하여 프로젝트 1에서 구현한 메모리에 올리는 역할을 수행한다. <br>
작업을 수행하기 위해 2 pass를 이용하였다. 또한 load된 프로그램을 run할 수 있다
<hr/>
