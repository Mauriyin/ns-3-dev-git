#pragma once
#include "ns3/ns3-ai-rl.h"


namespace ns3 {

struct mlInput
{
	double x;
	double y;
}Packed;

struct mlOutput
{
	double tttAdjustment;
}Packed;

class MROENV : public Ns3AIRL<mlInput, mlOutput>
{
	public:
		MROENV (void) = delete;
		MROENV (uint16_t id);
		double tableRead(double x, double y);
};
}// namespace ns3

