///definitions for random fucntions///
#include <Helpers.h>
#include <random>
#include <ctime>
#include <assert.h>

namespace Helpers
{
	namespace Random
	{
		std::default_random_engine Engine(static_cast<unsigned long>(std::time(0)));
	};
};

const float Helpers::Random::Float(float begin, float end)
{
	assert(begin < end);
	std::uniform_real_distribution<float> dist(begin, end);
	return dist(Engine);
}

const int Helpers::Random::Int(int begin, int end)
{
	assert(begin < end);
	std::uniform_int_distribution<int> dist(begin, end);
	return dist(Engine);
}