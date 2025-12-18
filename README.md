# 🎮 Rhythm-Fantasy
I2C, GPIO, LED, OLED를 활용하여 개발한 리듬게임 게임

# 📖 프로젝트 개요
이 프로젝트는 라즈베리파이 보드, SSD1306 OLED모듈과 GPIO 버튼, LED 등을 이용한 리듬 게임 컨트롤러를 설계하고, 
ncurses 화면을 통해 음악에 따라 내려오는 노트를 적절한 타이밍에 스위치를 눌러 없애는 기능을 구현한 시스템입니다. 
음악번호를 선택 후 게임을 실행하면, .WAV 형태로 디렉토리에 저장된 음악파일이 재생됩니다.
노트가 가로선에 걸칠 때 스위치를 누르면 해당 스위치의 led가 깜빡이고, 화면에는 led색깔의 점수가 표시되며, 
OLED에는 추가된 총 점수와 콤보 점수가 나타납니다.

# 🕹️ 주요 기능
- 게임에서 실행할 음악 번호를 선택해줍니다.
<img width="1167" height="774" alt="image" src="https://github.com/user-attachments/assets/e0be15cf-962b-4841-9643-c8f9caffa4dd" />

- 음악이 흘러나오면서 3초의 카운트 다운이 시작됩니다.
<img width="1134" height="774" alt="image" src="https://github.com/user-attachments/assets/e7c3983f-dbd4-4463-8490-7a2402bbd036" />

- 내려오는 '@'기호가 가로선에 닿을 때 스위치를 눌러줍니다.
- 스위치를 누르는 타이밍에 따라 Great, Good, Miss점수가 측정되며, 점수는 화면 오른쪽 상단에서 확인할 수 있습니다.
- Great, Good을 연속으로 맞추면 콤보 점수가 추가되며 콤보 점수는 연속 맞추기가 끝나면 총점에 추가되고 0으로 초기화됩니다. (총점과 콤보 점수는 화면 오른쪽 상단에 표시)
- 실행중인 노래 제목과 플레이어 이름은 화면 왼쪽 상단에 표시됩니다.
- 또한 '@'기호가 없어진 부분에는 해당 레인의 led색깔에 해당하는 점수기호가 잠깐 표시됩니다. (Great = 'A', Good = 'B', Miss = 'X')
<img width="1283" height="919" alt="image" src="https://github.com/user-attachments/assets/c93476aa-1cce-4b54-a7af-700615bbf4f0" />
<img width="1305" height="291" alt="image" src="https://github.com/user-attachments/assets/c2f9fd40-f9e4-40d2-be5c-5ee64278a486" />
<img width="1300" height="244" alt="image" src="https://github.com/user-attachments/assets/37950970-ff6f-4256-acbd-e9763d490590" />
<img width="1313" height="307" alt="image" src="https://github.com/user-attachments/assets/52c0315c-c117-4aa4-a723-16c5c6c8158e" />

- 스위치를 누를 때마다 각 스위치에 해당하는 led도 잠깐 깜빡거립니다. (빨강, 파랑, 노랑 순)
- OLED에는 총점과 콤보 점수가 실시간으로 반영되어 표시됩니다.
<img width="1104" height="820" alt="image" src="https://github.com/user-attachments/assets/ea8e9261-af85-4969-b3de-50cf7e892069" />

- 게임이 끝나면 총점, 최고 콤보 점수, 총 Great수, 총 Good수, 초 Miss수가 표시됩니다.
- 아무 키를 누르면 터미널 창으로 돌아갑니다.
<img width="1566" height="836" alt="image" src="https://github.com/user-attachments/assets/86f55fe2-91e5-4d28-9214-743f0b07de67" />


# 📁 프로젝트 구조


