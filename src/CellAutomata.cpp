#include "CellAutomata.hpp"
#include "candybox/Memory.hpp"

void CellAutomata::generate()
{
	std::random_device dev;
	std::mt19937 rng(dev());
	std::uniform_real_distribution<double> dist(0.f, 100.f);

	m_occupancy.resize(m_rows * m_cols);
	m_occupancyRev.resize(m_rows * m_cols);

	// initialize, 45% chance to be walls
	for (int i = 0; i < m_cols * m_rows; ++i)
	{
		if (dist(rng) < 45.f) { m_occupancy[i] = true; }
		else { m_occupancy[i] = false; }
	}

	// generate connected walls and floors.
	for (int i = 0; i < 4; ++i)
	{
		m_occupancyRev = m_occupancy;
		for (int x = 0; x < m_cols; ++x)
		{
			for (int y = 0; y < m_rows; ++y)
			{
				int r1 = countWalls(x, y, 1);
				int r2 = countWalls(x, y, 2) - r1;

				if (x <= 1 || y <= 1 || x >= m_cols - 2 || y >= m_rows - 2)
				{
					r2 += 5;
					r1 += 4;
				}

				bool cell = m_occupancyRev[y * m_cols + x];
				if (!cell)
				{
					if (r1 >= 5) m_occupancyRev[y * m_cols + x] = true;
					if (r2 >= 8) m_occupancyRev[y * m_cols + x] = true;
				}
				else
				{
					if (r1 < 4) m_occupancyRev[y * m_cols + x] = false;
					if (r2 <= 7) m_occupancyRev[y * m_cols + x] = false;
				}
			}
		}
		m_occupancy = m_occupancyRev;
	}

	// remove isolated walls and floors
	for (int i = 0; i < 6; ++i)
	{
		m_occupancyRev = m_occupancy;
		for (int x = 0; x < m_cols; ++x)
		{
			for (int y = 0; y < m_rows; ++y)
			{
				int r1 = countWalls(x, y, 1);

				bool cell = m_occupancyRev[y * m_cols + x];
				if (x == 0 || y == 0 || x + 1 == m_cols || y + 1 == m_rows)
				{
					m_occupancyRev[y * m_cols + x] = true;
					continue;
				}
				if (!cell)
				{
					if (r1 >= 5) { m_occupancyRev[y * m_cols + x] = true; }
				}
				else
				{
					if (r1 <= 3) m_occupancyRev[y * m_cols + x] = false;
				}
			}
		}
		m_occupancy = m_occupancyRev;
	}

	// enclose walls
	for (int x = 0; x < m_cols; ++x)
	{
		for (int y = 0; y < m_rows; ++y)
		{
			int r1 = countWalls(x, y, 1);

			bool cell = m_occupancy[y * m_cols + x];
			if (x == 0 || y == 0 || x + 1 == m_cols || y + 1 == m_rows)
			{
				m_occupancy[y * m_cols + x] = true;
				continue;
			}
			if (!cell)
			{
				if (r1 >= 5) { m_occupancy[y * m_cols + x] = true; }
			}
			else
			{
				if (r1 <= 3) m_occupancy[y * m_cols + x] = false;
			}
		}
	}

	// find borders
	m_borders.clear();
	std::vector<bool> visited;
	visited.resize(m_cols * m_rows); // should be all 'false'
	for (;;)
	{
		// find first un-visited floor
		int x, y;
		bool has = false;
		for (y = 0; y < m_rows; y++)
		{
			if (has) break;
			for (x = 0; x < m_cols; x++)
			{
				const int index = y * m_cols + x;
				if (!m_occupancy[index] && !visited[index])
				{
					has = true;
					break;
				}
			}
		}

		if (!has) break; // we have iterated all the floors

		// add new border
		m_borders.emplace_back();
		std::vector<Coord> &border = m_borders[m_borders.size() - 1];

		struct Record
		{
			int x = -1, y = -1;
			int count = 0;
			Record(int x0, int y0, int cnt) : x(x0), y(y0), count(cnt) { }
		};
		std::queue<Record> q;
		q.emplace(x, y, 0);
		while (!q.empty())
		{
			// iterate over neighbors
			Record cur = q.front();
			q.pop();
			if (countWalls(cur.x, cur.y, 1) > 0) { border.emplace_back(cur.x, cur.y); }
			int offsets[4][2] = {{0, -1}, {1, 0}, {0, 1}, {-1, 0}};
			for (const auto &offset : offsets)
			{
				if (!inBound(cur.x, cur.y, offset[0], offset[1])) continue;
				const int index = (cur.y + offset[1]) * m_cols + cur.x + offset[0];
				if (visited[index]) continue;
				visited[index] = true;
				if (m_occupancy[index]) continue;

				q.emplace(cur.x + offset[0], cur.y + offset[1], 0);
			}
		}
	}

	print();
	auto map = static_cast<char *>(candybox::_malloc(sizeof(char) * m_rows * m_cols));
	for (int y = 0; y < m_rows; y++)
		for (int x = 0; x < m_cols; x++)
		{
			map[y * m_cols + x] = m_occupancy[y * m_cols + x] ? '.' : ' ';
		}
	printf("total borders: %zu\n", m_borders.size());
	for (const auto &border : m_borders)
		for (const auto &c : border) map[c.y * m_cols + c.x] = '#';
	for (int y = 0; y < m_rows; ++y)
	{
		for (int x = 0; x < m_cols; ++x) printf("%c", map[y * m_cols + x]);
		printf("\n");
	}
	printf("\n");
	candybox::_free(map);
}

void CellAutomata::print() const
{
	for (int y = 0; y < m_rows; ++y)
	{
		for (int x = 0; x < m_cols; ++x)
			printf("%c", m_occupancy[y * m_cols + x] ? '.' : ' ');
		printf("\n");
	}
	printf("\n");
}

bool CellAutomata::inBound(int x, int y, int offx, int offy) const
{
	if (offx < 0)
	{
		if (x < -offx) return false;
	}
	else
	{
		if (x + offx >= m_cols) { return false; }
	}
	if (offy < 0)
	{
		if (y < -offy) return false;
	}
	else
	{
		if (y + offy >= m_rows) { return false; }
	}
	return true;
}

int CellAutomata::countWalls(int x, int y, int diff) const
{
	int c = 0;
	diff = std::abs(diff);
	for (int offx = -diff; offx <= diff; offx++)
	{
		for (int offy = -diff; offy <= diff; offy++)
		{
			if (offx == 0 && offy == 0) continue;
			if (!inBound(x, y, offx, offy)) continue;
			if (m_occupancy[(y + offy) * m_cols + x + offx]) c++;
		}
	}
	return c;
}
