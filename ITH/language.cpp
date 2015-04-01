/*  Copyright (C) 2010-2012  kaosu (qiupf2000@gmail.com)
 *  This file is part of the Interactive Text Hooker.

 *  Interactive Text Hooker is free software: you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.

 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.

 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
const wchar_t* Warning=L"경고!";
//command.cpp
const wchar_t* ErrorSyntax=L"명령어 오류";
//inject.cpp
const wchar_t* ErrorRemoteThread=L"원격 스레드를 생성할 수 없음.";
const wchar_t* ErrorOpenProcess=L"프로세스를 열 수 없음.";
const wchar_t* ErrorNoProcess=L"프로세스를 찾을 수 없음";
const wchar_t* SelfAttach=L"ITH.exe에 부착하지 말아 주세요";
const wchar_t* AlreadyAttach=L"프로세스가 이미 부착됨.";
const wchar_t* FormatInject=L"프로세스 %d에 인젝션. 모듈 기반 %.8X";
//main.cpp
const wchar_t* NotAdmin=L"SeDebugPrevilege을 활성화 할 수 없습니다. ITH가 제대로 작동하지 못합니다.\r\n\
관리자 계정으로 실행하시거나 UAC를 끄시고 ITH를 실행해 주세요.";
//pipe.cpp
const wchar_t* ErrorCreatePipe=L"텍스트 파이프를 생성할 수 없거나, 요청이 너무 많습니다.";
const wchar_t* FormatDetach=L"프로세스 %d가 탈착됨.";
const wchar_t* ErrorCmdQueueFull=L"명령어 대기열이 가득참.";
const wchar_t* ErrorNoAttach=L"프로세스가 부착되지 않음.";

//profile.cpp
const wchar_t* ErrorMonitor=L"프로세스를 감시할 수 없음.";
//utility.cpp
const wchar_t* InitMessage=L"Copyright (C) 2010-2012  kaosu (qiupf2000@gmail.com)\r\n\
소스코드 <https://code.google.com/p/interactive-text-hooker/>\r\n\
일반토론 <https://groups.google.com/forum/?fromgroups#!forum/interactive-text-hooker>\r\n\
한글화 @mireado<https://twitter.com/mireado>";
const wchar_t* BackgroundMsg=L"도움말을 보시려면, \"help\", \"도움말\"이나 \"도움\"을 입력하세요.";
const wchar_t* ErrorLinkExist=L"연결이 존재함.";
const wchar_t* ErrorCylicLink=L"연결실패. 순환연결은 허용되지 않습니다.";
const wchar_t* FormatLink=L"출발스레드%.4x에서 도착스레드%.4x로 연결.";
const wchar_t* ErrorLink=L"연결실패. 출발/도착 스레드를 찾을 수 없음.";
const wchar_t* ErrorDeleteCombo=L"글상자에서 지우기 실패.";

//window.cpp
const wchar_t* ClassName=L"ITH";
const wchar_t* ClassNameAdmin=L"ITH (관리자)";
const wchar_t* ErrorNotSplit=L"먼저 문단 나누기를 활성화해주세요!";
const wchar_t* ErrorNotModule=L"먼저 모듈을 활성화해주세요!";
//Main window buttons
const wchar_t* ButtonTitleProcess=L"프로세스";
const wchar_t* ButtonTitleThread=L"스레드";
const wchar_t* ButtonTitleHook=L"후킹";
const wchar_t* ButtonTitleProfile=L"프로필";
const wchar_t* ButtonTitleOption=L"옵션";
const wchar_t* ButtonTitleClear=L"지우기";
const wchar_t* ButtonTitleSave=L"저장";
const wchar_t* ButtonTitleTop=L"항상위";
//Hook window
const wchar_t* SpecialHook=L"H코드 후킹, AGTH 코드는 지원하지 않습니다.";
//Process window
const wchar_t* TabTitlePID=L"PID";
const wchar_t* TabTitleMemory=L"메모리";
const wchar_t* TabTitleName=L"이름";
const wchar_t* TabTitleTID=L"TID";
const wchar_t* TabTitleStart=L"시작";
const wchar_t* TabTitleModule=L"모듈";
const wchar_t* TabTitleState=L"상태";
const wchar_t* SuccessAttach=L"프로세스에 ITH 부착성공.";
const wchar_t* FailAttach=L"프로세스에 ITH 부착실패.";
const wchar_t* SuccessDetach=L"프로세스에서 ITH 탈착성공.";
const wchar_t* FailDetach=L"ITH 탈착실패.";
//Profile window
const wchar_t* ProfileExist=L"프로필이 이미 존재함.";
const wchar_t* SuccessAddProfile=L"프로필 추가됨.";
const wchar_t* FailAddProfile=L"프로필 추가실패";
const wchar_t* TabTitleNumber=L"No.";
const wchar_t* NoFile=L"파일을 찾을 수 없음.";
const wchar_t* PathDismatch=L"프로세스 이름이 일치하지 않습니다, 계속하시겠습니까?";
const wchar_t* SuccessImportProfile=L"프로필 가져오기 성공";
//const wchar_t* SuccessAddProfile=L"Profile added.";
