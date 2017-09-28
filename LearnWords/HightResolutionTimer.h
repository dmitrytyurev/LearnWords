#pragma once

//-----------------------------------------------------------------------------
// ����� HightResolutionTimer ��������� ���������� �������.
// ���������� ������� �����. ������� ���������� ������� ������� � ������������.
//-----------------------------------------------------------------------------

class HightResolutionTimer
{
public:
	HightResolutionTimer();
	HightResolutionTimer(int ticsPerSecond);        // ������� ���������� ����� � �������
	void start(int ticsPerSecond);
	__int64 get() const;                            // �������� ������� �������� ������� � ������������� ����
	double  get_dbl() const;                        // �������� ������� �������� ������� � ���� double
	static __int64 get_cpu_clock() { __asm rdtsc }  // �������� ������������ ������� ������ 

private:
	__int64 _startValue;
	double  _coeff;
};
  