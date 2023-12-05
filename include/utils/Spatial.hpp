#ifndef IE_SPATIAL_HPP
#define IE_SPATIAL_HPP

#include <cstdint>
#include <vector>

namespace ie {
/*███████╗██████╗  █████╗ ████████╗██╗ █████╗ ██╗     */
/*██╔════╝██╔══██╗██╔══██╗╚══██╔══╝██║██╔══██╗██║     */
/*███████╗██████╔╝███████║   ██║   ██║███████║██║     */
/*╚════██║██╔═══╝ ██╔══██║   ██║   ██║██╔══██║██║     */
/*███████║██║     ██║  ██║   ██║   ██║██║  ██║███████╗*/
/*╚══════╝╚═╝     ╚═╝  ╚═╝   ╚═╝   ╚═╝╚═╝  ╚═╝╚══════╝*/

//! \defgroup Spatial
//! @{

/// Packed Hilbert R-tree.
/// You can not add or remove items after initialization, but it is fast to create a new
/// one.
/// https://en.wikipedia.org/wiki/Hilbert_R-tree#Packed_Hilbert_R-trees
class Spatial
{
	explicit Spatial(uint32_t n);
	~Spatial();

	uint32_t Add(float minx, float miny, float maxx, float maxy);

	void AddAll(float *data, uint32_t nFloat);

	void Finish();

	void Search(
	    float minx,
	    float miny,
	    float maxx,
	    float maxy,
	    uint32_t *results,
	    uint32_t *nResults);

	void Neighbors(
	    float x,
	    float y,
	    float maxDist,
	    uint32_t maxNeighbors,
	    std::vector<uint32_t> &neighbors);

private:
	float *mBoxes; ///< minx, miny, maxx, maxy
	uint32_t mNumBoxes; ///< Number of floats in "boxes".
	uint32_t *mIndices;
	uint32_t mNumItems; ///< Number of bounding boxes added.
	uint32_t mNodeSize;
	uint32_t mNumNodes;
	uint32_t mLevelBounds[32]{};
	uint32_t mNumBounds;
	uint32_t mPos;
	float mMinx, mMaxx, mMiny, mMaxy; ///< Bounding box of all the bboxes added.
};

//! @}
} // namespace ie

#endif // IE_SPATIAL_HPP

