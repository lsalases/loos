/*
  Fmt.cpp

  Formatter based heavily on example from Stroustrup's C++ book...
*/



#include <Fmt.hpp>


//! Return the bound formatter
BoundFmt Fmt::operator()(double d) const { return(BoundFmt(*this, d)); }

//! Create the output with the specified formatter
std::ostream& operator<<(std::ostream& os, const BoundFmt& bf) {
  std::ostringstream s;

  s.precision(bf.f.prc);
  s.setf(bf.f.fmt, std::ios_base::floatfield);
  s.width(bf.f.wdth);
  s.fill(bf.f.fil);

  switch(bf.f.ali) {
  case Fmt::LEFT:
    s.setf(std::ios_base::left, std::ios_base::adjustfield); break;
  case Fmt::RIGHT:
    s.setf(std::ios_base::right, std::ios_base::adjustfield); break;
  default:
    s.setf(std::ios_base::internal, std::ios_base::adjustfield); break;
  }

  if (bf.f.trl)
    s.setf(std::ios_base::showpoint);
  else
    s.unsetf(std::ios_base::showpoint);

  if (bf.f.pos)
    s.setf(std::ios_base::showpos);
  else
    s.unsetf(std::ios_base::showpos);

  s << bf.val;
  return(os << s.str());
}

