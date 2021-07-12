#include "Assistance.h"

int main(int argc, char** argv)
{
	using namespace asst;

	Assistance asst;
	if (!asst.setSimulatorType(SimulatorType::BlueStacks)) {
		DebugTraceError("Find Simulator Error");
		getchar();
		return -1;
	}

	DebugTraceInfo("Start");
	asst.start();

	getchar();

	DebugTraceInfo("Stop");
	asst.stop();

	return 0;
}