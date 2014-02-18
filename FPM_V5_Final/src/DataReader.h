#ifndef _DATAREADER_H_
#define _DATAREADER_H_

#include <string>

namespace FPM {
  class FPMLayout;
	class DataReader {
	public:
		static bool ReadOASIS(std::string fileName, FPMLayout &layout);
	};

}

#endif //_DATAREADER_H_
