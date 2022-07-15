/////////////////////////////////////////////////////////////////////////////
//	ArmManager.cpp
//
//		Make the interface between any of the KREON arm dll and your program.
//
/////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "ArmManager.h"
#include <stdio.h>
#include <tchar.h>
#include "VXElementsWrapper.h"


/////////////////////////////////////////////////////////////////////////////
CArmManager::CArmManager(/*const*/ TCHAR* DLLName)
{
	m_hModule			= NULL;
	m_bDllOk			= true;
	m_NbButtons			= -1;
	m_TriggerDuration	= 0xFFFFFFFF;
	m_DllName			= NULL;

	m_lpfnDllIsConnected						= NULL;
	m_lpfnDllBeginArm							= NULL;
	m_lpfnDllBeginArmWithCalibration			= NULL;
	m_lpfnDllEndArm								= NULL;
	m_lpfnDllBeginAcquisition					= NULL;
	m_lpfnDllGetAcquisitionMode					= NULL;
	m_lpfnDllEndAcquisition						= NULL;
	m_lpfnDllGetCurrentPosition					= NULL;
	m_lpfnDllGetCurrentPositionAndJointAngles	= NULL;
	m_lpfnDllGetTriggeredPosition				= NULL;
	m_lpfnDllGetTriggeredPositionAndJointAngles	= NULL;
	m_lpfnDllGetButtonState						= NULL;
	m_lpfnDllGetParameter						= NULL;
	m_lpfnDllSetParameter						= NULL;
	m_lpfnDllHasPropertiesWindow				= NULL;
	m_lpfnDllPropertiesWindow					= NULL;
	m_lpfnDllProbeCalibration					= NULL;
	m_lpfnDllOpenKreonArmProbesManagement		= NULL;

	//	Loads the DLL
	m_DllName = DLLName;
/*
	m_hModule = ::LoadLibraryEx(DLLName,NULL,LOAD_WITH_ALTERED_SEARCH_PATH);
	if (m_hModule)
	{
		size_t nameSize = _tcslen(DLLName)+1;
		m_DllName = new TCHAR[nameSize];
		_tcscpy_s(m_DllName,nameSize,DLLName);

		m_lpfnDllIsConnected			= (LPFNDLLISCONNECTED)			GetProcAddress(m_hModule,"IsConnected");
		m_lpfnDllBeginArm				= (LPFNDLLBEGINARM)				GetProcAddress(m_hModule,"BeginArm");
		m_lpfnDllBeginArmWithCalibration= (LPFNDLLBEGINARMWITHCALIBRATION)	GetProcAddress(m_hModule,"BeginArmWithCalibration");
		m_lpfnDllEndArm					= (LPFNDLLENDARM)				GetProcAddress(m_hModule,"EndArm");
		m_lpfnDllBeginAcquisition		= (LPFNDLLBEGINACQUISITION)		GetProcAddress(m_hModule,"BeginAcquisition");
		m_lpfnDllGetAcquisitionMode		= (LPFNDLLGETACQUISITIONMODE)	GetProcAddress(m_hModule,"GetAcquisitionMode");
		m_lpfnDllEndAcquisition			= (LPFNDLLENDACQUISITION)		GetProcAddress(m_hModule,"EndAcquisition");
		m_lpfnDllGetCurrentPosition		= (LPFNDLLGETCURRENTPOSITION)	GetProcAddress(m_hModule,"GetCurrentPosition");
		m_lpfnDllGetCurrentPositionAndJointAngles	= (LPFNDLLGETCURRENTPOSITIONANDJOINTANGLES)		GetProcAddress(m_hModule,"GetCurrentPositionAndJointAngles");
		m_lpfnDllGetTriggeredPosition	= (LPFNDLLGETTRIGGEREDPOSITION)	GetProcAddress(m_hModule,"GetTriggeredPosition");
		m_lpfnDllGetTriggeredPositionAndJointAngles	= (LPFNDLLGETTRIGGEREDPOSITIONANDJOINTANGLES)	GetProcAddress(m_hModule,"GetTriggeredPositionAndJointAngles");
		m_lpfnDllGetButtonState			= (LPFNDLLGETBUTTONSTATE)		GetProcAddress(m_hModule,"GetButtonState");
		m_lpfnDllGetParameter			= (LPFNDLLGETPARAMETER)			GetProcAddress(m_hModule,"GetParameter");
		m_lpfnDllSetParameter			= (LPFNDLLSETPARAMETER)			GetProcAddress(m_hModule,"SetParameter");
		m_lpfnDllHasPropertiesWindow	= (LPFNDLLHASPROPERTIESWINDOW)	GetProcAddress(m_hModule,"HasPropertiesWindow");
		m_lpfnDllPropertiesWindow		= (LPFNDLLPROPERTIESWINDOW)		GetProcAddress(m_hModule,"PropertiesWindow");
		m_lpfnDllProbeCalibration		= (LPFNDLLPROBECALIBRATION)		GetProcAddress(m_hModule,"ProbeCalibration");
		m_lpfnDllOpenKreonArmProbesManagement = (LPFNDLLOPENKREONARMPROBESMANAGEMENT)		GetProcAddress(m_hModule,"OpenKreonArmProbesManagement");

		if(	m_lpfnDllIsConnected			&&
			m_lpfnDllBeginArm				&&	m_lpfnDllEndArm					&&
			m_lpfnDllBeginAcquisition		&&	m_lpfnDllEndAcquisition			&&
			m_lpfnDllGetAcquisitionMode		&&
			m_lpfnDllGetCurrentPosition		&&	m_lpfnDllGetTriggeredPosition	&&
			m_lpfnDllGetButtonState			&&
			m_lpfnDllGetParameter			&&	m_lpfnDllSetParameter			&&
			m_lpfnDllHasPropertiesWindow	&&	m_lpfnDllPropertiesWindow		&&
			m_lpfnDllProbeCalibration)
			m_bDllOk = true;
		else
			printf("Error in CArmManager::CArmManager: Invalid arm DLL\n");
	}
	else
		printf("Error in CArmManager::CArmManager: LoadLibrary failed\n");
*/
}

/////////////////////////////////////////////////////////////////////////////
bool CArmManager::IsInitialized()
{
	return true;
}

bool CArmManager::IsConnected()
{
	return CArmManager::wrapper.IsConnected();
}

int CArmManager::BeginArm()
{
	return CArmManager::wrapper.BeginArm();
}

int	CArmManager::EndArm()
{
	return CArmManager::wrapper.EndArm();
}

int	CArmManager::BeginAcquisition(int Mode)
{
	return CArmManager::wrapper.BeginAcquisition(Mode);
}

int	CArmManager::GetAcquisitionMode(void)
{
	return CArmManager::wrapper.GetAcquisitionMode();
}

int	CArmManager::EndAcquisition()
{
	return CArmManager::wrapper.EndAcquisition();
}

int	CArmManager::GetCurrentPosition(double* Matrix,bool* ButtonState)
{
	return CArmManager::wrapper.GetCurrentPosition(Matrix, ButtonState);
}

int	CArmManager::GetTriggeredPosition(double* Matrix,bool* ButtonState)
{
	return CArmManager::wrapper.GetTriggeredPosition(Matrix, ButtonState);
}

int	CArmManager::GetButtonState(bool* ButtonState)
{
	return CArmManager::wrapper.GetButtonState(ButtonState);
}

bool CArmManager::GetParameter(char* Name,void* Value)
{
	return CArmManager::wrapper.GetParameter(Name, Value);
}

bool CArmManager::SetParameter(char* Name,void* Value)
{
	return CArmManager::wrapper.SetParameter(Name, Value);
}

bool CArmManager::HasPropertiesWindow(void)
{
	return CArmManager::wrapper.HasPropertiesWindow();
}

void CArmManager::PropertiesWindow(HWND hWndParent)
{
	return CArmManager::wrapper.PropertiesWindow(hWndParent);
}

int	CArmManager::ProbeCalibration(int nPositions, KR_POSITION Positions[], int CalibrationMethod, KR_TOUCH_PROBE * pProbe, double SphereDiameter)
{
	return CArmManager::wrapper.ProbeCalibration(nPositions, Positions, CalibrationMethod, pProbe, SphereDiameter);
}


/////////////////////////////////////////////////////////////////////////////
CArmManager::~CArmManager()
{
	printf("CArmManager::~CArmManager\n");

	if (m_hModule)
	{
//		if (m_lpfnDllIsConnected && IsConnected() && m_lpfnDllEndArm)
		if(IsConnected())
			EndArm();

		// Free the library unless it is KRMicroScribe.dll (because it uses FlyCapture2.dll that may crash when freed).
//		if (_wcsicmp(m_DllName,_T("KRMicroScribe.dll")))
			::FreeLibrary(m_hModule);
		m_hModule = NULL;
	}

//	if (m_DllName)
//		delete [] m_DllName;
}

/////////////////////////////////////////////////////////////////////////////
int CArmManager::GetNbButtons(void)
{
	if ( (m_NbButtons==-1) && m_lpfnDllGetParameter )
		GetParameter("NbButtons",&m_NbButtons);
	return m_NbButtons;
}

/////////////////////////////////////////////////////////////////////////////
void CArmManager::SetFrequency(int Frequency)
{
//	if (m_lpfnDllSetParameter)
		SetParameter("AcquisitionFrequency",&Frequency);
}

/////////////////////////////////////////////////////////////////////////////
DWORD CArmManager::GetTriggerDuration(void)
{
	if (m_TriggerDuration==0xFFFFFFFF)
	{
//		if (m_lpfnDllGetParameter)
			GetParameter("TriggerDuration",&m_TriggerDuration);

		if (m_TriggerDuration==0xFFFFFFFF)
			return 200;		//	Default value for most arms
	}

	return m_TriggerDuration;
}
