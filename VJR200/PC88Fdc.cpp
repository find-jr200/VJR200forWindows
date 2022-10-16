////////////////////////////////////////////////////////////
// PC-8801 FDC Emulator
//
// Written by Manuke

////////////////////////////////////////////////////////////
// include
#include "stdafx.h"
#include "StdHeader.h"

#include "PC88Fdc.h"
#include "JRSystem.h"
#include "VJR200.h"


extern JRSystem sys;

////////////////////////////////////////////////////////////
// implementation of CPC88Fdc

////////////////////////////////////////////////////////////
// attribute

// drive count

int CPC88Fdc::m_nDriveCount;

// disk image

CDiskImage* CPC88Fdc::m_apDiskImage[DRIVE_MAX];

// data register

uint8_t CPC88Fdc::m_btDataReg;

// status register

uint8_t CPC88Fdc::m_btStatusReg;

// ST0-3

uint8_t CPC88Fdc::m_abtST[4];

// backup of ST0-3

uint8_t CPC88Fdc::m_abtSTBackup[4];

// phase

int CPC88Fdc::m_nPhase;

// command data

uint8_t CPC88Fdc::m_abtCommands[9];

// command data count

int CPC88Fdc::m_nCommandCount;

// result data

uint8_t CPC88Fdc::m_abtResults[7];

// result data count

int CPC88Fdc::m_nResultCount;

// result data pointer

int CPC88Fdc::m_nResultPtr;

// STEP signal gap time

int CPC88Fdc::m_nStepRateTime;

// head load time

int CPC88Fdc::m_nHeadLoadTime;

// head unload time

int CPC88Fdc::m_nHeadUnloadTime;

// non DMA mode

bool CPC88Fdc::m_bNonDMAMode;

// presented cylinder number

int CPC88Fdc::m_anPresentCylinderNumber[DRIVE_MAX];

// presented head address

int CPC88Fdc::m_anPresentHeadAddress[DRIVE_MAX];

// presented sector number

int CPC88Fdc::m_anPresentSectorNumber[DRIVE_MAX];

// sector count

int CPC88Fdc::m_anSectorCount[DRIVE_MAX];

// presented sector offset

uint32_t CPC88Fdc::m_awPresentSectorOfs[DRIVE_MAX];

// READ DIAG information

std::vector<uint8_t> CPC88Fdc::m_vectReadDiagInfo;

// sector information list

std::list<CPC88Fdc::CSectorInfo> CPC88Fdc::m_listSectorInfo;

// executing drive number

int CPC88Fdc::m_nExecDrive;

// executing C

int CPC88Fdc::m_nExecC;

// executing H

int CPC88Fdc::m_nExecH;

// executing R

int CPC88Fdc::m_nExecR;

// executing N

int CPC88Fdc::m_nExecN;

// backup of executing C

int CPC88Fdc::m_nExecCBackup;

// backup of executing H

int CPC88Fdc::m_nExecHBackup;

// backup of executing R

int CPC88Fdc::m_nExecRBackup;

// backup of executing N

int CPC88Fdc::m_nExecNBackup;

// executing R2

int CPC88Fdc::m_nExecR2;

// backup of executing R2

int CPC88Fdc::m_nExecR2Backup;

// executing EOT

int CPC88Fdc::m_nExecEOT;

// executing GPL

int CPC88Fdc::m_nExecGPL;

// executing DTL

int CPC88Fdc::m_nExecDTL;

// executing MFM

bool CPC88Fdc::m_bExecMFM;

// executing multi track

bool CPC88Fdc::m_bExecMultiTrack;

// executing DAM

bool CPC88Fdc::m_bExecDAM;

// executing DDAM

bool CPC88Fdc::m_bExecDDAM;

// executing skip

bool CPC88Fdc::m_bExecSkip;

// executing data offset

uint32_t CPC88Fdc::m_dwExecDataOfs;

// executing data size

int CPC88Fdc::m_nExecDataSize;

// executing DDAM find

bool CPC88Fdc::m_bExecFindDDAM;

// executing end of cylinder

bool CPC88Fdc::m_bExecEndOfCylinder;

// executing CRC error id

bool CPC88Fdc::m_bExecCRCErrorID;

// executing CRC error data

bool CPC88Fdc::m_bExecCRCErrorData;

// executing sector first

bool CPC88Fdc::m_bExecSectorFirst;

// executing data error

bool CPC88Fdc::m_bExecDataError;

// executing  multi track moved

bool CPC88Fdc::m_bExecMultiTrackMoved;

// scan data matched

bool CPC88Fdc::m_bScanMatched;

// FDC interrupt requested

bool CPC88Fdc::m_bFdcInterruptRequest;

// interrupt delay

int CPC88Fdc::m_nInterruptDelay;

// interrupt wait

int CPC88Fdc::m_nInterruptWait;

// interrupt vector change callback function

CPC88Fdc::IntVectChangeCallback CPC88Fdc::m_pIntVectChangeCallback;

// added by FIND
bool CPC88Fdc::bChangeStatus = false;
int CPC88Fdc::readWait = 0;
bool CPC88Fdc::bChangePolling = false;

////////////////////////////////////////////////////////////
// create & destroy

// default constructor

CPC88Fdc::CPC88Fdc() {
	m_nDriveCount = 2;
	for (int nDrive = 0; nDrive < DRIVE_MAX; nDrive++) {
		m_apDiskImage[nDrive] = NULL;
	}
	m_pIntVectChangeCallback = NULL;
}

// destructor

CPC88Fdc::~CPC88Fdc() {
}

////////////////////////////////////////////////////////////
// initialize

// initialize at first

void CPC88Fdc::Initialize() {
	for (int nDrive = 0; nDrive < DRIVE_MAX; nDrive++) {
		m_anPresentCylinderNumber[nDrive] = 0;
		m_anPresentHeadAddress[nDrive] = 0;
		ClearTrackInfo(nDrive);
	}
	m_nInterruptDelay = INTERRUPT_DELAY;

	bChangeStatus = false;
	bChangePolling = false;
}

// reset

void CPC88Fdc::Reset() {
	m_btStatusReg = uint8_t(STATUS_REQ_FOR_MASTER);
	m_btDataReg = 0x00;
	m_nStepRateTime = m_nHeadLoadTime = m_nHeadUnloadTime = 0;
	m_bNonDMAMode = true;
	m_vectReadDiagInfo.clear();
	m_listSectorInfo.clear();
	SetPhase(PHASE_COMMAND);
	m_bFdcInterruptRequest = false;
	m_nInterruptWait = 0;
	bChangeStatus = false;

	if (IsDriveReady(0) || IsDriveReady(1)) {
		SetIntrCode();
		UpdateInterruptRequest(true, RESET_WAIT);
	}
}

// added by FIND
void CPC88Fdc::SetIntrCode() {
	bool rd0 = IsDriveReady(0), rd1 = IsDriveReady(1);
	if (rd0 || rd1) {
		uint8_t a = 0xc0;
		uint8_t a1 = rd1 ? 1 : 0;
		uint8_t a2 = rd0 && rd1 ? 1 : 0;
		m_abtST[0] = a + a1 + a2;
	}
}


////////////////////////////////////////////////////////////
// operation

// backup status

void CPC88Fdc::BackupStatus() {
	memcpy(m_abtSTBackup, m_abtST, sizeof(m_abtST));
	m_nExecCBackup = m_nExecC;
	m_nExecHBackup = m_nExecH;
	m_nExecRBackup = m_nExecR;
	m_nExecNBackup = m_nExecN;
	m_nExecR2Backup = m_nExecR2;
}

// restore status

void CPC88Fdc::RestoreStatus() {
	memcpy(m_abtST, m_abtSTBackup, sizeof(m_abtST));
	m_nExecC = m_nExecCBackup;
	m_nExecH = m_nExecHBackup;
	m_nExecR = m_nExecRBackup;
	m_nExecN = m_nExecNBackup;
	m_nExecR2 = m_nExecR2Backup;
}

// clear track info

void CPC88Fdc::ClearTrackInfo(int nDrive) {
	m_anPresentSectorNumber[nDrive] = 0;
	m_anSectorCount[nDrive] = -1;
	m_awPresentSectorOfs[nDrive] = 0;
	m_vectReadDiagInfo.clear();
	m_listSectorInfo.clear();
}

// initialize track info

bool CPC88Fdc::InitTrackInfo(int nDrive) {
	m_anSectorCount[nDrive] = 0;
	if (IsDriveReady(nDrive)) {
		int nTrack = m_anPresentCylinderNumber[nDrive]*2+
			m_anPresentHeadAddress[nDrive];
		if (nTrack < CDiskImage::TRACK_MAX) {
			uint32_t dwTrackSize = m_apDiskImage[nDrive]->GetTrackSize(nTrack);
			if (dwTrackSize >= 16+1) {
				UGptr gptr = m_apDiskImage[nDrive]->GetTrackImagePtr(nTrack)+4;
				uint8_t btMFM, btDAM;
				uint16_t wSectorCount;
				gptr >> wSectorCount >> btMFM >> btDAM;
				if ((wSectorCount > 0) && (wSectorCount <= 32) &&
					((btMFM == 0x00) || (btMFM == 0x40)) &&
					((btDAM == 0x00) || (btDAM == 0x10)))
				{
					m_anSectorCount[nDrive] = wSectorCount;
				}
			}
		}
	}
	return m_anSectorCount[nDrive] > 0;
}

// do seek

void CPC88Fdc::DoSeek(int nDrive, int nCylinder, int nHeadAddress) {
	int nCylinderNew = nCylinder;
	if (nCylinderNew < 0) {
		nCylinderNew = m_anPresentCylinderNumber[nDrive];
	}
	int nHeadAddresNew = nHeadAddress;
	if (nHeadAddresNew < 0) {
		nHeadAddresNew = m_anPresentHeadAddress[nDrive];
	}
	if ((nCylinderNew != m_anPresentCylinderNumber[nDrive]) ||
		(nHeadAddresNew != m_anPresentHeadAddress[nDrive]))
	{
		m_anPresentCylinderNumber[nDrive] = nCylinderNew;
		m_anPresentHeadAddress[nDrive] = nHeadAddresNew;
		ClearTrackInfo(nDrive);
	}
}

// find sector

bool CPC88Fdc::FindSector() {
	m_bExecMultiTrackMoved = false;
	if (!IsDriveEquip(m_nExecDrive) || !IsDriveReady(m_nExecDrive)) {
		BuildST0(m_nExecDrive, false, ST0_INT_CODE_AT);
		BuildST1(true, false, true, false, false);
		BuildST2(true, false, false, false, false, false, false);
		return false;
	}
	while (true) {
		if (m_anSectorCount[m_nExecDrive] < 0) {
			InitTrackInfo(m_nExecDrive);
		}
		if (m_anSectorCount[m_nExecDrive] <= 0) {
			BuildST0(m_nExecDrive, false, ST0_INT_CODE_AT);
			BuildST1(true, false, true, false, false);
			BuildST2(true, false, false, false, false, false, false);
			return false;
		}
		UGptr gptr;
		int nTrack = m_anPresentCylinderNumber[m_nExecDrive]*2+
			m_anPresentHeadAddress[m_nExecDrive];
		uint8_t* pbtTrack = m_apDiskImage[m_nExecDrive]->GetTrackImagePtr(nTrack);
		uint16_t wSectorCount, wSectorSize = 0;
		uint32_t dwDataOfs = 0;
		uint8_t* pbtSector = pbtTrack+m_awPresentSectorOfs[m_nExecDrive];
		bool bMissingAddressMark = true,
			bBadCylinder = false, bNoCylinder = false;
		uint8_t btC = 0, btH = 0, btR = 0, btN = 0;
		int nSector;
		for (
			nSector = 0;
			nSector < m_anSectorCount[m_nExecDrive];
			nSector++)
		{
			bool bBreak = false;
			gptr = pbtSector;
			uint8_t btMFM, btDAM, btStatus;
			gptr >> btC >> btH >> btR >> btN;
			gptr >> wSectorCount >> btMFM >> btDAM >> btStatus;
			(void)wSectorCount; // not use
			gptr.m_pByte += 5;
			gptr >> wSectorSize;
			if ((m_bExecMFM && (btMFM == 0x00)) ||
				(!m_bExecMFM && (btMFM != 0x00)))
			{
				bMissingAddressMark = false;
				if (m_nExecC < 0) {
					bBreak = true;
				} else if (
					(btC == m_nExecC) && (btH == m_nExecH) &&
					(btR == m_nExecR) && (btN == m_nExecN))
				{
					if ((m_bExecDAM && (btDAM == 0x00)) ||
						(m_bExecDDAM && (btDAM != 0x00)))
					{
						bBreak = true;
					} else {
						m_bExecFindDDAM = true;
						if (!m_bExecSkip) {
							bBreak = true;
						}
					}
				} else {
					if (btC != m_nExecC) {
						if (btC == 0xFF) {
							bBadCylinder = true;
						} else {
							bNoCylinder = true;
						}
					}
				}
			}
			dwDataOfs = gptr.GetDiff(pbtTrack);
			gptr.m_pByte += wSectorSize;
			if (++m_anPresentSectorNumber[m_nExecDrive] >=
					m_anSectorCount[m_nExecDrive])
			{
				m_anPresentSectorNumber[m_nExecDrive] = 0;
				gptr = pbtTrack;
			}
			pbtSector = gptr.m_pByte;
			m_awPresentSectorOfs[m_nExecDrive] = gptr.GetDiff(pbtTrack);
			if (bBreak) {
				if ((btStatus & 0xF0) == 0xA0) {
					m_bExecCRCErrorID = true;
				} else if ((btStatus & 0xF0) == 0xA0) {
					m_bExecCRCErrorData = true;
				}
				break;
			}
		}
		if (nSector >= m_anSectorCount[m_nExecDrive]) {
			if (!m_bExecMultiTrack ||
				(m_anPresentHeadAddress[m_nExecDrive] != 0))
			{
				BuildST0(m_nExecDrive, false, ST0_INT_CODE_AT);
				BuildST1(false, false, true, false, true);
				BuildST2(
					bMissingAddressMark, bBadCylinder, false, false,
					bNoCylinder, false, false);
				return false;
			}
			m_bExecMultiTrackMoved = true;
			m_nExecR2 = 1;
			DoSeek(m_nExecDrive, -1, 1);
		} else if (m_bExecCRCErrorID) {
			BuildST0(m_nExecDrive, false, ST0_INT_CODE_AT);
			BuildST1(false, false, false, true, true);
			BuildST2(
				false, false, false,
				false, false, false, false);
			return false;
		} else {
			m_dwExecDataOfs = dwDataOfs;
			if ((m_nExecN != 0) || (m_nExecDTL < 1) || (m_nExecDTL > 128)) {
				m_nExecDataSize = wSectorSize;
			} else {
				m_nExecDataSize = m_nExecDTL;
			}
			if (!m_bExecCRCErrorData) {
				if (m_nExecC >= 0) {
					m_nExecR++;
					if (m_nExecR > m_nExecEOT) {
						m_nExecR = 1;
						if (m_bExecMultiTrack) {
							if (m_nExecH == 0) {
								m_nExecH++;
							} else {
								m_nExecH = 0;
								m_nExecC++;
							}
						} else {
							m_nExecC++;
						}
					}
					m_nExecR2++;
					if (m_nExecR2 > m_anSectorCount[m_nExecDrive]) {
						if (m_bExecMultiTrack &&
							(m_anPresentHeadAddress[m_nExecDrive] == 0))
						{
							m_nExecR2 = 1;
						} else {
							m_bExecEndOfCylinder = true;
						}
					}
				} else {
					m_nExecC = btC;
					m_nExecH = btH;
					m_nExecR = btR;
					m_nExecN = btN;
				}
			}
			BuildST0(
				m_nExecDrive, false,
				m_bExecCRCErrorData?
					ST0_INT_CODE_AT: ST0_INT_CODE_NT);
			BuildST1(
				false, false, false, m_bExecCRCErrorData, false);
			BuildST2(
				false, false, false, false,
				false, m_bExecCRCErrorData, m_bExecFindDDAM);
			break;
		}
	}
	return true;
}

// build READ DIAG information

bool CPC88Fdc::BuildReadDiagInfo() {
	bool bResult = false;
	ClearTrackInfo(m_nExecDrive);
	do {
		m_bExecMultiTrack = false;
		m_bExecSkip = false;
		m_nExecC = -1;
		m_nExecH = -1;
		m_nExecR = -1;
		m_nExecR2 = -1;
		m_nExecN = -1;
		m_nExecEOT = 100;
		m_nExecGPL = 0;
		m_nExecDTL = 0;
		m_bExecDAM = true;
		m_bExecDDAM = true;
		m_dwExecDataOfs = 0;
		m_nExecDataSize = 0;
		m_bExecFindDDAM = false;
		m_bExecEndOfCylinder = false;
		m_bExecCRCErrorID = m_bExecCRCErrorData = false;
		m_bExecSectorFirst = true;
		m_bExecDataError = false;
		bool bResult = FindSector();
		if (bResult) {
			m_listSectorInfo.push_back(
				CSectorInfo(
					m_nExecC, m_nExecH, m_nExecR, m_nExecN,
					m_bExecFindDDAM,
					m_dwExecDataOfs, m_nExecDataSize));
		}
	} while (bResult);
	int nSectorCount = (int)m_listSectorInfo.size();
	if (nSectorCount > 0) {
		int nTrack = m_anPresentCylinderNumber[m_nExecDrive]*2+
			m_anPresentHeadAddress[m_nExecDrive];
		uint8_t* pbtTrack =
			m_apDiskImage[m_nExecDrive]->GetTrackImagePtr(nTrack);
		int nGap1Length, nGap4aLength, nGap2Length, nGapXLength,
			anGap3Length[4],
			nSyncLength, nIAM1Length, nIAM2Length, nIDAM1Length, nIDAM2Length,
			nDAM1Length, nDAM2Length;
		uint8_t btGapData, btSyncData, btIAM1Data, btIAM2Data,
			btIDAM1Data, btIDAM2Data, btDAM1Data, btDAM2Data, btDDAM2Data;
		if (m_bExecMFM) {
			nGap4aLength    = DI_MFM_GAP4A_LENGTH;
			nGap1Length     = DI_MFM_GAP1_LENGTH;
			nGap2Length     = DI_MFM_GAP2_LENGTH;
			nGapXLength     = DI_MFM_GAPX_LENGTH;
			anGap3Length[0] = DI_MFM_GAP3_LENGTH0;
			anGap3Length[1] = DI_MFM_GAP3_LENGTH1;
			anGap3Length[2] = DI_MFM_GAP3_LENGTH2;
			anGap3Length[3] = DI_MFM_GAP3_LENGTH3;
			nSyncLength     = DI_MFM_SYNC_LENGTH;
			nIAM1Length     = DI_MFM_IAM1_LENGTH;
			nIAM2Length     = DI_MFM_IAM2_LENGTH;
			nIDAM1Length    = DI_MFM_IDAM1_LENGTH;
			nIDAM2Length    = DI_MFM_IDAM2_LENGTH;
			nDAM1Length     = DI_MFM_DAM1_LENGTH;
			nDAM2Length     = DI_MFM_DAM2_LENGTH;
			btGapData       = DI_MFM_GAP_DATA;
			btSyncData      = DI_MFM_SYNC_DATA;
			btIAM1Data      = DI_MFM_IAM1_DATA;
			btIAM2Data      = DI_MFM_IAM2_DATA;
			btIDAM1Data     = DI_MFM_IDAM1_DATA;
			btIDAM2Data     = DI_MFM_IDAM2_DATA;
			btDAM1Data      = DI_MFM_DAM1_DATA;
			btDAM2Data      = DI_MFM_DAM2_DATA;
			btDDAM2Data     = DI_MFM_DDAM2_DATA;
		} else {
			nGap4aLength    = DI_FM_GAP4A_LENGTH;
			nGap1Length     = DI_FM_GAP1_LENGTH;
			nGap2Length     = DI_FM_GAP2_LENGTH;
			nGapXLength     = DI_FM_GAPX_LENGTH;
			anGap3Length[0] = DI_FM_GAP3_LENGTH0;
			anGap3Length[1] = DI_FM_GAP3_LENGTH1;
			anGap3Length[2] = DI_FM_GAP3_LENGTH2;
			anGap3Length[3] = DI_FM_GAP3_LENGTH3;
			nSyncLength     = DI_FM_SYNC_LENGTH;
			nIAM1Length     = DI_FM_IAM1_LENGTH;
			nIAM2Length     = DI_FM_IAM2_LENGTH;
			nIDAM1Length    = DI_FM_IDAM1_LENGTH;
			nIDAM2Length    = DI_FM_IDAM2_LENGTH;
			nDAM1Length     = DI_FM_DAM1_LENGTH;
			nDAM2Length     = DI_FM_DAM2_LENGTH;
			btGapData       = DI_FM_GAP_DATA;
			btSyncData      = DI_FM_SYNC_DATA;
			btIAM1Data      = DI_FM_IAM1_DATA;
			btIAM2Data      = DI_FM_IAM2_DATA;
			btIDAM1Data     = DI_FM_IDAM1_DATA;
			btIDAM2Data     = DI_FM_IDAM2_DATA;
			btDAM1Data      = DI_FM_DAM1_DATA;
			btDAM2Data      = DI_FM_DAM2_DATA;
			btDDAM2Data     = DI_FM_DDAM2_DATA;
		}
		uint32_t dwTotalSize = nGap4aLength+nSyncLength+
			nIAM1Length+nIAM2Length+nGap1Length;
		std::list<CSectorInfo>::iterator itSectorInfo;
		m_dwExecDataOfs = 0;
		for (
			itSectorInfo = m_listSectorInfo.begin();
			itSectorInfo != m_listSectorInfo.end();
			itSectorInfo++)
		{
			int nParamN = (*itSectorInfo).GetN();
			if (nParamN > 3) {
				nParamN = 3;
			}
			if (m_dwExecDataOfs <= 0) {
				m_dwExecDataOfs = dwTotalSize+nSyncLength+
					nIDAM1Length+nIDAM2Length+
					4+2+nGap2Length+
					nSyncLength+nDAM1Length+nDAM2Length;
			}
			dwTotalSize += nSyncLength+nIDAM1Length+nIDAM2Length+
				4+2+nGap2Length+
				nSyncLength+nDAM1Length+nDAM2Length+
				(*itSectorInfo).GetDataLength()+2+anGap3Length[nParamN];
		}
		dwTotalSize += nGapXLength;
		m_vectReadDiagInfo.resize(dwTotalSize, 0x00);
		std::vector<uint8_t>::iterator itDiagInfo = m_vectReadDiagInfo.begin();
		int i;
		for (i = 0; i < nGap4aLength; i++) {
			*(itDiagInfo++) = btGapData;
		}
		for (i = 0; i < nSyncLength; i++) {
			*(itDiagInfo++) = btSyncData;
		}
		for (i = 0; i < nIAM1Length; i++) {
			*(itDiagInfo++) = btIAM1Data;
		}
		for (i = 0; i < nIAM2Length; i++) {
			*(itDiagInfo++) = btIAM2Data;
		}
		for (i = 0; i < nGap1Length; i++) {
			*(itDiagInfo++) = btGapData;
		}
		for (
			itSectorInfo = m_listSectorInfo.begin();
			itSectorInfo != m_listSectorInfo.end();
			itSectorInfo++)
		{
			for (i = 0; i < nSyncLength; i++) {
				*(itDiagInfo++) = btSyncData;
			}
			for (i = 0; i < nIDAM1Length; i++) {
				*(itDiagInfo++) = btIDAM1Data;
			}
			for (i = 0; i < nIDAM2Length; i++) {
				*(itDiagInfo++) = btIDAM2Data;
			}
			*(itDiagInfo++) = (uint8_t)(*itSectorInfo).GetC();
			*(itDiagInfo++) = (uint8_t)(*itSectorInfo).GetH();
			*(itDiagInfo++) = (uint8_t)(*itSectorInfo).GetR();
			*(itDiagInfo++) = (uint8_t)(*itSectorInfo).GetN();
			// CRC
			*(itDiagInfo++) = 0;
			*(itDiagInfo++) = 0;
			for (i = 0; i < nGap2Length; i++) {
				*(itDiagInfo++) = btGapData;
			}
			for (i = 0; i < nSyncLength; i++) {
				*(itDiagInfo++) = btSyncData;
			}
			for (i = 0; i < nDAM1Length; i++) {
				*(itDiagInfo++) = btDAM1Data;
			}
			for (i = 0; i < nDAM2Length; i++) {
				*(itDiagInfo++) = (uint8_t)
					((*itSectorInfo).IsDDAM()? btDDAM2Data: btDAM2Data);
			}
			uint8_t* pbtData = pbtTrack+(*itSectorInfo).GetDataOfs();
			for (i = 0; i < (*itSectorInfo).GetDataLength(); i++) {
				*(itDiagInfo++) = *(pbtData++);
			}
			// CRC
			*(itDiagInfo++) = 0;
			*(itDiagInfo++) = 0;
			int nParamN = (*itSectorInfo).GetN();
			if (nParamN > 3) {
				nParamN = 3;
			}
			for (i = 0; i < anGap3Length[nParamN]; i++) {
				*(itDiagInfo++) = btGapData;
			}
		}
	}
	return nSectorCount > 0;
}

// format track

bool CPC88Fdc::FormatTrack(int nSC, int nD) {
	ClearTrackInfo(m_nExecDrive);
	if (!IsDriveReady(m_nExecDrive) ||
		m_apDiskImage[m_nExecDrive]->IsWriteProtected())
	{
		BuildST0(m_nExecDrive, false, ST0_INT_CODE_AT);
		BuildST1(false, true, false, false, false);
		BuildST2(false, false, false, false, false, false, false);
		return false;
	}
	int nTrack = m_anPresentCylinderNumber[m_nExecDrive]*2+
		m_anPresentHeadAddress[m_nExecDrive];
	if (nTrack >= CDiskImage::TRACK_MAX) {
		BuildST0(m_nExecDrive, false, ST0_INT_CODE_AT);
		BuildST1(false, true, false, false, false);
		BuildST2(false, false, false, false, false, false, false);
		return false;
	}
	uint16_t wSectorCount = (uint16_t)nSC, wSectorSize = 0;
	if ((m_nExecN >= 0x00) && (m_nExecN <= 0x06)) {
		wSectorSize = (uint16_t)(128 << m_nExecN);
	}
	uint32_t dwTrackSize = m_apDiskImage[m_nExecDrive]->GetTrackSize(nTrack);
	if ((wSectorCount <= 0) ||
		(wSectorSize <= 0) ||
		((uint32_t)((16+wSectorSize)*wSectorCount) > dwTrackSize))
	{
		BuildST0(m_nExecDrive, false, ST0_INT_CODE_AT);
		BuildST1(false, true, false, false, false);
		BuildST2(false, false, false, false, false, false, false);
		return false;
	}
	UGptr gptr = m_apDiskImage[m_nExecDrive]->GetTrackImagePtr(nTrack);
	memset(gptr.m_pByte, 0x00, dwTrackSize);
	uint8_t btC = 0xFF, btH = 0xFF, btR = 0xFF, btN = 0xFF,
		btMFM = (uint8_t)(m_bExecMFM? 0x00: 0x40),
		btDAM = 0x00,
		btStatus = 0x00,
		btDummy = 0x00;
	for (int nSector = 0; nSector < wSectorCount; nSector++) {
		gptr << btC << btH << btR << btN <<
			wSectorCount <<
			btMFM << btDAM << btStatus <<
			btDummy << btDummy << btDummy << btDummy << btDummy <<
			wSectorSize;
		memset(gptr.m_pByte, nD, wSectorSize);
		gptr.m_pByte += wSectorSize;
	}
	if (!InitTrackInfo(m_nExecDrive)) {
		BuildST0(m_nExecDrive, false, ST0_INT_CODE_AT);
		BuildST1(false, true, false, false, false);
		BuildST2(false, false, false, false, false, false, false);
		return false;
	}
	BuildST0(m_nExecDrive, false, ST0_INT_CODE_NT);
	BuildST1(false, false, false, false, false);
	BuildST2(false, false, false, false, false, false, false);
	return true;
}

// set drive count

void CPC88Fdc::SetDriveCount(int nDriveCount) {
	m_nDriveCount = nDriveCount;
	for (int nDrive = m_nDriveCount; nDrive < DRIVE_MAX; nDrive++) {
		m_apDiskImage[nDrive] = NULL;
		m_anPresentCylinderNumber[nDrive] = 0;
		m_anPresentHeadAddress[nDrive] = 0;
		ClearTrackInfo(nDrive);
	}
}

//read data register

uint8_t CPC88Fdc::ReadDataReg() {
	UpdateInterruptRequest(false);
	if (m_nPhase == PHASE_EXECUTION) {
		switch (GetCommand()) {
		case COMMAND_READ_DATA:
		case COMMAND_READ_DELETED_DATA:
			m_bExecSectorFirst = false;
			if (m_bExecDataError) {
				m_btDataReg = 0;
				m_nExecDataSize--;
			} else if (
				!IsDriveReady(m_nExecDrive) ||
				(m_anSectorCount[m_nExecDrive] <= 0))
			{
				m_bExecDataError = true;
				m_btDataReg = 0;
				m_nExecDataSize--;
				m_bExecEndOfCylinder = false;
				BuildST0(m_nExecDrive, false, ST0_INT_CODE_AT);
				BuildST1(false, false, false, true, false);
				BuildST2(
					true, false, false, false,
					false, true, m_bExecFindDDAM);
			} else {
				int nTrack = m_anPresentCylinderNumber[m_nExecDrive]*2+
					m_anPresentHeadAddress[m_nExecDrive];
				const uint8_t* pbtTrack =
					m_apDiskImage[m_nExecDrive]->GetTrackImagePtr(nTrack);
				m_btDataReg = *(pbtTrack+m_dwExecDataOfs);
				m_dwExecDataOfs++;
				if (--m_nExecDataSize <= 0) {
					m_bExecSectorFirst = true;
					BackupStatus();
					if (!m_bExecCRCErrorData && !m_bExecEndOfCylinder) {
						FindSector();
					} else {
						BuildST0(
							m_nExecDrive, false,
							ST0_INT_CODE_AT);
						BuildST1(
							false, false, false,
							m_bExecCRCErrorData, m_bExecCRCErrorData);
						BuildST2(
							false, false, false, false,
							false, m_bExecCRCErrorData, m_bExecFindDDAM);
					}
				}
			}
			if (m_nExecDataSize <= 0) {
				SetPhase(PHASE_RESULT);
				SetResult(m_abtST[0]);
				SetResult(m_abtST[1]);
				SetResult(m_abtST[2]);
				SetResult(uint8_t(m_nExecC));
				SetResult(uint8_t(m_nExecH));
				SetResult(uint8_t(m_nExecR));
				SetResult(uint8_t(m_nExecN));

				m_btStatusReg &= 0x7f;
				readWait = READ_WAIT;
				UpdateInterruptRequest(true);
				SetLed(true);
			}
			break;
		case COMMAND_READ_DIAGNOSTIC:
			if (!IsDriveReady(m_nExecDrive) ||
				(m_anSectorCount[m_nExecDrive] <= 0))
			{
				m_btDataReg = 0x00;
				BuildST0(m_nExecDrive, false, ST0_INT_CODE_AT);
				BuildST1(false, false, false, true, false);
				BuildST2(
					true, false, false, false,
					false, true, false);
			} else {
				m_btDataReg = m_vectReadDiagInfo[m_dwExecDataOfs++];
				if (m_dwExecDataOfs >= m_vectReadDiagInfo.size()) {
					m_dwExecDataOfs = 0;
				}
			}
			SetLed(true);
			break;
		}
	} else if (m_nPhase == PHASE_RESULT) {
		m_btDataReg = m_abtResults[m_nResultPtr++];
		if (m_nResultPtr >= m_nResultCount) {
			SetPhase(PHASE_COMMAND);
		}
	}
	return m_btDataReg;
}

// write data register

void CPC88Fdc::WriteDataReg(uint8_t btDataReg) {
	UpdateInterruptRequest(false);
	m_btDataReg = btDataReg;
	if (m_nPhase == PHASE_RESULT) {
		SetPhase(PHASE_COMMAND);
	}
	if (m_nPhase == PHASE_COMMAND) { // *********************************** PHASE_COMMAND ***********************************
		m_btStatusReg |= STATUS_FDC_BUSY;
		m_abtCommands[m_nCommandCount++] = btDataReg;
		switch (GetCommand()) {
		case COMMAND_READ_DATA:
		case COMMAND_READ_DELETED_DATA:
			if (m_nCommandCount >= 9) {
				m_nExecDrive = m_abtCommands[1] & 0x03;
				m_bExecMultiTrack = ((m_abtCommands[0] & 0x80) != 0);
				m_bExecMFM = ((m_abtCommands[0] & 0x40) != 0);
				m_bExecSkip = ((m_abtCommands[0] & 0x20) != 0);
				m_nExecC = m_abtCommands[2];
				m_nExecH = m_abtCommands[3];
				m_nExecR = m_abtCommands[4];
				m_nExecR2 = 1;
				m_nExecN = m_abtCommands[5];
				m_nExecEOT = m_abtCommands[6];
				m_nExecGPL = m_abtCommands[7];
				m_nExecDTL = m_abtCommands[8];
				m_bExecDAM = (GetCommand() == COMMAND_READ_DATA);
				m_bExecDDAM = (GetCommand() == COMMAND_READ_DELETED_DATA);
				m_dwExecDataOfs = 0;
				m_nExecDataSize = 0;
				m_bExecFindDDAM = false;
				m_bExecEndOfCylinder = false;
				m_bExecCRCErrorID = m_bExecCRCErrorData = false;
				m_bExecSectorFirst = true;
				m_bExecDataError = false;
				BackupStatus();
				DoSeek(
					m_nExecDrive,
					-1, ((m_abtCommands[1] & 0x04) != 0)? 1: 0);
				bool bResult = FindSector();
				if (!bResult) {
					SetPhase(PHASE_RESULT);
					SetResult(m_abtST[0]);
					SetResult(m_abtST[1]);
					SetResult(m_abtST[2]);
					SetResult(uint8_t(m_nExecC));
					SetResult(uint8_t(m_nExecH));
					SetResult(uint8_t(m_nExecR));
					SetResult(uint8_t(m_nExecN));
				} else {
					SetPhase(PHASE_EXECUTION);
					m_btStatusReg |= STATUS_DATA_IO;
				}
				UpdateInterruptRequest(true);
				SetLed(true);
			}
			break;
		case COMMAND_READ_DIAGNOSTIC:
			if (m_nCommandCount >= 9) {
				m_nExecDrive = m_abtCommands[1] & 0x03;
				m_bExecMFM = ((m_abtCommands[0] & 0x40) != 0);
				bool bResult = BuildReadDiagInfo();
				if (!bResult) {
					SetPhase(PHASE_RESULT);
					SetResult(m_abtST[0]);
					SetResult(m_abtST[1]);
					SetResult(m_abtST[2]);
					SetResult(uint8_t(m_nExecC));
					SetResult(uint8_t(m_nExecH));
					SetResult(uint8_t(m_nExecR));
					SetResult(uint8_t(m_nExecN));
				} else {
					SetPhase(PHASE_EXECUTION);
					m_btStatusReg |= STATUS_DATA_IO;
				}
				UpdateInterruptRequest(true);
				SetLed(true);
			}
			break;
		case COMMAND_READ_ID:
			if (m_nCommandCount >= 2) {
				m_nExecDrive = m_abtCommands[1] & 0x03;
				m_bExecMultiTrack =false;
				m_bExecMFM = ((m_abtCommands[0] & 0x40) != 0);
				m_bExecSkip = false;
				m_nExecC = -1;
				m_nExecH = -1;
				m_nExecR = m_nExecR2 = -1;
				m_nExecN = -1;
				m_nExecEOT = 999;
				m_nExecGPL = 0;
				m_nExecDTL = 0xFF;
				m_bExecDAM = true;
				m_bExecDDAM = true;
				m_dwExecDataOfs = 0;
				m_nExecDataSize = 0;
				m_bExecFindDDAM = false;
				m_bExecEndOfCylinder = false;
				m_bExecCRCErrorID = m_bExecCRCErrorData = false;
				m_bExecSectorFirst = true;
				m_bExecDataError = false;
				DoSeek(
					m_nExecDrive,
					-1, ((m_abtCommands[1] & 0x04) != 0)? 1: 0);
				bool bResult = FindSector();
				if (bResult) {
					BuildST0(
						m_nExecDrive, false,
						ST0_INT_CODE_NT);
					BuildST1(false, false, false, false, false);
					BuildST2(
						false, false, false, false,
						false, false, false);
				}
				SetPhase(PHASE_RESULT);
				SetResult(m_abtST[0]);
				SetResult(m_abtST[1]);
				SetResult(m_abtST[2]);
				SetResult(uint8_t(m_nExecC));
				SetResult(uint8_t(m_nExecH));
				SetResult(uint8_t(m_nExecR));
				SetResult(uint8_t(m_nExecN));
				UpdateInterruptRequest(true);
				SetLed(true);
			}
			break;
		case COMMAND_WRITE_DATA:
		case COMMAND_WRITE_DELETED_DATA:
			if (m_nCommandCount >= 9) {
				m_nExecDrive = m_abtCommands[1] & 0x03;
				m_bExecMultiTrack = ((m_abtCommands[0] & 0x80) != 0);
				m_bExecMFM = ((m_abtCommands[0] & 0x40) != 0);
				m_bExecSkip = ((m_abtCommands[0] & 0x20) != 0);
				m_nExecC = m_abtCommands[2];
				m_nExecH = m_abtCommands[3];
				m_nExecR = m_abtCommands[4];
				m_nExecR2 = 1;
				m_nExecN = m_abtCommands[5];
				m_nExecEOT = m_abtCommands[6];
				m_nExecGPL = m_abtCommands[7];
				m_nExecDTL = m_abtCommands[8];
				m_bExecDAM = (GetCommand() == COMMAND_WRITE_DATA);
				m_bExecDDAM = (GetCommand() == COMMAND_WRITE_DELETED_DATA);
				m_dwExecDataOfs = 0;
				m_nExecDataSize = 0;
				m_bExecFindDDAM = false;
				m_bExecEndOfCylinder = false;
				m_bExecCRCErrorID = m_bExecCRCErrorData = false;
				m_bExecSectorFirst = true;
				m_bExecDataError = false;
				bool bResult;
				if (!IsDriveReady(m_nExecDrive) ||
					m_apDiskImage[m_nExecDrive]->IsWriteProtected())
				{
					BuildST0(m_nExecDrive, false, ST0_INT_CODE_AT);
					BuildST1(false, true, false, false, false);
					BuildST2(true, false, false, false, false, false, false);
					bResult = false;
				} else {
					BackupStatus();
					DoSeek(
						m_nExecDrive,
						-1, ((m_abtCommands[1] & 0x04) != 0)? 1: 0);
					bResult = FindSector();
				}
				if (!bResult) {
					SetPhase(PHASE_RESULT);
					SetResult(m_abtST[0]);
					SetResult(m_abtST[1]);
					SetResult(m_abtST[2]);
					SetResult(uint8_t(m_nExecC));
					SetResult(uint8_t(m_nExecH));
					SetResult(uint8_t(m_nExecR));
					SetResult(uint8_t(m_nExecN));
				} else {
					SetPhase(PHASE_EXECUTION);
				}
				UpdateInterruptRequest(true);
				SetLed(true);
			}
			break;
		case COMMAND_WRITE_ID: // FORMAT
			if (m_nCommandCount >= 6) {
				m_nExecDrive = m_abtCommands[1] & 0x03;
				m_bExecMFM = ((m_abtCommands[0] & 0x40) != 0);
				m_nExecN = m_abtCommands[2];
				m_nExecGPL = m_abtCommands[4];
				int nSC = m_abtCommands[3],
					nD = m_abtCommands[5];
				m_nExecDataSize = nSC*4;
				m_dwExecDataOfs = 0;
				m_bExecDataError = false;
				DoSeek(
					m_nExecDrive,
					-1, ((m_abtCommands[1] & 0x04) != 0)? 1: 0);
				bool bResult = FormatTrack(nSC, nD);
				if (!bResult) {
					SetPhase(PHASE_RESULT);
					SetResult(m_abtST[0]);
					SetResult(m_abtST[1]);
					SetResult(m_abtST[2]);
					SetResult(m_abtCommands[2]);
					SetResult(m_abtCommands[3]);
					SetResult(m_abtCommands[4]);
					SetResult(m_abtCommands[5]);
				} else {
					SetPhase(PHASE_EXECUTION);
				}
				UpdateInterruptRequest(true);
				SetLed(true);
			}
			break;
		case COMMAND_SCAN_EQUAL:
		case COMMAND_SCAN_LOW_OR_EQUAL:
		case COMMAND_SCAN_HIGH_OR_EQUAL:
			if (m_nCommandCount >= 9) {
				m_nExecDrive = m_abtCommands[1] & 0x03;
				m_bExecMultiTrack = ((m_abtCommands[0] & 0x80) != 0);
				m_bExecMFM = ((m_abtCommands[0] & 0x40) != 0);
				m_bExecSkip = ((m_abtCommands[0] & 0x20) != 0);
				m_nExecC = m_abtCommands[2];
				m_nExecH = m_abtCommands[3];
				m_nExecR = m_abtCommands[4];
				m_nExecR2 = 1;
				m_nExecN = m_abtCommands[5];
				m_nExecEOT = m_abtCommands[6];
				m_nExecGPL = m_abtCommands[7];
				m_nExecDTL = m_abtCommands[8];
				m_bExecDAM = true;
				m_bExecDDAM = false;
				m_dwExecDataOfs = 0;
				m_nExecDataSize = 0;
				m_bExecFindDDAM = false;
				m_bExecEndOfCylinder = false;
				m_bExecCRCErrorID = m_bExecCRCErrorData = false;
				m_bExecSectorFirst = true;
				m_bExecDataError = false;
				m_bScanMatched = false;
				bool bResult;
				BackupStatus();
				DoSeek(
					m_nExecDrive,
					-1, ((m_abtCommands[1] & 0x04) != 0)? 1: 0);
				bResult = FindSector();
				if (!bResult) {
					SetPhase(PHASE_RESULT);
					SetResult(m_abtST[0]);
					SetResult(m_abtST[1]);
					SetResult(m_abtST[2]);
					SetResult(uint8_t(m_nExecC));
					SetResult(uint8_t(m_nExecH));
					SetResult(uint8_t(m_nExecR));
					SetResult(uint8_t(m_nExecN));
				} else {
					SetPhase(PHASE_EXECUTION);
				}
				UpdateInterruptRequest(true);
			}
			break;
		case COMMAND_SEEK:
		case COMMAND_RECALIBRATE:
			if (m_nCommandCount >=
					((GetCommand() == COMMAND_SEEK)? 3: 2))
			{
				int nDrive = m_abtCommands[1] & 0x03;
				if (IsDriveEquip(nDrive)) {
					DoSeek(
						nDrive,
						(GetCommand() == COMMAND_SEEK)? m_abtCommands[2]: 0,
						((m_abtCommands[1] & ST3_HEAD_ADDRESS) != 0)? 1: 0);
				}
				BuildST0(nDrive, true, ST0_INT_CODE_NT);
				SetPhase(PHASE_COMMAND);

				// added by FIND
				int srt = 0x10 - m_nStepRateTime;
				int c = abs(m_anPresentCylinderNumber[nDrive] - m_abtCommands[2]);
				UpdateInterruptRequest(true, ++c * srt * SRT_UNIT_TIME);
			}
			break;
		case COMMAND_SENSE_INT_STATUS:
			SetPhase(PHASE_RESULT);

			if (((m_abtST[0] & ST0_INT_CODE) == ST0_INT_CODE) ||
				((m_abtST[0] & ST0_SEEK_END) != 0))
			{
				int nDrive = m_abtST[0] & ST0_UNIT_SELECT;
				SetResult(m_abtST[0]);
				SetResult(uint8_t(m_anPresentCylinderNumber[nDrive]));

			} else {
				 SetResult(0x80);
			}
			bChangeStatus = false;
			m_abtST[0] = 0;
			break;
		case COMMAND_SENSE_DEVICE_STATUS:
			if (m_nCommandCount >= 2) {
				int nDrive = m_abtCommands[1] & 0x03;
				m_anPresentHeadAddress[nDrive] =
					((m_abtCommands[1] & ST3_HEAD_ADDRESS) != 0)? 1: 0;
				BuildST3(nDrive);
				SetPhase(PHASE_RESULT);
				SetResult(m_abtST[3]);
				UpdateInterruptRequest(true);
			}
			break;
		case COMMAND_SPECIFY:
			if (m_nCommandCount >= 3) {
				m_nStepRateTime = (m_abtCommands[1] & 0xF0) >> 4;
				m_nHeadLoadTime = (m_abtCommands[2] & 0xFE) >> 1;
				m_nHeadUnloadTime = m_abtCommands[1] & 0x0F;
				m_bNonDMAMode = ((m_abtCommands[2] & 0x01) != 0);
				SetPhase(PHASE_COMMAND);
				
				bChangePolling = true;
			}
			break;
		default:
			SetPhase(PHASE_RESULT);
			SetResult(0x80);
			UpdateInterruptRequest(true);
			break;
		}
	} else if (m_nPhase == PHASE_EXECUTION) { // *********************************** PHASE_EXECUTION ***********************************
		switch (GetCommand()) {
		case COMMAND_WRITE_DATA:
		case COMMAND_WRITE_DELETED_DATA:
			m_bExecSectorFirst = false;
			if (m_bExecDataError) {
				m_nExecDataSize--;
			} else if (
				!IsDriveReady(m_nExecDrive) ||
				m_apDiskImage[m_nExecDrive]->IsWriteProtected() ||
				(m_anSectorCount[m_nExecDrive] <= 0))
			{
				m_bExecDataError = true;
				m_nExecDataSize--;
				m_bExecEndOfCylinder = false;
				BuildST0(m_nExecDrive, false, ST0_INT_CODE_AT);
				BuildST1(false, false, false, true, false);
				BuildST2(
					true, false, false, false,
					false, true, m_bExecFindDDAM);
			} else {
				int nTrack = m_anPresentCylinderNumber[m_nExecDrive]*2+
					m_anPresentHeadAddress[m_nExecDrive];
				uint8_t* pbtTrack =
					m_apDiskImage[m_nExecDrive]->GetTrackImagePtr(nTrack);
				*(pbtTrack+m_dwExecDataOfs) = m_btDataReg;
				m_dwExecDataOfs++;
				if ((--m_nExecDataSize <= 0) &&
					!m_bExecCRCErrorData && !m_bExecEndOfCylinder)
				{
					m_bExecSectorFirst = true;
					BackupStatus();
					FindSector();
				}
			}
			if (m_nExecDataSize <= 0) {
				SetPhase(PHASE_RESULT);
				SetResult(m_abtST[0]);
				SetResult(m_abtST[1]);
				SetResult(m_abtST[2]);
				SetResult(uint8_t(m_nExecC));
				SetResult(uint8_t(m_nExecH));
				SetResult(uint8_t(m_nExecR));
				SetResult(uint8_t(m_nExecN));
				UpdateInterruptRequest(true);
				SetLed(true);
			}
			break;
		case COMMAND_WRITE_ID:
			if (m_bExecDataError) {
				m_nExecDataSize--;
			} else if (
				!IsDriveReady(m_nExecDrive) ||
				m_apDiskImage[m_nExecDrive]->IsWriteProtected() ||
				(m_anSectorCount[m_nExecDrive] <= 0))
			{
				m_bExecDataError = true;
				m_nExecDataSize--;
				BuildST0(m_nExecDrive, false, ST0_INT_CODE_AT);
				BuildST1(false, true, false, false, false);
				BuildST2(false, false, false, false, false, false, false);
			} else {
				int nTrack = m_anPresentCylinderNumber[m_nExecDrive]*2+
					m_anPresentHeadAddress[m_nExecDrive];
				uint8_t* pbtTrack =
					m_apDiskImage[m_nExecDrive]->GetTrackImagePtr(nTrack);
				*(pbtTrack+m_dwExecDataOfs) = m_btDataReg;
				m_dwExecDataOfs++;
				m_nExecDataSize--;
				if (m_nExecDataSize%4 == 0) {
					m_dwExecDataOfs += 10;
					m_dwExecDataOfs += *(uint16_t*)(pbtTrack+m_dwExecDataOfs);
					m_dwExecDataOfs += 2;
				}
			}
			if (m_nExecDataSize <= 0) {
				SetPhase(PHASE_RESULT);
				SetResult(m_abtST[0]);
				SetResult(m_abtST[1]);
				SetResult(m_abtST[2]);
				SetResult(uint8_t(m_nExecC));
				SetResult(uint8_t(m_nExecH));
				SetResult(uint8_t(m_nExecR));
				SetResult(uint8_t(m_nExecN));
				UpdateInterruptRequest(true);
			}
			SetLed(true);
			break;
		case COMMAND_SCAN_EQUAL:
		case COMMAND_SCAN_LOW_OR_EQUAL:
		case COMMAND_SCAN_HIGH_OR_EQUAL:
			if (m_bExecSectorFirst) {
				m_bScanMatched = true;
			}
			m_bExecSectorFirst = false;
			if (m_bExecDataError) {
				m_nExecDataSize--;
			} else if (
				!IsDriveReady(m_nExecDrive) ||
				(m_anSectorCount[m_nExecDrive] <= 0))
			{
				m_bExecDataError = true;
				m_nExecDataSize--;
				m_bExecEndOfCylinder = false;
				BuildST0(m_nExecDrive, false, ST0_INT_CODE_AT);
				BuildST1(false, false, false, true, false);
				BuildST2(
					true, false, false, false,
					false, true, m_bExecFindDDAM);
			} else {
				int nTrack = m_anPresentCylinderNumber[m_nExecDrive]*2+
					m_anPresentHeadAddress[m_nExecDrive];
				const uint8_t* pbtTrack =
					m_apDiskImage[m_nExecDrive]->GetTrackImagePtr(nTrack);
				uint8_t btDiskData = *(pbtTrack+m_dwExecDataOfs);
				if (m_btDataReg != 0xFF) {
					switch (GetCommand()) {
					case COMMAND_SCAN_EQUAL:
						if (btDiskData != m_btDataReg) {
							m_bScanMatched = false;
						}
						break;
					case COMMAND_SCAN_LOW_OR_EQUAL:
						if (btDiskData > m_btDataReg) {
							m_bScanMatched = false;
						}
						break;
					case COMMAND_SCAN_HIGH_OR_EQUAL:
						if (btDiskData < m_btDataReg) {
							m_bScanMatched = false;
						}
						break;
					}
				}
				m_dwExecDataOfs++;
				if (--m_nExecDataSize <= 0) {
					if (m_bExecCRCErrorData) {
						// nothing
					} else if (m_bScanMatched || m_bExecEndOfCylinder) {
						BuildST0(
							m_nExecDrive, false, ST0_INT_CODE_NT);
						BuildST1(
							false, false, false, false, false);
						BuildST2(
							false, false,
							!m_bScanMatched, m_bScanMatched,
							false, false, false);
					} else {
						m_bExecSectorFirst = true;
						BackupStatus();
						FindSector();
					}
				}
			}
			if (m_nExecDataSize <= 0) {
				SetPhase(PHASE_RESULT);
				SetResult(m_abtST[0]);
				SetResult(m_abtST[1]);
				SetResult(m_abtST[2]);
				SetResult(uint8_t(m_nExecC));
				SetResult(uint8_t(m_nExecH));
				SetResult(uint8_t(m_nExecR));
				SetResult(uint8_t(m_nExecN));
				UpdateInterruptRequest(true);
			}
			break;
		}
	}
}

// terminal count

void CPC88Fdc::TerminalCount() {
	if (m_nPhase == PHASE_EXECUTION) {
		switch (GetCommand()) {
		case COMMAND_READ_DATA:
		case COMMAND_READ_DELETED_DATA:
		case COMMAND_WRITE_DATA:
		case COMMAND_WRITE_DELETED_DATA:
			if (m_bExecSectorFirst) {
				if (m_bExecMultiTrackMoved) {
					DoSeek(m_nExecDrive, -1, 0);
				}
				RestoreStatus();
			}
			SetPhase(PHASE_RESULT);
			SetResult(m_abtST[0]);
			SetResult(m_abtST[1]);
			SetResult(m_abtST[2]);
			SetResult(uint8_t(m_nExecC));
			SetResult(uint8_t(m_nExecH));
			SetResult(uint8_t(m_nExecR));
			SetResult(uint8_t(m_nExecN));
			UpdateInterruptRequest(true);
			break;
		case COMMAND_READ_DIAGNOSTIC:
			m_vectReadDiagInfo.clear();
			m_listSectorInfo.clear();
			SetPhase(PHASE_RESULT);
			SetResult(m_abtST[0]);
			SetResult(m_abtST[1]);
			SetResult(m_abtST[2]);
			SetResult(uint8_t(m_nExecC));
			SetResult(uint8_t(m_nExecH));
			SetResult(uint8_t(m_nExecR));
			SetResult(uint8_t(m_nExecN));
			UpdateInterruptRequest(true);
			break;
		case COMMAND_WRITE_ID:
			SetPhase(PHASE_RESULT);
			SetResult(m_abtST[0]);
			SetResult(m_abtST[1]);
			SetResult(m_abtST[2]);
			SetResult(uint8_t(m_nExecC));
			SetResult(uint8_t(m_nExecH));
			SetResult(uint8_t(m_nExecR));
			SetResult(uint8_t(m_nExecN));
			UpdateInterruptRequest(true);
			break;
		case COMMAND_SCAN_EQUAL:
		case COMMAND_SCAN_LOW_OR_EQUAL:
		case COMMAND_SCAN_HIGH_OR_EQUAL:
			if (!m_bExecSectorFirst) {
				m_nExecDataSize = 0;
				if (m_bExecCRCErrorData) {
					// nothing
				} else if (m_bScanMatched || m_bExecEndOfCylinder) {
					BuildST0(
						m_nExecDrive, false, ST0_INT_CODE_NT);
					BuildST1(
						false, false, false, false, false);
					BuildST2(
						false, false,
						!m_bScanMatched, m_bScanMatched,
						false, false, false);
				} else {
					m_bExecSectorFirst = true;
					BackupStatus();
					FindSector();
				}
				if (m_nExecDataSize <= 0) {
					SetPhase(PHASE_RESULT);
					SetResult(m_abtST[0]);
					SetResult(m_abtST[1]);
					SetResult(m_abtST[2]);
					SetResult(uint8_t(m_nExecC));
					SetResult(uint8_t(m_nExecH));
					SetResult(uint8_t(m_nExecR));
					SetResult(uint8_t(m_nExecN));
					UpdateInterruptRequest(true);
				}
			}
			break;
		}
	} else if (m_nPhase == PHASE_RESULT) {
		switch (GetCommand()) {
		case COMMAND_READ_DATA:
		case COMMAND_READ_DELETED_DATA:
		case COMMAND_WRITE_DATA:
		case COMMAND_WRITE_DELETED_DATA:
			if ((m_nResultPtr <= 0) && m_bExecSectorFirst) {
				if (m_bExecMultiTrackMoved) {
					DoSeek(m_nExecDrive, -1, 0);
				}
				RestoreStatus();
				SetPhase(PHASE_RESULT);
				SetResult(m_abtST[0]);
				SetResult(m_abtST[1]);
				SetResult(m_abtST[2]);
				SetResult(uint8_t(m_nExecC));
				SetResult(uint8_t(m_nExecH));
				SetResult(uint8_t(m_nExecR));
				SetResult(uint8_t(m_nExecN));
				UpdateInterruptRequest(true);
			}
			break;
		}
	}
}


void CPC88Fdc::SetLed(bool b)
{

	if ((m_nExecDrive + 1) & 1) {
		sys.pFddSystem->FDDActive(0);
	}
	else if ((m_nExecDrive + 1) & 2) {
		sys.pFddSystem->FDDActive(1);
	}
}