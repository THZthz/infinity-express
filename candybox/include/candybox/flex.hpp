#ifndef CANDYBOX_FLEX_HPP__
#define CANDYBOX_FLEX_HPP__

#include <vector>
#include <cassert>

namespace candybox {
// NOLINTBEGIN(modernize-use-nodiscard)

//! \defgroup flex
//! @{

//███████╗██╗     ███████╗██╗  ██╗
//██╔════╝██║     ██╔════╝╚██╗██╔╝
//█████╗  ██║     █████╗   ╚███╔╝
//██╔══╝  ██║     ██╔══╝   ██╔██╗
//██║     ███████╗███████╗██╔╝ ██╗
//╚═╝     ╚══════╝╚══════╝╚═╝  ╚═╝

enum class FlexAlign
{
	kAuto = 0, ///< Can only be used in alignSelf.
	kStretch,
	kCenter,
	kStart,
	kEnd,
	kSpaceBetween,
	kSpaceAround,
	kSpaceEvenly,
	//kBaseline ///< Align with the first line of the text. // TODO
};

enum class FlexPosition
{
	kRelative = 0,
	kAbsolute
};

enum class FlexDirection
{
	kRow = 0,
	kRowReverse,
	kColumn,
	kColumnReverse
};

enum class FlexWrap
{
	kNoWrap = 0,
	kWrap,
	kWrapReverse
};

class FlexItem;

// size[0] == width, size[1] == height
typedef void (*FlexSelfSizing)(FlexItem *item, float size[2]);

class FlexItem
{
public:
	struct FlexItemProperties
	{
		float width{NAN}, height{NAN};
		float left{NAN}, right{NAN}, top{NAN}, bottom{NAN};
		float paddingLeft{0}, paddingRight{0}, paddingTop{0}, paddingBottom{0};
		float marginLeft{0}, marginRight{0}, marginTop{0}, marginBottom{0};
		/// How items align in main axis.
		FlexAlign justifyContent{FlexAlign::kStart};
		/// Define the alignment of multiple axses.
		FlexAlign alignContent{FlexAlign::kStretch};
		/// How items align in cross axis.
		FlexAlign alignItems{FlexAlign::kStretch};
		/// Override alignItems property of the parent.
		FlexAlign alignSelf{FlexAlign::kAuto};
		FlexPosition position{FlexPosition::kRelative};
		/// Decide the direction of main axis (where items are arranged).
		FlexDirection direction{FlexDirection::kColumn};
		FlexWrap wrap{FlexWrap::kNoWrap};
		/// Magnitude factor.
		float grow{0};
		float shrink{0};
		/// Item with smaller order value is further forward.
		int order{0};
		/// Defines the main axis space occupied by the item before the excess space is
		/// allocated. Based on this property, it is calculated whether there is extra space
		/// for the main axis. Its default value is auto, which is the original size of
		/// the item.
		float basis{NAN};

		/// An item can store an arbitrary pointer, which can be used by bindings as
		/// the address of a managed object.
		void *managedPtr{nullptr};

		/// An item can provide a selfSizing callback function that will be called
		/// during layout and which can customize the dimensions (width and height)
		/// of the item.
		FlexSelfSizing selfSizing{nullptr};
	};

private:
	FlexItemProperties mProps{};
	float mFrame[4]{0, 0, 0, 0};
	FlexItem *mParent{nullptr};
	std::vector<FlexItem *> mChildren;
	bool mShouldOrderChildren{false};

public:
	FlexItem() = default;

	void add(FlexItem *child);
	void insert(unsigned int index, FlexItem *child);
	FlexItem *deleteAt(unsigned int index);
	// Layout the items associated with this item, as well as their children.
	/// This function can only be called on a root item whose `width' and `height'
	/// properties have been set.
	/// Also notice that you can't retreive its size and position by "FrameXX".
	/// In order to properly set this, you will need another FlexItem to contain it.
	void performLayout();
	unsigned int getCount() const;
	FlexItem *getChild(unsigned int index) const;
	FlexItem *getParent() const;
	FlexItem *getRoot() const;
	float frameX() const { return mFrame[0]; }
	float frameY() const { return mFrame[1]; }
	float frameWidth() const { return mFrame[2]; }
	float frameHeight() const { return mFrame[3]; }

	// Getters and Setters to encapsulate private properties.

	// clang-format off
	void setWidth(float w) { mProps.width = w; }
	void setHeight(float h) { mProps.height = h; }
	void setSize(float w, float h) { mProps.width = w; mProps.height = h; }
	void setLeft(float l) { mProps.left = l; }
	void setRight(float r) { mProps.right = r; }
	void setTop(float t) { mProps.top = t; }
	void setBottom(float b) { mProps.bottom = b; }
	void setPaddingLeft(float l) { mProps.paddingLeft = l; }
	void setPaddingRight(float r) { mProps.paddingRight = r; }
	void setPaddingTop(float t) { mProps.paddingTop = t; }
	void setPaddingBottom(float b) { mProps.paddingBottom = b; }
	void setPadding(float left, float right, float top, float bottom) { setPaddingLeft(left); setPaddingRight(right); setPaddingTop(top); setPaddingBottom(bottom); }
	void setMarginLeft(float l) { mProps.marginLeft = l; }
	void setMarginRight(float r) { mProps.marginRight = r; }
	void setMarginTop(float t) { mProps.marginTop = t; }
	void setMarginBottom(float b) { mProps.marginBottom = b; }
	void setMargin(float left, float right, float top, float bottom) { setMarginLeft(left); setMarginRight(right); setMarginTop(top); setMarginBottom(bottom); }
	void setJustifyContent(FlexAlign align) { mProps.justifyContent = align; assert(align != FlexAlign::kAuto); }
	void setAlignContent(FlexAlign align) { mProps.alignContent = align; assert(align != FlexAlign::kAuto); }
	void setAlignItems(FlexAlign align) { mProps.alignItems = align; assert(align != FlexAlign::kAuto); }
	void setAlignSelf(FlexAlign align) { mProps.alignSelf = align; }
	void setPosition(FlexPosition position) { mProps.position = position; }
	void setDirection(FlexDirection direction) { mProps.direction = direction; }
	void setWrap(FlexWrap wrap) { mProps.wrap = wrap; }
	void setGrow(float grow) { mProps.grow = grow; }
	void setShrink(float shrink) { mProps.shrink = shrink; }
	void setOrder(int order) { mProps.order = order; updateShouldOrderChildren(); }
	void setBasis(float basis) { mProps.basis = basis; }
	void setManagedPtr(void *ptr) { mProps.managedPtr = ptr; }
	void setSelfSizing(FlexSelfSizing sizing) { mProps.selfSizing = sizing; }
	float getWidth() const { return mProps.width; }
	float getHeight() const { return mProps.height; }
	float getLeft() const  { return mProps.left; }
	float getRight() const { return mProps.right; }
	float getTop() const { return mProps.top; }
	float getBottom() const { return mProps.bottom; }
	float getPaddingLeft() const { return mProps.paddingLeft; }
	float getPaddingRight() const { return mProps.paddingLeft; }
	float getPaddingTop() const { return mProps.paddingTop; }
	float getPaddingBottom() const { return mProps.paddingBottom; }
	float getMarginLeft() const { return mProps.marginLeft; }
	float getMarginRight() const { return mProps.marginRight; }
	float getMarginTop() const { return mProps.marginTop; }
	float getMarginBottom() const { return mProps.marginBottom; }
	FlexAlign getJustifyContent() const { return mProps.justifyContent; }
	FlexAlign getAlignContent() const { return mProps.alignContent; }
	FlexAlign getAlignItems() const { return mProps.alignItems; }
	FlexAlign getAlignSelf() const { return mProps.alignSelf; }
	FlexPosition getPosition() const { return mProps.position; }
	FlexDirection getDirection() const { return mProps.direction; }
	FlexWrap getWrap() const { return mProps.wrap; }
	float getGrow() const { return mProps.grow; }
	float getShrink() const { return mProps.shrink; }
	int getOrder() const { return mProps.order; }
	float getBasis() const { return mProps.basis; }
	void *getManagedPtr() const { return mProps.managedPtr; }
	FlexSelfSizing getSelfSizing() const { return mProps.selfSizing; }
	// clang-format on

private:
	struct FlexLayout;

	void updateShouldOrderChildren();
	void childAdded(FlexItem *child);
	static void childRemoved(FlexItem *child);
	static FlexAlign childAlign(FlexItem *child, FlexItem *parent);
	static void layoutItems(
	    FlexItem *item,
	    unsigned int childBegin,
	    unsigned int childEnd,
	    unsigned int childrenCount,
	    FlexLayout *layout);
	static void layoutItem(FlexItem *item, float width, float height);
	FlexItem *childAt(FlexLayout *layout, uint32_t index);
};

//! @}
// NOLINTEND(modernize-use-nodiscard)

} // namespace candybox

#endif // CANDYBOX_FLEX_HPP__
