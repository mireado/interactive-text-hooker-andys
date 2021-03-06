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

#include <ITH\HookManager.h>
#include <ITH\version.h>
#include <ITH\IHF_SYS.h>
#include <ITH\ntdll.h>
#include <ITH\BitMap.h>
#include "language.h"
#include <emmintrin.h>
#include "common\const.h"

#define MAX_ENTRY 0x40

WCHAR user_entry[0x40];
static BYTE null_buffer[4]={0,0,0,0};
static BYTE static_small_buffer[0x100];
static DWORD zeros[4]={0,0,0,0};
HookManager* man;
//BitMap* pid_map;
DWORD clipboard_flag,split_time,repeat_count,global_filter,cyclic_remove;

LPWSTR HookNameInitTable[]={
	L"ConsoleOutput",
	L"GetTextExtentPoint32A",
	L"GetGlyphOutlineA",
	L"ExtTextOutA",
	L"TextOutA",
	L"GetCharABCWidthsA",
	L"DrawTextA",
	L"DrawTextExA",
	L"GetTextExtentPoint32W",
	L"GetGlyphOutlineW",
	L"ExtTextOutW",
	L"TextOutW",
	L"GetCharABCWidthsW",
	L"DrawTextW",
	L"DrawTextExW"
	};
LPVOID DefaultHookAddr[14];
DWORD GetHookName(LPWSTR str, DWORD pid, DWORD hook_addr, DWORD max)
{
	DWORD len = 0;
	max--; //for '\0' magic marker.
	if (pid==0) 
	{
		len = wcslen(HookNameInitTable[0]);
		if (len >= max) len = max;
		memcpy(str, HookNameInitTable[0], len << 1);
		str[len] = 0;
		return len;
	}
	//man->LockProcessHookman(pid);
	ProcessRecord* pr = man->GetProcessRecord(pid);
	if (pr == 0) return 0;
	NtWaitForSingleObject(pr->hookman_mutex,0,0);
	Hook* hks=(Hook*)pr->hookman_map;
	for (int i=0;i<MAX_HOOK;i++)
	{
		if (hks[i].Address()==hook_addr)
		{
			len = hks[i].NameLength();
			if (len >= max) len = max;
			NtReadVirtualMemory(pr->process_handle,hks[i].Name(),str,len << 1,&len);
			if (str[len - 1] == 0) len--;
			else str[len] = 0;
			break;
		}
	}
	NtReleaseMutant(pr->hookman_mutex,0);
	//man->UnlockProcessHookman(pid);
	return len;
}
int GetHookNameByIndex(LPWSTR str, DWORD pid, DWORD index)
{
	if (pid==0) 
	{
		wcscpy(str,HookNameInitTable[0]);
		return wcslen(HookNameInitTable[0]);
	}
	DWORD len=0;
	//man->LockProcessHookman(pid);
	ProcessRecord* pr = man->GetProcessRecord(pid);
	if (pr == 0) return 0;
	//NtWaitForSingleObject(pr->hookman_mutex,0,0); //already locked
	Hook* hks=(Hook*)pr->hookman_map;
	if (hks[index].Address())
	{
		NtReadVirtualMemory(pr->process_handle,hks[index].Name(),str,hks[index].NameLength()<<1,&len);
		len=hks[index].NameLength();
	}
	//NtReleaseMutant(pr->hookman_mutex,0);
	return len;
}
/*int GetHookString(LPWSTR str, DWORD pid, DWORD hook_addr, DWORD status)
{
	LPWSTR begin=str;
	str+=swprintf(str,L"%4d:0x%08X:",pid,hook_addr); 
	str+=GetHookName(str,pid,hook_addr);
	return str-begin;
}*/
bool GetProcessPath(HANDLE hProc, LPWSTR path)
{
	PROCESS_BASIC_INFORMATION info;
	LDR_DATA_TABLE_ENTRY entry;
	PEB_LDR_DATA ldr;
	PEB peb; 
	if (NT_SUCCESS(NtQueryInformationProcess(hProc, ProcessBasicInformation, &info, sizeof(info), 0)))
	if (info.PebBaseAddress)
	if (NT_SUCCESS(NtReadVirtualMemory(hProc, info.PebBaseAddress, &peb,sizeof(peb), 0)))
	if (NT_SUCCESS(NtReadVirtualMemory(hProc, peb.Ldr, &ldr, sizeof(ldr), 0)))
	if (NT_SUCCESS(NtReadVirtualMemory(hProc, (LPVOID)ldr.InLoadOrderModuleList.Flink,
		&entry, sizeof(LDR_DATA_TABLE_ENTRY), 0)))
	if (NT_SUCCESS(NtReadVirtualMemory(hProc, entry.FullDllName.Buffer,
		path, MAX_PATH * 2, 0))) return true;
	return false;
}
bool GetProcessPath(DWORD pid, LPWSTR path)
{
	CLIENT_ID id;
	OBJECT_ATTRIBUTES oa = {};
	HANDLE hProc; 
	NTSTATUS status;
	id.UniqueProcess = pid;
	id.UniqueThread = 0;
	oa.uLength = sizeof(oa);
	status = NtOpenProcess(&hProc , PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, &oa, &id);
	if (NT_SUCCESS(status))
	{
		bool flag = GetProcessPath(hProc, path);
		NtClose(hProc);
		return flag;
	}
	else return false;
};

void ThreadTable::SetThread(DWORD num, TextThread* ptr)
{
	int number=num;
	if (number>=size)
	{
		while (number>=size) size<<=1;
		TextThread** temp;
		if (size<0x10000)
		{
			temp=new TextThread*[size];
			memcpy(temp,storage,used*sizeof(TextThread*));
		}
		delete []storage;
		storage=temp;
	}
	storage[number]=ptr;
	if (ptr==0)
	{
		if (number==used-1) 
			while (storage[used-1]==0) used--;
	}
	else if (number>=used) used=number+1;
}
TextThread* ThreadTable::FindThread(DWORD number)
{
	if (number<=(DWORD)used)
		return storage[number];
	else return 0;
}

static const char sse_table_eq[0x100]={
	-1,1,-1,1, -1,1,-1,1, -1,1,-1,1, -1,1,-1,1, //0, compare 1
	-1,-1,1,1, -1,-1,1,1, -1,-1,1,1, -1,-1,1,1, //1, compare 2
	-1,1,-1,1, -1,1,-1,1, -1,1,-1,1, -1,1,-1,1, //0, compare 1
	-1,-1,-1,-1, 1,1,1,1, -1,-1,-1,-1, 1,1,1,1, //3, compare 3
	-1,1,-1,1, -1,1,-1,1, -1,1,-1,1, -1,1,-1,1, //0, compare 1
	-1,-1,1,1, -1,-1,1,1, -1,-1,1,1, -1,-1,1,1, //1, compare 2
	-1,1,-1,1, -1,1,-1,1, -1,1,-1,1, -1,1,-1,1, //0, compare 1
	-1,-1,-1,-1, -1,-1,-1,-1, 1,1,1,1, 1,1,1,1, //7, compare 4
	-1,1,-1,1, -1,1,-1,1, -1,1,-1,1, -1,1,-1,1, //0, compare 1
	-1,-1,1,1, -1,-1,1,1, -1,-1,1,1, -1,-1,1,1, //1, compare 2
	-1,1,-1,1, -1,1,-1,1, -1,1,-1,1, -1,1,-1,1, //0, compare 1
	-1,-1,-1,-1, 1,1,1,1, -1,-1,-1,-1, 1,1,1,1, //3, compare 3
	-1,1,-1,1, -1,1,-1,1, -1,1,-1,1, -1,1,-1,1, //0, compare 1
	-1,-1,1,1, -1,-1,1,1, -1,-1,1,1, -1,-1,1,1, //1, compare 2
	-1,1,-1,1, -1,1,-1,1, -1,1,-1,1, -1,1,-1,1, //0, compare 1
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0 //f, equal
};
char original_cmp(const ThreadParameter* t1, const ThreadParameter* t2)
{
	int t;
	t=t1->pid-t2->pid;
	if (t==0)
	{
		t=t1->hook-t2->hook;
		if (t==0)
		{
			t=t1->retn-t2->retn;
			if (t==0)
			{
				t=t1->spl-t2->spl;
				if (t==0) return 0;
				return t1->spl>t2->spl?1:-1;
			}
			else return t1->retn>t2->retn?1:-1;
		}
		else return t1->hook>t2->hook?1:-1;
	}
	else return t1->pid>t2->pid?1:-1;
	//return t>0?1:-1;
}
char TCmp::operator()(const ThreadParameter* t1, const ThreadParameter* t2)
	//SSE speed up. Compare four integers in const time without branching.
	//The AVL tree branching operation needs 2 bit of information.
	//One bit for equality and one bit for "less than" or "greater than".

{
	union{__m128 m0;__m128i i0;};
	union{__m128 m1;__m128i i1;};
	union{__m128 m2;__m128i i2;};
	int k0,k1;
	i1 = _mm_loadu_si128((const __m128i*)t1);
	i2 = _mm_loadu_si128((const __m128i*)t2);
	i0 = _mm_cmpgt_epi32(i1,i2);
	k0 = _mm_movemask_ps(m0);
	i1 = _mm_cmpeq_epi32(i1,i2);
	k1 = _mm_movemask_ps(m1);
	return sse_table_eq[k1*16+k0];
}
void TCpy::operator()(ThreadParameter* t1, const ThreadParameter* t2)
{
	memcpy(t1,t2,sizeof(ThreadParameter));
}
int TLen::operator()(const ThreadParameter* t) {return 0;}

#define NAMED_PIPE_DISCONNECT 1
//Class member of HookManger
HookManager::HookManager()
{
	TextThread* entry;
	head.key=new ThreadParameter;
	head.key->pid=0;
	head.key->hook=-1;
	head.key->retn=-1;
	head.key->spl=-1;
	head.data=0;
	create = 0;
	remove = 0;
	thread_table=new ThreadTable;
	entry=new TextThread(0, -1,-1,-1, new_thread_number++);
	thread_table->SetThread(0,entry);
	SetCurrent(entry);
	entry->Status()|=USING_UNICODE;
	entry->Status()|=USING_UTF8;
	//texts->SetUnicode(true);
	//entry->AddToCombo();
	//entry->ComboSelectCurrent();

	//if (background==0) entry->AddToStore((BYTE*)BackgroundMsg,wcslen(BackgroundMsg)<<1,0,1);

	InitializeCriticalSection(&hmcs);
	destroy_event=IthCreateEvent(0,0,0);
}
HookManager::~HookManager()
{
	//LARGE_INTEGER timeout={-1000*1000,-1};	
	//IthBreak();
	NtWaitForSingleObject(destroy_event,0,0);
	NtClose(destroy_event);
	NtClose(cmd_pipes[0]);
	NtClose(recv_threads[0]);
	delete thread_table;
	delete head.key;
	DeleteCriticalSection(&hmcs);
}
TextThread* HookManager::FindSingle(DWORD pid, DWORD hook, DWORD retn, DWORD split)
{
	if (pid==0) return thread_table->FindThread(0);
	ThreadParameter tp={pid,hook,retn,split};
	TreeNode<ThreadParameter*,DWORD> *node = Search(&tp);
	if (node == 0) return 0;
	else return thread_table->FindThread(node->data);
}
TextThread* HookManager::FindSingle(DWORD number)
{
	if (number&0x80008000) return 0;
	return thread_table->FindThread(number);
}

void HookManager::DetachProcess(DWORD pid)
{
}
void HookManager::SetCurrent(TextThread* it)
{
	if (current) 
		current->Status() &= ~CURRENT_SELECT;
	current=it;
	it->Status()|=CURRENT_SELECT;
}
void HookManager::SelectCurrent(DWORD num)
{
	TextThread* st = FindSingle(num);
	if (st)
	{
		SetCurrent(st);
		if (reset) reset(st);
		//st->ResetEditText();		
	}
}
void HookManager::RemoveSingleHook(DWORD pid, DWORD addr)
{
	EnterCriticalSection(&hmcs);
	DWORD max=thread_table->Used();
	TextThread* it;
	bool flag=false;
	DWORD number;
	for (DWORD i=1;i<=max;i++)
	{
		it=thread_table->FindThread(i);
		if (it)
		{
			if (it->PID()==pid&&it->Addr()==addr)
			{
				flag |= (it == current);
				//flag|=it->RemoveFromCombo();
				thread_table->SetThread(i,0);
				if (it->Number()<new_thread_number)
					new_thread_number=it->Number();				
				Delete(it->GetThreadParameter());
				if (remove) remove(it);
				delete it;
			}
		}
	}
	for (DWORD i=0;i<=max;i++)
	{
		it=thread_table->FindThread(i);
		if (it==0) continue;
		WORD ln=it->LinkNumber();
		if (it->Link() && thread_table->FindThread(ln)==0)
		{
			it->LinkNumber()=-1;
			it->Link()=0;
		}
	}
	if (flag)
	{
		current=0;
		if (head.Left)
			number=head.Left->data;
		else number=0;
		it = thread_table->FindThread(number);
		SetCurrent(it);
		if (reset) reset(it);
		//it->ResetEditText();
	}
	LeaveCriticalSection(&hmcs);
}
void HookManager::RemoveSingleThread(DWORD number)
{
	if (number==0) return;
	bool flag;
	EnterCriticalSection(&hmcs);
	TextThread* it=thread_table->FindThread(number);
	if (it)
	{
		thread_table->SetThread(number,0);
		Delete(it->GetThreadParameter());
		if (remove) remove(it);
		flag = (it == current);
		if (it->Number()<new_thread_number)
			new_thread_number=it->Number();
		delete it;
		for (int i=0;i<=thread_table->Used();i++)
		{
			it=thread_table->FindThread(i);
			if (it==0) continue;
			if (it->LinkNumber()==number)
			{
				it->Link()=0;
				it->LinkNumber()=-1;
			}
		}
		if (flag)
		{
			current=0;
			if (head.Left)
				number=head.Left->data;
			else number=0;

			SetCurrent(thread_table->FindThread(number));
			if (reset) reset(current);
			//it->ResetEditText();
		}
	}
	LeaveCriticalSection(&hmcs);
}
void HookManager::RemoveProcessContext(DWORD pid)
{
	TextThread* it;
	bool flag=false;
	DWORD ln;
	EnterCriticalSection(&hmcs);
	for (int i=1;i<thread_table->Used();i++)
	{
		it=thread_table->FindThread(i);
		if (it)
		{
			if (it->PID()==pid)
			{
				if (false==Delete(it->GetThreadParameter()))
					if (debug) __asm int 3
				flag |= (it == current);
				//flag|=it->RemoveFromCombo();
				if (it->Number()<new_thread_number)
					new_thread_number=it->Number();
				thread_table->SetThread(i,0);
				if (remove)	remove(it);
				delete it;
			}
		}
	}
	for (int i=0;i<thread_table->Used();i++)
	{
		it=thread_table->FindThread(i);
		if (it==0) continue;
		if (it->Link()==0) continue;
		ln=it->LinkNumber();
		if (thread_table->FindThread(ln)==0)
		{
			it->LinkNumber()=-1;
			it->Link()=0;
		}
	}
	if (flag)
	{
		current=0;
		if (head.Left)
			ln=head.Left->data;
		else ln=0;
		SetCurrent(thread_table->FindThread(ln));
		if (reset) reset(current);
		//if (it) it->ResetEditText();
	}
	LeaveCriticalSection(&hmcs);
}
void HookManager::RegisterThread(TextThread* it, DWORD num)
{
	thread_table->SetThread(num,it);
}
void HookManager::RegisterPipe(HANDLE text, HANDLE cmd, HANDLE thread)
{
	text_pipes[register_count]=text;
	cmd_pipes[register_count]=cmd;
	recv_threads[register_count]=thread;
	register_count++;
	if (register_count==1) NtSetEvent(destroy_event,0);
	else NtClearEvent(destroy_event);
}
void HookManager::RegisterProcess(DWORD pid, DWORD hookman, DWORD module, DWORD engine)
{
	WCHAR str[0x40],path[MAX_PATH];	
	//pid_map->Set(pid>>2);
	EnterCriticalSection(&hmcs);
	record[register_count-1].pid_register=pid;
	record[register_count-1].hookman_register=hookman;
	record[register_count-1].module_register=module;
	record[register_count-1].engine_register=engine;
	swprintf(str,L"ITH_SECTION_%d",pid);
	HANDLE hSection=IthCreateSection(str,0x2000,PAGE_READONLY);
	LPVOID map=0; 
	DWORD map_size=0x1000;
	NtMapViewOfSection(hSection,NtCurrentProcess(),&map,0,
		0x1000,0,&map_size,ViewUnmap,0,PAGE_READONLY);
	record[register_count-1].hookman_section=hSection;
	record[register_count-1].hookman_map=map;
	HANDLE hProc;	
	CLIENT_ID id;
	id.UniqueProcess=pid;
	id.UniqueThread=0;
	OBJECT_ATTRIBUTES oa={0};
	oa.uLength=sizeof(oa);
	if (NT_SUCCESS(NtOpenProcess(&hProc,
		PROCESS_QUERY_INFORMATION|
		PROCESS_CREATE_THREAD|
		PROCESS_VM_READ| 
		PROCESS_VM_WRITE|
		PROCESS_VM_OPERATION,
		&oa,&id))) record[register_count-1].process_handle=hProc;
	else
	{
		man->AddConsoleOutput(ErrorOpenProcess);
		return;
	}

	swprintf(str,L"ITH_HOOKMAN_%d",pid);
	record[register_count-1].hookman_mutex=IthOpenMutex(str);
	if (GetProcessPath(pid,path)==false) path[0]=0;
	swprintf(str,L"%.4d:%s",pid,wcsrchr(path,L'\\')+1);
	current_pid = pid;
	if (attach) attach(pid);
	LeaveCriticalSection(&hmcs);
}
void HookManager::UnRegisterProcess(DWORD pid)
{
	int i;
	EnterCriticalSection(&hmcs);

	

	for (i=0;i<MAX_REGISTER;i++) if(record[i].pid_register==pid) break;

	if (i<MAX_REGISTER)
	{
		NtClose(text_pipes[i]);
		NtClose(cmd_pipes[i]);
		NtClose(recv_threads[i]);
		NtClose(record[i].hookman_mutex);
		NtClose(record[i].process_handle);
		NtClose(record[i].hookman_section);
		NtUnmapViewOfSection(NtCurrentProcess(),record[i].hookman_map);
		for (;i<MAX_REGISTER;i++)
		{
			record[i]=record[i+1];
			text_pipes[i]=text_pipes[i+1];
			cmd_pipes[i]=cmd_pipes[i+1];
			recv_threads[i]=recv_threads[i+1];
			if (text_pipes[i]==0) break;
		}
		register_count--;
		if (current_pid == pid)
		{
			if (register_count) current_pid = record[0].pid_register;
			else current_pid = 0;
		}
		RemoveProcessContext(pid);

	}
	//pid_map->Clear(pid>>2);

	if (register_count==1) NtSetEvent(destroy_event,0);
	LeaveCriticalSection(&hmcs);
	if (detach) detach(pid);
}
void HookManager::SetName(DWORD type)
{
	WCHAR c;
	if (type&PRINT_DWORD) c=L'H';
	else if (type&USING_UNICODE) 
	{
		if (type&STRING_LAST_CHAR) c=L'L';
		else if (type&USING_STRING) c=L'Q';
		else c=L'W';
	}
	else
	{
		if (type&USING_STRING) c=L'S';
		else if (type&BIG_ENDIAN) c=L'A';
		else c=L'B';
	}
	swprintf(user_entry,L"UserHook%c",c);
}
void HookManager::AddLink(WORD from, WORD to)
{
	bool flag=false;
	TextThread *from_thread, *to_thread;
	EnterCriticalSection(&hmcs);
	from_thread=thread_table->FindThread(from);
	to_thread=thread_table->FindThread(to);
	if (to_thread&&from_thread)
	{
		if (from_thread->GetThreadParameter()->pid != to_thread->GetThreadParameter()->pid)
			AddConsoleOutput(L"Link to different process.");
		else if (from_thread->Link()==to_thread) 
			AddConsoleOutput(ErrorLinkExist);
		else if (to_thread->CheckCycle(from_thread))
			AddConsoleOutput(ErrorCylicLink);
		else
		{
			from_thread->Link()=to_thread;
			from_thread->LinkNumber()=to;
			WCHAR str[0x40];
			swprintf(str,FormatLink,from,to);
			AddConsoleOutput(str);
		}
	}
	else 
		AddConsoleOutput(ErrorLink);
	LeaveCriticalSection(&hmcs);
}
void HookManager::UnLink(WORD from)
{
	bool flag=false;
	TextThread *from_thread;
	EnterCriticalSection(&hmcs);
	from_thread=thread_table->FindThread(from);

	if (from_thread)
	{
		from_thread->Link() = 0;
		from_thread->LinkNumber() = 0xFFFF;
		AddConsoleOutput(L"Link deleted.");
	}
	else 
		AddConsoleOutput(L"Thread not exist.");
	LeaveCriticalSection(&hmcs);
}
void HookManager::UnLinkAll(WORD from)
{
	bool flag=false;
	TextThread *from_thread;
	EnterCriticalSection(&hmcs);
	from_thread=thread_table->FindThread(from);

	if (from_thread)
	{
		from_thread->UnLinkAll();
		AddConsoleOutput(L"Link deleted.");
	}
	else 
		AddConsoleOutput(L"Thread not exist.");
	LeaveCriticalSection(&hmcs);
}

void HookManager::DispatchText(DWORD pid, BYTE* text, DWORD hook, DWORD retn, DWORD spl, int len)
{
	bool flag=false;
	TextThread *it;
	DWORD number;
	if (text==0) return;
	if (len==0) return;
	EnterCriticalSection(&hmcs);
	ThreadParameter tp={pid,hook,retn,spl};
	TreeNode<ThreadParameter*,DWORD> *in;
	number=-1;
	if (pid)
	{
		in=Search(&tp);
		if (in) number=in->data;
	}
	else number=0;
	if (number!=-1)
	{
		it=thread_table->FindThread(number);
	}
	else
	{
		Insert(&tp,new_thread_number);
		it=new TextThread(pid, hook,retn,spl,new_thread_number);
		RegisterThread(it, new_thread_number);
		WCHAR entstr[0x200];
		it->GetEntryString(entstr);
		AddConsoleOutput(entstr);
		while (thread_table->FindThread(++new_thread_number));	
		if (create) create(it);
		
		
	}
	it->AddText(text,len,false,number==0);
	LeaveCriticalSection(&hmcs);	
}
void HookManager::AddConsoleOutput(LPCWSTR text)
{
	if (text)
	{
		int len=wcslen(text)<<1;
		TextThread *console=thread_table->FindThread(0);
		//EnterCriticalSection(&hmcs);
		console->AddText((BYTE*)text,len,false,true);
		console->AddText((BYTE*)L"\r\n",4,false,true);
		//LeaveCriticalSection(&hmcs);
	}
}
void HookManager::ClearText(DWORD pid, DWORD hook, DWORD retn, DWORD spl)
{
	bool flag=false;
	TextThread *it;
	EnterCriticalSection(&hmcs);
	ThreadParameter tp={pid,hook,retn,spl};
	TreeNode<ThreadParameter*,DWORD> *in;
	in=Search(&tp);
	if (in)
	{
		it=thread_table->FindThread(in->data);
		it->Reset();
		//SetCurrent(it);
		if (reset) reset(it);
		//it->ResetEditText();
	}
	LeaveCriticalSection(&hmcs);
}
void HookManager::ClearCurrent()
{
	EnterCriticalSection(&hmcs);
	current->Reset();
	if (reset) reset(current);
	//current->ResetEditText();

	LeaveCriticalSection(&hmcs);
}
void HookManager::ResetRepeatStatus()
{
	EnterCriticalSection(&hmcs);
	int i;
	TextThread* t;
	for (i=1;i<thread_table->Used();i++)
	{
		t=thread_table->FindThread(i);
		if (t==0) continue;
		t->ResetRepeatStatus();
	}
	LeaveCriticalSection(&hmcs);
}
void HookManager::LockHookman(){EnterCriticalSection(&hmcs);}
void HookManager::UnlockHookman(){LeaveCriticalSection(&hmcs);}
/*void HookManager::SetProcessEngineType(DWORD pid, DWORD type)
{
	int i;
	for (i=0;i<MAX_REGISTER;i++)
		if (record[i].pid_register==pid) break;
	if (i<MAX_REGISTER)
	{
		record[i].engine_register|=type;
	}
}*/
ProcessRecord* HookManager::GetProcessRecord(DWORD pid)
{
	int i;
	ProcessRecord *pr;
	EnterCriticalSection(&hmcs);
	for (i = 0; i < MAX_REGISTER; i++)
		if (record[i].pid_register == pid) break;
	pr = i < MAX_REGISTER ? record + i : 0;
	LeaveCriticalSection(&hmcs);
	return pr;
}
DWORD HookManager::GetProcessIDByPath(LPWSTR str)
{
	WCHAR path[MAX_PATH];
	for (int i=0;i<8&&record[i].process_handle;i++)
	{
		::GetProcessPath(record[i].process_handle,path);
		if (_wcsicmp(path,str)==0)
			return record[i].pid_register;
	}
	return 0;
}
DWORD HookManager::GetCurrentPID()
{
	return current_pid;
}
HANDLE HookManager::GetCmdHandleByPID(DWORD pid)
{
	int i;
	HANDLE h;
	EnterCriticalSection(&hmcs);
	for (i=0;i<MAX_REGISTER;i++)
		if (record[i].pid_register==pid) break;
	h = i<MAX_REGISTER?cmd_pipes[i]:0;
	LeaveCriticalSection(&hmcs);
	return h;
}

MK_BASIC_TYPE(DWORD)
MK_BASIC_TYPE(LPVOID)

DWORD Hash(LPWSTR module, int length)
{
	bool flag=(length==-1);
	DWORD hash=0;
	for (;*module&&(flag||length--);module++)
	{
		hash=((hash>>7)|(hash<<25))+(*module);
	}
	return hash;
}

DWORD	GetCurrentPID()
{
	return man->GetCurrentPID();
}
HANDLE	GetCmdHandleByPID(DWORD pid)
{
	return man->GetCmdHandleByPID(pid);
}

void GetCode(const HookParam& hp, LPWSTR buffer, DWORD pid)
{
	WCHAR c;
	LPWSTR ptr=buffer;
	if (hp.type&PRINT_DWORD) c=L'H';
	else if (hp.type&USING_UNICODE)
	{
		if (hp.type&USING_STRING) c=L'Q';
		else if (hp.type&STRING_LAST_CHAR) c=L'L';
		else c=L'W';
	}
	else
	{
		if (hp.type&USING_STRING) c=L'S';
		else if (hp.type&BIG_ENDIAN) c=L'A';
		else if (hp.type&STRING_LAST_CHAR) c=L'E';
		else c=L'B';
	}
	ptr+=swprintf(ptr,L"/H%c",c);
	if(hp.type&NO_CONTEXT) *ptr++=L'N';
	if (hp.off>>31) ptr+=swprintf(ptr,L"-%X",-(hp.off+4));
	else ptr+=swprintf(ptr,L"%X",hp.off);
	if (hp.type&DATA_INDIRECT)
	{
		if (hp.ind>>31) ptr+=swprintf(ptr,L"*-%X",-hp.ind);
		else ptr+=swprintf(ptr,L"*%X",hp.ind);
	}
	if (hp.type&USING_SPLIT)
	{
		if (hp.split>>31) ptr+=swprintf(ptr,L":-%X",-(4+hp.split));
		else ptr+=swprintf(ptr,L":%X",hp.split);
	}
	if (hp.type&SPLIT_INDIRECT)
	{
		if (hp.split_ind>>31) ptr+=swprintf(ptr,L"*-%X",-hp.split_ind);
		else ptr+=swprintf(ptr,L"*%X",hp.split_ind);
	}
	if (hp.module)
	{
		if (pid)
		{
			WCHAR path[MAX_PATH];
			MEMORY_BASIC_INFORMATION info;
			ProcessRecord* pr = man->GetProcessRecord(pid);
			if (pr)
			{
				HANDLE hProc=pr->process_handle;
				if (NT_SUCCESS(NtQueryVirtualMemory(hProc,(PVOID)hp.addr,MemorySectionName,path,MAX_PATH*2,0)))
				if (NT_SUCCESS(NtQueryVirtualMemory(hProc,(PVOID)hp.addr,MemoryBasicInformation,&info,sizeof(info),0)))
					ptr+=swprintf(ptr,L"@%X:%s",hp.addr-(DWORD)info.AllocationBase,wcsrchr(path,L'\\')+1);
			}

		}
		else
		{
			ptr+=swprintf(ptr,L"@%X!%X",hp.addr,hp.module);
			if (hp.function) ptr+=swprintf(ptr,L"!%X",hp.function);
		}
	}
	else
		ptr+=swprintf(ptr,L"@%X",hp.addr);

}
void AddLink(WORD from, WORD to) {man->AddLink(from,to);}
