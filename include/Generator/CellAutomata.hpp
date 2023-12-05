#ifndef IE_CELLAUTOMATA_HPP
#define IE_CELLAUTOMATA_HPP

#include <random>

class CellAutomata
{
public:

	void generate()
	{
		std::random_device dev;
		std::mt19937 rng(dev());
		std::uniform_real_distribution<double> dist(0.f, 100.f);

		m_occupancy.resize(m_rows * m_cols);

		// initialize, 45% chance to be walls.
		for (uint32_t i = 0; i < m_cols * m_rows; ++i)
		{
			if (dist(rng) < 45.f) { m_occupancy[i] = true; }
			else { m_occupancy[i] = false; }
		}
	}

private:
	int m_cols = 50, m_rows = 50;
	std::vector<bool> m_occupancy;
};

#endif // IE_CELLAUTOMATA_HPP
