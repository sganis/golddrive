extern "C" {
#include "../sanfs/sanssh.h"
}
#include "CppUnitTest.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace unittest
{		
	TEST_CLASS(UnitTest1)
	{
	public:
		
		TEST_METHOD(TestMethod1)
		{
			Assert::AreEqual(1,1);
		}

	};
}