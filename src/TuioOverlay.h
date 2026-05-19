#ifndef _TUIOOVERLAY_H_
#define _TUIOOVERLAY_H_

#include <TUIO/TuioClient.h>
#include <opencv2/core/core.hpp>
#
class TuioOverlay {

	public:

		TuioOverlay(int _tw, int _th);

		void process(cv::Mat* input);

	protected:

		int tw,th;

		TUIO::TuioClient* client;
		std::map<int,cv::Mat> pix;
};

#endif // _TUIOOVERLAY_H_
