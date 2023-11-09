#ifdef RENET_EXPORTS
	#define RENET_API __declspec(dllexport)
#else
	#define RENET_API __declspec(dllimport)
#endif

//#include "vld.h"

namespace RENET	{}
using namespace RENET;

#define START_NAMESPACE namespace RENET {
#define END_NAMESPACE }

