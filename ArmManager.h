//	-------------------------------------------------------------------------
//	ArmManager.h
//
//		Make the interface between any of the KREON arm dll and your program.
//
//	Methods :
//
//		CArmManager(TCHAR* ArmName)
//			ArmName is the name of the arm you want to work with
//			(Simu, Faro, Romer...)
//
//		IsInitialized
//			returns true if the Kreon arm DLL was correctly loaded, else false.
//
//		IsConnected
//			returns true if the arm is connected to the PC, else false
//
//		BeginArm
//			Initializes the arm and memory. You need to call this method
//			before using the arm.
//			returns KR_ARM_OK if the initialization has been correctly done
//			and a negative value if there was a problem.
//
//		EndArm
//			Frees memory and disconnects from the arm.
//			returns KR_ARM_OK if it is correct and a negative value if there
//			was a problem.
//
//		BeginAcquisition(int Mode)
//			Puts the arm in an acquisition mode. 4 acquisition modes are
//			available, but there are not supported by all arms :
//			1 : the arm sends his current position when you ask it
//				(with the GetCurrentPosition method).
//			2 : touch probe mode : the arm sends his position triggered by
//				a button (use GetTriggeredPosition)
//			3 : asynchronous scan mode (use GetTriggeredPosition)
//			4 : synchronous scan mode (use GetTriggeredPosition)
//			5 : used to scan the buttons (use GetButtonState)
//			returns :
//			KR_ARM_OK or any KR_ARM_* return code.
//
//		GetAcquisitionMode
//			Returns the current value of 'Mode', see BeginAcquisition
//
//		EndAcquisition
//			Stops the acquisition mode
//			returns 1 if it is correct and a negative value if there was a
//			problem.
//
//		GetCurrentPosition(double* Matrix,bool* ButtonState)
//			Used in mode 1 (see BeginAcquisition) to get the current position
//			of the arm.
//			Matrix : array of 16 doubles allocated by your program. The matrix
//			is organized with this order :
//			Matrix[0]	Matrix[4]	Matrix[8]	Matrix[12]
//			Matrix[1]	Matrix[5]	Matrix[9]	Matrix[13]
//			Matrix[2]	Matrix[6]	Matrix[10]	Matrix[14]
//			Matrix[3]	Matrix[7]	Matrix[11]	Matrix[15]
//			ButtonState : array of booleans allocated by your program (see
//			GetParameter to know how many buttons the arm has). This parameter
//			can be NULL if you only need to get the position.
//			returns :
//			KR_ARM_OK or any KR_ARM_* return code.
//
//		GetCurrentPositionAndJointAngles(double* Matrix,bool* ButtonState,int * NbJoints,double ** JointAngles,bool bDegrees)
//			Same as GetCurrentPosition with 3 extra parameters:
//			NbJoints points to an int that will receive the number of joints.
//			JointAngles points to a double* pointer that will receive the address of an array of double containing the angles.
//			Note that the array is allocated when BeginArm is called for the first time and reallocated at each subsequent BeginArm.
//			bDegrees is a boolean that tells the returned angles should be in degrees (or radians).
//			If the arm does not support these optional parameters, *NbJoints=0 and *JointAngles=NULL
//			and the function just acts like GetCurrentPosition.
//
//		GetTriggeredPosition(double* Matrix,bool* ButtonState)
//			Get the last position of the arm when it received a trigger signal from the scanner.
//			See GetCurrentPosition for details.
//
//		GetTriggeredPositionAndJointAngles(double* Matrix,bool* ButtonState,int * NbJoints,double ** JointAngles,bool bDegrees)
//			Same as GetTriggeredPosition with 3 extra parameters.
//			See GetCurrentPositionAndJointAngles for details.
//
//		GetButtonState(bool* ButtonState)
//			ButtonState : array of booleans allocated by your program (see
//			GetParameter to know how many buttons the arm has).
//			returns 1 if it is correct and a negative value if there was a
//			problem.
//
//		GetParameter(char* ParamName,char* ParamValue)
//			Allow your program to get some arm parameters.
//			ParamName is the name of the parameter, and ParamValue is a pointer to a variable whose type depends of ParamName.
//				ParamName					Type	Meaning
//				NbButtons					int		Number of buttons for this arm 
//				TriggerDuration				WORD	Duration of the trigger needed by the arm (in ?s)
//				AsynchronousModeSupported	bool	Tells whether the arm supports scanning in Asynchronous (fixed frequency) mode
//				ContinuousModeSupported		bool	Same as AsynchronousModeSupported
//				SynchronousModeSupported	bool	Tells whether the arm supports scanning in Synchronous (optimized frequency) mode
//				OneShotModeSupported		bool	Same as SynchronousModeSupported
//				OnlyOneThread				bool	Tells whether the arms does not support to be accessed from different threads (it is not recommended anyway)
//				ProbeDiameter				double  Diameter of the current probe (only for Kreon arms that support this feature)
//				ArmModel					char*	Static string giving the name of the arm (BeginArm must have been called first).
//					The currently known names are: "Arm Simulator", "Cimcore 3000i", "Cimcore Infinite", "Cimcore Infinite 2.0", "Cimcore Stinger II",
//					"Faro Advantage", "Faro Edge", "Faro Fusion", "Faro Gold", "Faro Millenium", "Faro Platinum", "Faro Prime", "Faro Quantum",
//					"Faro Silver", "Faro Titanium", "FriulRobot Baces3D", "Kreon Ace", "Kreon Baces", "MicroScribe", "NDI",
//					"Romer Absolute", "Romer Sigma/Omega", "Romer System6/Romer100/Armony", "Staubli Robot", "ZettMess"
//			returns true if the parameter is recognized, else false.
//
//		SetParameter(char* ParamName,char* Value)
//			Allow your program to set an arm parameter.
//			Not implemented yet.
//
//		ProbeCalibration(int nPositions, KR_POSITION Positions[], int CalibrationMethod, KR_TOUCH_PROBE * pProbe, double SphereDiameter)
//			Computes a probe calibration (pointed by pProbe) from the
//			nPositions positions stored in the Positions vector.
//			CalibrationMethod can be 1 (by sphere) or 2 (by hole).
//			SphereDiameter is required if calibration method 1 is chosen.
//			The returned value is an error code from:
//			- KR_ERR_NO_ERROR
//			- KR_ERR_PROBE_CALIBRATION_UNKNOWN_METHOD
//			- KR_ERR_PROBE_CALIBRATION_NOT_ENOUGH_DATA

//
//	---------------------------------------------------------------------------
#ifndef __ARM_MANAGER__
#define	__ARM_MANAGER__

#include <windows.h>
#include <tchar.h>
#include "KreonDefines.h"
#include "VXElementsWrapper.h"
#include "PA_Enums.hpp"
#include "PA_CB.hpp"

#include <thread>


/////////////////////////////////////////////////////////////////////////////
class CArmManager
{
public:
	CArmManager(TCHAR* ArmName, PA_Enums::PositioningDevice armType);
	~CArmManager();

	//	Functions located in the Kreon DLL
	bool	IsInitialized();// {return m_bDllOk; }
	bool	IsConnected();// {return m_lpfnDllIsConnected() != 0; }
	int		BeginArm();// {return m_lpfnDllBeginArm(); }
	int		EndArm();// {return m_lpfnDllEndArm(); }
	int		BeginAcquisition(int Mode);// {return m_lpfnDllBeginAcquisition(Mode); }
	int		GetAcquisitionMode(void);// {return m_lpfnDllGetAcquisitionMode(); }
	int		EndAcquisition();// { return m_lpfnDllEndAcquisition(); }
	int		GetCurrentPosition(double* Matrix, bool* ButtonState);// {return m_lpfnDllGetCurrentPosition(Matrix, ButtonState); }
	int		GetTriggeredPosition(double* Matrix, bool* ButtonState);// {return m_lpfnDllGetTriggeredPosition(Matrix, ButtonState); }
	int		GetButtonState(bool* ButtonState);// {return m_lpfnDllGetButtonState(ButtonState); }
	bool	GetParameter(/*const*/ char* Name, void* Value);// {return m_lpfnDllGetParameter(Name, Value); }
	bool	SetParameter(/*const*/ char* Name, void* Value);// {return m_lpfnDllSetParameter(Name, Value); }
	bool	HasPropertiesWindow(void);// {return m_lpfnDllHasPropertiesWindow(); }
	void	PropertiesWindow(HWND hWndParent);// {m_lpfnDllPropertiesWindow(hWndParent); }
	int		ProbeCalibration(int nPositions, KR_POSITION Positions[], int CalibrationMethod, KR_TOUCH_PROBE * pProbe, double SphereDiameter = 0.0);
//																	{return m_lpfnDllProbeCalibration(nPositions,Positions,CalibrationMethod,pProbe,SphereDiameter);}

	//	Local functions
	int		GetNbButtons(void);
	void	SetFrequency(int Frequency);
	DWORD	GetTriggerDuration(void);

private:
	// to download custom dll
	VXElementsWrapper wrapper;

	HINSTANCE	m_hModule;
	bool		m_bDllOk;
	int			m_NbButtons;
	DWORD		m_TriggerDuration;
	TCHAR *		m_DllName;

	PA_Enums::PositioningDevice m_ArmType;

	std::thread *m_rcvThread;
	std::thread *m_readCBThread;

	typedef bool (*LPFNDLLISCONNECTED)(void);
	LPFNDLLISCONNECTED			m_lpfnDllIsConnected;

	typedef int  (*LPFNDLLBEGINARM)(void);
	LPFNDLLBEGINARM				m_lpfnDllBeginArm;

	typedef int  (*LPFNDLLBEGINARMWITHCALIBRATION)(char * calibration);
	LPFNDLLBEGINARMWITHCALIBRATION	m_lpfnDllBeginArmWithCalibration;

	typedef int  (*LPFNDLLENDARM)(void);
	LPFNDLLENDARM				m_lpfnDllEndArm;

	typedef int  (*LPFNDLLBEGINACQUISITION)(int Mode);
	LPFNDLLBEGINACQUISITION		m_lpfnDllBeginAcquisition;

	typedef int	 (*LPFNDLLGETACQUISITIONMODE)(void);
	LPFNDLLGETACQUISITIONMODE	m_lpfnDllGetAcquisitionMode;

	typedef int  (*LPFNDLLENDACQUISITION)(void);
	LPFNDLLENDACQUISITION		m_lpfnDllEndAcquisition;

	typedef int  (*LPFNDLLGETCURRENTPOSITION)(double* Matrix,bool* ButtonState);
	LPFNDLLGETCURRENTPOSITION	m_lpfnDllGetCurrentPosition;

	typedef int  (*LPFNDLLGETCURRENTPOSITIONANDJOINTANGLES)(double* Matrix,bool* ButtonState,int * NbJoints,double ** JointAngles,bool bDegrees);
	LPFNDLLGETCURRENTPOSITIONANDJOINTANGLES	m_lpfnDllGetCurrentPositionAndJointAngles;

	typedef int  (*LPFNDLLGETTRIGGEREDPOSITION)(double* Matrix,bool* ButtonState);
	LPFNDLLGETTRIGGEREDPOSITION	m_lpfnDllGetTriggeredPosition;

	typedef int  (*LPFNDLLGETTRIGGEREDPOSITIONANDJOINTANGLES)(double* Matrix,bool* ButtonState,int * NbJoints,double ** JointAngles,bool bDegrees);
	LPFNDLLGETTRIGGEREDPOSITIONANDJOINTANGLES	m_lpfnDllGetTriggeredPositionAndJointAngles;

	typedef int  (*LPFNDLLGETBUTTONSTATE)(bool* ButtonState);
	LPFNDLLGETBUTTONSTATE		m_lpfnDllGetButtonState;

	typedef bool (*LPFNDLLGETPARAMETER)(const char* ParamName,void* ParamValue);
	LPFNDLLGETPARAMETER			m_lpfnDllGetParameter;

	typedef bool (*LPFNDLLSETPARAMETER)(const char* ParamName,void* ParamValue);
	LPFNDLLSETPARAMETER			m_lpfnDllSetParameter;

	typedef bool (*LPFNDLLHASPROPERTIESWINDOW)(void);
	LPFNDLLHASPROPERTIESWINDOW	m_lpfnDllHasPropertiesWindow;

	typedef void (*LPFNDLLPROPERTIESWINDOW)(HWND hWndParent);
	LPFNDLLPROPERTIESWINDOW		m_lpfnDllPropertiesWindow;

	typedef int (*LPFNDLLPROBECALIBRATION)(int nPositions, KR_POSITION Positions[], int CalibrationMethod, KR_TOUCH_PROBE * pProbe, double SphereDiameter);
	LPFNDLLPROBECALIBRATION		m_lpfnDllProbeCalibration;

	typedef int (*LPFNDLLOPENKREONARMPROBESMANAGEMENT)(void);
	LPFNDLLOPENKREONARMPROBESMANAGEMENT	m_lpfnDllOpenKreonArmProbesManagement;
};

void rcvCobotMessage(PA_Communication::CircularBuffer &cbRx);
void readBuffer(PA_Communication::CircularBuffer &cbRx);


#endif