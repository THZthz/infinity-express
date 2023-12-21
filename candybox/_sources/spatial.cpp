#include <cstring>
#include <cassert>
#include "candybox/Spatial.hpp"
#include "candybox/Linear.hpp"
#include "candybox/Memory.hpp"
#include "candybox/Hilbert.hpp"
#include "candybox/Heap.hpp"
using namespace ie;

Spatial::Spatial(uint32_t n)
{
	// calculate the total number of nodes in the R-tree to allocate space for
	// and the index of each tree level (used in search later).
	memset(m_levelBounds, 0, sizeof(m_levelBounds));
	uint32_t numItems = n;
	uint32_t numNodes = n;
	int i = 0;
	m_nodeSize = 16; // TODO: can be changed, between (2, 65535).
	m_levelBounds[i++] = numNodes * 4;
	do {
		n = (uint32_t)Ceil((float)n / (float)m_nodeSize);
		numNodes += n;
		m_levelBounds[i++] = numNodes * 4;
		assert(i <= (int)(sizeof(m_levelBounds) / sizeof(m_levelBounds[0]))); // TODO
	} while (n != 1);
	m_numBounds = i;
	m_numNodes = numNodes;

	m_numBoxes = 0;
	m_boxes = static_cast<float *>(_calloc(1, sizeof(float) * numNodes * 4));
	m_indices = static_cast<uint32_t *>(_calloc(1, sizeof(uint32_t) * numNodes));
	assert(m_boxes && m_indices);
	if (!m_boxes || !m_indices)
	{
		if (m_boxes) _free(m_boxes);
		if (m_indices) _free(m_indices);
		return;
	}
	m_pos = 0;
	m_numItems = numItems;
	m_minx = FLT_MAX;
	m_miny = FLT_MAX;
	m_maxx = -FLT_MAX;
	m_maxy = -FLT_MAX;
}

Spatial::~Spatial()
{
	if (m_boxes) _free(m_boxes);
	if (m_indices) _free(m_indices);
}

uint32_t
Spatial::add(float minx, float miny, float maxx, float maxy)
{
	uint32_t index = m_pos >> 2;
	m_indices[index] = index;
	m_boxes[m_pos++] = minx;
	m_boxes[m_pos++] = miny;
	m_boxes[m_pos++] = maxx;
	m_boxes[m_pos++] = maxy;
	m_numBoxes += 4;

	if (minx < m_minx) m_minx = minx;
	if (miny < m_miny) m_miny = miny;
	if (maxx > m_maxx) m_maxx = maxx;
	if (maxy > m_maxy) m_maxy = maxy;

	return index;
}

void
Spatial::addAll(float *data, uint32_t nFloat)
{
	uint32_t i;
	for (i = 0; i < nFloat; i += 4) { add(data[i], data[i + 1], data[i + 2], data[i + 3]); }
}

static void
swap(uint32_t *values, float *boxes, uint32_t *indices, uint32_t i, uint32_t j)
{
	uint32_t temp = values[i];
	values[i] = values[j];
	values[j] = temp;

	uint32_t k = 4 * i;
	uint32_t m = 4 * j;

	float a = boxes[k];
	float b = boxes[k + 1];
	float c = boxes[k + 2];
	float d = boxes[k + 3];
	boxes[k] = boxes[m];
	boxes[k + 1] = boxes[m + 1];
	boxes[k + 2] = boxes[m + 2];
	boxes[k + 3] = boxes[m + 3];
	boxes[m] = a;
	boxes[m + 1] = b;
	boxes[m + 2] = c;
	boxes[m + 3] = d;

	uint32_t e = indices[i];
	indices[i] = indices[j];
	indices[j] = e;
}

static void
sort(uint32_t *values,
     float *boxes,
     uint32_t *indices,
     uint32_t left,
     uint32_t right,
     uint32_t nodeSize)
{
	if (left / nodeSize >= right / nodeSize) return;

	uint32_t pivot = values[(left + right) >> 1];
	uint32_t i = left - 1;
	uint32_t j = right + 1;

	for (;;)
	{
		do i++;
		while (values[i] < pivot);
		do j--;
		while (values[j] > pivot);
		if (i >= j) break;
		swap(values, boxes, indices, i, j);
	}

	sort(values, boxes, indices, left, j, nodeSize);
	sort(values, boxes, indices, j + 1, right, nodeSize);
}

void
Spatial::finish()
{
	assert(m_pos >> 2 == m_numItems);

	if (m_numItems <= m_nodeSize)
	{
		// Only one node, skip sorting and just fill the root box.
		m_boxes[m_pos++] = m_minx;
		m_boxes[m_pos++] = m_miny;
		m_boxes[m_pos++] = m_maxx;
		m_boxes[m_pos++] = m_maxy;
		m_numBoxes += 4;
		return;
	}

	float width = m_maxx == -FLT_MAX ? 1 : m_maxx - m_minx;
	float height = m_maxy == -FLT_MAX ? 1 : m_maxy - m_miny;
	auto *hilbertValues = static_cast<uint32_t *>(_malloc(sizeof(uint32_t) * m_numItems));
	uint32_t hilbertMax = (1 << 16) - 1;

	// Map item centers into Hilbert coordinate space and calculate Hilbert values.
	for (uint32_t i = 0, pos = 0; i < m_numItems; i++)
	{
		float minX = m_boxes[pos++];
		float minY = m_boxes[pos++];
		float maxX = m_boxes[pos++];
		float maxY = m_boxes[pos++];

		// TODO
		uint32_t x, y;
		x = (uint32_t)floorf(
		    (float)hilbertMax * ((float)(minX + maxX) / 2 - (float)m_minx) / (float)width);
		y = (uint32_t)floorf(
		    (float)hilbertMax * ((float)(minY + maxY) / 2 - (float)m_miny) / (float)height);
		hilbertValues[i] = hilbert::HilbertXYToIndex(16, x, y);
	}

	// sort items by their Hilbert value (for packing later).
	sort(hilbertValues, m_boxes, m_indices, 0, m_numItems - 1, m_nodeSize);

	// generate nodes at each tree level, bottom-up.
	for (uint32_t i = 0, pos = 0; i < m_numBounds - 1; i++)
	{
		uint32_t end = m_levelBounds[i];

		// generate a parent node for each block of consecutive <nodeSize> nodes.
		while (pos < end)
		{
			uint32_t nodeIndex = pos;

			// calculate bbox for the new node.
			float nodeMinX = m_boxes[pos++];
			float nodeMinY = m_boxes[pos++];
			float nodeMaxX = m_boxes[pos++];
			float nodeMaxY = m_boxes[pos++];

			for (uint32_t j = 1; j < m_nodeSize && pos < end; j++)
			{
				nodeMinX = Min(nodeMinX, m_boxes[pos++]);
				nodeMinY = Min(nodeMinY, m_boxes[pos++]);
				nodeMaxX = Max(nodeMaxX, m_boxes[pos++]);
				nodeMaxY = Max(nodeMaxY, m_boxes[pos++]);
			}

			// add the new node to the tree data.
			m_indices[m_pos >> 2] = nodeIndex;
			m_boxes[m_pos++] = nodeMinX;
			m_boxes[m_pos++] = nodeMinY;
			m_boxes[m_pos++] = nodeMaxX;
			m_boxes[m_pos++] = nodeMaxY;
			m_numBoxes += 4;
		}
	}
}

uint32_t
upperBound(uint32_t value, uint32_t n, uint32_t *arr)
{
	uint32_t i = 0;
	uint32_t j = n - 1;
	while (i < j)
	{
		uint32_t m = (i + j) >> 1;
		if (arr[m] > value) { j = m; }
		else { i = m + 1; }
	}

	return arr[i];
}

void
Spatial::search(float minx, float miny, float maxx, float maxy, std::vector<uint32_t> &results)
{
	results.clear();
	if (m_numBoxes == 0) return;

	uint32_t nodeIndex = m_numBoxes - 4;
	uint32_t queue[512];
	uint32_t nQueue = 0;

	for (;;)
	{
		// Find the end index of the node.
		uint32_t bound = upperBound(nodeIndex, m_numBounds, m_levelBounds);
		uint32_t end = Min(nodeIndex + m_nodeSize * 4, bound);

		// Search through child nodes.
		for (uint32_t pos = nodeIndex; pos < end; pos += 4)
		{
			// Check if node bbox intersects with query bbox.
			if (maxx < m_boxes[pos]) continue; // maxX < nodeMinX
			if (maxy < m_boxes[pos + 1]) continue; // maxY < nodeMinY
			if (minx > m_boxes[pos + 2]) continue; // minX > nodeMaxX
			if (miny > m_boxes[pos + 3]) continue; // minY > nodeMaxY

			uint32_t index = m_indices[pos >> 2] | 0;

			if (nodeIndex >= m_numItems * 4)
			{
				queue[nQueue++] = index; // node; add it to the search queue.
				assert(nQueue <= sizeof(queue) / sizeof(queue[0]));
			}
			else
			{
				results.emplace_back(index); // leaf item.
			}
		}

		if (nQueue == 0) break;
		nodeIndex = queue[--nQueue];
	}
}

static inline float
axisDist(float k, float min, float max)
{
	return k < min ? min - k : k <= max ? 0 : k - max;
}

struct QueueIndex
{
	float dist;
	uint32_t index;
};

static int
Smaller(const void *a, const void *b)
{
	auto *pa = (QueueIndex *)a;
	auto *pb = (QueueIndex *)b;
	return pa->dist < pb->dist;
}

void
Spatial::neighbors(
    float x,
    float y,
    float maxDist,
    uint32_t maxNeighbors,
    std::vector<uint32_t> &neighbors)
{
	neighbors.clear();
	if (m_numBoxes == 0) return;

	uint32_t nodeIndex = m_numBoxes - 4;

	QueueIndex queue[512]; // Priority queue.
	uint32_t nQueue = 0;

	float maxDistSq = Pow2(maxDist);

	for (;;)
	{
		// Find the end index of the node.
		uint32_t lBound = nodeIndex + m_nodeSize * 4;
		uint32_t uBound = upperBound(nodeIndex, m_numBounds, m_levelBounds);
		uint32_t end = lBound < uBound ? lBound : uBound; // Minimum of two.

		// Add child nodes to the queue.
		for (uint32_t pos = nodeIndex; pos < end; pos += 4)
		{
			uint32_t index = m_indices[pos >> 2] | 0;

			float dx = axisDist(x, m_boxes[pos], m_boxes[pos + 2]);
			float dy = axisDist(y, m_boxes[pos + 1], m_boxes[pos + 3]);
			float dist = dx * dx + dy * dy;
			if (dist > maxDistSq) continue;

			// Check if we can add more item to queue.
			assert(nQueue < sizeof(queue) / sizeof(queue[0]));

			if (nodeIndex >= m_numItems * 4)
			{
				// node (use even id).
				QueueIndex qi{dist, index << 1};
				HeapPush(queue, sizeof(QueueIndex), nQueue, &qi, Smaller);
				nQueue++;
			}
			else
			{
				// leaf item (use odd id).
				QueueIndex qi{dist, (index << 1) + 1};
				HeapPush(queue, sizeof(QueueIndex), nQueue, &qi, Smaller);
				nQueue++;
			}
		}

		// Pop items from the queue.
		while (nQueue && (queue[0].index & 1))
		{
			float dist = queue[0].dist;
			if (dist > maxDistSq) { goto end; }

			neighbors.push_back(queue[0].index >> 1);
			HeapPop(queue, sizeof(QueueIndex), nQueue, Smaller);
			nQueue--;
			if (neighbors.size() == maxNeighbors) { goto end; }
		}

		if (nQueue)
		{
			nodeIndex = queue[0].index >> 1;
			HeapPop(queue, sizeof(QueueIndex), nQueue, Smaller);
			nQueue--;
		}
		else { break; }
	}

end:
	(void)0;
}
