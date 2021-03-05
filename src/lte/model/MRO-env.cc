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
    mlInput->x = x;
    SetCompleted();
    
    auto mlOutput = ActionGetterCond();
    int ret = mlOutput->tttAdjustment;
    GetCompleted();
    return ret;
}
}// namespace ns3



















