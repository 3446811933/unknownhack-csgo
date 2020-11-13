#include "VMProtectSDK.h"

#define VMPBegin(m) VMProtectBegin(m)
#define VMPVirt(m) VMProtectBeginVirtualization(m)
#define VMPMut(m) VMProtectBeginMutation(m)
#define VMPUltra(m) VMProtectBeginUltra(m)
#define VMPEnd() VMProtectEnd()

#define VMPDetectDebuggers() VMProtectIsDebuggerPresent(true)
#define VMPDetectVM() VMProtectIsVirtualMachinePresent()
#define VMPValidateCRC() VMProtectIsValidImageCRC()

#define VMPGetHWID(h, s) VMProtectGetCurrentHWID(h, s)

#define XOR(s) VMProtectDecryptStringA(s)