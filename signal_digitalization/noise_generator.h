#pragma once
#include <stdlib.h>
#include <random>

class NoiseGenerator {
public:
	NoiseGenerator() {

	}

	static float GenerateGussianNoise()
	{
		// Define random generator with Gaussian distribution
		const float mean = 0.0f;
		const float stddev = 0.1f;
		std::default_random_engine generator;
		std::normal_distribution<float> dist(mean, stddev);
		return dist(generator);
	}

	void tick(float time) {
		//change dirrection of the noise
		if (std::abs(target - temp) < 0.1f || time - last_change > 2.f)
		{
			target = GenerateGussianNoise() * 5.f;
			auto rand_ = rand() % 3;
			if (rand_ == 0)
				target *= -1.f;
			else if (rand_ == 1)
				target = 0.f;
	
			last_change = time;
		}

		//change per tick
		bool going_up = temp < target;
		auto change = going_up ? GenerateGussianNoise() : GenerateGussianNoise() * -1.f;

		//add chance to not change value
		if ((rand() % 3) != 0)
			change = 0.f;

		//apply changes
		temp += change * 0.05f;
	}

	float GetNoiseData() {
		return temp;
	}

private:
	float temp = 0;
	bool going_up = false;
	float last_change = 0.f;
	
	float target = 0.f;
};