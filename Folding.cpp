#include <cstdlib>
#include <thimble/all.h>

#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <aff3ct.hpp>
using namespace aff3ct;

//---------------------------------folding----------------------------------------------------------
//
//
std::vector<std::vector<float>> folding(std::vector<float> v, int m)
{

	int sum = std::accumulate(v.begin(), v.end(), 0);

	auto fold = [m](std::string a, int b)
		{
			if ((size(a) + 1) % (m + 1) != 0)
			{
				return std::move(a) + std::to_string(b);
			}

			return std::move(a) + " " + std::to_string(b);
		};

	std::string s = std::accumulate(std::next(v.begin()), v.end(),
		std::to_string(v[0]), // start with first element
		fold);

	std::cout << "Sum: " << sum << '\n' << '\n'
		<< "String:    " << s << '\n' << '\n'
		<< "Vector:    ";


	std::vector<std::vector<float>> res(v.size() / m);

	for (int i = 0; i < v.size(); i++)
	{
		res[i / m].push_back(v[i]);
	}

	for (auto& w : res)
	{
		for (auto i : w) std::cout << i; std::cout << " ";
	}
	return res;
}




std::vector<int> unfolding(std::vector<std::vector<int>> v, int m)
{
	int size = 0;
	for (int i = 0; i < v.size(); i++)	size += v[i].size();
	std::vector<int>res(0);
	res.reserve(size);

	std::cout << '\n' << '\n' << "Unfolding: ";

	for (auto& w : v)
	{
		for (auto i : w) res.push_back(i);
	}
	for (auto& z : res) std::cout << z;
	std::cout << '\n';
	return res;
}




std::vector<int> foldingint(std::vector<int> v, int m)
{
	int sum = std::accumulate(v.begin(), v.end(), 0);

	auto fold = [m](std::string a, int b)
		{
			if ((size(a) + 1) % (m + 1) != 0)
			{
				return std::move(a) + std::to_string(b);
			}

			return std::move(a) + " " + std::to_string(b);
		};

	std::string s = std::accumulate(std::next(v.begin()), v.end(),
		std::to_string(v[0]), // start with first element
		fold);

	//std::cout << "String:    " << s << '\n' << '\n';

	std::istringstream is(s);
	std::vector<int> res((std::istream_iterator<int>(is)),
		(std::istream_iterator<int>()));

	return res;
}
//
//
//--------------------------------------folding-----------------------------------------------------
