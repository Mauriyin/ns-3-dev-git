#include "MRO-env.h"

namespace ns3 {

MROENV::MROENV (uint16_t id) : Ns3AIRL<mlInput, mlOutput> (id)
{
  SetCond (2, 0);
}

double MROENV::tableRead(double x, double y)
{
    auto mlInput = EnvSetterCond();
    mlInput->x = x;
    mlInput->y = y;
    SetCompleted();
    
    auto mlOutput = ActionGetterCond();
    double ret = mlOutput->tttAdjustment;
    GetCompleted();
    return ret;
}

void MROENV::loadIds(double time, int imsi, int cellId, double rsrp)
{
	auto mlInput = EnvSetterCond();
	mlInput->time = time;
	mlInput->imsi = imsi;
  mlInput->cellId = cellId;
  mlInput->rsrp = rsrp;
	SetCompleted();
}


void MROENV::passPacketReception(double time, double packetSize, int packetReceiverId)
{
  auto mlInput = EnvSetterCond();
  mlInput->time = time;
  mlInput->packetSize = packetSize;
  mlInput->packetReceiverId = packetReceiverId;
  mlInput->packetRxFlag = 1;
  SetCompleted();
}





}// namespace ns3



















