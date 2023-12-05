#include <cmath>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cstdint>
#include <cassert>

#include "utils/Flex.hpp"

#ifdef _MSC_VER
#	pragma warning(push)
#	pragma warning(disable : 4267)
#endif

using namespace ie;

inline void
FlexItem::UpdateShouldOrderChildren()
{
	if (mProps.order != 0 && mParent != nullptr) { mParent->mShouldOrderChildren = true; }
}

void
FlexItem::ChildAdded(FlexItem *child)
{
	assert(child->mParent == nullptr && "child already has a parent");
	child->mParent = this;
	child->UpdateShouldOrderChildren();
}

void
FlexItem::ChildRemoved(FlexItem *child)
{
	child->mParent = nullptr;
}

void
FlexItem::Add(FlexItem *child)
{
	// Check duplicates.
	for (auto &c : mChildren)
		if (c == child) return;

	mChildren.emplace_back(child);
	ChildAdded(child);
}

void
FlexItem::Insert(unsigned int index, FlexItem *child)
{
	assert(index <= mChildren.size());
	mChildren.insert(mChildren.begin() + index, child);
	ChildAdded(child);
}

FlexItem *
FlexItem::Delete(unsigned int index)
{
	assert(index < mChildren.size());
	assert(!mChildren.empty());

	FlexItem *child = mChildren[index];
	ChildRemoved(child);
	mChildren.erase(mChildren.begin() + index);
	return child;
}

unsigned int
FlexItem::Count() const
{
	return mChildren.size();
}

FlexItem *
FlexItem::Child(unsigned int index) const
{
	assert(index < mChildren.size());
	return mChildren[index];
}

FlexItem *
FlexItem::Parent() const
{
	return mParent;
}

FlexItem *
FlexItem::Root() const
{
	auto *item = const_cast<FlexItem *>(this);
	while (mParent != nullptr) item = mParent;
	return item;
}

struct FlexLayoutLine
{
	unsigned int childBegin;
	unsigned int childEnd;
	float size;

	FlexLayoutLine() = default;
	FlexLayoutLine(unsigned int cb, unsigned int ce, float s)
	    : childBegin(cb), childEnd(ce), size(s)
	{
	}
};

struct FlexItem::FlexLayout
{
	// Set during init.
	bool wrap; // Whether wrap items.
	bool mainReversed; // Whether main axis is reversed.
	bool crossReversed; // Whether cross axis is reversed (wrap only)
	bool vertical; // true if main axis is vertical.
	float sizeDim; // Main axis parent size.
	float alignDim; // Cross axis parent size.
	unsigned int mainPosIdx; // Main axis position.
	unsigned int crossPosIdx; // Cross axis position.
	unsigned int mainSizeIdx; // Main axis size.
	unsigned int crossSizeIdx; // Cross axis size.
	std::vector<unsigned int> orderedIndices;

	// Set for each line layout.
	float lineDim; // The cross axis size.
	float flexDim; // The flexible part of the main axis size.
	float extraFlexDim; // Sizes of flexible items.
	float flexGrows;
	float flexShrinks;
	float pos2; // cross axis position.

	// Calculated layout lines - only tracked when needed:
	//   - if the root's alignContent property isn't set to FlexAlign::kStart
	//   - or if any child item doesn't have a cross-axis size set
	bool needLines;
	std::vector<FlexLayoutLine> lines;
	float linesSizes;

	FlexLayout(FlexItem *item, float width, float height) : lineDim(0.f), extraFlexDim(0.f)
	{
		assert(item->mProps.paddingLeft >= 0);
		assert(item->mProps.paddingRight >= 0);
		assert(item->mProps.paddingTop >= 0);
		assert(item->mProps.paddingBottom >= 0);
		width -= item->mProps.paddingLeft + item->mProps.paddingRight;
		height -= item->mProps.paddingTop + item->mProps.paddingBottom;
		assert(width >= 0);
		assert(height >= 0);

		mainReversed = false;
		vertical = true;
		switch (item->mProps.direction)
		{
			case FlexDirection::kRowReverse: mainReversed = true;
			case FlexDirection::kRow:
				vertical = false;
				sizeDim = width;
				alignDim = height;
				mainPosIdx = 0;
				crossPosIdx = 1;
				mainSizeIdx = 2;
				crossSizeIdx = 3;
				break;

			case FlexDirection::kColumnReverse: mainReversed = true;
			case FlexDirection::kColumn:
				sizeDim = height;
				alignDim = width;
				mainPosIdx = 1;
				crossPosIdx = 0;
				mainSizeIdx = 3;
				crossSizeIdx = 2;
				break;
		}

		// Generate indices list for children.
		orderedIndices.resize(item->mChildren.size());
		if (item->mShouldOrderChildren && !item->mChildren.empty())
		{
			// Creating a list of item indices sorted using the children's `order'
			// attribute values. We are using a simple insertion sort as we need
			// stability (insertion order must be preserved) and cross-platform
			// support. We should eventually switch to merge sort (or something
			// else) if the number of items becomes significant enough.
			for (unsigned int i = 0; i < item->mChildren.size(); i++)
			{
				orderedIndices[i] = i;
				for (unsigned int j = i; j > 0; j--)
				{
					unsigned int prev = orderedIndices[j - 1];
					unsigned int curr = orderedIndices[j];
					if (item->mChildren[prev]->mProps.order <=
					    item->mChildren[curr]->mProps.order)
						break;
					orderedIndices[j - 1] = curr;
					orderedIndices[j] = prev;
				}
			}
		}
		else
		{
			for (unsigned int i = 0; i < orderedIndices.size(); i++) orderedIndices[i] = i;
		}

		flexDim = 0.f;
		flexGrows = 0.f;
		flexShrinks = 0.f;

		crossReversed = false;
		wrap = item->mProps.wrap != FlexWrap::kNoWrap;
		if (wrap && item->mProps.wrap == FlexWrap::kWrapReverse)
		{
			crossReversed = true;
			pos2 = alignDim;
		}
		else { pos2 = vertical ? item->mProps.paddingLeft : item->mProps.paddingTop; }

		needLines = wrap && item->mProps.alignContent != FlexAlign::kStart;
		linesSizes = 0.f;
	}

	void Reset()
	{
		lineDim = wrap ? 0 : alignDim;
		flexDim = sizeDim;
		extraFlexDim = 0;
		flexGrows = 0;
		flexShrinks = 0;
	}
};

FlexItem *
FlexItem::ChildAt(FlexItem::FlexLayout *layout, uint32_t index)
{
	return mChildren[layout->orderedIndices[index]];
}

#define MainPos_(child)   child->mFrame[layout->mainPosIdx]
#define CrossPos_(child)  child->mFrame[layout->crossPosIdx]
#define MainSize_(child)  child->mFrame[layout->mainSizeIdx]
#define CrossSize_(child) child->mFrame[layout->crossSizeIdx]

#define ChildMargin_(child, if_vertical, if_horizontal)                                       \
	(layout->vertical                                                                         \
	     ? child->mProps.margin##if_vertical                                                  \
	     : child->mProps.margin##if_horizontal)

static bool
LayoutAlign(
    FlexAlign align,
    float flexDim,
    unsigned int childrenCount,
    float *pos_p,
    float *spacing_p,
    bool stretchAllowed)
{
	assert(flexDim > 0);

	float pos = 0;
	float spacing = 0;
	switch (align)
	{
		case FlexAlign::kStart: break;

		case FlexAlign::kEnd: pos = flexDim; break;

		case FlexAlign::kCenter: pos = flexDim / 2; break;

		case FlexAlign::kSpaceBetween:
			if (childrenCount > 0) { spacing = flexDim / (float)(childrenCount - 1); }
			break;

		case FlexAlign::kSpaceAround:
			if (childrenCount > 0)
			{
				spacing = flexDim / (float)childrenCount;
				pos = spacing / 2;
			}
			break;

		case FlexAlign::kSpaceEvenly:
			if (childrenCount > 0)
			{
				spacing = flexDim / (float)(childrenCount + 1);
				pos = spacing;
			}
			break;

		case FlexAlign::kStretch:
			if (stretchAllowed)
			{
				spacing = flexDim / (float)childrenCount;
				break;
			}
			// fall through

		default: return false;
	}

	*pos_p = pos;
	*spacing_p = spacing;
	return true;
}

FlexAlign
FlexItem::ChildAlign(FlexItem *child, FlexItem *parent)
{
	FlexAlign align = child->mProps.alignSelf;
	if (align == FlexAlign::kAuto) align = parent->mProps.alignItems;
	return align;
}

void
FlexItem::LayoutItems(
    FlexItem *item,
    unsigned int childBegin,
    unsigned int childEnd,
    unsigned int childrenCount,
    FlexLayout *layout)
{
	assert(childrenCount <= childEnd - childBegin);
	if (childrenCount <= 0) return;

	// If the container has a positive flexible space, let's add to it
	// the sizes of all flexible children.
	if (layout->flexDim > 0 && layout->extraFlexDim > 0)
		layout->flexDim += layout->extraFlexDim;

	// Determine the main axis initial position and optional spacing.
	float pos = 0;
	float spacing = 0;
	if (layout->flexGrows == 0 && layout->flexDim > 0)
	{
		bool ret = LayoutAlign(
		    item->mProps.justifyContent, layout->flexDim, childrenCount, &pos, &spacing,
		    false);
		assert(ret && "incorrect justifyContent");
		if (layout->mainReversed) { pos = layout->sizeDim - pos; }
	}

	if (layout->mainReversed)
	{
		pos -= layout->vertical ? item->mProps.paddingBottom : item->mProps.paddingRight;
	}
	else { pos += layout->vertical ? item->mProps.paddingTop : item->mProps.paddingLeft; }
	if (layout->wrap && layout->crossReversed) { layout->pos2 -= layout->lineDim; }

	for (unsigned int i = childBegin; i < childEnd; i++)
	{
		FlexItem *child = item->ChildAt(layout, i);

		// Already positioned.
		if (child->mProps.position == FlexPosition::kAbsolute) continue;

		// Grow or shrink the main axis item size if needed.
		float flexSize = 0;
		if (layout->flexDim > 0)
		{
			if (child->mProps.grow != 0)
			{
				MainSize_(child) = 0; // Ignore previous size when growing.
				flexSize = (layout->flexDim / layout->flexGrows) * child->mProps.grow;
			}
		}
		else if (layout->flexDim < 0)
		{
			if (child->mProps.shrink != 0)
			{
				flexSize = (layout->flexDim / layout->flexShrinks) * child->mProps.shrink;
			}
		}
		MainSize_(child) += flexSize;

		// Set the cross axis position (and stretch the cross axis size if needed).
		float alignSize = CrossSize_(child);
		float alignPos = layout->pos2;
		switch (ChildAlign(child, item))
		{
			case FlexAlign::kEnd:
				alignPos += layout->lineDim - alignSize - ChildMargin_(child, Right, Bottom);
				break;

			case FlexAlign::kCenter:
				alignPos +=
				    (layout->lineDim / 2) - (alignSize / 2) +
				    (ChildMargin_(child, Left, Top) - ChildMargin_(child, Right, Bottom));
				break;

			case FlexAlign::kStretch:
				if (alignSize == 0)
				{
					CrossSize_(child) =
					    layout->lineDim -
					    (ChildMargin_(child, Left, Top) + ChildMargin_(child, Right, Bottom));
				}
				// fall through

			case FlexAlign::kStart: alignPos += ChildMargin_(child, Left, Top); break;

			default: assert(false && "incorrect alignSelf");
		}
		CrossPos_(child) = alignPos;

		// Set the main axis position.
		if (layout->mainReversed)
		{
			pos -= ChildMargin_(child, Bottom, Right);
			pos -= MainSize_(child);
			MainPos_(child) = pos;
			pos -= spacing;
			pos -= ChildMargin_(child, Top, Left);
		}
		else
		{
			pos += ChildMargin_(child, Top, Left);
			MainPos_(child) = pos;
			pos += MainSize_(child);
			pos += spacing;
			pos += ChildMargin_(child, Bottom, Right);
		}

		// Now that the item has a frame, we can layout its children.
		LayoutItem(child, child->mFrame[2], child->mFrame[3]);
	}

	if (layout->wrap && !layout->crossReversed) { layout->pos2 += layout->lineDim; }

	if (layout->needLines)
	{
		FlexLayoutLine line{childBegin, childEnd, layout->lineDim};
		layout->lines.emplace_back(line);
		layout->linesSizes += line.size;
	}
}

static float
AbsoluteSize(float val, float pos1, float pos2, float dim)
{
	return !std::isnan(val)
	           ? val
	           : (!std::isnan(pos1) && !std::isnan(pos2) ? dim - pos2 - pos1 : 0);
}

static float
AbsolutePos(float pos1, float pos2, float size, float dim)
{
	return !std::isnan(pos1) ? pos1 : (!std::isnan(pos2) ? dim - size - pos2 : 0);
}

void
FlexItem::LayoutItem(FlexItem *item, float width, float height)
{
	if (item->mChildren.empty()) return;

	FlexLayout layout_s{item, width, height}, *layout = &layout_s;

	layout->Reset();

	unsigned int lastLayoutChild = 0;
	unsigned int relativeChildrenCount = 0;
	for (unsigned int i = 0; i < item->mChildren.size(); i++)
	{
		FlexItem *child = item->ChildAt(layout, i);

		// Items with an absolute position have their frames determined
		// directly and are skipped during layout.
		if (child->mProps.position == FlexPosition::kAbsolute)
		{
			const FlexItemProperties &cp = child->mProps;
			float childWidth = AbsoluteSize(cp.width, cp.left, cp.right, width);
			float childHeight = AbsoluteSize(cp.height, cp.top, cp.bottom, height);
			float childX = AbsolutePos(cp.left, cp.right, childWidth, width);
			float childY = AbsolutePos(cp.top, cp.bottom, childHeight, height);

			child->mFrame[0] = childX;
			child->mFrame[1] = childY;
			child->mFrame[2] = childWidth;
			child->mFrame[3] = childHeight;

			// Now that the item has a frame, we can layout its children.
			LayoutItem(child, childWidth, childHeight);

			continue;
		}

		// Initialize frame.
		child->mFrame[0] = 0;
		child->mFrame[1] = 0;
		child->mFrame[2] = child->mProps.width;
		child->mFrame[3] = child->mProps.height;

		// Main axis size defaults to 0.
		if (std::isnan(MainSize_(child))) MainSize_(child) = 0.f;

		// Cross axis size defaults to the parent's size (or line size in wrap
		// mode, which is calculated later on).
		if (std::isnan(CrossSize_(child)))
		{
			if (layout->wrap) { layout->needLines = true; }
			else
			{
				CrossSize_(child) =
				    (layout->vertical ? width : height) - ChildMargin_(child, Left, Top) -
				    ChildMargin_(child, Right, Bottom);
				if (child->mParent)
				{
					CrossSize_(child) -=
					    layout->vertical
					        ? (child->mParent->mProps.paddingLeft +
					           child->mParent->mProps.paddingRight)
					        : (child->mParent->mProps.paddingTop +
					           child->mParent->mProps.paddingBottom);
				}
			}
		}

		// Call the selfSizing callback if provided. Only non-NAN values
		// are taken into account. If the item's cross-axis align property
		// is set to stretch, ignore the value returned by the callback.
		if (child->mProps.selfSizing != nullptr)
		{
			float size[2] = {child->mFrame[2], child->mFrame[3]};

			child->mProps.selfSizing(child, size);

			for (unsigned int j = 0; j < 2; j++)
			{
				const unsigned int sizeOff = j + 2;
				if (sizeOff == layout->crossSizeIdx &&
				    ChildAlign(child, item) == FlexAlign::kStretch)
				{
					continue;
				}
				float val = size[j];
				if (!std::isnan(val)) { child->mFrame[sizeOff] = val; }
			}
		}

		// Honor the `basis' property which overrides the main-axis size.
		if (!std::isnan(child->mProps.basis))
		{
			assert(child->mProps.basis >= 0);
			MainSize_(child) = child->mProps.basis;
		}

		float childSize = MainSize_(child);
		if (layout->wrap)
		{
			if (layout->flexDim < childSize)
			{
				// Not enough space for this child on this line, layout the
				// remaining items and move it to a new line.
				LayoutItems(item, lastLayoutChild, i, relativeChildrenCount, layout);

				layout->Reset();
				lastLayoutChild = i;
				relativeChildrenCount = 0;
			}

			float childSize2 = CrossSize_(child);
			if (!std::isnan(childSize2) && childSize2 > layout->lineDim)
			{
				layout->lineDim = childSize2;
			}
		}

		assert(child->mProps.grow >= 0);
		assert(child->mProps.shrink >= 0);

		layout->flexGrows += child->mProps.grow;
		layout->flexShrinks += child->mProps.shrink;

		layout->flexDim -=
		    childSize + ChildMargin_(child, Top, Left) + ChildMargin_(child, Bottom, Right);

		relativeChildrenCount++;

		if (childSize > 0 && child->mProps.grow > 0) { layout->extraFlexDim += childSize; }
	}

	// Layout remaining items in wrap mode, or everything otherwise.
	LayoutItems(item, lastLayoutChild, item->mChildren.size(), relativeChildrenCount, layout);

	// In wrap mode we may need to tweak the position of each line according to
	// the alignContent property as well as the cross-axis size of items that
	// haven't been set yet.
	if (layout->needLines && !layout->lines.empty())
	{
		float pos = 0;
		float spacing = 0;
		float flexDim = layout->alignDim - layout->linesSizes;
		if (flexDim > 0)
		{
			bool ret = LayoutAlign(
			    item->mProps.alignContent, flexDim, layout->lines.size(), &pos, &spacing,
			    true);
			assert(ret && "incorrect alignContent");
		}

		float oldPos = 0;
		if (layout->crossReversed)
		{
			pos = layout->alignDim - pos;
			oldPos = layout->alignDim;
		}

		for (const auto &line : layout->lines)
		{
			if (layout->crossReversed)
			{
				pos -= line.size;
				pos -= spacing;
				oldPos -= line.size;
			}

			// Re-position the children of this line, honoring any child
			// alignment previously set within the line.
			for (unsigned int j = line.childBegin; j < line.childEnd; j++)
			{
				FlexItem *child = item->ChildAt(layout, j);
				if (child->mProps.position == FlexPosition::kAbsolute)
				{
					// Should not be re-positioned.
					continue;
				}
				if (std::isnan(CrossSize_(child)))
				{
					// If the child's cross axis size hasn't been set it, it
					// defaults to the line size.
					CrossSize_(child) =
					    line.size +
					    (item->mProps.alignContent == FlexAlign::kStretch ? spacing : 0) -
					    (layout->vertical
					         ? (child->mProps.marginLeft + child->mProps.marginRight)
					         : (child->mProps.marginTop = child->mProps.marginBottom));
				}
				CrossPos_(child) = pos + (CrossPos_(child) - oldPos);
			}

			if (!layout->crossReversed)
			{
				pos += line.size;
				pos += spacing;
				oldPos += line.size;
			}
		}
	}
}

#undef ChildMargin_
#undef MainPos_
#undef CrossPos_
#undef MainSize_
#undef CrossSize_
#undef ChildAt_

void
FlexItem::Layout()
{
	assert(mParent == nullptr);
	assert(!std::isnan(mProps.width));
	assert(!std::isnan(mProps.height));
	assert(mProps.selfSizing == nullptr);

	LayoutItem(this, mProps.width, mProps.height);
}

#ifdef _MSC_VER
#	pragma warning(pop)
#endif
