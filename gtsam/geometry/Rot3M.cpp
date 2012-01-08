/* ----------------------------------------------------------------------------

 * GTSAM Copyright 2010, Georgia Tech Research Corporation, 
 * Atlanta, Georgia 30332-0415
 * All Rights Reserved
 * Authors: Frank Dellaert, et al. (see THANKS for the full author list)

 * See LICENSE for the license information

 * -------------------------------------------------------------------------- */

/**
 * @file    Rot3M.cpp
 * @brief   Rotation (internal: 3*3 matrix representation*)
 * @author  Alireza Fathi
 * @author  Christian Potthast
 * @author  Frank Dellaert
 * @author  Richard Roberts
 */

#ifndef GTSAM_DEFAULT_QUATERNIONS

#include <gtsam/geometry/Rot3.h>
#include <boost/math/constants/constants.hpp>
#include <cmath>

using namespace std;

namespace gtsam {

static const Matrix I3 = eye(3);

/* ************************************************************************* */
Rot3::Rot3() :
    r1_(Point3(1.0,0.0,0.0)),
    r2_(Point3(0.0,1.0,0.0)),
    r3_(Point3(0.0,0.0,1.0)) {}

/* ************************************************************************* */
Rot3::Rot3(const Point3& r1, const Point3& r2, const Point3& r3) :
    r1_(r1), r2_(r2), r3_(r3) {}

/* ************************************************************************* */
Rot3::Rot3(double R11, double R12, double R13,
    double R21, double R22, double R23,
    double R31, double R32, double R33) :
      r1_(Point3(R11, R21, R31)),
      r2_(Point3(R12, R22, R32)),
      r3_(Point3(R13, R23, R33)) {}

/* ************************************************************************* */
Rot3::Rot3(const Matrix& R):
  r1_(Point3(R(0,0), R(1,0), R(2,0))),
  r2_(Point3(R(0,1), R(1,1), R(2,1))),
  r3_(Point3(R(0,2), R(1,2), R(2,2))) {}

/* ************************************************************************* */
Rot3::Rot3(const Quaternion& q) {
  Eigen::Matrix3d R = q.toRotationMatrix();
  r1_ = Point3(R.col(0));
  r2_ = Point3(R.col(1));
  r3_ = Point3(R.col(2));
}

/* ************************************************************************* */
Rot3 Rot3::Rx(double t) {
	double st = sin(t), ct = cos(t);
	return Rot3(
			1,  0,  0,
			0, ct,-st,
			0, st, ct);
}

/* ************************************************************************* */
Rot3 Rot3::Ry(double t) {
	double st = sin(t), ct = cos(t);
	return Rot3(
			ct, 0, st,
			0, 1,  0,
			-st, 0, ct);
}

/* ************************************************************************* */
Rot3 Rot3::Rz(double t) {
	double st = sin(t), ct = cos(t);
	return Rot3(
			ct,-st, 0,
			st, ct, 0,
			0,  0, 1);
}

/* ************************************************************************* */
// Considerably faster than composing matrices above !
Rot3 Rot3::RzRyRx(double x, double y, double z) {
	double cx=cos(x),sx=sin(x);
	double cy=cos(y),sy=sin(y);
	double cz=cos(z),sz=sin(z);
	double ss_ = sx * sy;
	double cs_ = cx * sy;
	double sc_ = sx * cy;
	double cc_ = cx * cy;
	double c_s = cx * sz;
	double s_s = sx * sz;
	double _cs = cy * sz;
	double _cc = cy * cz;
	double s_c = sx * cz;
	double c_c = cx * cz;
	double ssc = ss_ * cz, csc = cs_ * cz, sss = ss_ * sz, css = cs_ * sz;
	return Rot3(
			_cc,- c_s + ssc,  s_s + csc,
			_cs,  c_c + sss, -s_c + css,
			-sy,        sc_,        cc_
	);
}

/* ************************************************************************* */
Rot3 Rot3::rodriguez(const Vector& w, double theta) {
	// get components of axis \omega
	double wx = w(0), wy=w(1), wz=w(2);
	double wwTxx = wx*wx, wwTyy = wy*wy, wwTzz = wz*wz;
#ifndef NDEBUG
	double l_n = wwTxx + wwTyy + wwTzz;
	if (std::abs(l_n-1.0)>1e-9) throw domain_error("rodriguez: length of n should be 1");
#endif

	double c = cos(theta), s = sin(theta), c_1 = 1 - c;

	double swx = wx * s, swy = wy * s, swz = wz * s;
	double C00 = c_1*wwTxx, C01 = c_1*wx*wy, C02 = c_1*wx*wz;
	double                  C11 = c_1*wwTyy, C12 = c_1*wy*wz;
	double                                   C22 = c_1*wwTzz;

	return Rot3(
			  c + C00, -swz + C01,  swy + C02,
			swz + C01,    c + C11, -swx + C12,
		 -swy + C02,  swx + C12,    c + C22);
}

/* ************************************************************************* */
Rot3 Rot3::rodriguez(const Vector& w) {
	double t = w.norm();
	if (t < 1e-10) return Rot3();
	return rodriguez(w/t, t);
}

/* ************************************************************************* */
bool Rot3::equals(const Rot3 & R, double tol) const {
	return equal_with_abs_tol(matrix(), R.matrix(), tol);
}

/* ************************************************************************* */
Rot3 Rot3::compose (const Rot3& R2,
    boost::optional<Matrix&> H1, boost::optional<Matrix&> H2) const {
  if (H1) *H1 = R2.transpose();
  if (H2) *H2 = I3;
  return *this * R2;
}

/* ************************************************************************* */
Point3 Rot3::operator*(const Point3& p) const { return rotate(p); }

/* ************************************************************************* */
Rot3 Rot3::inverse(boost::optional<Matrix&> H1) const {
  if (H1) *H1 = -matrix();
  return Rot3(
      r1_.x(), r1_.y(), r1_.z(),
      r2_.x(), r2_.y(), r2_.z(),
      r3_.x(), r3_.y(), r3_.z());
}

/* ************************************************************************* */
Rot3 Rot3::between (const Rot3& R2,
    boost::optional<Matrix&> H1, boost::optional<Matrix&> H2) const {
  if (H1) *H1 = -(R2.transpose()*matrix());
  if (H2) *H2 = I3;
  return between_default(*this, R2);
}

/* ************************************************************************* */
Rot3 Rot3::operator*(const Rot3& R2) const {
  return Rot3(rotate(R2.r1_), rotate(R2.r2_), rotate(R2.r3_));
}

/* ************************************************************************* */
Point3 Rot3::rotate(const Point3& p,
    boost::optional<Matrix&> H1,  boost::optional<Matrix&> H2) const {
	if (H1 || H2) {
			const Matrix R(matrix());
			if (H1) *H1 = R * skewSymmetric(-p.x(), -p.y(), -p.z());
			if (H2) *H2 = R;
		}
  return r1_ * p.x() + r2_ * p.y() + r3_ * p.z();
}

/* ************************************************************************* */
// see doc/math.lyx, SO(3) section
Point3 Rot3::unrotate(const Point3& p,
    boost::optional<Matrix&> H1, boost::optional<Matrix&> H2) const {
  Point3 q(
			r1_.x()*p.x()+r1_.y()*p.y()+r1_.z()*p.z(),
			r2_.x()*p.x()+r2_.y()*p.y()+r2_.z()*p.z(),
			r3_.x()*p.x()+r3_.y()*p.y()+r3_.z()*p.z()
  		); // q = Rt*p
  if (H1) *H1 = skewSymmetric(q.x(), q.y(), q.z());
  if (H2) *H2 = transpose();
  return q;
}

/* ************************************************************************* */
// Log map at identity - return the canonical coordinates of this rotation
Vector Rot3::Logmap(const Rot3& R) {

	static const double PI = boost::math::constants::pi<double>();

	// Get trace(R)
	double tr = R.r1_.x()+R.r2_.y()+R.r3_.z();

	// when trace == -1, i.e., when theta = +-pi, +-3pi, +-5pi, etc.
	// we do something special
	if (std::abs(tr+1.0) < 1e-10) {
    if(std::abs(R.r3_.z()+1.0) > 1e-10)
      return (PI / sqrt(2.0+2.0*R.r3_.z())) *
          Vector_(3, R.r3_.x(), R.r3_.y(), 1.0+R.r3_.z());
    else if(std::abs(R.r2_.y()+1.0) > 1e-10)
      return (PI / sqrt(2.0+2.0*R.r2_.y())) *
          Vector_(3, R.r2_.x(), 1.0+R.r2_.y(), R.r2_.z());
    else // if(std::abs(R.r1_.x()+1.0) > 1e-10)  This is implicit
      return (PI / sqrt(2.0+2.0*R.r1_.x())) *
          Vector_(3, 1.0+R.r1_.x(), R.r1_.y(), R.r1_.z());
  } else {
		double magnitude;
		double tr_3 = tr-3.0; // always negative
		if (tr_3<-1e-7) {
			double theta = acos((tr-1.0)/2.0);
			magnitude = theta/(2.0*sin(theta));
		} else {
			// when theta near 0, +-2pi, +-4pi, etc. (trace near 3.0)
			// use Taylor expansion: magnitude \approx 1/2-(t-3)/12 + O((t-3)^2)
			magnitude = 0.5 - tr_3*tr_3/12.0;
		}
		return magnitude*Vector_(3,
				R.r2_.z()-R.r3_.y(),
				R.r3_.x()-R.r1_.z(),
				R.r1_.y()-R.r2_.x());
  }
}

/* ************************************************************************* */
Rot3 Rot3::retract(const Vector& omega, Rot3::CoordinatesMode mode) const {
  if(mode == Rot3::EXPMAP) {
    return (*this)*Expmap(omega);
  } else if(mode == Rot3::CALEY) {
    const double x = omega(0), y = omega(1), z = omega(2);
    const double x2 = x*x, y2 = y*y, z2 = z*z;
    const double xy = x*y, xz = x*z, yz = y*z;
    const double f = 1.0 / (4.0 + x2 + y2 + z2), _2f = 2.0*f;
    return (*this)* Rot3(
        (4+x2-y2-z2)*f, (xy - 2*z)*_2f, (xz + 2*y)*_2f,
        (xy + 2*z)*_2f, (4-x2+y2-z2)*f, (yz - 2*x)*_2f,
        (xz - 2*y)*_2f, (yz + 2*x)*_2f, (4-x2-y2+z2)*f
    );
  } else if(mode == Rot3::SLOW_CALEY) {
    Matrix Omega = skewSymmetric(omega);
    return (*this)*Cayley<3>(-Omega/2);
  } else {
    assert(false);
    exit(1);
  }
}

/* ************************************************************************* */
Vector Rot3::localCoordinates(const Rot3& T, Rot3::CoordinatesMode mode) const {
  if(mode == Rot3::EXPMAP) {
    return Logmap(between(T));
  } else if(mode == Rot3::CALEY) {
    // Create a fixed-size matrix
    Eigen::Matrix3d A(between(T).matrix());
    // Mathematica closed form optimization (procrastination?) gone wild:
    const double a=A(0,0),b=A(0,1),c=A(0,2);
    const double d=A(1,0),e=A(1,1),f=A(1,2);
    const double g=A(2,0),h=A(2,1),i=A(2,2);
    const double di = d*i, ce = c*e, cd = c*d, fg=f*g;
    const double M = 1 + e - f*h + i + e*i;
    const double K = 2.0 / (cd*h + M + a*M -g*(c + ce) - b*(d + di - fg));
    const double x = (a * f - cd + f) * K;
    const double y = (b * f - ce - c) * K;
    const double z = (fg - di - d) * K;
    return -2 * Vector_(3, x, y, z);
  } else if(mode == Rot3::SLOW_CALEY) {
    // Create a fixed-size matrix
    Eigen::Matrix3d A(between(T).matrix());
    // using templated version of Cayley
    Matrix Omega = Cayley<3>(A);
    return -2*Vector_(3,Omega(2,1),Omega(0,2),Omega(1,0));
  } else {
    assert(false);
    exit(1);
  }
}

/* ************************************************************************* */
Matrix Rot3::matrix() const {
	Matrix R(3,3);
	R <<
			r1_.x(), r2_.x(), r3_.x(),
			r1_.y(), r2_.y(), r3_.y(),
			r1_.z(), r2_.z(), r3_.z();
	return R;
}

/* ************************************************************************* */
Matrix Rot3::transpose() const {
	Matrix Rt(3,3);
	Rt <<
			r1_.x(), r1_.y(), r1_.z(),
			r2_.x(), r2_.y(), r2_.z(),
			r3_.x(), r3_.y(), r3_.z();
	return Rt;
}

/* ************************************************************************* */
Point3 Rot3::column(int index) const{
	if(index == 3)
		return r3_;
	else if(index == 2)
		return r2_;
	else if(index == 1)
		return r1_; // default returns r1
	else
	  throw invalid_argument("Argument to Rot3::column must be 1, 2, or 3");
}

/* ************************************************************************* */
Point3 Rot3::r1() const { return r1_; }

/* ************************************************************************* */
Point3 Rot3::r2() const { return r2_; }

/* ************************************************************************* */
Point3 Rot3::r3() const { return r3_; }

/* ************************************************************************* */
Vector Rot3::xyz() const {
	Matrix I;Vector q;
	boost::tie(I,q)=RQ(matrix());
	return q;
}

/* ************************************************************************* */
Vector Rot3::ypr() const {
	Vector q = xyz();
	return Vector_(3,q(2),q(1),q(0));
}

/* ************************************************************************* */
Vector Rot3::rpy() const {
	Vector q = xyz();
	return Vector_(3,q(0),q(1),q(2));
}

/* ************************************************************************* */
Quaternion Rot3::toQuaternion() const {
  return Quaternion((Eigen::Matrix3d() <<
      r1_.x(), r2_.x(), r3_.x(),
      r1_.y(), r2_.y(), r3_.y(),
      r1_.z(), r2_.z(), r3_.z()).finished());
}

/* ************************************************************************* */
pair<Matrix, Vector> RQ(const Matrix& A) {

	double x = -atan2(-A(2, 1), A(2, 2));
	Rot3 Qx = Rot3::Rx(-x);
	Matrix B = A * Qx.matrix();

	double y = -atan2(B(2, 0), B(2, 2));
	Rot3 Qy = Rot3::Ry(-y);
	Matrix C = B * Qy.matrix();

	double z = -atan2(-C(1, 0), C(1, 1));
	Rot3 Qz = Rot3::Rz(-z);
	Matrix R = C * Qz.matrix();

	Vector xyz = Vector_(3, x, y, z);
	return make_pair(R, xyz);
}

/* ************************************************************************* */

} // namespace gtsam

#endif
