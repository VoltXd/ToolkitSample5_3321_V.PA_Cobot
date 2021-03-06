//	--------------------------------------------------------------
//	MainFrm.cpp : main file of the KREON Toolkit Sample 5 project
//
//
//	This example program shows how to work with a Kreon scanner with
//	the Kreon Toolkit in "server" mode and a 3D measurement arm.
//	See details in ArmManager.h to know how to work with arms.
//
//	Notes : Library "WSock32.lib" is needed to build this program.
//			KreonServer.exe, KreonCore.dll, the DLL that manages the
//			scanner (e.g. KRZLS.dll, KRZII.dll...) and the DLL that
//			manages the arm (KR<ArmName>.dll) are needed to run it.

//	--------------------------------------------------------------
#include "stdafx.h"
#include "ToolkitSample5.h"
#include "MainFrm.h"
#include "ArmManager.h"
#include "KreonServer.h"
#include "SocketManager.h"
#include "KR_Specific.h"
#include "resource.h"
#include <fstream> //matthias
#include <thread>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



//	Arm's buttons states polling
#define	ARM_BUTTONS_TIMER_ID		1
#define	ARM_BUTTONS_TIMER_PERIOD	25		//	ms




//#define	ARM_NAME			_T("KreonArm")	// Kreon Ace/Baces
//#define	ARM_NAME			_T("Baces3D")	// FriulROBOT Baces3D
//#define	ARM_NAME			_T("Cimcore")	// Romer-Cimcore Stinger2/3000i/Infinite/Infinite2
//#define	ARM_NAME			_T("Faro")		// Faro Silver/Gold/Millenium
//#define	ARM_NAME			_T("FaroUSB")	// Faro Platinum/Titanium/Advantage/Quantum/Fusion
//#define	ARM_NAME			_T("RDS")		// Romer Absolute
//#define	ARM_NAME			_T("Romer")		// Romer France System6/Armony (Serial)
//#define	ARM_NAME			_T("RomerEth")	// Romer France Sigma/Omega (Ethernet)
//#define	ARM_NAME			_T("MS")		// MicroScribe
//#define	ARM_NAME			_T("Simu")
#define	ARM_NAME			_T("VXElementsWrapper")


CMainFrame * theMainFrame = NULL;

KR_CALIBRATION_FILE_LIST*	volatile g_CalibrationFileList = NULL;
CArmManager					*ArmManager;
CSocketManager				*SocketManager;
CChildView					CMainFrame::m_wndView;
HWND						CMainFrame::m_StatichWnd;
DWORD						StartTime;
int							NbLines;
double						PositioningMatrix[16];
bool						g_bScanning=false;	// Tells whether scanner+arm acquisition is running, no matter if it is during Positioning or a real scanning session (indicated by m_bScanning)
int							g_NbArmButtons=0;
bool *						g_pbButtonState=NULL;
int							g_nWaitForInitKreonCallback=0;
static bool					g_bEndKreon = false;
static int					g_WaitingForCommand = 0;

/////////////////////////////////////////////////////////////////////////////
IMPLEMENT_DYNAMIC(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_WM_CLOSE()
	ON_WM_SETFOCUS()
	ON_COMMAND(ID_KREONTOOLKIT_VIDEOSETUP, OnKreontoolkitVideosetup)
	ON_COMMAND(ID_KREONTOOLKIT_STARTSCAN, OnKreontoolkitStartscan)
	ON_COMMAND(ID_KREONTOOLKIT_POSITIONING, OnKreontoolkitPositioning)
	ON_COMMAND(ID_KREONTOOLKIT_PAUSESCAN, OnKreontoolkitPausescan)
	ON_UPDATE_COMMAND_UI(ID_KREONTOOLKIT_STARTSCAN, OnUpdateKreontoolkitStartscan)
	ON_UPDATE_COMMAND_UI(ID_KREONTOOLKIT_PAUSESCAN, OnUpdateKreontoolkitPausescan)
	ON_COMMAND(ID_KREONTOOLKIT_ARMPROPERTIES, OnKreonArmProperties)
	ON_COMMAND(ID_KREONTOOLKIT_SCANNERPROPERTIES, OnKreonScannerProperties)
	ON_COMMAND(ID_KREONTOOLKIT_LICENSE, OnKreontoolkitLicense)
	ON_COMMAND(ID_POSITIONINGDEVICE_COBOTTX2, OnPositioningDeviceCobot)
	ON_COMMAND(ID_POSITIONINGDEVICE_CTRACK, OnPositioningDeviceCtrack)
	ON_MESSAGE(WM_SOCKET_MESSAGE, OnDataSocket)
	ON_WM_TIMER()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()



/////////////////////////////////////////////////////////////////////////////
CMainFrame::CMainFrame()
{
	ArmManager=NULL;
	SocketManager=NULL;
	m_bKreonInitialized=false;
	m_bCobotCommunicationInitialized = false;
	m_bScanning=false;
	m_bPositioning=false;
	theMainFrame = this;

	positioningDevice = PA_Enums::CobotTx2Touch;
	//CMenu *pMenu = GetMenu();
	//if (pMenu != nullptr)
	//{
	//	pMenu->CheckMenuItem(ID_POSITIONINGDEVICE_COBOTTX2, MF_UNCHECKED | MF_BYCOMMAND);
	//	pMenu->CheckMenuItem(ID_POSITIONINGDEVICE_CTRACK, MF_CHECKED | MF_BYCOMMAND);
	//}

	//	Initialize the positioning matrix (You are encouraged to reload the last saved matrix from a previous session)
	PositioningMatrix[0]=1.0;	PositioningMatrix[4]=0.0;	PositioningMatrix[8]=0.0;	PositioningMatrix[12]=0.0;
	PositioningMatrix[1]=0.0;	PositioningMatrix[5]=1.0;	PositioningMatrix[9]=0.0;	PositioningMatrix[13]=0.0;
	PositioningMatrix[2]=0.0;	PositioningMatrix[6]=0.0;	PositioningMatrix[10]=1.0;	PositioningMatrix[14]=0.0;
	PositioningMatrix[3]=0.0;	PositioningMatrix[7]=0.0;	PositioningMatrix[11]=0.0;	PositioningMatrix[15]=1.0;
}

/////////////////////////////////////////////////////////////////////////////
CMainFrame::~CMainFrame()
{
	if (ArmManager)
	{
		delete ArmManager;
		ArmManager=NULL;
	}

	if (g_pbButtonState)
		delete [] g_pbButtonState;

	if (g_CalibrationFileList)
	{
		delete g_CalibrationFileList;
		g_CalibrationFileList = NULL;
	}
}

/////////////////////////////////////////////////////////////////////////////
int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	// create a view to occupy the client area of the frame
	if (!m_wndView.Create(NULL, NULL, AFX_WS_DEFAULT_VIEW,
		CRect(0, 0, 0, 0), this, AFX_IDW_PANE_FIRST, NULL))
	{
		TRACE0("Failed to create view window\n");
		return -1;
	}

	m_StatichWnd = m_hWnd;

	SetWindowPos(NULL,0,0,512,360,SWP_NOMOVE);
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if ( !CFrameWnd::PreCreateWindow(cs) )
		return FALSE;
	cs.dwExStyle &= ~WS_EX_CLIENTEDGE;
	cs.lpszClass = AfxRegisterWndClass(0);
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

/////////////////////////////////////////////////////////////////////////////
void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnSetFocus(CWnd* /*pOldWnd*/)
{
	// forward focus to the view window
	m_wndView.SetFocus();
}

/////////////////////////////////////////////////////////////////////////////
BOOL CMainFrame::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
	// let the view have first crack at the command
	if (m_wndView.OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
		return TRUE;

	// otherwise, do default handling
	return CFrameWnd::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

void CMainFrame::OnClose()
{
	if (m_bScanning)
	{
		g_bScanning = false;
		SocketManager->EndAcquisition();
		ArmManager->EndAcquisition();
		m_bScanning=false;
		ArmManager->EndArm();

		MSG Msg;
		while (PeekMessage(&Msg,NULL,0,0,PM_REMOVE))
		{
			TranslateMessage(&Msg);
			DispatchMessage(&Msg);
			Sleep(1);
		}
		Sleep(500);
	}

	if (SocketManager)
	{
		g_bEndKreon = false;

		// End the KREON system and the thread
		BYTE SensorOff=0;
		SocketManager->EndKreon(SensorOff);

		while (!g_bEndKreon && SocketManager->IsSocketConnected())
		{
			MSG Msg;
			while (PeekMessage(&Msg,NULL,0,0,PM_REMOVE))
			{
				TranslateMessage(&Msg);
				DispatchMessage(&Msg);
			}
			Sleep(10);
		}

		delete SocketManager;
		SocketManager=NULL;
	}

	CFrameWnd::OnClose();
}

/////////////////////////////////////////////////////////////////////////////
void OnKreonCallback(char *Command)
{
	//	When the server receives a command, it runs it, and then, sends an ack to the client
	//	You should always read this ack to know if the command ran correctly or not.
	//	The answer is composed of a minimum of 3 bytes (except KR_SOCKET_CALLBACK):
	//	Command[0] = Name of the command (e.g. KR_SOCKET_INITKREON, KR_SOCKET_GETXYZLASERLINE...)
	//	Command[1] = Return value of the command (most of the time: true if ok, else false)
	//	Command[2] = New value of the "LastErrorCode"
	switch(Command[0])
	{
		case KR_SOCKET_INITKREON:
			if(!Command[1])
			{
#ifdef _UNICODE
				bool bUnicode = true;
#else
				bool bUnicode = false;
#endif
				SocketManager->GetLastErrorString((BYTE)Command[2], bUnicode);
				g_nWaitForInitKreonCallback = -1;
			}
			else
				g_nWaitForInitKreonCallback = 0;
			break;

		case KR_SOCKET_GETLASTERRORSTRING:
			if(Command[1])
			{
				CString tmp;
				tmp.Format(_T("Error: %s"),(TCHAR*)&Command[3]);
				AfxMessageBox(tmp);
			}
			break;

		case KR_SOCKET_GETVIDEOSETTINGSLIST:
			if (Command[1])
			{
				BYTE nbSettings = Command[3];
				WORD idxCurrent = *(WORD*)&Command[4];
				CString txtSettings;
				WORD idx = 0;
				int skipCallbackHeader = 3+1+sizeof(WORD);
				TCHAR temp[64];
				for (int i = 0; i < nbSettings; i++)
				{
					mbstowcs_s(NULL, temp, 64, Command + skipCallbackHeader + idx, 64);
					txtSettings.Append( temp);
					if (idx == idxCurrent)
						txtSettings += _T(" (current)");
					txtSettings += _T("\n");
					idx += (WORD)strlen(Command + skipCallbackHeader + idx) + 1;	// +1 for terminal zero of each name
				}
				AfxMessageBox(txtSettings);
			}
			break;

		case KR_SOCKET_SPHERECALIBRATION:
			if (Command[1])
			{
				// If you called this:
				// SocketManager->SphereCalibration(CalibrationScan,CalibrationMatrix,Type5axes,SupportDirection,SphereDiameter);
				// then, the following data items are returned here:
				double SphereCenter[3];
				double Sigma;
				double CalibrationMatrix[16];
				memcpy(SphereCenter,&Command[3],3*sizeof(double));
				memcpy(&Sigma,&Command[3+3*sizeof(double)],sizeof(double));
				memcpy(CalibrationMatrix,&Command[3+sizeof(double)+3*sizeof(double)],16*sizeof(double));

				CString output;
				output.Format(_T("SphereCalibration results:\nSigma = %f"),Sigma);
				AfxMessageBox(output);
			}
			else
			{
				AfxMessageBox(_T("SphereCalibration failed"));
#ifdef _UNICODE
				bool bUnicode = true;
#else
				bool bUnicode = false;
#endif
				SocketManager->GetLastErrorString((BYTE)Command[2], bUnicode);
			}
			break;

		case KR_SOCKET_ENDKREON:
			g_bEndKreon = true;
			break;

		case KR_SOCKET_GETCALIBRATIONFILELIST:
			{
				// Allocate memory for the list
				KR_CALIBRATION_FILE_LIST* list = new KR_CALIBRATION_FILE_LIST;
				memcpy(list,&Command[3],sizeof(KR_CALIBRATION_FILE_LIST));

				if (Command[1])
				{
					/*
					// This sample code displays the calibration file list in a message box
					CString output = "Calibration file list:\n";
					int offsetFilename = 0;	// point towards the first file name
					for (UINT i=0; i<list->nbFiles; i++)
					{
						output += list->list+offsetFilename; // Add the file name to the output string
						if (list->offsetCurrentFile==offsetFilename)
							 output += " (current)";
						output += "\n";
						offsetFilename += strlen(list->list+offsetFilename)+1; // point towards the next file name
					}
					AfxMessageBox(output);
					*/
				}

				g_CalibrationFileList = list; // tells CMainFrame::Init() to stop waiting for a reply from the server
			}
			break;

		case KR_SOCKET_SETEXTERNALTRIGGERDURATION:
			if (Command[1])
			{
				g_WaitingForCommand = 0;
			}
			else
			{
				g_WaitingForCommand = -1;
			}
			break;

		case KR_SOCKET_OPENVIDEOSETTINGWINDOW:
			if (Command[1])
			{
				//	You can get the HWND of the window created by the Kreon SDK here.
				HWND hwnd;
				memcpy(&hwnd,&Command[3],sizeof(HWND));
			}
			break;

		case KR_SOCKET_OPENPOSITIONINGWINDOW:
		case KR_SOCKET_OPENPOSITIONINGWINDOWEXT:
			if (Command[1])
			{
				theMainFrame->m_bPositioning = true;

				//	You can get the HWND of the windows created by the Kreon SDK here.
				HWND hwnd;
				memcpy(&hwnd,&Command[3],sizeof(HWND));

				// Is the StartScan button enabled?
				if (Command[3+sizeof(HWND)])
				{
					//	If the StartScan button is enabled, it means that the user can start scanning directly by pressing an arm button.
					if (ArmManager->BeginAcquisition(KR_ARM_MODE_BUTTONS)==KR_ARM_OK)
						::SetTimer(CMainFrame::m_StatichWnd,ARM_BUTTONS_TIMER_ID,ARM_BUTTONS_TIMER_PERIOD,NULL);
				}
				g_WaitingForCommand = 0;
			}
			else
			{
				g_WaitingForCommand = -1;
			}
			break;

		case KR_SOCKET_GETCALIBRATIONMATRIX:
			if (Command[1])
			{
				//	you should save somewhere the calibration matrix
				//	(not done here as it is just an example)
				double receivedPositioningMatrix[16];
				memcpy(receivedPositioningMatrix,&Command[3],16*sizeof(double));

				//	you can also get the standard deviation from the toolkit if
				//	you need it
				double Sigma;
				memcpy(&Sigma,&Command[3 + 16*sizeof(double)],sizeof(double));

				//	and the shape default...
				double ShapeDefault;
				memcpy(&ShapeDefault,&Command[3 + 17*sizeof(double)],sizeof(double));

				double StatisticShapeDefault;
				memcpy(&StatisticShapeDefault,&Command[3 + 18*sizeof(double)],sizeof(double));
			}
			break;

		case KR_SOCKET_GETXYZLASERLINE:
			if (Command[1])
			{
				CDC*	cdc=CMainFrame::m_wndView.GetDC();
				CString txt;
				int		n;
				RECT rect;
				rect.left = 0;
				rect.top = 0;
				rect.bottom = 300;
				rect.right = 500;
				CBrush brush((COLORREF)0xFFFFFF);
				cdc->FillRect(&rect,&brush);

				memcpy(&n,&Command[3],sizeof(int));
				txt.Format(_T("%d scanline(s) ready in the buffer"),n);
				cdc->TextOut(10, 100, txt);

				if (n>0)
				{
					//	Get the laser line
					DWORD LaserLineSizeA,LaserLineSizeB,LaserLineSize;
					DWORD LaserLineSizeAInBytes,LaserLineSizeBInBytes,LaserLineSizeInBytes;
					memcpy(&LaserLineSizeA,&Command[3+sizeof(int)],sizeof(DWORD));
					memcpy(&LaserLineSizeB,&Command[3+sizeof(int)+sizeof(DWORD)+(LaserLineSizeA*3*sizeof(double))+(16*sizeof(double))],sizeof(DWORD));
					LaserLineSize = LaserLineSizeA+LaserLineSizeB;
					double * LaserLine = new double[3*LaserLineSize];
					LaserLineSizeAInBytes = LaserLineSizeA*3*sizeof(double);
					LaserLineSizeBInBytes = LaserLineSizeB*3*sizeof(double);
					LaserLineSizeInBytes = LaserLineSizeAInBytes + LaserLineSizeBInBytes;
					if (LaserLineSizeA)
						memcpy(LaserLine,&Command[3+sizeof(int)+sizeof(DWORD)],LaserLineSizeAInBytes);
					if (LaserLineSizeB)
						memcpy(LaserLine+(3*LaserLineSizeA),&Command[3+sizeof(int)+sizeof(DWORD)+LaserLineSizeAInBytes+(16*sizeof(double))+sizeof(DWORD)],LaserLineSizeBInBytes);

					double	MachineMatrix[16];
					memcpy(MachineMatrix,&Command[3+sizeof(int)+sizeof(DWORD)+LaserLineSizeAInBytes],16*sizeof(double));
					/*
						The Machine Matrix gives the position and direction of the machine
						MachineMatrix[ 0] == Xleft
						MachineMatrix[ 1] == Yleft
						MachineMatrix[ 2] == Zleft
						MachineMatrix[ 3] == 0
						MachineMatrix[ 4] == Xup
						MachineMatrix[ 5] == Yup
						MachineMatrix[ 6] == Zup
						MachineMatrix[ 7] == 0
						MachineMatrix[ 8] == Xdirection
						MachineMatrix[ 9] == Ydirection
						MachineMatrix[10] == Zdirection
						MachineMatrix[11] == 0
						MachineMatrix[12] == Xposition
						MachineMatrix[13] == Yposition
						MachineMatrix[14] == Zposition
						MachineMatrix[15] == 1
					*/

					// The acquisition number counts the acquisitions. The TimeStamp gives an approximation (in PC time) of the moment the acquisition was actually performed.
					// For some specific applications, those may be useful to check that there are no missing acquisitions compared to the values received in the KR_CMD_TRIGGERED_POSITION_NEEDED callback.
					DWORD AcqNumber,TimeStamp;
					memcpy(&AcqNumber,&Command[3+sizeof(int)+2*sizeof(DWORD)+LaserLineSizeInBytes+16*sizeof(double)],sizeof(DWORD));
					memcpy(&TimeStamp,&Command[3+sizeof(int)+3*sizeof(DWORD)+LaserLineSizeInBytes+16*sizeof(double)],sizeof(DWORD));

					if (LaserLineSize)
						txt.Format(_T("LaserLine has %d points"), LaserLineSize);
					else
						txt = _T("LaserLine has no points (or too small move compared to Step)");
					cdc->TextOut(10, 120, txt);
					if (LaserLineSizeA)
						txt.Format(_T("A: (%8.3f , %8.3f , %8.3f) -> (%8.3f , %8.3f , %8.3f)"),
							LaserLine[0],LaserLine[1],LaserLine[2],LaserLine[((LaserLineSizeA-1)*3)+0],LaserLine[((LaserLineSizeA-1)*3)+1],LaserLine[((LaserLineSizeA-1)*3)+2]);
					else
						txt = (_T("No points on camera A"));
					cdc->TextOut(10,140,txt);
					if (LaserLineSizeB)
						txt.Format(_T("B: (%8.3f , %8.3f , %8.3f) -> (%8.3f , %8.3f , %8.3f)"),
							LaserLine[(LaserLineSizeA*3)+0],LaserLine[(LaserLineSizeA*3)+1],LaserLine[(LaserLineSizeA*3)+2],LaserLine[((LaserLineSize-1)*3)+0],LaserLine[((LaserLineSize-1)*3)+1],LaserLine[((LaserLineSize-1)*3)+2]);
					else
						txt = (_T("No points on camera B"));
					cdc->TextOut(10,160,txt);

					#define MAX_LINES_TO_COMPUTE_FREQUENCY 100
					static CList<DWORD,DWORD> Times;
					DWORD CurrentTime = GetTickCount();
					if (NbLines==0)
					{
						StartTime=CurrentTime;
						Times.RemoveAll();
					}
					Times.AddTail(CurrentTime);
					NbLines++;
					if (NbLines>2)
					{
						DWORD OldestTime,NbLinesForFrequency;
						if (NbLines<MAX_LINES_TO_COMPUTE_FREQUENCY)
						{
							OldestTime = Times.GetHead();
							NbLinesForFrequency = NbLines;
						}
						else
						{
							OldestTime = Times.RemoveHead();
							NbLinesForFrequency = MAX_LINES_TO_COMPUTE_FREQUENCY;
						}
						double Time=(CurrentTime-OldestTime)/1000.0;
						int ElapsedTime=(CurrentTime-StartTime)/1000;
						static DWORD LastDisplayTime=StartTime;
						static CString DisplayedString(_T(""));
						if ( ((CurrentTime-LastDisplayTime)>500) && (Time>0) )
						{
							LastDisplayTime = CurrentTime;
							DisplayedString.Format(_T("Received lines: %03d, time: %03d, Freq. over last %3u lines: %2.1f"),
								NbLines,ElapsedTime,NbLinesForFrequency,(double)NbLinesForFrequency/Time);
						}
						cdc->TextOut(10,180,DisplayedString);
					}

					txt = _T("Buttons state:");
					for(int i=0 ; i<g_NbArmButtons ; i++)
						txt.AppendFormat(_T(" %d"),g_pbButtonState[i]);
					cdc->TextOut(10, 200, txt);

					// Code from Matthias Bordron, to write the current laser line into points.txt
					std::ofstream fichier("points.txt", std::ios::out | std::ios::app);
					if (fichier)
					{
						unsigned int target;
						target = (unsigned int)LaserLineSize;
						unsigned int zero = 0;
						if (target != zero)
						{
							CString text;
							for (unsigned int a = 0; a<target; a++)
							{
								fichier << LaserLine[a * 3] << " " << LaserLine[(a * 3) + 1] << " " << LaserLine[(a * 3) + 2] << " ";
							}
							fichier << std::endl;
							fichier.close();
						}
					}
					else
					{
						AfxMessageBox(_T("Impossible d'ouvrir fichier"));
					}
					// end Matthias Bordron

					delete [] LaserLine;
				}

				CMainFrame::m_wndView.ReleaseDC(cdc);
			}
			break;

		case KR_SOCKET_GETLASERPLANECORNERS:
			if (Command[1])
			{
				double corners[4*2];	//	4 UV coordinates
				memcpy(corners,&Command[3],4*2*sizeof(double));
				CString strCorners;
				strCorners.Format(_T("Corners:\n")
								  _T("   (%7.3f,%7.3f) (%7.3f,%7.3f)\n")
								  _T("(%7.3f,%7.3f)       (%7.3f,%7.3f)\n"),
								  corners[0],corners[1], corners[2],corners[3],
								  corners[6],corners[7], corners[4],corners[5]);
				AfxMessageBox(strCorners);
			}
			break;

		case KR_SOCKET_CALLBACK:
			{
				UCHAR Cmd;
				Cmd=Command[1];
				switch(Cmd)
				{
					case KR_CMD_POSITIONING_WINDOW_CLOSED:
						{
							// If you started a timer and button interrogation when you opened the window, and
							// if the user did not perform any action that stopped that timer/interrogation,
							// then, those should be stopped here:
							::KillTimer(CMainFrame::m_StatichWnd,ARM_BUTTONS_TIMER_ID);
							if (ArmManager->GetAcquisitionMode() != KR_ARM_MODE_IDLE)
								ArmManager->EndAcquisition();

							DWORD  Param;
							memcpy(&Param,&Command[2],sizeof(DWORD));
							if (Param==1)
							{
								//	The positioning process returned 1 index
								//	To get its resulting matrix, just send a "KR_SOCKET_GETCALIBRATIONMATRIX"
								//	command to the server.
								BYTE Index = 0;
								SocketManager->GetCalibrationMatrix(Index);
							}

							theMainFrame->m_bPositioning = false;
						}
						break;

					case KR_CMD_VIDEOSETUP_WINDOW_CLOSED:
						//	The video setup window has been closed by the user
						//	(or by a call to the KR_SOCKET_CLOSEVIDEOSETTINGWINDOW command)
						break;

					case KR_CMD_SCAN_FREQUENCY:
						{
							//	Some arms need to know the frequency they will work at
							DWORD  Param;
							memcpy(&Param,&Command[2],sizeof(DWORD));
							ArmManager->SetParameter("AcquisitionFrequency",&Param);
						}
						break;

					case KR_CMD_SCAN_STARTED:
						//	The scan is starting...
						//	So, we can stop polling the arm buttons
						::KillTimer(CMainFrame::m_StatichWnd,ARM_BUTTONS_TIMER_ID);
						ArmManager->EndAcquisition();
						//	And start the acquisition mode of the arm
						ArmManager->BeginAcquisition(KR_ARM_MODE_SYNCHRONOUS);
						g_bScanning = true;
						//	Finally, as the arm is ready, start the scanner digitization.
						SocketManager->StartScannerAcquisitionInPositioningWindow();
						break;

					case KR_CMD_SCAN_PAUSED:
						{
							g_bScanning = false;
							ArmManager->EndAcquisition();
							ArmManager->BeginAcquisition(KR_ARM_MODE_BUTTONS);

							if (g_NbArmButtons>0)
							{
								bool bButtonPressed = true;
								//	Wait until the button is released
								while (bButtonPressed)
								{
									bButtonPressed = false;
									if (ArmManager->GetButtonState(g_pbButtonState)==KR_ARM_OK)
										for (int i=0; i<g_NbArmButtons ; i++)
											if (g_pbButtonState[i])
											{
												bButtonPressed = true;
												Sleep(10);	// To avoid calling GetButtonState too much
												break;
											}
								}
							}

							::SetTimer(CMainFrame::m_StatichWnd,ARM_BUTTONS_TIMER_ID,ARM_BUTTONS_TIMER_PERIOD,NULL);
						}
						break;

					case KR_CMD_SCAN_STOPPED:
						::KillTimer(CMainFrame::m_StatichWnd,ARM_BUTTONS_TIMER_ID);
						ArmManager->EndAcquisition();
						break;

					case KR_CMD_SCAN_LINE_READY:
						{
							//	Ask the laser line to the toolkit
							SocketManager->GetXYZLaserLine();
						}
						break;

					case KR_CMD_TRIGGERED_POSITION_NEEDED:
						{
							// The acquisition number counts the acquisitions. The TimeStamp gives an approximation (in PC time) of the moment the acquisition was actually performed.
							// For some specific applications, those may be useful to match the requested position here with the XYZ laser line that you will receive in the KR_SOCKET_GETXYZLASERLINE callback.
							DWORD AcqNumber,TimeStamp;
							memcpy(&AcqNumber,&Command[2+sizeof(DWORD)],sizeof(DWORD));
							memcpy(&TimeStamp,&Command[2],sizeof(DWORD));

							if (!g_bScanning)	// Ignore the callback if you already stoppped scanning
								break;

							double	PositionMatrix[16];
//#define __USE_JOINT_ANGLES__	//	Uncomment to get the joint angles (only available for Baces3D arms)
#ifdef __USE_JOINT_ANGLES__
							double * JointAngles;
							int nJoints;
							int Result=ArmManager->GetTriggeredPositionAndJointAngles(PositionMatrix,g_bButtonState,&nJoints,&JointAngles,true);
#else
							int Result=ArmManager->GetTriggeredPosition(PositionMatrix,g_pbButtonState);
#endif
							if ((Result==KR_ARM_OK) || (Result==KR_ARM_OK_BUT_POSITION_UNAVAILABLE) || (Result==KR_ARM_MECHANICAL_STOP))
							{
								bool bPauseButton=false;
								if (Result==KR_ARM_OK)
								{
									if (g_NbArmButtons>0)
									{
										for (int b=0 ; (b<g_NbArmButtons) && !bPauseButton ; b++)
											bPauseButton = g_pbButtonState[b];
									}
								}

								//	You may want to stop the acquisition if a button is pressed
								if (bPauseButton)
								{
									g_bScanning = false;
									SocketManager->EndAcquisition();

									// In the context of this example program, if really scanning (not Positioning),
									// you can do as if the user selected "Stop scanning" in the menu.
									if (!theMainFrame->m_bPositioning)
									{
										theMainFrame->m_bScanning = false;
										ArmManager->EndAcquisition();

										// Let's wait for button release before actually polling the buttons to restart scanning
										ArmManager->BeginAcquisition(KR_ARM_MODE_BUTTONS);

										if (g_NbArmButtons>0)
										{
											bool bButtonPressed = true;
											//	Wait until the button is released
											while (bButtonPressed)
											{
												bButtonPressed = false;
												if (ArmManager->GetButtonState(g_pbButtonState)==KR_ARM_OK)
													for (int i=0; i<g_NbArmButtons ; i++)
														if (g_pbButtonState[i])
														{
															bButtonPressed = true;
															Sleep(10);	// To avoid calling GetButtonState too much
															break;
														}
											}
										}

										// Now we know the buttons are released, so let's poll them to start scanning
										::SetTimer(CMainFrame::m_StatichWnd,ARM_BUTTONS_TIMER_ID,ARM_BUTTONS_TIMER_PERIOD,NULL);

										CDC*	cdc=CMainFrame::m_wndView.GetDC();
										RECT rect;
										rect.left = 0;
										rect.top = 0;
										rect.bottom = 300;
										rect.right = 500;
										CBrush brush((COLORREF)0xFFFFFF);
										cdc->FillRect(&rect,&brush);
										cdc->TextOut(10, 100, _T("Scan stopped by arm button. Press it again to restart scanning..."));
									}
								}
								else
								{
									//	The Kreon SDK needs a triggered position from the arm
									if (Result==KR_ARM_OK)
									{
#ifdef __USE_JOINT_ANGLES__
										//	The code below creates a text string that displays the position and the corresponding joint angles
										CString txt;
										txt.Format(_T("Pos: %.3f,%.3f,%.3f / %d joints:"),PositionMatrix[12],PositionMatrix[13],PositionMatrix[14],nJoints);
										if (nJoints && JointAngles)
										{
											CString temp;
											for (int i=0; i<nJoints; i++)
											{
												temp.Format(_T(" %.1f"),JointAngles[i]);
												txt += temp;
											}
										}
										txt += _T("\n");
										OutputDebugString(txt);
#endif
										SocketManager->SetMachinePosition(PositionMatrix);
									}
									else	// Or at least know that there was no position available
										SocketManager->SetNoMachinePositionAvailable();

									//	These functions automatically request for a new
									//	acquisition from the scanner (which will need
									//	a triggered position, and so on...)
								}
							}
							else
							{
								g_bScanning = false;
								SocketManager->EndAcquisition();
							}
						}
						break;

					case KR_CMD_SCANNER_ERROR:
						g_bScanning = false;
						SocketManager->EndAcquisition();
						break;

					case KR_CMD_KEY_PRESSED:
						//	A function key has been pressed in the positioning window or in the video setup window
						DWORD Key;
						memcpy(&Key,&Command[2],sizeof(DWORD));
						if (Key==VK_F1)
						{
							//	...
						}
						break;

					case KR_CMD_POSITIONING_DELETED:
						//	The last positioning has been deleted in the positioning window.
						//	You should restart the timer to read the arm buttons again.
						ArmManager->BeginAcquisition(KR_ARM_MODE_BUTTONS);
						::SetTimer(CMainFrame::m_StatichWnd,ARM_BUTTONS_TIMER_ID,ARM_BUTTONS_TIMER_PERIOD,NULL);
						break;
				}
			}
			break;
	}
}

/////////////////////////////////////////////////////////////////////////////
bool CMainFrame::Init()
{
	if (!m_bKreonInitialized)
	{
		if (SocketManager)
		{
			delete SocketManager;
			SocketManager=NULL;
		}
		if (ArmManager)
		{
			if (ArmManager->IsConnected())
				ArmManager->EndArm();
			delete ArmManager;
			ArmManager=NULL;
		}

		//	Load the library (DLL) of the arm
		TCHAR DLLName[MAX_PATH];
		_stprintf_s(DLLName,_countof(DLLName),_T("%s.DLL"),ARM_NAME);
//		_stprintf_s(DLLName,_countof(DLLName),_T("KR%s.DLL"),ARM_NAME);	// Use this instead if you do not put arm files in subfolders
		ArmManager = new CArmManager(DLLName, positioningDevice);
		if (!ArmManager->IsInitialized())
		{
			CString ErrMsg;
			ErrMsg.Format(_T("Error: unable to load library \"%s\"."),DLLName);
			AfxMessageBox(ErrMsg);
			return false;
		}

		//	The number of arm buttons depends on the arm model. Let's interrogate the arm manager about the actual number of buttons
		if (g_pbButtonState)
			delete [] g_pbButtonState;
		if (ArmManager->GetParameter("NbButtons",&g_NbArmButtons))
			g_pbButtonState = new bool [g_NbArmButtons];

		//	Initialize and run the Kreon server
		SocketManager = new CSocketManager();
		if (SocketManager->InitServer(OnKreonCallback,m_hWnd,WM_SOCKET_MESSAGE)!=KR_ERR_NO_ERROR)
		{
			delete SocketManager;
			SocketManager=NULL;
			delete ArmManager;
			ArmManager=NULL;
			return false;
		}


		//	Get the calibration files list (Field "needsCalFile" from KR_CALIBRATION_FILE_LIST
		//  tells whether the scanner needs a calibration file or not so it can be called with any scanner)
		//	One or more calibration files should be present in the <Hardware>\..\Calibration directory
		if (g_CalibrationFileList)
		{
			delete g_CalibrationFileList;
			g_CalibrationFileList = NULL;
		}
		SocketManager->GetCalibrationFileList();

		// Wait until we receive the calibration file list from the server
		while (!g_CalibrationFileList)
		{
			MSG Msg;
			if (PeekMessage(&Msg,NULL,0,0,PM_REMOVE))
			{
				TranslateMessage(&Msg);
				DispatchMessage(&Msg);
			}
			Sleep(0);
		}

		char * CalibrationFile;
		if (!g_CalibrationFileList->nbFiles)
		{
			//	Free CalibrationFileList (if you don't need it any more)
			delete g_CalibrationFileList;
			g_CalibrationFileList = NULL;
			return false;	// Fail because there is no calibration file available
		}
		if (g_CalibrationFileList->offsetCurrentFile == -1)	// No calibration file was selected?
			g_CalibrationFileList->offsetCurrentFile = 0;		// ...so we take the first one.
		CalibrationFile = g_CalibrationFileList->list+g_CalibrationFileList->offsetCurrentFile;

		//	Initialize the Kreon Toolkit
		BYTE MachineType = 99;  // See KR_SOCKET_INITKREON HTML documentation for a complete list of machine types
		UINT CodeClient  = 0;	// <- insert your client code here (provided by Kreon Technologies)

		g_nWaitForInitKreonCallback = 1;
		SocketManager->InitKreon(MachineType, CalibrationFile, CodeClient /*,"FR"*/); // <- optional language parameter
		//	Be sure that KreonServer finished processing the initialization before going any further
		while (g_nWaitForInitKreonCallback>0)
		{
			MSG Msg;
			if (PeekMessage(&Msg,NULL,0,0,PM_REMOVE))
			{
				TranslateMessage(&Msg);
				DispatchMessage(&Msg);
			}
			Sleep(1);
		}

		//	Free CalibrationFileList (if you don't need it any more)
		delete g_CalibrationFileList;
		g_CalibrationFileList = NULL;

		if (g_nWaitForInitKreonCallback!=-1)
			m_bKreonInitialized=true;
	}

	return m_bKreonInitialized;
}

/////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnKreontoolkitLicense() 
{
	if (!m_bKreonInitialized)
		if (!Init())
			return;

	// Please provide some way to display this dialog in your application
	SocketManager->OpenLicenseDialog(m_hWnd);
}

/////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnKreontoolkitVideosetup() 
{
	if (!m_bKreonInitialized)
		if (!Init())
			return;

	SocketManager->OpenVideoSettingWindow(true);
}

/////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnKreontoolkitPositioning() 
{
	// One should obviously not try positioning while scanning
	if (m_bScanning)
		OnKreontoolkitPausescan();

	if (!m_bKreonInitialized)
		if (!Init())
			return;

#define WINDOWED_CALIBRATION // If you want to make your own positioning window, see the code in #else branch
#ifdef WINDOWED_CALIBRATION

	//	Begin the arm
	if (!ArmManager->IsConnected())
	{
		if (ArmManager->BeginArm()!=KR_ARM_OK)
		{
			CString ErrMsg;
			ErrMsg.Format(_T("Error: Unable to initialize the arm. Please check the connection"));
			AfxMessageBox(ErrMsg);
			return;
		}
	}

	//	The arm and the ECU are correctly initialized.
	//	We need to inform the ECU about the duration of the external trigger.
	//	To get this information, we use the "GetParameter" method of CArmManager
	//	(see ArmManager.h) je
	WORD TriggerDuration = 500;
	ArmManager->GetParameter("TriggerDuration",&TriggerDuration);
	g_WaitingForCommand = 1;
	SocketManager->SetExternalTriggerDuration(TriggerDuration);
	while (g_WaitingForCommand == 1)
	{
		Sleep(10);
		MSG Msg;
		while (PeekMessage(&Msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&Msg);
			DispatchMessage(&Msg);
		}
	}
	ASSERT(g_WaitingForCommand == 0);

	BYTE nbIndex = 1;	// For an arm, we use only one index
	KR_INDEX_EXT myIndex;
	memset(&myIndex,0,sizeof(myIndex));
	memcpy(myIndex.PositioningMatrix,PositioningMatrix,16*sizeof(double));
	strcpy_s(myIndex.Name,_countof(myIndex.Name),"MyIndex");
	g_WaitingForCommand = 1;
	SocketManager->OpenPositioningWindowExt(&myIndex,nbIndex,
		KR_SCAN_METHOD_SYNCHRONOUS,true,(BYTE)15,0.0,false,false,m_hWnd,false,0xFF);
	while (g_WaitingForCommand == 1)
	{
		Sleep(10);
		MSG Msg;
		while (PeekMessage(&Msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&Msg);
			DispatchMessage(&Msg);
		}
	}
	ASSERT(g_WaitingForCommand == 0);

#else

	// The code below prepares all the input data
	// for SphereCalibration (positioning without window)

	KR_LASER_LINE* CalibrationScan = NULL; // List of laser lines
	double PositioningMatrixUsedDuringScan[16];
	bool Type5axes = true; // The machine has 5 axes or more (typically an arm)
	long SupportDirection = 0;
	double SphereDiameter = 25; // millimeters

	// For this example, we read the laser lines from a file
	int	LineNumber;
	FILE* SphereCWKFile;
	fopen_s(&SphereCWKFile, "sphere_cal.cwk", "rb");
	if (!SphereCWKFile)
	{
		AfxMessageBox(_T("Failed to open simulation file!"));
		return;
	}
	else
		CKR_Specific::CWK_GetGeneral(SphereCWKFile, PositioningMatrixUsedDuringScan, &LineNumber);

	double * LaserLine = new double[KR_MAX_POINTS_PER_ACQ*3];
	UINT LaserLineSize=0;
	double Matrix[16];
	double matPosInv[16];
	memcpy(matPosInv, PositioningMatrixUsedDuringScan, 16*sizeof(double));
	CKR_Specific::MatrixInvert(matPosInv);

	for  (int i=0; i<LineNumber; i++)
	{
		// In this example, we read each laser line data from a CWK file; but, in your final application, you will
		// certainly record the scan lines in a chained list of KR_LASER_LINE when you receive a KR_SOCKET_GETXYZLASERLINE
		// callback while performing a calibration scan, the same way as you would perform a normal scan, except that
		// InitAcquisition would have been called with ScanMethod KR_SCAN_METHOD_SYNCHRONOUS_POSITIONING.
		if (CKR_Specific::CWK_GetNextLine(SphereCWKFile, LaserLine, &LaserLineSize, Matrix) && LaserLineSize)
		{
			// In CWK files, the read Matrix is the Change of Basis Transform Matrix, which is equal to "MachinePositionMatrix . PositioningMatrix".
			// So, to get the Machine Position Matrix required by command SphereCalibration, we multiply the read Matrix by the inverted positioning matrix.
			// Obviously, this is not necessary if you directly recorded the Machine Position Matrix while scanning in your application.
			CKR_Specific::MatrixMultiply(Matrix, matPosInv);

			KR_LASER_LINE* KRLaserLine = new KR_LASER_LINE;
			KRLaserLine->SizeA = LaserLineSize;
			KRLaserLine->SizeB = 0;	//	0 for all scanners but the Aquilon that can differentiate the points acquired on each camera
			KRLaserLine->Size = KRLaserLine->SizeA + KRLaserLine->SizeB;
			memcpy(KRLaserLine->Matrix, Matrix, 16*sizeof(double));
			KRLaserLine->Points = new double [3*KRLaserLine->Size];
			// The coordinates are expressed as XYZ coordinates = Machine Position Matrix . PositioningMatrixUsedDuringScan . UVW coordinates ;
			// but they are probably wrong because the PositioningMatrixUsedDuringScan is probably wrong, hence the need to perform new Positioning.
			// This is not a problem. The SphereCalibration will process the points to compute the new Positioning Matrix that should be used for the next scanning sessions.
			if (KRLaserLine->SizeA>0)
				memcpy(KRLaserLine->Points, LaserLine, 3*KRLaserLine->SizeA*sizeof(double));
			if (KRLaserLine->SizeB>0)
			{
				// Two-camera scanners (Aquilon) make a difference between the points viewed by each camera when scanning in mode KR_SCAN_METHOD_SYNCHRONOUS_POSITIONING
				int offset = 3*KRLaserLine->SizeA;
				memcpy(KRLaserLine->Points+offset, LaserLine+offset, 3*KRLaserLine->SizeB*sizeof(double));
			}
			KRLaserLine->IO = 0;
			KRLaserLine->NextLine = CalibrationScan;
			CalibrationScan = KRLaserLine;
		}
	}
	fclose(SphereCWKFile);

	// Everything is ready to compute the positioning data...
	SocketManager->SphereCalibration(CalibrationScan, PositioningMatrixUsedDuringScan, Type5axes, SupportDirection, SphereDiameter);

	// Free allocated memory
	delete [] LaserLine;
	while (CalibrationScan)
	{
		KR_LASER_LINE * pNext = CalibrationScan->NextLine;
		delete [] CalibrationScan->Points;
		delete CalibrationScan;
		CalibrationScan = pNext;
	}
#endif
}


/////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnTimer(UINT_PTR nIDEvent)
{
	static bool st_bTimerBusy=false;
	if (st_bTimerBusy)
		return;
	else
		st_bTimerBusy=true;

	if (nIDEvent==ARM_BUTTONS_TIMER_ID)
	{
		if (g_NbArmButtons>0)
		{
			if (ArmManager->GetButtonState(g_pbButtonState)==KR_ARM_OK)
			{
				bool bButtonPressed = false;
				for (int i=0; i<g_NbArmButtons ; i++)
					if (g_pbButtonState[i])
					{
						bButtonPressed = true;
						break;
					}

				if (bButtonPressed)
				{
					//	Wait until the button is released
					while (bButtonPressed)
					{
						bButtonPressed = false;
						if (ArmManager->GetButtonState(g_pbButtonState)==KR_ARM_OK)
							for (int i=0; i<g_NbArmButtons ; i++)
								if (g_pbButtonState[i])
								{
									bButtonPressed = true;
									Sleep(10);	// To avoid calling GetButtonState too much
									break;
								}
					}

					if (theMainFrame->m_bPositioning)
					{
						g_bScanning = true;
						SocketManager->BeginAcquisition();	// When the Positioning window is open, this will actually generate a KR_CMD_SCAN_STARTED callback
					}
					else	// Scanning
					{
						//	We can stop polling the arm buttons, now
						::KillTimer(CMainFrame::m_StatichWnd,ARM_BUTTONS_TIMER_ID);
						ArmManager->EndAcquisition();
						//	And start the acquisition mode of the arm
						ArmManager->BeginAcquisition(KR_ARM_MODE_SYNCHRONOUS);

						NbLines=0;
						theMainFrame->m_bScanning=true;
						g_bScanning = true;
						SocketManager->BeginAcquisition();
					}
				}
			}
		}
	}

	CFrameWnd::OnTimer(nIDEvent);
	st_bTimerBusy = false;
}

/////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnKreontoolkitStartscan() 
{
	if (!m_bKreonInitialized)
		if (!Init())
			return;

	//	Begin the arm
	if (!ArmManager->IsConnected())
	{
		if (ArmManager->BeginArm()!=KR_ARM_OK)
		{
			CString ErrMsg;
			ErrMsg.Format(_T("Error : unable to initialize the arm. Check the connection"));
			AfxMessageBox(ErrMsg);
			return;
		}
	}
	else
	{
		// If you started a timer and button interrogation when the acquisition was paused by pressing an arm button the window,
		// and if the user did not perform any action that stopped that timer/interrogation,
		// then, those should be stopped here:
		::KillTimer(CMainFrame::m_StatichWnd,ARM_BUTTONS_TIMER_ID);
		if (ArmManager->GetAcquisitionMode() != KR_ARM_MODE_IDLE)
			ArmManager->EndAcquisition();
	}

	//	The arm and the ECU are correctly initialized.
	//	We need to inform the ECU about the duration of the external trigger.
	//	To get this information, we use the "GetParameter" method of CArmManager
	//	(see ArmManager.h)
	WORD TriggerDuration;
	ArmManager->GetParameter("TriggerDuration",&TriggerDuration);
	SocketManager->SetExternalTriggerDuration(TriggerDuration);

	int  Frequency = 20;
	float Step=0.2f;
	UINT  Decimation = 1;
	SocketManager->InitAcquisition(KR_SCAN_METHOD_SYNCHRONOUS,
								   Frequency,	// not used in synchronous mode
								   Step, PositioningMatrix, Decimation);
	ArmManager->BeginAcquisition(KR_ARM_MODE_SYNCHRONOUS);	// synchronous scan mode
	NbLines =0;
	m_bScanning = true;
	g_bScanning = true;
	SocketManager->BeginAcquisition();
}

/////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnUpdateKreontoolkitStartscan(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(!m_bScanning);
}

/////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnKreontoolkitPausescan() 
{
	g_bScanning = false;
	SocketManager->EndAcquisition();
	ArmManager->EndAcquisition();
	m_bScanning=false;
}

/////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnUpdateKreontoolkitPausescan(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_bScanning);
}

/////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnKreonArmProperties(void)
{
	if (!m_bKreonInitialized)
		if (!Init())
			return;

	ArmManager->PropertiesWindow(m_hWnd);
}

/////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnKreonScannerProperties(void)
{
	if (!m_bKreonInitialized)
		if (!Init())
			return;

	SocketManager->OpenScannerPropertiesWindow(m_hWnd);
}

void CMainFrame::OnPositioningDeviceCobot()
{
	CMenu *pMenu = GetMenu();
	if (pMenu != nullptr)
	{
		pMenu->CheckMenuItem(ID_POSITIONINGDEVICE_COBOTTX2, MF_CHECKED | MF_BYCOMMAND);
		pMenu->CheckMenuItem(ID_POSITIONINGDEVICE_CTRACK, MF_UNCHECKED | MF_BYCOMMAND);
	}

	positioningDevice = PA_Enums::CobotTx2Touch;

	// Restart Kreon
	m_bKreonInitialized = false;

	// Restart arm manager
	if (ArmManager != NULL)
		if (ArmManager->IsConnected())
			ArmManager->EndArm();
	delete ArmManager;
	ArmManager = NULL;

	ArmManager = new CArmManager(ARM_NAME, positioningDevice);
}

void CMainFrame::OnPositioningDeviceCtrack()
{
	CMenu *pMenu = GetMenu();
	if (pMenu != nullptr)
	{
		pMenu->CheckMenuItem(ID_POSITIONINGDEVICE_COBOTTX2, MF_UNCHECKED | MF_BYCOMMAND);
		pMenu->CheckMenuItem(ID_POSITIONINGDEVICE_CTRACK, MF_CHECKED | MF_BYCOMMAND);
	}


	positioningDevice = PA_Enums::Ctrack;

	// Restart Kreon
	m_bKreonInitialized = false;

	// Restart arm manager
	if (ArmManager != NULL)
		if (ArmManager->IsConnected())
			ArmManager->EndArm();
	delete ArmManager;
	ArmManager = NULL;

	ArmManager = new CArmManager(ARM_NAME, positioningDevice);
}


/////////////////////////////////////////////////////////////////////////////
LRESULT CMainFrame::OnDataSocket(WPARAM /*wParam*/,LPARAM /*lParam*/)
{
	SocketManager->OnDataSocket();
	return 0L;
}