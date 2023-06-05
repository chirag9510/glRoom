#include "StateManager.h"
#include <wtypes.h>


#ifdef __cplusplus
extern "C" {
#endif

	__declspec(dllexport) DWORD NvOptimusEnablement = 1;
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;

#ifdef __cplusplus
}
#endif


int main(int argc, char** argv)
{	
	std::unique_ptr<StateManager> sm = std::make_unique<StateManager>();
	sm->run();
	return 0;
}