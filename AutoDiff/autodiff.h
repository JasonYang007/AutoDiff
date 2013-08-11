#if !defined(INCLUDED_AUTODIFF_H)
#define INCLUDED_AUTODIFF_H

#include <iostream>
#include <functional>

namespace autodiff
{

// Class declarations
template<class T, class TExpr>
struct ADExpr;

struct ExpConst {};
struct ExpVar {};

template<class T>
struct ExpCore
{
	explicit ExpCore(T const val=T(), T const adj=T())
		: value(val), adjoint(adj) {}

	ExpCore& operator=(T const val) { value = val; return *this; }

	T & Value() { return value; }
	T const & Value() const { return value;}

	T & Adjoint() { return adjoint; }
	T const & Adjoint() const { return adjoint;}

	void SetAdjoint(T const adj) { adjoint = adj; }

private:
	T value;
	T adjoint;
};

template<class E, E Op>
struct op_traits;

//*** BINARY OPERATIONS
//--- ENUM & traits
enum AD_BINOP { AD_COMMA,
				AD_ASSIGN, 
				AD_PLUS, AD_MINUS, AD_MULT, AD_DIV 
			  };

namespace autdiff_binary
{
	template<class T>
	T & assign(T & left, T const & right)
	{
		return left = right;
	}
}

#define AD_DEFINE_BINOP_TRAITS(OPNAME,OP,STR) \
	template<> \
	struct op_traits<AD_BINOP, OPNAME>\
	{\
		template<class T>\
		static T call(T const x, T const y) { return OP(x,y); }\
		\
		static char const * str() { return STR; }\
	};

AD_DEFINE_BINOP_TRAITS(AD_PLUS, std::plus<T>(), "+")
AD_DEFINE_BINOP_TRAITS(AD_MINUS, std::minus<T>(), "-")
AD_DEFINE_BINOP_TRAITS(AD_MULT, std::multiplies<T>(), "*")
AD_DEFINE_BINOP_TRAITS(AD_DIV, std::divides<T>(), "/")
AD_DEFINE_BINOP_TRAITS(AD_COMMA, , ",")

template<> 
struct op_traits<AD_BINOP, AD_ASSIGN>
{
template<class T>
static T & call(T & x, T const y) { return x=y; }

static char const * str() { return "="; }
};

//--- Binary expression
template<class T, AD_BINOP BinOp, class E1, class E2>
struct ExpBinary;

template<class T, AD_BINOP BinOp, class E1, class E2>
struct ADExpr<T, ExpBinary<T, BinOp, E1, E2> >
	: public ExpBinary<T, BinOp, E1, E2>
{
	typedef ExpBinary<T, BinOp, E1, E2> base_t;
	ADExpr(E1 const & e1, E2 const & e2)
		: base_t(e1,e2) {}
};

template<class T, AD_BINOP BinOp, class E1, class E2>
struct ExpBinaryCore
	: ExpCore<T>
{
	typedef ExpCore<T> base_t;
	ExpBinaryCore(E1 const & e1, E2 const & e2)
		: base_t(), expr1(e1), expr2(e2) {}

	T EvaluateValue()
	{
		T val1 = expr1.EvaluateValue();
		T val2 = expr2.EvaluateValue();
		return this->Value() = op_traits<AD_BINOP,BinOp>::call(val1, val2);
	}

	E1 expr1;
	E2 expr2;
};

template<class T, class E1, class E2>
struct ExpBinaryCore<T,AD_ASSIGN,E1,E2>
	: ExpCore<T>
{
	typedef ExpCore<T> base_t;
	ExpBinaryCore(E1 const & e1, E2 const & e2)
		: base_t(), expr1(e1), expr2(e2) {}

	T EvaluateValue()
	{
		return this->Value() = op_traits<AD_BINOP,AD_ASSIGN>::call(expr1.Value(), expr2.EvaluateValue());
	}

	E1 expr1;
	E2 expr2;
};

template<class T, class E1, class E2>
struct ExpBinary<T, AD_COMMA, E1, E2>
	:  public ExpBinaryCore<T,AD_COMMA,E1,E2>
{
	typedef ExpBinaryCore<T,AD_COMMA,E1,E2>	base_t;

	ExpBinary(E1 const & e1, E2 const & e2)
		: base_t(e1,e2) {}

	//void EvaluateAdjoint(bool bIsTop=false)
	void EvaluateAdjoint()
	{
		//T adj = bIsTop ? T(1) : T(0);
		//this->expr2.SetAdjoint(adj);
		this->expr2.EvaluateAdjoint();

		//this->expr1.SetAdjoint(T(0));
		this->expr1.EvaluateAdjoint();	
	}
};

template<class T, class E1, class E2>
struct ExpBinary<T, AD_ASSIGN, E1, E2>
	:  public ExpBinaryCore<T,AD_ASSIGN,E1,E2>
{
	typedef ExpBinaryCore<T,AD_ASSIGN,E1,E2>	base_t;
	typedef ADExpr<T,ExpBinary> expr_t;

	ExpBinary(E1 const & e1, E2 const & e2)
		: base_t(e1,e2) {}

	void EvaluateAdjoint()
	{
		T adj = this->expr1.AdjointFetch();
		this->expr2.SetAdjoint(adj);
		this->expr2.EvaluateAdjoint();
	}
};

template<class T, class E1, class E2>
struct ExpBinary<T, AD_PLUS, E1, E2>
	:  public ExpBinaryCore<T,AD_PLUS,E1,E2>
{
	typedef ExpBinaryCore<T,AD_PLUS,E1,E2>	base_t;

	ExpBinary(E1 const & e1, E2 const & e2)
		: base_t(e1,e2) {}

	void EvaluateAdjoint()
	{
		T adj = this->Adjoint();
		this->expr1.SetAdjoint(adj);
		this->expr1.EvaluateAdjoint();

		this->expr2.SetAdjoint(adj);
		this->expr2.EvaluateAdjoint();
	}
};

template<class T, class E1, class E2>
struct ExpBinary<T, AD_MINUS, E1, E2>
	:  public ExpBinaryCore<T,AD_MINUS,E1,E2>
{
	typedef ExpBinaryCore<T,AD_MINUS,E1,E2>	base_t;

	ExpBinary(E1 const & e1, E2 const & e2)
		: base_t(e1,e2) {}

	void EvaluateAdjoint()
	{
		T adj = this->Adjoint();
		this->expr1.SetAdjoint(adj);
		this->expr1.EvaluateAdjoint();

		this->expr2.SetAdjoint(-adj);
		this->expr2.EvaluateAdjoint();
	}
};

template<class T, class E1, class E2>
struct ExpBinary<T, AD_MULT, E1, E2>
	:  public ExpBinaryCore<T, AD_MULT, E1, E2>
{
	typedef ExpBinaryCore<T,AD_MULT,E1,E2>	base_t;

	ExpBinary(E1 const & e1, E2 const & e2)
		: base_t(e1,e2) {}

	void EvaluateAdjoint()
	{
		T adj = this->Adjoint();
		T x = this->expr1.Value();
		T y = this->expr2.Value();
		this->expr1.SetAdjoint(adj * y);
		this->expr1.EvaluateAdjoint();

		this->expr2.SetAdjoint(adj * x);
		this->expr2.EvaluateAdjoint();
	}
};

template<class T, class E1, class E2>
struct ExpBinary<T, AD_DIV, E1, E2>
	:  public ExpBinaryCore<T, AD_DIV, E1, E2>
{
	typedef ExpBinaryCore<T,AD_DIV,E1,E2>	base_t;

	ExpBinary(E1 const & e1, E2 const & e2)
		: base_t(e1,e2) {}

	void EvaluateAdjoint()
	{
		T adj = this->Adjoint();
		T x = this->expr1.Value();
		T y = this->expr2.Value();
		this->expr1.SetAdjoint(adj / y);
		this->expr1.EvaluateAdjoint();

		this->expr2.SetAdjoint(adj * (-x/(y*y)));
		this->expr2.EvaluateAdjoint();
	}
};

template<class T, AD_BINOP BinOp, class E1, class E2>
std::ostream & operator<<(std::ostream & ostr, ExpBinary<T, BinOp, E1, E2> const & expr)
{
	return ostr << '[' << op_traits<AD_BINOP,BinOp>::str() << ':' << expr.Value() << "]("
		   		<< expr.expr1 << ',' << expr.expr2 << ')';
}

// ADExpr: expression templates
//----	Const expression
template<class T>
struct ADExpr<T, ExpConst>
	: public ExpCore<T>
{
	typedef ExpCore<T> base_t;

	ADExpr(T const val)
		: base_t(val, T())
	{}
	
	T EvaluateValue() { return this->Value(); }
	void EvaluateAdjoint() {}
};

template<class T>
std::ostream & operator<<(std::ostream & ostr, ADExpr<T,ExpConst> const & expr)
{
	return ostr << expr.Value();
}

//----	Variable expression
template<class T>
struct ADExpr<T,ExpVar>
{
	ADExpr(T * v_ptr, T *a_ptr)
		: adjoint(), val_ptr(v_ptr), adj_ptr(a_ptr)
	{}

	T & Value() { return *val_ptr; }
	T const & Value() const { return *val_ptr;}

	T & Adjoint() { return adjoint; }
	T const & Adjoint() const { return adjoint;}

	void SetAdjoint(T const adj) { adjoint = adj; }

	T EvaluateValue()
	{
		return Value();
	}

	void EvaluateAdjoint()
	{
		*adj_ptr += adjoint;
	}

	T & AdjointFetch()
	{
		return adjoint = *adj_ptr;
	}

	T const & AdjointFetch() const
	{
		return adjoint = *adj_ptr;
	}
	

protected:
	ADExpr()
		: adjoint(), val_ptr(0L), adj_ptr(0L)
	{}

	void set_pointer(T * v_ptr, T * a_ptr)
	{
		val_ptr = v_ptr;
		adj_ptr = a_ptr;
	}
private:
	T  adjoint;
	T * val_ptr;
	T * adj_ptr;
};


template<class T>
std::ostream & operator<<(std::ostream & ostr, ADExpr<T,ExpVar> const & expr)
{
	return ostr << '(' << expr.Value() << ',' << expr.Adjoint() << ')';
}

// ADVar
#define ADVAR_DEFINE_ASSIGN_OP(OP) \
	ADVar & operator OP(T const val)\
	{\
		base_data_t::Value() OP val;\
		return *this;\
	}

#define ADVAR_DEFINE_ASSIGN()\
	ADVAR_DEFINE_ASSIGN_OP(=)\
	ADVAR_DEFINE_ASSIGN_OP(+=)\
	ADVAR_DEFINE_ASSIGN_OP(-=)\
	ADVAR_DEFINE_ASSIGN_OP(*=)\
	ADVAR_DEFINE_ASSIGN_OP(/=)

template<class T>
struct ADVar
	: ADExpr<T,ExpVar>, ExpCore<T>
{
	typedef ADExpr<T,ExpVar> base_t;
	typedef ExpCore<T>	 base_data_t;

	using base_data_t::Value;
	using base_data_t::Adjoint;

	ADVar(T const val = T())
		: base_t()
		, base_data_t(val,T())
	{
		base_t::set_pointer(&base_data_t::Value(), &base_data_t::Adjoint());
	}

	ADVAR_DEFINE_ASSIGN()

	void SetAdjoint(T const adj)
	{
		base_t::Adjoint()=base_data_t::Adjoint()=adj;
	}

	void SetAsRoot()
	{
		this->SetAdjoint(T(1));
	}

	template<class E>
	ADExpr<T,ExpBinary<T,AD_ASSIGN,base_t,ADExpr<T,E> > > 
	operator=(ADExpr<T,E> const & other)
	{
		return ADExpr<T,ExpBinary<T,AD_ASSIGN,base_t,ADExpr<T,E> > >(*this, other);
	}
};

template<class T>
std::ostream & operator<<(std::ostream & ostr, ADVar<T> const & expr)
{
	return ostr << '(' << expr.Value() << ',' << expr.Adjoint() << ')';
}

//*** Binary operations

#define AD_DEFINE_BINARY_OP(OPNAME, BINFUNC)\
	template<class T, class E1, class E2>\
	ADExpr<T,ExpBinary<T,OPNAME,ADExpr<T,E1>,ADExpr<T,E2> > > \
		BINFUNC(ADExpr<T,E1> const & e1, ADExpr<T,E2> const & e2)\
	{\
		return ADExpr<T,ExpBinary<T,OPNAME,ADExpr<T,E1>,ADExpr<T,E2> > >(e1, e2);\
	}

AD_DEFINE_BINARY_OP(AD_PLUS, operator+)
AD_DEFINE_BINARY_OP(AD_MINUS, operator-)
AD_DEFINE_BINARY_OP(AD_MULT, operator*)
AD_DEFINE_BINARY_OP(AD_DIV, operator/)

template<class T, class E1, class E2>
ADExpr<T,ExpBinary<T,AD_COMMA,ADExpr<T,E1>,ADExpr<T,E2> > > 
	operator ,(ADExpr<T,E1> const & e1, ADExpr<T,E2> const & e2)
{
	return ADExpr<T,ExpBinary<T,AD_COMMA,ADExpr<T,E1>,ADExpr<T,E2> > >(e1, e2);
}


} /* end of namespace: autodiff */


#endif /* INCLUDED_AUTODIFF_H */
