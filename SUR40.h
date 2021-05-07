#ifndef _SUR40_H_
#define _SUR40_H_

#include "V4L2.h"

class SUR40: public V4L2 {

	public:

		SUR40(const char* pipe, int devnum);

		void remove_background();

};

#endif // _SUR40_H_
