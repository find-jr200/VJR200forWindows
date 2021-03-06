// license:BSD-3-Clause
// copyright-holders:FIND
////////////////////////////////////////////////////////////////////////////////////////////////////
//
// class CjrFormat
// CJRファイルフォーマットの操作全般
//
////////////////////////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include <vector>
#include <sys/stat.h>
#include "CjrFormat.h"
#include "Address.h"
#include "JRSystem.h"

using namespace std;

extern JRSystem sys;

CEREAL_CLASS_VERSION(CjrFormat, CEREAL_VER);


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// 以下は波形データ作成のためのサポートクラス
//
////////////////////////////////////////////////////////////////////////////////////////////////////
class ByteGetter
{
public:
	ByteGetter(vector<uint8_t>* vu)
	{
		b = vu;
		it = b->begin();
	}

	uint8_t Get()
	{
		return *(it++);
	}

protected:
	vector<uint8_t>* b;
	vector<uint8_t>::iterator it;
};

class BitGetterL // リーダー部分用
{
public:
	BitGetterL(uint8_t bp)
	{
		b = bp;
		p = 0;
	}
	int Get()
	{
		return ((b & (1 << p++)) ? 1 : 0);
	}
protected:
	uint8_t b;
	int p;
};

class BitGetterD // データ部分用
{
public:
	BitGetterD(uint8_t bp)
	{
		b = bp;
		p = 0;

		array[0] = 0;
		array[9] = 1;
		array[10] = 1;
		array[11] = 1;

		for (int i = 1; i < 9; ++i) {
			array[i] = (b & (1 << (i - 1)) ? 1 : 0);
		}
	}

	int Get()
	{
		return array[p++];
	}
protected:
	uint8_t b;
	int p;
	int array[12];
};


class WaveGetter
{
public:
	WaveGetter()
	{
	}

	WaveGetter(int ip)
	{
		i = ip;
	}

	void Set(int ip)
	{
		i = ip;
		p = 0;
	}

	int Get(int baud /* 0 なら2400、それ以外なら600*/, int& sign)
	{
		if (baud != 0) {
			if (i == 0) // 600 baud
			{
				if (sign == 1)
					return zero_p[p++];
				else
					return zero_n[p++];
			}
			else {
				if (sign == 1)
					return one_p[p++];
				else
					return one_n[p++];
			}
		}
		else { // 2400baud
			if (i == 0) {
				if (sign == 1)
					return zero_f_p[p++];
				else
					return zero_f_n[p++];
			}
			else {
				int ret = 0;
				if (sign == 1)
					ret = one_f_p[p];
				else
					ret = one_f_n[p];

				if (++p == 2) {
					sign *= -1;
				};
				return ret;
			}
		}
	}

protected:
	int i;
	int p = 0;
	// 600baud
	const int zero_p[8] = { 0, 1, 0, 1, 0, 1, 0, 1 };
	const int zero_n[8] = { 1, 0, 1, 0, 1, 0, 1, 0 };
	const int  one_p[8] = { 0, 0, 1, 1, 0, 0, 1, 1 };
	const int  one_n[8] = { 1, 1, 0, 0, 1, 1, 0, 0 };
	// 2400baud
	const int zero_f_p[2] = { 0, 1 };
	const int zero_f_n[2] = { 1, 0 };
	const int  one_f_p[2] = { 0, 0 };
	const int  one_f_n[2] = { 1, 1 };
};




////////////////////////////////////////////////////////////////////////////////////////////////////
//
// CJR FORMAT 実装
//
////////////////////////////////////////////////////////////////////////////////////////////////////

CjrFormat::CjrFormat()
{
	for (int i = 0; i < 136; ++i) // 136
	{
		leaderData.push_back(0xff);
	}

	for (int i = 0; i < 12; ++i) // 12
	{
		intermData.push_back(0xff);
	}

	pLoadData = new uint8_t[ALLOC_SIZE]; // なんとなく5MB確保
}


CjrFormat::~CjrFormat()
{
	for (unsigned int i = 0; i < loadDataBlock.size(); ++i) {
		if (loadDataBlock[i] != nullptr) delete loadDataBlock[i];
	}

	delete [] pLoadData;

	if (fp != nullptr)
		fclose(fp);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// CJRFormatクラスにCJRデータをセット
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void CjrFormat::SetCjrData(uint8_t* d, int size)
{
	cjrData.assign(d, d + size);
	startAddress = cjrData[37] << 8;
	startAddress += cjrData[38];
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// ボーレートを設定しチェックサムも変更
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void CjrFormat::AdjustBaudRate(int d)
{
	cjrData[23] = d;
	int sum = 0;

	for (int i = 0; i < 32; ++i) sum += cjrData[i];
	cjrData[32] = static_cast<uint8_t>(sum);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// CJRデータからBINを抜き出して返す
//
////////////////////////////////////////////////////////////////////////////////////////////////////
uint8_t* CjrFormat::GetBinDataArray()
{
	int blockSize = 0;
	int currentPoint = 0;

	if (cjrData.size() == 0) return nullptr;

	// ヘッダ読み飛ばし
	currentPoint = 6 + cjrData[3] + 1;
	// データブロック・フッタブロック読み出し
	while (cjrData[currentPoint + 2] != static_cast<uint8_t>(0xff))
	{
		blockSize = cjrData[currentPoint + 3] == 0 ? 256 : cjrData[currentPoint + 3];
		currentPoint += 6;
		for (int i = 0; i < blockSize; ++i)
		{
			binData.push_back(static_cast<uint8_t>(cjrData[currentPoint++]));
		}
		++currentPoint;
	}

	return binData.data();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// BINデータのサイズを返す
//
////////////////////////////////////////////////////////////////////////////////////////////////////
int CjrFormat::GetBinSize()
{
	return (int)binData.size();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// CJRデータのサイズを返す
//
////////////////////////////////////////////////////////////////////////////////////////////////////
int CjrFormat::GetCjrSize()
{
	return (int)cjrData.size();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// CJRデータがBASICならtrue、マシン語ならfalseを返す
//
////////////////////////////////////////////////////////////////////////////////////////////////////
bool CjrFormat::isBasic()
{
	if (cjrData.size() != 0) {
		if (cjrData[22] == 0)
			return true;
		else
			return false;
	}
	else {
		assert(false);
		return 0;
	}

}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// CJRのロードスタートアドレスを返す
//
////////////////////////////////////////////////////////////////////////////////////////////////////
int CjrFormat::GetStartAddress()
{
	return startAddress;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// BINデータからCJRに変換して返す
//
// 引数:jrFileName JR用のファイル名　　startAddress ロードスタートアドレス　　basic BASICならtrue、マシン語ならfalse
//
////////////////////////////////////////////////////////////////////////////////////////////////////
uint8_t* CjrFormat::GetCjrDataArray(char* jrFileName, int startAddress, bool basic)
{
	int blockCount = 0, checkSum = 0, pointer = 0, blockHead = 0;

	if (binData.size() == 0) return nullptr;

	if (basic) startAddress = 0x801;

	// ヘッダブロック
	cjrData.push_back(2);
	cjrData.push_back(0x2a);
	cjrData.push_back(static_cast<uint8_t>(blockCount));
	cjrData.push_back(0x1a);
	cjrData.push_back(0xff);
	cjrData.push_back(0xff);
	char n[16] = {0};
	strncpy(n, jrFileName, 16);
	for (int i = (int)strlen(jrFileName); i < 16; ++i)
		n[i] = 0;
	cjrData.insert(cjrData.end(), n, n + 16); // ファイル名
	cjrData.push_back(static_cast<uint8_t>(basic ? 0 : 1)); // マシン語 or BASIC
	cjrData.push_back(0); // ボーレート

	cjrData.push_back(0xff);
	cjrData.push_back(0xff);
	cjrData.push_back(0xff);
	cjrData.push_back(0xff);
	cjrData.push_back(0xff);
	cjrData.push_back(0xff);
	cjrData.push_back(0xff);
	cjrData.push_back(0xff);

	for (uint16_t i = 0; i < cjrData.size(); ++i)
		checkSum += (int)cjrData[i];
	cjrData.push_back(static_cast<uint8_t>(checkSum));

	++blockCount;

	// データブロック
	while (true)
	{
		blockHead = (int)cjrData.size();
		cjrData.push_back(2);
		cjrData.push_back(0x2a);
		cjrData.push_back(static_cast<uint8_t>(blockCount));
		cjrData.push_back(0); // ダミー（ブロックサイズ）
		int sizePoint = (int)cjrData.size() - 1;

		cjrData.push_back(static_cast<uint8_t>(startAddress >> 8));
		cjrData.push_back(static_cast<uint8_t>(startAddress & 0xff));

		int j;
		for (j = 0; j < 256; ++j)
		{
			cjrData.push_back(binData[pointer]);
			++pointer;
			if (pointer == binData.size()) break;
		}

		cjrData[sizePoint] = static_cast<uint8_t>((j == 256) ? 0 : j + 1);

		// チェックサム計算
		checkSum = 0;
		for (unsigned int i = blockHead; i < cjrData.size(); ++i)
			checkSum += (int)cjrData[i];
		cjrData.push_back(static_cast<uint8_t>(checkSum));

		++blockCount;
		startAddress += j;

		if (pointer == binData.size()) break;
	}

	// フッタブロック
	++startAddress;
	cjrData.push_back(2);
	cjrData.push_back(0x2a);
	cjrData.push_back(0xff);
	cjrData.push_back(0xff);
	cjrData.push_back(static_cast<uint8_t>(startAddress >> 8));
	cjrData.push_back(static_cast<uint8_t>(startAddress & 0xff));

	return cjrData.data();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// CJRデータを返す
//
////////////////////////////////////////////////////////////////////////////////////////////////////
uint8_t * CjrFormat::GetCjrDataArray()
{
	return cjrData.data();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// CJRデータをロードするためにブロックごと分割してにloadDataBlockに追加
//
////////////////////////////////////////////////////////////////////////////////////////////////////
bool CjrFormat::SetLoadDataBlock(int fileSize)
{
	if (cjrData.size() == 0) return false;

	AdjustBaudRate(0); //////////////////// 2400baudに設定

	int c = 0, p = 0, blockNo, dataSize;
	loadDataBlock.push_back(new vector<uint8_t>());
	if (cjrData[2] == 0) {
		loadDataBlock[0]->insert(loadDataBlock[0]->end(), cjrData.begin(), cjrData.begin() + 33);
		p = 33;
		++c;
	}
	while ((blockNo = cjrData[p + 2]) < 0xff && p < fileSize) {
		dataSize = cjrData[p + 3];
		if (dataSize == 0) dataSize = 256;
		dataSize += 7;
		loadDataBlock.push_back(new vector<uint8_t>());
		loadDataBlock[c]->insert(loadDataBlock[c]->end(), &cjrData[p], &cjrData[p + dataSize]);
		p += dataSize;
		++c;
	}
	if (p >= fileSize) return false;

	loadDataBlock.push_back(new vector<uint8_t>());
	loadDataBlock[c]->insert(loadDataBlock[c]->end(), cjrData.end() - 6, cjrData.end());

	itBlock = loadDataBlock.begin();
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// 引数のloadDataにロード用の波形データを格納
//
////////////////////////////////////////////////////////////////////////////////////////////////////
int CjrFormat::GetLoadData(uint8_t* loadData)
{
	bool isLeader = true;
	int p = 0;
	int count = 0;
	int sign = 1;

	while (itBlock != loadDataBlock.end())
	{
		if (isLeader) {
			// Leader, intermission
			if (count == 0) { // Leader
				for (unsigned int i = 0; i < leaderData.size(); ++i) {
					ByteGetter by(&leaderData);
					BitGetterL bi(by.Get());
					for (int k = 0; k < 8; ++k) { // 1バイトのループ
						WaveGetter wg(bi.Get());
						for (int l = 0; l < 8; ++l) { // 1ビットのループ
							int x = wg.Get(1, sign);
							*(loadData + p) = x;
							++p;
						}
					}
				}
			}
			else { // intermission
				for (unsigned int i = 0; i < intermData.size(); ++i) {
					ByteGetter by(&intermData);
					BitGetterL bi(by.Get());
					for (int k = 0; k < 8; ++k) { // 1バイトのループ
						WaveGetter wg(bi.Get());
						for (int l = 0; l < 8; ++l) { // 1ビットのループ
							int x = wg.Get(1, sign);
							*(loadData + p) = x;
							++p;
						}
					}
				}
			}
			isLeader = false;
		} else { // data部
			ByteGetter byteG(*itBlock);
			for (unsigned int i = 0; i < (*itBlock)->size(); ++i) { // 1データブロックのループ
				uint8_t bytedata = byteG.Get();
				BitGetterD bitG(bytedata);
				for (int k = 0; k < 12; ++k) { // 1バイトのループ
					WaveGetter wg;
					wg.Set(bitG.Get());

					if (count == 0) { // ヘッダブロックのみ600baud固定
						for (int l = 0; l < 8; ++l) { // 1ビットのループ
							*(loadData + p) = wg.Get(1, sign);
							++p;
						}
					}
					else {
						for (int l = 0; l < 2; ++l) { // 1ビットのループ
							*(loadData + p) = wg.Get(0, sign);
							++p;
						}
					}
				}
			}
			isLeader = true;
			++itBlock;
			++count;
		}

		for (unsigned int i = 0; i < intermData.size(); ++i) {
			ByteGetter by(&intermData);
			BitGetterL bi(by.Get());
			for (int k = 0; k < 8; ++k) { // 1バイトのループ
				WaveGetter wg(bi.Get());
				for (int l = 0; l < 8; ++l) { // 1ビットのループ
					int x = wg.Get(1, sign);
					*(loadData + p) = x;
					++p;
				}
			}
		}
	}

	return p;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// メモリ空間からBASICリストをCJR化して返す
//
////////////////////////////////////////////////////////////////////////////////////////////////////
uint8_t* CjrFormat::MemToCjrBasic(char* jrFileName)
{
	uint16_t d;
	int nextAddress = 0, current = 0x801;
	Address* a = sys.pAddress;

	d = a->ReadByte(current++);
	while (d != 0)
	{
		binData.push_back((uint8_t)d);
		nextAddress = d << 8;
		d = a->ReadByte(current++);
		nextAddress += d;
		binData.push_back((uint8_t)d);

		for (int i = current; i < nextAddress; ++i) {
			binData.push_back(a->ReadByte(current++));
		}

		d = a->ReadByte(current++);
	}

	binData.push_back((uint8_t)d);
	binData.push_back(a->ReadByte(current++));
	binData.push_back(a->ReadByte(current++));

	return GetCjrDataArray(jrFileName, 0x801, true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// メモリ空間から引数のstartからendまでをマシン語CJRにして返す
//
////////////////////////////////////////////////////////////////////////////////////////////////////
uint8_t* CjrFormat::MemToCjrBin(char* jrFileName, int start, int end)
{
	Address* a = sys.pAddress;

	for (int i = start; i <= end; ++i) {
		binData.push_back(sys.pAddress->ReadByte(i));
	}
	return GetCjrDataArray(jrFileName, start, false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// 初期化
//
////////////////////////////////////////////////////////////////////////////////////////////////////

bool CjrFormat::Init(const TCHAR * fn)
{
	_tcsncpy(fileName, fn, MAX_PATH);

	struct _stat buf;
	int st = _tstat(fileName, &buf);
	if (st != 0)
		return false;
	fileSize = buf.st_size;

	fp = _tfopen(fileName, _T("rb"));
	if (fp == nullptr)
		return false;

	uint8_t* binData = new uint8_t[65536];
	if (!binData) return false;
	fread(binData, sizeof(uint8_t), fileSize, fp);
	fclose(fp);
	fp = nullptr;

	SetCjrData(binData, (int)fileSize);
	if (!SetLoadDataBlock(fileSize)) {
		delete[] binData;
		return false;
	}
	loadDataSize = GetLoadData(pLoadData);
	pointer = 0;
	delete[] binData;

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// タイプを文字列で返す
//
////////////////////////////////////////////////////////////////////////////////////////////////////
const TCHAR* CjrFormat::GetType()
{
	return _T("CJR");
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// ファイル名を返す
//
////////////////////////////////////////////////////////////////////////////////////////////////////
const TCHAR* CjrFormat::GetFileName()
{
	return fileName;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// 1バイト書き込み
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void CjrFormat::WriteByte(uint8_t b)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// 1ビット分のロードデータを返す
//
////////////////////////////////////////////////////////////////////////////////////////////////////
uint8_t CjrFormat::GetLoadData()
{
	bCountStart = true;
	return *(pLoadData + pointer);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// CPUの消費クロック分カウンタを進める
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void CjrFormat::TickCounter(int c)
{
	if (!bCountStart) return;

	counter += c;
	if (counter >= DATA_CYCLE) {
		counter = 0;
		if (pointer <loadDataSize)
			++pointer;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// ファイル位置をバイト数で返す
//
////////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t CjrFormat::GetPoiner()
{
	return pointer;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// ファイル位置を秒数で返す
//
////////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t CjrFormat::GetTimeCounter()
{
	return pointer * 209 / 1000000;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// リモート端子が録音・再生になった時の処理
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void CjrFormat::remoteOn()
{
	counter = 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// リモート端子が停止になった時の処理
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void CjrFormat::remoteOff()
{
	bCountStart = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// ファイル位置を先頭へ
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void CjrFormat::Top()
{
	pointer = 0;
}

void CjrFormat::Next()
{
}

void CjrFormat::Prev()
{
}

void CjrFormat::FF()
{
}

void CjrFormat::Rew()
{
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Read only 設定
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void CjrFormat::SetReadOnly(bool b)
{
	bReadOnly = b;
}