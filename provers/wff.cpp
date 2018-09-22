
#include "wff.h"
#include "mm/ptengine.h"

//#define LOG_WFF

template class TWff<PropTag>;
template class TWff<PredTag>;
template class TVar<PropTag>;
template class TVar<PredTag>;
template class TTrue<PropTag>;
template class TTrue<PredTag>;
template class TFalse<PropTag>;
template class TFalse<PredTag>;
template class TNot<PropTag>;
template class TNot<PredTag>;
template class TImp<PropTag>;
template class TImp<PredTag>;
template class TBiimp<PropTag>;
template class TBiimp<PredTag>;
template class TAnd<PropTag>;
template class TAnd<PredTag>;
template class TOr<PropTag>;
template class TOr<PredTag>;
template class TNand<PropTag>;
template class TNand<PredTag>;
template class TXor<PropTag>;
template class TXor<PredTag>;
template class TAnd3<PropTag>;
template class TAnd3<PredTag>;
template class TOr3<PropTag>;
template class TOr3<PredTag>;
