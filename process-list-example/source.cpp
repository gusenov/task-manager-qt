#define _WIN32_IE  0x0600
#define _WIN32_WINNT 0x0600


#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <TlHelp32.h>
#include  <psapi.h>
#include <stdio.h>


// ----------------------------------
#define ID_PRIORITY              100
#define ID_BTN_SETPR              101

//-----------------------------------

struct TaskInfo
{
       PROCESSENTRY32 proc;
       SIZE_T mem;
       DWORD priority;
     
 };
 //Список названий приоритетов процессов
 char *TextPriority[7]=
 {
 "Выше среднего", //0
 "Ниже среднего", //1
 "Средний",//2
 "Высокий",//3
 "Низкий",//4
 "Реального времени",//5
 "Не определен"//6
 };
 
 //массив значений классов приоритетов процесса
 DWORD priorList[6]=
 {
 ABOVE_NORMAL_PRIORITY_CLASS,
 BELOW_NORMAL_PRIORITY_CLASS,
 NORMAL_PRIORITY_CLASS,
 HIGH_PRIORITY_CLASS,
 IDLE_PRIORITY_CLASS,
 REALTIME_PRIORITY_CLASS
 };
 
 //Получить название приоритета процесса по значению
 char* GetTextOfPriority(DWORD p)
 {
 char *txt;
 
 switch(p)
 {
 	case ABOVE_NORMAL_PRIORITY_CLASS: txt=TextPriority[0]; break;
 	case BELOW_NORMAL_PRIORITY_CLASS:txt=TextPriority[1]; break;
 	case NORMAL_PRIORITY_CLASS: txt=TextPriority[2]; break;
 	
 	case HIGH_PRIORITY_CLASS: txt=TextPriority[3]; break;
 	case IDLE_PRIORITY_CLASS: txt=TextPriority[4]; break;
 	case REALTIME_PRIORITY_CLASS: txt=TextPriority[5]; break;
 	default: txt=TextPriority[6]; break;
}

return txt;

}

//-----------------------------

char szClassName[ ] = "ProcessList";
HWND hTaskListView;
HWND hWndMain;
HINSTANCE hInstance;
HMENU hMainMenu=NULL;
HWND hBtnRun;
HWND hBtnKillProc;
HWND hBtnStop;
HFONT listFont;
HFONT btnFont;  
HWND hWndComboBox;

CRITICAL_SECTION CrSectionUpd; 
int iSort=1;
HANDLE hUpdateThread=NULL;
TaskInfo tlist[300];
int numTask=0;
DWORD TimeOut=1000; // задержка между запросам списка процессов
int iMark=0;
const int  FooterHeight=0;
const int  LeftInfo=480;
int listRight=450;

//----------------------------------------------------------
LRESULT CALLBACK WindowProcedure (HWND, UINT, WPARAM, LPARAM);
DWORD WINAPI UpdateThread(LPVOID lpvParam);
BOOL StartUpdater(HWND hwnd);
//int GetCpuUsage(BOOL first);
void CALLBACK UpdateTimerProc(HWND hwnd,UINT uMsg,UINT_PTR idEvent,DWORD dwTime);
void EraseTaskList(void){ numTask=0; }
int GetCountTaskList(void){return numTask;}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//--------------------------------------------------
// Процедура добавления данных о процессе в список
//--------------------------------------------------
void AddTaskToList(PROCESSENTRY32 *p,SIZE_T mem,DWORD pri)
{
     if(numTask >=300 ) return;
     
     tlist[numTask].proc=*p;
     tlist[numTask].mem=mem;
     tlist[numTask].priority=pri;
     numTask++;
}

//--------------------------------------------------------
// Четыре функции compareTask, используемые при сортировке 
// данных внутри окна списка процессов
//--------------------------------------------------------
 int compareTask0 (const void * a, const void * b)
{
    TaskInfo *t1,*t2;
    
    t1=(TaskInfo*)a;
    t2=(TaskInfo*)b;
    char s1[256],s2[256];
    strcpy(s1,t1->proc.szExeFile);
    strcpy(s2,t2->proc.szExeFile);
     int     i=0;
    while(s1[i]!=0){toupper (s1[i]); i++;}
        i=0;
    while(s2[i]!=0){toupper (s2[i]); i++;}
    

   if(iSort > 0) return ( strcmp(t1->proc.szExeFile,t2->proc.szExeFile) );
   else return ( strcmp(t2->proc.szExeFile,t1->proc.szExeFile) );
}
// Сортировка по ID процесса
 int compareTask1 (const void * a, const void * b)
{
    TaskInfo *t1,*t2;
    
    t1=(TaskInfo*)a;
    t2=(TaskInfo*)b;
    if(iSort > 0) return(    t1->proc.th32ProcessID - t2->proc.th32ProcessID);
    else return(    t2->proc.th32ProcessID - t1->proc.th32ProcessID);

}
// Сортировка по объему памяти
 int compareTask2 (const void * a, const void * b)
{
    TaskInfo *t1,*t2;
    
    t1=(TaskInfo*)a;
    t2=(TaskInfo*)b;
    if(iSort > 0)return(    t1->mem - t2->mem );
    else return(    t2->mem - t1->mem );
 
}
// Сортировка по числу потоков
 int compareTask3 (const void * a, const void * b)
{
    TaskInfo *t1,*t2;
    
    t1=(TaskInfo*)a;
    t2=(TaskInfo*)b;
  if(iSort > 0)  return ( t1->proc.cntThreads - t2->proc.cntThreads );
  else return ( t2->proc.cntThreads - t1->proc.cntThreads );
}


//----------------------------------------------------
// Процедура получения данных о запущенных процессах
// от операционной системы
//----------------------------------------------------
BOOL GetProcessList()
{
    HANDLE hSnapShot;
    PROCESSENTRY32 ProcEntry;
    PROCESS_MEMORY_COUNTERS mc;
    SIZE_T sz;
    DWORD errorCode;
    BOOL bResult;
    
    hSnapShot=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
    if(hSnapShot==INVALID_HANDLE_VALUE)
    {
         errorCode=GetLastError();
       
         return FALSE;
    }
    ProcEntry.dwSize=sizeof(PROCESSENTRY32);
    
    bResult=Process32First(hSnapShot, &ProcEntry);
    if(bResult==FALSE)
    {
         errorCode=GetLastError();

         CloseHandle(hSnapShot);
         return FALSE;
    }
    
    while(bResult==TRUE)
    {
                        
    HANDLE hProc;
    
 	hProc=OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ,FALSE,ProcEntry.th32ProcessID);
	
	if( hProc!=NULL )
    {
        
    sz=0;
    mc.cb=sizeof(mc);
   
    if( GetProcessMemoryInfo( hProc, &mc, sizeof(mc)) != 0 )
    {
         sz=mc.WorkingSetSize/1024;
     }
    
      DWORD pri=GetPriorityClass(hProc);
      
     AddTaskToList(&ProcEntry,sz,pri);
   
    
   CloseHandle(hProc);
}//endif

     bResult =  Process32Next(hSnapShot, &ProcEntry);      
	            
    }
    CloseHandle(hSnapShot);
    
    
 if(iSort==1||iSort==-1) qsort (tlist, numTask, sizeof(TaskInfo), compareTask0);
 if(iSort==2||iSort==-2) qsort (tlist, numTask, sizeof(TaskInfo), compareTask1);
 if(iSort==3||iSort==-3) qsort (tlist, numTask, sizeof(TaskInfo), compareTask2);
 if(iSort==4||iSort==-4) qsort (tlist, numTask, sizeof(TaskInfo), compareTask3);
 
return true;
}



//----------------------------------------
// Процедура запуска формирования списка процедур 
// с новыми данными
//----------------------------------------
void LoadTaskList()
{
     
   iMark=ListView_GetSelectionMark(hTaskListView);
     
   EraseTaskList();
   GetProcessList();
   ListView_SetItemCount(hTaskListView,GetCountTaskList());
   ListView_SetSelectionMark(hTaskListView,iMark);

}

//Отображение элементов списка 
TCHAR text[512];  
//-----------------------------------------------------------
// Процедура отрабатывающая запрос окна-списка на получение
// данных для отображения 
//
//----------------------------------------------------------
void OnGetDisplayInfoList(LV_DISPINFO *pInfo)
{
     
     LV_ITEM* pItem= &(pInfo)->item;
     if(pItem->mask & LVIF_TEXT)
     {
       int i=  pItem->iItem;
    
      
        if( pItem->iSubItem == 0 )  wsprintf(text,TEXT("%s"),tlist[i].proc.szExeFile); 
        else   if( pItem->iSubItem == 1 )  wsprintf(text,TEXT("%0d"),tlist[i].proc.th32ProcessID);  
       // else  if( pItem->iSubItem == 2 )  wsprintf(text,TEXT("%d KB"),tlist[i].mem); 
       // else if( pItem->iSubItem == 3 )  wsprintf(text,TEXT("%d"),tlist[i].proc.cntThreads); 
        else if( pItem->iSubItem == 2 )  wsprintf(text,TEXT("%s"),GetTextOfPriority(tlist[i].priority)); 
               else lstrcpyn(text,TEXT("none"),5); 
  
         lstrcpyn(pItem->pszText, text, pItem->cchTextMax);
      }
}

 
//-----------------------------------------------------------
// Процедура создания шрифтов, используемых в окне программы
//
//----------------------------------------------------------
void InitFont()
{
 	LOGFONT logfont;
    
	logfont.lfHeight         =  15;
    logfont.lfWidth          =  0;
    logfont.lfEscapement     =  0;
    logfont.lfOrientation    =  0;
    logfont.lfWeight         =  FW_NORMAL;//FW_BOLD;//
    logfont.lfItalic         =  FALSE;
    logfont.lfUnderline      =  FALSE;
    logfont.lfStrikeOut      =  FALSE;
    logfont.lfCharSet        =  RUSSIAN_CHARSET;
    logfont.lfOutPrecision   =  OUT_DEFAULT_PRECIS;
    logfont.lfClipPrecision  =  CLIP_DEFAULT_PRECIS;
    logfont.lfQuality        =  DEFAULT_QUALITY;
    logfont.lfPitchAndFamily =  FIXED_PITCH | FF_DONTCARE;
    strcpy((char *)logfont.lfFaceName, "Tahoma");

	listFont=CreateFontIndirect(&logfont);
	// strcpy((char *)logfont.lfFaceName, "Arial");
    btnFont=CreateFontIndirect(&logfont);


     }    
     
//-------------------------------------------------------------
// Процедура обработки события создания основного окна
// создаются все дочерние элементы главного окна:
// Список и кнопки
//-------------------------------------------------------------
void  OnCreateMainWindow(HWND hwnd,WPARAM wParam,LPARAM lParam)
{

     InitCommonControls();
     InitFont();
     
     RECT rct;
     GetClientRect(hwnd,&rct);
     
     int listBottom=rct.bottom-FooterHeight;


    	// Выпадающий список - выбора логических дисков
    hWndComboBox = CreateWindow(WC_COMBOBOX, TEXT(""), 
     CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_CHILD | WS_OVERLAPPED | WS_VISIBLE,
     LeftInfo,290, 150,125, hwnd, (HMENU)ID_PRIORITY, hInstance, NULL);


    for(int i=0;i<6;i++)
    {
    	//if(szCurrentDirPath[0]==drvList[i].name[0]) ic=i;
    	
       ComboBox_AddString(hWndComboBox,TextPriority[i]);
    }   
 
    SetWindowFont(hWndComboBox,btnFont,true);

     //----------------------------------------------------------------
//      hBtnRun=CreateWindowEx(0,WC_BUTTON ,"Новая задача...",
 //    WS_CHILD|WS_BORDER|WS_VISIBLE|WS_TABSTOP,
 //    LeftInfo,listBottom-95,150,25,hwnd,(HMENU)ID_BTN_RUN,hInstance,NULL);

   //  SetWindowFont(hBtnRun,btnFont,true);
 
     //----------------------------------------------------------------
      hBtnKillProc=CreateWindowEx(0,WC_BUTTON ,"Задать приоритет",
     WS_CHILD|WS_BORDER|WS_VISIBLE|WS_TABSTOP,
     LeftInfo,330,150,25,hwnd,(HMENU)ID_BTN_SETPR,hInstance,NULL);

     SetWindowFont(hBtnKillProc,btnFont,true);

    
     //------------------------------------------------------------ 
     hTaskListView=CreateWindowEx(0,WC_LISTVIEW ,"",
     WS_CHILD|WS_BORDER|WS_VISIBLE|WS_TABSTOP |LVS_OWNERDATA|LVS_REPORT|LVS_SINGLESEL|LVS_SHOWSELALWAYS ,
     0,0,listRight,listBottom,hwnd,NULL,hInstance,NULL);
     
     if(hTaskListView==NULL) MessageBox(hwnd,"Err","Err",MB_OK);
     
     ListView_SetExtendedListViewStyle(hTaskListView,LVS_EX_ONECLICKACTIVATE | LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES);
     ShowWindow(hTaskListView,SW_SHOW);
     
     LVCOLUMN lvcol;
     lvcol.mask=LVCF_TEXT|LVCF_WIDTH|LVCF_FMT;
     lvcol.fmt=LVCFMT_CENTER;

     
     lvcol.cx=200;
      lvcol.pszText="Имя образа";
     ListView_InsertColumn(hTaskListView,0,&lvcol);
     
      lvcol.pszText="ID";
      lvcol.cx=50;
      ListView_InsertColumn(hTaskListView,1,&lvcol);
     

    
      lvcol.pszText="Приоритет";
      lvcol.cx=120;
       ListView_InsertColumn(hTaskListView,3,&lvcol);
     
//==========================================================
     ListView_SetItemCount(hTaskListView,GetCountTaskList());
 
     InitializeCriticalSection(&CrSectionUpd) ;
      
    
  
     StartUpdater(hwnd); // запуск потока обновления списка процессов
   
}

//-------------------------------------------------------------
// Процедура отрисовки в основном окне программы
//
//-------------------------------------------------------------
void OnPaintMainWindow(HWND hwnd,WPARAM wParam,LPARAM lParam) 
{

HDC hdc;/* A device context used for drawing */
PAINTSTRUCT ps;/* Also used during window drawing */
RECT rc;/* A rectangle used during drawing */
RECT rcText;
    
hdc = BeginPaint (hwnd, &ps);
GetClientRect (hwnd, &rc);

rcText=rc;
COLORREF clr_back=GetSysColor(COLOR_WINDOW);
COLORREF clr_txt;
LOGBRUSH logbr;


logbr.lbStyle=BS_SOLID; 
logbr.lbColor=GetSysColor(COLOR_WINDOW);

HBRUSH brBk=CreateBrushIndirect(&logbr);
SetBkColor(hdc,clr_back);
//	SetTextColor(hdc,clr_txt);
		
//SelectObject(hdc,brBk);
//rcText.left=LeftInfo;
//rcText.top=rc.top+25;
//DrawText (hdc, TEXT("ИНФОРМАЦИЯ"), -1, &rcText,DT_SINGLELINE|DT_LEFT );//| DT_CENTER | DT_VCENTER);

     
EndPaint (hwnd, &ps);  
}




//-------------------------------------------------------------
// Процедура запуска отдельного потока циклического обновления
// списка запущенных процессов
//-------------------------------------------------------------
BOOL StartUpdater(HWND hwnd)
{
DWORD     dwThreadId;

 HANDLE             hThread = CreateThread( 
            NULL,              // no security attribute 
            0,                 // default stack size 
            UpdateThread,    // thread proc
            (LPVOID) hwnd,    // thread parameter 
            0,                 // not suspended 
            &dwThreadId);      // returns thread ID 

         if (hThread == NULL) 
         {
         // поток  не запущен
            MessageBox(NULL,"CreateThread failed.","Ошибка",MB_OK);  
            return FALSE;
         }
         else CloseHandle(hThread); 
}

//-------------------------------------------------------------
// Процедура-Поток , обновляющая в цикле с задержкой 
// список запущенных процессов
//
//-------------------------------------------------------------
DWORD WINAPI UpdateThread(LPVOID lpvParam)
{
   
    HWND     hwnd; 
    hwnd=(HWND) lpvParam;
    BOOL bUpdaterRun;
    
    bUpdaterRun=TRUE;
    
   while( bUpdaterRun )
   { 
        
         EnterCriticalSection(&CrSectionUpd); 
         
    
       
        LoadTaskList();//формирование списка процессов

//==========Вывод данных в главное окно=======================

HDC hdc = GetDC (hWndMain);
char txt[256];
RECT rc;/* A rectangle used during drawing */

RECT rcText;
GetClientRect (hwnd, &rc);

//SelectObject(hdc,btnFont);
 RECT rr;
     rr.left=listRight;
     rr.right=rc.right;
     rr.top=rc.top+60;
     rr.bottom=270;
     
     FillRect(hdc,&rr,(HBRUSH)GetStockObject(WHITE_BRUSH));
     
     
    SelectObject(hdc,listFont);
    rcText=rc;
     
     
     wsprintf(txt,TEXT("Приоритет процесса:"));
     //wsprintf(txt,"Процессов: %ld",numTask);
     rcText.left=LeftInfo;
     rcText.top=rc.top+210;
     DrawText (hdc, txt, -1, &rcText,DT_SINGLELINE|DT_LEFT );
     
       int i=ListView_GetSelectionMark(hTaskListView);
     
     if( i == -1 )  wsprintf(txt,TEXT("Не выбран в списке"));
     else 
     {
          //HANDLE    hProcess;
          //DWORD id;
         // tlist[i].proc.th32ProcessID;
 
 
        wsprintf(txt,"%s",tlist[i].proc.szExeFile);
 
     
	   }
     
	 rcText.left=LeftInfo;
     rcText.top=rc.top+230;
     DrawText (hdc, txt, -1, &rcText,DT_SINGLELINE|DT_LEFT );

   
     
//================================================
    ReleaseDC (hwnd, hdc);

  
    InvalidateRect(hTaskListView,NULL,FALSE);
    UpdateWindow(hTaskListView);
    
    LeaveCriticalSection(&CrSectionUpd);  
  
  Sleep(TimeOut);
  
}// end while

}

//-------------------------------------------------------------
// Процедура обработки команды задания нового приоритета процесса
//
//-------------------------------------------------------------    
void OnClickBnt_SET()
{
     char txt[256];
     
    EnterCriticalSection(&CrSectionUpd); 
    
        
     int i=ListView_GetSelectionMark(hTaskListView);
     
     if( i == -1 )   MessageBox(hWndMain,"Не выбран процесс","Info",MB_OK|MB_ICONINFORMATION);
     else 
     {
     	
     //
         
         DWORD newPriority;
            
		 int ind=ComboBox_GetCurSel(hWndComboBox);
        
		  if( ind==CB_ERR ){ MessageBox(hWndMain,"Не выбран приоритет.","Info",MB_OK|MB_ICONINFORMATION); return;}
             
          newPriority=priorList[ind]; //новый приоритет

		  HANDLE    hProcess;
          DWORD id;
          id=tlist[i].proc.th32ProcessID;

                     	
          hProcess = OpenProcess(PROCESS_SET_INFORMATION , FALSE, id);
          
          if(hProcess!=NULL)
          {
             if( SetPriorityClass( hProcess,newPriority )==0)
             {
                  int err=GetLastError();
                  wsprintf(txt,"Неудачное изменение приоритета процесса %s.Код ошибки %d(%0x)",tlist[i].proc.szExeFile,err,err);
                  MessageBox(hWndMain,txt,"Ошибка",MB_ICONERROR);
             }
          
              CloseHandle(hProcess);
          }
          else  {
                  int err=GetLastError();
             
                  wsprintf(txt,"Нет доступа к процессу  %s. Код ошибки %d(%0x)",tlist[i].proc.szExeFile,err,err);
                  MessageBox(hWndMain,txt,"Ошибка",MB_ICONERROR);
               
                }
                
     }
  
     LeaveCriticalSection(&CrSectionUpd);  
    
}


//-------------------------------------------------------------
// Процедура обработки событий от кнопок главного окна
//
//-------------------------------------------------------------
void OnCommandMainWindow(HWND hwnd,WPARAM wParam,LPARAM lParam)
{
   
     if( LOWORD(wParam)== ID_BTN_SETPR )//завершение приложения
     {
         OnClickBnt_SET();
     }
      
}


//-------------------------------------------------------------
// Обработка события нажатия на заголовок
// колонки в списке процессов
//-------------------------------------------------------------
 void OnColumnClick(NM_LISTVIEW *pnm)
 {
 int i=pnm->iSubItem+1;
      if(iSort==i) iSort=-iSort;
      else iSort=i;

 }
 
//-------------------------------------------------------------
// Обработчик событий в главном окне программы
//
//-------------------------------------------------------------
LRESULT CALLBACK WindowProcedure (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)                  /* handle the messages */
    {     case WM_NOTIFY: 
          {
               if( ((LPNMHDR)lParam)->code == LVN_GETDISPINFO )
               {  OnGetDisplayInfoList((LV_DISPINFO*) lParam); }
               else
               if(((LPNMHDR)lParam)->code == LVN_COLUMNCLICK )
               {
               //                          
                    OnColumnClick((NM_LISTVIEW *) lParam);   
                                   
               }
           break;
          }
          case WM_SETFOCUS: SetFocus(hTaskListView); break;
           case WM_CREATE: OnCreateMainWindow(hwnd,wParam,lParam); break;
           case WM_PAINT:  OnPaintMainWindow(hwnd,wParam,lParam); break;
           case WM_COMMAND: OnCommandMainWindow(hwnd,wParam,lParam); break;
        case WM_DESTROY:
              DeleteCriticalSection(&CrSectionUpd);
            PostQuitMessage (0);       /* send a WM_QUIT to the message queue */
            break;
        default:                      /* for messages that we don't deal with */
            return DefWindowProc (hwnd, message, wParam, lParam);
    }

    return 0;
}

//-------------------------------------------------------------
//
//
//-------------------------------------------------------------
int WINAPI WinMain (HINSTANCE hThisInstance,
                    HINSTANCE hPrevInstance,
                    LPSTR lpszArgument,
                    int nFunsterStil)

{
    HWND hwnd;               /* This is the handle for our window */
    MSG messages;            /* Here messages to the application are saved */
    WNDCLASSEX wincl;        /* Data structure for the windowclass */

    hInstance=hThisInstance;

    /* The Window structure */
    wincl.hInstance = hThisInstance;
    wincl.lpszClassName = szClassName;
    wincl.lpfnWndProc = WindowProcedure;      /* This function is called by windows */
    wincl.style = CS_DBLCLKS;                 /* Catch double-clicks */
    wincl.cbSize = sizeof (WNDCLASSEX);

    /* Use default icon and mouse-pointer */
    wincl.hIcon = LoadIcon (NULL, IDI_APPLICATION);
    wincl.hIconSm = LoadIcon (NULL, IDI_APPLICATION);
    wincl.hCursor = LoadCursor (NULL, IDC_ARROW);
    wincl.lpszMenuName = NULL;                 /* No menu */
    wincl.cbClsExtra = 0;                      /* No extra bytes after the window class */
    wincl.cbWndExtra = 0;                      /* structure or the window instance */
    /* Use Windows's default color as the background of the window */
    wincl.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);

    /* Register the window class, and if it fails quit the program */
    if (!RegisterClassEx (&wincl))
        return 0;

    /* The class is registered, let's create the program*/
    hWndMain=hwnd = CreateWindowEx (
           0,                   /* Extended possibilites for variation */
           szClassName,         /* Classname */
           "Список процессов",       /* Title Text */
           WS_OVERLAPPED| WS_CAPTION| WS_SYSMENU |WS_MINIMIZEBOX,//WS_OVERLAPPEDWINDOW, /* default window */
           CW_USEDEFAULT,       /* Windows decides the position */
           CW_USEDEFAULT,       /* where the window ends up on the screen */
           700,                 /* The programs width */
           500,                 /* and height in pixels */
           HWND_DESKTOP,        /* The window is a child-window to desktop */
           NULL,                /* No menu */
           hThisInstance,       /* Program Instance handler */
           NULL                 /* No Window Creation data */
           );

    /* Make the window visible on the screen */
    ShowWindow (hwnd, nFunsterStil);

    /* Run the message loop. It will run until GetMessage() returns 0 */
    while (GetMessage (&messages, NULL, 0, 0))
    {
        /* Translate virtual-key messages into character messages */
        TranslateMessage(&messages);
        /* Send message to WindowProcedure */
        DispatchMessage(&messages);
    }

    /* The program return-value is 0 - The value that PostQuitMessage() gave */
    return messages.wParam;
}
