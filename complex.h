//   complex.h - declaration of class
//   of complex number
//
//   The code is property of LIBROW
//   You can use it on your own
//   When utilizing credit LIBROW site

#ifndef _COMPLEX_H_
#define _COMPLEX_H_

class complex
{
protected:
	//   Internal presentation - real and imaginary parts
	double m_re;
	double m_im;

public:
	//   Imaginary unity
	static const complex i;
	static const complex j;

	//   Constructors
	complex(): m_re(0.), m_im(0.) {}
	complex(double re, double im): m_re(re), m_im(im) {}
	complex(double val): m_re(val), m_im(0.) {}

	//   Assignment
	complex& operator= (const double val)
	{
		m_re = val;
		m_im = 0.;
		return *this;
	}

	//   Basic operations - taking parts
	double re() const { return m_re; }
	double im() const { return m_im; }

	//   Conjugate number
	complex conjugate() const
	{
		return complex(m_re, -m_im);
	}

	//   Norm   
	double norm() const
	{
		return m_re * m_re + m_im * m_im;
	}

	//   Arithmetic operations
	complex operator+ (const complex& other) const
	{
		return complex(m_re + other.m_re, m_im + other.m_im);
	}

	complex operator- (const complex& other) const
	{
		return complex(m_re - other.m_re, m_im - other.m_im);
	}

	complex operator* (const complex& other) const
	{
		return complex(m_re * other.m_re - m_im * other.m_im,
			m_re * other.m_im + m_im * other.m_re);
	}

	complex operator/ (const complex& other) const
	{
		const double denominator = other.m_re * other.m_re + other.m_im * other.m_im;
		return complex((m_re * other.m_re + m_im * other.m_im) / denominator,
			(m_im * other.m_re - m_re * other.m_im) / denominator);
	}

	complex& operator+= (const complex& other)
	{
		m_re += other.m_re;
		m_im += other.m_im;
		return *this;
	}

	complex& operator-= (const complex& other)
	{
		m_re -= other.m_re;
		m_im -= other.m_im;
		return *this;
	}

	complex& operator*= (const complex& other)
	{
		const double temp = m_re;
		m_re = m_re * other.m_re - m_im * other.m_im;
		m_im = m_im * other.m_re + temp * other.m_im;
		return *this;
	}

	complex& operator/= (const complex& other)
	{
		const double denominator = other.m_re * other.m_re + other.m_im * other.m_im;
		const double temp = m_re;
		m_re = (m_re * other.m_re + m_im * other.m_im) / denominator;
		m_im = (m_im * other.m_re - temp * other.m_im) / denominator;
		return *this;
	}

	complex& operator++ ()
	{
		++m_re;
		return *this;
	}

	complex operator++ (int)
	{
		complex temp(*this);
		++m_re;
		return temp;
	}

	complex& operator-- ()
	{
		--m_re;
		return *this;
	}

	complex operator-- (int)
	{
		complex temp(*this);
		--m_re;
		return temp;
	}

	complex operator+ (const double val) const
	{
		return complex(m_re + val, m_im);
	}

	complex operator- (const double val) const
	{
		return complex(m_re - val, m_im);
	}

	complex operator* (const double val) const
	{
		return complex(m_re * val, m_im * val);
	}

	complex operator/ (const double val) const
	{
		return complex(m_re / val, m_im / val);
	}

	complex& operator+= (const double val)
	{
		m_re += val;
		return *this;
	}

	complex& operator-= (const double val)
	{
		m_re -= val;
		return *this;
	}

	complex& operator*= (const double val)
	{
		m_re *= val;
		m_im *= val;
		return *this;
	}

	complex& operator/= (const double val)
	{
		m_re /= val;
		m_im /= val;
		return *this;
	}

	friend complex operator+ (const double left, const complex& right)
	{
		return complex(left + right.m_re, right.m_im);
	}

	friend complex operator- (const double left, const complex& right)
	{
		return complex(left - right.m_re, -right.m_im);
	}

	friend complex operator* (const double left, const complex& right)
	{
		return complex(left * right.m_re, left * right.m_im);
	}

	friend complex operator/ (const double left, const complex& right)
	{
		const double denominator = right.m_re * right.m_re + right.m_im * right.m_im;
		return complex(left * right.m_re / denominator,
			-left * right.m_im / denominator);
	}

	//   Boolean operators
	bool operator== (const complex &other) const
	{
		return m_re == other.m_re && m_im == other.m_im;
	}

	bool operator!= (const complex &other) const
	{
		return m_re != other.m_re || m_im != other.m_im;
	}

	bool operator== (const double val) const
	{
		return m_re == val && m_im == 0.;
	}

	bool operator!= (const double val) const
	{
		return m_re != val || m_im != 0.;
	}

	friend bool operator== (const double left, const complex& right)
	{
		return left == right.m_re && right.m_im == 0.;
	}

	friend bool operator!= (const double left, const complex& right)
	{
		return left != right.m_re || right.m_im != 0.;
	}
};

#endif