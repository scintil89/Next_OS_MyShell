# NEXT INSTITUTE OS Project
## MyShell
### 131035 사현우

Step1 (~16/04/05)
>
- 사용자로부터 입력 받은 명령 라인을 분석하여 토큰별로 출력.
>
- 명령어 목록
	- ls : 디렉토리에 존재하는 파일 리스트를 출력
	- ll : 디렉토리에 존재하는 모든 파일에 대한 상세한 내용이 담긴 리스트를 출력
	- cp : 파일을 복사
	- rm : 파일을 제거 
	- move : 파일 위치를 이동
	- cd : 디렉토리를 변경
	- pwd : 현재 작업중인 디렉토리
	- mkdir : 디렉토리를 생성
	- rmdir : 디렉토리를 제거
	- cpdir : 디렉토리를 복사
	- exit, quit : MyShell을 종료
>
- 명령어 목록에 없는 내용을 입력할 경우 command not found를 출력.
>
>
>
Step2 (~16/04/26)
>
- Redirection 구현
>
>
>
Step3 (~16/05/12)
>
- 셸의 외부 명령어 실행 (프로세스의 생성과 프로그램 실행) 구현
>
- 백그라운드 실행 처리 (내장 명령어에 대해서는 지원하지 않음)
>
- 두 프로그램의 표준출력과 입력을 연결하는 파이프 통신 구현
>
>
>
Step4_Final (~16/05/26)
>
- 디렉터리 복사 (멀티 스레드 프로그래밍) 구현
>
- 소스 디렉터리의 여러 파일에 대해 각각의 스레드를 생성하여 병렬적으로 파일 복사를 실행
>
- 각 스레드의 동작을 표시하는 메시지 출력
>
