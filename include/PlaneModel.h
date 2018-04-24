// written by Frank Neuhaus, released unter MIT License
// https://github.com/cfneuhaus/snippets/blob/master/PlaneModel.h

#ifndef PLANEMODEL_H
#define PLANEMODEL_H

#include <Eigen/Core>
#include <Eigen/Dense>
#include <vector>
#include <array>

template <typename precision = double> struct PlaneModel
{
	static const int ModelSize=3;

	typedef Eigen::Matrix< precision, 3, 1 > _vector;
	typedef Eigen::Matrix< precision, 3, 3 > _matrix;

	_vector n;
	double d;

	void compute(const std::vector<_vector>& data, const std::array<size_t,3>& indices)
	{
		_vector a=data[indices[1]]-data[indices[0]];
		_vector b=data[indices[2]]-data[indices[0]];
		n=a.cross(b).normalized();
		d=n.dot(data[indices[0]]);
	}
	int computeInliers(const std::vector<_vector>& data, double threshold)
	{
		int inliers=0;
		for (size_t i=0;i<data.size();i++)
		{
			if (fabs(n.dot(data[i])-d)<threshold)
				inliers++;
		}
		return inliers;
	}
	void refine(const std::vector<_vector>& data, double threshold)
	{
		_vector Ex=_vector::Zero();
		_matrix Exsqr=_matrix::Zero();
		int inliers=0;
		for (size_t i=0;i<data.size();i++)
		{
			if (fabs(n.dot(data[i])-d)<threshold)
			{
				inliers++;
				Ex+=data[i];
				Exsqr+=data[i]*data[i].transpose();
			}
		}
		Ex/=inliers;
		Exsqr/=inliers;

		_matrix cov=Exsqr-Ex*Ex.transpose();

		Eigen::SelfAdjointEigenSolver<_matrix> eig(cov);
		n=eig.eigenvectors().col(0);
		d=n.dot(Ex);
	}
};

#endif
