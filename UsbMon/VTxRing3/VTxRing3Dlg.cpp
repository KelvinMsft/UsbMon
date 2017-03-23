
// VTxRing3Dlg.cpp : implementation file
//

#include "stdafx.h"
#include "VTxRing3.h"
#include "VTxRing3Dlg.h"
#include "afxdialogex.h"
#include "cDrvCtrl.h"
#include "IOCTL.h"	
#include "tlhelp32.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include <winternl.h>

#pragma comment(lib,"ntdll.lib") // Need to link with ntdll.lib import library. You can find the ntdll.lib from the Windows DDK.

typedef struct _TRANSFER_IOCTL
{
	ULONG64 ProcID;
	ULONG64 HiddenType;
	ULONG64 Address;
}TRANSFERIOCTL, *PTRANSFERIOCTL;
// CAboutDlg dialog used for App About

#define FILE_DEVICE_HIDE	0x8000

#define IOCTL_BASE	0x800

#define CTL_CODE_HIDE(i)	\
	CTL_CODE(FILE_DEVICE_HIDE, IOCTL_BASE+i, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_USB_MAPPING_START		CTL_CODE_HIDE(1)			//初始化
#define IOCTL_USB_MAPPING_STOP		CTL_CODE_HIDE(2)			//初始化
#define IOCTL_HIDE_STOP				CTL_CODE_HIDE(3)			//初始化

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CVTxRing3Dlg dialog



CVTxRing3Dlg::CVTxRing3Dlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_VTXRING3_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CVTxRing3Dlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CVTxRing3Dlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDOK, &CVTxRing3Dlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CVTxRing3Dlg::OnBnClickedCancel)
	ON_BN_CLICKED(IDC_BUTTON1, &CVTxRing3Dlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON2, &CVTxRing3Dlg::OnBnClickedButton2)
END_MESSAGE_MAP()


// CVTxRing3Dlg message handlers

BOOL CVTxRing3Dlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CVTxRing3Dlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CVTxRing3Dlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CVTxRing3Dlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}
DWORD FindProcessId(WCHAR*processname)
{
	HANDLE hProcessSnap;
	PROCESSENTRY32 pe32;
	DWORD result = NULL;

	// Take a snapshot of all processes in the system.
	hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (INVALID_HANDLE_VALUE == hProcessSnap) return(FALSE);

	pe32.dwSize = sizeof(PROCESSENTRY32); // <----- IMPORTANT

										  // Retrieve information about the first process,
										  // and exit if unsuccessful
	if (!Process32First(hProcessSnap, &pe32))
	{
		CloseHandle(hProcessSnap);          // clean the snapshot object
	//	OutputDebugStringA("!!! Failed to gather information on system processes! \n");
		return(NULL);
	}

	do
	{
//		OutputDebugStringA("Checking process\n");
		if (0 == _wcsicmp(processname, pe32.szExeFile))
		{
			result = pe32.th32ProcessID;
			break;
		}
	} while (Process32Next(hProcessSnap, &pe32));

	CloseHandle(hProcessSnap);

	return result;
}
cDrvCtrl drv;
HANDLE g_hEvent;


#define DRV_PATH		"C:\\UsbMon.sys"
#define SERVICE_NAME	"UsbMontest5"
#define DISPLAY_NAME	SERVICE_NAME

 

typedef struct _USER_MOUDATA
{
	UCHAR x;
	UCHAR y;
	UCHAR z;
	UCHAR Click;
	BOOLEAN IsAbsolute;
}USERMOUDATA, *PUSERDATA;
BOOLEAN bStart = TRUE;
//-------------------------------------------------------------------------------------------//
PVOID WINAPI UsbReadThreadStart(PVOID params)
{
	USERMOUDATA* data = (USERMOUDATA*)params;
	while (bStart)
	{
		WaitForSingleObject(g_hEvent, INFINITE);
		if (data->x != 0 || data->y != 0 | data->z != 0 || data->Click != 0)
		{
			CString msg;
			msg.Format(L"x: %x, y: %x z: %x click: %x \r\n", data->x, data->y, data->z, data->Click, data->IsAbsolute);
			OutputDebugString(msg);
		}
	}
	return NULL;
}
//---------------------------------------------------------------------------------------//
void CVTxRing3Dlg::OnBnClickedOk()
{ 
	ULONG64	OutBuffer;
	DWORD	RetBytes;  
	g_hEvent = CreateEvent(NULL, FALSE, FALSE, NULL); // 创建事件
  
	CString err;
  
	//Create Service
	if (!drv.Install(DRV_PATH, SERVICE_NAME, DISPLAY_NAME))
	{
		OutputDebugStringA("Change Page A222\r\n");
		CloseHandle(g_hEvent);
		return;
	} 

	//Start Service
	if (!drv.Start(SERVICE_NAME))
	{
		OutputDebugStringA("Change Page 333Attribute to Writable \r\n");

		drv.Remove(SERVICE_NAME);
		CloseHandle(g_hEvent);
		return;
	}
 
	if (!drv.IoControl("\\\\.\\UsbMon", IOCTL_USB_MAPPING_START, g_hEvent, sizeof(HANDLE), &OutBuffer, sizeof(ULONG64), &RetBytes))
	{
		drv.Stop(SERVICE_NAME);
		drv.Remove(SERVICE_NAME);
		CloseHandle(g_hEvent);
		err.Format(L"Cannot IOCTL device : %x \r\n", GetLastError());
		AfxMessageBox(err);
		return;
	}

	err.Format(L"Out: %I64x \r\n", OutBuffer);
	OutputDebugString(err); 

	if (OutBuffer)
	{
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)UsbReadThreadStart, (PVOID)OutBuffer, 0, 0);
	}
	OutputDebugStringA("Successfully Hide \r\n"); 
}


void CVTxRing3Dlg::OnBnClickedCancel()
{
	CDialog::OnCancel(); 
	drv.Stop(SERVICE_NAME);
	drv.Remove(SERVICE_NAME);
}


void CVTxRing3Dlg::OnBnClickedButton1()
{
}

void CVTxRing3Dlg::OnBnClickedButton2()
{
	// TODO: Add your control notification handler code here
	drv.Stop(SERVICE_NAME);
	drv.Remove(SERVICE_NAME);
}
