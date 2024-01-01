#ifndef CANDYBOX_CELLAUTOMATA_HPP__
#define CANDYBOX_CELLAUTOMATA_HPP__

#include <random>
#include <vector>
#include <queue>
#include <memory>

class CellAutomata
{
public:
	void generate();

	void print() const;

private:
	bool inBound(int x, int y, int offx = 0, int offy = 0) const;

	int countWalls(int x, int y, int diff) const;

private:
	int m_cols = 50, m_rows = 50;
	std::vector<bool> m_occupancy;
	std::vector<bool> m_occupancyRev;

	struct Coord
	{
		int x = -1, y = -1;
		Coord(int x0, int y0) : x(x0), y(y0) { }
	};
	std::vector<std::vector<Coord>> m_borders;
};

#endif // CANDYBOX_CELLAUTOMATA_HPP__
