// written by Frank Neuhaus, released unter MIT License
// https://github.com/cfneuhaus/snippets/blob/master/SimpleRansac.h

#ifndef SIMPLERANSAC_H
#define SIMPLERANSAC_H

#include <vector>
#include <array>
#include <random>
#include <set>

template<typename Model_, typename DataType>
Model_ ransac(const std::vector<DataType>& data, double threshold, int numIterations, unsigned int randomSeed=std::random_device{}())
{
	/*if (data.size()<Model_::ModelSize)
		throw std::runtime_error("Not enough data");*/

	std::mt19937 engine{randomSeed};
	std::uniform_int_distribution<size_t> dis{0,data.size()-1};

	int bestInliers=-1;
	Model_ bestModel;

	for (int it=0;it<numIterations;it++)
	{
		// select points
		int found=0;
		std::array<size_t,Model_::ModelSize> indices;
		std::set<size_t> usedIndices;
		while (found < Model_::ModelSize)
		{
			size_t sample=dis(engine);
			if (usedIndices.find(sample)!=usedIndices.end())
				continue;
			usedIndices.insert(sample);
			indices[found++]=sample;
		}

		// compute model
		Model_ m;
		m.compute(data,indices);

		int inliers=m.computeInliers(data,threshold);
		if (inliers>bestInliers)
		{
			bestModel=m;
			bestInliers=inliers;
		}
	}
	bestModel.refine(data,threshold);
	return bestModel;
}

#endif

