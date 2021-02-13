프로젝트 2에서 생성한 obj 파일을 linking loading 하는 과정을 추가했다.
또한 load 된 프로그램을 실행시켜 register값의 변화를 확인한다.

-컴파일 방법
소스코드와 Makefile이 있는 directory에서 make를 입력하여 컴파일 할 수 있다.
이후 생성되는 20161643.out 파일을 ./20161643.out을 통해서 실행한다. 
주어진 명령어를 입력하여 실행 결과를 화면에서 확인 할 수 있다. 

-삭제 방법
make clean으로 2016143.o, 20161643.out, .lst, intermediate 파일을 삭제할 수 있다.
obj 파일의 경우 삭제되지 않게 수정하였다. 