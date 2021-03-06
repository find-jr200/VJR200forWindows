// license:BSD-3-Clause
// copyright-holders:FIND
#ifndef __ANALYZEWAVE_H__
#define __ANALYZEWAVE_H__
#include <cstdint>
#include <vector>
#include <math.h>
using namespace std;

enum BaudRate { LOW, HIGH };

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// class CjrParameter
//
// 定数のみ
//
////////////////////////////////////////////////////////////////////////////////////////////////////
class CjrParameter
{
public:
    static const int lowFreqSpan = 2;
    static const int hiFreqSpan = 1;
    static const int maxSpan = 2;
    static const int minSpan = 1;
    static const int LOW_F = 1200;
    static const int HIGH_F = 2400;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// class BaudParameter
//
// baudrateによって必要なサイクルを設定
//
////////////////////////////////////////////////////////////////////////////////////////////////////
class BaudParameter
{
public:
    int hiCycle;
    int lowCycle;

    void SetBaudRate(BaudRate b)
    {
        if (b == LOW)
        {
            hiCycle = 8;
            lowCycle = 4;
        }
        else
        {
            hiCycle = 2;
            lowCycle = 1;
        }
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// class WaveState
//
// Wavを解析しデータに変換するための状態を保持
//
////////////////////////////////////////////////////////////////////////////////////////////////////
class WaveState
{
public:
    bool PlusClear;
    bool MinusClear;
    bool TurnOver = false;

    void SetState(int data)
    {
        if (data > max) max = data;
        if (data < min) min = data;
        if (data >= 0) sign = 1; else if (data < 0) sign = -1; // データがゼロの場合は正にしておく
        if (data > threshold) PlusClear = true;
        if (data < threshold * -1) MinusClear = true;
        if (sign * lastSign < 0) TurnOver = true;
        else TurnOver = false;

        lastData = data;
        lastSign = sign;
    }

protected:
    int max = 0;
    int min = 0;
    int sign = 0;
    int lastSign = 0;
    int lastData = 0;
    int threshold = 64;
};



class AnalyzeWave
{
public:
    AnalyzeWave(vector<uint8_t>*);
    ~AnalyzeWave();
    bool AnalyzeWavData();
    bool WriteCjrFile();


protected:
    vector<uint8_t>* saveData; // JRが書き出したデータ
    vector<int8_t> convertedList; // saveDataを扱いやすく変換
    vector<uint8_t> cjrList; // 結果のCJR
    int bitData[8];
    WaveState wState;
    int readBytes = 0;
    long checkSum = 0;
    int blockCount = 0;
    int lastData = 0;
    bool bWaveOn = false;
    uint8_t blockHead[3]; // ブロック先頭の 02, 2a, xx の部分
    int dataCount;
    BaudParameter bparam;
    CjrParameter param;
    unsigned int readCount = 0;

    int GetSample();
    int GetHalfSpan();
    int BitToByte(int* BitData);
    void SetConvertedList();

};

#endif