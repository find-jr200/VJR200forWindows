////////////////////////////////////////////////////////////
// PC-8801 FDC Emulator
//
// Written by Manuke

#ifndef PC88Fdc_DEFINED
#define PC88Fdc_DEFINED

////////////////////////////////////////////////////////////
// declare

class CPC88Fdc;

////////////////////////////////////////////////////////////
// include
#include <vector>
#include <list>
#include "DiskImage.h"
#include <cereal/cereal.hpp>

////////////////////////////////////////////////////////////
// declaration of CPC88Fdc

class CPC88Fdc {
// typedef
public:
	// type of interrupt vector change callback function
	typedef void (*IntVectChangeCallback)();

// enum
protected:
	// phase
	enum {
		PHASE_COMMAND   = 1,
		PHASE_EXECUTION = 2,
		PHASE_RESULT    = 3
	};
	// command
	enum {
		COMMAND_READ_DATA           = 0x06,
		COMMAND_READ_DELETED_DATA   = 0x0C,
		COMMAND_READ_DIAGNOSTIC     = 0x02,
		COMMAND_READ_ID             = 0x0A,
		COMMAND_WRITE_DATA          = 0x05,
		COMMAND_WRITE_DELETED_DATA  = 0x09,
		COMMAND_WRITE_ID            = 0x0D,
		COMMAND_SCAN_EQUAL          = 0x11,
		COMMAND_SCAN_LOW_OR_EQUAL   = 0x19,
		COMMAND_SCAN_HIGH_OR_EQUAL  = 0x1D,
		COMMAND_SEEK                = 0x0F,
		COMMAND_RECALIBRATE         = 0x07,
		COMMAND_SENSE_INT_STATUS    = 0x08,
		COMMAND_SENSE_DEVICE_STATUS = 0x04,
		COMMAND_SPECIFY             = 0x03
	};
	// status
	enum {
		STATUS_FD0_BUSY       = 0x01,
		STATUS_FD1_BUSY       = 0x02,
		STATUS_FD2_BUSY       = 0x04,
		STATUS_FD3_BUSY       = 0x08,
		STATUS_FDC_BUSY       = 0x10,
		STATUS_NON_DMA_MODE   = 0x20,
		STATUS_DATA_IO        = 0x40,
		STATUS_REQ_FOR_MASTER = 0x80
	};
	// ST0
	enum {
		ST0_UNIT_SELECT     = 0x03,
		ST0_HEAD_ADDRESS    = 0x04,
		ST0_NOT_READY       = 0x08,
		ST0_EQUIPMENT_CHECK = 0x10,
		ST0_SEEK_END        = 0x20,
		ST0_INT_CODE        = 0xC0,
		ST0_INT_CODE_NT     = 0x00,
		ST0_INT_CODE_AT     = 0x40,
		ST0_INT_CODE_IC     = 0x80,
		ST0_INT_CODE_AI     = 0xC0
	};
	// ST1
	enum {
		ST1_MISS_ADDR_MARK  = 0x01,
		ST1_NOT_WRITABLE    = 0x02,
		ST1_NO_DATA         = 0x04,
		ST1_OVER_RUN        = 0x10,
		ST1_DATA_ERROR      = 0x20,
		ST1_END_OF_CYLINDER = 0x80
	};
	// ST2
	enum {
		ST2_MISS_ADDR_MARK_IN_DF = 0x01,
		ST2_BAD_CYLINDER         = 0x02,
		ST2_SCAN_NOT_SATISFIED   = 0x04,
		ST2_SCAN_EQUAL_HIT       = 0x08,
		ST2_NO_CYLINDER          = 0x10,
		ST2_DATA_ERROR_IN_DF     = 0x20,
		ST2_CONTROL_MARK         = 0x40
	};
	// ST3
	enum {
		ST3_UNIT_SELECT     = 0x03,
		ST3_HEAD_ADDRESS    = 0x04,
		ST3_TWO_SIDE        = 0x08,
		ST3_TRACK_0         = 0x10,
		ST3_READY           = 0x20,
		ST3_WRITE_PROTECTED = 0x40,
		ST3_FAULT           = 0x80
	};
	// intreserve
	enum {
		INTRESERVE_NONE   = -1,
		INTRESERVE_SEEK_0 = 0,
		INTRESERVE_SEEK_1 = 1,
		INTRESERVE_SEEK_2 = 2,
		INTRESERVE_SEEK_3 = 3
	};
	// disk information
	enum {
		DI_MFM_GAP4A_LENGTH = 80,
		DI_MFM_GAP1_LENGTH  = 50,
		DI_MFM_GAP2_LENGTH  = 22,
		DI_MFM_GAP3_LENGTH0 = 24,
		DI_MFM_GAP3_LENGTH1 = 54,
		DI_MFM_GAP3_LENGTH2 = 84,
		DI_MFM_GAP3_LENGTH3 = 116,
		DI_MFM_GAPX_LENGTH  = 80, // ???
		DI_MFM_SYNC_LENGTH  = 12,
		DI_MFM_IAM1_LENGTH  = 3,
		DI_MFM_IAM2_LENGTH  = 1,
		DI_MFM_IDAM1_LENGTH = 3,
		DI_MFM_IDAM2_LENGTH = 1,
		DI_MFM_DAM1_LENGTH  = 3,
		DI_MFM_DAM2_LENGTH  = 1,
		DI_MFM_GAP_DATA     = 0x4E,
		DI_MFM_SYNC_DATA    = 0x00,
		DI_MFM_IAM1_DATA    = 0xC2,
		DI_MFM_IAM2_DATA    = 0xFC,
		DI_MFM_IDAM1_DATA   = 0xA1,
		DI_MFM_IDAM2_DATA   = 0xFE,
		DI_MFM_DAM1_DATA    = 0xA1,
		DI_MFM_DAM2_DATA    = 0xFB,
		DI_MFM_DDAM2_DATA   = 0xF8,
		DI_FM_GAP4A_LENGTH = 40,
		DI_FM_GAP1_LENGTH  = 26,
		DI_FM_GAP2_LENGTH  = 11,
		DI_FM_GAP3_LENGTH0 = 27,
		DI_FM_GAP3_LENGTH1 = 42,
		DI_FM_GAP3_LENGTH2 = 58,
		DI_FM_GAP3_LENGTH3 = 74,
		DI_FM_GAPX_LENGTH  = 40, // ???
		DI_FM_SYNC_LENGTH  = 6,
		DI_FM_IAM1_LENGTH  = 0,
		DI_FM_IAM2_LENGTH  = 1,
		DI_FM_IDAM1_LENGTH = 0,
		DI_FM_IDAM2_LENGTH = 1,
		DI_FM_DAM1_LENGTH  = 0,
		DI_FM_DAM2_LENGTH  = 1,
		DI_FM_GAP_DATA     = 0xFF,
		DI_FM_SYNC_DATA    = 0x00,
		DI_FM_IAM1_DATA    = 0xC2,
		DI_FM_IAM2_DATA    = 0xFC,
		DI_FM_IDAM1_DATA   = 0xA1,
		DI_FM_IDAM2_DATA   = 0xFE,
		DI_FM_DAM1_DATA    = 0xA1,
		DI_FM_DAM2_DATA    = 0xFB,
		DI_FM_DDAM2_DATA   = 0xF8
	};

public:
	// FDC information
	enum {
		DRIVE_MAX = 2,
		INTERRUPT_DELAY = 50
	};

// class
protected:
	// declaration and implementation of CSectorInfo
	class CSectorInfo {
	// attribute
	protected:
		// C, H, R, N
		int m_nC, m_nH, m_nR, m_nN;
		// DDAM
		bool m_bDDAM;
		// data offset
		uint32_t m_dwDataOfs;
		// data length
		int m_nDataLength;

	public:
		// get C
		int GetC() const {
			return m_nC;
		}
		// get H
		int GetH() const {
			return m_nH;
		}
		// get R
		int GetR() const {
			return m_nR;
		}
		// get N
		int GetN() const {
			return m_nN;
		}
		// is DDAM
		bool IsDDAM() const {
			return m_bDDAM;
		}
		// get data offset
		uint32_t GetDataOfs() const {
			return m_dwDataOfs;
		}
		// get data length
		int GetDataLength() const {
			return m_nDataLength;
		}

	// create & destroy
	public:
		// default constructor
		CSectorInfo() :
			m_nC(0), m_nH(0),
			m_nR(0), m_nN(0),
			m_bDDAM(false),
			m_dwDataOfs(0),
			m_nDataLength(0)
		{
		}
		// copy constructor
		CSectorInfo(const CSectorInfo& siOther) :
			m_nC(siOther.m_nC), m_nH(siOther.m_nH),
			m_nR(siOther.m_nR), m_nN(siOther.m_nN),
			m_bDDAM(siOther.m_bDDAM),
			m_dwDataOfs(siOther.m_dwDataOfs),
			m_nDataLength(siOther.m_nDataLength)
		{
		}
		// constructor(parameter specified)
		CSectorInfo(
				int nC, int nH, int nR, int nN,
				bool bDDAM = false,
				uint32_t dwDataOfs = 0,
				int nDataLength = 0) :
			m_nC(nC), m_nH(nH),
			m_nR(nR), m_nN(nN),
			m_bDDAM(bDDAM),
			m_dwDataOfs(dwDataOfs),
			m_nDataLength(nDataLength)
		{
		}

	// operator
	public:
		// let
		CSectorInfo& operator=(const CSectorInfo& siOther) {
			m_nC = siOther.m_nC;
			m_nH = siOther.m_nH;
			m_nR = siOther.m_nR;
			m_nN = siOther.m_nN;
			m_bDDAM = siOther.m_bDDAM;
			m_dwDataOfs = siOther.m_dwDataOfs;
			m_nDataLength = siOther.m_nDataLength;
			return *this;
		}
	};

// attribute
protected:
	// drive count
	static int m_nDriveCount;
	// disk image
	static CDiskImage* m_apDiskImage[DRIVE_MAX];
	// data register
	static uint8_t m_btDataReg;
	// status register
	static uint8_t m_btStatusReg;
	// ST0-3
	static uint8_t m_abtST[4];
	// backup of ST0-3
	static uint8_t m_abtSTBackup[4];
	// phase
	static int m_nPhase;
	// command data
	static uint8_t m_abtCommands[9];
	// command data count
	static int m_nCommandCount;
	// result data
	static uint8_t m_abtResults[7];
	// result data count
	static int m_nResultCount;
	// result data pointer
	static int m_nResultPtr;
	// STEP signal gap time
	static int m_nStepRateTime;
	// head load time
	static int m_nHeadLoadTime;
	// head unload time
	static int m_nHeadUnloadTime;
	// non DMA mode
	static bool m_bNonDMAMode;
	// presented cylinder number
	static int m_anPresentCylinderNumber[DRIVE_MAX];
	// presented head address
	static int m_anPresentHeadAddress[DRIVE_MAX];
	// presented sector number
	static int m_anPresentSectorNumber[DRIVE_MAX];
	// sector count
	static int m_anSectorCount[DRIVE_MAX];
	// presented sector offset
	static uint32_t m_awPresentSectorOfs[DRIVE_MAX];
	// READ DIAG information
	static std::vector<uint8_t> m_vectReadDiagInfo;
	// sector information list
	static std::list<CSectorInfo> m_listSectorInfo;
	// executing drive number
	static int m_nExecDrive;
	// executing C
	static int m_nExecC;
	// executing H
	static int m_nExecH;
	// executing R
	static int m_nExecR;
	// executing N
	static int m_nExecN;
	// backup of executing C
	static int m_nExecCBackup;
	// backup of executing H
	static int m_nExecHBackup;
	// backup of executing R
	static int m_nExecRBackup;
	// backup of executing N
	static int m_nExecNBackup;
	// executing R2
	static int m_nExecR2;
	// backup of executing R2
	static int m_nExecR2Backup;
	// executing EOT
	static int m_nExecEOT;
	// executing GPL
	static int m_nExecGPL;
	// executing DTL
	static int m_nExecDTL;
	// executing MFM
	static bool m_bExecMFM;
	// executing multi track
	static bool m_bExecMultiTrack;
	// executing DAM
	static bool m_bExecDAM;
	// executing DDAM
	static bool m_bExecDDAM;
	// executing skip
	static bool m_bExecSkip;
	// executing data offset
	static uint32_t m_dwExecDataOfs;
	// executing data size
	static int m_nExecDataSize;
	// executing DDAM find
	static bool m_bExecFindDDAM;
	// executing end of cylinder
	static bool m_bExecEndOfCylinder;
	// executing CRC error id
	static bool m_bExecCRCErrorID;
	// executing CRC error data
	static bool m_bExecCRCErrorData;
	// executing sector first
	static bool m_bExecSectorFirst;
	// executing data error
	static bool m_bExecDataError;
	// executing  multi track moved
	static bool m_bExecMultiTrackMoved;
	// scan data matched
	static bool m_bScanMatched;
	// FDC interrupt requested
	static bool m_bFdcInterruptRequest;
	// interrupt delay
	static int m_nInterruptDelay;
	// interrupt wait
	static int m_nInterruptWait;

	// interrupt vector change callback function
	static IntVectChangeCallback m_pIntVectChangeCallback;

public:
	// get drive count
	static int GetDriveCount() {
		return m_nDriveCount;
	}
	// get disk image
	static CDiskImage* GetDiskImage(int nDrive) {
		return m_apDiskImage[nDrive];
	}
	// set disk image
	static void SetDiskImage(int nDrive, CDiskImage* pDiskImage) {
		m_apDiskImage[nDrive] = pDiskImage;
		ClearTrackInfo(nDrive);
	}
	// get command
	static int GetCommand() {
		return m_abtCommands[0] & 0x1F;
	}
	// set reult
	static void SetResult(uint8_t btResult) {
		m_abtResults[m_nResultCount++] = btResult;
	}
	// is drive equipped
	static bool IsDriveEquip(int nDrive) {
		return nDrive < m_nDriveCount;
	}
	// is drive ready
	static bool IsDriveReady(int nDrive) {
		return (nDrive < m_nDriveCount) && (m_apDiskImage[nDrive] != NULL);
	}
	// is FDC interrupt requested
	static bool IsFdcInterruptRequest() {
		return m_bFdcInterruptRequest;
	}
	// set FDC interrupt requested
	static void SetFdcInterruptRequest(bool bFdcInterruptRequest) {
		m_bFdcInterruptRequest = bFdcInterruptRequest;
	}
	// get interrupt delay
	static int GetInterruptDelay() {
		return m_nInterruptDelay;
	}
	// set interrupt delay
	static void SetInterruptDelay(int nInterruptDelay) {
		m_nInterruptDelay = nInterruptDelay;
	}
	// get interrupt wait
	static int GetInterruptWait() {
		return m_nInterruptWait;
	}

	// set interrupt vector change callback function
	static void SetIntVectChangeCallback(
		IntVectChangeCallback pIntVectChangeCallback)
	{
		m_pIntVectChangeCallback = pIntVectChangeCallback;
	}

// create & destroy
public:
	// default constructor
	CPC88Fdc();
	// destructor
	~CPC88Fdc();

// initialize
public:
	// initialize at first
	static void Initialize();
	// reset
	static void Reset();

// operation
protected:
	// update interrupt request
	static void UpdateInterruptRequest(bool bFdcInterruptRequest) {
		bool bFdcInterruptRequest2 = bFdcInterruptRequest;
		if (bFdcInterruptRequest2) {
			m_nInterruptWait = m_nInterruptDelay;
			if (m_nInterruptWait > 0) {
				bFdcInterruptRequest2 = false;
			}
		}
		if (bFdcInterruptRequest2 != m_bFdcInterruptRequest) {
			m_bFdcInterruptRequest = bFdcInterruptRequest2;
			m_pIntVectChangeCallback();
		}
	}

	// added by FIND 2022.6.26
	static void UpdateInterruptRequest(bool bFdcInterruptRequest, int intDelay) {
		bool bFdcInterruptRequest2 = bFdcInterruptRequest;
		if (bFdcInterruptRequest2) {
			m_nInterruptWait = intDelay;
			if (m_nInterruptWait > 0) {
				bFdcInterruptRequest2 = false;
			}
		}
		if (bFdcInterruptRequest2 != m_bFdcInterruptRequest) {
			m_bFdcInterruptRequest = bFdcInterruptRequest2;
			m_pIntVectChangeCallback();
		}
	}

	// set phase
	static void SetPhase(int nPhase) {
		m_nPhase = nPhase;
		if (m_nPhase == PHASE_COMMAND) {
			m_nCommandCount = 0;
			m_btStatusReg &=
				uint8_t(~(STATUS_FDC_BUSY | STATUS_DATA_IO | STATUS_NON_DMA_MODE));
		} else if (m_nPhase == PHASE_RESULT) {
			m_nResultCount = m_nResultPtr = 0;
			m_btStatusReg |= uint8_t(STATUS_FDC_BUSY | STATUS_DATA_IO);
			m_btStatusReg &= uint8_t(~STATUS_NON_DMA_MODE);
		} else {
			m_btStatusReg |= STATUS_FDC_BUSY | STATUS_NON_DMA_MODE;
		}
	}
	// build ST0
	static void BuildST0(int nDrive, bool bSeekEnd, int nInterruptCode) {
		m_abtST[0] = uint8_t(
			nDrive |
			((!bSeekEnd && IsDriveReady(nDrive) &&
					(m_anPresentHeadAddress[nDrive] != 0))?
				ST0_HEAD_ADDRESS: 0) |
			(bSeekEnd?
				(ST0_SEEK_END | nInterruptCode):
				(IsDriveReady(nDrive)?
					nInterruptCode:
					(ST0_NOT_READY |
						(((nInterruptCode == ST0_INT_CODE_NT)?
							ST0_INT_CODE_AT: nInterruptCode))))) |
			(!IsDriveEquip(nDrive)? ST0_EQUIPMENT_CHECK: 0));
	}
	// build ST1
	static void BuildST1(
		bool bMissingAddressMark,
		bool bNotWritable,
		bool bNoData,
		bool bDataError,
		bool bEndOfCylinder)
	{
		m_abtST[1] = uint8_t(
			(bMissingAddressMark? ST1_MISS_ADDR_MARK: 0) |
			(bNotWritable? ST1_NOT_WRITABLE: 0) |
			(bNoData? ST1_NO_DATA: 0) |
			(bDataError? ST1_DATA_ERROR: 0) |
			(bEndOfCylinder? ST1_END_OF_CYLINDER: 0));
	}
	// build ST2
	static void BuildST2(
		bool bMissingAddressMarkInDataField,
		bool bBadCylinder,
		bool bScanNotSatisfied,
		bool bScanEqualHit,
		bool bNoCylinder,
		bool bDataErrorInDataField,
		bool bControlMark)
	{
		m_abtST[2] = uint8_t(
			(bMissingAddressMarkInDataField? ST2_MISS_ADDR_MARK_IN_DF: 0) |
			(bBadCylinder? ST2_BAD_CYLINDER: 0) |
			(bScanNotSatisfied? ST2_SCAN_NOT_SATISFIED: 0) |
			(bScanEqualHit? ST2_SCAN_EQUAL_HIT: 0) |
			(bNoCylinder? ST2_NO_CYLINDER: 0) |
			(bDataErrorInDataField? ST2_DATA_ERROR_IN_DF: 0) |
			(bControlMark? ST2_CONTROL_MARK: 0));
	}
	// build ST3
	static void BuildST3(int nDrive) {
		m_abtST[3] = uint8_t(
			nDrive |
			(IsDriveReady(nDrive)?
				(ST3_READY |
					(m_apDiskImage[nDrive]->IsWriteProtected()?
						ST3_WRITE_PROTECTED: 0) |
					((m_anPresentCylinderNumber[nDrive] == 0)?
						ST3_TRACK_0: 0)):
				0) |
			(IsDriveEquip(nDrive)? ST3_TWO_SIDE: ST3_FAULT));
	}
	// backup status
	static void BackupStatus();
	// restore status
	static void RestoreStatus();
	// clear track info
	static void ClearTrackInfo(int nDrive);
	// initialize track info
	static bool InitTrackInfo(int nDrive);
	// do seek
	static void DoSeek(int nDrive, int nCylinder, int nHeadAddress);
	// find sector
	static bool FindSector();
	// build READ DIAG information
	static bool BuildReadDiagInfo();
	// format track
	static bool FormatTrack(int nSC, int nD);

public:
	// set drive count
	static void SetDriveCount(int nDriveCount);
	// pass clock
	static void PassClock(int nClock) {
		if (m_nInterruptWait > 0) {
			if ((m_nInterruptWait -= nClock) <= 0) {
				m_bFdcInterruptRequest = true;
				m_pIntVectChangeCallback();
			}
		}

		// added by FIND
		if (readWait > 0) {
			if (readWait -= nClock <= 0) {
				m_btStatusReg |= STATUS_REQ_FOR_MASTER;
			}
		}
	}

	// read status register
	static uint8_t ReadStatusReg() {
		return m_btStatusReg;
	}
	//read data register
	static uint8_t ReadDataReg();
	// write data register
	static void WriteDataReg(uint8_t btDataReg);
	// terminal count
	static void TerminalCount();

	static void SetLed(bool b);

	static void SetChangeStatus(bool b)
	{
		bChangeStatus = b;

		if (bChangePolling && bChangeStatus) {

			SetIntrCode();
			UpdateInterruptRequest(true);
		}
	}

	static bool GetChangeStatus()
	{
		return bChangeStatus;
	}

	static void SetIntrCode();

	template <class Archive>
	void serialize(Archive & ar, std::uint32_t const version);

protected:
	// added by FIND
	static const int SRT_UNIT_TIME = 8000; // 2ms     ステップレートタイムの基本単位
	static const int READ_WAIT = 195; // 195clock = 1バイト読み書き48.8us
	static const int RESET_WAIT = 5000; // 1.25ms

	static bool bChangeStatus;
	static bool bChangePolling;
	static int readWait;
};



template<class Archive>
inline void CPC88Fdc::serialize(Archive & ar, std::uint32_t const version)
{
	ar(m_nDriveCount, m_btDataReg, m_btStatusReg);

	for (int i = 0; i < 4; ++i) {
		ar(m_abtST[i]);
		ar(m_abtSTBackup[i]);
	}

	ar(m_nPhase);

	for (int i = 0; i < 9; ++i) {
		ar(m_abtCommands[i]);
	}

	ar(m_nCommandCount);

	for (int i = 0; i < 7; ++i) {
		ar(m_abtResults[i]);
	}

	ar(m_nResultCount, m_nResultPtr, m_nStepRateTime, m_nHeadLoadTime, m_nHeadUnloadTime, m_bNonDMAMode);

	for (int i = 0; i < DRIVE_MAX; ++i) {
		ar(m_anPresentCylinderNumber[i]);
		ar(m_anPresentHeadAddress[i]);
		ar(m_anPresentSectorNumber[i]);
		ar(m_anSectorCount[i]);
		ar(m_awPresentSectorOfs[i]);
	}

	ar(m_nExecDrive, m_nExecC, m_nExecH, m_nExecR, m_nExecN);
	ar(m_nExecCBackup, m_nExecHBackup, m_nExecRBackup, m_nExecNBackup, m_nExecR2, m_nExecR2Backup);
	ar(m_nExecEOT, m_nExecGPL, m_nExecDTL, m_bExecMFM, m_bExecMultiTrack, m_bExecDAM, m_bExecDDAM);
	ar(m_bExecSkip, m_dwExecDataOfs, m_nExecDataSize, m_bExecFindDDAM, m_bExecEndOfCylinder, m_bExecCRCErrorID);
	ar(m_bExecCRCErrorData, m_bExecSectorFirst, m_bExecDataError, m_bExecMultiTrackMoved, m_bScanMatched);
	ar(m_bFdcInterruptRequest, m_nInterruptDelay, m_nInterruptWait);
	ar(bChangeStatus, bChangePolling, readWait);
};


#endif // PC88Fdc_DEFINED
