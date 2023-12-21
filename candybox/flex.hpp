#ifndef IE_FLEX_HPP
#define IE_FLEX_HPP

#include <vector>
#include <cassert>

namespace ie {
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

	void Add(FlexItem *child);
	void Insert(unsigned int index, FlexItem *child);
	FlexItem *Delete(unsigned int index);
	// Layout the items associated with this item, as well as their children.
	/// This function can only be called on a root item whose `width' and `height'
	/// properties have been set.
	/// Also notice that you can't retreive its size and position by "FrameXX".
	/// In order to properly set this, you will need another FlexItem to contain it.
	void Layout();
	unsigned int Count() const;
	FlexItem *Child(unsigned int index) const;
	FlexItem *Parent() const;
	FlexItem *Root() const;
	float FrameX() const { return mFrame[0]; }
	float FrameY() const { return mFrame[1]; }
	float FrameWidth() const { return mFrame[2]; }
	float FrameHeight() const { return mFrame[3]; }

	// Getters and Setters to encapsulate private properties.

	// clang-format off
	void SetWidth(float w) { mProps.width = w; }
	void SetHeight(float h) { mProps.height = h; }
	void SetSize(float w, float h) { mProps.width = w; mProps.height = h; }
	void SetLeft(float l) { mProps.left = l; }
	void SetRight(float r) { mProps.right = r; }
	void SetTop(float t) { mProps.top = t; }
	void SetBottom(float b) { mProps.bottom = b; }
	void SetPaddingLeft(float l) { mProps.paddingLeft = l; }
	void SetPaddingRight(float r) { mProps.paddingRight = r; }
	void SetPaddingTop(float t) { mProps.paddingTop = t; }
	void SetPaddingBottom(float b) { mProps.paddingBottom = b; }
	void SetPadding(float left, float right, float top, float bottom) { SetPaddingLeft(left); SetPaddingRight(right); SetPaddingTop(top); SetPaddingBottom(bottom); }
	void SetMarginLeft(float l) { mProps.marginLeft = l; }
	void SetMarginRight(float r) { mProps.marginRight = r; }
	void SetMarginTop(float t) { mProps.marginTop = t; }
	void SetMarginBottom(float b) { mProps.marginBottom = b; }
	void SetMargin(float left, float right, float top, float bottom) { SetMarginLeft(left); SetMarginRight(right); SetMarginTop(top); SetMarginBottom(bottom); }
	void SetJustifyContent(FlexAlign align) { mProps.justifyContent = align; assert(align != FlexAlign::kAuto); }
	void SetAlignContent(FlexAlign align) { mProps.alignContent = align; assert(align != FlexAlign::kAuto); }
	void SetAlignItems(FlexAlign align) { mProps.alignItems = align; assert(align != FlexAlign::kAuto); }
	void SetAlignSelf(FlexAlign align) { mProps.alignSelf = align; }
	void SetPosition(FlexPosition position) { mProps.position = position; }
	void SetDirection(FlexDirection direction) { mProps.direction = direction; }
	void SetWrap(FlexWrap wrap) { mProps.wrap = wrap; }
	void SetGrow(float grow) { mProps.grow = grow; }
	void SetShrink(float shrink) { mProps.shrink = shrink; }
	void SetOrder(int order) { mProps.order = order; UpdateShouldOrderChildren(); }
	void SetBasis(float basis) { mProps.basis = basis; }
	void SetManagedPtr(void *ptr) { mProps.managedPtr = ptr; }
	void SetSelfSizing(FlexSelfSizing sizing) { mProps.selfSizing = sizing; }
	float GetWidth() const { return mProps.width; }
	float GetHeight() const { return mProps.height; }
	float GetLeft() const  { return mProps.left; }
	float GetRight() const { return mProps.right; }
	float GetTop() const { return mProps.top; }
	float GetBottom() const { return mProps.bottom; }
	float GetPaddingLeft() const { return mProps.paddingLeft; }
	float GetPaddingRight() const { return mProps.paddingLeft; }
	float GetPaddingTop() const { return mProps.paddingTop; }
	float GetPaddingBottom() const { return mProps.paddingBottom; }
	float GetMarginLeft() const { return mProps.marginLeft; }
	float GetMarginRight() const { return mProps.marginRight; }
	float GetMarginTop() const { return mProps.marginTop; }
	float GetMarginBottom() const { return mProps.marginBottom; }
	FlexAlign GetJustifyContent() const { return mProps.justifyContent; }
	FlexAlign GetAlignContent() const { return mProps.alignContent; }
	FlexAlign GetAlignItems() const { return mProps.alignItems; }
	FlexAlign GetAlignSelf() const { return mProps.alignSelf; }
	FlexPosition GetPosition() const { return mProps.position; }
	FlexDirection GetDirection() const { return mProps.direction; }
	FlexWrap GetWrap() const { return mProps.wrap; }
	float GetGrow() const { return mProps.grow; }
	float GetShrink() const { return mProps.shrink; }
	int GetOrder() const { return mProps.order; }
	float GetBasis() const { return mProps.basis; }
	void *GetManagedPtr() const { return mProps.managedPtr; }
	FlexSelfSizing GetSelfSizing() const { return mProps.selfSizing; }
	// clang-format on

private:
	struct FlexLayout;

	void UpdateShouldOrderChildren();
	void ChildAdded(FlexItem *child);
	static void ChildRemoved(FlexItem *child);
	static FlexAlign ChildAlign(FlexItem *child, FlexItem *parent);
	static void LayoutItems(
	    FlexItem *item,
	    unsigned int childBegin,
	    unsigned int childEnd,
	    unsigned int childrenCount,
	    FlexLayout *layout);
	static void LayoutItem(FlexItem *item, float width, float height);
	FlexItem *ChildAt(FlexLayout *layout, uint32_t index);
};

//! @}
// NOLINTEND(modernize-use-nodiscard)

} // namespace ie

#endif // IE_FLEX_HPP
