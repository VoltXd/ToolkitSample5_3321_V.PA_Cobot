#pragma once

namespace PA_Enums
{
	enum PositioningDevice
	{
		CobotTx2Touch,
		Ctrack
	};

	enum CobotMessage
	{
		Acquire = 0xFEU,
		EndAcquire = 0xEFU,
		Position = 0x01U
	};
}
