#ifndef CANDYBOX_BRESENHAM_HPP__
#define CANDYBOX_BRESENHAM_HPP__

namespace candybox {

//! \defgroup bresenham
//! @{

/// \brief A struct used for computing a bresenham line.
/// \example <pre><code>
/// 		// to use this library, simply follow a template:<br/>
/// 		// (xo, yo): origin, (xd, yd): destination<br/>
/// 		Bresenham data{xo, yo, xd, yd};<br/>
/// 		do {<br/>
/// 			// do something with (xo, yo)<br/>
/// 			// ...<br/>
/// 		} while (!data.Step(&xo, &yo));<br/>
/// </code></pre>
template <typename T = int> struct Bresenham {
  T step_x;
  T step_y;
  T e;
  T delta_x;
  T delta_y;
  T orig_x;
  T orig_y;
  T dest_x;
  T dest_y;

  /// \brief Initialize a bresenham_bresenham_data_t struct. After calling this function,
  /// you can use "Step" to iterate over the individual points on the line.
  /// \param x_from The starting x position.
  /// \param y_from The starting y position.
  /// \param x_to The ending x position.
  /// \param y_to The ending y position.
  /// \param data Pointer to a bresenham_bresenham_data_t struct.
  Bresenham(T x_from, T y_from, T x_to, T y_to);

  /// \brief Render the next point on a line, returns true once the line has ended.
  /// The starting point is excluded by this function.*After the ending point is reached,
  /// the next call will return true.
  /// \param cur_x An pointer to fill with the next x position.
  /// \param cur_y An pointer to fill with the next y position.
  /// \param data Pointer to a initialized bresenham_bresenham_data_t struct.
  /// \return true after the ending point has been reached, false otherwise.
  bool Step(T *cur_x, T *cur_y);
};

template <typename T>
inline Bresenham<T>::Bresenham(T x_from, T y_from, T x_to, T y_to)
    : orig_x(x_from),
      orig_y(y_from),
      dest_x(x_to),
      dest_y(y_to),
      delta_x(x_to - x_from),
      delta_y(y_to - y_from) {
  // Set step_x.
  if (delta_x > 0) step_x = 1;
  else if (delta_x < 0) step_x = -1;
  else step_x = 0;

  // Set step_y.
  if (delta_y > 0) step_y = 1;
  else if (delta_y < 0) step_y = -1;
  else step_y = 0;

  if (step_x * delta_x > step_y * delta_y) {
    e = step_x * delta_x;
    delta_x *= 2;
    delta_y *= 2;
  } else {
    e = step_y * delta_y;
    delta_x *= 2;
    delta_y *= 2;
  }
}

template <typename T> inline bool Bresenham<T>::Step(T *cur_x, T *cur_y) {
  if (step_x * delta_x > step_y * delta_y) {
    if (orig_x == dest_x) return true;
    orig_x += step_x;
    e -= step_y * delta_y;
    if (e < 0) {
      orig_y += step_y;
      e += step_x * delta_x;
    }
  } else {
    if (orig_y == dest_y) return true;
    orig_y += step_y;
    e -= step_x * delta_x;
    if (e < 0) {
      orig_x += step_x;
      e += step_y * delta_y;
    }
  }
  *cur_x = orig_x;
  *cur_y = orig_y;
  return false;
}

} // namespace candybox

#endif // CANDYBOX_BRESENHAM_HPP__
